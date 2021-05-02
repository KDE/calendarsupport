/*
 * SPDX-FileCopyrightText: 2014 Sandro Knauß <knauss@kolabsys.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
 */

#include "noteeditdialog.h"

#include <Akonadi/Notes/NoteUtils>
#include <AkonadiWidgets/CollectionComboBox>

#include <KPIMTextEdit/RichTextEditor>
#include <KPIMTextEdit/RichTextEditorWidget>

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>

#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

using namespace CalendarSupport;

QAbstractItemModel *NoteEditDialog::_k_noteEditStubModel = nullptr;

NoteEditDialog::NoteEditDialog(QWidget *parent)
    : QDialog(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(i18nc("@title:window", "Create Note"));
    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    auto mainWidget = new QWidget(this);
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(mainWidget);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &NoteEditDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    buttonBox->button(QDialogButtonBox::Cancel)->setText(i18nc("@action:button", "Cancel"));

    mOkButton = buttonBox->button(QDialogButtonBox::Ok);
    mOkButton->setObjectName(QStringLiteral("save-button"));
    mOkButton->setDefault(true);
    mOkButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    mOkButton->setText(i18nc("@action:button", "Save"));
    mOkButton->setIcon(QIcon::fromTheme(QStringLiteral("view-pim-notes")));
    mOkButton->setEnabled(false);

    auto layout = new QGridLayout(mainWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    auto hbox = new QHBoxLayout;
    hbox->setContentsMargins(0, 0, 0, 0);
    hbox->setSpacing(2);

    mNoteTitle = new QLineEdit;
    mNoteTitle->setClearButtonEnabled(true);
    mNoteTitle->setObjectName(QStringLiteral("notetitle"));
    mNoteTitle->setFocus();
    connect(mNoteTitle, &QLineEdit::textChanged, this, &NoteEditDialog::slotUpdateButtons);

    mCollectionCombobox = new Akonadi::CollectionComboBox(_k_noteEditStubModel);
    mCollectionCombobox->setAccessRightsFilter(Akonadi::Collection::CanCreateItem);
    mCollectionCombobox->setMinimumWidth(250);
    mCollectionCombobox->setMimeTypeFilter(QStringList() << Akonadi::NoteUtils::noteMimeType());
    mCollectionCombobox->setObjectName(QStringLiteral("akonadicombobox"));
#ifndef QT_NO_ACCESSIBILITY
    mCollectionCombobox->setAccessibleDescription(i18nc("@info", "Calendar where the new note will be stored."));
#endif
    mCollectionCombobox->setToolTip(i18nc("@info:tooltip", "Calendar where the new note will be stored."));
    connect(mCollectionCombobox, QOverload<int>::of(&Akonadi::CollectionComboBox::currentIndexChanged), this, &NoteEditDialog::slotCollectionChanged);
    connect(mCollectionCombobox, QOverload<int>::of(&Akonadi::CollectionComboBox::activated), this, &NoteEditDialog::slotCollectionChanged);

    mNoteText = new KPIMTextEdit::RichTextEditorWidget(parent);
    mNoteText->setObjectName(QStringLiteral("notetext"));
    connect(mNoteText->editor(), &KPIMTextEdit::RichTextEditor::textChanged, this, &NoteEditDialog::slotUpdateButtons);

    // First line
    hbox->addWidget(mNoteTitle);
    hbox->addSpacing(5);
    hbox->addWidget(mCollectionCombobox);

    auto lab = new QLabel(i18nc("@label specify the title for this note", "Title:"), this);
    layout->addWidget(lab, 0, 0);
    layout->addLayout(hbox, 0, 1);

    // Second Line
    lab = new QLabel(i18nc("@label specify the text for this note", "Text:"), this);
    layout->addWidget(lab, 1, 0);
    layout->setAlignment(lab, Qt::AlignTop);
    layout->addWidget(mNoteText, 1, 1);

    readConfig();
}

NoteEditDialog::~NoteEditDialog()
{
    disconnect(mNoteText->editor(), &KPIMTextEdit::RichTextEditor::textChanged, this, &NoteEditDialog::slotUpdateButtons);
    writeConfig();
}

void NoteEditDialog::readConfig()
{
    KConfigGroup group(KSharedConfig::openStateConfig(), "NoteEditDialog");

    const QSize size = group.readEntry("Size", QSize(500, 300));
    if (size.isValid()) {
        resize(size);
    }
}

void NoteEditDialog::writeConfig()
{
    KConfigGroup group(KSharedConfig::openStateConfig(), "NoteEditDialog");
    group.writeEntry("Size", size());
    group.sync();
}

void NoteEditDialog::slotUpdateButtons()
{
    if (mNoteTitle->text().trimmed().isEmpty() && mNoteText->isEmpty()) {
        mOkButton->setEnabled(false);
    } else {
        mOkButton->setEnabled(true);
    }
}

Akonadi::Collection NoteEditDialog::collection() const
{
    return mCollection;
}

void NoteEditDialog::slotCollectionChanged(int index)
{
    Q_UNUSED(index)
    setCollection(mCollectionCombobox->currentCollection());
}

void NoteEditDialog::setCollection(const Akonadi::Collection &value)
{
    if (mCollection != value) {
        mCollection = value;
        Q_EMIT collectionChanged(mCollection);
    }
}

void NoteEditDialog::accept()
{
    QDialog::accept();
    const Akonadi::Collection collection = mCollectionCombobox->currentCollection();
    if (!collection.isValid()) {
        return;
    }

    if (mNoteTitle->text().isEmpty() && mNoteText->isEmpty()) {
        return;
    }

    Akonadi::NoteUtils::NoteMessageWrapper note(mItem.payload<KMime::Message::Ptr>());
    note.setTitle(mNoteTitle->text());
    if (mNoteText->acceptRichText()) {
        note.setText(mNoteText->editor()->toHtml(), Qt::RichText);
    } else {
        note.setText(mNoteText->editor()->toPlainText(), Qt::PlainText);
    }
    mItem.setPayload<KMime::Message::Ptr>(note.message());
    Q_EMIT createNote(mItem, collection);
}

void NoteEditDialog::load(const Akonadi::Item &item)
{
    mItem = item;
    Akonadi::NoteUtils::NoteMessageWrapper note(item.payload<KMime::Message::Ptr>());
    mNoteText->editor()->setHtml(note.text());
    if (note.textFormat() == Qt::RichText) {
        mNoteText->setAcceptRichText(true);
    } else {
        mNoteText->setAcceptRichText(false);
    }
    mNoteTitle->setText(note.title());
}

KMime::Message::Ptr NoteEditDialog::note() const
{
    if (mItem.hasPayload<KMime::Message::Ptr>()) {
        return mItem.payload<KMime::Message::Ptr>();
    } else {
        return KMime::Message::Ptr();
    }
}
