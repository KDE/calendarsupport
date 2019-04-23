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

#include "utils.h"
#include "kcalprefs.h"

#include <Collection>
#include <CollectionDialog>
#include <EntityDisplayAttribute>
#include <EntityTreeModel>
#include <Item>

#include <AkonadiCore/AgentInstance>
#include <AkonadiCore/AgentManager>
#include <AkonadiCore/EntityDisplayAttribute>
#include <Akonadi/Calendar/ETMCalendar>
#include <Akonadi/Calendar/PublishDialog>
#include <akonadi/calendar/calendarsettings.h>

#include <KHolidays/HolidayRegion>

#include <KCalCore/CalFilter>
#include <KCalCore/Event>
#include <KCalCore/FreeBusy>
#include <KCalCore/Incidence>
#include <KCalCore/Journal>
#include <KCalCore/MemoryCalendar>
#include <KCalCore/Todo>
#include <KCalCore/ICalFormat>
#include <KCalCore/FileStorage>

#include <KCalUtils/DndFactory>
#include <KCalUtils/ICalDrag>
#include <KCalUtils/VCalDrag>

#include <KMessageBox>
#include <KIO/FileCopyJob>
#include <KIconLoader>
#include <KLocalizedString>

#include <QUrl>
#include <QAbstractItemModel>
#include <QDrag>
#include <QMimeData>
#include <QModelIndex>
#include <QPointer>
#include <QFileDialog>
#include <QUrlQuery>
#include <QRegularExpression>
#include <QTimeZone>

#include "calendarsupport_debug.h"

using namespace CalendarSupport;
using namespace KHolidays;
using namespace KCalCore;

KCalCore::Incidence::Ptr CalendarSupport::incidence(const Akonadi::Item &item)
{
    //relying on exception for performance reasons
    try {
        return item.payload<KCalCore::Incidence::Ptr>();
    } catch (const Akonadi::PayloadException &) {
        return KCalCore::Incidence::Ptr();
    }
}

KCalCore::Event::Ptr CalendarSupport::event(const Akonadi::Item &item)
{
    //relying on exception for performance reasons
    try {
        KCalCore::Incidence::Ptr incidence = item.payload<KCalCore::Incidence::Ptr>();
        if (hasEvent(incidence)) {
            return item.payload<KCalCore::Event::Ptr>();
        }
    } catch (const Akonadi::PayloadException &) {
        return KCalCore::Event::Ptr();
    }
    return KCalCore::Event::Ptr();
}

KCalCore::Event::Ptr CalendarSupport::event(const KCalCore::Incidence::Ptr &incidence)
{
    if (hasEvent(incidence)) {
        return incidence.staticCast<KCalCore::Event>();
    }
    return KCalCore::Event::Ptr();
}

KCalCore::Incidence::List CalendarSupport::incidencesFromItems(const Akonadi::Item::List &items)
{
    KCalCore::Incidence::List incidences;
    for (const Akonadi::Item &item : items) {
        if (const KCalCore::Incidence::Ptr e = CalendarSupport::incidence(item)) {
            incidences.push_back(e);
        }
    }
    return incidences;
}

KCalCore::Todo::Ptr CalendarSupport::todo(const Akonadi::Item &item)
{
    try {
        KCalCore::Incidence::Ptr incidence = item.payload<KCalCore::Incidence::Ptr>();
        if (hasTodo(incidence)) {
            return item.payload<KCalCore::Todo::Ptr>();
        }
    } catch (const Akonadi::PayloadException &) {
        return KCalCore::Todo::Ptr();
    }
    return KCalCore::Todo::Ptr();
}

KCalCore::Todo::Ptr CalendarSupport::todo(const KCalCore::Incidence::Ptr &incidence)
{
    if (hasTodo(incidence)) {
        return incidence.staticCast<KCalCore::Todo>();
    }
    return KCalCore::Todo::Ptr();
}

KCalCore::Journal::Ptr CalendarSupport::journal(const Akonadi::Item &item)
{
    try {
        KCalCore::Incidence::Ptr incidence = item.payload<KCalCore::Incidence::Ptr>();
        if (hasJournal(incidence)) {
            return item.payload<KCalCore::Journal::Ptr>();
        }
    } catch (const Akonadi::PayloadException &) {
        return KCalCore::Journal::Ptr();
    }
    return KCalCore::Journal::Ptr();
}

