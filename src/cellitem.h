/*
  SPDX-FileCopyrightText: 2003 Cornelius Schumacher <schumacher@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#pragma once

#include "calendarsupport_export.h"
#include <QList>

namespace CalendarSupport
{
class CALENDARSUPPORT_EXPORT CellItem
{
public:
    CellItem()
    {
    }

    virtual ~CellItem()
    {
    }

    void setSubCells(int v);
    Q_REQUIRED_RESULT int subCells() const;

    void setSubCell(int v);
    Q_REQUIRED_RESULT int subCell() const;

    virtual bool overlaps(CellItem *other) const = 0;

    Q_REQUIRED_RESULT virtual QString label() const;

    /**
      Place item @p placeItem into stripe containing items @p cells in a
      way that items don't overlap.
      @param cells The list of other cell items to be laid out parallel to the placeItem.
      @param placeItem The item to be laid out.

      @return Placed items
    */
    static QList<CellItem *> placeItem(const QList<CellItem *> &cells, CellItem *placeItem);

private:
    int mSubCells = 0;
    int mSubCell = -1;
};
}

