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

#include <Akonadi/CalendarBase>
#include <Akonadi/CalendarUtils>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/ETMCalendar>
#include <Akonadi/ItemFetchScope>

#include <KCalUtils/IncidenceFormatter>

#include <KJob>
#include <QRegularExpression>
#include <QTextBrowser>

#include <QVBoxLayout>

using namespace CalendarSupport;

TextBrowser::TextBrowser(QWidget *parent)
    : QTextBrowser(parent)
{
    setFrameStyle(QFrame::NoFrame);
}

void TextBrowser::doSetSource(const QUrl &name, QTextDocument::ResourceType type)
{
    Q_UNUSED(type);
    QString uri = name.toString();
    // QTextBrowser for some reason insists on putting // or / in links,
    // this is a crude workaround
    if (uri.startsWith(QLatin1String("uid:")) || uri.startsWith(QLatin1String("kmail:"))
        || uri.startsWith(QStringLiteral("urn:x-ical").section(QLatin1Char(':'), 0, 0)) || uri.startsWith(QLatin1String("news:"))
        || uri.startsWith(QLatin1String("mailto:"))) {
        uri.replace(QRegularExpression(QLatin1String("^([^:]+:)/+")), QStringLiteral("\\1"));
    }

    if (uri.startsWith(QLatin1String("ATTACH:"))) {
        Q_EMIT attachmentUrlClicked(uri);
    } else {
        UriHandler::process(uri);
    }
}

class CalendarSupport::IncidenceViewerPrivate
{
public:
    explicit IncidenceViewerPrivate(IncidenceViewer *parent)
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
            text = KCalUtils::IncidenceFormatter::extensiveDisplayStr(Akonadi::CalendarUtils::displayName(mETM, mParentCollection),
                                                                      Akonadi::CalendarUtils::incidence(mCurrentItem),
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
        mAttachmentHandler->view(attachmentName, Akonadi::CalendarUtils::incidence(mCurrentItem));
    }

    Akonadi::EntityTreeModel *mETM = nullptr;
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
    , d(new IncidenceViewerPrivate(this))
{
    d->mETM = calendar->entityTreeModel();
    init();
}

IncidenceViewer::IncidenceViewer(Akonadi::EntityTreeModel *etm, QWidget *parent)
    : QWidget(parent)
    , d(new IncidenceViewerPrivate(this))
{
    d->mETM = etm;
    init();
}

IncidenceViewer::IncidenceViewer(QWidget *parent)
    : QWidget(parent)
    , d(new IncidenceViewerPrivate(this))
{
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

IncidenceViewer::~IncidenceViewer() = default;

void IncidenceViewer::setCalendar(Akonadi::ETMCalendar *calendar)
{
    d->mETM = calendar->entityTreeModel();
}

void IncidenceViewer::setModel(Akonadi::EntityTreeModel *model)
{
    d->mETM = model;
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
