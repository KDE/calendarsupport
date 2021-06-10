/*
  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company
  SPDX-FileContributor: Tobias Koenig <tokoe@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "calendarsupport_export.h"

#include <ItemMonitor>

#include <QDate>
#include <QWidget>

class QAbstractItemModel;

namespace Akonadi
{
class ETMCalendar;
}

namespace CalendarSupport
{
/**
 * @short A viewer component for incidences in Akonadi.
 *
 * This widgets provides a way to show an incidence from the
 * Akonadi storage.
 *
 * Example:
 *
 * @code
 *
 * using namespace CalendarSupport;
 *
 * const Item item = ...
 *
 * IncidenceViewer *viewer = new IncidenceViewer( this );
 * viewer->setIncidence( item );
 *
 * @endcode
 *
 * @author Tobias Koenig <tokoe@kde.org>
 * @since 4.5
 */
class CALENDARSUPPORT_EXPORT IncidenceViewer : public QWidget, public Akonadi::ItemMonitor
{
    Q_OBJECT

public:
    /**
     * Creates a new incidence viewer.
     *
     * *param
     * @param calendar is a pointer to a Calendar instance.
     * @param parent it the parent widget.
     */
    explicit IncidenceViewer(Akonadi::ETMCalendar *calendar, QWidget *parent = nullptr);

    /**
     * Creates a new incidence viewer.
     *
     * *param
     * @param parent it the parent widget.
     */
    explicit IncidenceViewer(QWidget *parent = nullptr);

    /**
     * Destroys the incidence viewer.
     */
    ~IncidenceViewer() override;

    /**
     * Sets the Calendar for this viewer.
     * @param calendar is a pointer to a Calendar instance.
     */
    void setCalendar(Akonadi::ETMCalendar *calendar);

    /**
     * Returns the incidence that is currently displayed.
     */
    Q_REQUIRED_RESULT Akonadi::Item incidence() const;

    /**
     * Returns the active date used for the currently displayed incidence
     */
    Q_REQUIRED_RESULT QDate activeDate() const;

    /**
     * Returns the attachment model for the currently displayed incidence.
     */
    QAbstractItemModel *attachmentModel() const;

    /**
     * Sets whether the view shall be cleared as soon as an empty incidence is
     * set (default) or @p delayed when the next valid incidence is set.
     */
    void setDelayedClear(bool delayed);

    /**
     * Sets the default @p message that shall be shown if no incidence is set.
     */
    void setDefaultMessage(const QString &message);

    /**
     * Sets an additional @p text that is shown above the incidence.
     */
    void setHeaderText(const QString &text);

public Q_SLOTS:
    /**
     * Sets the @p incidence that shall be displayed in the viewer.
     *
     * @param activeDate The active date is used to calculate the actual date of
     *                   the selected incidence in case of recurring incidences.
     */
    void setIncidence(const Akonadi::Item &incidence, QDate activeDate = QDate());

protected:
    /**
     * Initialize the widget settings.
     */
    void init();

private:
    /**
     * This method is called whenever the displayed incidence @p item has been changed.
     */
    void itemChanged(const Akonadi::Item &item) override;

    /**
     * This method is called whenever the displayed incidence has been
     * removed from Akonadi.
     */
    void itemRemoved() override;

private:
    //@cond PRIVATE
    class Private;
    Private *const d;

    Q_PRIVATE_SLOT(d, void slotParentCollectionFetched(KJob *))
    //@endcond
};
}

