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

void CalPrintJournal::loadConfig()
{
    if (mConfig) {
        KConfigGroup config(mConfig, "Journalprint");
        mUseDateRange = config.readEntry("JournalsInRange", false);
    }
    setSettingsWidget();
}

void CalPrintJournal::saveConfig()
{
    qCDebug(CALENDARSUPPORT_LOG);

    readSettingsWidget();
    if (mConfig) {
        KConfigGroup config(mConfig, "Journalprint");
        config.writeEntry("JournalsInRange", mUseDateRange);
    }
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
