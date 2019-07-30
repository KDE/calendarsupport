/*
  Copyright (c) 2009, 2010 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
    Author: Frank Osterfeld <osterfeld@kde.org>
    Author: Andras Mantia <andras@kdab.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/
#ifndef CALENDARSUPPORT_UTILS_H
#define CALENDARSUPPORT_UTILS_H

#include "calendarsupport_export.h"

#include <Collection>
#include <Item>
#include <Akonadi/Calendar/ETMCalendar>

#include <KCalendarCore/Event>
#include <KCalendarCore/Incidence>
#include <KCalendarCore/Journal>
#include <KCalendarCore/ScheduleMessage>
#include <KCalendarCore/Todo>

#include <QModelIndex>

namespace KCalendarCore {
class CalFilter;
}

namespace Akonadi {
class ETMCalendar;
}

class QAbstractItemModel;
class QDrag;
class QMimeData;

typedef QList<QModelIndex> QModelIndexList;

namespace CalendarSupport {
class Calendar;
/**
 * returns the incidence from an akonadi item, or a null pointer if the item has no such payload
 */
CALENDARSUPPORT_EXPORT KCalendarCore::Incidence::Ptr incidence(const Akonadi::Item &item);

/**
 * returns the event from an akonadi item, or a null pointer if the item has no such payload
 */
CALENDARSUPPORT_EXPORT KCalendarCore::Event::Ptr event(const Akonadi::Item &item);

/**
 * returns the event from an incidence, or a null pointer if the item has no such payload
 */
CALENDARSUPPORT_EXPORT KCalendarCore::Event::Ptr event(const KCalendarCore::Incidence::Ptr &incidence);

/**
 * returns incidence pointers from an akonadi item.
 */
CALENDARSUPPORT_EXPORT KCalendarCore::Incidence::List incidencesFromItems(
    const Akonadi::Item::List &items);

/**
 * returns the todo from an akonadi item, or a null pointer if the item has no such payload
 */
CALENDARSUPPORT_EXPORT KCalendarCore::Todo::Ptr todo(const Akonadi::Item &item);

/**
 * returns the todo from an incidence, or a null pointer if the item has no such payload
 */
CALENDARSUPPORT_EXPORT KCalendarCore::Todo::Ptr todo(const KCalendarCore::Incidence::Ptr &incidence);

/**
 * returns the journal from an akonadi item, or a null pointer if the item has no such payload
 */
CALENDARSUPPORT_EXPORT KCalendarCore::Journal::Ptr journal(const Akonadi::Item &item);

/**
 * returns the journal from an incidence, or a null pointer if the item has no such payload
 */
CALENDARSUPPORT_EXPORT KCalendarCore::Journal::Ptr journal(const KCalendarCore::Incidence::Ptr &incidence);

/**
 * returns whether an Akonadi item contains an incidence
 */
CALENDARSUPPORT_EXPORT bool hasIncidence(const Akonadi::Item &item);

/**
 * returns whether an Akonadi item contains an event
 */
CALENDARSUPPORT_EXPORT bool hasEvent(const Akonadi::Item &item);

/**
 * returns whether an incidence contains an event
 */
CALENDARSUPPORT_EXPORT bool hasEvent(const KCalendarCore::Incidence::Ptr &incidence);

/**
 * returns whether an Akonadi item contains a todo
 */
CALENDARSUPPORT_EXPORT bool hasTodo(const Akonadi::Item &item);

/**
 * returns whether an incidence contains a todo
 */
CALENDARSUPPORT_EXPORT bool hasTodo(const KCalendarCore::Incidence::Ptr &incidence);

/**
 * returns whether an Akonadi item contains a journal
 */
CALENDARSUPPORT_EXPORT bool hasJournal(const Akonadi::Item &item);

/**
 * returns whether an incidence contains a journal
 */
CALENDARSUPPORT_EXPORT bool hasJournal(const KCalendarCore::Incidence::Ptr &incidence);

/**
 * returns @p true if the URL represents an Akonadi item and has one of the given mimetypes.
 */
bool isValidIncidenceItemUrl(const QUrl &url, const QStringList &supportedMimeTypes);

bool isValidIncidenceItemUrl(const QUrl &url);

/**
 * returns @p true if the mime data object contains any of the following:
 *
 * - an Akonadi item with a supported KCal mimetype
 * - an iCalendar
 * - a VCard
 */
CALENDARSUPPORT_EXPORT bool canDecode(const QMimeData *mimeData);

CALENDARSUPPORT_EXPORT QList<QUrl> incidenceItemUrls(const QMimeData *mimeData);

