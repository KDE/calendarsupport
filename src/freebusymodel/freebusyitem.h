/*
  SPDX-FileCopyrightText: 2000, 2001, 2004 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  SPDX-FileCopyrightText: 2010 Andras Mantia <andras@kdab.com>
  SPDX-FileCopyrightText: 2010 Casey Link <casey@kdab.com>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "calendarsupport_export.h"

#include <KCalendarCore/FreeBusy>

namespace CalendarSupport
{
/*!
 * \class CalendarSupport::FreeBusyItem
 * \inmodule CalendarSupport
 * \inheaderfile CalendarSupport/FreeBusyItem
 *
 * The FreeBusyItem is the whole line for a given attendee..
 */
class CALENDARSUPPORT_EXPORT FreeBusyItem
{
public:
    using Ptr = QSharedPointer<FreeBusyItem>;

    /*!
     * \a parentWidget is passed to Akonadi when fetching free/busy data.
     */
    FreeBusyItem(const KCalendarCore::Attendee &attendee, QWidget *parentWidget);
    /*!
     */
    ~FreeBusyItem() = default;

    /*!
     */
    [[nodiscard]] KCalendarCore::Attendee attendee() const;
    /*!
     */
    void setFreeBusy(const KCalendarCore::FreeBusy::Ptr &fb);
    /*!
     */
    [[nodiscard]] KCalendarCore::FreeBusy::Ptr freeBusy() const;

    /*!
     */
    [[nodiscard]] QString email() const;
    /*!
     */
    void setUpdateTimerID(int id);
    /*!
     */
    [[nodiscard]] int updateTimerID() const;

    /*!
     */
    void startDownload(bool forceDownload);
    /*!
     */
    void setIsDownloading(bool d);
    /*!
     */
    [[nodiscard]] bool isDownloading() const;

Q_SIGNALS:
    /*!
     */
    void attendeeChanged(const KCalendarCore::Attendee &attendee);

private:
    const KCalendarCore::Attendee mAttendee;
    KCalendarCore::FreeBusy::Ptr mFreeBusy;

    // This is used for the update timer
    int mTimerID = 0;

    // Only run one download job at a time
    bool mIsDownloading = false;

    QWidget *const mParentWidget;
};
}
