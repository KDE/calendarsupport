/*
  SPDX-FileCopyrightText: 2003 Reinhold Kainhofer <reinhold@kainhofer.com>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "plugin.h"

#include <Akonadi/Calendar/ETMCalendar>

#include <KCalendarCore/Incidence>
#include <KConfig>

#include <QDate>
#include <QPrinter>

namespace CalendarSupport
{
/**
  Base class of Calendar printer class.
*/
class CalPrinterBase
{
public:
    enum PrintType { Incidence = 100, Day = 200, Week = 300, Month = 400, Year = 900, Todolist = 1000, Journallist = 2000, WhatsNext = 2100, ItemList = 2200 };
};

/**
  Base class for Calendar printing classes. Each sub class represents one
  calendar print format.
*/
class PrintPlugin : public Plugin
{
public:
    PrintPlugin()
        : Plugin()
        , mConfigWidget(nullptr)
        , mCalendar(nullptr)
    {
    }

    ~PrintPlugin() override
    {
    }

    using List = QList<PrintPlugin *>;
    static int interfaceVersion()
    {
        return 2;
    }

    virtual void setConfig(KConfig *cfg)
    {
        mConfig = cfg;
    }

    virtual void setCalendar(const Akonadi::ETMCalendar::Ptr &cal)
    {
        mCalendar = cal;
    }

    virtual void setSelectedIncidences(const KCalendarCore::Incidence::List &inc)
    {
        mSelectedIncidences = inc;
    }

    virtual KCalendarCore::Incidence::List selectedIncidences() const
    {
        return mSelectedIncidences;
    }

    /**
     Returns KConfig group name where store settings
    */
    virtual QString groupName() const = 0;
    /**
      Returns short description of print format.
    */
    virtual QString description() const = 0;
    /**
      Returns long description of print format.
    */
    QString info() const override = 0;

    /**
      Returns the sort ID of the plugin. This value will be used to identify
      the config widget in the widget stack, and to sort the plugin name in the
      print style selection list.
      If another plugin uses the same ID or a value of -1 is returned, a unique
      (negative) ID will be automatically generated and thus the position of
      the plugin in the selection list is undefined.
    */
    virtual int sortID() const
    {
        return -1;
    }

    /**
      Returns true if the plugin should be enabled; false otherwise.
    */
    virtual bool enabled() const
    {
        return false;
    }

    QWidget *configWidget(QWidget *w)
    {
        if (!mConfigWidget) {
            mConfigWidget = createConfigWidget(w);
            setSettingsWidget();
        }
        return mConfigWidget;
    }

    /* Create the config widget. setSettingsWidget will be automatically
       called on it */
    virtual QWidget *createConfigWidget(QWidget *) = 0;

    /**
      Actually do the printing.
    */
    virtual void doPrint(QPrinter *printer) = 0;

    /**
      Orientation of printout. Default is Portrait. If your plugin wants
      to use some other orientation as default (e.g. depending on some
      config settings), implement this function in your subclass and
      return the desired orientation.
    */
    virtual QPageLayout::Orientation defaultOrientation() const
    {
        return QPageLayout::Portrait;
    }

    /**
      Load complete configuration.  Each implementation calls its parent's
      implementation to load parent configuration options, then loads its own.
    */
    virtual void doLoadConfig()
    {
    }

    /**
      Save complete configuration.  Each implementation saves its own
      configuration options, then calls its parent's implementation to save
      parent options.
    */
    virtual void doSaveConfig()
    {
    }

public:
    /**
      Read settings from configuration widget and apply them to current object.
    */
    virtual void readSettingsWidget()
    {
    }

    /**
      Set configuration widget to reflect settings of current object.
    */
    virtual void setSettingsWidget()
    {
    }

    /**
      Set date range which should be printed.
    */
    virtual void setDateRange(const QDate &from, const QDate &to)
    {
        mFromDate = from;
        mToDate = to;
    }

protected:
    QDate mFromDate;
    QDate mToDate;

protected:
    QPointer<QWidget> mConfigWidget;
    /** The printer object. This will only be available in the doPrint method
        of the selected plugin */
    QPrinter *mPrinter = nullptr;
    Akonadi::ETMCalendar::Ptr mCalendar;
    KCalendarCore::Incidence::List mSelectedIncidences;
    KConfig *mConfig = nullptr;
};

class PrintPluginFactory : public PluginFactory
{
    Q_OBJECT
public:
    PrintPlugin *createPluginFactory() override = 0;
};
}

