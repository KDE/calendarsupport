/*
  SPDX-FileCopyrightText: 2004 David Faure <faure@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/

#pragma once

#include "calendarsupport_export.h"
#include <KIdentityManagementCore/IdentityManager>

namespace CalendarSupport
{
/*!
 * \class CalendarSupport::IdentityManager
 * \inmodule CalendarSupport
 * \inheaderfile CalendarSupport/IdentityManager
 *
 * \brief The IdentityManager class
 */
class CALENDARSUPPORT_EXPORT IdentityManager : public KIdentityManagementCore::IdentityManager
{
    Q_OBJECT
public:
    /*!
     * Constructs an IdentityManager.
     * \param parent The parent QObject.
     * \param name The name of the manager.
     */
    explicit IdentityManager(QObject *parent = nullptr, const char *name = nullptr)
        : KIdentityManagementCore::IdentityManager(true /*readonly*/, parent, name)
    {
    }

protected:
    /*!
     * Creates a default identity with the given full name and email address.
     * \param fullName The full name for the default identity.
     * \param emailAddress The email address for the default identity.
     */
    void createDefaultIdentity(QString &fullName, QString &emailAddress) override;
};

KIdentityManagementCore::IdentityManager *identityManager();
}
