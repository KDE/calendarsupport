/*
  SPDX-FileCopyrightText: 2015 Sandro Knauß <knauss@kolabsys.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#include "tagcache.h"
#include "calendarsupport_debug.h"

#include <AkonadiCore/Monitor>
#include <AkonadiCore/TagAttribute>
#include <AkonadiCore/TagFetchJob>
#include <AkonadiCore/TagFetchScope>

using namespace CalendarSupport;

TagCache::TagCache()
    : QObject()
    , mMonitor(new Akonadi::Monitor(this))
{
    mMonitor->setObjectName(QStringLiteral("TagCacheMonitor"));
    mMonitor->setTypeMonitored(Akonadi::Monitor::Tags);
    mMonitor->tagFetchScope().fetchAttribute<Akonadi::TagAttribute>();
    connect(mMonitor, &Akonadi::Monitor::tagAdded, this, &TagCache::onTagAdded);
    connect(mMonitor, &Akonadi::Monitor::tagRemoved, this, &TagCache::onTagRemoved);
    connect(mMonitor, &Akonadi::Monitor::tagChanged, this, &TagCache::onTagChanged);
    retrieveTags();
}

Akonadi::Tag TagCache::getTagByGid(const QByteArray &gid) const
{
    return mCache.value(mGidCache.value(gid));
}

Akonadi::Tag TagCache::getTagByName(const QString &name) const
{
    return mCache.value(mNameCache.value(name));
}

void TagCache::onTagAdded(const Akonadi::Tag &tag)
{
    mCache.insert(tag.id(), tag);
    mGidCache.insert(tag.gid(), tag.id());
    mNameCache.insert(tag.name(), tag.id());
}

void TagCache::onTagChanged(const Akonadi::Tag &tag)
{
    onTagAdded(tag);
}

void TagCache::onTagRemoved(const Akonadi::Tag &tag)
{
    mCache.remove(tag.id());
    mGidCache.remove(tag.gid());
    mNameCache.remove(tag.name());
}

void TagCache::retrieveTags()
{
    auto tagFetchJob = new Akonadi::TagFetchJob(this);
    tagFetchJob->fetchScope().fetchAttribute<Akonadi::TagAttribute>();
    connect(tagFetchJob, &Akonadi::TagFetchJob::result, this, &TagCache::onTagsFetched);
}

void TagCache::onTagsFetched(KJob *job)
{
    if (job->error()) {
        qCWarning(CALENDARSUPPORT_LOG) << "Failed to fetch tags: " << job->errorString();
        return;
    }
    auto fetchJob = static_cast<Akonadi::TagFetchJob *>(job);
    const Akonadi::Tag::List lst = fetchJob->tags();
    for (const Akonadi::Tag &tag : lst) {
        onTagAdded(tag);
    }
}
