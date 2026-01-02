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
 */
class CALENDARSUPPORT_EXPORT IdentityManager : public KIdentityManagementCore::IdentityManager
{
    Q_OBJECT
public:
    /*!
     * \brief IdentityManager
     */
    explicit IdentityManager(QObject *parent = nullptr, const char *name = nullptr)
        : KIdentityManagementCore::IdentityManager(true /*readonly*/, parent, name)
    {
    }

protected:
    void createDefaultIdentity(QString &fullName, QString &emailAddress) override;
};

KIdentityManagementCore::IdentityManager *identityManager();
}
