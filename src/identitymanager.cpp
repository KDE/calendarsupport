/*
  SPDX-FileCopyrightText: 2004 David Faure <faure@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#include "identitymanager.h"
#include "kcalprefs.h"

using namespace CalendarSupport;

// This is called to create a default identity in case emailidentities has none
// (i.e. the user never used KMail before)
// We provide the values from KCalPrefs, since those are configurable in korganizer.
void IdentityManager::createDefaultIdentity(QString &fullName, QString &emailAddress)
{
    fullName = KCalPrefs::instance()->fullName();
    emailAddress = KCalPrefs::instance()->email();
}

Q_GLOBAL_STATIC(CalendarSupport::IdentityManager, globalIdentityManager)

KIdentityManagement::IdentityManager *CalendarSupport::identityManager()
{
    return globalIdentityManager;
}
