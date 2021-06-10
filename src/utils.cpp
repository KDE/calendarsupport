/*
  SPDX-FileCopyrightText: 2009, 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  SPDX-FileContributor: Frank Osterfeld <osterfeld@kde.org>
  SPDX-FileContributor: Andras Mantia <andras@kdab.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#include "utils.h"
#include "calendarsupport_debug.h"
#include "kcalprefs.h"

#include <AkonadiCore/AgentInstance>
#include <AkonadiCore/AgentManager>
#include <AkonadiCore/EntityDisplayAttribute>
#include <AkonadiCore/EntityTreeModel>

#include <AkonadiWidgets/CollectionDialog>

#include <Akonadi/Calendar/BlockAlarmsAttribute>
#include <Akonadi/Calendar/ETMCalendar>

#include <KHolidays/HolidayRegion>

#include <KCalendarCore/CalFilter>
#include <KCalendarCore/FileStorage>
#include <KCalendarCore/FreeBusy>
#include <KCalendarCore/MemoryCalendar>

#include <KCalUtils/DndFactory>
#include <KCalUtils/ICalDrag>
#include <KCalUtils/VCalDrag>

#include <KLocalizedString>

#include <QApplication>
#include <QDrag>
#include <QFile>
#include <QMimeData>
#include <QPointer>
#include <QStyle>
#include <QUrlQuery>

KCalendarCore::Incidence::Ptr CalendarSupport::incidence(const Akonadi::Item &item)
{
    // relying on exception for performance reasons
    try {
        return item.payload<KCalendarCore::Incidence::Ptr>();
    } catch (const Akonadi::PayloadException &) {
        return KCalendarCore::Incidence::Ptr();
    }
}

KCalendarCore::Event::Ptr CalendarSupport::event(const Akonadi::Item &item)
{
    // relying on exception for performance reasons
    try {
        auto incidence = item.payload<KCalendarCore::Incidence::Ptr>();
        if (hasEvent(incidence)) {
            return item.payload<KCalendarCore::Event::Ptr>();
        }
    } catch (const Akonadi::PayloadException &) {
        return KCalendarCore::Event::Ptr();
    }
    return KCalendarCore::Event::Ptr();
}

KCalendarCore::Event::Ptr CalendarSupport::event(const KCalendarCore::Incidence::Ptr &incidence)
{
    if (hasEvent(incidence)) {
        return incidence.staticCast<KCalendarCore::Event>();
    }
    return KCalendarCore::Event::Ptr();
}

KCalendarCore::Incidence::List CalendarSupport::incidencesFromItems(const Akonadi::Item::List &items)
{
    KCalendarCore::Incidence::List incidences;
    for (const Akonadi::Item &item : items) {
        if (const KCalendarCore::Incidence::Ptr e = CalendarSupport::incidence(item)) {
            incidences.push_back(e);
        }
    }
    return incidences;
}

KCalendarCore::Todo::Ptr CalendarSupport::todo(const Akonadi::Item &item)
{
    try {
        auto incidence = item.payload<KCalendarCore::Incidence::Ptr>();
        if (hasTodo(incidence)) {
            return item.payload<KCalendarCore::Todo::Ptr>();
        }
    } catch (const Akonadi::PayloadException &) {
        return KCalendarCore::Todo::Ptr();
    }
    return KCalendarCore::Todo::Ptr();
}

KCalendarCore::Todo::Ptr CalendarSupport::todo(const KCalendarCore::Incidence::Ptr &incidence)
{
    if (hasTodo(incidence)) {
        return incidence.staticCast<KCalendarCore::Todo>();
    }
    return KCalendarCore::Todo::Ptr();
}

KCalendarCore::Journal::Ptr CalendarSupport::journal(const Akonadi::Item &item)
{
    try {
        auto incidence = item.payload<KCalendarCore::Incidence::Ptr>();
        if (hasJournal(incidence)) {
            return item.payload<KCalendarCore::Journal::Ptr>();
        }
    } catch (const Akonadi::PayloadException &) {
        return KCalendarCore::Journal::Ptr();
    }
    return KCalendarCore::Journal::Ptr();
}

KCalendarCore::Journal::Ptr CalendarSupport::journal(const KCalendarCore::Incidence::Ptr &incidence)
{
    if (hasJournal(incidence)) {
        return incidence.staticCast<KCalendarCore::Journal>();
    }
    return KCalendarCore::Journal::Ptr();
}

bool CalendarSupport::hasIncidence(const Akonadi::Item &item)
{
    return item.hasPayload<KCalendarCore::Incidence::Ptr>();
}

bool CalendarSupport::hasEvent(const Akonadi::Item &item)
{
    return item.hasPayload<KCalendarCore::Event::Ptr>();
}

bool CalendarSupport::hasEvent(const KCalendarCore::Incidence::Ptr &incidence)
{
    return incidence && incidence->type() == KCalendarCore::Incidence::TypeEvent;
}

bool CalendarSupport::hasTodo(const Akonadi::Item &item)
{
    return item.hasPayload<KCalendarCore::Todo::Ptr>();
}

bool CalendarSupport::hasTodo(const KCalendarCore::Incidence::Ptr &incidence)
{
    return incidence && incidence->type() == KCalendarCore::Incidence::TypeTodo;
}

bool CalendarSupport::hasJournal(const Akonadi::Item &item)
{
    return item.hasPayload<KCalendarCore::Journal::Ptr>();
}

bool CalendarSupport::hasJournal(const KCalendarCore::Incidence::Ptr &incidence)
{
    return incidence && incidence->type() == KCalendarCore::Incidence::TypeJournal;
}

QMimeData *CalendarSupport::createMimeData(const Akonadi::Item::List &items)
{
    if (items.isEmpty()) {
        return nullptr;
    }

    KCalendarCore::MemoryCalendar::Ptr cal(new KCalendarCore::MemoryCalendar(QTimeZone::systemTimeZone()));

    QList<QUrl> urls;
    int incidencesFound = 0;
    for (const Akonadi::Item &item : items) {
        const KCalendarCore::Incidence::Ptr incidence(CalendarSupport::incidence(item));
        if (!incidence) {
            continue;
        }
        ++incidencesFound;
        urls.push_back(item.url());
        KCalendarCore::Incidence::Ptr i(incidence->clone());
        cal->addIncidence(i);
    }

    if (incidencesFound == 0) {
        return nullptr;
    }

    std::unique_ptr<QMimeData> mimeData(new QMimeData);

    mimeData->setUrls(urls);

    if (KCalUtils::ICalDrag::populateMimeData(mimeData.get(), cal)) {
        return mimeData.release();
    } else {
        return nullptr;
    }
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
        drag->setPixmap(QIcon::fromTheme(QStringLiteral("view-calendar-day")).pixmap(qApp->style()->pixelMetric(QStyle::PM_ToolBarIconSize)));
    } else if (common == "Todo") {
        drag->setPixmap(QIcon::fromTheme(QStringLiteral("view-calendar-tasks")).pixmap(qApp->style()->pixelMetric(QStyle::PM_ToolBarIconSize)));
    }

    return drag.release();
}

#endif

static bool itemMatches(const Akonadi::Item &item, const KCalendarCore::CalFilter *filter)
{
    assert(filter);
    KCalendarCore::Incidence::Ptr inc = CalendarSupport::incidence(item);
    if (!inc) {
        return false;
    }
    return filter->filterIncidence(inc);
}

Akonadi::Item::List CalendarSupport::applyCalFilter(const Akonadi::Item::List &items_, const KCalendarCore::CalFilter *filter)
{
    Q_ASSERT(filter);
    Akonadi::Item::List items(items_);
    items.erase(std::remove_if(items.begin(),
                               items.end(),
                               [filter](const Akonadi::Item &item) {
                                   return !itemMatches(item, filter);
                               }),
                items.end());
    return items;
}

bool CalendarSupport::isValidIncidenceItemUrl(const QUrl &url, const QStringList &supportedMimeTypes)
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
                                   QStringList() << KCalendarCore::Event::eventMimeType() << KCalendarCore::Todo::todoMimeType()
                                                 << KCalendarCore::Journal::journalMimeType() << KCalendarCore::FreeBusy::freeBusyMimeType());
}

static bool containsValidIncidenceItemUrl(const QList<QUrl> &urls)
{
    return std::find_if(urls.begin(),
                        urls.end(),
                        [](const QUrl &url) {
                            return CalendarSupport::isValidIncidenceItemUrl(url);
                        })
        != urls.constEnd();
}

bool CalendarSupport::canDecode(const QMimeData *md)
{
    if (md) {
        return containsValidIncidenceItemUrl(md->urls()) || KCalUtils::ICalDrag::canDecode(md) || KCalUtils::VCalDrag::canDecode(md);
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
        if (isValidIncidenceItemUrl(i, QStringList() << KCalendarCore::Todo::todoMimeType())) {
            urls.push_back(i);
        }
    }
    return urls;
}

bool CalendarSupport::mimeDataHasIncidence(const QMimeData *mimeData)
{
    return !incidenceItemUrls(mimeData).isEmpty() || !incidences(mimeData).isEmpty();
}

KCalendarCore::Todo::List CalendarSupport::todos(const QMimeData *mimeData)
{
    KCalendarCore::Todo::List todos;

#ifndef QT_NO_DRAGANDDROP
    KCalendarCore::Calendar::Ptr cal(KCalUtils::DndFactory::createDropCalendar(mimeData));
    if (cal) {
        const KCalendarCore::Todo::List calTodos = cal->todos();
        todos.reserve(calTodos.count());
        for (const KCalendarCore::Todo::Ptr &i : calTodos) {
            todos.push_back(KCalendarCore::Todo::Ptr(i->clone()));
        }
    }
#endif

    return todos;
}

KCalendarCore::Incidence::List CalendarSupport::incidences(const QMimeData *mimeData)
{
    KCalendarCore::Incidence::List incidences;

#ifndef QT_NO_DRAGANDDROP
    KCalendarCore::Calendar::Ptr cal(KCalUtils::DndFactory::createDropCalendar(mimeData));
    if (cal) {
        const KCalendarCore::Incidence::List calIncidences = cal->incidences();
        incidences.reserve(calIncidences.count());
        for (const KCalendarCore::Incidence::Ptr &i : calIncidences) {
            incidences.push_back(KCalendarCore::Incidence::Ptr(i->clone()));
        }
    }
#endif

    return incidences;
}

Akonadi::Collection CalendarSupport::selectCollection(QWidget *parent, int &dialogCode, const QStringList &mimeTypes, const Akonadi::Collection &defCollection)
{
    QPointer<Akonadi::CollectionDialog> dlg(new Akonadi::CollectionDialog(parent));
    dlg->setWindowTitle(i18nc("@title:window", "Select Calendar"));
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
    auto item = idx.data(Akonadi::EntityTreeModel::ItemRole).value<Akonadi::Item>();
    item.setParentCollection(idx.data(Akonadi::EntityTreeModel::ParentCollectionRole).value<Akonadi::Collection>());
    return item;
}

Akonadi::Collection::List CalendarSupport::collectionsFromModel(const QAbstractItemModel *model, const QModelIndex &parentIndex, int start, int end)
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

Akonadi::Item::List CalendarSupport::itemsFromModel(const QAbstractItemModel *model, const QModelIndex &parentIndex, int start, int end)
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
    if (resourceName.contains(QLatin1String("kolab"))) {
        QString typeStr = cName; // contents type: "Calendar", "Tasks", etc
        QString ownerStr; // folder owner: "fred", "ethel", etc
        QString nameStr; // folder name: "Public", "Test", etc
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
                } else if (tName.startsWith(QLatin1String("shared.tasks"), Qt::CaseInsensitive)
                           || tName.startsWith(QLatin1String("shared.todo"), Qt::CaseInsensitive)) {
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
                } else if (tName != i18n("Calendar") && tName != i18n("Tasks") && tName != i18n("Journal") && tName != i18n("Notes")) {
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
                return i18nc("%1 is folder contents", "My Kolab %1", typeStr);
            } else if (!ownerStr.compare(QLatin1String("SHARED"), Qt::CaseInsensitive) || !ownerStr.compare(QLatin1String("CALENDAR"), Qt::CaseInsensitive)
                       || !ownerStr.compare(QLatin1String("RESOURCES"), Qt::CaseInsensitive)) {
                return i18nc("%1 is folder name, %2 is folder contents", "Shared Kolab %1 %2", nameStr, typeStr);
            } else {
                if (nameStr.isEmpty()) {
                    return i18nc("%1 is folder owner name, %2 is folder contents", "%1's Kolab %2", ownerStr, typeStr);
                } else {
                    return i18nc("%1 is folder owner name, %2 is folder name, %3 is folder contents", "%1's %2 Kolab %3", ownerStr, nameStr, typeStr);
                }
            }
        } else {
            return i18nc("%1 is folder contents", "Kolab %1", typeStr);
        }
    } // end kolab section

    // Dav Groupware
    if (resourceName.contains(QLatin1String("davgroupware"))) {
        const QString resourceDisplayName = Akonadi::AgentManager::self()->instance(resourceName).name();
        return i18nc("%1 is the folder name", "%1 in %2", fullCollection.displayName(), resourceDisplayName);
    } // end caldav section

    // Google
    if (resourceName.contains(QLatin1String("google"))) {
        QString ownerStr; // folder owner: "user@gmail.com"
        if (calendar) {
            Akonadi::Collection p = c.parentCollection();
            ownerStr = calendar->collection(p.id()).displayName();
        }

        const QString nameStr = c.displayName(); // folder name: can be anything

        QString typeStr;
        const QString mimeStr = c.contentMimeTypes().join(QLatin1Char(','));
        if (mimeStr.contains(QLatin1String(".event"))) {
            typeStr = i18n("Calendar");
        } else if (mimeStr.contains(QLatin1String(".todo"))) {
            typeStr = i18n("Tasks");
        } else if (mimeStr.contains(QLatin1String(".journal"))) {
            typeStr = i18n("Journal");
        } else if (mimeStr.contains(QLatin1String(".note"))) {
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
                return i18nc("%1 is folder owner name, %2 is folder contents", "%1's Google %2", ownerStr, typeStr);
            } else {
                return i18nc("%1 is folder owner name, %2 is folder name", "%1's %2", ownerStr, nameStr);
            }
        } else {
            return i18nc("%1 is folder contents", "Google %1", typeStr);
        }
    } // end google section

    // Not groupware so the collection is "mine"
    const QString dName = fullCollection.displayName();

    if (!dName.isEmpty()) {
        return fullCollection.name().startsWith(QLatin1String("akonadi_")) ? i18n("My %1", dName) : dName;
    } else if (!fullCollection.name().isEmpty()) {
        return fullCollection.name();
    } else {
        return i18nc("unknown resource", "Unknown");
    }
}

QString CalendarSupport::toolTipString(const Akonadi::Collection &coll, bool richText)
{
    Q_UNUSED(richText)

    QString str = QStringLiteral("<qt>");

    // Display Name
    QString displayName;
    if (coll.hasAttribute<Akonadi::EntityDisplayAttribute>()) {
        displayName = coll.attribute<Akonadi::EntityDisplayAttribute>()->displayName();
    }

    if (displayName.isEmpty()) {
        displayName = coll.name();
    }
    if (coll.id() == CalendarSupport::KCalPrefs::instance()->defaultCalendarId()) {
        displayName = i18nc("this is the default calendar", "%1 (Default Calendar)", displayName);
    }
    str += QLatin1String("<b>") + displayName + QLatin1String("</b>");
    str += QLatin1String("<hr>");

    // Calendar Type
    QString calendarType;
    if (!coll.isVirtual()) {
        const Akonadi::AgentInstance instance = Akonadi::AgentManager::self()->instance(coll.resource());
        calendarType = instance.type().name();
    } else {
        calendarType = i18nc("a virtual folder type", "Virtual");
    }
    str += QLatin1String("<i>") + i18n("Folder type:") + QLatin1String("</i>");
    str += QLatin1String("&nbsp;") + calendarType;

    // Content Type
    QStringList mimeTypes = coll.contentMimeTypes();
    mimeTypes.removeAll(QStringLiteral("inode/directory"));
    QString mimeTypeStr;
    if (!mimeTypes.isEmpty()) {
        mimeTypeStr = QLocale().createSeparatedList(mimeTypes.replaceInStrings(QStringLiteral("application/x-vnd.akonadi.calendar."), QString()));
    } else {
        mimeTypeStr = i18nc("collection has no mimetypes to show the user", "none");
    }
    str += QLatin1String("<br>");
    str += QLatin1String("<i>") + i18n("Content type:") + QLatin1String("</i>");
    str += QLatin1String("&nbsp;") + mimeTypeStr;
    str += QLatin1String("</br>");

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

    // Blocking reminders?
    QStringList blockList;
    if (coll.hasAttribute<Akonadi::BlockAlarmsAttribute>()) {
        if (coll.attribute<Akonadi::BlockAlarmsAttribute>()->isEverythingBlocked()) {
            blockList << i18nc("blocking all reminders for this calendar", "all");
        } else {
            if (coll.attribute<Akonadi::BlockAlarmsAttribute>()->isAlarmTypeBlocked(KCalendarCore::Alarm::Audio)) {
                blockList << i18nc("blocking audio reminders for this calendar", "audio");
            } else if (coll.attribute<Akonadi::BlockAlarmsAttribute>()->isAlarmTypeBlocked(KCalendarCore::Alarm::Display)) {
                blockList << i18nc("blocking display pop-up dialog reminders for this calendar", "display");
            } else if (coll.attribute<Akonadi::BlockAlarmsAttribute>()->isAlarmTypeBlocked(KCalendarCore::Alarm::Email)) {
                blockList << i18nc("blocking email reminders for this calendar", "email");
            } else if (coll.attribute<Akonadi::BlockAlarmsAttribute>()->isAlarmTypeBlocked(KCalendarCore::Alarm::Procedure)) {
                blockList << i18nc("blocking run a command reminders for this calendar", "procedure");
            } else {
                blockList << i18nc("blocking unknown type reminders for this calendar", "other");
            }
        }
    } else {
        blockList << i18nc("not blocking any reminder types for this calendar", "none");
    }
    str += QLatin1String("<br>");
    str += QLatin1String("<i>") + i18n("Blocked Reminders:") + QLatin1String("</i>");
    str += QLatin1String("&nbsp;");
    str += QLocale().createSeparatedList(blockList);
    str += QLatin1String("</br>");

    str += QLatin1String("</qt>");
    return str;
}

QString CalendarSupport::subMimeTypeForIncidence(const KCalendarCore::Incidence::Ptr &incidence)
{
    return incidence->mimeType();
}

QList<QDate> CalendarSupport::workDays(QDate startDate, QDate endDate)
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
                    const KHolidays::Holiday &h = list.at(i);
                    if (h.dayType() == KHolidays::Holiday::NonWorkday) {
                        result.removeAll(h.observedStartDate());
                    }
                }
            }
        }
    }

    return result;
}

QStringList CalendarSupport::holiday(QDate date)
{
    QStringList hdays;

    bool showCountryCode = (KCalPrefs::instance()->mHolidays.count() > 1);
    const QStringList holidays = KCalPrefs::instance()->mHolidays;
    for (const QString &regionStr : holidays) {
        KHolidays::HolidayRegion region(regionStr);
        if (region.isValid()) {
            const KHolidays::Holiday::List list = region.holidays(date);
            const int listCount = list.count();
            for (int i = 0; i < listCount; ++i) {
                // don't add duplicates.
                // TODO: won't find duplicates in different languages however.
                const QString name = list.at(i).name();
                if (showCountryCode) {
                    // If more than one holiday region, append the country code to the holiday
                    // display name to help the user identify which region it belongs to.
                    const QRegularExpression holidaySE(i18nc("search pattern for holidayname", "^%1", name));
                    if (hdays.filter(holidaySE).isEmpty()) {
                        const QString pholiday = i18n("%1 (%2)", name, region.countryCode());
                        hdays.append(pholiday);
                    } else {
                        // More than 1 region has the same holiday => remove the country code
                        // i.e don't show "Holiday (US)" and "Holiday(FR)"; just show "Holiday".
                        const QRegularExpression holidayRE(i18nc("replace pattern for holidayname (countrycode)", "^%1 \\(.*\\)", name));
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

QStringList CalendarSupport::categories(const KCalendarCore::Incidence::List &incidences)
{
    QStringList cats, thisCats;
    // @TODO: For now just iterate over all incidences. In the future,
    // the list of categories should be built when reading the file.
    for (const KCalendarCore::Incidence::Ptr &incidence : incidences) {
        thisCats = incidence->categories();
        const QStringList::ConstIterator send(thisCats.constEnd());
        for (QStringList::ConstIterator si = thisCats.constBegin(); si != send; ++si) {
            if (!cats.contains(*si)) {
                cats.append(*si);
            }
        }
    }
    return cats;
}

bool CalendarSupport::mergeCalendar(const QString &srcFilename, const KCalendarCore::Calendar::Ptr &destCalendar)
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
    KCalendarCore::FileStorage storage(destCalendar);
    storage.setFileName(srcFilename);
    bool loadedSuccesfully = storage.load();
    destCalendar->endBatchAdding();

    return loadedSuccesfully;
}

void CalendarSupport::createAlarmReminder(const KCalendarCore::Alarm::Ptr &alarm, KCalendarCore::IncidenceBase::IncidenceType type)
{
    int duration; // in secs
    switch (CalendarSupport::KCalPrefs::instance()->mReminderTimeUnits) {
    default:
    case 0: // mins
        duration = CalendarSupport::KCalPrefs::instance()->mReminderTime * 60;
        break;
    case 1: // hours
        duration = CalendarSupport::KCalPrefs::instance()->mReminderTime * 60 * 60;
        break;
    case 2: // days
        duration = CalendarSupport::KCalPrefs::instance()->mReminderTime * 60 * 60 * 24;
        break;
    }
    alarm->setType(KCalendarCore::Alarm::Display);
    alarm->setEnabled(true);
    if (type == KCalendarCore::Incidence::TypeEvent) {
        alarm->setStartOffset(KCalendarCore::Duration(-duration));
    } else {
        alarm->setEndOffset(KCalendarCore::Duration(-duration));
    }
}
