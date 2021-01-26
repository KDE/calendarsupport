/*
  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
    Author: Stephen Kelly <stephen@kdab.com>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "incidenceattachmentmodel.h"

#include <EntityTreeModel>
#include <ItemFetchJob>
#include <ItemFetchScope>
#include <Monitor>

using namespace CalendarSupport;
using namespace Akonadi;

namespace CalendarSupport
{
class IncidenceAttachmentModelPrivate
{
    IncidenceAttachmentModelPrivate(IncidenceAttachmentModel *qq, const QPersistentModelIndex &modelIndex, const Akonadi::Item &item = Akonadi::Item())
        : q_ptr(qq)
        , m_modelIndex(modelIndex)
        , m_item(item)
    {
        if (modelIndex.isValid()) {
            QObject::connect(modelIndex.model(), SIGNAL(dataChanged(QModelIndex, QModelIndex)), qq, SLOT(resetModel()));
        } else if (item.isValid()) {
            createMonitor();
            resetInternalData();
        }
    }

    void resetModel()
    {
        Q_Q(IncidenceAttachmentModel);
        q->beginResetModel();
        resetInternalData();
        q->endResetModel();
        Q_EMIT q->rowCountChanged();
    }

    void itemFetched(Akonadi::Item::List list)
    {
        Q_ASSERT(list.size() == 1);
        setItem(list.first());
    }

    void setItem(const Akonadi::Item &item);

    void createMonitor()
    {
        if (m_monitor) {
            return;
        }

        m_monitor = new Akonadi::Monitor(q_ptr);
        m_monitor->setObjectName(QStringLiteral("IncidenceAttachmentModelMonitor"));
        m_monitor->setItemMonitored(m_item);
        m_monitor->itemFetchScope().fetchFullPayload(true);
        QObject::connect(m_monitor, SIGNAL(itemChanged(Akonadi::Item, QSet<QByteArray>)), q_ptr, SLOT(resetModel()));
        QObject::connect(m_monitor, SIGNAL(itemRemoved(Akonadi::Item)), q_ptr, SLOT(resetModel()));
    }

    void resetInternalData()
    {
        Item item = m_item;
        if (m_modelIndex.isValid()) {
            item = m_modelIndex.data(EntityTreeModel::ItemRole).value<Akonadi::Item>();
        }

        if (!item.isValid() || !item.hasPayload<KCalendarCore::Incidence::Ptr>()) {
            m_incidence = KCalendarCore::Incidence::Ptr();
            return;
        }
        m_incidence = item.payload<KCalendarCore::Incidence::Ptr>();
    }

    Q_DECLARE_PUBLIC(IncidenceAttachmentModel)
    IncidenceAttachmentModel *const q_ptr;

    QModelIndex m_modelIndex;
    Akonadi::Item m_item;
    KCalendarCore::Incidence::Ptr m_incidence;
    Akonadi::Monitor *m_monitor = nullptr;
};
}

IncidenceAttachmentModel::IncidenceAttachmentModel(const QPersistentModelIndex &modelIndex, QObject *parent)
    : QAbstractListModel(parent)
    , d_ptr(new IncidenceAttachmentModelPrivate(this, modelIndex))
{
}

IncidenceAttachmentModel::IncidenceAttachmentModel(const Akonadi::Item &item, QObject *parent)
    : QAbstractListModel(parent)
    , d_ptr(new IncidenceAttachmentModelPrivate(this, QModelIndex(), item))
{
}

IncidenceAttachmentModel::IncidenceAttachmentModel(QObject *parent)
    : QAbstractListModel(parent)
    , d_ptr(new IncidenceAttachmentModelPrivate(this, QModelIndex()))
{
}

IncidenceAttachmentModel::~IncidenceAttachmentModel()
{
    delete d_ptr;
}

KCalendarCore::Incidence::Ptr IncidenceAttachmentModel::incidence() const
{
    Q_D(const IncidenceAttachmentModel);
    return d->m_incidence;
}

void IncidenceAttachmentModel::setIndex(const QPersistentModelIndex &modelIndex)
{
    Q_D(IncidenceAttachmentModel);
    beginResetModel();
    d->m_modelIndex = modelIndex;
    d->m_item = Akonadi::Item();
    d->resetInternalData();
    endResetModel();
    Q_EMIT rowCountChanged();
}

void IncidenceAttachmentModel::setItem(const Akonadi::Item &item)
{
    Q_D(IncidenceAttachmentModel);
    if (!item.hasPayload<KCalendarCore::Incidence::Ptr>()) {
        auto job = new ItemFetchJob(item);
        job->fetchScope().fetchFullPayload(true);
        connect(job, SIGNAL(itemsReceived(Akonadi::Item::List)), SLOT(itemFetched(Akonadi::Item::List)));
        return;
    }
    d->setItem(item);
}

void IncidenceAttachmentModelPrivate::setItem(const Akonadi::Item &item)
{
    Q_Q(IncidenceAttachmentModel);
    q->beginResetModel();
    m_modelIndex = QModelIndex();
    m_item = item;
    createMonitor();
    resetInternalData();
    q->endResetModel();
    Q_EMIT q->rowCountChanged();
}

int IncidenceAttachmentModel::rowCount(const QModelIndex &) const
{
    Q_D(const IncidenceAttachmentModel);
    if (!d->m_incidence) {
        return 0;
    } else {
        return d->m_incidence->attachments().size();
    }
}

QVariant IncidenceAttachmentModel::data(const QModelIndex &index, int role) const
{
    Q_D(const IncidenceAttachmentModel);
    if (!d->m_incidence) {
        return QVariant();
    }

    const KCalendarCore::Attachment attachment = d->m_incidence->attachments().at(index.row());
    switch (role) {
    case Qt::DisplayRole:
        return attachment.label();
    case AttachmentDataRole:
        return attachment.decodedData();
    case MimeTypeRole:
        return attachment.mimeType();
    }
    return QVariant();
}

QVariant IncidenceAttachmentModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return QAbstractItemModel::headerData(section, orientation, role);
}

QHash<int, QByteArray> CalendarSupport::IncidenceAttachmentModel::roleNames() const
{
    QHash<int, QByteArray> roleNames = QAbstractListModel::roleNames();
    roleNames.insert(IncidenceAttachmentModel::MimeTypeRole, "mimeType");
    return roleNames;
}

#include "moc_incidenceattachmentmodel.cpp"
