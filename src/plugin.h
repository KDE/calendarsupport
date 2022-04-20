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

   This class is used for calendarview's decoration plugins.
*/
class CALENDARSUPPORT_EXPORT Plugin : public QObject
{
    Q_OBJECT

public:
    Plugin(QObject *parent = nullptr, const QVariantList &args = {})
        : QObject(parent)
    {
        Q_UNUSED(args);
    }

    virtual QString info() const = 0;

    virtual void configure(QWidget *)
    {
    }
};

}
