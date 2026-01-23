/*
  SPDX-FileCopyrightText: 2003 Cornelius Schumacher <schumacher@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/

#pragma once

#include "calendarsupport_export.h"
#include <QList>

namespace CalendarSupport
{
/*!
 * \class CalendarSupport::CellItem
 * \inmodule CalendarSupport
 * \inheaderfile CalendarSupport/CellItem
 *
 * \brief The CellItem class
 */
class CALENDARSUPPORT_EXPORT CellItem
{
public:
    /*!
     */
    CellItem() = default;

    virtual ~CellItem() = default;

    /*!
     */
    void setSubCells(int v);
    /*!
     */
    [[nodiscard]] int subCells() const;

    /*!
     */
    void setSubCell(int v);
    /*!
     */
    [[nodiscard]] int subCell() const;

    /*!
     */
    virtual bool overlaps(CellItem *other) const = 0;

    /*!
     */
    [[nodiscard]] virtual QString label() const;

    /*!
      Place item \a placeItem into stripe containing items \a cells in a
      way that items don't overlap.
      \a cells The list of other cell items to be laid out parallel to the placeItem.
      \a placeItem The item to be laid out.

      Returns Placed items
    */
    static QList<CellItem *> placeItem(const QList<CellItem *> &cells, CellItem *placeItem);

private:
    int mSubCells = 0;
    int mSubCell = -1;
};
}