CALENDARSUPPORT_EXPORT QList<QUrl> todoItemUrls(const QMimeData *mimeData);

CALENDARSUPPORT_EXPORT bool mimeDataHasIncidence(const QMimeData *mimeData);

CALENDARSUPPORT_EXPORT KCalendarCore::Todo::List todos(const QMimeData *mimeData);

CALENDARSUPPORT_EXPORT KCalendarCore::Incidence::List incidences(const QMimeData *mimeData);

/**
* creates mime data object for dragging an akonadi item containing an incidence
*/
CALENDARSUPPORT_EXPORT QMimeData *createMimeData(const Akonadi::Item &item);

/**
* creates mime data object for dragging akonadi items containing an incidence
*/
CALENDARSUPPORT_EXPORT QMimeData *createMimeData(const Akonadi::Item::List &items);

#ifndef QT_NO_DRAGANDDROP
/**
 * creates a drag object for dragging an akonadi item containing an incidence
 */
CALENDARSUPPORT_EXPORT QDrag *createDrag(const Akonadi::Item &item, QWidget *parent);

/**
 * creates a drag object for dragging akonadi items containing an incidence
 */
CALENDARSUPPORT_EXPORT QDrag *createDrag(const Akonadi::Item::List &items, QWidget *parent);
#endif
/**
  Applies a filter to a list of items containing incidences.
  Items not containing incidences or not matching the filter are removed.
  Helper method anologous to KCalendarCore::CalFilter::apply()
  @see KCalendarCore::CalFilter::apply()
  @param items the list of items to filter
  @param filter the filter to apply to the list of items
  @return the filtered list of items
*/
CALENDARSUPPORT_EXPORT Akonadi::Item::List applyCalFilter(const Akonadi::Item::List &items,
                                                          const KCalendarCore::CalFilter *filter);

/**
  Shows a modal dialog that allows to select a collection.

  @param will contain the dialogCode, QDialog::Accepted if the user pressed Ok,
  QDialog::Rejected otherwise
  @param parent The optional parent of the modal dialog.
  @return The select collection or an invalid collection if
  there was no collection selected.
*/
CALENDARSUPPORT_EXPORT Akonadi::Collection selectCollection(
    QWidget *parent, int &dialogCode, const QStringList &mimeTypes,
    const Akonadi::Collection &defaultCollection = Akonadi::Collection());

CALENDARSUPPORT_EXPORT Akonadi::Item itemFromIndex(const QModelIndex &index);

CALENDARSUPPORT_EXPORT Akonadi::Item::List itemsFromModel(
    const QAbstractItemModel *model,
    const QModelIndex &parentIndex = QModelIndex(), int start = 0, int end = -1);

CALENDARSUPPORT_EXPORT Akonadi::Collection::List collectionsFromModel(
    const QAbstractItemModel *model,
    const QModelIndex &parentIndex = QModelIndex(), int start = 0, int end = -1);

CALENDARSUPPORT_EXPORT Akonadi::Collection collectionFromIndex(const QModelIndex &index);

CALENDARSUPPORT_EXPORT Akonadi::Collection::Id collectionIdFromIndex(const QModelIndex &index);

CALENDARSUPPORT_EXPORT Akonadi::Collection::List collectionsFromIndexes(
    const QModelIndexList &indexes);

CALENDARSUPPORT_EXPORT QString displayName(Akonadi::ETMCalendar *calendar,
                                           const Akonadi::Collection &coll);

CALENDARSUPPORT_EXPORT QString subMimeTypeForIncidence(
    const KCalendarCore::Incidence::Ptr &incidence);

/**
 * Returns a list containing work days between @p start and @end.
 */
CALENDARSUPPORT_EXPORT QList<QDate> workDays(const QDate &start, const QDate &end);

/**
 * Creates a nicely formatted toolTip string for a calendar, containing some quick,
 * useful information to the user.
 *
 * @param coll is the Akonadi collection representing the calendar.
 * @param richText switches off richText (on by default) [CURRENTLY UNIMPLEMENTED]
 *
 * @return a QString containing the calendar info suitable for a toolTip.
 * @since 5.9
 */
CALENDARSUPPORT_EXPORT QString toolTipString(const Akonadi::Collection &coll, bool richText=true);

/**
 * Returns a list of holidays that occur at @param date.
 */
CALENDARSUPPORT_EXPORT QStringList holiday(const QDate &date);

CALENDARSUPPORT_EXPORT QStringList categories(const KCalendarCore::Incidence::List &incidences);

CALENDARSUPPORT_EXPORT bool mergeCalendar(const QString &srcFilename,
                                          const KCalendarCore::Calendar::Ptr &destCalendar);
}

#endif
