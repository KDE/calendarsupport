/*
  SPDX-FileCopyrightText: 2002 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2005 Rafal Rzepecki <divide@users.sourceforge.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "calendarsupport_export.h"

#include <QHash>
#include <QObject>

class KCoreConfigSkeleton;
class QColor;

namespace CalendarSupport
{
class CALENDARSUPPORT_EXPORT CategoryConfig : public QObject
{
    Q_OBJECT
public:
    explicit CategoryConfig(KCoreConfigSkeleton *cfg, QObject *parent = nullptr);
    ~CategoryConfig() override;
    Q_REQUIRED_RESULT QStringList customCategories() const;
    void setCustomCategories(const QStringList &categories);

    Q_REQUIRED_RESULT QHash<QString, QColor> readColors() const;
    void setColors(const QHash<QString, QColor> &colors);

    void writeConfig();

    static const QString categorySeparator;

private:
    Q_DISABLE_COPY(CategoryConfig)
    class Private;
    Private *const d;
};
}

