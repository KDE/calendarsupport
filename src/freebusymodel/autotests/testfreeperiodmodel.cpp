/*
  SPDX-FileCopyrightText: 2010 Casey Link <unnamedrambler@gmail.com>
  SPDX-FileCopyrightText: 2009-2010 Klaralvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "testfreeperiodmodel.h"
#include "../freeperiodmodel.h"

#include <KCalendarCore/Duration>
#include <KCalendarCore/Period>

#include <QAbstractItemModelTester>
#include <QTest>

using namespace CalendarSupport;

// Workaround QTBUG-51789 causing a crash when QtWebEngineWidgets
// is linked into a QCoreApplication.
QTEST_GUILESS_MAIN(FreePeriodModelTest)

void FreePeriodModelTest::testModelValidity()
{
    auto model = new FreePeriodModel(this);
    new QAbstractItemModelTester(model, this);

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
    auto model = new FreePeriodModel(this);
    new QAbstractItemModelTester(model, this);

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
    auto period1 = model->data(index, FreePeriodModel::PeriodRole).value<KCalendarCore::Period>();
    index = model->index(1, 0);
    auto period2 = model->data(index, FreePeriodModel::PeriodRole).value<KCalendarCore::Period>();

    QCOMPARE(period1.start(), startDt);
    QCOMPARE(period1.end(), endPeriod1);
    QCOMPARE(period2.start(), startPeriod2);
    QCOMPARE(period2.end(), endDt);
}
