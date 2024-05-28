/*
  SPDX-FileCopyrightText: 2009 KDAB
  SPDX-FileContributor: Frank Osterfeld <frank@kdab.net>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#pragma once

#include "calendarsupport_export.h"

#include <QObject>

#include <Akonadi/Collection>

#include <memory>

class QItemSelection;
class QItemSelectionModel;

namespace CalendarSupport
{
class CollectionSelectionPrivate;

class CALENDARSUPPORT_EXPORT CollectionSelection : public QObject
{
    Q_OBJECT
public:
    explicit CollectionSelection(QItemSelectionModel *selectionModel, QObject *parent = nullptr);
    ~CollectionSelection() override;

    QItemSelectionModel *model() const;
    [[nodiscard]] Akonadi::Collection::List selectedCollections() const;
    [[nodiscard]] QList<Akonadi::Collection::Id> selectedCollectionIds() const;
    [[nodiscard]] bool contains(const Akonadi::Collection &c) const;
    [[nodiscard]] bool contains(Akonadi::Collection::Id id) const;

    [[nodiscard]] bool hasSelection() const;

Q_SIGNALS:
    void selectionChanged(const Akonadi::Collection::List &selected, const Akonadi::Collection::List &deselected);
    void collectionDeselected(const Akonadi::Collection &);
    void collectionSelected(const Akonadi::Collection &);

private:
    CALENDARSUPPORT_NO_EXPORT void slotSelectionChanged(const QItemSelection &, const QItemSelection &);

    std::unique_ptr<CollectionSelectionPrivate> const d;
};
}
