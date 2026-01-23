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
    /*! Constructor disabled for public. Use instance() to create a KCalPrefs
    object. */
    KCalPrefs();
    ~KCalPrefs() override;

    /*! Get instance of KCalPrefs. It is made sure that there is only one
    instance. */
    static KCalPrefs *instance();

    /*! Set preferences to default values */
    void usrSetDefaults() override;

    /*! Read preferences from config file */
    void usrRead() override;

    /*! Write preferences to config file */
    bool usrSave() override;

    /*! Fill empty mail fields with default values. */
    void fillMailDefaults();

public:
    // preferences data
    /*!
     * \brief fullName
     * \return
     */
    QString fullName();
    /*!
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
