/*
 * SPDX-FileCopyrightText: 2014 Sandro Knauß <knauss@kolabsys.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 *
 */

#pragma once

#include "calendarsupport_export.h"

#include "freebusyitemmodel.h"

#include <KCalendarCore/Calendar>
#include <KCalendarCore/Event>

namespace CalendarSupport
{
class FreeBusyCalendarPrivate;
class CALENDARSUPPORT_EXPORT FreeBusyCalendar : public QObject
{
    Q_OBJECT
public:
    explicit FreeBusyCalendar(QObject *parent = nullptr);

    ~FreeBusyCalendar() override;

    void setModel(FreeBusyItemModel *model);
    FreeBusyItemModel *model() const;
    KCalendarCore::Calendar::Ptr calendar() const;

private:
    void onRowsChanged(const QModelIndex &, const QModelIndex &);
    void onRowsInserted(const QModelIndex &, int, int);
    void onRowsRemoved(const QModelIndex &, int, int);
    void onLayoutChanged();
    void deleteAllEvents();
    FreeBusyCalendarPrivate *const d;
};
}
