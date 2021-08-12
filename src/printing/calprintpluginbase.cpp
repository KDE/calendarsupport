/*
  SPDX-FileCopyrightText: 1998 Preston Brown <pbrown@kde.org>
  SPDX-FileCopyrightText: 2003 Reinhold Kainhofer <reinhold@kainhofer.com>
  SPDX-FileCopyrightText: 2008 Ron Goodheart <rong.dev@gmail.com>
  SPDX-FileCopyrightText: 2012-2013 Allen Winter <winter@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#include "calprintpluginbase.h"
#include "cellitem.h"
#include "kcalprefs.h"
#include "utils.h"

#include <Item>

#include "calendarsupport_debug.h"
#include <KConfig>
#include <KConfigGroup>
#include <KWordWrap>

#include <KLocalizedString>
#include <QAbstractTextDocumentLayout>
#include <QFrame>
#include <QLabel>
#include <QLocale>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QTimeZone>
#include <QVBoxLayout>
#include <qmath.h> // qCeil krazy:exclude=camelcase since no QMath

using namespace CalendarSupport;

static QString cleanStr(const QString &instr)
{
    QString ret = instr;
    return ret.replace(QLatin1Char('\n'), QLatin1Char(' '));
}

const QColor CalPrintPluginBase::sHolidayBackground = QColor(244, 244, 244);

/******************************************************************
 **              The Todo positioning structure                  **
 ******************************************************************/
class CalPrintPluginBase::TodoParentStart
{
public:
    TodoParentStart(QRect pt = QRect(), bool hasLine = false, bool page = true)
        : mRect(pt)
        , mHasLine(hasLine)
        , mSamePage(page)
    {
    }

    QRect mRect;
    bool mHasLine;
    bool mSamePage;
};

/******************************************************************
 **                     The Print item                           **
 ******************************************************************/

class PrintCellItem : public CellItem
{
public:
    PrintCellItem(const KCalendarCore::Event::Ptr &event, const QDateTime &start, const QDateTime &end)
        : mEvent(event)
        , mStart(start)
        , mEnd(end)
    {
    }

    KCalendarCore::Event::Ptr event() const
    {
        return mEvent;
    }

    QString label() const override
    {
        return mEvent->summary();
    }

    QDateTime start() const
    {
        return mStart;
    }

    QDateTime end() const
    {
        return mEnd;
    }

    /** Calculate the start and end date/time of the recurrence that
        happens on the given day */
    bool overlaps(CellItem *o) const override
    {
        auto other = static_cast<PrintCellItem *>(o);
        return !(other->start() >= end() || other->end() <= start());
    }

private:
    KCalendarCore::Event::Ptr mEvent;
    QDateTime mStart, mEnd;
};

/******************************************************************
 **                    The Print plugin                          **
 ******************************************************************/

CalPrintPluginBase::CalPrintPluginBase()
    : PrintPlugin()
    , mUseColors(true)
    , mPrintFooter(true)
    , mHeaderHeight(-1)
    , mSubHeaderHeight(SUBHEADER_HEIGHT)
    , mFooterHeight(-1)
    , mMargin(MARGIN_SIZE)
    , mPadding(PADDING_SIZE)
{
}

CalPrintPluginBase::~CalPrintPluginBase()
{
}

QWidget *CalPrintPluginBase::createConfigWidget(QWidget *w)
{
    auto wdg = new QFrame(w);
    auto layout = new QVBoxLayout(wdg);

    auto title = new QLabel(description(), wdg);
    QFont titleFont(title->font());
    titleFont.setPointSize(20);
    titleFont.setBold(true);
    title->setFont(titleFont);

    layout->addWidget(title);
    layout->addWidget(new QLabel(info(), wdg));
    layout->addSpacing(20);
    layout->addWidget(new QLabel(i18n("This printing style does not have any configuration options."), wdg));
    layout->addStretch();
    return wdg;
}

void CalPrintPluginBase::doPrint(QPrinter *printer)
{
    if (!printer) {
        return;
    }
    mPrinter = printer;
    QPainter p;

    mPrinter->setColorMode(mUseColors ? QPrinter::Color : QPrinter::GrayScale);

    p.begin(mPrinter);
    // TODO: Fix the margins!!!
    // the painter initially begins at 72 dpi per the Qt docs.
    // we want half-inch margins.
    int margins = margin();
    p.setViewport(margins, margins, p.viewport().width() - 2 * margins, p.viewport().height() - 2 * margins);
    //   QRect vp( p.viewport() );
    // vp.setRight( vp.right()*2 );
    // vp.setBottom( vp.bottom()*2 );
    //   p.setWindow( vp );
    int pageWidth = p.window().width();
    int pageHeight = p.window().height();
    //   int pageWidth = p.viewport().width();
    //   int pageHeight = p.viewport().height();

    print(p, pageWidth, pageHeight);

    p.end();
    mPrinter = nullptr;
}

void CalPrintPluginBase::doLoadConfig()
{
    if (mConfig) {
        KConfigGroup group(mConfig, groupName());
        mConfig->sync();
        QDateTime dt = QDateTime::currentDateTime();
        mFromDate = group.readEntry("FromDate", dt).date();
        mToDate = group.readEntry("ToDate", dt).date();
        mUseColors = group.readEntry("UseColors", true);
        mPrintFooter = group.readEntry("PrintFooter", true);
        mShowNoteLines = group.readEntry("Note Lines", false);
        mExcludeConfidential = group.readEntry("Exclude confidential", true);
        mExcludePrivate = group.readEntry("Exclude private", true);
    } else {
        qCDebug(CALENDARSUPPORT_LOG) << "No config available in loadConfig!!!!";
    }
}

void CalPrintPluginBase::doSaveConfig()
{
    if (mConfig) {
        KConfigGroup group(mConfig, groupName());
        QDateTime dt = QDateTime::currentDateTime(); // any valid QDateTime will do
        dt.setDate(mFromDate);
        group.writeEntry("FromDate", dt);
        dt.setDate(mToDate);
        group.writeEntry("ToDate", dt);
        group.writeEntry("UseColors", mUseColors);
        group.writeEntry("PrintFooter", mPrintFooter);
        group.writeEntry("Note Lines", mShowNoteLines);
        group.writeEntry("Exclude confidential", mExcludeConfidential);
        group.writeEntry("Exclude private", mExcludePrivate);
        mConfig->sync();
    } else {
        qCDebug(CALENDARSUPPORT_LOG) << "No config available in saveConfig!!!!";
    }
}

bool CalPrintPluginBase::useColors() const
{
    return mUseColors;
}

void CalPrintPluginBase::setUseColors(bool useColors)
{
    mUseColors = useColors;
}

bool CalPrintPluginBase::printFooter() const
{
    return mPrintFooter;
}

void CalPrintPluginBase::setPrintFooter(bool printFooter)
{
    mPrintFooter = printFooter;
}

QPageLayout::Orientation CalPrintPluginBase::orientation() const
{
    return mPrinter ? mPrinter->pageLayout().orientation() : QPageLayout::Portrait;
}

QColor CalPrintPluginBase::getTextColor(const QColor &c) const
{
    double luminance = (c.red() * 0.299) + (c.green() * 0.587) + (c.blue() * 0.114);
    return (luminance > 128.0) ? QColor(0, 0, 0) : QColor(255, 255, 255);
}

QTime CalPrintPluginBase::dayStart() const
{
    QTime start(8, 0, 0);
    QDateTime dayBegins = KCalPrefs::instance()->dayBegins();
    if (dayBegins.isValid()) {
        start = dayBegins.time();
    }
    return start;
}

void CalPrintPluginBase::setColorsByIncidenceCategory(QPainter &p, const KCalendarCore::Incidence::Ptr &incidence) const
{
    QColor bgColor = categoryBgColor(incidence);
    if (bgColor.isValid()) {
        p.setBrush(bgColor);
    }
    QColor tColor(getTextColor(bgColor));
    if (tColor.isValid()) {
        p.setPen(tColor);
    }
}

QColor CalPrintPluginBase::categoryColor(const QStringList &categories) const
{
    if (categories.isEmpty()) {
        return KCalPrefs::instance()->unsetCategoryColor();
    }
    // FIXME: Correctly treat events with multiple categories
    const QString cat = categories.at(0);
    QColor bgColor;
    if (cat.isEmpty()) {
        bgColor = KCalPrefs::instance()->unsetCategoryColor();
    } else {
        bgColor = KCalPrefs::instance()->categoryColor(cat);
    }
    return bgColor;
}

QColor CalPrintPluginBase::categoryBgColor(const KCalendarCore::Incidence::Ptr &incidence) const
{
    if (incidence) {
        QColor backColor = categoryColor(incidence->categories());
        if (incidence->type() == KCalendarCore::Incidence::TypeTodo) {
            if ((incidence.staticCast<KCalendarCore::Todo>())->isOverdue()) {
                backColor = QColor(255, 100, 100); // was KOPrefs::instance()->todoOverdueColor();
            }
        }
        return backColor;
    } else {
        return QColor();
    }
}

QString CalPrintPluginBase::holidayString(QDate date) const
{
    const QStringList lst = holiday(date);
    return lst.join(i18nc("@item:intext delimiter for joining holiday names", ","));
}

