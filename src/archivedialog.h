/*
  SPDX-FileCopyrightText: 2000, 2001 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/
#pragma once

#include "calendarsupport_export.h"

#include <Akonadi/ETMCalendar>
#include <QDialog>

class QComboBox;
class KDateComboBox;
class QSpinBox;
class KUrlRequester;

class QCheckBox;
class QRadioButton;
class QPushButton;

namespace Akonadi
{
class IncidenceChanger;
class ETMCalendar;
}

namespace CalendarSupport
{
/*!
 * \class CalendarSupport::ArchiveDialog
 * \inmodule CalendarSupport
 * \inheaderfile CalendarSupport/ArchiveDialog
 */
class CALENDARSUPPORT_EXPORT ArchiveDialog : public QDialog
{
    Q_OBJECT
public:
    /*!
     * Constructs an ArchiveDialog.
     * \param calendar The ETM calendar to archive from.
     * \param changer The incidence changer used to delete events.
     * \param parent The parent widget.
     */
    ArchiveDialog(const Akonadi::ETMCalendar::Ptr &calendar, Akonadi::IncidenceChanger *changer, QWidget *parent = nullptr);
    /*!
     * Destroys the ArchiveDialog.
     */
    ~ArchiveDialog() override;

Q_SIGNALS:
    // connected by KODialogManager to CalendarView
    /*!
     * Emitted when events have been deleted during archival.
     */
    void eventsDeleted();
    /*!
     * Emitted when the auto-archiving settings have been modified.
     */
    void autoArchivingSettingsModified();

private:
    CALENDARSUPPORT_NO_EXPORT void slotEventsDeleted();
    CALENDARSUPPORT_NO_EXPORT void slotUser1();
    CALENDARSUPPORT_NO_EXPORT void slotEnableUser1();
    CALENDARSUPPORT_NO_EXPORT void slotActionChanged();
    CALENDARSUPPORT_NO_EXPORT void showWhatsThis();
    KUrlRequester *mArchiveFile = nullptr;
    KDateComboBox *mDateEdit = nullptr;
    QCheckBox *mDeleteCb = nullptr;
    QRadioButton *mArchiveOnceRB = nullptr;
    QRadioButton *mAutoArchiveRB = nullptr;
    QSpinBox *mExpiryTimeNumInput = nullptr;
    QComboBox *mExpiryUnitsComboBox = nullptr;
    QCheckBox *mEvents = nullptr;
    QCheckBox *mTodos = nullptr;
    Akonadi::IncidenceChanger *mChanger = nullptr;
    Akonadi::ETMCalendar::Ptr mCalendar;
    QPushButton *const mUser1Button;
};
}
