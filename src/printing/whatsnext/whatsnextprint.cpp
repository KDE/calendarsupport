/*
  This file is part of KOrganizer.

  SPDX-FileCopyrightText: 2004 Reinhold Kainhofer <reinhold@kainhofer.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#include "whatsnextprint.h"

class WhatsNextPrintFactory : public KOrg::PrintPluginFactory
{
public:
    KOrg::PrintPlugin *createPluginFactory()
    {
        return new CalPrintWhatsNext;
    }
};

/**************************************************************
 *           Print What's Next
 **************************************************************/

QWidget *CalPrintWhatsNext::createConfigWidget(QWidget *w)
{
    return new CalPrintWhatsNextConfig(w);
}

void CalPrintWhatsNext::readSettingsWidget()
{
    CalPrintWhatsNextConfig *cfg
        = dynamic_cast<CalPrintWhatsNextConfig *>((QWidget *)mConfigWidget);
    if (cfg) {
        mFromDate = cfg->mFromDate->date();
        mToDate = cfg->mToDate->date();
    }
}

void CalPrintWhatsNext::setSettingsWidget()
{
    CalPrintWhatsNextConfig *cfg
        = dynamic_cast<CalPrintWhatsNextConfig *>((QWidget *)mConfigWidget);
    if (cfg) {
        cfg->mFromDate->setDate(mFromDate);
        cfg->mToDate->setDate(mToDate);
    }
}

void CalPrintWhatsNext::loadConfig()
{
    if (mConfig) {
        KConfigGroup config(mConfig, "Whatsnextprint");
        //TODO: Read in settings
    }
    setSettingsWidget();
}

void CalPrintWhatsNext::saveConfig()
{
    qCDebug(CALENDARSUPPORT_LOG);

    readSettingsWidget();
    if (mConfig) {
        KConfigGroup config(mConfig, "Whatsnextprint");
        //TODO: Write out settings
    }
}

void CalPrintWhatsNext::setDateRange(const QDate &from, const QDate &to)
{
    CalPrintPluginBase::setDateRange(from, to);
    CalPrintWhatsNextConfig *cfg
        = dynamic_cast<CalPrintWhatsNextConfig *>((QWidget *)mConfigWidget);
    if (cfg) {
        cfg->mFromDate->setDate(from);
        cfg->mToDate->setDate(to);
    }
}

void CalPrintWhatsNext::print(QPainter &p, int width, int height)
{
    Q_UNUSED(p);
    Q_UNUSED(width);
    Q_UNUSED(height);
    //TODO: Print something!
}
