/*
  SPDX-FileCopyrightText: 2004 Reinhold Kainhofer <reinhold@kainhofer.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#pragma once

#include "calprintpluginbase.h"
using namespace Qt::Literals::StringLiterals;

#include "ui_calprintjournalconfig_base.h"

namespace CalendarSupport
{
class CalPrintJournal : public CalPrintPluginBase
{
public:
    CalPrintJournal()
        : CalPrintPluginBase()
    {
    }

    ~CalPrintJournal() override = default;

    [[nodiscard]] QString groupName() const override
    {
        return u"Print journal"_s;
    }

    [[nodiscard]] QString description() const override
    {
        return i18n("Print &journal");
    }

    [[nodiscard]] QString info() const override
    {
        return i18n("Prints all journals for a given date range");
    }

    QWidget *createConfigWidget(QWidget *) override;
    [[nodiscard]] int sortID() const override
    {
        return CalPrinterBase::Journallist;
    }

    [[nodiscard]] bool enabled() const override
    {
        return true;
    }

public:
    void print(QPainter &p, int width, int height) override;
    void readSettingsWidget() override;
    void setSettingsWidget() override;
    void doLoadConfig() override;
    void doSaveConfig() override;
    void setDateRange(const QDate &from, const QDate &to) override;

protected:
    /**
      Draws single journal item.

      Obeys configuration options #mExcludeConfidential, #excludePrivate.
      @param journal The item to be printed.
      @param p QPainter of the printout
      @param x x-coordinate of the upper left coordinate of the first item
      @param y y-coordinate of the upper left coordinate of the first item
      @param width width of the whole list
      @param pageHeight Total height allowed for the list on a page. If an item
                   would be below that line, a new page is started.
    */
    void drawJournal(const KCalendarCore::Journal::Ptr &journal, QPainter &p, int x, int &y, int width, int pageHeight);
    bool mUseDateRange;
};

class CalPrintJournalConfig : public QWidget, public Ui::CalPrintJournalConfig_Base
{
public:
    explicit CalPrintJournalConfig(QWidget *parent)
        : QWidget(parent)
    {
        setupUi(this);
    }
};
}
