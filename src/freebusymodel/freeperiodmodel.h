/*
  Copyright (C) 2010 Casey Link <unnamedrambler@gmail.com>
  Copyright (C) 2009-2010 Klaralvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>

  This library is free software; you can redistribute it and/or modify it
  under the terms of the GNU Library General Public License as published by
  the Free Software Foundation; either version 2 of the License, or (at your
  option) any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
  License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to the
  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.
*/

#ifndef INCIDENCEEDITOR_FREEPERIODMODEL_H
#define INCIDENCEEDITOR_FREEPERIODMODEL_H

#include "calendarsupport_export.h"

#include <KCalendarCore/Period>

#include <QAbstractTableModel>

namespace CalendarSupport {
class CALENDARSUPPORT_EXPORT FreePeriodModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Roles {
        PeriodRole = Qt::UserRole
    };
    explicit FreePeriodModel(QObject *parent = nullptr);
    ~FreePeriodModel() override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Q_REQUIRED_RESULT int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    Q_REQUIRED_RESULT int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    Q_REQUIRED_RESULT QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

public Q_SLOTS:
    void slotNewFreePeriods(const KCalendarCore::Period::List &freePeriods);

private:
    /** Splits period blocks in the provided list, so that each period occurs on one day */
    KCalendarCore::Period::List splitPeriodsByDay(const KCalendarCore::Period::List &freePeriods);

    QString day(int index) const;
    QString date(int index) const;
    QString stringify(int index) const;
    QString tooltipify(int index) const;

    KCalendarCore::Period::List mPeriodList;
    friend class FreePeriodModelTest;
};
}
#endif
