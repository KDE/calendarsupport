/*
  SPDX-FileCopyrightText: 2001, 2003 Cornelius Schumacher <schumacher@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include "calendarsupport_export.h"

#include <KPluginFactory>

namespace CalendarSupport
{
/**
   @class Plugin

   @brief Specifies the plugin interface.

   This class is shared between korganizer's print plugins and
   calendarview's decoration plugins.
*/
class Plugin
{
    enum { INTERFACE_VERSION = 2 };

public:
    static int interfaceVersion()
    {
        return INTERFACE_VERSION;
    }

    static QString serviceType()
    {
        return QStringLiteral("Calendar/Plugin");
    }

    Plugin()
    {
    }

    virtual ~Plugin()
    {
    }

    virtual QString info() const = 0;

    virtual void configure(QWidget *)
    {
    }
};

class CALENDARSUPPORT_EXPORT PluginFactory : public KPluginFactory
{
    Q_OBJECT
public:
    virtual Plugin *createPluginFactory() = 0;

protected:
    QObject *createObject(QObject *, const char *, const QStringList &) override
    {
        return nullptr;
    }
};
}

