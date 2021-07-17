/*
  SPDX-FileCopyrightText: 2004 Reinhold Kainhofer <reinhold@kainhofer.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#include "journalprint.h"
#include "calendarsupport_debug.h"
#include "utils.h"
#include <KConfigGroup>

using namespace CalendarSupport;

/**************************************************************
 *           Print Journal
 **************************************************************/

QWidget *CalPrintJournal::createConfigWidget(QWidget *w)
{
    return new CalPrintJournalConfig(w);
}

void CalPrintJournal::readSettingsWidget()
{
    auto cfg = dynamic_cast<CalPrintJournalConfig *>((QWidget *)mConfigWidget);
    if (cfg) {
        mPrintFooter = cfg->mPrintFooter->isChecked();
        mFromDate = cfg->mFromDate->date();
        mToDate = cfg->mToDate->date();
        mUseDateRange = cfg->mRangeJournals->isChecked();
        mExcludeConfidential = cfg->mExcludeConfidential->isChecked();
        mExcludePrivate = cfg->mExcludePrivate->isChecked();
    }
}

void CalPrintJournal::setSettingsWidget()
{
    auto cfg = dynamic_cast<CalPrintJournalConfig *>((QWidget *)mConfigWidget);
    if (cfg) {
        cfg->mPrintFooter->setChecked(mPrintFooter);
        cfg->mFromDate->setDate(mFromDate);
        cfg->mToDate->setDate(mToDate);
        cfg->mExcludeConfidential->setChecked(mExcludeConfidential);
        cfg->mExcludePrivate->setChecked(mExcludePrivate);

        if (mUseDateRange) {
            cfg->mRangeJournals->setChecked(true);
            cfg->mFromDateLabel->setEnabled(true);
            cfg->mFromDate->setEnabled(true);
            cfg->mToDateLabel->setEnabled(true);
            cfg->mToDate->setEnabled(true);
        } else {
            cfg->mAllJournals->setChecked(true);
            cfg->mFromDateLabel->setEnabled(false);
            cfg->mFromDate->setEnabled(false);
            cfg->mToDateLabel->setEnabled(false);
            cfg->mToDate->setEnabled(false);
        }
    }
}

void CalPrintJournal::doLoadConfig()
{
    CalPrintPluginBase::doLoadConfig();
    if (mConfig) {
        KConfigGroup config(mConfig, "Journalprint");
        mUseDateRange = config.readEntry("JournalsInRange", false);
    }
    setSettingsWidget();
}

void CalPrintJournal::doSaveConfig()
{
    qCDebug(CALENDARSUPPORT_LOG);

    readSettingsWidget();
    if (mConfig) {
        KConfigGroup config(mConfig, "Journalprint");
        config.writeEntry("JournalsInRange", mUseDateRange);
    }
    CalPrintPluginBase::doSaveConfig();
}

void CalPrintJournal::setDateRange(const QDate &from, const QDate &to)
{
    CalPrintPluginBase::setDateRange(from, to);
    auto cfg = dynamic_cast<CalPrintJournalConfig *>((QWidget *)mConfigWidget);
    if (cfg) {
        cfg->mFromDate->setDate(from);
        cfg->mToDate->setDate(to);
    }
}

void CalPrintJournal::drawJournal(const KCalendarCore::Journal::Ptr &journal, QPainter &p, int x, int &y, int width, int pageHeight)
{
    QFont oldFont(p.font());
    p.setFont(QFont(QStringLiteral("sans-serif"), 15));
    QString headerText;
    QString dateText(QLocale::system().toString(journal->dtStart().toLocalTime().date(), QLocale::LongFormat));

    if (journal->summary().isEmpty()) {
        headerText = dateText;
    } else {
        headerText = i18nc("Description - date", "%1 - %2", journal->summary(), dateText);
    }

    QRect rect(p.boundingRect(x, y, width, -1, Qt::TextWordWrap, headerText));
    if (rect.bottom() > pageHeight) {
        if (mPrintFooter) {
            drawFooter(p, {0, pageHeight, width, footerHeight()});
        }
        // Start new page...
        y = 0;
        mPrinter->newPage();
        rect = p.boundingRect(x, y, width, -1, Qt::TextWordWrap, headerText);
    }
    QRect newrect;
    p.drawText(rect, Qt::TextWordWrap, headerText, &newrect);
    p.setFont(oldFont);

    y = newrect.bottom() + 4;

    p.drawLine(x + 3, y, x + width - 6, y);
    y += 5;
    if (!(journal->organizer().fullName().isEmpty())) {
        drawTextLines(p, i18n("Person: %1", journal->organizer().fullName()), x, y, width, pageHeight, false);
        y += 7;
    }
    if (!(journal->description().isEmpty())) {
        drawTextLines(p, journal->description(), x, y, width, pageHeight, journal->descriptionIsRich());
        y += 7;
    }
    y += 10;
}

void CalPrintJournal::print(QPainter &p, int width, int height)
{
    int x = 0, y = 0;
    KCalendarCore::Journal::List journals(mCalendar->journals(KCalendarCore::JournalSortDate, KCalendarCore::SortDirectionAscending));
    if (mUseDateRange) {
        const KCalendarCore::Journal::List allJournals = journals;
        journals.clear();
        for (const KCalendarCore::Journal::Ptr &j : allJournals) {
            const QDate dt = j->dtStart().date();
            if (mFromDate <= dt && dt <= mToDate) {
                journals.append(j);
            }
        }
    }

    QRect headerBox(0, 0, width, headerHeight());
    QRect footerBox(0, height - footerHeight(), width, footerHeight());
    height -= footerHeight();

    drawHeader(p, i18n("Journal entries"), QDate(), QDate(), headerBox);
    y = headerHeight() + 15;

    for (const KCalendarCore::Journal::Ptr &j : std::as_const(journals)) {
        Q_ASSERT(j);
        if (j
            && (!mExcludeConfidential || j->secrecy() != KCalendarCore::Incidence::SecrecyConfidential)
            && (!mExcludePrivate || j->secrecy() != KCalendarCore::Incidence::SecrecyPrivate)) {
            drawJournal(j, p, x, y, width, height);
        }
    }

    if (mPrintFooter) {
        drawFooter(p, footerBox);
    }
}
