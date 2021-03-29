/*
  SPDX-FileCopyrightText: 2000, 2001 Cornelius Schumacher <schumacher@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/
#pragma once

#include "calendarsupport_export.h"
#include "kcalprefs_base.h"

#include <Collection>

namespace CalendarSupport
{
class CALENDARSUPPORT_EXPORT KCalPrefs : public KCalPrefsBase
{
    Q_OBJECT
public:
    /** Constructor disabled for public. Use instance() to create a KCalPrefs
    object. */
    KCalPrefs();
    ~KCalPrefs() override;

    /** Get instance of KCalPrefs. It is made sure that there is only one
    instance. */
    static KCalPrefs *instance();

    /** Set preferences to default values */
    void usrSetDefaults() override;

    /** Read preferences from config file */
    void usrRead() override;

    /** Write preferences to config file */
    bool usrSave() override;

    /** Fill empty mail fields with default values. */
    void fillMailDefaults();

public:
    // preferences data
    QString fullName();
    QString email();
    /// Returns all email addresses for the user.
    QStringList allEmails();
    /// Returns all email addresses together with the full username for the user.
    QStringList fullEmails();
    /// Return true if the given email belongs to the user
    bool thatIsMe(const QString &email);

    Akonadi::Collection::Id defaultCalendarId() const;
    void setDefaultCalendarId(Akonadi::Collection::Id);

    void setCategoryColor(const QString &cat, const QColor &color);
    QColor categoryColor(const QString &cat) const;
    bool hasCategoryColor(const QString &cat) const;

    void setDayBegins(const QDateTime &dateTime);
    QDateTime dayBegins() const;

private:
    class Private;
    Private *const d;
};
}