KCalCore::Journal::Ptr CalendarSupport::journal(const KCalCore::Incidence::Ptr &incidence)
{
    if (hasJournal(incidence)) {
        return incidence.staticCast<KCalCore::Journal>();
    }
    return KCalCore::Journal::Ptr();
}

bool CalendarSupport::hasIncidence(const Akonadi::Item &item)
{
    return item.hasPayload<KCalCore::Incidence::Ptr>();
}

bool CalendarSupport::hasEvent(const Akonadi::Item &item)
{
    return item.hasPayload<KCalCore::Event::Ptr>();
}

bool CalendarSupport::hasEvent(const KCalCore::Incidence::Ptr &incidence)
{
    return incidence && incidence->type() == KCalCore::Incidence::TypeEvent;
}

bool CalendarSupport::hasTodo(const Akonadi::Item &item)
{
    return item.hasPayload<KCalCore::Todo::Ptr>();
}

bool CalendarSupport::hasTodo(const KCalCore::Incidence::Ptr &incidence)
{
    return incidence && incidence->type() == KCalCore::Incidence::TypeTodo;
}

bool CalendarSupport::hasJournal(const Akonadi::Item &item)
{
    return item.hasPayload<KCalCore::Journal::Ptr>();
}

bool CalendarSupport::hasJournal(const KCalCore::Incidence::Ptr &incidence)
{
    return incidence && incidence->type() == KCalCore::Incidence::TypeJournal;
}

QMimeData *CalendarSupport::createMimeData(const Akonadi::Item::List &items)
{
    if (items.isEmpty()) {
        return nullptr;
    }

    KCalCore::MemoryCalendar::Ptr cal(new KCalCore::MemoryCalendar(QTimeZone::systemTimeZone()));

    QList<QUrl> urls;
    int incidencesFound = 0;
    for (const Akonadi::Item &item : items) {
        const KCalCore::Incidence::Ptr incidence(CalendarSupport::incidence(item));
        if (!incidence) {
            continue;
        }
        ++incidencesFound;
        urls.push_back(item.url());
        KCalCore::Incidence::Ptr i(incidence->clone());
        cal->addIncidence(i);
    }

    if (incidencesFound == 0) {
        return nullptr;
    }

    std::unique_ptr<QMimeData> mimeData(new QMimeData);

    mimeData->setUrls(urls);

    KCalUtils::ICalDrag::populateMimeData(mimeData.get(), cal);

    return mimeData.release();
}

QMimeData *CalendarSupport::createMimeData(const Akonadi::Item &item)
{
    return createMimeData(Akonadi::Item::List() << item);
}

#ifndef QT_NO_DRAGANDDROP
QDrag *CalendarSupport::createDrag(const Akonadi::Item &item, QWidget *parent)
{
    return createDrag(Akonadi::Item::List() << item, parent);
}

#endif

static QByteArray findMostCommonType(const Akonadi::Item::List &items)
{
    QByteArray prev;
    if (items.isEmpty()) {
        return "Incidence";
    }

    for (const Akonadi::Item &item : items) {
        if (!CalendarSupport::hasIncidence(item)) {
            continue;
        }
        const QByteArray type = CalendarSupport::incidence(item)->typeStr();
        if (!prev.isEmpty() && type != prev) {
            return "Incidence";
        }
        prev = type;
    }
    return prev;
}

#ifndef QT_NO_DRAGANDDROP
QDrag *CalendarSupport::createDrag(const Akonadi::Item::List &items, QWidget *parent)
{
    std::unique_ptr<QDrag> drag(new QDrag(parent));
    drag->setMimeData(CalendarSupport::createMimeData(items));

    const QByteArray common = findMostCommonType(items);
    if (common == "Event") {
        drag->setPixmap(BarIcon(QStringLiteral("view-calendar-day")));
    } else if (common == "Todo") {
        drag->setPixmap(BarIcon(QStringLiteral("view-calendar-tasks")));
    }

    return drag.release();
}

#endif

static bool itemMatches(const Akonadi::Item &item, const KCalCore::CalFilter *filter)
{
    assert(filter);
    KCalCore::Incidence::Ptr inc = CalendarSupport::incidence(item);
    if (!inc) {
        return false;
    }
    return filter->filterIncidence(inc);
}

