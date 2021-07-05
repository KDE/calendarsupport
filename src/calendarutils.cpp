/*
  SPDX-FileCopyrightText: 2010 Bertjan Broeksema <broeksema@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

// NOTE: The code of the following methods is taken from
//       kdepim/korganizer/calendarview.cpp:
//       - makeIndependent (was incidence_unsub)
//       - makeChildrenIndependent

#include "calendarutils.h"
#include "utils.h"

#include <Akonadi/Calendar/IncidenceChanger>
#include <KCalendarCore/Incidence>

#include "calendarsupport_debug.h"
#include <KLocalizedString>
#include <KMessageBox>

using namespace CalendarSupport;
using namespace KCalendarCore;

/// CalendarUtilsPrivate

struct MultiChange {
    Akonadi::Item parent;
    QVector<Akonadi::Item::Id> children;
    bool success = true;

    explicit MultiChange(const Akonadi::Item &parent = Akonadi::Item())
        : parent(parent)
    {
    }

    bool inProgress() const
    {
        return parent.isValid() && !children.isEmpty();
    }
};

namespace CalendarSupport
{
class CalendarUtilsPrivate
{
public:
    /// Methods
    CalendarUtilsPrivate(const Akonadi::ETMCalendar::Ptr &calendar, CalendarUtils *qq);
    void handleChangeFinish(int changeId, const Akonadi::Item &item, Akonadi::IncidenceChanger::ResultCode resultCode, const QString &errorString);

    bool purgeCompletedSubTodos(const KCalendarCore::Todo::Ptr &todo, bool &allPurged);

    /// Members
    Akonadi::ETMCalendar::Ptr mCalendar;
    Akonadi::IncidenceChanger *mChanger = nullptr;
    MultiChange mMultiChange;

private:
    CalendarUtils *const q_ptr;
    Q_DECLARE_PUBLIC(CalendarUtils)
};
}

CalendarUtilsPrivate::CalendarUtilsPrivate(const Akonadi::ETMCalendar::Ptr &calendar, CalendarUtils *qq)
    : mCalendar(calendar)
    , mChanger(new Akonadi::IncidenceChanger(qq))
    , q_ptr(qq)
{
    Q_Q(CalendarUtils);
    Q_ASSERT(mCalendar);

    q->connect(mChanger,
               SIGNAL(modifyFinished(int, Akonadi::Item, Akonadi::IncidenceChanger::ResultCode, QString)),
               SLOT(handleChangeFinish(int, Akonadi::Item, Akonadi::IncidenceChanger::ResultCode, QString)));
}

void CalendarUtilsPrivate::handleChangeFinish(int, const Akonadi::Item &item, Akonadi::IncidenceChanger::ResultCode resultCode, const QString &errorString)
{
    Q_Q(CalendarUtils);
    const bool success = resultCode == Akonadi::IncidenceChanger::ResultCodeSuccess;
    if (mMultiChange.inProgress()) {
        mMultiChange.children.remove(mMultiChange.children.indexOf(item.id()));
        mMultiChange.success = mMultiChange.success && success;

        // Are we still in progress?
        if (!mMultiChange.inProgress()) {
            const Akonadi::Item parent = mMultiChange.parent;
            const bool success = mMultiChange.success;

            // Reset the multi change.
            mMultiChange = MultiChange();
            Q_ASSERT(!mMultiChange.inProgress());

            if (success) {
                qCDebug(CALENDARSUPPORT_LOG) << "MultiChange finished";
                Q_EMIT q->actionFinished(parent);
            } else {
                qCDebug(CALENDARSUPPORT_LOG) << "MultiChange failed";
                Q_EMIT q->actionFailed(parent, QString());
            }
        }
    } else {
        if (success) {
            qCDebug(CALENDARSUPPORT_LOG) << "Change finished";
            Q_EMIT q->actionFinished(item);
        } else {
            qCDebug(CALENDARSUPPORT_LOG) << "Change failed";
            Q_EMIT q->actionFailed(Akonadi::Item(), errorString);
        }
    }
}

bool CalendarUtilsPrivate::purgeCompletedSubTodos(const KCalendarCore::Todo::Ptr &todo, bool &allPurged)
{
    if (!todo) {
        return true;
    }

    bool deleteThisTodo = true;
    const Akonadi::Item::List subTodos = mCalendar->childItems(todo->uid());
    for (const Akonadi::Item &item : subTodos) {
        if (CalendarSupport::hasTodo(item)) {
            deleteThisTodo &= purgeCompletedSubTodos(item.payload<KCalendarCore::Todo::Ptr>(), allPurged);
        }
    }

    if (deleteThisTodo) {
        if (todo->isCompleted()) {
            if (!mChanger->deleteIncidence(mCalendar->item(todo), nullptr)) {
                allPurged = false;
            }
        } else {
            deleteThisTodo = false;
        }
    } else {
        if (todo->isCompleted()) {
            allPurged = false;
        }
    }
    return deleteThisTodo;
}

/// CalendarUtils

CalendarUtils::CalendarUtils(const Akonadi::ETMCalendar::Ptr &calendar, QObject *parent)
    : QObject(parent)
    , d_ptr(new CalendarUtilsPrivate(calendar, this))
{
    Q_ASSERT(calendar);
}

CalendarUtils::~CalendarUtils()
{
    delete d_ptr;
}

Akonadi::ETMCalendar::Ptr CalendarUtils::calendar() const
{
    Q_D(const CalendarUtils);
    return d->mCalendar;
}

bool CalendarUtils::makeIndependent(const Akonadi::Item &item)
{
    Q_D(CalendarUtils);
    Q_ASSERT(item.isValid());

    if (d->mMultiChange.inProgress() && !d->mMultiChange.children.contains(item.id())) {
        return false;
    }

    const Incidence::Ptr inc = CalendarSupport::incidence(item);
    if (!inc || inc->relatedTo().isEmpty()) {
        return false;
    }

    Incidence::Ptr oldInc(inc->clone());
    inc->setRelatedTo(QString());
    return d->mChanger->modifyIncidence(item, oldInc);
}

bool CalendarUtils::makeChildrenIndependent(const Akonadi::Item &item)
{
    Q_D(CalendarUtils);
    Q_ASSERT(item.isValid());

    if (d->mMultiChange.inProgress()) {
        return false;
    }

    const Incidence::Ptr inc = CalendarSupport::incidence(item);
    const Akonadi::Item::List subIncs = d->mCalendar->childItems(item.id());

    if (!inc || subIncs.isEmpty()) {
        return false;
    }

    d->mMultiChange = MultiChange(item);
    bool allStarted = true;
    for (const Akonadi::Item &subInc : std::as_const(subIncs)) {
        d->mMultiChange.children.append(subInc.id());
        allStarted = allStarted && makeIndependent(subInc);
    }

    Q_ASSERT(allStarted); // OKay, maybe we should not assert here, but one or
    // changes could have been started, so just returning
    // false isn't suitable either.

    return true;
}

/// Todo specific methods.

void CalendarUtils::purgeCompletedTodos()
{
    Q_D(CalendarUtils);
    bool allDeleted = true;
    //  startMultiModify( i18n( "Purging completed to-dos" ) );
    KCalendarCore::Todo::List todos = calendar()->rawTodos();
    KCalendarCore::Todo::List rootTodos;

    for (const KCalendarCore::Todo::Ptr &todo : std::as_const(todos)) {
        if (todo && todo->relatedTo().isEmpty()) { // top level todo //REVIEW(AKONADI_PORT)
            rootTodos.append(todo);
        }
    }

    // now that we have a list of all root todos, check them and their children
    for (const KCalendarCore::Todo::Ptr &todo : std::as_const(rootTodos)) {
        d->purgeCompletedSubTodos(todo, allDeleted);
    }

    //  endMultiModify();
    if (!allDeleted) {
        KMessageBox::information(nullptr,
                                 i18nc("@info", "Unable to purge to-dos with uncompleted children."),
                                 i18nc("@title:window", "Delete To-do"),
                                 QStringLiteral("UncompletedChildrenPurgeTodos"));
    }
}

#include "moc_calendarutils.cpp"
