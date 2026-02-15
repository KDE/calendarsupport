/*
  SPDX-FileCopyrightText: 2000, 2001 Cornelius Schumacher <schumacher@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/
#pragma once

#include "calendarsupport_export.h"
#include "kcalprefs_base.h"

#include <Akonadi/Collection>

#include <memory>

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
     */
    void usrSetDefaults() override;

    /*!
     * Reads preferences from the configuration file.
     */
    void usrRead() override;

    /*!
     * Writes preferences to the configuration file.
     * \return true if successful, false otherwise.
     */
    bool usrSave() override;

    /*!
     * Fills empty mail fields with default values.
     */
    void fillMailDefaults();

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
     */
    Akonadi::Collection::Id defaultCalendarId() const;
    /*!
     */
    void setDefaultCalendarId(Akonadi::Collection::Id);

    /*!
     */
    void setDayBegins(const QDateTime &dateTime);
    /*!
     */
    QDateTime dayBegins() const;

private:
    std::unique_ptr<KCalPrefsPrivate> const d;
};
}
