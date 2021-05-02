/*
  SPDX-FileCopyrightText: 2010 Klarlvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
  SPDX-FileContributor: Allen Winter <allen.winter@kdab.com>

  SPDX-FileCopyrightText: 2014 Sergio Martins <iamsergio@gmail.com>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

/**
  @file
  This file is part of the API for handling calendar data and provides
  static functions for dealing with calendar incidence attachments.

  @brief
  vCalendar/iCalendar attachment handling.

  @author Allen Winter \<winter@kde.org\>
*/
#include "attachmenthandler.h"
#include "calendarsupport/utils.h"
#include "calendarsupport_debug.h"

#include <ItemFetchJob>

#include <KIO/FileCopyJob>
#include <KIO/JobUiDelegate>
#include <KIO/OpenUrlJob>
#include <KIO/StatJob>
#include <KJob>
#include <KJobWidgets>
#include <KLocalizedString>
#include <KMessageBox>

#include <QDesktopServices>
#include <QFile>
#include <QFileDialog>
#include <QMimeDatabase>
#include <QPointer>
#include <QTemporaryFile>

using namespace KCalendarCore;
using namespace Akonadi;

namespace CalendarSupport
{
struct ReceivedInfo {
    QString uid;
    QString attachmentName;
};

class Q_DECL_HIDDEN AttachmentHandler::Private
{
public:
    Private(QWidget *parent)
        : mParent(parent)
    {
    }

