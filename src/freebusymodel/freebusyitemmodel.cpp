/*
  SPDX-FileCopyrightText: 2010 Casey Link <unnamedrambler@gmail.com>
  SPDX-FileCopyrightText: 2009-2010 Klaralvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "freebusyitemmodel.h"

#include <Akonadi/Calendar/FreeBusyManager>

#include <KLocalizedString>

#include <QLocale>
#include <QTimerEvent>

using namespace CalendarSupport;

class ItemPrivateData
{
public:
    ItemPrivateData(ItemPrivateData *parent)
        : parentItem(parent)
    {
    }

    ~ItemPrivateData()
    {
        qDeleteAll(childItems);
    }

    ItemPrivateData *child(int row)
    {
        return childItems.value(row);
    }

    void appendChild(ItemPrivateData *item)
    {
        childItems.append(item);
    }

    ItemPrivateData *removeChild(int row)
    {
        return childItems.takeAt(row);
    }

    int childCount() const
    {
        return childItems.count();
    }

    int row() const
    {
        if (parentItem) {
            return parentItem->childItems.indexOf(const_cast<ItemPrivateData *>(this));
        }
        return 0;
    }

    ItemPrivateData *parent()
    {
        return parentItem;
    }

private:
    QList<ItemPrivateData *> childItems;
    ItemPrivateData *parentItem = nullptr;
};

class CalendarSupport::FreeBusyItemModelPrivate
{
public:
    ~FreeBusyItemModelPrivate()
    {
        delete mRootData;
    }

    QTimer mReloadTimer;
    bool mForceDownload = false;
    QList<FreeBusyItem::Ptr> mFreeBusyItems;
    ItemPrivateData *mRootData = nullptr;
};

FreeBusyItemModel::FreeBusyItemModel(QObject *parent)
    : QAbstractItemModel(parent)
    , d(new CalendarSupport::FreeBusyItemModelPrivate)
{
    qRegisterMetaType<KCalendarCore::Attendee>();
    qRegisterMetaType<KCalendarCore::FreeBusy::Ptr>("KCalendarCore::FreeBusy::Ptr");
    qRegisterMetaType<KCalendarCore::Period>("KCalendarCore::Period");

    Akonadi::FreeBusyManager *m = Akonadi::FreeBusyManager::self();
    connect(m, &Akonadi::FreeBusyManager::freeBusyRetrieved, this, &FreeBusyItemModel::slotInsertFreeBusy);

    connect(&d->mReloadTimer, &QTimer::timeout, this, &FreeBusyItemModel::autoReload);
    d->mReloadTimer.setSingleShot(true);

    d->mRootData = new ItemPrivateData(nullptr);
}

FreeBusyItemModel::~FreeBusyItemModel()
{
    delete d;
}

QVariant FreeBusyItemModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    auto data = (ItemPrivateData *)index.internalPointer();

    if (data->parent() == d->mRootData) {
        int row = index.row();
        if (row >= d->mFreeBusyItems.size()) {
            return QVariant();
        }

        switch (role) {
        case Qt::DisplayRole:
            return d->mFreeBusyItems.at(row)->attendee().fullName();
        case FreeBusyItemModel::AttendeeRole:
            return QVariant::fromValue(d->mFreeBusyItems.at(row)->attendee());
        case FreeBusyItemModel::FreeBusyRole:
            if (d->mFreeBusyItems.at(row)->freeBusy()) {
                return QVariant::fromValue(d->mFreeBusyItems.at(row)->freeBusy());
            } else {
                return QVariant();
            }
        default:
            return QVariant();
        }
    }

    FreeBusyItem::Ptr fbitem = d->mFreeBusyItems.at(data->parent()->row());
    if (!fbitem->freeBusy() || index.row() >= fbitem->freeBusy()->busyPeriods().size()) {
        return QVariant();
    }

    KCalendarCore::FreeBusyPeriod period = fbitem->freeBusy()->fullBusyPeriods().at(index.row());
    switch (role) {
    case Qt::DisplayRole: // return something to make modeltest happy
        return QStringLiteral("%1 - %2").arg(QLocale().toString(period.start().toLocalTime(), QLocale::ShortFormat),
                                             QLocale().toString(period.end().toLocalTime(), QLocale::ShortFormat));
    case FreeBusyItemModel::FreeBusyPeriodRole:
        return QVariant::fromValue(period);
    default:
        return QVariant();
    }
}

int FreeBusyItemModel::rowCount(const QModelIndex &parent) const
{
    ItemPrivateData *parentData = nullptr;
    if (parent.column() > 0) {
        return 0;
    }

    if (!parent.isValid()) {
        parentData = d->mRootData;
    } else {
        parentData = static_cast<ItemPrivateData *>(parent.internalPointer());
    }

    return parentData->childCount();
}

int FreeBusyItemModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 1;
}

QModelIndex FreeBusyItemModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }

    ItemPrivateData *parentData = nullptr;
    if (!parent.isValid()) {
        parentData = d->mRootData;
    } else {
        parentData = static_cast<ItemPrivateData *>(parent.internalPointer());
    }

    ItemPrivateData *childData = parentData->child(row);
    if (childData) {
        return createIndex(row, column, childData);
    } else {
        return QModelIndex();
    }
}

QModelIndex FreeBusyItemModel::parent(const QModelIndex &child) const
{
    if (!child.isValid()) {
        return QModelIndex();
    }

    auto childData = static_cast<ItemPrivateData *>(child.internalPointer());
    ItemPrivateData *parentData = childData->parent();
    if (parentData == d->mRootData) {
        return QModelIndex();
    }

    return createIndex(parentData->row(), 0, parentData);
}

QVariant FreeBusyItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal && section == 0) {
        return i18n("Attendee");
    }
    return QVariant();
}

void FreeBusyItemModel::addItem(const FreeBusyItem::Ptr &freebusy)
{
    int row = d->mFreeBusyItems.size();
    beginInsertRows(QModelIndex(), row, row);
    d->mFreeBusyItems.append(freebusy);
    auto data = new ItemPrivateData(d->mRootData);
    d->mRootData->appendChild(data);
    endInsertRows();

    if (freebusy->freeBusy() && freebusy->freeBusy()->fullBusyPeriods().size() > 0) {
        QModelIndex parent = index(row, 0);
        setFreeBusyPeriods(parent, freebusy->freeBusy()->fullBusyPeriods());
    }
    updateFreeBusyData(freebusy);
}

void FreeBusyItemModel::setFreeBusyPeriods(const QModelIndex &parent, const KCalendarCore::FreeBusyPeriod::List &list)
{
    if (!parent.isValid()) {
        return;
    }

    auto parentData = static_cast<ItemPrivateData *>(parent.internalPointer());
    int fb_count = list.size();
    int childCount = parentData->childCount();
    QModelIndex first = index(0, 0, parent);
    QModelIndex last = index(childCount - 1, 0, parent);

    if (childCount > 0 && fb_count < childCount) {
        beginRemoveRows(parent, fb_count - 1 < 0 ? 0 : fb_count - 1, childCount - 1);
        for (int i = childCount - 1; i > fb_count; --i) {
            delete parentData->removeChild(i);
        }
        endRemoveRows();
        if (fb_count > 0) {
            last = index(fb_count - 1, 0, parent);
            Q_EMIT dataChanged(first, last);
        }
    } else if (fb_count > childCount) {
        beginInsertRows(parent, childCount, fb_count - 1);
        for (int i = childCount; i < fb_count; ++i) {
            auto childData = new ItemPrivateData(parentData);
            parentData->appendChild(childData);
        }
        endInsertRows();
        if (childCount > 0) {
            last = index(childCount - 1, 0, parent);
            Q_EMIT dataChanged(first, last);
        }
    } else if (fb_count == childCount && fb_count > 0) {
        Q_EMIT dataChanged(first, last);
    }
}

void FreeBusyItemModel::clear()
{
    beginResetModel();
    d->mFreeBusyItems.clear();
    delete d->mRootData;
    d->mRootData = new ItemPrivateData(nullptr);
    endResetModel();
}

void FreeBusyItemModel::removeRow(int row)
{
    beginRemoveRows(QModelIndex(), row, row);
    d->mFreeBusyItems.removeAt(row);
    ItemPrivateData *data = d->mRootData->removeChild(row);
    delete data;
    endRemoveRows();
}

void FreeBusyItemModel::removeItem(const FreeBusyItem::Ptr &freebusy)
{
    int row = d->mFreeBusyItems.indexOf(freebusy);
    if (row >= 0) {
        removeRow(row);
    }
}

void FreeBusyItemModel::removeAttendee(const KCalendarCore::Attendee &attendee)
{
    FreeBusyItem::Ptr anItem;
    for (int i = 0; i < d->mFreeBusyItems.count(); ++i) {
        anItem = d->mFreeBusyItems[i];
        if (anItem->attendee() == attendee) {
            if (anItem->updateTimerID() != 0) {
                killTimer(anItem->updateTimerID());
            }
            removeRow(i);
            break;
        }
    }
}

bool FreeBusyItemModel::containsAttendee(const KCalendarCore::Attendee &attendee)
{
    FreeBusyItem::Ptr anItem;
    for (int i = 0; i < d->mFreeBusyItems.count(); ++i) {
        anItem = d->mFreeBusyItems[i];
        if (anItem->attendee() == attendee) {
            return true;
        }
    }
    return false;
}

void FreeBusyItemModel::updateFreeBusyData(const FreeBusyItem::Ptr &item)
{
    if (item->isDownloading()) {
        // This item is already in the process of fetching the FB list
        return;
    }

    if (item->updateTimerID() != 0) {
        // An update timer is already running. Reset it
        killTimer(item->updateTimerID());
    }

    // This item does not have a download running, and no timer is set
    // Do the download in one second
    item->setUpdateTimerID(startTimer(1000));
}

void FreeBusyItemModel::timerEvent(QTimerEvent *event)
{
    killTimer(event->timerId());
    for (FreeBusyItem::Ptr item : std::as_const(d->mFreeBusyItems)) {
        if (item->updateTimerID() == event->timerId()) {
            item->setUpdateTimerID(0);
            item->startDownload(d->mForceDownload);
            return;
        }
    }
}

void FreeBusyItemModel::slotInsertFreeBusy(const KCalendarCore::FreeBusy::Ptr &fb, const QString &email)
{
    if (!fb) {
        return;
    }

    if (fb->fullBusyPeriods().isEmpty()) {
        return;
    }

    fb->sortList();

    for (FreeBusyItem::Ptr item : std::as_const(d->mFreeBusyItems)) {
        if (item->email() == email) {
            item->setFreeBusy(fb);
            const int row = d->mFreeBusyItems.indexOf(item);
            const QModelIndex parent = index(row, 0);
            Q_EMIT dataChanged(parent, parent);
            setFreeBusyPeriods(parent, fb->fullBusyPeriods());
        }
    }
}

void FreeBusyItemModel::autoReload()
{
    d->mForceDownload = false;
    reload();
}

void FreeBusyItemModel::reload()
{
    for (FreeBusyItem::Ptr item : std::as_const(d->mFreeBusyItems)) {
        if (d->mForceDownload) {
            item->startDownload(d->mForceDownload);
        } else {
            updateFreeBusyData(item);
        }
    }
}

void FreeBusyItemModel::triggerReload()
{
    d->mReloadTimer.start(1000);
}

void FreeBusyItemModel::cancelReload()
{
    d->mReloadTimer.stop();
}
