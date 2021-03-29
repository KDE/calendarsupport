/*
  SPDX-FileCopyrightText: 2010 Casey Link <unnamedrambler@gmail.com>
  SPDX-FileCopyrightText: 2009-2010 Klaralvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "calendarsupport_export.h"

#include <KCalendarCore/Period>

#include <QAbstractTableModel>

namespace CalendarSupport
{
class CALENDARSUPPORT_EXPORT FreePeriodModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Roles { PeriodRole = Qt::UserRole };
    explicit FreePeriodModel(QObject *parent = nullptr);
    ~FreePeriodModel() override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Q_REQUIRED_RESULT int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    Q_REQUIRED_RESULT int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    Q_REQUIRED_RESULT QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

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
