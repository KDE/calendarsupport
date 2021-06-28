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
    void loadConfig() override;
    void saveConfig() override;

protected:
    int printCaptionAndText(QPainter &p, QRect box, const QString &caption, const QString &text, const QFont &captionFont, const QFont &textFont);

    bool mShowOptions;
    bool mShowSubitemsNotes;
    bool mShowAttendees;
    bool mShowAttachments;
};

class CalPrintDay : public CalPrintPluginBase
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

public:
    void print(QPainter &p, int width, int height) override;
    void readSettingsWidget() override;
    void setSettingsWidget() override;
    void loadConfig() override;
    void saveConfig() override;
    void setDateRange(const QDate &from, const QDate &to) override;

protected:
    enum eDayPrintType { Filofax = 0, Timetable, SingleTimetable } mDayPrintType;
    QTime mStartTime, mEndTime;
    bool mIncludeDescription;
    bool mIncludeCategories;
    bool mSingleLineLimit;
    bool mIncludeTodos;
    bool mIncludeAllEvents;
    bool mExcludeTime;
    bool mExcludeConfidential;
    bool mExcludePrivate;
};

class CalPrintWeek : public CalPrintPluginBase
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

public:
    void print(QPainter &p, int width, int height) override;
    void readSettingsWidget() override;
    void setSettingsWidget() override;
    void loadConfig() override;
    void saveConfig() override;
    void setDateRange(const QDate &from, const QDate &to) override;

protected:
    enum eWeekPrintType { Filofax = 0, Timetable, SplitWeek } mWeekPrintType;
    QTime mStartTime, mEndTime;
    bool mSingleLineLimit;
    bool mIncludeTodos;
    bool mIncludeDescription;
    bool mIncludeCategories;
    bool mIncludeAllEvents;
    bool mExcludeTime;
    bool mExcludeConfidential;
    bool mExcludePrivate;
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
    void loadConfig() override;
    void saveConfig() override;
    void setDateRange(const QDate &from, const QDate &to) override;

protected:
    bool mWeekNumbers;
    bool mRecurDaily;
    bool mRecurWeekly;
    bool mIncludeTodos;
    bool mSingleLineLimit;
    bool mIncludeDescription;
    bool mIncludeCategories;
    bool mExcludeConfidential;
    bool mExcludePrivate;
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
    void loadConfig() override;
    void saveConfig() override;

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
    bool mExcludeConfidential;
    bool mExcludePrivate;
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

