/*
  SPDX-FileCopyrightText: 2004 David Faure <faure@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#pragma once

#include "calendarsupport_export.h"
#include <KIdentityManagement/IdentityManager>

namespace CalendarSupport
{
class CALENDARSUPPORT_EXPORT IdentityManager : public KIdentityManagement::IdentityManager
{
    Q_OBJECT
public:
    explicit IdentityManager(QObject *parent = nullptr, const char *name = nullptr)
        : KIdentityManagement::IdentityManager(true /*readonly*/, parent, name)
    {
    }

protected:
    void createDefaultIdentity(QString &fullName, QString &emailAddress) override;
};

KIdentityManagement::IdentityManager *identityManager();
}

