/*
  This file is part of KOrganizer.

  SPDX-FileCopyrightText: 2004 Reinhold Kainhofer <reinhold@kainhofer.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#pragma once

#include "calprintpluginbase.h"
#include "ui_calprintwhatsnextconfig_base.h"

using namespace KOrg;

class CalPrintWhatsNext : public CalPrintPluginBase
{
public:
    CalPrintWhatsNext()
        : CalPrintPluginBase()
    {
    }

    virtual ~CalPrintWhatsNext()
    {
    }

    virtual QString description()
    {
        return i18n("Print What's Next");
    }

    virtual QString info() const
    {
        return i18n("Prints a list of all upcoming events and todos.");
    }

    virtual QWidget *createConfigWidget(QWidget *);

    virtual int sortID()
    {
        return CalPrinterBase::WhatsNext;
    }

    virtual bool enabled()
    {
        return true;
    }

public:
    virtual void print(QPainter &p, int width, int height);
    virtual void readSettingsWidget();
    virtual void setSettingsWidget();
    virtual void loadConfig();
    virtual void saveConfig();
    virtual void setDateRange(const QDate &from, const QDate &to);

protected:
    bool mUseDateRange;
};

class CalPrintWhatsNextConfig : public QWidget, public Ui::CalPrintWhatsNextConfig_Base
{
public:
    explicit CalPrintWhatsNextConfig(QWidget *parent)
        : QWidget(parent)
    {
        setupUi(this);
    }
};

