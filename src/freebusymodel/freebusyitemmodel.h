/*
  SPDX-FileCopyrightText: 2010 Casey Link <unnamedrambler@gmail.com>
  SPDX-FileCopyrightText: 2009-2010 Klaralvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "calendarsupport_export.h"

#include "freebusyitem.h"

#include <QAbstractItemModel>
#include <QTimer>

class ItemPrivateData;

namespace CalendarSupport
{
/**
 * The FreeBusyItemModel is a 2-level tree structure.
 *
 * The top level parent nodes represent the freebusy items, and
 * the 2nd-level child nodes represent the FreeBusyPeriods of the parent
 * freebusy item.
 */
class FreeBusyItemModelPrivate;
class CALENDARSUPPORT_EXPORT FreeBusyItemModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    enum Roles { AttendeeRole = Qt::UserRole, FreeBusyRole, FreeBusyPeriodRole };

    explicit FreeBusyItemModel(QObject *parent = nullptr);
    ~FreeBusyItemModel() override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void addItem(const FreeBusyItem::Ptr &freebusy);

    void clear();
    void removeAttendee(const KCalendarCore::Attendee &attendee);
    void removeItem(const FreeBusyItem::Ptr &freebusy);
    void removeRow(int row);

    Q_REQUIRED_RESULT bool containsAttendee(const KCalendarCore::Attendee &attendee);

    /**
     * Queues a reload of free/busy data.
     * All current attendees will have their free/busy data
     * redownloaded from Akonadi.
     */
    void triggerReload();

    /**
     * cancel reloading
     */
    void cancelReload();

    /**
     * Reload FB items
     */
    void reload();

public Q_SLOTS:
    void slotInsertFreeBusy(const KCalendarCore::FreeBusy::Ptr &fb, const QString &email);

protected:
    void timerEvent(QTimerEvent *) override;

private:
    // Only download FB if the auto-download option is set in config
    void autoReload();

    void setFreeBusyPeriods(const QModelIndex &parent, const KCalendarCore::FreeBusyPeriod::List &list);
    void updateFreeBusyData(const FreeBusyItem::Ptr &);

    FreeBusyItemModelPrivate *const d;
};
}
