/*
 * SPDX-FileCopyrightText: 2014 Sandro Knauß <knauss@kolabsys.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 *
 */

#include "freebusycalendar.h"
#include "calendarsupport_debug.h"

#include <KCalendarCore/CalFormat>
#include <KCalendarCore/FreeBusyPeriod>
#include <KCalendarCore/MemoryCalendar>

#include <KLocalizedString>

#include <QTimeZone>

using namespace CalendarSupport;

class CalendarSupport::FreeBusyCalendarPrivate
{
public:
    FreeBusyCalendarPrivate()
    {
    }

    FreeBusyItemModel *mModel = nullptr;
    KCalendarCore::Calendar::Ptr mCalendar;
    QMap<QModelIndex, KCalendarCore::Event::Ptr> mFbEvent;
};

FreeBusyCalendar::FreeBusyCalendar(QObject *parent)
    : QObject(parent)
    , d(new CalendarSupport::FreeBusyCalendarPrivate)
{
    d->mCalendar = KCalendarCore::Calendar::Ptr(new KCalendarCore::MemoryCalendar(QTimeZone::systemTimeZone()));
    qCDebug(CALENDARSUPPORT_LOG) << "creating" << this;
}

FreeBusyCalendar::~FreeBusyCalendar()
{
    qCDebug(CALENDARSUPPORT_LOG) << "deleting" << this;
    delete d;
}

KCalendarCore::Calendar::Ptr FreeBusyCalendar::calendar() const
{
    return d->mCalendar;
}

FreeBusyItemModel *FreeBusyCalendar::model() const
{
    return d->mModel;
}

void FreeBusyCalendar::setModel(FreeBusyItemModel *model)
{
    if (model != d->mModel) {
        if (d->mModel) {
            disconnect(d->mModel, nullptr, nullptr, nullptr);
        }
        d->mModel = model;
        connect(d->mModel, &QAbstractItemModel::layoutChanged, this, &FreeBusyCalendar::onLayoutChanged);
        connect(d->mModel, &QAbstractItemModel::modelReset, this, &FreeBusyCalendar::onLayoutChanged);
        connect(d->mModel, &QAbstractItemModel::rowsAboutToBeRemoved, this, &FreeBusyCalendar::onRowsRemoved);
        connect(d->mModel, &QAbstractItemModel::rowsInserted, this, &FreeBusyCalendar::onRowsInserted);
        connect(d->mModel, &QAbstractItemModel::dataChanged, this, &FreeBusyCalendar::onRowsChanged);
    }
}

void FreeBusyCalendar::deleteAllEvents()
{
    const KCalendarCore::Event::List lstEvents = d->mCalendar->events();
    for (const KCalendarCore::Event::Ptr &event : lstEvents) {
        d->mCalendar->deleteEvent(event);
    }
}

void FreeBusyCalendar::onLayoutChanged()
{
    if (!d->mFbEvent.isEmpty()) {
        deleteAllEvents();
        d->mFbEvent.clear();
        for (int i = d->mModel->rowCount() - 1; i >= 0; --i) {
            QModelIndex parent = d->mModel->index(i, 0);
            onRowsInserted(parent, 0, d->mModel->rowCount(parent) - 1);
        }
    }
}

void FreeBusyCalendar::onRowsInserted(const QModelIndex &parent, int first, int last)
{
    if (!parent.isValid()) {
        return;
    }
    for (int i = first; i <= last; ++i) {
        QModelIndex index = d->mModel->index(i, 0, parent);

        const KCalendarCore::FreeBusyPeriod &period = d->mModel->data(index, FreeBusyItemModel::FreeBusyPeriodRole).value<KCalendarCore::FreeBusyPeriod>();
        const KCalendarCore::FreeBusy::Ptr &fb = d->mModel->data(parent, FreeBusyItemModel::FreeBusyRole).value<KCalendarCore::FreeBusy::Ptr>();

        KCalendarCore::Event::Ptr inc = KCalendarCore::Event::Ptr(new KCalendarCore::Event());
        inc->setDtStart(period.start());
        inc->setDtEnd(period.end());
        inc->setUid(QLatin1String("fb-") + fb->uid() + QLatin1String("-") + QString::number(i));

        inc->setCustomProperty("FREEBUSY", "STATUS", QString::number(period.type()));
        QString summary = period.summary();
        if (summary.isEmpty()) {
            switch (period.type()) {
            case KCalendarCore::FreeBusyPeriod::Free:
                summary = i18n("Free");
                break;
            case KCalendarCore::FreeBusyPeriod::Busy:
                summary = i18n("Busy");
                break;
            case KCalendarCore::FreeBusyPeriod::BusyUnavailable:
                summary = i18n("Unavailable");
                break;
            case KCalendarCore::FreeBusyPeriod::BusyTentative:
                summary = i18n("Tentative");
                break;
            default:
                summary = i18n("Unknown");
            }
        }
        inc->setSummary(summary);

        d->mFbEvent.insert(index, inc);
        d->mCalendar->addEvent(inc);
    }
}

void FreeBusyCalendar::onRowsRemoved(const QModelIndex &parent, int first, int last)
{
    if (!parent.isValid()) {
        for (int i = first; i <= last; ++i) {
            QModelIndex index = d->mModel->index(i, 0);
            onRowsRemoved(index, 0, d->mModel->rowCount(index) - 1);
        }
    } else {
        for (int i = first; i <= last; ++i) {
            QModelIndex index = d->mModel->index(i, 0, parent);
            KCalendarCore::Event::Ptr inc = d->mFbEvent.take(index);
            d->mCalendar->deleteEvent(inc);
        }
    }
}

void FreeBusyCalendar::onRowsChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    if (!topLeft.parent().isValid()) {
        return;
    }
    for (int i = topLeft.row(); i <= bottomRight.row(); ++i) {
        QModelIndex index = d->mModel->index(i, 0, topLeft.parent());
        KCalendarCore::Event::Ptr inc = d->mFbEvent.value(index);
        d->mCalendar->beginChange(inc);
        d->mCalendar->endChange(inc);
    }
}