KCalendarCore::Event::Ptr CalPrintPluginBase::holidayEvent(QDate date) const
{
    QString hstring(holidayString(date));
    if (hstring.isEmpty()) {
        return KCalendarCore::Event::Ptr();
    }

    KCalendarCore::Event::Ptr holiday(new KCalendarCore::Event);
    holiday->setSummary(hstring);
    holiday->setCategories(i18n("Holiday"));

    QDateTime kdt(date, QTime(0, 0), Qt::LocalTime);
    holiday->setDtStart(kdt);
    holiday->setDtEnd(kdt);
    holiday->setAllDay(true);

    return holiday;
}

int CalPrintPluginBase::headerHeight() const
{
    if (mHeaderHeight >= 0) {
        return mHeaderHeight;
    } else if (orientation() == QPageLayout::Portrait) {
        return PORTRAIT_HEADER_HEIGHT;
    } else {
        return LANDSCAPE_HEADER_HEIGHT;
    }
}

void CalPrintPluginBase::setHeaderHeight(const int height)
{
    mHeaderHeight = height;
}

int CalPrintPluginBase::subHeaderHeight() const
{
    return mSubHeaderHeight;
}

void CalPrintPluginBase::setSubHeaderHeight(const int height)
{
    mSubHeaderHeight = height;
}

int CalPrintPluginBase::footerHeight() const
{
    if (!mPrintFooter) {
        return 0;
    }

    if (mFooterHeight >= 0) {
        return mFooterHeight;
    } else if (orientation() == QPageLayout::Portrait) {
        return PORTRAIT_FOOTER_HEIGHT;
    } else {
        return LANDSCAPE_FOOTER_HEIGHT;
    }
}

void CalPrintPluginBase::setFooterHeight(const int height)
{
    mFooterHeight = height;
}

int CalPrintPluginBase::margin() const
{
    return mMargin;
}

void CalPrintPluginBase::setMargin(const int margin)
{
    mMargin = margin;
}

int CalPrintPluginBase::padding() const
{
    return mPadding;
}

void CalPrintPluginBase::setPadding(const int padding)
{
    mPadding = padding;
}

int CalPrintPluginBase::borderWidth() const
{
    return mBorder;
}

void CalPrintPluginBase::setBorderWidth(const int borderwidth)
{
    mBorder = borderwidth;
}

void CalPrintPluginBase::drawBox(QPainter &p, int linewidth, QRect rect)
{
    QPen pen(p.pen());
    QPen oldpen(pen);
    // no border
    if (linewidth >= 0) {
        pen.setWidth(linewidth);
        p.setPen(pen);
    } else {
        p.setPen(Qt::NoPen);
    }
    p.drawRect(rect);
    p.setPen(oldpen);
}

void CalPrintPluginBase::drawShadedBox(QPainter &p, int linewidth, const QBrush &brush, QRect rect)
{
    QBrush oldbrush(p.brush());
    p.setBrush(brush);
    drawBox(p, linewidth, rect);
    p.setBrush(oldbrush);
}

void CalPrintPluginBase::printEventString(QPainter &p, QRect box, const QString &str, int flags)
{
    QRect newbox(box);
    newbox.adjust(3, 1, -1, -1);
    p.drawText(newbox, (flags == -1) ? (Qt::AlignTop | Qt::AlignLeft | Qt::TextWordWrap) : flags, str);
}

void CalPrintPluginBase::showEventBox(QPainter &p, int linewidth, QRect box, const KCalendarCore::Incidence::Ptr &incidence, const QString &str, int flags)
{
    QPen oldpen(p.pen());
    QBrush oldbrush(p.brush());
    QColor bgColor(categoryBgColor(incidence));
    if (mUseColors && bgColor.isValid()) {
        p.setBrush(bgColor);
    } else {
        p.setBrush(QColor(232, 232, 232));
    }
    drawBox(p, (linewidth > 0) ? linewidth : EVENT_BORDER_WIDTH, box);
    if (mUseColors && bgColor.isValid()) {
        p.setPen(getTextColor(bgColor));
    }
    printEventString(p, box, str, flags);
    p.setPen(oldpen);
    p.setBrush(oldbrush);
}

void CalPrintPluginBase::drawSubHeaderBox(QPainter &p, const QString &str, QRect box)
{
    drawShadedBox(p, BOX_BORDER_WIDTH, QColor(232, 232, 232), box);
    QFont oldfont(p.font());
    p.setFont(QFont(QStringLiteral("sans-serif"), 10, QFont::Bold));
    p.drawText(box, Qt::AlignHCenter | Qt::AlignTop, str);
    p.setFont(oldfont);
}

void CalPrintPluginBase::drawVerticalBox(QPainter &p, int linewidth, QRect box, const QString &str, int flags)
{
    p.save();
    p.rotate(-90);
    QRect rotatedBox(-box.top() - box.height(), box.left(), box.height(), box.width());
    showEventBox(p, linewidth, rotatedBox, KCalendarCore::Incidence::Ptr(), str, (flags == -1) ? Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine : flags);

    p.restore();
}

/*
 * Return value: If expand, bottom of the printed box, otherwise vertical end
 * of the printed contents inside the box.
 */
int CalPrintPluginBase::drawBoxWithCaption(QPainter &p,
                                           QRect allbox,
                                           const QString &caption,
                                           const QString &contents,
                                           bool sameLine,
                                           bool expand,
                                           const QFont &captionFont,
                                           const QFont &textFont,
                                           bool richContents)
{
    QFont oldFont(p.font());
    //   QFont captionFont( "sans-serif", 11, QFont::Bold );
    //   QFont textFont( "sans-serif", 11, QFont::Normal );
    //   QFont captionFont( "Tahoma", 11, QFont::Bold );
    //   QFont textFont( "Tahoma", 11, QFont::Normal );

    QRect box(allbox);

    // Bounding rectangle for caption, single-line, clip on the right
    QRect captionBox(box.left() + padding(), box.top() + padding(), 0, 0);
    p.setFont(captionFont);
    captionBox = p.boundingRect(captionBox, Qt::AlignLeft | Qt::AlignTop | Qt::TextSingleLine, caption);
    p.setFont(oldFont);
    if (captionBox.right() > box.right()) {
        captionBox.setRight(box.right());
    }
    if (expand && captionBox.bottom() + padding() > box.bottom()) {
        box.setBottom(captionBox.bottom() + padding());
    }

    // Bounding rectangle for the contents (if any), word break, clip on the bottom
    QRect textBox(captionBox);
    if (!contents.isEmpty()) {
        if (sameLine) {
            textBox.setLeft(captionBox.right() + padding());
        } else {
            textBox.setTop(captionBox.bottom() + padding());
        }
        textBox.setRight(box.right());
    }
    drawBox(p, BOX_BORDER_WIDTH, box);
    p.setFont(captionFont);
    p.drawText(captionBox, Qt::AlignLeft | Qt::AlignTop | Qt::TextSingleLine, caption);

    if (!contents.isEmpty()) {
        if (sameLine) {
            QString contentText = toPlainText(contents);
            p.setFont(textFont);
            p.drawText(textBox, Qt::AlignLeft | Qt::AlignTop | Qt::TextSingleLine, contentText);
        } else {
            QTextDocument rtb;
            int borderWidth = 2 * BOX_BORDER_WIDTH;
            if (richContents) {
                rtb.setHtml(contents);
            } else {
                rtb.setPlainText(contents);
            }
            int boxHeight = allbox.height();
            if (!sameLine) {
                boxHeight -= captionBox.height();
            }
            rtb.setPageSize(QSize(textBox.width(), boxHeight));
            rtb.setDefaultFont(textFont);
            p.save();
            p.translate(textBox.x() - borderWidth, textBox.y());
            QRect clipBox(0, 0, box.width(), boxHeight);
            QAbstractTextDocumentLayout::PaintContext ctx;
            ctx.palette.setColor(QPalette::Text, p.pen().color());
            p.setClipRect(clipBox);
            ctx.clip = clipBox;
            rtb.documentLayout()->draw(&p, ctx);
            p.restore();
            textBox.setBottom(textBox.y() + rtb.documentLayout()->documentSize().height());
        }
    }
    p.setFont(oldFont);

    if (expand) {
        return box.bottom();
    } else {
        return textBox.bottom();
    }
}