Akonadi::Item::List CalendarSupport::applyCalFilter(const Akonadi::Item::List &items_,
                                                    const KCalCore::CalFilter *filter)
{
    Q_ASSERT(filter);
    Akonadi::Item::List items(items_);
    items.erase(std::remove_if(items.begin(), items.end(), [filter](const Akonadi::Item &item) {
        return !itemMatches(item, filter);
    }), items.end());
    return items;
}

bool CalendarSupport::isValidIncidenceItemUrl(const QUrl &url,
                                              const QStringList &supportedMimeTypes)
{
    if (!url.isValid()) {
        return false;
    }

    if (url.scheme() != QLatin1String("akonadi")) {
        return false;
    }

    return supportedMimeTypes.contains(QUrlQuery(url).queryItemValue(QStringLiteral("type")));
}

bool CalendarSupport::isValidIncidenceItemUrl(const QUrl &url)
{
    return isValidIncidenceItemUrl(url,
                                   QStringList() << KCalCore::Event::eventMimeType()
                                                 << KCalCore::Todo::todoMimeType()
                                                 << KCalCore::Journal::journalMimeType()
                                                 << KCalCore::FreeBusy::freeBusyMimeType());
}

static bool containsValidIncidenceItemUrl(const QList<QUrl> &urls)
{
    return
        std::find_if(urls.begin(), urls.end(), [](const QUrl &url) {
        return CalendarSupport::isValidIncidenceItemUrl(url);
    }) != urls.constEnd();
}

bool CalendarSupport::canDecode(const QMimeData *md)
{
    if (md) {
        return
            containsValidIncidenceItemUrl(md->urls())
            || KCalUtils::ICalDrag::canDecode(md)
            || KCalUtils::VCalDrag::canDecode(md);
    } else {
        return false;
    }
}

QList<QUrl> CalendarSupport::incidenceItemUrls(const QMimeData *mimeData)
{
    QList<QUrl> urls;
    const QList<QUrl> urlsList = mimeData->urls();
    for (const QUrl &i : urlsList) {
        if (isValidIncidenceItemUrl(i)) {
            urls.push_back(i);
        }
    }
    return urls;
}

QList<QUrl> CalendarSupport::todoItemUrls(const QMimeData *mimeData)
{
    QList<QUrl> urls;

    const QList<QUrl> urlList = mimeData->urls();
    for (const QUrl &i : urlList) {
        if (isValidIncidenceItemUrl(i, QStringList() << KCalCore::Todo::todoMimeType())) {
            urls.push_back(i);
        }
    }
    return urls;
}

bool CalendarSupport::mimeDataHasIncidence(const QMimeData *mimeData)
{
    return !incidenceItemUrls(mimeData).isEmpty()
           || !incidences(mimeData).isEmpty();
}

KCalCore::Todo::List CalendarSupport::todos(const QMimeData *mimeData)
{
    KCalCore::Todo::List todos;

#ifndef QT_NO_DRAGANDDROP
    KCalCore::Calendar::Ptr cal(KCalUtils::DndFactory::createDropCalendar(mimeData));
    if (cal) {
        const KCalCore::Todo::List calTodos = cal->todos();
        todos.reserve(calTodos.count());
        for (const KCalCore::Todo::Ptr &i : calTodos) {
            todos.push_back(KCalCore::Todo::Ptr(i->clone()));
        }
    }
#endif

    return todos;
}

KCalCore::Incidence::List CalendarSupport::incidences(const QMimeData *mimeData)
{
    KCalCore::Incidence::List incidences;

#ifndef QT_NO_DRAGANDDROP
    KCalCore::Calendar::Ptr cal(KCalUtils::DndFactory::createDropCalendar(mimeData));
    if (cal) {
        const KCalCore::Incidence::List calIncidences = cal->incidences();
        incidences.reserve(calIncidences.count());
        for (const KCalCore::Incidence::Ptr &i : calIncidences) {
            incidences.push_back(KCalCore::Incidence::Ptr(i->clone()));
        }
    }
#endif

    return incidences;
}

