/*
  SPDX-FileCopyrightText: 2015 Sandro Knauß <knauss@kolabsys.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#pragma once

#include <AkonadiCore/Tag>

namespace Akonadi
{
class Monitor;
}
class KJob;

namespace CalendarSupport
{
/**
 * A tag cache
 */
class TagCache : public QObject
{
    Q_OBJECT
public:
    TagCache();
    Q_REQUIRED_RESULT Akonadi::Tag getTagByGid(const QByteArray &gid) const;
    Q_REQUIRED_RESULT Akonadi::Tag getTagByName(const QString &name) const;

private Q_SLOTS:
    void onTagAdded(const Akonadi::Tag &);
    void onTagChanged(const Akonadi::Tag &);
    void onTagRemoved(const Akonadi::Tag &);
    void onTagsFetched(KJob *);

private:
    void retrieveTags();

    QHash<Akonadi::Tag::Id, Akonadi::Tag> mCache;
    QHash<QByteArray, Akonadi::Tag::Id> mGidCache;
    QHash<QString, Akonadi::Tag::Id> mNameCache;
    Akonadi::Monitor *const mMonitor;
};
}