int CalPrintPluginBase::drawHeader(QPainter &p, const QString &title, QDate month1, QDate month2, QRect allbox, bool expand, QColor backColor)
{
    // print previous month for month view, print current for to-do, day and week
    int smallMonthWidth = (allbox.width() / 4) - 10;
    if (smallMonthWidth > 100) {
        smallMonthWidth = 100;
    }

    QRect box(allbox);
    QRect textRect(allbox);

    QFont oldFont(p.font());
    QFont newFont(QStringLiteral("sans-serif"), (textRect.height() < 60) ? 16 : 18, QFont::Bold);
    if (expand) {
        p.setFont(newFont);
        QRect boundingR = p.boundingRect(textRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap, title);
        p.setFont(oldFont);
        int h = boundingR.height();
        if (h > allbox.height()) {
            box.setHeight(h);
            textRect.setHeight(h);
        }
    }

    if (!backColor.isValid()) {
        backColor = QColor(232, 232, 232);
    }

    drawShadedBox(p, BOX_BORDER_WIDTH, backColor, box);

    const auto oldPen {p.pen()};
    p.setPen(getTextColor(backColor));

    // prev month left, current month centered, next month right
    QRect monthbox2(box.right() - 10 - smallMonthWidth, box.top(), smallMonthWidth, box.height());
    if (month2.isValid()) {
        drawSmallMonth(p, QDate(month2.year(), month2.month(), 1), monthbox2);
        textRect.setRight(monthbox2.left());
    }
    QRect monthbox1(box.left() + 10, box.top(), smallMonthWidth, box.height());
    if (month1.isValid()) {
        drawSmallMonth(p, QDate(month1.year(), month1.month(), 1), monthbox1);
        textRect.setLeft(monthbox1.right());
    }

    // Set the margins
    p.setFont(newFont);
    p.drawText(textRect, Qt::AlignCenter | Qt::AlignVCenter | Qt::TextWordWrap, title);

    p.setPen(oldPen);
    p.setFont(oldFont);

    return textRect.bottom();
}

int CalPrintPluginBase::drawFooter(QPainter &p, QRect footbox)
{
    QFont oldfont(p.font());
    p.setFont(QFont(QStringLiteral("sans-serif"), 6));
    QString dateStr = QLocale::system().toString(QDateTime::currentDateTime(), QLocale::LongFormat);
    p.drawText(footbox, Qt::AlignCenter | Qt::AlignVCenter | Qt::TextSingleLine, i18nc("print date: formatted-datetime", "printed: %1", dateStr));
    p.setFont(oldfont);

    return footbox.bottom();
}

void CalPrintPluginBase::drawSmallMonth(QPainter &p, QDate qd, QRect box)
{
    int weekdayCol = weekdayColumn(qd.dayOfWeek());
    int month = qd.month();
    QDate monthDate(QDate(qd.year(), qd.month(), 1));
    // correct begin of week
    QDate monthDate2(monthDate.addDays(-weekdayCol));

    double cellWidth = double(box.width()) / double(7);
    int rownr = 3 + (qd.daysInMonth() + weekdayCol - 1) / 7;
    // 3 Pixel after month name, 2 after day names, 1 after the calendar
    double cellHeight = (box.height() - 5) / rownr;
    QFont oldFont(p.font());
    auto newFont = QFont(QStringLiteral("sans-serif"));
    newFont.setPixelSize(cellHeight);
    p.setFont(newFont);

    // draw the title
    QRect titleBox(box);
    titleBox.setHeight(p.fontMetrics().height());
    p.drawText(titleBox, Qt::AlignTop | Qt::AlignHCenter, QLocale::system().monthName(month));

    // draw days of week
    QRect wdayBox(box);
    wdayBox.setTop(int(box.top() + 3 + cellHeight));
    wdayBox.setHeight(int(2 * cellHeight) - int(cellHeight));

    for (int col = 0; col < 7; ++col) {
        QString tmpStr = QLocale::system().dayName(monthDate2.dayOfWeek())[0].toUpper();
        wdayBox.setLeft(int(box.left() + col * cellWidth));
        wdayBox.setRight(int(box.left() + (col + 1) * cellWidth));
        p.drawText(wdayBox, Qt::AlignCenter, tmpStr);
        monthDate2 = monthDate2.addDays(1);
    }

    // draw separator line
    int calStartY = wdayBox.bottom() + 2;
    p.drawLine(box.left(), calStartY, box.right(), calStartY);
    monthDate = monthDate.addDays(-weekdayCol);

    for (int row = 0; row < (rownr - 2); row++) {
        for (int col = 0; col < 7; col++) {
            if (monthDate.month() == month) {
                QRect dayRect(int(box.left() + col * cellWidth), int(calStartY + row * cellHeight), 0, 0);
                dayRect.setRight(int(box.left() + (col + 1) * cellWidth));
                dayRect.setBottom(int(calStartY + (row + 1) * cellHeight));
                p.drawText(dayRect, Qt::AlignCenter, QString::number(monthDate.day()));
            }
            monthDate = monthDate.addDays(1);
        }
    }
    p.setFont(oldFont);
}

/*
 * This routine draws a header box over the main part of the calendar
 * containing the days of the week.
 */
void CalPrintPluginBase::drawDaysOfWeek(QPainter &p, QDate fromDate, QDate toDate, QRect box)
{
    double cellWidth = double(box.width() - 1) / double(fromDate.daysTo(toDate) + 1);
    QDate cellDate(fromDate);
    QRect dateBox(box);
    int i = 0;

    while (cellDate <= toDate) {
        dateBox.setLeft(box.left() + int(i * cellWidth));
        dateBox.setRight(box.left() + int((i + 1) * cellWidth));
        drawDaysOfWeekBox(p, cellDate, dateBox);
        cellDate = cellDate.addDays(1);
        ++i;
    }
}

void CalPrintPluginBase::drawDaysOfWeekBox(QPainter &p, QDate qd, QRect box)
{
    drawSubHeaderBox(p, QLocale::system().dayName(qd.dayOfWeek()), box);
}

void CalPrintPluginBase::drawTimeLine(QPainter &p, QTime fromTime, QTime toTime, QRect box)
{
    drawBox(p, BOX_BORDER_WIDTH, box);

    int totalsecs = fromTime.secsTo(toTime);
    float minlen = (float)box.height() * 60. / (float)totalsecs;
    float cellHeight = (60. * (float)minlen);
    float currY = box.top();
    // TODO: Don't use half of the width, but less, for the minutes!
    int xcenter = box.left() + box.width() / 2;

    QTime curTime(fromTime);
    QTime endTime(toTime);
    if (fromTime.minute() > 30) {
        curTime = QTime(fromTime.hour() + 1, 0, 0);
    } else if (fromTime.minute() > 0) {
        curTime = QTime(fromTime.hour(), 30, 0);
        float yy = currY + minlen * (float)fromTime.secsTo(curTime) / 60.;
        p.drawLine(xcenter, (int)yy, box.right(), (int)yy);
        curTime = QTime(fromTime.hour() + 1, 0, 0);
    }
    currY += (float(fromTime.secsTo(curTime) * minlen) / 60.);

    while (curTime < endTime) {
        p.drawLine(box.left(), (int)currY, box.right(), (int)currY);
        int newY = (int)(currY + cellHeight / 2.);
        QString numStr;
        if (newY < box.bottom()) {
            QFont oldFont(p.font());
            // draw the time:
            if (!QLocale().timeFormat().contains(QLatin1String("AP"))) { // 12h clock
                p.drawLine(xcenter, (int)newY, box.right(), (int)newY);
                numStr.setNum(curTime.hour());
                if (cellHeight > 30) {
                    p.setFont(QFont(QStringLiteral("sans-serif"), 14, QFont::Bold));
                } else {
                    p.setFont(QFont(QStringLiteral("sans-serif"), 12, QFont::Bold));
                }
                p.drawText(box.left() + 4, (int)currY + 2, box.width() / 2 - 2, (int)cellHeight, Qt::AlignTop | Qt::AlignRight, numStr);
                p.setFont(QFont(QStringLiteral("helvetica"), 10, QFont::Normal));
                p.drawText(xcenter + 4, (int)currY + 2, box.width() / 2 + 2, (int)(cellHeight / 2) - 3, Qt::AlignTop | Qt::AlignLeft, QStringLiteral("00"));
            } else {
                p.drawLine(box.left(), (int)newY, box.right(), (int)newY);
                QTime time(curTime.hour(), 0);
                numStr = QLocale::system().toString(time, QLocale::ShortFormat);
                if (box.width() < 60) {
                    p.setFont(QFont(QStringLiteral("sans-serif"), 7, QFont::Bold)); // for weekprint
                } else {
                    p.setFont(QFont(QStringLiteral("sans-serif"), 12, QFont::Bold)); // for dayprint
                }
                p.drawText(box.left() + 2, (int)currY + 2, box.width() - 4, (int)cellHeight / 2 - 3, Qt::AlignTop | Qt::AlignLeft, numStr);
            }
            currY += cellHeight;
            p.setFont(oldFont);
        } // enough space for half-hour line and time
        if (curTime.secsTo(endTime) > 3600) {
            curTime = curTime.addSecs(3600);
        } else {
            curTime = endTime;
        }
    }
}

