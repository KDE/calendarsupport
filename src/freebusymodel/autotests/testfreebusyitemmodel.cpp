/*
  SPDX-FileCopyrightText: 2010 Casey Link <unnamedrambler@gmail.com>
  SPDX-FileCopyrightText: 2009-2010 Klaralvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "testfreebusyitemmodel.h"
#include "../freebusyitem.h"
#include "../freebusyitemmodel.h"

#include <KCalendarCore/Attendee>

#include <QAbstractItemModelTester>
#include <QTest>

using namespace CalendarSupport;

// Workaround QTBUG-51789 causing a crash when QtWebEngineWidgets
// is linked into a QCoreApplication.
QTEST_GUILESS_MAIN(FreeBusyItemModelTest)

void FreeBusyItemModelTest::testModelValidity()
{
    auto model = new FreeBusyItemModel(this);
    new QAbstractItemModelTester(model, this);

    QVERIFY(model->rowCount() == 0);

    const QDateTime dt1(QDate(2010, 7, 24), QTime(7, 0, 0), Qt::UTC);
    const QDateTime dt2(QDate(2010, 7, 24), QTime(10, 0, 0), Qt::UTC);
    KCalendarCore::Attendee a1(QStringLiteral("fred"), QStringLiteral("fred@example.com"));
    KCalendarCore::FreeBusy::Ptr fb1(new KCalendarCore::FreeBusy());

    fb1->addPeriod(dt1, KCalendarCore::Duration(60 * 60));
    fb1->addPeriod(dt2, KCalendarCore::Duration(60 * 60));

    FreeBusyItem::Ptr item1(new FreeBusyItem(a1, nullptr));
    item1->setFreeBusy(fb1);

    model->addItem(item1);
    QVERIFY(model->rowCount() == 1);
    QVERIFY(model->containsAttendee(a1));

    QModelIndex i = model->index(0, 0);
    QCOMPARE(a1.fullName(), model->data(i, Qt::DisplayRole).toString());
    QCOMPARE(a1, model->data(i, FreeBusyItemModel::AttendeeRole).value<KCalendarCore::Attendee>());
    QCOMPARE(item1->freeBusy(), model->data(i, FreeBusyItemModel::FreeBusyRole).value<KCalendarCore::FreeBusy::Ptr>());

    QCOMPARE(model->rowCount(i), 2);

    model->removeRow(0);
    QVERIFY(model->rowCount() == 0);

    model->addItem(item1);
    QVERIFY(model->rowCount() == 1);

    model->removeAttendee(a1);
    QVERIFY(model->rowCount() == 0);

    model->addItem(item1);
    QVERIFY(model->rowCount() == 1);

    model->removeItem(item1);
    QVERIFY(model->rowCount() == 0);

    model->addItem(item1);
    QVERIFY(model->rowCount() == 1);

    model->clear();
    QVERIFY(model->rowCount() == 0);
}

void FreeBusyItemModelTest::testModelValidity2()
{
    auto model = new FreeBusyItemModel(this);
    new QAbstractItemModelTester(model, this);

    const QDateTime dt1(QDate(2010, 7, 24), QTime(7, 0, 0), Qt::UTC);
    const QDateTime dt2(QDate(2010, 7, 24), QTime(10, 0, 0), Qt::UTC);
    const QDateTime dt3(QDate(2010, 7, 24), QTime(12, 0, 0), Qt::UTC);
    const QDateTime dt4(QDate(2010, 7, 24), QTime(14, 0, 0), Qt::UTC);

    KCalendarCore::Attendee a1(QStringLiteral("fred"), QStringLiteral("fred@example.com"));
    KCalendarCore::Attendee a2(QStringLiteral("joe"), QStringLiteral("joe@example.com"));
    KCalendarCore::Attendee a3(QStringLiteral("max"), QStringLiteral("max@example.com"));
    KCalendarCore::FreeBusy::Ptr fb1(new KCalendarCore::FreeBusy());
    KCalendarCore::FreeBusy::Ptr fb2(new KCalendarCore::FreeBusy());
    KCalendarCore::FreeBusy::Ptr fb3(new KCalendarCore::FreeBusy());

    fb1->addPeriod(dt1, KCalendarCore::Duration(60 * 60));
    fb1->addPeriod(dt2, KCalendarCore::Duration(60 * 60));

    fb2->addPeriod(dt1, KCalendarCore::Duration(60 * 60));
    fb2->addPeriod(dt2, KCalendarCore::Duration(60 * 60));
    fb2->addPeriod(dt3, KCalendarCore::Duration(60 * 60));

    fb3->addPeriod(dt1, KCalendarCore::Duration(60 * 60));
    fb3->addPeriod(dt2, KCalendarCore::Duration(60 * 60));
    fb3->addPeriod(dt4, KCalendarCore::Duration(60 * 60 * 2));

    FreeBusyItem::Ptr item1(new FreeBusyItem(a1, nullptr));
    item1->setFreeBusy(fb1);
    FreeBusyItem::Ptr item2(new FreeBusyItem(a2, nullptr));
    FreeBusyItem::Ptr item3(new FreeBusyItem(a3, nullptr));

    model->addItem(item1);
    model->addItem(item2);
    model->addItem(item3);

    QCOMPARE(model->rowCount(), 3);

    QVERIFY(model->containsAttendee(a1));
    QVERIFY(model->containsAttendee(a2));
    QVERIFY(model->containsAttendee(a3));

    QModelIndex i1 = model->index(0, 0);
    QCOMPARE(a1.fullName(), model->data(i1, Qt::DisplayRole).toString());
    QCOMPARE(a1, model->data(i1, FreeBusyItemModel::AttendeeRole).value<KCalendarCore::Attendee>());
    QCOMPARE(item1->freeBusy(), model->data(i1, FreeBusyItemModel::FreeBusyRole).value<KCalendarCore::FreeBusy::Ptr>());

    QModelIndex i2 = model->index(1, 0);
    QCOMPARE(a2.fullName(), model->data(i2, Qt::DisplayRole).toString());
    QCOMPARE(a2, model->data(i2, FreeBusyItemModel::AttendeeRole).value<KCalendarCore::Attendee>());
    QVERIFY(model->rowCount(i2) == 0);
    QVERIFY(model->data(i2, FreeBusyItemModel::FreeBusyRole).isValid() == false);

    QModelIndex i3 = model->index(2, 0);
    QCOMPARE(a3.fullName(), model->data(i3, Qt::DisplayRole).toString());
    QCOMPARE(a3, model->data(i3, FreeBusyItemModel::AttendeeRole).value<KCalendarCore::Attendee>());
    QVERIFY(model->rowCount(i3) == 0);
    QVERIFY(model->data(i3, FreeBusyItemModel::FreeBusyRole).isValid() == false);

    model->slotInsertFreeBusy(fb2, QStringLiteral("joe@example.com"));
    QCOMPARE(item2->freeBusy(), model->data(i2, FreeBusyItemModel::FreeBusyRole).value<KCalendarCore::FreeBusy::Ptr>());
    QVERIFY(model->rowCount(i2) == fb2->fullBusyPeriods().size());

    QModelIndex i2_0 = model->index(0, 0, i2);
    QCOMPARE(fb2->fullBusyPeriods().first(), model->data(i2_0, FreeBusyItemModel::FreeBusyPeriodRole).value<KCalendarCore::FreeBusyPeriod>());
    QModelIndex i2_1 = model->index(1, 0, i2);
    QCOMPARE(fb2->fullBusyPeriods().at(1), model->data(i2_1, FreeBusyItemModel::FreeBusyPeriodRole).value<KCalendarCore::FreeBusyPeriod>());
    QModelIndex i2_2 = model->index(2, 0, i2);
    QCOMPARE(fb2->fullBusyPeriods().last(), model->data(i2_2, FreeBusyItemModel::FreeBusyPeriodRole).value<KCalendarCore::FreeBusyPeriod>());

    model->slotInsertFreeBusy(fb3, QStringLiteral("max@example.com"));
    QCOMPARE(item3->freeBusy(), model->data(i3, FreeBusyItemModel::FreeBusyRole).value<KCalendarCore::FreeBusy::Ptr>());
    QVERIFY(model->rowCount(i3) == fb3->fullBusyPeriods().size());

    QModelIndex i3_0 = model->index(0, 0, i3);
    QCOMPARE(fb3->fullBusyPeriods().first(), model->data(i3_0, FreeBusyItemModel::FreeBusyPeriodRole).value<KCalendarCore::FreeBusyPeriod>());
    QModelIndex i3_1 = model->index(1, 0, i3);
    QCOMPARE(fb3->fullBusyPeriods().at(1), model->data(i3_1, FreeBusyItemModel::FreeBusyPeriodRole).value<KCalendarCore::FreeBusyPeriod>());
    QModelIndex i3_2 = model->index(2, 0, i3);
    QCOMPARE(fb3->fullBusyPeriods().last(), model->data(i3_2, FreeBusyItemModel::FreeBusyPeriodRole).value<KCalendarCore::FreeBusyPeriod>());

    model->removeAttendee(a2);

    QCOMPARE(2, model->rowCount());

    QVERIFY(model->containsAttendee(a1) == true);
    QVERIFY(model->containsAttendee(a2) == false);
    QVERIFY(model->containsAttendee(a3) == true);

    i3_0 = model->index(0, 0, i3);
    QCOMPARE(fb3->fullBusyPeriods().first(), model->data(i3_0, FreeBusyItemModel::FreeBusyPeriodRole).value<KCalendarCore::FreeBusyPeriod>());
    i3_1 = model->index(1, 0, i3);
    QCOMPARE(fb3->fullBusyPeriods().at(1), model->data(i3_1, FreeBusyItemModel::FreeBusyPeriodRole).value<KCalendarCore::FreeBusyPeriod>());
    i3_2 = model->index(2, 0, i3);
    QCOMPARE(fb3->fullBusyPeriods().last(), model->data(i3_2, FreeBusyItemModel::FreeBusyPeriodRole).value<KCalendarCore::FreeBusyPeriod>());
}

void FreeBusyItemModelTest::testInsertFreeBusy()
{
    auto model = new FreeBusyItemModel(this);
    new QAbstractItemModelTester(model, this);

    const QDateTime dt1(QDate(2010, 7, 24), QTime(7, 0, 0), Qt::UTC);
    const QDateTime dt2(QDate(2010, 7, 24), QTime(10, 0, 0), Qt::UTC);
    KCalendarCore::Attendee a1(QStringLiteral("fred"), QStringLiteral("fred@example.com"));
    KCalendarCore::FreeBusy::Ptr fb1(new KCalendarCore::FreeBusy());
    fb1->addPeriod(dt1, KCalendarCore::Duration(60 * 60));
    fb1->addPeriod(dt2, KCalendarCore::Duration(60 * 60));

    const QDateTime dt3(QDate(2010, 7, 24), QTime(12, 0, 0), Qt::UTC);
    const QDateTime dt4(QDate(2010, 7, 24), QTime(14, 0, 0), Qt::UTC);
    KCalendarCore::FreeBusy::Ptr fb2(new KCalendarCore::FreeBusy());
    fb2->addPeriod(dt1, KCalendarCore::Duration(60 * 60));
    fb2->addPeriod(dt2, KCalendarCore::Duration(60 * 60));
    fb2->addPeriod(dt3, KCalendarCore::Duration(60 * 60));
    fb2->addPeriod(dt4, KCalendarCore::Duration(60 * 60 * 2));

    FreeBusyItem::Ptr item1(new FreeBusyItem(a1, nullptr));
    item1->setFreeBusy(fb1);

    model->addItem(item1);

    QModelIndex i = model->index(0, 0);
    QCOMPARE(model->rowCount(i), 2);

    model->slotInsertFreeBusy(fb2, QStringLiteral("fred@example.com"));

    QCOMPARE(model->rowCount(i), 4);
}
