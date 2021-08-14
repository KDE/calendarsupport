/*
  SPDX-FileCopyrightText: 1998 Preston Brown <pbrown@kde.org>
  SPDX-FileCopyrightText: 2003 Reinhold Kainhofer <reinhold@kainhofer.com>
  SPDX-FileCopyrightText: 2008 Ron Goodheart <rong.dev@gmail.com>
  SPDX-FileCopyrightText: 2012-2013 Allen Winter <winter@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/
#pragma once

#include "calendarsupport_export.h"
#include "calprintpluginbase.h"

#include "ui_calprintdayconfig_base.h"
#include "ui_calprintincidenceconfig_base.h"
#include "ui_calprintmonthconfig_base.h"
#include "ui_calprinttodoconfig_base.h"
#include "ui_calprintweekconfig_base.h"

#include <KLocalizedString>

namespace CalendarSupport
{
class CALENDARSUPPORT_EXPORT CalPrintIncidence : public CalPrintPluginBase
{
public:
    CalPrintIncidence();
    ~CalPrintIncidence() override;
    Q_REQUIRED_RESULT QString groupName() const override
    {
        return QStringLiteral("Print incidence");
    }

    Q_REQUIRED_RESULT QString description() const override
    {
        return i18n("Print &incidence");
    }

    Q_REQUIRED_RESULT QString info() const override
    {
        return i18n("Prints an incidence on one page");
    }

    Q_REQUIRED_RESULT int sortID() const override
    {
        return CalPrinterBase::Incidence;
    }

    // Enable the Print Incidence option only if there are selected incidences.
    Q_REQUIRED_RESULT bool enabled() const override
    {
        return !mSelectedIncidences.isEmpty();
    }

    QWidget *createConfigWidget(QWidget *) override;
    Q_REQUIRED_RESULT QPageLayout::Orientation defaultOrientation() const override
    {
        return QPageLayout::Portrait;
    }

public:
    void print(QPainter &p, int width, int height) override;
    void readSettingsWidget() override;
    void setSettingsWidget() override;
    void doLoadConfig() override;
    void doSaveConfig() override;

protected:
    int printCaptionAndText(QPainter &p, QRect box, const QString &caption, const QString &text, const QFont &captionFont, const QFont &textFont);

    bool mShowOptions;
    bool mShowSubitemsNotes;
    bool mShowAttendees;
    bool mShowAttachments;
};

class CalPrintTimetable : public CalPrintPluginBase
{
public:
    CalPrintTimetable();
    ~CalPrintTimetable() override;
    void doLoadConfig() override;
    void doSaveConfig() override;

protected:

    /**
      Draw the all-day box for the agenda print view (the box on top which
      doesn't have a time on the time scale associated).

      Obeys configuration options #mExcludeConfidential, #mExcludePrivate, #mIncludeCategories.
      @param p QPainter of the printout
      @param eventList The list of all-day events that are supposed to be printed
             inside this box
      @param qd The date of the currently printed day
      @param box coordinates of the all day box.
      @param workDays List of workDays
    */
    void drawAllDayBox(QPainter &p,
                      const KCalendarCore::Event::List &eventList,
                      QDate qd,
                      QRect box,
                      const QList<QDate> &workDays);

    /**
      Draw the timetable view of the given time range from fromDate to toDate.
      On the left side the time scale is printed (using drawTimeLine), then each
      day gets one column (printed using drawAgendaDayBox),
      and the events are displayed as boxes (like in korganizer's day/week view).
      The first cell of each column contains the all-day events (using
      drawAllDayBox with expandable=false).

      Obeys configuration options #mExcludeConfidential, #mExcludePrivate,
      #mIncludeAllEvents, #mIncludeCategories, #mIncludeDescription, #mStartTime, #mEndTime.
      @param p QPainter of the printout
      @param fromDate First day to be included in the page
      @param toDate Last day to be included in the page
      @param box coordinates of the time table.
    */
    void drawTimeTable(QPainter &p,
                       QDate fromDate,
                       QDate toDate,
                       QRect box);

    QTime mStartTime, mEndTime; /**< Earliest and latest times of day to print. */
    bool mSingleLineLimit;  /**< Should all fields be printed on the same line? */
    bool mIncludeTodos; /**< Should to-dos be printed? */
    bool mIncludeDescription;   /**< Should incidence descriptions be printed? */
    bool mIncludeCategories;    /**< Should incidence tags be printed? */
    bool mIncludeAllEvents; /**< If events occur outside the start/end times, should the times be adjusted? */
    bool mExcludeTime;  /**< Should incidence times of day be printed? */
};

