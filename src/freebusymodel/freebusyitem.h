/*
  Copyright (c) 2000,2001,2004 Cornelius Schumacher <schumacher@kde.org>
  Copyright (c) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Copyright (c) 2010 Andras Mantia <andras@kdab.com>
  Copyright (C) 2010 Casey Link <casey@kdab.com>

  This library is free software; you can redistribute it and/or modify it
  under the terms of the GNU Library General Public License as published by
  the Free Software Foundation; either version 2 of the License, or (at your
  option) any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
  License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to the
  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.
*/

#ifndef INCIDENCEEDITOR_FREEBUSYITEM_H
#define INCIDENCEEDITOR_FREEBUSYITEM_H

#include "calendarsupport_export.h"

#include <KCalendarCore/FreeBusy>

namespace CalendarSupport {
/**
 * The FreeBusyItem is the whole line for a given attendee..
 */
class CALENDARSUPPORT_EXPORT FreeBusyItem
{
public:
    typedef QSharedPointer<FreeBusyItem> Ptr;

    /**
     * @param parentWidget is passed to Akonadi when fetching free/busy data.
     */
    FreeBusyItem(const KCalendarCore::Attendee &attendee, QWidget *parentWidget);
    ~FreeBusyItem()
    {
    }

    KCalendarCore::Attendee attendee() const;
    void setFreeBusy(const KCalendarCore::FreeBusy::Ptr &fb);
    KCalendarCore::FreeBusy::Ptr freeBusy() const;

    Q_REQUIRED_RESULT QString email() const;
    void setUpdateTimerID(int id);
    Q_REQUIRED_RESULT int updateTimerID() const;

    void startDownload(bool forceDownload);
    void setIsDownloading(bool d);
    Q_REQUIRED_RESULT bool isDownloading() const;

Q_SIGNALS:
    void attendeeChanged(const KCalendarCore::Attendee &attendee);
    void freebusyChanged(const KCalendarCore::FreeBusy::Ptr fb);

private:
    KCalendarCore::Attendee mAttendee;
    KCalendarCore::FreeBusy::Ptr mFreeBusy;

    // This is used for the update timer
    int mTimerID;

    // Only run one download job at a time
    bool mIsDownloading = false;

    QWidget *mParentWidget = nullptr;
};
}
#endif
