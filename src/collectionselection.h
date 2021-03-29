/*
  SPDX-FileCopyrightText: 2009 KDAB
  SPDX-FileContributor: Frank Osterfeld <frank@kdab.net>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#pragma once

#include "calendarsupport_export.h"

#include <QObject>

#include <Collection>

class QItemSelection;
class QItemSelectionModel;

namespace CalendarSupport
{
class CALENDARSUPPORT_EXPORT CollectionSelection : public QObject
{
    Q_OBJECT
public:
    explicit CollectionSelection(QItemSelectionModel *selectionModel, QObject *parent = nullptr);
    ~CollectionSelection() override;

    QItemSelectionModel *model() const;
    Q_REQUIRED_RESULT Akonadi::Collection::List selectedCollections() const;
    Q_REQUIRED_RESULT QList<Akonadi::Collection::Id> selectedCollectionIds() const;
    bool contains(const Akonadi::Collection &c) const;
    bool contains(Akonadi::Collection::Id id) const;

    Q_REQUIRED_RESULT bool hasSelection() const;

Q_SIGNALS:
    void selectionChanged(const Akonadi::Collection::List &selected, const Akonadi::Collection::List &deselected);
    void collectionDeselected(const Akonadi::Collection &);
    void collectionSelected(const Akonadi::Collection &);

private:
    void slotSelectionChanged(const QItemSelection &, const QItemSelection &);
    class Private;
    Private *const d;
};
}

