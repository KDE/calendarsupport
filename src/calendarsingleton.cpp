/*
  SPDX-FileCopyrightText: 2013 Sérgio Martins <iamsergio@gmail.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#include "calendarsingleton.h"
#include "kcalprefs.h"

#include <KCalendarCore/Person>

/**
 * Singleton is implemented through qApp parenting because we can't rely on K_GLOBAL_STATIC.
 *
 * QWidgets and QAbstractItemModels can't be global because their dtor depends on other globals
 * and the order of global destruction is undefined.
 */
Akonadi::ETMCalendar::Ptr CalendarSupport::calendarSingleton(bool createIfNull)
{
    static Akonadi::ETMCalendar::Ptr calendar;

    if (!calendar && createIfNull) {
        calendar = Akonadi::ETMCalendar::Ptr(new Akonadi::ETMCalendar());
        calendar->setCollectionFilteringEnabled(false);
        calendar->setOwner(KCalendarCore::Person(KCalPrefs::instance()->fullName(), KCalPrefs::instance()->email()));
    }

    return calendar;
}