void CalPrintPluginBase::drawAgendaDayBox(QPainter &p,
                                          const KCalendarCore::Event::List &events,
                                          QDate qd,
                                          bool expandable,
                                          QTime fromTime,
                                          QTime toTime,
                                          QRect oldbox,
                                          bool includeDescription,
                                          bool includeCategories,
                                          bool excludeTime,
                                          const QList<QDate> &workDays)
{
    QTime myFromTime, myToTime;
    if (fromTime.isValid()) {
        myFromTime = fromTime;
    } else {
        myFromTime = QTime(0, 0, 0);
    }
    if (toTime.isValid()) {
        myToTime = toTime;
    } else {
        myToTime = QTime(23, 59, 59);
    }

    if (!workDays.contains(qd)) {
        drawShadedBox(p, BOX_BORDER_WIDTH, sHolidayBackground, oldbox);
    } else {
        drawBox(p, BOX_BORDER_WIDTH, oldbox);
    }
    QRect box(oldbox);
    // Account for the border with and cut away that margin from the interior
    //   box.setRight( box.right()-BOX_BORDER_WIDTH );

    if (expandable) {
        // Adapt start/end times to include complete events
        for (const KCalendarCore::Event::Ptr &event : std::as_const(events)) {
            Q_ASSERT(event);
            if (!event
                || (mExcludeConfidential && event->secrecy() == KCalendarCore::Incidence::SecrecyConfidential)
                || (mExcludePrivate && event->secrecy() == KCalendarCore::Incidence::SecrecyPrivate)) {
                continue;
            }
            // skip items without times so that we do not adjust for all day items
            if (event->allDay()) {
                continue;
            }
            if (event->dtStart().time() < myFromTime) {
                myFromTime = event->dtStart().time();
            }
            if (event->dtEnd().time() > myToTime) {
                myToTime = event->dtEnd().time();
            }
        }
    }

    // calculate the height of a cell and of a minute
    int totalsecs = myFromTime.secsTo(myToTime);
    float minlen = box.height() * 60. / totalsecs;
    float cellHeight = 60. * minlen;
    float currY = box.top();

    // print grid:
    QTime curTime(QTime(myFromTime.hour(), 0, 0));
    currY += myFromTime.secsTo(curTime) * minlen / 60;

    while (curTime < myToTime && curTime.isValid()) {
        if (currY > box.top()) {
            p.drawLine(box.left(), int(currY), box.right(), int(currY));
        }
        currY += cellHeight / 2;
        if ((currY > box.top()) && (currY < box.bottom())) {
            // enough space for half-hour line
            QPen oldPen(p.pen());
            p.setPen(QColor(192, 192, 192));
            p.drawLine(box.left(), int(currY), box.right(), int(currY));
            p.setPen(oldPen);
        }
        if (curTime.secsTo(myToTime) > 3600) {
            curTime = curTime.addSecs(3600);
        } else {
            curTime = myToTime;
        }
        currY += cellHeight / 2;
    }

    QDateTime startPrintDate = QDateTime(qd, myFromTime);
    QDateTime endPrintDate = QDateTime(qd, myToTime);

    // Calculate horizontal positions and widths of events taking into account
    // overlapping events

    QList<CellItem *> cells;

    for (const KCalendarCore::Event::Ptr &event : std::as_const(events)) {
        if (!event
            || (mExcludeConfidential && event->secrecy() == KCalendarCore::Incidence::SecrecyConfidential)
            || (mExcludePrivate && event->secrecy() == KCalendarCore::Incidence::SecrecyPrivate)) {
            continue;
        }
        if (event->allDay()) {
            continue;
        }
        QList<QDateTime> times = event->startDateTimesForDate(qd, QTimeZone::systemTimeZone());
        cells.reserve(times.count());
        for (auto it = times.constBegin(); it != times.constEnd(); ++it) {
            cells.append(new PrintCellItem(event, (*it).toLocalTime(), event->endDateForStart(*it).toLocalTime()));
        }
    }

    QListIterator<CellItem *> it1(cells);
    while (it1.hasNext()) {
        CellItem *placeItem = it1.next();
        CellItem::placeItem(cells, placeItem);
    }

    QListIterator<CellItem *> it2(cells);
    while (it2.hasNext()) {
        auto placeItem = static_cast<PrintCellItem *>(it2.next());
        drawAgendaItem(placeItem, p, startPrintDate, endPrintDate, minlen, box, includeDescription, includeCategories, excludeTime);
    }
}

void CalPrintPluginBase::drawAgendaItem(PrintCellItem *item,
                                        QPainter &p,
                                        const QDateTime &startPrintDate,
                                        const QDateTime &endPrintDate,
                                        float minlen,
                                        QRect box,
                                        bool includeDescription,
                                        bool includeCategories,
                                        bool excludeTime)
{
    KCalendarCore::Event::Ptr event = item->event();

    // start/end of print area for event
    QDateTime startTime = item->start();
    QDateTime endTime = item->end();
    if ((startTime < endPrintDate && endTime > startPrintDate) || (endTime > startPrintDate && startTime < endPrintDate)) {
        if (startTime < startPrintDate) {
            startTime = startPrintDate;
        }
        if (endTime > endPrintDate) {
            endTime = endPrintDate;
        }
        int currentWidth = box.width() / item->subCells();
        int currentX = box.left() + item->subCell() * currentWidth;
        int currentYPos = int(box.top() + startPrintDate.secsTo(startTime) * minlen / 60.);
        int currentHeight = int(box.top() + startPrintDate.secsTo(endTime) * minlen / 60.) - currentYPos;

        QRect eventBox(currentX, currentYPos, currentWidth, currentHeight);
        QString str;
        if (excludeTime) {
            if (event->location().isEmpty()) {
                str = cleanStr(event->summary());
            } else {
                str = i18nc("summary, location", "%1, %2", cleanStr(event->summary()), cleanStr(event->location()));
            }
        } else {
            if (event->location().isEmpty()) {
                str = i18nc("starttime - endtime summary",
                            "%1-%2 %3",
                            QLocale::system().toString(item->start().time(), QLocale::ShortFormat),
                            QLocale::system().toString(item->end().time(), QLocale::ShortFormat),
                            cleanStr(event->summary()));
            } else {
                str = i18nc("starttime - endtime summary, location",
                            "%1-%2 %3, %4",
                            QLocale::system().toString(item->start().time(), QLocale::ShortFormat),
                            QLocale::system().toString(item->end().time(), QLocale::ShortFormat),
                            cleanStr(event->summary()),
                            cleanStr(event->location()));
            }
        }
        if (includeCategories && !event->categoriesStr().isEmpty()) {
                str = i18nc("summary, categories", "%1, %2", str, event->categoriesStr());
        }
        if (includeDescription && !event->description().isEmpty()) {
            str += QLatin1Char('\n');
            if (event->descriptionIsRich()) {
                str += toPlainText(event->description());
            } else {
                str += event->description();
            }
        }
        QFont oldFont(p.font());
        if (eventBox.height() < 24) {
            if (eventBox.height() < 12) {
                if (eventBox.height() < 8) {
                    p.setFont(QFont(QStringLiteral("sans-serif"), 4));
                } else {
                    p.setFont(QFont(QStringLiteral("sans-serif"), 5));
                }
            } else {
                p.setFont(QFont(QStringLiteral("sans-serif"), 6));
            }
        } else {
            p.setFont(QFont(QStringLiteral("sans-serif"), 8));
        }
        showEventBox(p, EVENT_BORDER_WIDTH, eventBox, event, str);
        p.setFont(oldFont);
    }
}

