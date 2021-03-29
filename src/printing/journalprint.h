/*
  SPDX-FileCopyrightText: 2004 Reinhold Kainhofer <reinhold@kainhofer.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#pragma once

#include "calendarsupport_export.h"
#include "calprintpluginbase.h"
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

    ~CalPrintJournal() override
    {
    }

    Q_REQUIRED_RESULT QString groupName() const override
    {
        return QStringLiteral("Print journal");
    }

    Q_REQUIRED_RESULT QString description() const override
    {
        return i18n("Print &journal");
    }

    Q_REQUIRED_RESULT QString info() const override
    {
        return i18n("Prints all journals for a given date range");
    }

    QWidget *createConfigWidget(QWidget *) override;
    Q_REQUIRED_RESULT int sortID() const override
    {
        return CalPrinterBase::Journallist;
    }

    Q_REQUIRED_RESULT bool enabled() const override
    {
        return true;
    }

public:
    void print(QPainter &p, int width, int height) override;
    void readSettingsWidget() override;
    void setSettingsWidget() override;
    void loadConfig() override;
    void saveConfig() override;
    void setDateRange(const QDate &from, const QDate &to) override;

protected:
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

