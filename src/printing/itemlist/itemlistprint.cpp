/*
  This file is part of KOrganizer.

  SPDX-FileCopyrightText: 2004 Reinhold Kainhofer <reinhold@kainhofer.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#include "itemlistprint.h"

class ItemListPrintFactory : public KOrg::PrintPluginFactory
{
public:
    KOrg::PrintPlugin *createPluginFactory()
    {
        return new CalPrintItemList;
    }
};

/**************************************************************
 *           Print Item List
 **************************************************************/

QWidget *CalPrintItemList::createConfigWidget(QWidget *w)
{
    return new CalPrintItemListConfig(w);
}

void CalPrintItemList::readSettingsWidget()
{
    CalPrintItemListConfig *cfg
        = dynamic_cast<CalPrintItemListConfig *>((QWidget *)mConfigWidget);
    if (cfg) {
        mFromDate = cfg->mFromDate->date();
        mToDate = cfg->mToDate->date();
    }
}

void CalPrintItemList::setSettingsWidget()
{
    CalPrintItemListConfig *cfg
        = dynamic_cast<CalPrintItemListConfig *>((QWidget *)mConfigWidget);
    if (cfg) {
        cfg->mFromDate->setDate(mFromDate);
        cfg->mToDate->setDate(mToDate);
    }
}

void CalPrintItemList::loadConfig()
{
    if (mConfig) {
        KConfigGroup config(mConfig, "Itemlistprint");
        //TODO: Read in settings
    }
    setSettingsWidget();
}

void CalPrintItemList::saveConfig()
{
    qCDebug(CALENDARSUPPORT_LOG);

    readSettingsWidget();
    if (mConfig) {
        KConfigGroup config(mConfig, "Itemlistprint");
        //TODO: Write out settings
    }
}

void CalPrintItemList::setDateRange(const QDate &from, const QDate &to)
{
    CalPrintPluginBase::setDateRange(from, to);
    CalPrintItemListConfig *cfg
        = dynamic_cast<CalPrintItemListConfig *>((QWidget *)mConfigWidget);
    if (cfg) {
        cfg->mFromDate->setDate(from);
        cfg->mToDate->setDate(to);
    }
}

void CalPrintItemList::print(QPainter &p, int width, int height)
{
    Q_UNUSED(p)
    Q_UNUSED(width)
    Q_UNUSED(height)
    //TODO: Print something!
}