void CalPrintPluginBase::drawDayBox(QPainter &p,
                                    QDate qd,
                                    QTime fromTime,
                                    QTime toTime,
                                    QRect box,
                                    bool fullDate,
                                    bool printRecurDaily,
                                    bool printRecurWeekly,
                                    bool singleLineLimit,
                                    bool includeDescription,
                                    bool includeCategories)
{
    QString dayNumStr;
    const auto local = QLocale::system();

    QTime myFromTime, myToTime;
    if (fromTime.isValid()) {
        myFromTime = fromTime;
    } else {
        myFromTime = QTime(0, 0, 0);
    }
    if (toTime.isValid()) {
        myToTime = toTime;
    } else {
        myToTime = QTime(23, 59, 59);
    }

    if (fullDate) {
        dayNumStr = i18nc("weekday, shortmonthname daynumber",
                          "%1, %2 %3",
                          QLocale::system().dayName(qd.dayOfWeek()),
                          QLocale::system().monthName(qd.month(), QLocale::ShortFormat),
                          QString::number(qd.day()));
    } else {
        dayNumStr = QString::number(qd.day());
    }

    QRect subHeaderBox(box);
    subHeaderBox.setHeight(mSubHeaderHeight);
    drawShadedBox(p, BOX_BORDER_WIDTH, p.background(), box);
    drawShadedBox(p, 0, QColor(232, 232, 232), subHeaderBox);
    drawBox(p, BOX_BORDER_WIDTH, box);
    QString hstring(holidayString(qd));
    const QFont oldFont(p.font());

    QRect headerTextBox(subHeaderBox);
    headerTextBox.setLeft(subHeaderBox.left() + 5);
    headerTextBox.setRight(subHeaderBox.right() - 5);
    if (!hstring.isEmpty()) {
        p.setFont(QFont(QStringLiteral("sans-serif"), 8, QFont::Bold, true));
        p.drawText(headerTextBox, Qt::AlignLeft | Qt::AlignVCenter, hstring);
    }
    p.setFont(QFont(QStringLiteral("sans-serif"), 10, QFont::Bold));
    p.drawText(headerTextBox, Qt::AlignRight | Qt::AlignVCenter, dayNumStr);

    const KCalendarCore::Event::List eventList =
        mCalendar->events(qd, QTimeZone::systemTimeZone(), KCalendarCore::EventSortStartDate, KCalendarCore::SortDirectionAscending);

    QString timeText;
    p.setFont(QFont(QStringLiteral("sans-serif"), 7));

    int textY = mSubHeaderHeight; // gives the relative y-coord of the next printed entry
    unsigned int visibleEventsCounter = 0;
    for (const KCalendarCore::Event::Ptr &currEvent : std::as_const(eventList)) {
        Q_ASSERT(currEvent);
        if (!currEvent->allDay()) {
            if (currEvent->dtEnd().toLocalTime().time() <= myFromTime || currEvent->dtStart().toLocalTime().time() > myToTime) {
                continue;
            }
        }
        if ((!printRecurDaily && currEvent->recurrenceType() == KCalendarCore::Recurrence::rDaily)
            || (!printRecurWeekly && currEvent->recurrenceType() == KCalendarCore::Recurrence::rWeekly)) {
            continue;
        }
        if ((mExcludeConfidential && currEvent->secrecy() == KCalendarCore::Incidence::SecrecyConfidential)
            || (mExcludePrivate && currEvent->secrecy() == KCalendarCore::Incidence::SecrecyPrivate)) {
            continue;
        }
        if (currEvent->allDay() || currEvent->isMultiDay()) {
            timeText.clear();
        } else {
            timeText = local.toString(currEvent->dtStart().toLocalTime().time(), QLocale::ShortFormat) + QLatin1Char(' ');
        }
        p.save();
        if (mUseColors) {
            setColorsByIncidenceCategory(p, currEvent);
        }
        QString summaryStr = currEvent->summary();
        if (!currEvent->location().isEmpty()) {
            summaryStr = i18nc("summary, location", "%1, %2", summaryStr, currEvent->location());
        }
        if (includeCategories && !currEvent->categoriesStr().isEmpty()) {
            summaryStr = i18nc("summary, categories", "%1, %2", summaryStr, currEvent->categoriesStr());
        }
        drawIncidence(p, box, timeText, summaryStr, currEvent->description(), textY, singleLineLimit, includeDescription, currEvent->descriptionIsRich());
        p.restore();
        visibleEventsCounter++;

        if (textY >= box.height()) {
            const QChar downArrow(0x21e3);

            const unsigned int invisibleIncidences = (eventList.count() - visibleEventsCounter) + mCalendar->todos(qd).count();
            if (invisibleIncidences > 0) {
                const QString warningMsg = QStringLiteral("%1 (%2)").arg(downArrow).arg(invisibleIncidences);

                QFontMetrics fm(p.font());
                QRect msgRect = fm.boundingRect(warningMsg);
                msgRect.setRect(box.right() - msgRect.width() - 2, box.bottom() - msgRect.height() - 2, msgRect.width(), msgRect.height());

                p.save();
                p.setPen(Qt::red); // krazy:exclude=qenums we don't allow custom print colors
                p.drawText(msgRect, Qt::AlignLeft, warningMsg);
                p.restore();
            }
            break;
        }
    }

    if (textY < box.height()) {
        KCalendarCore::Todo::List todos = mCalendar->todos(qd);
        for (const KCalendarCore::Todo::Ptr &todo : std::as_const(todos)) {
            if (!todo->allDay()) {
                if ((todo->hasDueDate() && todo->dtDue().toLocalTime().time() <= myFromTime)
                    || (todo->hasStartDate() && todo->dtStart().toLocalTime().time() > myToTime)) {
                    continue;
                }
            }
            if ((!printRecurDaily && todo->recurrenceType() == KCalendarCore::Recurrence::rDaily)
                || (!printRecurWeekly && todo->recurrenceType() == KCalendarCore::Recurrence::rWeekly)) {
                continue;
            }
            if ((mExcludeConfidential && todo->secrecy() == KCalendarCore::Incidence::SecrecyConfidential)
                || (mExcludePrivate && todo->secrecy() == KCalendarCore::Incidence::SecrecyPrivate)) {
                continue;
            }
            if (todo->hasStartDate() && !todo->allDay()) {
                timeText = QLocale().toString(todo->dtStart().toLocalTime().time(), QLocale::ShortFormat) + QLatin1Char(' ');
            } else {
                timeText.clear();
            }
            p.save();
            if (mUseColors) {
                setColorsByIncidenceCategory(p, todo);
            }
            QString summaryStr = todo->summary();
            if (!todo->location().isEmpty()) {
                summaryStr = i18nc("summary, location", "%1, %2", summaryStr, todo->location());
            }

            QString str;
            if (todo->hasDueDate()) {
                if (!todo->allDay()) {
                    str = i18nc("to-do summary (Due: datetime)",
                                "%1 (Due: %2)",
                                summaryStr,
                                QLocale().toString(todo->dtDue().toLocalTime(), QLocale::ShortFormat));
                } else {
                    str = i18nc("to-do summary (Due: date)",
                                "%1 (Due: %2)",
                                summaryStr,
                                QLocale().toString(todo->dtDue().toLocalTime().date(), QLocale::ShortFormat));
                }
            } else {
                str = summaryStr;
            }
            drawIncidence(p, box, timeText, i18n("To-do: %1", str), todo->description(), textY, singleLineLimit, includeDescription, todo->descriptionIsRich());
            p.restore();
        }
    }
    if (mShowNoteLines) {
        drawNoteLines(p, box, box.y() + textY);
    }

    p.setFont(oldFont);
}

void CalPrintPluginBase::drawIncidence(QPainter &p,
                                       QRect dayBox,
                                       const QString &time,
                                       const QString &summary,
                                       const QString &description,
                                       int &textY,
                                       bool singleLineLimit,
                                       bool includeDescription,
                                       bool richDescription)
{
    qCDebug(CALENDARSUPPORT_LOG) << "summary =" << summary << ", singleLineLimit=" << singleLineLimit;

    int flags = Qt::AlignLeft | Qt::OpaqueMode;
    QFontMetrics fm = p.fontMetrics();
    const int borderWidth = p.pen().width() + 1;
    QRect timeBound = p.boundingRect(dayBox.x() + borderWidth, dayBox.y() + textY, dayBox.width(), fm.lineSpacing(), flags, time);

    int summaryWidth = time.isEmpty() ? 0 : timeBound.width() + 3;
    QRect summaryBound =
        QRect(dayBox.x() + borderWidth + summaryWidth, dayBox.y() + textY + 1, dayBox.width() - summaryWidth - (borderWidth * 2), dayBox.height() - textY);

    QString summaryText = summary;
    QString descText = toPlainText(description);
    bool boxOverflow = false;

    if (singleLineLimit) {
        if (includeDescription && !descText.isEmpty()) {
            summaryText += QLatin1String(", ") + descText;
        }
        int totalHeight = fm.lineSpacing() + borderWidth;
        int textBoxHeight = (totalHeight > (dayBox.height() - textY)) ? dayBox.height() - textY : totalHeight;
        summaryBound.setHeight(textBoxHeight);
        QRect lineRect(dayBox.x() + borderWidth, dayBox.y() + textY, dayBox.width() - (borderWidth * 2), textBoxHeight);
        drawBox(p, 1, lineRect);
        if (!time.isEmpty()) {
            p.drawText(timeBound, flags, time);
        }
        p.drawText(summaryBound, flags, summaryText);
    } else {
        QTextDocument textDoc;
        QTextCursor textCursor(&textDoc);
        textCursor.insertText(summaryText);
        if (includeDescription && !description.isEmpty()) {
            textCursor.insertText(QStringLiteral("\n"));
            if (richDescription) {
                textCursor.insertHtml(description);
            } else {
                textCursor.insertText(descText);
            }
        }
        textDoc.setPageSize(QSize(summaryBound.width(), summaryBound.height()));
        p.save();
        QRect clipBox(0, 0, summaryBound.width(), summaryBound.height());
        p.translate(summaryBound.x(), summaryBound.y());
        summaryBound.setHeight(textDoc.documentLayout()->documentSize().height());
        if (summaryBound.bottom() > dayBox.bottom()) {
            summaryBound.setBottom(dayBox.bottom());
        }
        clipBox.setHeight(summaryBound.height());
        p.restore();

        p.save();
        QRect backBox(timeBound.x(), timeBound.y(), dayBox.width() - (borderWidth * 2), clipBox.height());
        drawBox(p, 1, backBox);

        if (!time.isEmpty()) {
            if (timeBound.bottom() > dayBox.bottom()) {
                timeBound.setBottom(dayBox.bottom());
            }
            timeBound.moveTop(timeBound.y() + (summaryBound.height() - timeBound.height()) / 2);
            p.drawText(timeBound, flags, time);
        }
        p.translate(summaryBound.x(), summaryBound.y());

        QAbstractTextDocumentLayout::PaintContext ctx;
        ctx.palette.setColor(QPalette::Text, p.pen().color());
        p.setClipRect(clipBox);
        ctx.clip = clipBox;
        textDoc.documentLayout()->draw(&p, ctx);

        p.restore();
        boxOverflow = textDoc.pageCount() > 1;
    }
    if (summaryBound.bottom() < dayBox.bottom()) {
        QPen oldPen(p.pen());
        p.setPen(QPen());
        p.drawLine(dayBox.x(), summaryBound.bottom(), dayBox.x() + dayBox.width(), summaryBound.bottom());
        p.setPen(oldPen);
    }
    textY += summaryBound.height();

    // show that we have overflowed the box
    if (boxOverflow) {
        QPolygon poly(3);
        int x = dayBox.x() + dayBox.width();
        int y = dayBox.y() + dayBox.height();
        poly.setPoint(0, x - 10, y);
        poly.setPoint(1, x, y - 10);
        poly.setPoint(2, x, y);
        QBrush oldBrush(p.brush());
        p.setBrush(QBrush(Qt::black));
        p.drawPolygon(poly);
        p.setBrush(oldBrush);
        textY = dayBox.height();
    }
}

