/*
  SPDX-FileCopyrightText: 2010 Casey Link <unnamedrambler@gmail.com>
  SPDX-FileCopyrightText: 2009-2010 Klaralvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>

namespace CalendarSupport
{
class FreeBusyItemModelTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testModelValidity();
    void testModelValidity2();
    void testInsertFreeBusy();
};
}
