/*
  SPDX-FileCopyrightText: 2001, 2003 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/

#include "kcalprefs.h"
using namespace Qt::Literals::StringLiterals;

#include "calendarsupport_debug.h"
#include "identitymanager.h"

#include <Akonadi/EntityTreeModel>
#include <Akonadi/TagAttribute>
#include <Akonadi/TagCache>
#include <Akonadi/TagModifyJob>

#include <KMime/HeaderParsing>

#include <QAbstractItemModel>
#include <QPointer>

#include <KEmailAddress>
#include <KIdentityManagementCore/Identity>

#include <KEMailSettings>

using namespace CalendarSupport;

Q_GLOBAL_STATIC(KCalPrefs, globalPrefs)

class CalendarSupport::KCalPrefsPrivate
{
public:
    KCalPrefsPrivate()
        : mDefaultCategoryColor(QColor(151, 235, 121))
    {
    }

    ~KCalPrefsPrivate() = default;

    // The default calendars are persisted by stable remote path; these ids are the in-memory
    // result of resolving the paths against mCollectionModel (or a legacy id awaiting migration).
    QString mDefaultEventCalendarPath;
    QString mDefaultTodoCalendarPath;
    Akonadi::Collection::Id mDefaultEventCalendarId{-1};
    Akonadi::Collection::Id mDefaultTodoCalendarId{-1};
    QPointer<QAbstractItemModel> mCollectionModel;

    const QColor mDefaultCategoryColor;
    QDateTime mDayBegins;
};

KCalPrefs::KCalPrefs()
    : d(new KCalPrefsPrivate())
{
    Akonadi::TagCache::instance();
}

KCalPrefs::~KCalPrefs() = default;

KCalPrefs *KCalPrefs::instance()
{
    static bool firstCall = true;

    if (firstCall) {
        firstCall = false;
        globalPrefs->load();
    }

    return globalPrefs;
}

void KCalPrefs::setPrefsDefaults()
{
    KCalPrefs::usrSetDefaults();
}

void KCalPrefs::usrSetDefaults()
{
    // Default should be set a bit smarter, respecting username and locale
    // settings for example.

    KEMailSettings const settings;
    QString tmp = settings.getSetting(KEMailSettings::RealName);
    if (!tmp.isEmpty()) {
        setUserName(tmp);
    }
    tmp = settings.getSetting(KEMailSettings::EmailAddress);
    if (!tmp.isEmpty()) {
        setUserEmail(tmp);
    }
    fillMailDefaults();

    KConfigSkeleton::usrSetDefaults();
}

// Resolves a stored stable path to a live collection id in `model`, or -1 if not (yet) present.
static Akonadi::Collection::Id resolveStableKeyToId(const QAbstractItemModel *model, const QString &path)
{
    if (!model || path.isEmpty()) {
        return -1;
    }
    const QModelIndex idx = Akonadi::EntityTreeModel::modelIndexForStableKey(model, path);
    if (!idx.isValid()) {
        return -1;
    }
    return idx.data(Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>().id();
}

// Computes the stable path of the collection with id `id` in `model`, or an empty string.
static QString computeStableKeyForId(const QAbstractItemModel *model, Akonadi::Collection::Id id)
{
    if (!model || id < 0) {
        return QString();
    }
    const QModelIndex idx = Akonadi::EntityTreeModel::modelIndexForCollection(model, Akonadi::Collection(id));
    if (!idx.isValid()) {
        return QString();
    }
    return Akonadi::EntityTreeModel::stableKeyForCollectionIndex(idx);
}

Akonadi::Collection::Id KCalPrefs::defaultEventCalendarId() const
{
    if (d->mDefaultEventCalendarId < 0 && d->mCollectionModel) {
        d->mDefaultEventCalendarId = resolveStableKeyToId(d->mCollectionModel, d->mDefaultEventCalendarPath);
    }
    return d->mDefaultEventCalendarId;
}

void KCalPrefs::setDefaultEventCalendarId(Akonadi::Collection::Id id)
{
    d->mDefaultEventCalendarId = id;
    d->mDefaultEventCalendarPath = computeStableKeyForId(d->mCollectionModel, id);
}

Akonadi::Collection::Id KCalPrefs::defaultTodoCalendarId() const
{
    if (d->mDefaultTodoCalendarId < 0 && d->mCollectionModel) {
        d->mDefaultTodoCalendarId = resolveStableKeyToId(d->mCollectionModel, d->mDefaultTodoCalendarPath);
    }
    return d->mDefaultTodoCalendarId;
}

void KCalPrefs::setDefaultTodoCalendarId(Akonadi::Collection::Id id)
{
    d->mDefaultTodoCalendarId = id;
    d->mDefaultTodoCalendarPath = computeStableKeyForId(d->mCollectionModel, id);
}

QString KCalPrefs::defaultEventCalendarPath() const
{
    return d->mDefaultEventCalendarPath;
}

QString KCalPrefs::defaultTodoCalendarPath() const
{
    return d->mDefaultTodoCalendarPath;
}

void KCalPrefs::setCollectionModel(QAbstractItemModel *model)
{
    if (d->mCollectionModel == model) {
        return;
    }
    if (d->mCollectionModel) {
        disconnect(d->mCollectionModel, &QAbstractItemModel::rowsInserted, this, nullptr);
    }
    d->mCollectionModel = model;
    if (model) {
        connect(model, &QAbstractItemModel::rowsInserted, this, [this]() {
            resolveDefaultCalendars();
        });
        resolveDefaultCalendars(); // the model may already be populated
    }
}

void KCalPrefs::resolveDefaultCalendars()
{
    if (!d->mCollectionModel) {
        return;
    }

    bool migrated = false;
    const auto resolveOne = [&](QString &path, Akonadi::Collection::Id &id) {
        if (!path.isEmpty()) {
            // The path is authoritative; (re)resolve it to the current id.
            const Akonadi::Collection::Id resolvedId = resolveStableKeyToId(d->mCollectionModel, path);
            if (resolvedId >= 0) {
                id = resolvedId;
            }
        } else if (id >= 0) {
            // Migration: a legacy numeric id but no path yet. Capture the path while the id is
            // still valid (usrSave() then drops the legacy id key).
            const QString newPath = computeStableKeyForId(d->mCollectionModel, id);
            if (!newPath.isEmpty()) {
                path = newPath;
                migrated = true;
            }
        }
    };
    resolveOne(d->mDefaultEventCalendarPath, d->mDefaultEventCalendarId);
    resolveOne(d->mDefaultTodoCalendarPath, d->mDefaultTodoCalendarId);

    if (migrated) {
        savePrefs();
    }

    // Once both defaults have settled, stop reacting to further model changes.
    const auto settled = [](const QString &path, Akonadi::Collection::Id id) {
        return path.isEmpty() ? (id < 0) : (id >= 0);
    };
    if (settled(d->mDefaultEventCalendarPath, d->mDefaultEventCalendarId) && settled(d->mDefaultTodoCalendarPath, d->mDefaultTodoCalendarId)) {
        disconnect(d->mCollectionModel, &QAbstractItemModel::rowsInserted, this, nullptr);
    }
}
void KCalPrefs::fillMailDefaults()
{
    userEmailItem()->swapDefault();
    QString const defEmail = userEmailItem()->value();
    userEmailItem()->swapDefault();

    if (userEmail() == defEmail) {
        // No korg settings - but maybe there's a kcontrol[/kmail] setting available
        KEMailSettings const settings;
        if (!settings.getSetting(KEMailSettings::EmailAddress).isEmpty()) {
            mEmailControlCenter = true;
        }
    }
}

void KCalPrefs::readPrefs()
{
    usrRead();
}

void KCalPrefs::usrRead()
{
    KConfigGroup const generalConfig(config(), u"General"_s);

    KConfigGroup const defaultCalendarConfig(config(), u"Calendar"_s);
    // The default calendars are stored by stable remote path. When a path is present it is
    // authoritative and the id is left invalid until resolved from it (a bare id can silently
    // point at a different collection after a resource re-list or a database move). A leftover
    // numeric id is only read when no path exists yet, as input for the one-time migration.
    d->mDefaultEventCalendarPath = defaultCalendarConfig.readEntry("Default Event Calendar Path", QString());
    if (d->mDefaultEventCalendarPath.isEmpty()) {
        d->mDefaultEventCalendarId = defaultCalendarConfig.readEntry("Default Event Calendar", -1);
        if (d->mDefaultEventCalendarId == -1) {
            d->mDefaultEventCalendarId = defaultCalendarConfig.readEntry("Default Calendar", -1); // oldest key
        }
    } else {
        d->mDefaultEventCalendarId = -1;
    }
    d->mDefaultTodoCalendarPath = defaultCalendarConfig.readEntry("Default Todo Calendar Path", QString());
    if (d->mDefaultTodoCalendarPath.isEmpty()) {
        d->mDefaultTodoCalendarId = defaultCalendarConfig.readEntry("Default Todo Calendar", -1);
    } else {
        d->mDefaultTodoCalendarId = -1;
    }

    KConfigSkeleton::usrRead();
    fillMailDefaults();
}

bool KCalPrefs::savePrefs()
{
    return usrSave();
}

bool KCalPrefs::usrSave()
{
    KConfigGroup const generalConfig(config(), u"General"_s);

    KConfigGroup defaultCalendarConfig(config(), u"Calendar"_s);
    // Persist the stable path and drop the legacy numeric id once we have one. If no path could be
    // determined yet (no model was set this run), keep the legacy id rather than lose the setting.
    const auto saveOne = [&](const char *pathKey, const char *idKey, const QString &path, Akonadi::Collection::Id id) {
        if (!path.isEmpty()) {
            defaultCalendarConfig.writeEntry(pathKey, path);
            defaultCalendarConfig.deleteEntry(idKey);
        } else {
            defaultCalendarConfig.deleteEntry(pathKey);
            if (id >= 0) {
                defaultCalendarConfig.writeEntry(idKey, id);
            } else {
                defaultCalendarConfig.deleteEntry(idKey);
            }
        }
    };
    saveOne("Default Event Calendar Path", "Default Event Calendar", d->mDefaultEventCalendarPath, d->mDefaultEventCalendarId);
    saveOne("Default Todo Calendar Path", "Default Todo Calendar", d->mDefaultTodoCalendarPath, d->mDefaultTodoCalendarId);
    if (!d->mDefaultEventCalendarPath.isEmpty()) {
        defaultCalendarConfig.deleteEntry("Default Calendar"); // oldest legacy alias
    }

    return KConfigSkeleton::usrSave();
}

QString KCalPrefs::fullName()
{
    QString tusername;
    if (mEmailControlCenter) {
        KEMailSettings const settings;
        tusername = settings.getSetting(KEMailSettings::RealName);
    } else {
        tusername = userName();
    }

    // Quote the username as it might contain commas and other quotable chars.
    tusername = KEmailAddress::quoteNameIfNecessary(tusername);

    QString tname;
    QString temail;
    // ignore the return value from extractEmailAddressAndName() because
    // it will always be false since tusername does not contain "@domain".
    KEmailAddress::extractEmailAddressAndName(tusername, temail, tname);
    return tname;
}

QString KCalPrefs::email()
{
    if (mEmailControlCenter) {
        KEMailSettings const settings;
        return settings.getSetting(KEMailSettings::EmailAddress);
    } else {
        return userEmail();
    }
}

QStringList KCalPrefs::allEmails()
{
    // Grab emails from the email identities
    QStringList lst = CalendarSupport::identityManager()->allEmails();
    // Add emails configured in korganizer
    lst += mAdditionalMails;
    // Add the email entered as the userEmail here
    lst += email();

    // Warning, this list could contain duplicates.
    return lst;
}

QStringList KCalPrefs::fullEmails()
{
    QStringList fullEmails;

    // Grab emails from the email identities
    KIdentityManagementCore::IdentityManager const *idmanager = CalendarSupport::identityManager();
    QStringList lst = idmanager->identities();

    fullEmails.reserve(1 + mAdditionalMails.count() + lst.count());
    // The user name and email from the config dialog:
    fullEmails << u"%1 <%2>"_s.arg(fullName(), email());

    QStringList::Iterator it;
    KIdentityManagementCore::IdentityManager::ConstIterator it1;
    KIdentityManagementCore::IdentityManager::ConstIterator const end1(idmanager->end());
    for (it1 = idmanager->begin(); it1 != end1; ++it1) {
        fullEmails << (*it1).fullEmailAddr();
    }
    // Add emails configured in korganizer
    lst = mAdditionalMails;
    for (it = lst.begin(); it != lst.end(); ++it) {
        fullEmails << u"%1 <%2>"_s.arg(fullName(), *it);
    }

    // Warning, this list could contain duplicates.
    return fullEmails;
}

bool KCalPrefs::thatIsMe(const QString &_email)
{
    // NOTE: this method is called for every created agenda view item,
    // so we need to keep performance in mind

    /* identityManager()->thatIsMe() is quite expensive since it does parsing of
       _email in a way which is unnecessarily complex for what we can have here,
       so we do that ourselves. This makes sense since this

    if ( Akonadi::identityManager()->thatIsMe( _email ) ) {
      return true;
    }
    */

    // in case email contains a full name, strip it out.
    // the below is the simpler but slower version of the following code:
    // const QString email = CalendarSupport::getEmailAddress( _email );
    const QByteArray tmp = _email.toUtf8();
    const char *cursor = tmp.constData();
    const char *end = tmp.data() + tmp.length();
    KMime::Types::Mailbox mbox;
    KMime::HeaderParsing::parseMailbox(cursor, end, mbox);
    const QString mboxEmail = mbox.addrSpec().asString();
    if (this->email() == mboxEmail) {
        return true;
    }

    CalendarSupport::IdentityManager::ConstIterator it;
    CalendarSupport::IdentityManager::ConstIterator const endId(CalendarSupport::identityManager()->end());
    for (it = CalendarSupport::identityManager()->begin(); it != endId; ++it) {
        if ((*it).matchesEmailAddress(mboxEmail)) {
            return true;
        }
    }

    if (mAdditionalMails.contains(mboxEmail)) {
        return true;
    }

    return false;
}

#include "moc_kcalprefs.cpp"
