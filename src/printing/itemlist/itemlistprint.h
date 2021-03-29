/*
  This file is part of KOrganizer.

  SPDX-FileCopyrightText: 2004 Reinhold Kainhofer <reinhold@kainhofer.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#pragma once

#include "calprintpluginbase.h"
#include "ui_calprintitemlistconfig_base.h"

using namespace KOrg;

class CalPrintItemList : public CalPrintPluginBase
{
public:
    CalPrintItemList()
        : CalPrintPluginBase()
    {
    }

    virtual ~CalPrintItemList()
    {
    }

    virtual QString description()
    {
        return i18n("Print Item list");
    }

    virtual QString info() const
    {
        return i18n("Prints a list of events and to-dos");
    }

    virtual QWidget *createConfigWidget(QWidget *);

    virtual int sortID()
    {
        return CalPrinterBase::ItemList;
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

class CalPrintItemListConfig : public QWidget, public Ui::CalPrintItemListConfig_Base
{
public:
    explicit CalPrintItemListConfig(QWidget *parent)
        : QWidget(parent)
    {
        setupUi(this);
    }
};

