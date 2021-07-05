/*
  SPDX-FileCopyrightText: 2000, 2001 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2004 David Faure <faure@kde.org>
  SPDX-FileCopyrightText: 2004 Reinhold Kainhofer <reinhold@kainhofer.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#include "eventarchiver.h"

#include "kcalprefs.h"
#include "utils.h"
#include <Akonadi/Calendar/IncidenceChanger>

#include <KCalendarCore/FileStorage>
#include <KCalendarCore/ICalFormat>
#include <KCalendarCore/MemoryCalendar>

#include <KCalUtils/Stringify>

#include "calendarsupport_debug.h"
#include <KIO/FileCopyJob>
#include <KIO/StatJob>
#include <KJobWidgets>
#include <KLocalizedString>
#include <KMessageBox>

#include <QLocale>
#include <QTemporaryFile>
#include <QTimeZone>

using namespace KCalendarCore;
using namespace KCalUtils;
using namespace CalendarSupport;

class GroupwareScoppedDisabler
{
public:
    GroupwareScoppedDisabler(Akonadi::IncidenceChanger *changer)
        : m_changer(changer)
    {
        m_wasEnabled = m_changer->groupwareCommunication();
        m_changer->setGroupwareCommunication(false);
    }

    ~GroupwareScoppedDisabler()
    {
        m_changer->setGroupwareCommunication(m_wasEnabled);
    }

    bool m_wasEnabled = false;
    Akonadi::IncidenceChanger *const m_changer;
};

EventArchiver::EventArchiver(QObject *parent)
    : QObject(parent)
{
}

EventArchiver::~EventArchiver()
{
}

void EventArchiver::runOnce(const Akonadi::ETMCalendar::Ptr &calendar, Akonadi::IncidenceChanger *changer, QDate limitDate, QWidget *widget)
{
    run(calendar, changer, limitDate, widget, true, true);
}

void EventArchiver::runAuto(const Akonadi::ETMCalendar::Ptr &calendar, Akonadi::IncidenceChanger *changer, QWidget *widget, bool withGUI)
{
    QDate limitDate(QDate::currentDate());
    const int expiryTime = KCalPrefs::instance()->mExpiryTime;
    switch (KCalPrefs::instance()->mExpiryUnit) {
    case KCalPrefs::UnitDays: // Days
        limitDate = limitDate.addDays(-expiryTime);
        break;
    case KCalPrefs::UnitWeeks: // Weeks
        limitDate = limitDate.addDays(-expiryTime * 7);
        break;
    case KCalPrefs::UnitMonths: // Months
        limitDate = limitDate.addMonths(-expiryTime);
        break;
    default:
        return;
    }
    run(calendar, changer, limitDate, widget, withGUI, false);
}

void EventArchiver::run(const Akonadi::ETMCalendar::Ptr &calendar,
                        Akonadi::IncidenceChanger *changer,
                        QDate limitDate,
                        QWidget *widget,
                        bool withGUI,
                        bool errorIfNone)
{
    GroupwareScoppedDisabler disabler(changer); // Disables groupware communication temporarily

    // We need to use rawEvents, otherwise events hidden by filters will not be archived.
    KCalendarCore::Event::List events;
    KCalendarCore::Todo::List todos;
    KCalendarCore::Journal::List journals;

    if (KCalPrefs::instance()->mArchiveEvents) {
        events = calendar->rawEvents(QDate(1769, 12, 1),
                                     // #29555, also advertised by the "limitDate not included" in the class docu
                                     limitDate.addDays(-1),
                                     QTimeZone::systemTimeZone(),
                                     true);
    }
    if (KCalPrefs::instance()->mArchiveTodos) {
        const KCalendarCore::Todo::List rawTodos = calendar->rawTodos();

        for (const KCalendarCore::Todo::Ptr &todo : rawTodos) {
            Q_ASSERT(todo);
            if (isSubTreeComplete(calendar, todo, limitDate)) {
                todos.append(todo);
            }
        }
    }

    const KCalendarCore::Incidence::List incidences = calendar->mergeIncidenceList(events, todos, journals);

    qCDebug(CALENDARSUPPORT_LOG) << "archiving incidences before" << limitDate << " ->" << incidences.count() << " incidences found.";
    if (incidences.isEmpty()) {
        if (withGUI && errorIfNone) {
            KMessageBox::information(widget,
                                     i18n("There are no items before %1", QLocale::system().toString(limitDate, QLocale::ShortFormat)),
                                     QStringLiteral("ArchiverNoIncidences"));
        }
        return;
    }

    switch (KCalPrefs::instance()->mArchiveAction) {
    case KCalPrefs::actionDelete:
        deleteIncidences(changer, limitDate, widget, calendar->itemList(incidences), withGUI);
        break;
    case KCalPrefs::actionArchive:
        archiveIncidences(calendar, changer, limitDate, widget, incidences, withGUI);
        break;
    }
}

void EventArchiver::deleteIncidences(Akonadi::IncidenceChanger *changer, QDate limitDate, QWidget *widget, const Akonadi::Item::List &items, bool withGUI)
{
    QStringList incidenceStrs;
    Akonadi::Item::List::ConstIterator it;
    Akonadi::Item::List::ConstIterator end(items.constEnd());
    incidenceStrs.reserve(items.count());
    for (it = items.constBegin(); it != end; ++it) {
        incidenceStrs.append(CalendarSupport::incidence(*it)->summary());
    }

    if (withGUI) {
        const int result = KMessageBox::warningContinueCancelList(widget,
                                                                  i18n("Delete all items before %1 without saving?\n"
                                                                       "The following items will be deleted:",
                                                                       QLocale::system().toString(limitDate, QLocale::ShortFormat)),
                                                                  incidenceStrs,
                                                                  i18n("Delete Old Items"),
                                                                  KStandardGuiItem::del());
        if (result != KMessageBox::Continue) {
            return;
        }
    }

    changer->deleteIncidences(items, /**parent=*/widget);

    // TODO: Q_EMIT only after hearing back from incidence changer
    Q_EMIT eventsDeleted();
}

