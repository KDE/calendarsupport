/*
  SPDX-FileCopyrightText: 2013 Sérgio Martins <iamsergio@gmail.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#pragma once

#include "calendarsupport_export.h"
#include <Akonadi/Calendar/ETMCalendar>

namespace CalendarSupport
{
/**
 * ETMCalendar to be used by kontact plugins to avoid having
 * 3 loaded calendars which occupy lots of memory.
 */
CALENDARSUPPORT_EXPORT Akonadi::ETMCalendar::Ptr calendarSingleton(bool createIfNull = true);
}

