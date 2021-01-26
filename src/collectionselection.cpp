/*
  SPDX-FileCopyrightText: 2009 KDAB
  SPDX-FileContributor: Frank Osterfeld <frank@kdab.net>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#include "collectionselection.h"
#include "utils.h"

#include <QItemSelectionModel>

using namespace CalendarSupport;

class Q_DECL_HIDDEN CollectionSelection::Private
{
public:
    explicit Private(QItemSelectionModel *model_)
        : model(model_)
    {
    }

    QItemSelectionModel *const model;
};

CollectionSelection::CollectionSelection(QItemSelectionModel *selectionModel, QObject *parent)
    : QObject(parent)
    , d(new Private(selectionModel))
{
    connect(selectionModel, &QItemSelectionModel::selectionChanged, this, &CollectionSelection::slotSelectionChanged);
}

CollectionSelection::~CollectionSelection()
{
    delete d;
}

QItemSelectionModel *CollectionSelection::model() const
{
    return d->model;
}

bool CollectionSelection::hasSelection() const
{
    return d->model->hasSelection();
}

bool CollectionSelection::contains(const Akonadi::Collection &c) const
{
    return selectedCollectionIds().contains(c.id());
}

bool CollectionSelection::contains(Akonadi::Collection::Id id) const
{
    return selectedCollectionIds().contains(id);
}

Akonadi::Collection::List CollectionSelection::selectedCollections() const
{
    Akonadi::Collection::List selected;
    const QModelIndexList selectedIndexes = d->model->selectedIndexes();
    selected.reserve(selectedIndexes.count());
    for (const QModelIndex &idx : selectedIndexes) {
        selected.append(collectionFromIndex(idx));
    }
    return selected;
}

QList<Akonadi::Collection::Id> CollectionSelection::selectedCollectionIds() const
{
    QList<Akonadi::Collection::Id> selected;
    const QModelIndexList selectedIndexes = d->model->selectedIndexes();
    selected.reserve(selectedIndexes.count());
    for (const QModelIndex &idx : selectedIndexes) {
        selected.append(collectionIdFromIndex(idx));
    }
    return selected;
}

void CollectionSelection::slotSelectionChanged(const QItemSelection &selectedIndexes, const QItemSelection &deselIndexes)
{
    const Akonadi::Collection::List selected = collectionsFromIndexes(selectedIndexes.indexes());
    const Akonadi::Collection::List deselected = collectionsFromIndexes(deselIndexes.indexes());

    Q_EMIT selectionChanged(selected, deselected);
    for (const Akonadi::Collection &c : deselected) {
        Q_EMIT collectionDeselected(c);
    }
    for (const Akonadi::Collection &c : selected) {
        Q_EMIT collectionSelected(c);
    }
}