void EventArchiver::archiveIncidences(const Akonadi::ETMCalendar::Ptr &calendar,
                                      Akonadi::IncidenceChanger *changer,
                                      QDate limitDate,
                                      QWidget *widget,
                                      const KCalendarCore::Incidence::List &incidences,
                                      bool withGUI)
{
    Q_UNUSED(limitDate)
    Q_UNUSED(withGUI)

    FileStorage storage(calendar);

    QString tmpFileName;
    // KSaveFile cannot be called with an open File Handle on Windows.
    // So we use QTemporaryFile only to generate a unique filename
    // and then close/delete the file again. This file must be deleted
    // here.
    {
        QTemporaryFile tmpFile;
        tmpFile.open();
        tmpFileName = tmpFile.fileName();
    }
    // Save current calendar to disk
    storage.setFileName(tmpFileName);
    if (!storage.save()) {
        qCDebug(CALENDARSUPPORT_LOG) << "Can't save calendar to temp file";
        return;
    }

    // Duplicate current calendar by loading in new calendar object
    MemoryCalendar::Ptr archiveCalendar(new MemoryCalendar(QTimeZone::systemTimeZone()));

    FileStorage archiveStore(archiveCalendar);
    archiveStore.setFileName(tmpFileName);
    auto format = new ICalFormat();
    archiveStore.setSaveFormat(format);
    if (!archiveStore.load()) {
        qCDebug(CALENDARSUPPORT_LOG) << "Can't load calendar from temp file";
        QFile::remove(tmpFileName);
        return;
    }

    // Strip active events from calendar so that only events to be archived
    // remain. This is not really efficient, but there is no other easy way.
    QStringList uids;
    Incidence::List allIncidences = archiveCalendar->rawIncidences();
    uids.reserve(incidences.count());
    for (const KCalendarCore::Incidence::Ptr &incidence : std::as_const(incidences)) {
        uids.append(incidence->uid());
    }
    for (const KCalendarCore::Incidence::Ptr &incidence : std::as_const(allIncidences)) {
        if (!uids.contains(incidence->uid())) {
            archiveCalendar->deleteIncidence(incidence);
        }
    }

    // Get or create the archive file
    QUrl archiveURL(KCalPrefs::instance()->mArchiveFile);
    QString archiveFile;
    QTemporaryFile downloadTempFile;

    bool fileExists = false;
    if (archiveURL.isLocalFile()) {
        fileExists = QFile::exists(archiveURL.toLocalFile());
    } else {
        auto job = KIO::statDetails(archiveURL, KIO::StatJob::SourceSide, KIO::StatBasic);

        KJobWidgets::setWindow(job, widget);
        fileExists = job->exec();
    }

    if (fileExists) {
        archiveFile = downloadTempFile.fileName();
        auto job = KIO::file_copy(archiveURL, QUrl::fromLocalFile(archiveFile));
        KJobWidgets::setWindow(job, widget);
        if (!job->exec()) {
            qCDebug(CALENDARSUPPORT_LOG) << "Can't download archive file";
            QFile::remove(tmpFileName);
            return;
        }
        // Merge with events to be archived.
        archiveStore.setFileName(archiveFile);
        if (!archiveStore.load()) {
            qCDebug(CALENDARSUPPORT_LOG) << "Can't merge with archive file";
            QFile::remove(tmpFileName);
            return;
        }
    } else {
        archiveFile = tmpFileName;
    }

    // Save archive calendar
    if (!archiveStore.save()) {
        QString errmess;
        if (format->exception()) {
            errmess = Stringify::errorMessage(*format->exception());
        } else {
            errmess = i18nc("save failure cause unknown", "Reason unknown");
        }
        KMessageBox::error(widget, i18n("Cannot write archive file %1. %2", archiveStore.fileName(), errmess));
        QFile::remove(tmpFileName);
        return;
    }

    // Upload if necessary
    QUrl srcUrl = QUrl::fromLocalFile(archiveFile);
    if (srcUrl != archiveURL) {
        auto job = KIO::file_copy(QUrl::fromLocalFile(archiveFile), archiveURL);
        KJobWidgets::setWindow(job, widget);
        if (!job->exec()) {
            KMessageBox::error(widget, i18n("Cannot write archive. %1", job->errorString()));
            QFile::remove(tmpFileName);
            return;
        }
    }

    QFile::remove(tmpFileName);

    // We don't want it to ask to send invitations for each incidence.
    changer->startAtomicOperation(i18n("Archiving events"));

    // Delete archived events from calendar
    const Akonadi::Item::List items = calendar->itemList(incidences);
    for (const Akonadi::Item &item : items) {
        changer->deleteIncidence(item, widget);
    } // TODO: Q_EMIT only after hearing back from incidence changer
    changer->endAtomicOperation();

    Q_EMIT eventsDeleted();
}

bool EventArchiver::isSubTreeComplete(const Akonadi::ETMCalendar::Ptr &calendar, const Todo::Ptr &todo, QDate limitDate, QStringList checkedUids) const
{
    if (!todo->isCompleted() || todo->completed().date() >= limitDate) {
        return false;
    }

    // This QList is only to prevent infinit recursion
    if (checkedUids.contains(todo->uid())) {
        // Probably will never happen, calendar.cpp checks for this
        qCWarning(CALENDARSUPPORT_LOG) << "To-do hierarchy loop detected!";
        return false;
    }

    checkedUids.append(todo->uid());
    const KCalendarCore::Incidence::List childs = calendar->childIncidences(todo->uid());
    for (const KCalendarCore::Incidence::Ptr &incidence : childs) {
        const Todo::Ptr t = incidence.dynamicCast<KCalendarCore::Todo>();
        if (t && !isSubTreeComplete(calendar, t, limitDate, checkedUids)) {
            return false;
        }
    }

    return true;
}
