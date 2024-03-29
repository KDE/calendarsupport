/*
  SPDX-FileCopyrightText: 2000, 2001 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2004 David Faure <faure@kde.org>
  SPDX-FileCopyrightText: 2004 Reinhold Kainhofer <reinhold@kainhofer.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#pragma once

#include "calendarsupport_export.h"

#include <Akonadi/ETMCalendar>
#include <Akonadi/Item>
#include <KCalendarCore/Event>
#include <KCalendarCore/Todo>

#include <QObject>

class QDate;

namespace Akonadi
{
class IncidenceChanger;
}

namespace CalendarSupport
{
/**
 * This class handles expiring and archiving of events.
 * It is used directly by the archivedialog, and it is also
 * triggered by actionmanager's timer for auto-archiving.
 *
 * The settings are not held in this class, but directly in KOPrefs (from korganizer.kcfg)
 * Be sure to set mArchiveAction and mArchiveFile before a manual archiving
 * mAutoArchive is used for auto archiving.
 */
class CALENDARSUPPORT_EXPORT EventArchiver : public QObject
{
    Q_OBJECT
public:
    explicit EventArchiver(QObject *parent = nullptr);
    ~EventArchiver() override;

    /**
     * Delete or archive events once
     * @param calendar the calendar to archive
     * @param limitDate all events *before* the limitDate (not included) will be deleted/archived.
     * @param widget parent widget for message boxes
     * Confirmation and "no events to process" dialogs will be shown
     */
    void runOnce(const Akonadi::ETMCalendar::Ptr &calendar, Akonadi::IncidenceChanger *changer, QDate limitDate, QWidget *widget);

    /**
     * Delete or archive events. This is called regularly, when auto-archiving
     * is enabled
     * @param calendar the calendar to archive
     * @param widget parent widget for message boxes
     * @param withGUI whether this is called from the dialog, so message boxes should be shown.
     * Note that error dialogs like "cannot save" are shown even if from this method, so widget
     * should be set in all cases.
     */
    void runAuto(const Akonadi::ETMCalendar::Ptr &calendar, Akonadi::IncidenceChanger *changer, QWidget *widget, bool withGUI);

Q_SIGNALS:
    void eventsDeleted();

private:
    CALENDARSUPPORT_NO_EXPORT void
    run(const Akonadi::ETMCalendar::Ptr &calendar, Akonadi::IncidenceChanger *changer, QDate limitDate, QWidget *widget, bool withGUI, bool errorIfNone);

    CALENDARSUPPORT_NO_EXPORT void
    deleteIncidences(Akonadi::IncidenceChanger *changer, QDate limitDate, QWidget *widget, const Akonadi::Item::List &items, bool withGUI);

    CALENDARSUPPORT_NO_EXPORT void archiveIncidences(const Akonadi::ETMCalendar::Ptr &calendar,
                                                     Akonadi::IncidenceChanger *changer,
                                                     QDate limitDate,
                                                     QWidget *widget,
                                                     const KCalendarCore::Incidence::List &incidences,
                                                     bool withGUI);

    /**
     * Checks if all to-dos under @p todo and including @p todo were completed before @p limitDate.
     * If not, we can't archive this to-do.
     * @param todo root of the sub-tree we are checking
     * @param limitDate
     * @param checkedUids used internally to prevent infinite recursion due to invalid calendar files
     */
    CALENDARSUPPORT_NO_EXPORT bool isSubTreeComplete(const Akonadi::ETMCalendar::Ptr &calendar,
                                                     const KCalendarCore::Todo::Ptr &todo,
                                                     QDate limitDate,
                                                     QStringList checkedUids = QStringList()) const;
};
}
