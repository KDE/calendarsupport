/*
  SPDX-FileCopyrightText: 2000, 2001, 2004 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  SPDX-FileCopyrightText: 2010 Andras Mantia <andras@kdab.com>
  SPDX-FileCopyrightText: 2010 Casey Link <casey@kdab.com>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "freebusyitem.h"

#include <Akonadi/Calendar/FreeBusyManager>

using namespace CalendarSupport;

FreeBusyItem::FreeBusyItem(const KCalendarCore::Attendee &attendee, QWidget *parentWidget)
    : mAttendee(attendee)
    , mParentWidget(parentWidget)
{
    Q_ASSERT(!attendee.isNull());
    setFreeBusy(KCalendarCore::FreeBusy::Ptr());
}

KCalendarCore::Attendee FreeBusyItem::attendee() const
{
    return mAttendee;
}

void FreeBusyItem::setFreeBusy(const KCalendarCore::FreeBusy::Ptr &fb)
{
    mFreeBusy = fb;
    mIsDownloading = false;
}

KCalendarCore::FreeBusy::Ptr FreeBusyItem::freeBusy() const
{
    return mFreeBusy;
}

QString FreeBusyItem::email() const
{
    return mAttendee.email();
}

void FreeBusyItem::setUpdateTimerID(int id)
{
    mTimerID = id;
}

int FreeBusyItem::updateTimerID() const
{
    return mTimerID;
}

void FreeBusyItem::startDownload(bool forceDownload)
{
    mIsDownloading = true;
    Akonadi::FreeBusyManager *m = Akonadi::FreeBusyManager::self();
    if (!m->retrieveFreeBusy(attendee().email(), forceDownload, mParentWidget)) {
        mIsDownloading = false;
    }
}

void FreeBusyItem::setIsDownloading(bool d)
{
    mIsDownloading = d;
}

bool FreeBusyItem::isDownloading() const
{
    return mIsDownloading;
}
