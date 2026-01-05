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

/*!
 * \class CalendarSupport::FreeBusyCalendar
 * \inmodule CalendarSupport
 * \inheaderfile CalendarSupport/FreeBusyCalendar
 *
 * \brief A FreeBusyCalendar exposes a FreeBusyItemModel as a KCalendarCore::Calendar::Ptr.
 */
class CALENDARSUPPORT_EXPORT FreeBusyCalendar : public QObject
{
    Q_OBJECT
public:
    /*!
     * \brief FreeBusyCalendar Constructor
     * \param parent
     */
    explicit FreeBusyCalendar(QObject *parent = nullptr);

    /*!
     */
    ~FreeBusyCalendar() override;

    /*!
     * \brief setModel Set the FreeBusyItemModel used by the FreeBusyCalendar.
     * \param model
     */
    void setModel(FreeBusyItemModel *model);

    /*!
     * \brief model
     * \return the FreeBusyItemModel used by the FreeBusyCalendar.
     */
    FreeBusyItemModel *model() const;

    /*!
     * \brief calendar
     * \return the calendar created from the FreeBusyItemModel.
     */
    KCalendarCore::Calendar::Ptr calendar() const;

private:
    CALENDARSUPPORT_NO_EXPORT void onRowsChanged(const QModelIndex &, const QModelIndex &);
    CALENDARSUPPORT_NO_EXPORT void onRowsInserted(const QModelIndex &, int, int);
    CALENDARSUPPORT_NO_EXPORT void onRowsRemoved(const QModelIndex &, int, int);
    CALENDARSUPPORT_NO_EXPORT void onLayoutChanged();
    CALENDARSUPPORT_NO_EXPORT void deleteAllEvents();

    std::unique_ptr<FreeBusyCalendarPrivate> const d;
};
}
