/*
  SPDX-FileCopyrightText: 2001, 2003 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#include "kcalprefs.h"
#include "calendarsupport_debug.h"
#include "identitymanager.h"

#include <Akonadi/TagAttribute>
#include <Akonadi/TagCache>
#include <Akonadi/TagModifyJob>

#include <KMime/HeaderParsing>

#include <KEmailAddress>
#include <KIdentityManagement/Identity>

#include <KEMailSettings>

using namespace CalendarSupport;

Q_GLOBAL_STATIC(KCalPrefs, globalPrefs)

class CalendarSupport::KCalPrefsPrivate
{
public:
    KCalPrefsPrivate()
        : mDefaultCalendarId(-1)
        , mDefaultCategoryColor(QColor(151, 235, 121))
    {
    }

    ~KCalPrefsPrivate() = default;

    Akonadi::Collection::Id mDefaultCalendarId;

    const QColor mDefaultCategoryColor;
    QDateTime mDayBegins;
};

KCalPrefs::KCalPrefs()
    : KCalPrefsBase()
    , d(new KCalPrefsPrivate())
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

    KEMailSettings settings;
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

Akonadi::Collection::Id KCalPrefs::defaultCalendarId() const
{
    return d->mDefaultCalendarId;
}

void KCalPrefs::setDefaultCalendarId(Akonadi::Collection::Id id)
{
    d->mDefaultCalendarId = id;
}

void KCalPrefs::fillMailDefaults()
{
    userEmailItem()->swapDefault();
    QString defEmail = userEmailItem()->value();
    userEmailItem()->swapDefault();

    if (userEmail() == defEmail) {
        // No korg settings - but maybe there's a kcontrol[/kmail] setting available
        KEMailSettings settings;
        if (!settings.getSetting(KEMailSettings::EmailAddress).isEmpty()) {
            mEmailControlCenter = true;
        }
    }
}

void KCalPrefs::usrRead()
{
    KConfigGroup generalConfig(config(), "General");

    KConfigGroup defaultCalendarConfig(config(), "Calendar");
    d->mDefaultCalendarId = defaultCalendarConfig.readEntry("Default Calendar", -1);

#if 0
    config()->setGroup("FreeBusy");
    if (mRememberRetrievePw) {
        d->mRetrievePassword
            = KStringHandler::obscure(config()->readEntry("Retrieve Server Password"));
    }
#endif

    KConfigSkeleton::usrRead();
    fillMailDefaults();
}

bool KCalPrefs::usrSave()
{
    KConfigGroup generalConfig(config(), "General");

#if 0
    if (mRememberRetrievePw) {
        config()->writeEntry("Retrieve Server Password",
                             KStringHandler::obscure(d->mRetrievePassword));
    } else {
        config()->deleteEntry("Retrieve Server Password");
    }
#endif

    KConfigGroup defaultCalendarConfig(config(), "Calendar");
    defaultCalendarConfig.writeEntry("Default Calendar", defaultCalendarId());

    return KConfigSkeleton::usrSave();
}

QString KCalPrefs::fullName()
{
    QString tusername;
    if (mEmailControlCenter) {
        KEMailSettings settings;
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
        KEMailSettings settings;
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
    KIdentityManagement::IdentityManager *idmanager = CalendarSupport::identityManager();
    QStringList lst = idmanager->identities();

    fullEmails.reserve(1 + mAdditionalMails.count() + lst.count());
    // The user name and email from the config dialog:
    fullEmails << QStringLiteral("%1 <%2>").arg(fullName(), email());

    QStringList::Iterator it;
    KIdentityManagement::IdentityManager::ConstIterator it1;
    KIdentityManagement::IdentityManager::ConstIterator end1(idmanager->end());
    for (it1 = idmanager->begin(); it1 != end1; ++it1) {
        fullEmails << (*it1).fullEmailAddr();
    }
    // Add emails configured in korganizer
    lst = mAdditionalMails;
    for (it = lst.begin(); it != lst.end(); ++it) {
        fullEmails << QStringLiteral("%1 <%2>").arg(fullName(), *it);
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
    const QString email = mbox.addrSpec().asString();

    if (this->email() == email) {
        return true;
    }

    CalendarSupport::IdentityManager::ConstIterator it;
    CalendarSupport::IdentityManager::ConstIterator endId(CalendarSupport::identityManager()->end());
    for (it = CalendarSupport::identityManager()->begin(); it != endId; ++it) {
        if ((*it).matchesEmailAddress(email)) {
            return true;
        }
    }

    if (mAdditionalMails.contains(email)) {
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