class MonthEventStruct
{
public:
    MonthEventStruct()
        : event(nullptr)
    {
    }

    MonthEventStruct(const QDateTime &s, const QDateTime &e, const KCalendarCore::Event::Ptr &ev)
    {
        event = ev;
        start = s;
        end = e;
        if (event->allDay()) {
            start = QDateTime(start.date(), QTime(0, 0, 0));
            end = QDateTime(end.date().addDays(1), QTime(0, 0, 0)).addSecs(-1);
        }
    }

    bool operator<(const MonthEventStruct &mes)
    {
        return start < mes.start;
    }

    QDateTime start;
    QDateTime end;
    KCalendarCore::Event::Ptr event;
};

void CalPrintPluginBase::drawMonth(QPainter &p,
                                   QDate dt,
                                   QRect box,
                                   int maxdays,
                                   int subDailyFlags,
                                   int holidaysFlags)
{
    p.save();
    QRect subheaderBox(box);
    subheaderBox.setHeight(subHeaderHeight());
    QRect borderBox(box);
    borderBox.setTop(subheaderBox.bottom() + 1);
    drawSubHeaderBox(p, QLocale::system().monthName(dt.month()), subheaderBox);
    // correct for half the border width
    int correction = (BOX_BORDER_WIDTH /*-1*/) / 2;
    QRect daysBox(borderBox);
    daysBox.adjust(correction, correction, -correction, -correction);

    int daysinmonth = dt.daysInMonth();
    if (maxdays <= 0) {
        maxdays = daysinmonth;
    }

    int d;
    float dayheight = float(daysBox.height()) / float(maxdays);

    QColor holidayColor(240, 240, 240);
    QColor workdayColor(255, 255, 255);
    int dayNrWidth = p.fontMetrics().boundingRect(QStringLiteral("99")).width();

    // Fill the remaining space (if a month has less days than others) with a crossed-out pattern
    if (daysinmonth < maxdays) {
        QRect dayBox(box.left(), daysBox.top() + qRound(dayheight * daysinmonth), box.width(), 0);
        dayBox.setBottom(daysBox.bottom());
        p.fillRect(dayBox, Qt::DiagCrossPattern);
    }
    // Backgrounded boxes for each day, plus day numbers
    QBrush oldbrush(p.brush());

    QList<QDate> workDays;

    {
        QDate startDate(dt.year(), dt.month(), 1);
        QDate endDate(dt.year(), dt.month(), daysinmonth);

        workDays = CalendarSupport::workDays(startDate, endDate);
    }

    for (d = 0; d < daysinmonth; ++d) {
        QDate day(dt.year(), dt.month(), d + 1);
        QRect dayBox(daysBox.left() /*+rand()%50*/, daysBox.top() + qRound(dayheight * d), daysBox.width() /*-rand()%50*/, 0);
        // FIXME: When using a border width of 0 for event boxes,
        // don't let the rectangles overlap, i.e. subtract 1 from the top or bottom!
        dayBox.setBottom(daysBox.top() + qRound(dayheight * (d + 1)) - 1);

        p.setBrush(workDays.contains(day) ? workdayColor : holidayColor);
        p.drawRect(dayBox);
        QRect dateBox(dayBox);
        dateBox.setWidth(dayNrWidth + 3);
        p.drawText(dateBox, Qt::AlignRight | Qt::AlignVCenter | Qt::TextSingleLine, QString::number(d + 1));
    }
    p.setBrush(oldbrush);
    int xstartcont = box.left() + dayNrWidth + 5;

    QDate start(dt.year(), dt.month(), 1);
    QDate end = start.addMonths(1);
    end = end.addDays(-1);

    const KCalendarCore::Event::List events = mCalendar->events(start, end);
    QMap<int, QStringList> textEvents;
    QList<CellItem *> timeboxItems;

    // 1) For multi-day events, show boxes spanning several cells, use CellItem
    //    print the summary vertically
    // 2) For sub-day events, print the concated summaries into the remaining
    //    space of the box (optional, depending on the given flags)
    // 3) Draw some kind of timeline showing free and busy times

    // Holidays
    // QList<KCalendarCore::Event::Ptr> holidays;
    for (QDate d(start); d <= end; d = d.addDays(1)) {
        KCalendarCore::Event::Ptr e = holidayEvent(d);
        if (e) {
            // holidays.append(e);
            if (holidaysFlags & TimeBoxes) {
                timeboxItems.append(new PrintCellItem(e, QDateTime(d, QTime(0, 0, 0)), QDateTime(d.addDays(1), QTime(0, 0, 0))));
            }
            if (holidaysFlags & Text) {
                textEvents[d.day()] << e->summary();
            }
        }
    }

    QVector<MonthEventStruct> monthentries;

    for (const KCalendarCore::Event::Ptr &e : std::as_const(events)) {
        if (!e
            || (mExcludeConfidential && e->secrecy() == KCalendarCore::Incidence::SecrecyConfidential)
            || (mExcludePrivate && e->secrecy() == KCalendarCore::Incidence::SecrecyPrivate)) {
            continue;
        }
        if (e->recurs()) {
            if (e->recursOn(start, QTimeZone::systemTimeZone())) {
                // This occurrence has possibly started before the beginning of the
                // month, so obtain the start date before the beginning of the month
                QList<QDateTime> starttimes = e->startDateTimesForDate(start, QTimeZone::systemTimeZone());
                for (auto it = starttimes.constBegin(); it != starttimes.constEnd(); ++it) {
                    monthentries.append(MonthEventStruct((*it).toLocalTime(), e->endDateForStart(*it).toLocalTime(), e));
                }
            }
            // Loop through all remaining days of the month and check if the event
            // begins on that day (don't use Event::recursOn, as that will
            // also return events that have started earlier. These start dates
            // however, have already been treated!
            KCalendarCore::Recurrence *recur = e->recurrence();
            QDate d1(start.addDays(1));
            while (d1 <= end) {
                if (recur->recursOn(d1, QTimeZone::systemTimeZone())) {
                    KCalendarCore::TimeList times(recur->recurTimesOn(d1, QTimeZone::systemTimeZone()));
                    for (KCalendarCore::TimeList::ConstIterator it = times.constBegin(); it != times.constEnd(); ++it) {
                        QDateTime d1start(d1, *it, Qt::LocalTime);
                        monthentries.append(MonthEventStruct(d1start, e->endDateForStart(d1start).toLocalTime(), e));
                    }
                }
                d1 = d1.addDays(1);
            }
        } else {
            monthentries.append(MonthEventStruct(e->dtStart().toLocalTime(), e->dtEnd().toLocalTime(), e));
        }
    }

    // TODO: to port the month entries sorting

    //  qSort( monthentries.begin(), monthentries.end() );

    QVector<MonthEventStruct>::ConstIterator mit = monthentries.constBegin();
    QDateTime endofmonth(end, QTime(0, 0, 0));
    endofmonth = endofmonth.addDays(1);
    for (; mit != monthentries.constEnd(); ++mit) {
        if ((*mit).start.date() == (*mit).end.date()) {
            // Show also single-day events as time line boxes
            if (subDailyFlags & TimeBoxes) {
                timeboxItems.append(new PrintCellItem((*mit).event, (*mit).start, (*mit).end));
            }
            // Show as text in the box
            if (subDailyFlags & Text) {
                textEvents[(*mit).start.date().day()] << (*mit).event->summary();
            }
        } else {
            // Multi-day events are always shown as time line boxes
            QDateTime thisstart((*mit).start);
            QDateTime thisend((*mit).end);
            if (thisstart.date() < start) {
                thisstart.setDate(start);
            }
            if (thisend > endofmonth) {
                thisend = endofmonth;
            }
            timeboxItems.append(new PrintCellItem((*mit).event, thisstart, thisend));
        }
    }

    // For Multi-day events, line them up nicely so that the boxes don't overlap
    QListIterator<CellItem *> it1(timeboxItems);
    while (it1.hasNext()) {
        CellItem *placeItem = it1.next();
        CellItem::placeItem(timeboxItems, placeItem);
    }
    QDateTime starttime(start, QTime(0, 0, 0));
    int newxstartcont = xstartcont;

    QFont oldfont(p.font());
    p.setFont(QFont(QStringLiteral("sans-serif"), 7));
    while (it1.hasNext()) {
        auto placeItem = static_cast<PrintCellItem *>(it1.next());
        int minsToStart = starttime.secsTo(placeItem->start()) / 60;
        int minsToEnd = starttime.secsTo(placeItem->end()) / 60;

        QRect eventBox(xstartcont + placeItem->subCell() * 17,
                       daysBox.top() + qRound(double(minsToStart * daysBox.height()) / double(maxdays * 24 * 60)),
                       14,
                       0);
        eventBox.setBottom(daysBox.top() + qRound(double(minsToEnd * daysBox.height()) / double(maxdays * 24 * 60)));
        drawVerticalBox(p, 0, eventBox, placeItem->event()->summary());
        newxstartcont = qMax(newxstartcont, eventBox.right());
    }
    xstartcont = newxstartcont;

    // For Single-day events, simply print their summaries into the remaining
    // space of the day's cell
    for (int d = 0; d < daysinmonth; ++d) {
        QStringList dayEvents(textEvents[d + 1]);
        QString txt = dayEvents.join(QLatin1String(", "));
        QRect dayBox(xstartcont, daysBox.top() + qRound(dayheight * d), 0, 0);
        dayBox.setRight(box.right());
        dayBox.setBottom(daysBox.top() + qRound(dayheight * (d + 1)));
        printEventString(p, dayBox, txt, Qt::AlignTop | Qt::AlignLeft | Qt::TextWrapAnywhere);
    }
    p.setFont(oldfont);
    drawBox(p, BOX_BORDER_WIDTH, borderBox);
    p.restore();
}

