/*
  SPDX-FileCopyrightText: 2003 Cornelius Schumacher <schumacher@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#include <QSet>

#include "cellitem.h"

#include "calendarsupport_debug.h"
#include <KLocalizedString>

using namespace CalendarSupport;

void CellItem::setSubCells(int v)
{
    mSubCells = v;
}

int CellItem::subCells() const
{
    return mSubCells;
}

void CellItem::setSubCell(int v)
{
    mSubCell = v;
}

int CellItem::subCell() const
{
    return mSubCell;
}

QString CellItem::label() const
{
    return xi18n("<placeholder>undefined</placeholder>");
}

QList<CellItem *> CellItem::placeItem(const QList<CellItem *> &cells, CellItem *placeItem)
{
    int maxSubCells = 0;
    QSet<int> subCellsInUse;

    // Find all items that overlap placeItem, the items that overlaps them, and so on.
    QList<CellItem *> overlappingItems {placeItem};
    for (int i = 0; i < overlappingItems.count(); i++) {
        const auto checkItem = overlappingItems.at(i);
        for (const auto item : cells) {
            if (item->overlaps(checkItem) && !overlappingItems.contains(item)) {
                qCDebug(CALENDARSUPPORT_LOG) << item->label() << "overlaps" << checkItem->label();
                overlappingItems.append(item);
                if (item->subCell() >= maxSubCells) {
                    maxSubCells = item->subCells();
                }
                if (checkItem == placeItem) {
                    subCellsInUse.insert(item->subCell());
                }
            }
        }
    }

    if (overlappingItems.count() > 1) {
        // Look for an unused subcell in placeItem's cells.  If all are used,
        // all overlapping items have to squeeze over.
        int i;
        for (i = 0; i < maxSubCells; ++i) {
            if (!subCellsInUse.contains(i)) {
                break;
            }
        }
        placeItem->setSubCell(i);
        if (i == maxSubCells) {
            maxSubCells += 1;
            for (auto item : overlappingItems) {
                item->setSubCells(maxSubCells);
            }
        }
        placeItem->setSubCells(maxSubCells);
        qCDebug(CALENDARSUPPORT_LOG) << "use subcell" << i << "of" << maxSubCells;
    } else {
        // Nothing overlapped placeItem, so:
        overlappingItems.clear();
        placeItem->setSubCell(0);
        placeItem->setSubCells(1);
    }

    return overlappingItems;
}