class CalPrintDay : public CalPrintTimetable
{
public:
    CalPrintDay();
    ~CalPrintDay() override;
    QString groupName() const override
    {
        return QStringLiteral("Print day");
    }

    QString description() const override
    {
        return i18n("Print da&y");
    }

    QString info() const override
    {
        return i18n("Prints all events of a single day on one page");
    }

    int sortID() const override
    {
        return CalPrinterBase::Day;
    }

    bool enabled() const override
    {
        return true;
    }

    QWidget *createConfigWidget(QWidget *) override;

    void print(QPainter &p, int width, int height) override;
    void readSettingsWidget() override;
    void setSettingsWidget() override;
    void doLoadConfig() override;
    void doSaveConfig() override;
    void setDateRange(const QDate &from, const QDate &to) override;

protected:
    enum eDayPrintType { Filofax = 0, Timetable, SingleTimetable } mDayPrintType;

    /**
      Draw the (filofax) table for a bunch of days, using drawDayBox.

      Obeys configuration options #mExcludeConfidential, #mExcludePrivate, #mShowNoteLines, #mUseColors,
      #mFromDate, #mToDate, #mStartTime, #mEndTime, #mSingleLineLimit,
      #mIncludeDescription, #mIncludeCategories.
      @param p QPainter of the printout
      @param box coordinates of the week box.
    */
    void drawDays(QPainter &p, QRect box);
};

class CalPrintWeek : public CalPrintTimetable
{
public:
    CalPrintWeek();
    ~CalPrintWeek() override;

    QString groupName() const override
    {
        return QStringLiteral("Print week");
    }

    QString description() const override
    {
        return i18n("Print &week");
    }

    QString info() const override
    {
        return i18n("Prints all events of one week on one page");
    }

    int sortID() const override
    {
        return CalPrinterBase::Week;
    }

    bool enabled() const override
    {
        return true;
    }

    QWidget *createConfigWidget(QWidget *) override;

    /**
      Returns the default orientation for the eWeekPrintType.
    */
    QPageLayout::Orientation defaultOrientation() const override;

    void print(QPainter &p, int width, int height) override;
    void readSettingsWidget() override;
    void setSettingsWidget() override;
    void doLoadConfig() override;
    void doSaveConfig() override;
    void setDateRange(const QDate &from, const QDate &to) override;

protected:
    enum eWeekPrintType { Filofax = 0, Timetable, SplitWeek } mWeekPrintType;

    /**
      Draw the week (filofax) table of the week containing the date qd. The first
      three days of the week will be shown in the first column (using drawDayBox),
      the remaining four in the second column, where the last two days of the week
      (typically Saturday and Sunday) only get half the height of the other day boxes.

      Obeys configuration options #mExcludeConfidential, #mExcludePrivate, #mShowNoteLines, #mUseColors,
      #mStartTime, #mEndTime, #mSingleLineLimit, #mIncludeDescription, #mIncludeCategories.
      @param p QPainter of the printout
      @param qd Arbitrary date within the week to be printed.
      @param box coordinates of the week box.
    */
    void drawWeek(QPainter &p, QDate qd, QRect box);
};

class CalPrintMonth : public CalPrintPluginBase
{
public:
    CalPrintMonth();
    ~CalPrintMonth() override;
    QString groupName() const override
    {
        return QStringLiteral("Print month");
    }

