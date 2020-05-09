/*
  Copyright (c) 2003 Cornelius Schumacher <schumacher@kde.org>
  Copyright (c) 2005 Rafal Rzepecki <divide@users.sourceforge.net>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/

#include "urihandler.h"
#include "calendarsupport_debug.h"

#include <KRun>
#include <KService>
#include <KDesktopFile>

#include <QUrl>
#include <QUrlQuery>
#include <QDesktopServices>

using namespace CalendarSupport;

namespace {

bool startService(const QString &desktopFileName, const QString &uri)
{
    KDesktopFile desktopFile(QStandardPaths::ApplicationsLocation, desktopFileName);
    const KService service{&desktopFile};
    if (!KRun::run(service.exec(), {QUrl{uri}}, nullptr)) {
        qCWarning(CALENDARSUPPORT_LOG) << "Failed to start" << desktopFileName;
        return false;
    }

    return true;
}

bool startKOrganizer(const QString &uri)
{
    return startService(QStringLiteral("korganizer-view.desktop"), uri);
}

bool startKMail(const QString &uri)
{
    return startService(QStringLiteral("kmail_view.desktop"), uri);
}

bool startKAddressbook(const QString &uri)
{
    return startService(QStringLiteral("kaddressbook-view.desktop"), uri);
}

} // namespace

bool UriHandler::process(const QString &uri)
{
    qCDebug(CALENDARSUPPORT_LOG) << uri;

    if (uri.startsWith(QLatin1String("kmail:"))) {
        // extract 'number' from 'kmail:<number>/<id>'
        const int start = uri.indexOf(QLatin1Char(':')) + 1;
        const int end = uri.indexOf(QLatin1Char('/'), start);
        const QString serialNumberStr = uri.mid(start, start - end);
        return startKMail(QStringLiteral("akonadi://?item=%1").arg(serialNumberStr));
    } else if (uri.startsWith(QLatin1String("mailto:"))) {
        return QDesktopServices::openUrl(QUrl(uri));
    } else if (uri.startsWith(QLatin1String("uid:"))) {
        const QString uid = uri.mid(4);
        return startKAddressbook(QStringLiteral("akonadi://?item=%1").arg(uid));
    } else if (uri.startsWith(QLatin1String("urn:x-ical"))) {
        const QString uid = QUrl::fromPercentEncoding(uri.toLatin1()).mid(11);
        return startKOrganizer(QStringLiteral("akonadi://?item=%1").arg(uid));
    } else if (uri.startsWith(QLatin1String("akonadi:"))) {
        const QString mimeType = QUrlQuery(QUrl(uri)).queryItemValue(QStringLiteral("type"));
        if (mimeType.toLower() == QLatin1String("message/rfc822")) {
            return startKMail(uri);
        } else if (mimeType.toLower() == QLatin1String("text/calendar")) {
            return startKOrganizer(uri);
        } else if (mimeType.toLower() == QLatin1String("text/directory")) {
            return startKAddressbook(uri);
        }
    } else {  // no special URI, let KDE handle it
        new KRun(QUrl(uri), nullptr);
    }

    return false;
}
