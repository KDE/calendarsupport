/*
  SPDX-FileCopyrightText: 2010 Bertjan Broeksema <broeksema@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/
#pragma once

#include <Akonadi/Calendar/ETMCalendar>
#include <Collection>

#include <QObject>

namespace Akonadi
{
class Item;
}

namespace CalendarSupport
{
class CalendarUtilsPrivate;

/** Some calendar/Incidence related utilitly methods.

    NOTE: this class will only start an modify job for an Item when no other job
          started by this class for the same Item is still running.
 */
class CalendarUtils : public QObject
{
    Q_OBJECT
public:
    /**
     * Creates a new CalendarUtils instance. The instance does not take owner ship
     * over the Calendar.
     */
    explicit CalendarUtils(const Akonadi::ETMCalendar::Ptr &calendar, QObject *parent = nullptr);

    ~CalendarUtils() override;

    /**
     * Returns the Caledar on which this utils class is operating.
     */
    Akonadi::ETMCalendar::Ptr calendar() const;

    /**
     * Makes the incidence from @param item independent from its parent. Returns
     * true when the ModifyJob to make the incidence independent was actually
     * started, false otherwise. This method is async, either actionFailed or
     * actionFinished will be emitted when the operation finished or failed.
     */
    bool makeIndependent(const Akonadi::Item &item);

    /**
     * Makes all children of the incindence from @param item independent
     * Returns true when one or more incidence(s) where made independent,
     * false otherwise.
     */
    bool makeChildrenIndependent(const Akonadi::Item &item);

    /** Todo specific methods ***************************************************/

    /**
     * Deletes the completed todos from all active collections in the Calendar.
     */
    void purgeCompletedTodos();

Q_SIGNALS:
    void actionFailed(const Akonadi::Item &item, const QString &msg);
    void actionFinished(const Akonadi::Item &item);

private:
    CalendarUtilsPrivate *const d_ptr;
    Q_DECLARE_PRIVATE(CalendarUtils)

    Q_PRIVATE_SLOT(
        d_ptr,
        void handleChangeFinish(int changeId, const Akonadi::Item &item, Akonadi::IncidenceChanger::ResultCode resultCode, const QString &errorString))
};
}

