/*
  SPDX-FileCopyrightText: 2004 Reinhold Kainhofer <reinhold@kainhofer.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#pragma once

#include "calprintpluginbase.h"
#include "ui_calprintyearconfig_base.h"

namespace CalendarSupport
{
class CalPrintYear : public CalPrintPluginBase
{
public:
    CalPrintYear()
        : CalPrintPluginBase()
    {
    }

    ~CalPrintYear() override
    {
    }

    Q_REQUIRED_RESULT QString groupName() const override
    {
        return QStringLiteral("Print year");
    }

    Q_REQUIRED_RESULT QString description() const override
    {
        return i18n("Print &year");
    }

    Q_REQUIRED_RESULT QString info() const override
    {
        return i18n("Prints a calendar for an entire year");
    }

    Q_REQUIRED_RESULT int sortID() const override
    {
        return CalPrinterBase::Year;
    }

    Q_REQUIRED_RESULT bool enabled() const override
    {
        return true;
    }

    QWidget *createConfigWidget(QWidget *) override;
    Q_REQUIRED_RESULT QPageLayout::Orientation defaultOrientation() const override;

public:
    void print(QPainter &p, int width, int height) override;
    void readSettingsWidget() override;
    void setSettingsWidget() override;
    void doLoadConfig() override;
    void doSaveConfig() override;
    void setDateRange(const QDate &from, const QDate &to) override;

protected:
    int mYear;
    int mPages;
    int mSubDaysEvents;
    int mHolidaysEvents;
};

class CalPrintYearConfig : public QWidget, public Ui::CalPrintYearConfig_Base
{
public:
    explicit CalPrintYearConfig(QWidget *parent)
        : QWidget(parent)
    {
        setupUi(this);
    }
};
}

