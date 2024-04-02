/*
  SPDX-FileCopyrightText: 2003 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2005 Rafal Rzepecki <divide@users.sourceforge.net>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#include "urihandler.h"

#include "calendarsupport_debug.h"

#include <KIO/ApplicationLauncherJob>
#include <KIO/OpenUrlJob>
#include <KService>

#include <QDesktopServices>
#include <QUrl>
#include <QUrlQuery>

using namespace CalendarSupport;
using namespace Qt::Literals::StringLiterals;
namespace
{
bool startService(const QString &desktopFileName, const QString &uri)
{
    const auto service = KService::serviceByDesktopName(desktopFileName);
    if (!service) {
        qWarning() << "Desktop file not found:" << desktopFileName << ".desktop  -- please check your installation";
        return false;
    }
    auto job = new KIO::ApplicationLauncherJob(service);
    job->setUrls({QUrl{uri}});
    QObject::connect(job, &KJob::result, job, [desktopFileName](KJob *job) {
        if (job->error()) {
            qCWarning(CALENDARSUPPORT_LOG) << "Failed to start" << desktopFileName << ":" << job->errorText();
        }
    });
    job->start();
    return true;
}

bool startKOrganizer(const QString &uri)
{
    return startService(QStringLiteral("korganizer-view"), uri);
}

bool startKMail(const QString &uri)
{
    return startService(QStringLiteral("kmail_view"), uri);
}

bool startKAddressbook(const QString &uri)
{
    return startService(QStringLiteral("kaddressbook-view"), uri);
}

} // namespace

bool UriHandler::process(const QString &uri)
{
    qCDebug(CALENDARSUPPORT_LOG) << uri;

    if (uri.startsWith("kmail:"_L1)) {
        // extract 'number' from 'kmail:<number>/<id>'
        const int start = uri.indexOf(QLatin1Char(':')) + 1;
        const int end = uri.indexOf(QLatin1Char('/'), start);
        const QString serialNumberStr = uri.mid(start, start - end);
        return startKMail(QStringLiteral("akonadi://?item=%1").arg(serialNumberStr));
    } else if (uri.startsWith("mailto:"_L1)) {
        return QDesktopServices::openUrl(QUrl(uri));
    } else if (uri.startsWith("uid:"_L1)) {
        const QString uid = uri.mid(4);
        return startKAddressbook(QStringLiteral("akonadi://?item=%1").arg(uid));
    } else if (uri.startsWith("urn:x-ical"_L1)) {
        const QString uid = QUrl::fromPercentEncoding(uri.toLatin1()).mid(11);
        return startKOrganizer(QStringLiteral("akonadi://?item=%1").arg(uid));
    } else if (uri.startsWith("akonadi:"_L1)) {
        const QString mimeType = QUrlQuery(QUrl(uri)).queryItemValue(QStringLiteral("type")).toLower();
        if (mimeType == "message/rfc822"_L1) {
            return startKMail(uri);
        } else if (mimeType == "text/calendar"_L1) {
            return startKOrganizer(uri);
        } else if (mimeType == "text/directory"_L1) {
            return startKAddressbook(uri);
        }
    } else { // no special URI, let KDE handle it
        auto job = new KIO::OpenUrlJob(QUrl(uri));
        job->start();
    }

    return false;
}