    QMap<KJob *, ReceivedInfo> mJobToReceivedInfo;
    QPointer<QWidget> const mParent;
};

AttachmentHandler::AttachmentHandler(QWidget *parent)
    : QObject(parent)
    , d(new Private(parent))
{
}

AttachmentHandler::~AttachmentHandler()
{
    delete d;
}

Attachment AttachmentHandler::find(const QString &attachmentName, const Incidence::Ptr &incidence)
{
    if (!incidence) {
        return Attachment();
    }

    // get the attachment by name from the incidence
    const Attachment::List as = incidence->attachments();
    Attachment a;
    if (!as.isEmpty()) {
        Attachment::List::ConstIterator it;
        Attachment::List::ConstIterator end(as.constEnd());

        for (it = as.constBegin(); it != end; ++it) {
            if ((*it).label() == attachmentName) {
                a = *it;
                break;
            }
        }
    }

    if (a.isEmpty()) {
        KMessageBox::error(d->mParent, i18n("No attachment named \"%1\" found in the incidence.", attachmentName));
        return Attachment();
    }

    if (a.isUri()) {
        auto job = KIO::statDetails(QUrl(a.uri()), KIO::StatJob::SourceSide, KIO::StatBasic);

        KJobWidgets::setWindow(job, d->mParent);
        if (!job->exec()) {
            KMessageBox::sorry(
                d->mParent,
                i18n("The attachment \"%1\" is a web link that is inaccessible from this computer. ", QUrl::fromPercentEncoding(a.uri().toLatin1())));
            return Attachment();
        }
    }
    return a;
}

Attachment AttachmentHandler::find(const QString &attachmentName, const ScheduleMessage::Ptr &message)
{
    if (!message) {
        return Attachment();
    }

    Incidence::Ptr incidence = message->event().dynamicCast<Incidence>();
    if (!incidence) {
        KMessageBox::error(d->mParent,
                           i18n("The calendar invitation stored in this email message is broken in some way. "
                                "Unable to continue."));
        return Attachment();
    }

    return find(attachmentName, incidence);
}

static QTemporaryFile *s_tempFile = nullptr;

static QUrl tempFileForAttachment(const Attachment &attachment)
{
    QUrl url;

    QMimeDatabase db;
    QStringList patterns = db.mimeTypeForName(attachment.mimeType()).globPatterns();
    if (!patterns.empty()) {
        s_tempFile = new QTemporaryFile(QDir::tempPath() + QLatin1String("/attachementview_XXXXXX") + patterns.first().remove(QLatin1Char('*')));
    } else {
        s_tempFile = new QTemporaryFile();
    }
    s_tempFile->setAutoRemove(false);
    s_tempFile->open();
    s_tempFile->setPermissions(QFile::ReadUser);
    s_tempFile->write(QByteArray::fromBase64(attachment.data()));
    s_tempFile->close();
    QFile tf(s_tempFile->fileName());
    if (tf.size() != attachment.size()) {
        // whoops. failed to write the entire attachment. return an invalid URL.
        delete s_tempFile;
        s_tempFile = nullptr;
        return url;
    }

    url.setPath(s_tempFile->fileName());
    return url;
}

bool AttachmentHandler::view(const Attachment &attachment)
{
    if (attachment.isEmpty()) {
        return false;
    }

    bool stat = true;
    if (attachment.isUri()) {
        QDesktopServices::openUrl(QUrl(attachment.uri()));
    } else {
        // put the attachment in a temporary file and launch it
        QUrl tempUrl = tempFileForAttachment(attachment);
        if (tempUrl.isValid()) {
            auto job = new KIO::OpenUrlJob(tempUrl, attachment.mimeType());
            job->setDeleteTemporaryFile(true);
            job->setRunExecutables(true);
            job->start();
        } else {
            stat = false;
            KMessageBox::error(d->mParent, i18n("Unable to create a temporary file for the attachment."));
        }
        delete s_tempFile;
        s_tempFile = nullptr;
    }
    return stat;
}

bool AttachmentHandler::view(const QString &attachmentName, const Incidence::Ptr &incidence)
{
    return view(find(attachmentName, incidence));
}

void AttachmentHandler::view(const QString &attachmentName, const QString &uid)
{
    Item item;
    item.setGid(uid);
    auto job = new ItemFetchJob(item);
    connect(job, &ItemFetchJob::result, this, &AttachmentHandler::slotFinishView);
    ReceivedInfo info;
    info.attachmentName = attachmentName;
    info.uid = uid;
    d->mJobToReceivedInfo[job] = info;
}

bool AttachmentHandler::view(const QString &attachmentName, const ScheduleMessage::Ptr &message)
{
    return view(find(attachmentName, message));
}

bool AttachmentHandler::saveAs(const Attachment &attachment)
{
    // get the saveas file name
    const QString saveAsFile = QFileDialog::getSaveFileName(d->mParent, i18n("Save Attachment"), attachment.label());
    if (saveAsFile.isEmpty()) {
        return false;
    }

    bool stat = false;
    if (attachment.isUri()) {
        // save the attachment url
        auto job = KIO::file_copy(QUrl(attachment.uri()), QUrl::fromLocalFile(saveAsFile));
        stat = job->exec();
    } else {
        // put the attachment in a temporary file and save it
        QUrl tempUrl = tempFileForAttachment(attachment);
        if (tempUrl.isValid()) {
            auto job = KIO::file_copy(tempUrl, QUrl::fromLocalFile(saveAsFile));
            stat = job->exec();
            if (!stat && job->error()) {
                KMessageBox::error(d->mParent, job->errorString());
            }
        } else {
            stat = false;
            KMessageBox::error(d->mParent, i18n("Unable to create a temporary file for the attachment."));
        }
        delete s_tempFile;
        s_tempFile = nullptr;
    }
    return stat;
}

bool AttachmentHandler::saveAs(const QString &attachmentName, const Incidence::Ptr &incidence)
{
    return saveAs(find(attachmentName, incidence));
}

void AttachmentHandler::saveAs(const QString &attachmentName, const QString &uid)
{
    Item item;
    item.setGid(uid);
    auto job = new ItemFetchJob(item);
    connect(job, &ItemFetchJob::result, this, &AttachmentHandler::slotFinishView);

    ReceivedInfo info;
    info.attachmentName = attachmentName;
    info.uid = uid;
    d->mJobToReceivedInfo[job] = info;
}

bool AttachmentHandler::saveAs(const QString &attachmentName, const ScheduleMessage::Ptr &message)
{
    return saveAs(find(attachmentName, message));
}

void AttachmentHandler::slotFinishSaveAs(KJob *job)
{
    ReceivedInfo info = d->mJobToReceivedInfo[job];
    bool success = false;

    if (job->error() != 0) {
        auto fetchJob = qobject_cast<ItemFetchJob *>(job);
        const Item::List items = fetchJob->items();
        if (!items.isEmpty()) {
            Incidence::Ptr incidence = CalendarSupport::incidence(items.first());
            success = incidence && saveAs(info.attachmentName, incidence);
        } else {
            qCWarning(CALENDARSUPPORT_LOG) << Q_FUNC_INFO << "No item found";
        }
    } else {
        qCWarning(CALENDARSUPPORT_LOG) << Q_FUNC_INFO << "Job error:" << job->errorString();
    }

    Q_EMIT saveAsFinished(info.uid, info.attachmentName, success);
    d->mJobToReceivedInfo.remove(job);
}

void AttachmentHandler::slotFinishView(KJob *job)
{
    ReceivedInfo info = d->mJobToReceivedInfo[job];
    bool success = false;

    if (job->error()) {
        auto fetchJob = qobject_cast<ItemFetchJob *>(job);
        const Item::List items = fetchJob->items();
        if (!items.isEmpty()) {
            Incidence::Ptr incidence = CalendarSupport::incidence(items.first());
            success = incidence && view(info.attachmentName, incidence);
        } else {
            qCWarning(CALENDARSUPPORT_LOG) << Q_FUNC_INFO << "No item found";
        }
    } else {
        qCWarning(CALENDARSUPPORT_LOG) << Q_FUNC_INFO << "Job error:" << job->errorString();
    }

    Q_EMIT viewFinished(info.uid, info.attachmentName, success);
    d->mJobToReceivedInfo.remove(job);
}
} // namespace CalendarSupport
