/*
  SPDX-FileCopyrightText: 2010 Casey Link <unnamedrambler@gmail.com>
  SPDX-FileCopyrightText: 2009-2010 Klaralvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "freeperiodmodel.h"

#include <KFormat>
#include <KLocalizedString>

#include <QDateTime>
#include <QLocale>
#include <QSet>
#include <QTimeZone>

using namespace CalendarSupport;

FreePeriodModel::FreePeriodModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

FreePeriodModel::~FreePeriodModel()
{
}

QVariant FreePeriodModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || !hasIndex(index.row(), index.column())) {
        return QVariant();
    }

    if (index.column() == 0) { // day
        switch (role) {
        case Qt::DisplayRole:
            return day(index.row());
        case Qt::ToolTipRole:
            return tooltipify(index.row());
        case FreePeriodModel::PeriodRole:
            return QVariant::fromValue(mPeriodList.at(index.row()));
        case Qt::TextAlignmentRole:
            return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
        default:
            return QVariant();
        }
    } else { // everything else
        switch (role) {
        case Qt::DisplayRole:
            return date(index.row());
        case Qt::ToolTipRole:
            return tooltipify(index.row());
        case FreePeriodModel::PeriodRole:
            return QVariant::fromValue(mPeriodList.at(index.row()));
        case Qt::TextAlignmentRole:
            return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);
        default:
            return QVariant();
        }
    }
}

int FreePeriodModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return mPeriodList.size();
    }
    return 0;
}

int FreePeriodModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 2;
}

QVariant FreePeriodModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return QAbstractItemModel::headerData(section, orientation, role);
}

void FreePeriodModel::slotNewFreePeriods(const KCalendarCore::Period::List &freePeriods)
{
    beginResetModel();
    mPeriodList.clear();
    mPeriodList = splitPeriodsByDay(freePeriods);
    std::sort(mPeriodList.begin(), mPeriodList.end());
    endResetModel();
}

KCalendarCore::Period::List FreePeriodModel::splitPeriodsByDay(const KCalendarCore::Period::List &freePeriods)
{
    KCalendarCore::Period::List splitList;
    for (const KCalendarCore::Period &period : freePeriods) {
        if (period.start().date() == period.end().date()) {
            splitList << period; // period occurs on the same day
            continue;
        }

        const int validPeriodSecs = 300; // 5 minutes
        KCalendarCore::Period tmpPeriod = period;
        while (tmpPeriod.start().date() != tmpPeriod.end().date()) {
            const QDateTime midnight(tmpPeriod.start().date(), QTime(23, 59, 59, 999), tmpPeriod.start().timeZone());
            KCalendarCore::Period firstPeriod(tmpPeriod.start(), midnight);
            KCalendarCore::Period secondPeriod(midnight.addMSecs(1), tmpPeriod.end());
            if (firstPeriod.duration().asSeconds() >= validPeriodSecs) {
                splitList << firstPeriod;
            }
            tmpPeriod = secondPeriod;
        }
        if (tmpPeriod.duration().asSeconds() >= validPeriodSecs) {
            splitList << tmpPeriod;
        }
    }

    // Perform some jiggery pokery to remove duplicates
    std::sort(splitList.begin(), splitList.end());
    splitList.erase(std::unique(splitList.begin(), splitList.end()), splitList.end());
    return splitList;
}

QString FreePeriodModel::day(int index) const
{
    KCalendarCore::Period period = mPeriodList.at(index);
    const QDate startDate = period.start().date();
    return ki18nc("@label Day of the week name, example: Monday,", "%1,")
        .subs(QLocale::system().dayName(startDate.dayOfWeek(), QLocale::LongFormat))
        .toString();
}

QString FreePeriodModel::date(int index) const
{
    KCalendarCore::Period period = mPeriodList.at(index);

    const QDate startDate = period.start().date();
    const QString startTime = QLocale::system().toString(period.start().time(), QLocale::ShortFormat);
    const QString endTime = QLocale::system().toString(period.end().time(), QLocale::ShortFormat);
    const QString longMonthName = QLocale::system().monthName(startDate.month());
    return ki18nc(
               "@label A time period duration. It is preceded/followed (based on the "
               "orientation) by the name of the week, see the message above. "
               "example: 12 June, 8:00am to 9:30am",
               "%1 %2, %3 to %4")
        .subs(startDate.day())
        .subs(longMonthName)
        .subs(startTime)
        .subs(endTime)
        .toString();
}

QString FreePeriodModel::stringify(int index) const
{
    KCalendarCore::Period period = mPeriodList.at(index);

    const QDate startDate = period.start().date();
    const QString startTime = QLocale().toString(period.start().time(), QLocale::ShortFormat);
    const QString endTime = QLocale().toString(period.end().time(), QLocale::ShortFormat);
    const QString longMonthName = QLocale::system().monthName(startDate.month(), QLocale::LongFormat);
    const QString dayofWeek = QLocale::system().dayName(startDate.dayOfWeek(), QLocale::LongFormat);

    // TODO i18n, ping chusslove
    return ki18nc(
               "@label A time period duration. KLocale is used to format the components. "
               "example: Monday, 12 June, 8:00am to 9:30am",
               "%1, %2 %3, %4 to %5")
        .subs(dayofWeek)
        .subs(startDate.day())
        .subs(longMonthName)
        .subs(startTime)
        .subs(endTime)
        .toString();
}

QString FreePeriodModel::tooltipify(int index) const
{
    KCalendarCore::Period period = mPeriodList.at(index);
    unsigned long duration = period.duration().asSeconds() * 1000; // we want milliseconds
    QString toolTip = QStringLiteral("<qt>");
    toolTip += QLatin1String("<b>") + i18nc("@info:tooltip", "Free Period") + QLatin1String("</b>");
    toolTip += QLatin1String("<hr>");
    toolTip += QLatin1String("<i>") + i18nc("@info:tooltip period start time", "Start:") + QLatin1String("</i>&nbsp;");
    toolTip += QLocale().toString(period.start().toLocalTime(), QLocale::ShortFormat);
    toolTip += QLatin1String("<br>");
    toolTip += QLatin1String("<i>") + i18nc("@info:tooltip period end time", "End:") + QLatin1String("</i>&nbsp;");
    toolTip += QLocale().toString(period.end().toLocalTime(), QLocale::ShortFormat);
    toolTip += QLatin1String("<br>");
    toolTip += QLatin1String("<i>") + i18nc("@info:tooltip period duration", "Duration:") + QLatin1String("</i>&nbsp;");
    toolTip += KFormat().formatSpelloutDuration(duration);
    toolTip += QLatin1String("</qt>");
    return toolTip;
}