Akonadi::Collection CalendarSupport::selectCollection(QWidget *parent, int &dialogCode,
                                                      const QStringList &mimeTypes,
                                                      const Akonadi::Collection &defCollection)
{
    QPointer<Akonadi::CollectionDialog> dlg(new Akonadi::CollectionDialog(parent));
    dlg->setWindowTitle(i18n("Select Calendar"));
    dlg->setDescription(i18n("Select the calendar where this item will be stored."));
    dlg->changeCollectionDialogOptions(Akonadi::CollectionDialog::KeepTreeExpanded);
    qCDebug(CALENDARSUPPORT_LOG) << "selecting collections with mimeType in " << mimeTypes;

    dlg->setMimeTypeFilter(mimeTypes);
    dlg->setAccessRightsFilter(Akonadi::Collection::CanCreateItem);
    if (defCollection.isValid()) {
        dlg->setDefaultCollection(defCollection);
    }
    Akonadi::Collection collection;

    // FIXME: don't use exec.
    dialogCode = dlg->exec();
    if (dlg && dialogCode == QDialog::Accepted) {
        collection = dlg->selectedCollection();

        if (!collection.isValid()) {
            qCWarning(CALENDARSUPPORT_LOG) << "An invalid collection was selected!";
        }
    }
    delete dlg;
    return collection;
}

Akonadi::Item CalendarSupport::itemFromIndex(const QModelIndex &idx)
{
    Akonadi::Item item = idx.data(Akonadi::EntityTreeModel::ItemRole).value<Akonadi::Item>();
    item.setParentCollection(
        idx.data(Akonadi::EntityTreeModel::ParentCollectionRole).value<Akonadi::Collection>());
    return item;
}

Akonadi::Collection::List CalendarSupport::collectionsFromModel(const QAbstractItemModel *model,
                                                                const QModelIndex &parentIndex,
                                                                int start, int end)
{
    const int endRow = end >= 0 ? end : model->rowCount(parentIndex) - 1;
    Akonadi::Collection::List collections;
    int row = start;
    QModelIndex i = model->index(row, 0, parentIndex);
    while (row <= endRow) {
        const Akonadi::Collection collection = collectionFromIndex(i);
        if (collection.isValid()) {
            collections << collection;
            QModelIndex childIndex = model->index(0, 0, i);
            if (childIndex.isValid()) {
                collections << collectionsFromModel(model, i);
            }
        }
        ++row;
        i = i.sibling(row, 0);
    }
    return collections;
}

Akonadi::Item::List CalendarSupport::itemsFromModel(const QAbstractItemModel *model,
                                                    const QModelIndex &parentIndex, int start,
                                                    int end)
{
    const int endRow = end >= 0 ? end : model->rowCount(parentIndex) - 1;
    Akonadi::Item::List items;
    int row = start;
    QModelIndex i = model->index(row, 0, parentIndex);
    while (row <= endRow) {
        const Akonadi::Item item = itemFromIndex(i);
        if (CalendarSupport::hasIncidence(item)) {
            items << item;
        } else {
            QModelIndex childIndex = model->index(0, 0, i);
            if (childIndex.isValid()) {
                items << itemsFromModel(model, i);
            }
        }
        ++row;
        i = i.sibling(row, 0);
    }
    return items;
}

