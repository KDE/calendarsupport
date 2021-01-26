/*
  SPDX-FileCopyrightText: 2003 Cornelius Schumacher <schumacher@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

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
    QList<CellItem *> conflictItems;
    int maxSubCells = 0;
    QMultiHash<int, CellItem *> subCellDict;

    // Find all items which are in same cell
    QList<CellItem *>::ConstIterator it;
    QList<CellItem *>::ConstIterator end(cells.constEnd());
    for (it = cells.constBegin(); it != end; ++it) {
        CellItem *item = *it;
        if (item == placeItem) {
            continue;
        }

        if (item->overlaps(placeItem)) {
            qCDebug(CALENDARSUPPORT_LOG) << "  Overlaps:" << item->label();

            conflictItems.append(item);
            if (item->subCells() > maxSubCells) {
                maxSubCells = item->subCells();
            }
            subCellDict.insert(item->subCell(), item);
        }
    }

    if (!conflictItems.empty()) {
        // Look for unused sub cell and insert item
        int i;
        for (i = 0; i < maxSubCells; ++i) {
            qCDebug(CALENDARSUPPORT_LOG) << "  Trying subcell" << i;
            if (!subCellDict.contains(i)) {
                qCDebug(CALENDARSUPPORT_LOG) << "  Use subcell" << i;
                placeItem->setSubCell(i);
                break;
            }
        }
        if (i == maxSubCells) {
            qCDebug(CALENDARSUPPORT_LOG) << "  New subcell" << i;
            placeItem->setSubCell(maxSubCells);
            maxSubCells++; // add new item to number of sub cells
        }

        qCDebug(CALENDARSUPPORT_LOG) << "  Sub cells:" << maxSubCells;

        // Write results to item to be placed
        conflictItems.append(placeItem);
        placeItem->setSubCells(maxSubCells);

        QList<CellItem *>::iterator it;
        QList<CellItem *>::iterator end(conflictItems.end());
        for (it = conflictItems.begin(); it != end; ++it) {
            (*it)->setSubCells(maxSubCells);
        }
        // Todo: Adapt subCells of items conflicting with conflicting items
    } else {
        placeItem->setSubCell(0);
        placeItem->setSubCells(1);
    }

    return conflictItems;
}
