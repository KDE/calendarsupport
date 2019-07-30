/*
  Copyright (c) 2008 Bruno Virlet <bvirlet@kdemail.net>

  This library is free software; you can redistribute it and/or modify it
  under the terms of the GNU Library General Public License as published by
  the Free Software Foundation; either version 2 of the License, or (at your
  option) any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
  License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to the
  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.
*/

#include "kcalmodel.h"

#include <Collection>
#include <Item>
#include <ItemFetchScope>

#include <KCalendarCore/FreeBusy>
#include <KCalendarCore/Event>
#include <KCalendarCore/Journal>
#include <KCalendarCore/Todo>

#include <KLocalizedString>

#include <QIcon>
using namespace CalendarSupport;

class Q_DECL_HIDDEN KCalModel::Private
{
public:
    Private(KCalModel *model)
        : q(model)
    {
    }

    static QStringList allMimeTypes()
    {
        QStringList types;
        types << KCalendarCore::Event::eventMimeType()
              << KCalendarCore::Todo::todoMimeType()
              << KCalendarCore::Journal::journalMimeType()
              << KCalendarCore::FreeBusy::freeBusyMimeType();
        return types;
    }

    bool collectionMatchesMimeTypes() const
    {
        const QStringList lst = allMimeTypes();
        for (const QString &type : lst) {
            if (q->collection().contentMimeTypes().contains(type)) {
                return true;
            }
        }
        return false;
    }

    bool collectionIsValid()
    {
        return
            !q->collection().isValid()
            || collectionMatchesMimeTypes()
            || q->collection().contentMimeTypes() == QStringList(QStringLiteral("inode/directory"));
    }

private:
    KCalModel *q;
};

KCalModel::KCalModel(QObject *parent)
    : ItemModel(parent)
    , d(new Private(this))
{
    fetchScope().fetchFullPayload();
}

KCalModel::~KCalModel()
{
    delete d;
}

QStringList KCalModel::mimeTypes() const
{
    return
        QStringList()
        << QStringLiteral("text/uri-list")
        << d->allMimeTypes();
}

int KCalModel::columnCount(const QModelIndex &) const
{
    if (d->collectionIsValid()) {
        return 4;
    } else {
        return 1;
    }
}

int KCalModel::rowCount(const QModelIndex &) const
{
    if (d->collectionIsValid()) {
        return ItemModel::rowCount();
    } else {
        return 1;
    }
}

QVariant KCalModel::data(const QModelIndex &index, int role) const
{
    if (role == ItemModel::IdRole) {
        return ItemModel::data(index, role);
    }

    if (!index.isValid() || index.row() >= rowCount()) {
        return QVariant();
    }

    // guard against use with collections that do not have the right contents
    if (!d->collectionIsValid()) {
        if (role == Qt::DisplayRole) {
            return i18nc("@info",
                         "This model can only handle event, task, journal or free-busy list folders. "
                         "The current collection holds mimetypes: %1",
                         collection().contentMimeTypes().join(QLatin1Char(',')));
        }
        return QVariant();
    }

    const Akonadi::Item item = itemForIndex(index);

    if (!item.hasPayload<KCalendarCore::Incidence::Ptr>()) {
        return QVariant();
    }

    const KCalendarCore::Incidence::Ptr incidence = item.payload<KCalendarCore::Incidence::Ptr>();

    // Icon for the model entry
    switch (role) {
    case Qt::DecorationRole:
        if (index.column() == 0) {
            if (incidence->type() == KCalendarCore::Incidence::TypeTodo) {
                return QIcon::fromTheme(QStringLiteral("view-pim-tasks"));
            } else if (incidence->type() == KCalendarCore::Incidence::TypeJournal) {
                return QIcon::fromTheme(QStringLiteral("view-pim-journal"));
            } else if (incidence->type() == KCalendarCore::Incidence::TypeEvent) {
                return QIcon::fromTheme(QStringLiteral("view-calendar"));
            } else {
                return QIcon::fromTheme(QStringLiteral("network-wired"));
            }
        }
        break;
    case Qt::DisplayRole:
        switch (index.column()) {
        case Summary:
            return incidence->summary();
        case DateTimeStart:
            return incidence->dtStart().toString();
        case DateTimeEnd:
            return incidence->dateTime(KCalendarCore::Incidence::RoleEnd).toString();
        case Type:
            return incidence->type();
        default:
            break;
        }
        break;
    default:
        return QVariant();
    }

    return QVariant();
}

QVariant KCalModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (!d->collectionIsValid()) {
        return QVariant();
    }

    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case Summary:
            return i18nc("@title:column, calendar event summary", "Summary");
        case DateTimeStart:
            return i18nc("@title:column, calendar event start date and time",
                         "Start date and time");
        case DateTimeEnd:
            return i18nc("@title:column, calendar event end date and time", "End date and time");
        case Type:
            return i18nc("@title:column, calendar event type", "Type");
        default:
            return QString();
        }
    }

    return Akonadi::ItemModel::headerData(section, orientation, role);
}