    QString description() const override
    {
        return i18n("Print mont&h");
    }

    QString info() const override
    {
        return i18n("Prints all events of one month on one page");
    }

    int sortID() const override
    {
        return CalPrinterBase::Month;
    }

    bool enabled() const override
    {
        return true;
    }

    QWidget *createConfigWidget(QWidget *) override;
    QPageLayout::Orientation defaultOrientation() const override
    {
        return QPageLayout::Landscape;
    }

public:
    void print(QPainter &p, int width, int height) override;
    void readSettingsWidget() override;
    void setSettingsWidget() override;
    void doLoadConfig() override;
    void doSaveConfig() override;
    void setDateRange(const QDate &from, const QDate &to) override;

protected:
    bool mWeekNumbers;
    bool mRecurDaily;
    bool mRecurWeekly;
    bool mIncludeTodos;
    bool mSingleLineLimit;
    bool mIncludeDescription;
    bool mIncludeCategories;
};

class CalPrintTodos : public CalPrintPluginBase
{
public:
    CalPrintTodos();
    ~CalPrintTodos() override;

    QString groupName() const override
    {
        return QStringLiteral("Print to-dos");
    }

    QString description() const override
    {
        return i18n("Print to-&dos");
    }

    QString info() const override
    {
        return i18n("Prints all to-dos in a (tree-like) list");
    }

    int sortID() const override
    {
        return CalPrinterBase::Todolist;
    }

    bool enabled() const override
    {
        return true;
    }

    QWidget *createConfigWidget(QWidget *) override;

public:
    void print(QPainter &p, int width, int height) override;
    void readSettingsWidget() override;
    void setSettingsWidget() override;
    void doLoadConfig() override;
    void doSaveConfig() override;

protected:
    QString mPageTitle;

    enum eTodoPrintType { TodosAll = 0, TodosUnfinished, TodosDueRange } mTodoPrintType;

    enum eTodoSortField {
        TodoFieldSummary = 0,
        TodoFieldStartDate,
        TodoFieldDueDate,
        TodoFieldPriority,
        TodoFieldPercentComplete,
        TodoFieldCategories,
        TodoFieldUnset
    } mTodoSortField;

    enum eTodoSortDirection { TodoDirectionAscending = 0, TodoDirectionDescending, TodoDirectionUnset } mTodoSortDirection;

    bool mIncludeDescription;
    bool mIncludePriority;
    bool mIncludeCategories;
    bool mIncludeStartDate;
    bool mIncludeDueDate;
    bool mIncludePercentComplete;
    bool mConnectSubTodos;
    bool mStrikeOutCompleted;
    bool mSortField;
    bool mSortDirection;
};

class CalPrintIncidenceConfig : public QWidget, public Ui::CalPrintIncidenceConfig_Base
{
    Q_OBJECT
public:
    explicit CalPrintIncidenceConfig(QWidget *parent)
        : QWidget(parent)
    {
        setupUi(this);
    }
};

class CalPrintDayConfig : public QWidget, public Ui::CalPrintDayConfig_Base
{
    Q_OBJECT
public:
    explicit CalPrintDayConfig(QWidget *parent)
        : QWidget(parent)
    {
        setupUi(this);
    }
};

class CalPrintWeekConfig : public QWidget, public Ui::CalPrintWeekConfig_Base
{
    Q_OBJECT
public:
    explicit CalPrintWeekConfig(QWidget *parent)
        : QWidget(parent)
    {
        setupUi(this);
    }
};

class CalPrintMonthConfig : public QWidget, public Ui::CalPrintMonthConfig_Base
{
    Q_OBJECT
public:
    explicit CalPrintMonthConfig(QWidget *parent)
        : QWidget(parent)
    {
        setupUi(this);
    }
};

class CalPrintTodoConfig : public QWidget, public Ui::CalPrintTodoConfig_Base
{
    Q_OBJECT
public:
    explicit CalPrintTodoConfig(QWidget *parent)
        : QWidget(parent)
    {
        setupUi(this);
    }
};
}