void CalPrintPluginBase::drawMonthTable(QPainter &p,
                                        QDate qd,
                                        QTime fromTime,
                                        QTime toTime,
                                        bool weeknumbers,
                                        bool recurDaily,
                                        bool recurWeekly,
                                        bool singleLineLimit,
                                        bool includeDescription,
                                        bool includeCategories,
                                        QRect box)
{
    int yoffset = mSubHeaderHeight;
    int xoffset = 0;
    QDate monthDate(QDate(qd.year(), qd.month(), 1));
    QDate monthFirst(monthDate);
    QDate monthLast(monthDate.addMonths(1).addDays(-1));

    int weekdayCol = weekdayColumn(monthDate.dayOfWeek());
    monthDate = monthDate.addDays(-weekdayCol);

    if (weeknumbers) {
        xoffset += 14;
    }

    int rows = (weekdayCol + qd.daysInMonth() - 1) / 7 + 1;
    double cellHeight = (box.height() - yoffset) / (1. * rows);
    double cellWidth = (box.width() - xoffset) / 7.;

    // Precalculate the grid...
    // rows is at most 6, so using 8 entries in the array is fine, too!
    int coledges[8], rowedges[8];
    for (int i = 0; i <= 7; ++i) {
        rowedges[i] = int(box.top() + yoffset + i * cellHeight);
        coledges[i] = int(box.left() + xoffset + i * cellWidth);
    }

    if (weeknumbers) {
        QFont oldFont(p.font());
        QFont newFont(p.font());
        newFont.setPointSize(6);
        p.setFont(newFont);
        QDate weekDate(monthDate);
        for (int row = 0; row < rows; ++row) {
            int calWeek = weekDate.weekNumber();
            QRect rc(box.left(), rowedges[row], coledges[0] - 3 - box.left(), rowedges[row + 1] - rowedges[row]);
            p.drawText(rc, Qt::AlignRight | Qt::AlignVCenter, QString::number(calWeek));
            weekDate = weekDate.addDays(7);
        }
        p.setFont(oldFont);
    }

    QRect daysOfWeekBox(box);
    daysOfWeekBox.setHeight(mSubHeaderHeight);
    daysOfWeekBox.setLeft(box.left() + xoffset);
    drawDaysOfWeek(p, monthDate, monthDate.addDays(6), daysOfWeekBox);

    QColor back = p.background().color();
    bool darkbg = false;
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < 7; ++col) {
            // show days from previous/next month with a grayed background
            if ((monthDate < monthFirst) || (monthDate > monthLast)) {
                p.setBackground(back.darker(120));
                darkbg = true;
            }
            QRect dayBox(coledges[col], rowedges[row], coledges[col + 1] - coledges[col], rowedges[row + 1] - rowedges[row]);
            drawDayBox(p,
                       monthDate,
                       fromTime,
                       toTime,
                       dayBox,
                       false,
                       recurDaily,
                       recurWeekly,
                       singleLineLimit,
                       includeDescription,
                       includeCategories);
            if (darkbg) {
                p.setBackground(back);
                darkbg = false;
            }
            monthDate = monthDate.addDays(1);
        }
    }
}

void CalPrintPluginBase::drawTodoLines(QPainter &p,
                                       const QString &entry,
                                       int x,
                                       int &y,
                                       int width,
                                       int pageHeight,
                                       bool richTextEntry,
                                       QList<TodoParentStart *> &startPoints,
                                       bool connectSubTodos)
{
    QString plainEntry = (richTextEntry) ? toPlainText(entry) : entry;

    QRect textrect(0, 0, width, -1);
    int flags = Qt::AlignLeft;
    QFontMetrics fm = p.fontMetrics();

    QStringList lines = plainEntry.split(QLatin1Char('\n'));
    for (int currentLine = 0; currentLine < lines.count(); currentLine++) {
        // split paragraphs into lines
        KWordWrap ww = KWordWrap::formatText(fm, textrect, flags, lines[currentLine]);
        QStringList textLine = ww.wrappedString().split(QLatin1Char('\n'));

        // print each individual line
        for (int lineCount = 0; lineCount < textLine.count(); lineCount++) {
            if (y >= pageHeight) {
                if (connectSubTodos) {
                    for (int i = 0; i < startPoints.size(); ++i) {
                        TodoParentStart *rct;
                        rct = startPoints.at(i);
                        int start = rct->mRect.bottom() + 1;
                        int center = rct->mRect.left() + (rct->mRect.width() / 2);
                        int to = y;
                        if (!rct->mSamePage) {
                            start = 0;
                        }
                        if (rct->mHasLine) {
                            p.drawLine(center, start, center, to);
                        }
                        rct->mSamePage = false;
                    }
                }
                y = 0;
                mPrinter->newPage();
            }
            y += fm.height();
            p.drawText(x, y, textLine[lineCount]);
        }
    }
}

