/*
  SPDX-FileCopyrightText: 2004 Reinhold Kainhofer <reinhold@kainhofer.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#include "yearprint.h"

#include "calendarsupport_debug.h"
#include <KConfigGroup>
#include <KLocalizedString>
using namespace CalendarSupport;

/**************************************************************
 *           Print Year
 **************************************************************/

QWidget *CalPrintYear::createConfigWidget(QWidget *w)
{
    return new CalPrintYearConfig(w);
}

void CalPrintYear::readSettingsWidget()
{
    auto cfg = dynamic_cast<CalPrintYearConfig *>((QWidget *)mConfigWidget);
    if (cfg) {
        mPrintFooter = cfg->mPrintFooter->isChecked();
        mYear = cfg->mYear->value();
        mPages = cfg->mPages->currentText().toInt();
        mSubDaysEvents = (cfg->mSubDays->currentIndex() == 0) ? Text : TimeBoxes;
        mHolidaysEvents = (cfg->mHolidays->currentIndex() == 0) ? Text : TimeBoxes;
        mExcludeConfidential = cfg->mExcludeConfidential->isChecked();
        mExcludePrivate = cfg->mExcludePrivate->isChecked();
    }
}

void CalPrintYear::setSettingsWidget()
{
    auto cfg = dynamic_cast<CalPrintYearConfig *>((QWidget *)mConfigWidget);
    if (cfg) {
        QDate start(mYear, 1, 1);
        const int months = 12;
        int prevPages = 0;
        for (int i = 1; i <= months; ++i) {
            const int pages = (months - 1) / i + 1;
            if (pages != prevPages) {
                prevPages = pages;
                cfg->mPages->addItem(QString::number(pages), pages);
            }
        }

        cfg->mPrintFooter->setChecked(mPrintFooter);
        cfg->mYear->setValue(mYear);
        cfg->mPages->setCurrentIndex(cfg->mPages->findData(mPages));

        cfg->mSubDays->setCurrentIndex((mSubDaysEvents == Text) ? 0 : 1);
        cfg->mHolidays->setCurrentIndex((mHolidaysEvents == Text) ? 0 : 1);
        cfg->mExcludeConfidential->setChecked(mExcludeConfidential);
        cfg->mExcludePrivate->setChecked(mExcludePrivate);
    }
}

void CalPrintYear::doLoadConfig()
{
    CalPrintPluginBase::doLoadConfig();
    if (mConfig) {
        KConfigGroup config(mConfig, "Yearprint");
        mYear = config.readEntry("Year", QDate::currentDate().year());
        mPages = config.readEntry("Pages", 1);
        mSubDaysEvents = config.readEntry("ShowSubDayEventsAs", static_cast<int>(TimeBoxes));
        mHolidaysEvents = config.readEntry("ShowHolidaysAs", static_cast<int>(Text));
    }
    setSettingsWidget();
}

void CalPrintYear::doSaveConfig()
{
    qCDebug(CALENDARSUPPORT_LOG);

    readSettingsWidget();
    if (mConfig) {
        KConfigGroup config(mConfig, "Yearprint");
        config.writeEntry("Year", mYear);
        config.writeEntry("Pages", mPages);
        config.writeEntry("Pages", mPages);
        config.writeEntry("ShowSubDayEventsAs", mSubDaysEvents);
        config.writeEntry("ShowHolidaysAs", mHolidaysEvents);
    }
    CalPrintPluginBase::doSaveConfig();
}

QPageLayout::Orientation CalPrintYear::defaultOrientation() const
{
    return (mPages == 1) ? QPageLayout::Landscape : QPageLayout::Portrait;
}

void CalPrintYear::setDateRange(const QDate &from, const QDate &to)
{
    CalPrintPluginBase::setDateRange(from, to);
    auto cfg = dynamic_cast<CalPrintYearConfig *>((QWidget *)mConfigWidget);
    if (cfg) {
        cfg->mYear->setValue(from.year());
    }
}

void CalPrintYear::print(QPainter &p, int width, int height)
{
    auto locale = QLocale::system();

    QRect headerBox(0, 0, width, headerHeight());
    QRect footerBox(0, height - footerHeight(), width, footerHeight());
    height -= footerHeight();

    QDate start(mYear, 1, 1);

    // Determine the nr of months and the max nr of days per month (dependent on
    // calendar system!!!!)
    QDate temp(start);
    const int months = 12;
    int maxdays = 1;
    for (int i = 1; i < months; ++i) {
        maxdays = qMax(maxdays, temp.daysInMonth());
        temp = temp.addMonths(1);
    }

    // Now determine the months per page so that the printout fits on
    // exactly mPages pages
    int monthsPerPage = (months - 1) / mPages + 1;
    int pages = (months - 1) / monthsPerPage + 1;
    int thismonth = 0;
    temp = start;
    for (int page = 0; page < pages; ++page) {
        if (page > 0) {
            mPrinter->newPage();
        }
        QDate end = start.addMonths(monthsPerPage);
        end = end.addDays(-1);
        QString stdate = locale.toString(start, QLocale::ShortFormat);
        QString endate = locale.toString(end, QLocale::ShortFormat);
        QString title =i18nc("date from-to", "%1\u2013%2", stdate, endate);
        drawHeader(p, title, start.addMonths(-1), start.addMonths(monthsPerPage), headerBox);

        QRect monthesBox(headerBox);
        monthesBox.setTop(monthesBox.bottom() + padding());
        monthesBox.setBottom(height);

        drawBox(p, BOX_BORDER_WIDTH, monthesBox);
        float monthwidth = float(monthesBox.width()) / float(monthsPerPage);

        for (int j = 0; j < monthsPerPage; ++j) {
            if (++thismonth > months) {
                break;
            }
            int xstart = static_cast<int>(j * monthwidth + 0.5);
            int xend = static_cast<int>((j + 1) * monthwidth + 0.5);
            QRect monthBox(xstart, monthesBox.top(), xend - xstart, monthesBox.height());
            drawMonth(p, temp, monthBox, maxdays, mSubDaysEvents, mHolidaysEvents);

            temp = temp.addMonths(1);
        }

        drawFooter(p, footerBox);
        start = start.addMonths(monthsPerPage);
    }
}
