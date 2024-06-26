/*
  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
    Author: Stephen Kelly <stephen@kdab.com>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <Akonadi/Attribute>
#include <Akonadi/Item>

#include <KCalendarCore/Incidence>

#include <QAbstractListModel>

#include <memory>

namespace Akonadi
{
}

namespace CalendarSupport
{
class IncidenceAttachmentModelPrivate;

class IncidenceAttachmentModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int attachmentCount READ rowCount NOTIFY rowCountChanged)

public:
    enum Roles {
        AttachmentDataRole = Qt::UserRole,
        MimeTypeRole,
        AttachmentCountRole,

        UserRole = Qt::UserRole + 100
    };

    explicit IncidenceAttachmentModel(const QPersistentModelIndex &modelIndex, QObject *parent = nullptr);

    explicit IncidenceAttachmentModel(const Akonadi::Item &item, QObject *parent = nullptr);

    explicit IncidenceAttachmentModel(QObject *parent = nullptr);

    ~IncidenceAttachmentModel() override;

    KCalendarCore::Incidence::Ptr incidence() const;

    void setItem(const Akonadi::Item &item);
    void setIndex(const QPersistentModelIndex &modelIndex);

    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

Q_SIGNALS:
    void rowCountChanged();

private:
    Q_DECLARE_PRIVATE(IncidenceAttachmentModel)
    std::unique_ptr<IncidenceAttachmentModelPrivate> const d_ptr;

    Q_PRIVATE_SLOT(d_func(), void resetModel())
    Q_PRIVATE_SLOT(d_func(), void itemFetched(Akonadi::Item::List))
};
}
