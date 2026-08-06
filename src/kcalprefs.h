/*
  SPDX-FileCopyrightText: 2000, 2001 Cornelius Schumacher <schumacher@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/
#pragma once

#include "calendarsupport_export.h"
#include "kcalprefs_base.h"

#include <Akonadi/Collection>

#include <memory>

class QAbstractItemModel;

namespace CalendarSupport
{
class KCalPrefsPrivate;

/*!
 * \class CalendarSupport::KCalPrefs
 * \inmodule CalendarSupport
 * \inheaderfile CalendarSupport/KCalPrefs
 *
 * \brief The KCalPrefs class
 */
class CALENDARSUPPORT_EXPORT KCalPrefs : public KCalPrefsBase
{
    Q_OBJECT
public:
    /*!
     * Constructs a KCalPrefs object.
     * Note: Constructor is disabled for public use. Use instance() to create a KCalPrefs object.
     */
    KCalPrefs();
    /*!
     * Destroys the KCalPrefs object.
     */
    ~KCalPrefs() override;

    /*!
     * Gets the singleton instance of KCalPrefs.
     * \return The singleton instance of KCalPrefs.
     */
    static KCalPrefs *instance();

    /*!
     * Sets preferences to their default values.
     * \since 6.8.0
     */
    void setPrefsDefaults();

    /*!
     * Reads preferences from the configuration file.
     * \since 6.8.0
     */
    void readPrefs();

    /*!
     * Writes preferences to the configuration file.
     * \return true if successful, false otherwise.
     * \since 6.8.0
     */
    bool savePrefs();

    /*!
     * Fills empty mail fields with default values.
     */
    void fillMailDefaults();

protected:
    void usrSetDefaults() override;
    void usrRead() override;
    bool usrSave() override;

public:
    // preferences data
    /*!
     * Gets the full name from preferences.
     * \return The full name.
     */
    QString fullName();
    /*!
     * Gets the email address from preferences.
     * \return The email address.
     */
    QString email();
    /// Returns all email addresses for the user.
    /*!
     * \return all email addresses for the user.
     */
    QStringList allEmails();
    /*!
     * \return all email addresses together with the full username for the user.
     */
    QStringList fullEmails();
    /*!
     * \brief fullName
     * \return true if the given email belongs to the user
     */
    bool thatIsMe(const QString &email);

    /*!
     * Gets the collection Id for the default events calendar.
     */
    Akonadi::Collection::Id defaultEventCalendarId() const;

    /*!
     * Sets the collection Id for the events calendar.
     */
    void setDefaultEventCalendarId(Akonadi::Collection::Id);

    /*!
     * Gets the collection Id for the default todo calendar.
     */
    Akonadi::Collection::Id defaultTodoCalendarId() const;

    /*!
     * Sets the collection Id for the todo calendar.
     */
    void setDefaultTodoCalendarId(Akonadi::Collection::Id);

    /*!
     * Sets the collection model (an EntityTreeModel or a proxy on top of one) used to resolve the
     * default calendars, which are persisted by stable remote path rather than by numeric id.
     *
     * Each application that reads the default calendars must call this once, when its calendar is
     * available. The default-calendar ids are then resolved from the stored paths against \a model
     * (and a legacy numeric id is migrated to a path). Without a model the ids resolve to -1, just
     * as they did before the calendar was loaded.
     * \since 6.9.0
     */
    void setCollectionModel(QAbstractItemModel *model);

    /*!
     * Returns the stable remote path persisted for the default events calendar (empty if none).
     * \since 6.9.0
     */
    [[nodiscard]] QString defaultEventCalendarPath() const;

    /*!
     * Returns the stable remote path persisted for the default todo calendar (empty if none).
     * \since 6.9.0
     */
    [[nodiscard]] QString defaultTodoCalendarPath() const;

private:
    // Resolves the default calendars from their stored paths against the collection model, and
    // migrates a legacy numeric id to a path. Runs as the model gets populated.
    void resolveDefaultCalendars();

    std::unique_ptr<KCalPrefsPrivate> const d;
};
}