Akonadi::Collection CalendarSupport::collectionFromIndex(const QModelIndex &index)
{
    return index.data(Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
}

Akonadi::Collection::Id CalendarSupport::collectionIdFromIndex(const QModelIndex &index)
{
    return index.data(Akonadi::EntityTreeModel::CollectionIdRole).value<Akonadi::Collection::Id>();
}

Akonadi::Collection::List CalendarSupport::collectionsFromIndexes(const QModelIndexList &indexes)
{
    Akonadi::Collection::List l;
    l.reserve(indexes.count());
    for (const QModelIndex &idx : indexes) {
        l.push_back(collectionFromIndex(idx));
    }
    return l;
}

QString CalendarSupport::displayName(Akonadi::ETMCalendar *calendar, const Akonadi::Collection &c)
{
    Akonadi::Collection fullCollection;
    if (calendar && calendar->collection(c.id()).isValid()) {
        fullCollection = calendar->collection(c.id());
    } else {
        fullCollection = c;
    }

    QString cName = fullCollection.name();
    const QString resourceName = fullCollection.resource();

    // Kolab Groupware
    if (resourceName.contains(QStringLiteral("kolab"))) {
        QString typeStr = cName; // contents type: "Calendar", "Tasks", etc
        QString ownerStr;        // folder owner: "fred", "ethel", etc
        QString nameStr;         // folder name: "Public", "Test", etc
        if (calendar) {
            Akonadi::Collection p = c.parentCollection();
            while (p != Akonadi::Collection::root()) {
                Akonadi::Collection tCol = calendar->collection(p.id());
                const QString tName = tCol.name();
                if (tName.startsWith(QLatin1String("shared.cal"), Qt::CaseInsensitive)) {
                    ownerStr = QStringLiteral("Shared");
                    nameStr = cName;
                    typeStr = i18n("Calendar");
                    break;
                } else if (tName.startsWith(QLatin1String("shared.tasks"), Qt::CaseInsensitive) ||
                           tName.startsWith(QLatin1String("shared.todo"), Qt::CaseInsensitive)) {
                    ownerStr = QStringLiteral("Shared");
                    nameStr = cName;
                    typeStr = i18n("Tasks");
                    break;
                } else if (tName.startsWith(QLatin1String("shared.journal"), Qt::CaseInsensitive)) {
                    ownerStr = QStringLiteral("Shared");
                    nameStr = cName;
                    typeStr = i18n("Journal");
                    break;
                } else if (tName.startsWith(QLatin1String("shared.notes"), Qt::CaseInsensitive)) {
                    ownerStr = QStringLiteral("Shared");
                    nameStr = cName;
                    typeStr = i18n("Notes");
                    break;
                } else if (tName != i18n("Calendar") &&
                           tName != i18n("Tasks") &&
                           tName != i18n("Journal") &&
                           tName != i18n("Notes")) {
                    ownerStr = tName;
                    break;
                } else {
                    nameStr = typeStr;
                    typeStr = tName;
                }
                p = p.parentCollection();
            }
        }

        if (!ownerStr.isEmpty()) {
            if (!ownerStr.compare(QLatin1String("INBOX"), Qt::CaseInsensitive)) {
                return i18nc("%1 is folder contents",
                             "My Kolab %1", typeStr);
            } else if (!ownerStr.compare(QLatin1String("SHARED"), Qt::CaseInsensitive) ||
                       !ownerStr.compare(QLatin1String("CALENDAR"), Qt::CaseInsensitive) ||
                       !ownerStr.compare(QLatin1String("RESOURCES"), Qt::CaseInsensitive)) {
                return i18nc("%1 is folder name, %2 is folder contents",
                             "Shared Kolab %1 %2", nameStr, typeStr);
            } else {
                if (nameStr.isEmpty()) {
                    return i18nc("%1 is folder owner name, %2 is folder contents",
                                 "%1's Kolab %2", ownerStr, typeStr);
                } else {
                    return i18nc(
                        "%1 is folder owner name, %2 is folder name, %3 is folder contents",
                        "%1's %2 Kolab %3", ownerStr, nameStr, typeStr);
                }
            }
        } else {
            return i18nc("%1 is folder contents",
                         "Kolab %1", typeStr);
        }
    } //end kolab section

    // Dav Groupware
    if (resourceName.contains(QStringLiteral("davgroupware"))) {
        const QString resourceDisplayName
            = Akonadi::AgentManager::self()->instance(resourceName).name();
        return i18nc("%1 is the folder name", "%1 in %2",
                     fullCollection.displayName(), resourceDisplayName);
    } //end caldav section

    // Google
    if (resourceName.contains(QStringLiteral("google"))) {
        QString ownerStr;        // folder owner: "user@gmail.com"
        if (calendar) {
            Akonadi::Collection p = c.parentCollection();
            ownerStr = calendar->collection(p.id()).displayName();
        }

        const QString nameStr = c.displayName(); // folder name: can be anything

        QString typeStr;
        const QString mimeStr = c.contentMimeTypes().join(QLatin1Char(','));
        if (mimeStr.contains(QStringLiteral(".event"))) {
            typeStr = i18n("Calendar");
        } else if (mimeStr.contains(QStringLiteral(".todo"))) {
            typeStr = i18n("Tasks");
        } else if (mimeStr.contains(QStringLiteral(".journal"))) {
            typeStr = i18n("Journal");
        } else if (mimeStr.contains(QStringLiteral(".note"))) {
            typeStr = i18n("Notes");
        } else {
            typeStr = mimeStr;
        }

        if (!ownerStr.isEmpty()) {
            const int atChar = ownerStr.lastIndexOf(QLatin1Char('@'));
            if (atChar >= 0) {
                ownerStr.truncate(atChar);
            }
            if (nameStr.isEmpty()) {
                return i18nc("%1 is folder owner name, %2 is folder contents",
                             "%1's Google %2", ownerStr, typeStr);
            } else {
                return i18nc("%1 is folder owner name, %2 is folder name",
                             "%1's %2", ownerStr, nameStr);
            }
        } else {
            return i18nc("%1 is folder contents",
                         "Google %1", typeStr);
        }
    } //end google section

    // Not groupware so the collection is "mine"
    const QString dName = fullCollection.displayName();

    if (!dName.isEmpty()) {
        return fullCollection.name().startsWith(QLatin1String("akonadi_")) ? i18n("My %1",
                                                                                   dName) : dName;
    } else if (!fullCollection.name().isEmpty()) {
        return fullCollection.name();
    } else {
        return i18nc("unknown resource", "Unknown");
    }
}

QString CalendarSupport::toolTipString(const Akonadi::Collection &coll, bool richText)
{
    Q_UNUSED(richText);

    QString str = QStringLiteral("<qt>");

    // Display Name
    QString displayName;
    if (coll.hasAttribute<Akonadi::EntityDisplayAttribute>()) {
        displayName = coll.attribute<Akonadi::EntityDisplayAttribute>()->displayName();
    }

    if (displayName.isEmpty()) {
        displayName = coll.name();
    }
    str += QLatin1String("<b>") + displayName + QLatin1String("</b>");
    str += QLatin1String("<hr>");

    // Calendar Type
    QString calendarType;
    if (!coll.isVirtual()) {
        const Akonadi::AgentInstance instance =
            Akonadi::AgentManager::self()->instance(coll.resource());
        calendarType = instance.type().name();
    } else {
        calendarType = i18nc("unknown calendar type", "unknown");
    }
    str += QLatin1String("<i>") + i18n("Calendar type:") + QLatin1String("</i>");
    str += QLatin1String("&nbsp;") + calendarType;

    // Read only?
    bool isReadOnly = !(coll.rights() & Akonadi::Collection::CanChangeItem);
    str += QLatin1String("<br>");
    str += QLatin1String("<i>") + i18n("Rights:") + QLatin1String("</i>");
    str += QLatin1String("&nbsp;");
    if (isReadOnly) {
        str += i18nc("the calendar is read-only", "read-only");
    } else {
        str += i18nc("the calendar is read and write", "read+write");
    }
    str += QLatin1String("</br>");

    str += QLatin1String("</qt>");
    return str;
 }

QString CalendarSupport::subMimeTypeForIncidence(const KCalCore::Incidence::Ptr &incidence)
{
    return incidence->mimeType();
}

QList<QDate> CalendarSupport::workDays(const QDate &startDate, const QDate &endDate)
{
    QList<QDate> result;

    const int mask(~(KCalPrefs::instance()->mWorkWeekMask));
    const int numDays = startDate.daysTo(endDate) + 1;

    for (int i = 0; i < numDays; ++i) {
        const QDate date = startDate.addDays(i);
        if (!(mask & (1 << (date.dayOfWeek() - 1)))) {
            result.append(date);
        }
    }

    if (KCalPrefs::instance()->mExcludeHolidays) {
        const QStringList holidays = KCalPrefs::instance()->mHolidays;
        for (const QString &regionStr : holidays) {
            KHolidays::HolidayRegion region(regionStr);
            if (region.isValid()) {
                const KHolidays::Holiday::List list = region.holidays(startDate, endDate);
                const int listCount(list.count());
                for (int i = 0; i < listCount; ++i) {
                    const Holiday &h = list.at(i);
                    if (h.dayType() == Holiday::NonWorkday) {
                        result.removeAll(h.observedStartDate());
                    }
                }
            }
        }
    }

    return result;
}

QStringList CalendarSupport::holiday(const QDate &date)
{
    QStringList hdays;

    bool showCountryCode = (KCalPrefs::instance()->mHolidays.count() > 1);
    const QStringList holidays = KCalPrefs::instance()->mHolidays;
    for (const QString &regionStr : holidays) {
        KHolidays::HolidayRegion region(regionStr);
        if (region.isValid()) {
            const Holiday::List list = region.holidays(date);
            const int listCount = list.count();
            for (int i = 0; i < listCount; ++i) {
                // don't add duplicates.
                // TODO: won't find duplicates in different languages however.
                const QString name = list.at(i).name();
                if (showCountryCode) {
                    // If more than one holiday region, append the country code to the holiday
                    // display name to help the user identify which region it belongs to.
                    const QRegularExpression holidaySE(
                        i18nc("search pattern for holidayname", "^%1", name));
                    if (hdays.filter(holidaySE).isEmpty()) {
                        const QString pholiday = i18n("%1 (%2)", name, region.countryCode());
                        hdays.append(pholiday);
                    } else {
                        // More than 1 region has the same holiday => remove the country code
                        // i.e don't show "Holiday (US)" and "Holiday(FR)"; just show "Holiday".
                        const QRegularExpression holidayRE(
                            i18nc("replace pattern for holidayname (countrycode)", "^%1 \\(.*\\)", name));
                        hdays.replaceInStrings(holidayRE, name);
                        hdays.removeDuplicates();
                    }
                } else {
                    if (!hdays.contains(name)) {
                        hdays.append(name);
                    }
                }
            }
        }
    }

    return hdays;
}

void CalendarSupport::saveAttachments(const Akonadi::Item &item, QWidget *parentWidget)
{
    Incidence::Ptr incidence = CalendarSupport::incidence(item);

    if (!incidence) {
        KMessageBox::sorry(
            parentWidget,
            i18n("No item selected."));
        return;
    }

    Attachment::List attachments = incidence->attachments();

    if (attachments.empty()) {
        return;
    }

    QString targetFile, targetDir;
    if (attachments.count() > 1) {
        // get the dir
        targetDir = QFileDialog::getExistingDirectory(parentWidget, i18n("Save Attachments To"));
        if (targetDir.isEmpty()) {
            return;
        }

        // we may not get a slash-terminated url out of KFileDialog
        if (!targetDir.endsWith(QLatin1Char('/'))) {
            targetDir.append(QLatin1Char('/'));
        }
    } else {
        // only one item, get the desired filename
        QString fileName = attachments.first()->label();
        if (fileName.isEmpty()) {
            fileName = i18nc("filename for an unnamed attachment", "attachment.1");
        }
        targetFile = QFileDialog::getSaveFileName(parentWidget, i18n("Save Attachment"), fileName);
        if (targetFile.isEmpty()) {
            return;
        }

        targetDir = QFileInfo(targetFile).absolutePath() + QLatin1Char('/');
    }

    for (const Attachment::Ptr &attachment : qAsConst(attachments)) {
        targetFile = targetDir + attachment->label();
        QUrl sourceUrl;
        if (attachment->isUri()) {
            sourceUrl = QUrl(attachment->uri());
        } else {
            sourceUrl = QUrl::fromLocalFile(incidence->writeAttachmentToTempFile(attachment));
        }
        // save the attachment url
        auto job = KIO::file_copy(sourceUrl, QUrl::fromLocalFile(targetFile));
        if (!job->exec() && job->error()) {
            KMessageBox::error(parentWidget, job->errorString());
        }
    }
}

QStringList CalendarSupport::categories(const KCalCore::Incidence::List &incidences)
{
    QStringList cats, thisCats;
    // @TODO: For now just iterate over all incidences. In the future,
    // the list of categories should be built when reading the file.
    for (const KCalCore::Incidence::Ptr &incidence : incidences) {
        thisCats = incidence->categories();
        const QStringList::ConstIterator send(thisCats.constEnd());
        for (QStringList::ConstIterator si = thisCats.constBegin();
             si != send; ++si) {
            if (!cats.contains(*si)) {
                cats.append(*si);
            }
        }
    }
    return cats;
}

bool CalendarSupport::mergeCalendar(const QString &srcFilename,
                                    const KCalCore::Calendar::Ptr &destCalendar)
{
    if (srcFilename.isEmpty()) {
        qCCritical(CALENDARSUPPORT_LOG) << "Empty filename.";
        return false;
    }

    if (!QFile::exists(srcFilename)) {
        qCCritical(CALENDARSUPPORT_LOG) << "File'" << srcFilename << "' doesn't exist.";
    }

    // merge in a file
    destCalendar->startBatchAdding();
    KCalCore::FileStorage storage(destCalendar);
    storage.setFileName(srcFilename);
    bool loadedSuccesfully = storage.load();
    destCalendar->endBatchAdding();

    return loadedSuccesfully;
}
