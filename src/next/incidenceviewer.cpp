/*
  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  SPDX-FileContributor: Tobias Koenig <tokoe@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "incidenceviewer.h"
#include "attachmenthandler.h"
#include "incidenceviewer_p.h"
#include "urihandler.h"
#include "utils.h"

#include "incidenceattachmentmodel.h"

#include <Akonadi/Calendar/CalendarBase>
#include <CollectionFetchJob>
#include <ItemFetchScope>

#include <KCalUtils/IncidenceFormatter>

#include <KJob>
#include <QTextBrowser>

#include <QVBoxLayout>

using namespace CalendarSupport;

TextBrowser::TextBrowser(QWidget *parent)
    : QTextBrowser(parent)
{
    setFrameStyle(QFrame::NoFrame);
}

void TextBrowser::setSource(const QUrl &name)
{
    QString uri = name.toString();
    // QTextBrowser for some reason insists on putting // or / in links,
    // this is a crude workaround
    if (uri.startsWith(QLatin1String("uid:")) || uri.startsWith(QLatin1String("kmail:"))
        || uri.startsWith(QStringLiteral("urn:x-ical").section(QLatin1Char(':'), 0, 0)) || uri.startsWith(QLatin1String("news:"))
        || uri.startsWith(QLatin1String("mailto:"))) {
        uri.replace(QRegExp(QLatin1String("^([^:]+:)/+")), QStringLiteral("\\1"));
    }

    if (uri.startsWith(QLatin1String("ATTACH:"))) {
        Q_EMIT attachmentUrlClicked(uri);
    } else {
        UriHandler::process(uri);
    }
}

class Q_DECL_HIDDEN IncidenceViewer::Private
{
public:
    Private(IncidenceViewer *parent)
        : mParent(parent)
    {
        mAttachmentHandler = new AttachmentHandler(parent);
        mBrowser = new TextBrowser;
        parent->connect(mBrowser, &TextBrowser::attachmentUrlClicked, parent, [this](const QString &str) {
            slotAttachmentUrlClicked(str);
        });
    }

    void updateView()
    {
        QString text;

        if (mCurrentItem.isValid()) {
            text = KCalUtils::IncidenceFormatter::extensiveDisplayStr(CalendarSupport::displayName(mCalendar, mParentCollection),
                                                                      CalendarSupport::incidence(mCurrentItem),
                                                                      mDate);
            text.prepend(mHeaderText);
            mBrowser->setHtml(text);
        } else {
            text = mDefaultText;
            if (!mDelayedClear) {
                mBrowser->setHtml(text);
            }
        }
    }

    void slotParentCollectionFetched(KJob *job)
    {
        mParentCollectionFetchJob = nullptr;
        mParentCollection = Akonadi::Collection();

        if (!job->error()) {
            auto fetchJob = qobject_cast<Akonadi::CollectionFetchJob *>(job);
            if (!fetchJob->collections().isEmpty()) {
                mParentCollection = fetchJob->collections().at(0);
            }
        }

        updateView();
    }

    void slotAttachmentUrlClicked(const QString &uri)
    {
        const QString attachmentName = QString::fromUtf8(QByteArray::fromBase64(uri.mid(7).toUtf8()));
        mAttachmentHandler->view(attachmentName, CalendarSupport::incidence(mCurrentItem));
    }

    Akonadi::ETMCalendar *mCalendar = nullptr;
    IncidenceViewer *const mParent;
    TextBrowser *mBrowser = nullptr;
    Akonadi::Item mCurrentItem;
    QString mHeaderText;
    QString mDefaultText;
    Akonadi::Collection mParentCollection;
    Akonadi::CollectionFetchJob *mParentCollectionFetchJob = nullptr;
    IncidenceAttachmentModel *mAttachmentModel = nullptr;
    AttachmentHandler *mAttachmentHandler = nullptr;
    QDate mDate;
    bool mDelayedClear = false;
};

IncidenceViewer::IncidenceViewer(Akonadi::ETMCalendar *calendar, QWidget *parent)
    : QWidget(parent)
    , d(new Private(this))
{
    d->mCalendar = calendar;
    init();
}

IncidenceViewer::IncidenceViewer(QWidget *parent)
    : QWidget(parent)
    , d(new Private(this))
{
    d->mCalendar = nullptr;
    init();
}

void IncidenceViewer::init()
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    d->mBrowser->setOpenLinks(true);
    d->mBrowser->setMinimumHeight(1);

    layout->addWidget(d->mBrowser);

    // always fetch full payload for incidences
    fetchScope().fetchFullPayload();
    fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);

    d->updateView();
}

IncidenceViewer::~IncidenceViewer()
{
    delete d;
}

void IncidenceViewer::setCalendar(Akonadi::ETMCalendar *calendar)
{
    d->mCalendar = calendar;
}

Akonadi::Item IncidenceViewer::incidence() const
{
    return ItemMonitor::item();
}

QDate IncidenceViewer::activeDate() const
{
    return d->mDate;
}

QAbstractItemModel *IncidenceViewer::attachmentModel() const
{
    if (!d->mAttachmentModel) {
        d->mAttachmentModel = new IncidenceAttachmentModel(const_cast<IncidenceViewer *>(this));
    }
    return d->mAttachmentModel;
}

void IncidenceViewer::setDelayedClear(bool delayed)
{
    d->mDelayedClear = delayed;
}

void IncidenceViewer::setDefaultMessage(const QString &message)
{
    d->mDefaultText = message;
}

void IncidenceViewer::setHeaderText(const QString &text)
{
    d->mHeaderText = text;
}

void IncidenceViewer::setIncidence(const Akonadi::Item &incidence, QDate date)
{
    d->mDate = date;
    ItemMonitor::setItem(incidence);

    d->updateView();
}

void IncidenceViewer::itemChanged(const Akonadi::Item &item)
{
    if (!item.hasPayload<KCalendarCore::Incidence::Ptr>()) {
        d->mBrowser->clear();
        return;
    }

    d->mCurrentItem = item;

    if (d->mAttachmentModel) {
        d->mAttachmentModel->setItem(d->mCurrentItem);
    }

    if (d->mParentCollectionFetchJob) {
        disconnect(d->mParentCollectionFetchJob, SIGNAL(result(KJob *)), this, SLOT(slotParentCollectionFetched(KJob *)));
        delete d->mParentCollectionFetchJob;
    }

    d->mParentCollectionFetchJob = new Akonadi::CollectionFetchJob(d->mCurrentItem.parentCollection(), Akonadi::CollectionFetchJob::Base, this);

    connect(d->mParentCollectionFetchJob, SIGNAL(result(KJob *)), this, SLOT(slotParentCollectionFetched(KJob *)));
}

void IncidenceViewer::itemRemoved()
{
    d->mCurrentItem = Akonadi::Item();
    d->mBrowser->clear();
}

#include "moc_incidenceviewer.cpp"
#include "moc_incidenceviewer_p.cpp"
