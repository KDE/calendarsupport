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

#include <memory>

namespace CalendarSupport
{
class FreeBusyCalendarPrivate;

/// A FreeBusyCalendar exposes a FreeBusyItemModel as a KCalendarCore::Calendar::Ptr.
class CALENDARSUPPORT_EXPORT FreeBusyCalendar : public QObject
{
    Q_OBJECT
public:
    /// Constructor
    explicit FreeBusyCalendar(QObject *parent = nullptr);

    ~FreeBusyCalendar() override;

    /// Set the FreeBusyItemModel used by the FreeBusyCalendar.
    void setModel(FreeBusyItemModel *model);

    /// Get the FreeBusyItemModel used by the FreeBusyCalendar.
    FreeBusyItemModel *model() const;

    /// Get the calendar created from the FreeBusyItemModel.
    KCalendarCore::Calendar::Ptr calendar() const;

private:
    void onRowsChanged(const QModelIndex &, const QModelIndex &);
    void onRowsInserted(const QModelIndex &, int, int);
    void onRowsRemoved(const QModelIndex &, int, int);
    void onLayoutChanged();
    void deleteAllEvents();

    std::unique_ptr<FreeBusyCalendarPrivate> const d;
};
}