void CalPrintPluginBase::drawTodo(int &count,
                                  const KCalendarCore::Todo::Ptr &todo,
                                  QPainter &p,
                                  KCalendarCore::TodoSortField sortField,
                                  KCalendarCore::SortDirection sortDir,
                                  bool connectSubTodos,
                                  bool strikeoutCompleted,
                                  bool desc,
                                  int posPriority,
                                  int posSummary,
                                  int posCategories,
                                  int posStartDt,
                                  int posDueDt,
                                  int posPercentComplete,
                                  int level,
                                  int x,
                                  int &y,
                                  int width,
                                  int pageHeight,
                                  const KCalendarCore::Todo::List &todoList,
                                  TodoParentStart *r)
{
    QString outStr;
    const auto locale = QLocale::system();
    QRect rect;
    TodoParentStart startpt;
    // This list keeps all starting points of the parent to-dos so the connection
    // lines of the tree can easily be drawn (needed if a new page is started)
    static QList<TodoParentStart *> startPoints;
    if (level < 1) {
        startPoints.clear();
    }

    y += 10;

    int left = posSummary + (level * 10);

    // If this is a sub-to-do, r will not be 0, and we want the LH side
    // of the priority line up to the RH side of the parent to-do's priority
    int lhs = posPriority;
    if (r) {
        lhs = r->mRect.right() + 1;
    }

    outStr.setNum(todo->priority());
    rect = p.boundingRect(lhs, y + 10, 5, -1, Qt::AlignCenter, outStr);
    // Make it a more reasonable size
    rect.setWidth(18);
    rect.setHeight(18);
    const int top = rect.top();

    // Draw a checkbox
    p.setBrush(QBrush(Qt::NoBrush));
    p.drawRect(rect);
    if (todo->isCompleted()) {
        // cross out the rectangle for completed to-dos
        p.drawLine(rect.topLeft(), rect.bottomRight());
        p.drawLine(rect.topRight(), rect.bottomLeft());
    }
    lhs = rect.right() + 5;

    // Priority
    if (posPriority >= 0 && todo->priority() > 0) {
        p.drawText(rect, Qt::AlignCenter, outStr);
    }
    startpt.mRect = rect; // save for later

    // Connect the dots
    if (r && level > 0 && connectSubTodos) {
        int bottom;
        int center(r->mRect.left() + (r->mRect.width() / 2));
        int to(rect.top() + (rect.height() / 2));
        int endx(rect.left());
        p.drawLine(center, to, endx, to); // side connector
        if (r->mSamePage) {
            bottom = r->mRect.bottom() + 1;
        } else {
            bottom = 0;
        }
        p.drawLine(center, bottom, center, to);
    }

    int posSoFar = width;  // Position of leftmost optional field.

    // due date
    if (posDueDt >= 0 && todo->hasDueDate()) {
        outStr = locale.toString(todo->dtDue().toLocalTime().date(), QLocale::ShortFormat);
        rect = p.boundingRect(posDueDt, top, x + width, -1, Qt::AlignTop | Qt::AlignLeft, outStr);
        p.drawText(rect, Qt::AlignTop | Qt::AlignLeft, outStr);
        posSoFar = posDueDt;
    }

    // start date
    if (posStartDt >= 0 && todo->hasStartDate()) {
        outStr = locale.toString(todo->dtStart().toLocalTime().date(), QLocale::ShortFormat);
        rect = p.boundingRect(posStartDt, top, x + width, -1, Qt::AlignTop | Qt::AlignLeft, outStr);
        p.drawText(rect, Qt::AlignTop | Qt::AlignLeft, outStr);
        posSoFar = posStartDt;
    }

    // percentage completed
    if (posPercentComplete >= 0) {
        int lwidth = 24;
        int lheight = p.fontMetrics().ascent();
        // first, draw the progress bar
        int progress = static_cast<int>(((lwidth * todo->percentComplete()) / 100.0 + 0.5));

        p.setBrush(QBrush(Qt::NoBrush));
        p.drawRect(posPercentComplete, top, lwidth, lheight);
        if (progress > 0) {
            p.setBrush(QColor(128, 128, 128));
            p.drawRect(posPercentComplete, top, progress, lheight);
        }

        // now, write the percentage
        outStr = i18n("%1%", todo->percentComplete());
        rect = p.boundingRect(posPercentComplete + lwidth + 3, top, x + width, -1, Qt::AlignTop | Qt::AlignLeft, outStr);
        p.drawText(rect, Qt::AlignTop | Qt::AlignLeft, outStr);
        posSoFar = posPercentComplete;
    }

    // categories
    QRect categoriesRect {0, 0, 0, 0};
    if (posCategories >= 0) {
        outStr = todo->categoriesStr();
        outStr.replace(QLatin1Char(','), QLatin1Char('\n'));
        rect = p.boundingRect(posCategories, top, posSoFar - posCategories, -1, Qt::TextWordWrap, outStr);
        p.drawText(rect, Qt::TextWordWrap, outStr, &categoriesRect);
        posSoFar = posCategories;
    }

    // summary
    outStr = todo->summary();
    rect = p.boundingRect(lhs, top, posSoFar - lhs - 5, -1, Qt::TextWordWrap, outStr);
    QFont oldFont(p.font());
    if (strikeoutCompleted && todo->isCompleted()) {
        QFont newFont(p.font());
        newFont.setStrikeOut(true);
        p.setFont(newFont);
    }
    QRect summaryRect;
    p.drawText(rect, Qt::TextWordWrap, outStr, &summaryRect);
    p.setFont(oldFont);

    y = std::max(categoriesRect.bottom(), summaryRect.bottom());

    // description
    if (desc && !todo->description().isEmpty()) {
        drawTodoLines(p, todo->description(), left, y, width - (left + 10 - x), pageHeight, todo->descriptionIsRich(), startPoints, connectSubTodos);
    }

    // Make a list of all the sub-to-dos related to this to-do.
    KCalendarCore::Todo::List t;
    const KCalendarCore::Incidence::List relations = mCalendar->childIncidences(todo->uid());

    for (const KCalendarCore::Incidence::Ptr &incidence : relations) {
        // In the future, to-dos might also be related to events
        // Manually check if the sub-to-do is in the list of to-dos to print
        // The problem is that relations() does not apply filters, so
        // we need to compare manually with the complete filtered list!
        KCalendarCore::Todo::Ptr subtodo = incidence.dynamicCast<KCalendarCore::Todo>();
        if (!subtodo) {
            continue;
        }
#ifdef AKONADI_PORT_DISABLED
        if (subtodo && todoList.contains(subtodo)) {
#else
        bool subtodoOk = false;
        if (subtodo) {
            for (const KCalendarCore::Todo::Ptr &tt : std::as_const(todoList)) {
                if (tt == subtodo) {
                    subtodoOk = true;
                    break;
                }
            }
        }
        if (subtodoOk) {
#endif
            if ((mExcludeConfidential && subtodo->secrecy() == KCalendarCore::Incidence::SecrecyConfidential)
                || (mExcludePrivate && subtodo->secrecy() == KCalendarCore::Incidence::SecrecyPrivate)) {
                continue;
            }
            t.append(subtodo);
        }
    }

    // has sub-todos?
    startpt.mHasLine = (relations.size() > 0);
    startPoints.append(&startpt);

    // Sort the sub-to-dos and print them
#ifdef AKONADI_PORT_DISABLED
    KCalendarCore::Todo::List sl = mCalendar->sortTodos(&t, sortField, sortDir);
#else
    KCalendarCore::Todo::List tl;
    tl.reserve(t.count());
    for (const KCalendarCore::Todo::Ptr &todo : std::as_const(t)) {
        tl.append(todo);
    }
    KCalendarCore::Todo::List sl = mCalendar->sortTodos(tl, sortField, sortDir);
#endif

    int subcount = 0;
    for (const KCalendarCore::Todo::Ptr &isl : std::as_const(sl)) {
        count++;
        if (++subcount == sl.size()) {
            startpt.mHasLine = false;
        }
        drawTodo(count,
                 isl,
                 p,
                 sortField,
                 sortDir,
                 connectSubTodos,
                 strikeoutCompleted,
                 desc,
                 posPriority,
                 posSummary,
                 posCategories,
                 posStartDt,
                 posDueDt,
                 posPercentComplete,
                 level + 1,
                 x,
                 y,
                 width,
                 pageHeight,
                 todoList,
                 &startpt);
    }
    startPoints.removeAll(&startpt);
}

int CalPrintPluginBase::weekdayColumn(int weekday)
{
    int w = weekday + 7 - QLocale().firstDayOfWeek();
    return w % 7;
}

void CalPrintPluginBase::drawTextLines(QPainter &p, const QString &entry, int x, int &y, int width, int pageHeight, bool richTextEntry)
{
    QString plainEntry = (richTextEntry) ? toPlainText(entry) : entry;

    QRect textrect(0, 0, width, -1);
    int flags = Qt::AlignLeft;
    QFontMetrics fm = p.fontMetrics();

    QStringList lines = plainEntry.split(QLatin1Char('\n'));
    for (int currentLine = 0; currentLine < lines.count(); currentLine++) {
        // split paragraphs into lines
        KWordWrap ww = KWordWrap::formatText(fm, textrect, flags, lines[currentLine]);
        QStringList textLine = ww.wrappedString().split(QLatin1Char('\n'));
        // print each individual line
        for (int lineCount = 0; lineCount < textLine.count(); lineCount++) {
            y += fm.height();
            if (y >= pageHeight) {
                if (mPrintFooter) {
                    drawFooter(p, {0, pageHeight, width, footerHeight()});
                }
                y = fm.height();
                mPrinter->newPage();
            }
            p.drawText(x, y, textLine[lineCount]);
        }
    }
}

void CalPrintPluginBase::drawSplitHeaderRight(QPainter &p, QDate fd, QDate td, QDate, int width, int height)
{
    QFont oldFont(p.font());

    QPen oldPen(p.pen());
    QPen pen(Qt::black, 4);

    QString title;
    QLocale locale;
    if (fd.month() == td.month()) {
        title = i18nc("Date range: Month dayStart - dayEnd",
                      "%1 %2\u2013%3",
                      locale.monthName(fd.month(), QLocale::LongFormat),
                      locale.toString(fd, QStringLiteral("dd")),
                      locale.toString(td, QStringLiteral("dd")));
    } else {
        title = i18nc("Date range: monthStart dayStart - monthEnd dayEnd",
                      "%1 %2\u2013%3 %4",
                      locale.monthName(fd.month(), QLocale::LongFormat),
                      locale.toString(fd, QStringLiteral("dd")),
                      locale.monthName(td.month(), QLocale::LongFormat),
                      locale.toString(td, QStringLiteral("dd")));
    }

    if (height < 60) {
        p.setFont(QFont(QStringLiteral("Times"), 22));
    } else {
        p.setFont(QFont(QStringLiteral("Times"), 28));
    }

    int lineSpacing = p.fontMetrics().lineSpacing();
    p.drawText(0, 0, width, lineSpacing, Qt::AlignRight | Qt::AlignTop, title);

    title.truncate(0);

    p.setPen(pen);
    p.drawLine(300, lineSpacing, width, lineSpacing);
    p.setPen(oldPen);

    if (height < 60) {
        p.setFont(QFont(QStringLiteral("Times"), 14, QFont::Bold, true));
    } else {
        p.setFont(QFont(QStringLiteral("Times"), 18, QFont::Bold, true));
    }

    title += QString::number(fd.year());
    p.drawText(0, lineSpacing + padding(), width, lineSpacing, Qt::AlignRight | Qt::AlignTop, title);

    p.setFont(oldFont);
}

void CalPrintPluginBase::drawNoteLines(QPainter &p, QRect box, int startY)
{
    int lineHeight = int(p.fontMetrics().lineSpacing() * 1.5);
    int linePos = box.y();
    int startPos = startY;
    // adjust line to start at multiple from top of box for alignment
    while (linePos < startPos) {
        linePos += lineHeight;
    }
    QPen oldPen(p.pen());
    p.setPen(Qt::DotLine);
    while (linePos < box.bottom()) {
        p.drawLine(box.left() + padding(), linePos, box.right() - padding(), linePos);
        linePos += lineHeight;
    }
    p.setPen(oldPen);
}

QString CalPrintPluginBase::toPlainText(const QString &htmlText)
{
    // this converts possible rich text to plain text
    return QTextDocumentFragment::fromHtml(htmlText).toPlainText();
}
