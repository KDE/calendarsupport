/*
 * SPDX-FileCopyrightText: 2021 Glen Ditchfield <GJDitchfield@acm.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QList>
#include <QTest>

#include "cellitem.h"

class PlaceItemTest : public QObject
{
    Q_OBJECT
public:

private Q_SLOTS:
    void soleItemHasNoOverlaps();
    void itemDoesNotOverlapItself();
    void twoItemsShareTheirCell();
    void variousOverlapPositions_data();
    void variousOverlapPositions();
    void fillLeftGap();
    void fillCenterGap();
    void transitiveOverlap();
};

using namespace CalendarSupport;

// Skeletal item class, not unlike EventViews::AgendaItem.  Instances represent
// a range of cell indexes from startAt up to but not including endBefore.
struct TestItem: public CellItem
{
    const QString name;
    const int startAt, endBefore;

    TestItem(const char *n, int s, int e)
        : name(QString::fromLatin1(n))
        , startAt(s)
        , endBefore(e)
    {
    }

    TestItem(const char *n, int s, int e, int subCell, int subCells)
        : name(QString::fromLatin1(n))
        , startAt(s)
        , endBefore(e)
    {
        setSubCell(subCell);
        setSubCells(subCells);
    }

    bool overlaps(CellItem *o) const override
    {
        auto other = static_cast<TestItem *>(o);
        return !(other->endBefore <= startAt || other->startAt >= endBefore);
    }

    QString label() const override
    {
        return name;
    }
};

void PlaceItemTest::soleItemHasNoOverlaps()
{
    auto item = std::make_unique<TestItem>("i", 1, 3);
    const QList<CellItem *> cells;
    auto overlappers = CellItem::placeItem(cells, item.get());
    QCOMPARE(overlappers.size(), 0);
    QCOMPARE(item->subCell(), 0);
    QCOMPARE(item->subCells(), 1);
}

void PlaceItemTest::itemDoesNotOverlapItself()
{
    auto item = std::make_unique<TestItem>("i", 1, 3);
    const QList<CellItem *> cells({item.get()});
    auto overlappers = CellItem::placeItem(cells, item.get());
    QCOMPARE(overlappers.size(), 0);
    QCOMPARE(item->subCell(), 0);
    QCOMPARE(item->subCells(), 1);
}

void PlaceItemTest::twoItemsShareTheirCell()
{
    auto oldItem = std::make_unique<TestItem>("i1", 1, 2, 0, 1);
    const QList<CellItem *> cells({oldItem.get()});
    auto newItem = std::make_unique<TestItem>("i2", 1, 2);
    auto overlappers = CellItem::placeItem(cells, newItem.get());
    QCOMPARE(overlappers.size(), 2);
    QCOMPARE(oldItem->subCells(), 2);
    QCOMPARE(newItem->subCells(), 2);
    QVERIFY(oldItem->subCell() != newItem->subCell());
    QVERIFY(oldItem->subCell() < oldItem->subCells());
    QVERIFY(newItem->subCell() < newItem->subCells());
}

void PlaceItemTest::variousOverlapPositions_data()
{
    QTest::addColumn<int>("startAt");
    QTest::addColumn<bool>("shouldOverlap");

    QTest::newRow("before top") << 0 << false;
    QTest::newRow("overlaps top") << 1 << true;
    QTest::newRow("at top") << 2 << true;
    QTest::newRow("inside") << 3 << true;
    QTest::newRow("at bottom") << 4 << true;
    QTest::newRow("overlaps bottom") << 5 << true;
    QTest::newRow("after bottom") << 6 << false;
}

// Evaluate placement of a new item at various positions relative to an existing
// item that covers many cells.
void PlaceItemTest::variousOverlapPositions()
{
    QFETCH(int, startAt);
    QFETCH(bool, shouldOverlap);

    auto oldItem = std::make_unique<TestItem>("old", 2, 6, 0, 1);
    const QList<CellItem *> cells({oldItem.get()});
    auto newItem = std::make_unique<TestItem>("new", startAt, startAt + 2);
    auto overlappers = CellItem::placeItem(cells, newItem.get());
    QCOMPARE(overlappers.size(), shouldOverlap ? 2 : 0);
    QVERIFY(!shouldOverlap || oldItem->subCell() != newItem->subCell());
    QCOMPARE(oldItem->subCells(), shouldOverlap ? 2 : 1);
    QCOMPARE(newItem->subCells(), shouldOverlap ? 2 : 1);
}

// |item1|              |item1|
// |_____||item2|  -->  |_____||item2|
//        |_____|       | new ||_____|
//                      |_____|
void PlaceItemTest::fillLeftGap()
{
    auto item1 = std::make_unique<TestItem>("item1", 0, 2, 0, 2);
    auto item2 = std::make_unique<TestItem>("item2", 1, 3, 1, 2);
    const QList<CellItem *> cells({item1.get(), item2.get()});
    auto newItem = std::make_unique<TestItem>("new", 2, 4);
    (void) CellItem::placeItem(cells, newItem.get());
    QCOMPARE(newItem->subCell(), item1->subCell());
    QCOMPARE(newItem->subCells(), 2);
}

//        |item1|                     |item1|
// |item2||_____||item3|  -->  |item2||_____||item3|
// |_____|       |_____|       |_____|| new ||_____|
//                                    |_____|
void PlaceItemTest::fillCenterGap()
{
    auto item1 = std::make_unique<TestItem>("item1", 0, 2, 1, 3);
    auto item2 = std::make_unique<TestItem>("item2", 1, 3, 0, 3);
    auto item3 = std::make_unique<TestItem>("item3", 1, 3, 2, 3);
    const QList<CellItem *> cells({item1.get(), item2.get(), item3.get()});
    auto newItem= std::make_unique<TestItem>("new", 2, 4);
    (void) CellItem::placeItem(cells, newItem.get());
    QCOMPARE(newItem->subCell(), item1->subCell());
    QCOMPARE(newItem->subCells(), 3);
}

// Items that do not overlap placeItem may also need adjustment.
// See https://bugs.kde.org/show_bug.cgi?id=64603
// |item1   |                 |item1|
// |________||item3   |  -->  |_____||item3|
// |item2   ||________|       |item2||_____|| new |
// |________|                 |_____|       |_____|
void PlaceItemTest::transitiveOverlap()
{
    auto item1 = std::make_unique<TestItem>("item1", 0, 2, 0, 2);
    auto item2 = std::make_unique<TestItem>("item2", 2, 4, 0, 2);
    auto item3 = std::make_unique<TestItem>("item3", 1, 3, 1, 2);
    const QList<CellItem *> cells({item1.get(), item2.get(), item3.get()});
    auto newItem= std::make_unique<TestItem>("new", 2, 4);
    (void) CellItem::placeItem(cells, newItem.get());
    QCOMPARE(newItem->subCells(), 3);
    QCOMPARE(item1->subCells(), 3);
    QCOMPARE(item2->subCells(), 3);
    QCOMPARE(item3->subCells(), 3);
}

QTEST_MAIN(PlaceItemTest)

#include "placeitemtest.moc"
