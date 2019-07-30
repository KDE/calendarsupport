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

#include "testfreeperiodmodel.h"
#include "modeltest.h"
#include "../freeperiodmodel.h"

#include <KCalendarCore/Period>
#include <KCalendarCore/Duration>

#include <QTest>

using namespace CalendarSupport;

// Workaround QTBUG-51789 causing a crash when QtWebEngineWidgets
// is linked into a QCoreApplication.
QTEST_GUILESS_MAIN(FreePeriodModelTest)

void FreePeriodModelTest::testModelValidity()
{
    FreePeriodModel *model = new FreePeriodModel(this);
    new ModelTest(model, this);

    const QDateTime dt1(QDate(2010, 7, 24), QTime(7, 0, 0), Qt::UTC);
    const QDateTime dt2(QDate(2010, 7, 24), QTime(10, 0, 0), Qt::UTC);

    KCalendarCore::Period::List list;

    list << KCalendarCore::Period(dt1, KCalendarCore::Duration(60 * 60));
    list << KCalendarCore::Period(dt2, KCalendarCore::Duration(60 * 60));

    QCOMPARE(model->rowCount(), 0);
    model->slotNewFreePeriods(list);
    QCOMPARE(model->rowCount(), 2);
}

void FreePeriodModelTest::testSplitByDay()
{
    FreePeriodModel *model = new FreePeriodModel(this);
    new ModelTest(model, this);

    const QDateTime startDt(QDate(2010, 7, 24), QTime(8, 0, 0), Qt::UTC);
    const QDateTime endDt(QDate(2010, 7, 25), QTime(8, 0, 0), Qt::UTC);

    KCalendarCore::Period::List list;

    // This period goes from 8am on the 24th to 8am on the 25th
    list << KCalendarCore::Period(startDt, endDt);

    QCOMPARE(model->rowCount(), 0);

    // as part of adding the new periods
    // the model should split the above period into two
    // one from 8am-12 on the 24th, and the second from 00-08 on the 25th
    model->slotNewFreePeriods(list);

    const QDateTime endPeriod1(QDate(2010, 7, 24), QTime(23, 59, 59, 999), Qt::UTC);
    const QDateTime startPeriod2(QDate(2010, 7, 25), QTime(0, 0, 0, 0), Qt::UTC);

    QModelIndex index = model->index(0, 0);
    KCalendarCore::Period period1
        = model->data(index, FreePeriodModel::PeriodRole).value<KCalendarCore::Period>();
    index = model->index(1, 0);
    KCalendarCore::Period period2
        = model->data(index, FreePeriodModel::PeriodRole).value<KCalendarCore::Period>();

    QCOMPARE(period1.start(), startDt);
    QCOMPARE(period1.end(), endPeriod1);
    QCOMPARE(period2.start(), startPeriod2);
    QCOMPARE(period2.end(), endDt);
}
