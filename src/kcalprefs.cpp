/*
  SPDX-FileCopyrightText: 2001, 2003 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/

#include "kcalprefs.h"
using namespace Qt::Literals::StringLiterals;

#include "calendarsupport_debug.h"
#include "identitymanager.h"

#include <Akonadi/TagAttribute>
#include <Akonadi/TagCache>
#include <Akonadi/TagModifyJob>

#include <KMime/HeaderParsing>

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

    Akonadi::Collection::Id mDefaultEventCalendarId{-1};
    Akonadi::Collection::Id mDefaultTodoCalendarId{-1};

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

Akonadi::Collection::Id KCalPrefs::defaultEventCalendarId() const
{
    return d->mDefaultEventCalendarId;
}

void KCalPrefs::setDefaultEventCalendarId(Akonadi::Collection::Id id)
{
    d->mDefaultEventCalendarId = id;
}

Akonadi::Collection::Id KCalPrefs::defaultTodoCalendarId() const
{
    return d->mDefaultTodoCalendarId;
}

void KCalPrefs::setDefaultTodoCalendarId(Akonadi::Collection::Id id)
{
    d->mDefaultTodoCalendarId = id;
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

void KCalPrefs::usrRead()
{
    KConfigGroup const generalConfig(config(), u"General"_s);

    KConfigGroup const defaultCalendarConfig(config(), u"Calendar"_s);
    d->mDefaultEventCalendarId = defaultCalendarConfig.readEntry("Default Event Calendar", -1);
    // fallback to the old setting
    if (d->mDefaultEventCalendarId == -1) {
        d->mDefaultEventCalendarId = defaultCalendarConfig.readEntry("Default Calendar", -1);
    }
    d->mDefaultTodoCalendarId = defaultCalendarConfig.readEntry("Default Todo Calendar", -1);

    KConfigSkeleton::usrRead();
    fillMailDefaults();
}

bool KCalPrefs::usrSave()
{
    KConfigGroup const generalConfig(config(), u"General"_s);

    KConfigGroup defaultCalendarConfig(config(), u"Calendar"_s);
    defaultCalendarConfig.writeEntry("Default Event Calendar", defaultEventCalendarId());
    defaultCalendarConfig.writeEntry("Default Todo Calendar", defaultTodoCalendarId());

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

void KCalPrefs::setDayBegins(const QDateTime &dateTime)
{
    d->mDayBegins = dateTime;
}

QDateTime KCalPrefs::dayBegins() const
{
    return d->mDayBegins;
}

#include "moc_kcalprefs.cpp"
