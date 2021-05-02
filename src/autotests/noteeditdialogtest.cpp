/*
 * SPDX-FileCopyrightText: 2014 Sandro Knauß <knauss@kolabsys.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
 */

#include "noteeditdialogtest.h"
#include "noteeditdialog.h"

#include <Akonadi/Notes/NoteUtils>
#include <AkonadiCore/EntityTreeModel>
#include <AkonadiWidgets/CollectionComboBox>

#include <KPIMTextEdit/RichTextEditor>
#include <KPIMTextEdit/RichTextEditorWidget>

#include <QLineEdit>
#include <QPushButton>
#include <QSignalSpy>
#include <QStandardItemModel>
#include <QStandardPaths>

using namespace CalendarSupport;

#include <QTest>
NoteEditDialogTest::NoteEditDialogTest()
{
    QStandardPaths::setTestModeEnabled(true);
    qRegisterMetaType<Akonadi::Collection>();
    qRegisterMetaType<Akonadi::Item>();
    qRegisterMetaType<KMime::Message::Ptr>();

    auto model = new QStandardItemModel;
    for (int id = 42; id < 51; ++id) {
        Akonadi::Collection collection(id);
        collection.setRights(Akonadi::Collection::AllRights);
        collection.setName(QString::number(id));
        collection.setContentMimeTypes(QStringList() << Akonadi::NoteUtils::noteMimeType());

        auto item = new QStandardItem(collection.name());
        item->setData(QVariant::fromValue(collection), Akonadi::EntityTreeModel::CollectionRole);
        item->setData(QVariant::fromValue(collection.id()), Akonadi::EntityTreeModel::CollectionIdRole);

        model->appendRow(item);
    }
    NoteEditDialog::_k_noteEditStubModel = model;
}

void NoteEditDialogTest::shouldHaveDefaultValuesOnCreation()
{
    NoteEditDialog edit;
    QVERIFY(!edit.note());
    auto notetitle = edit.findChild<QLineEdit *>(QStringLiteral("notetitle"));
    auto notetext = edit.findChild<KPIMTextEdit::RichTextEditorWidget *>(QStringLiteral("notetext"));
    auto ok = edit.findChild<QPushButton *>(QStringLiteral("save-button"));
    QVERIFY(notetitle);
    QCOMPARE(notetitle->text(), QString());
    QVERIFY(notetext);
    QCOMPARE(notetext->toPlainText(), QString());
    QVERIFY(ok);
    QCOMPARE(ok->isEnabled(), false);
}

void NoteEditDialogTest::shouldEmitCollectionChanged()
{
    NoteEditDialog edit;
    QSignalSpy spy(&edit, &NoteEditDialog::collectionChanged);
    edit.setCollection(Akonadi::Collection(42));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).value<Akonadi::Collection>(), Akonadi::Collection(42));
}

void NoteEditDialogTest::shouldNotEmitWhenCollectionIsNotChanged()
{
    NoteEditDialog edit;
    edit.setCollection(Akonadi::Collection(42));
    QSignalSpy spy(&edit, &NoteEditDialog::collectionChanged);
    edit.setCollection(Akonadi::Collection(42));
    QCOMPARE(spy.count(), 0);
}

void NoteEditDialogTest::shouldHaveSameValueAfterSet()
{
    NoteEditDialog edit;

    Akonadi::NoteUtils::NoteMessageWrapper note;
    Akonadi::Item item;
    item.setMimeType(Akonadi::NoteUtils::noteMimeType());
    KMime::Message::Ptr message(note.message());
    item.setPayload(message);

    edit.setCollection(Akonadi::Collection(42));
    edit.load(item);
    QCOMPARE(edit.collection(), Akonadi::Collection(42));
    QCOMPARE(edit.note()->encodedContent(), message->encodedContent());
}

void NoteEditDialogTest::shouldHaveFilledText()
{
    NoteEditDialog edit;

    Akonadi::NoteUtils::NoteMessageWrapper note;
    QString title = QStringLiteral("title");
    QString text = QStringLiteral("text");
    note.setTitle(title);
    note.setText(text);
    Akonadi::Item item;
    item.setMimeType(Akonadi::NoteUtils::noteMimeType());
    item.setPayload(note.message());

    edit.load(item);
    auto notetitle = edit.findChild<QLineEdit *>(QStringLiteral("notetitle"));
    auto notetext = edit.findChild<KPIMTextEdit::RichTextEditorWidget *>(QStringLiteral("notetext"));
    QCOMPARE(notetitle->text(), title);
    QCOMPARE(notetext->toPlainText(), text);
}

void NoteEditDialogTest::shouldHaveRichText()
{
    NoteEditDialog edit;

    Akonadi::NoteUtils::NoteMessageWrapper note;
    QString title = QStringLiteral("title");
    QString text = QStringLiteral("text");
    note.setTitle(title);
    note.setText(text, Qt::RichText);
    Akonadi::Item item;
    item.setMimeType(Akonadi::NoteUtils::noteMimeType());
    item.setPayload(note.message());

    edit.load(item);
    auto notetext = edit.findChild<KPIMTextEdit::RichTextEditorWidget *>(QStringLiteral("notetext"));
    QCOMPARE(notetext->toPlainText(), text);
    QVERIFY(notetext->editor()->acceptRichText());
}

void NoteEditDialogTest::shouldDefaultCollectionIsValid()
{
    NoteEditDialog edit;
    auto akonadicombobox = edit.findChild<Akonadi::CollectionComboBox *>(QStringLiteral("akonadicombobox"));
    QVERIFY(akonadicombobox);
    QVERIFY(akonadicombobox->currentCollection().isValid());
}

void NoteEditDialogTest::shouldEmitCollectionChangedWhenCurrentCollectionWasChanged()
{
    NoteEditDialog edit;
    auto akonadicombobox = edit.findChild<Akonadi::CollectionComboBox *>(QStringLiteral("akonadicombobox"));
    akonadicombobox->setCurrentIndex(0);
    QCOMPARE(akonadicombobox->currentIndex(), 0);
    QSignalSpy spy(&edit, &NoteEditDialog::collectionChanged);
    akonadicombobox->setCurrentIndex(3);
    QCOMPARE(akonadicombobox->currentIndex(), 3);
    QCOMPARE(spy.count(), 1);
}

void NoteEditDialogTest::shouldEmitCorrectCollection()
{
    NoteEditDialog edit;
    auto akonadicombobox = edit.findChild<Akonadi::CollectionComboBox *>(QStringLiteral("akonadicombobox"));

    Akonadi::NoteUtils::NoteMessageWrapper note;
    QString title = QStringLiteral("title");
    QString text = QStringLiteral("text");
    note.setTitle(title);
    note.setText(text);
    Akonadi::Item item;
    item.setMimeType(Akonadi::NoteUtils::noteMimeType());
    item.setPayload(note.message());

    edit.load(item);
    akonadicombobox->setCurrentIndex(3);
    Akonadi::Collection col = akonadicombobox->currentCollection();
    QSignalSpy spy(&edit, &NoteEditDialog::createNote);
    auto ok = edit.findChild<QPushButton *>(QStringLiteral("save-button"));
    QTest::mouseClick(ok, Qt::LeftButton);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(1).value<Akonadi::Collection>(), col);
}

void NoteEditDialogTest::shouldNotEmitNoteWhenTitleIsEmpty()
{
    NoteEditDialog edit;
    Akonadi::NoteUtils::NoteMessageWrapper note;
    Akonadi::Item item;
    item.setMimeType(Akonadi::NoteUtils::noteMimeType());
    item.setPayload(note.message());

    edit.load(item);
    QSignalSpy spy(&edit, &NoteEditDialog::createNote);
    auto ok = edit.findChild<QPushButton *>(QStringLiteral("save-button"));

    auto notetitle = edit.findChild<QLineEdit *>(QStringLiteral("notetitle"));
    notetitle->setText(QString());
    QTest::mouseClick(ok, Qt::LeftButton);
    QCOMPARE(spy.count(), 0);
    notetitle->setText(QStringLiteral("F"));
    QTest::mouseClick(ok, Qt::LeftButton);
    QCOMPARE(spy.count(), 1);
}

void NoteEditDialogTest::shouldNotEmitNoteWhenTextIsEmpty()
{
    NoteEditDialog edit;
    Akonadi::NoteUtils::NoteMessageWrapper note;
    Akonadi::Item item;
    item.setMimeType(Akonadi::NoteUtils::noteMimeType());
    item.setPayload(note.message());

    edit.load(item);
    QSignalSpy spy(&edit, &NoteEditDialog::createNote);
    auto ok = edit.findChild<QPushButton *>(QStringLiteral("save-button"));

    // Need to set title to empty, 'cause NoteUtils uses default title: "New Note"
    auto notetitle = edit.findChild<QLineEdit *>(QStringLiteral("notetitle"));
    notetitle->setText(QString());

    QTest::mouseClick(ok, Qt::LeftButton);
    QCOMPARE(spy.count(), 0);
    auto notetext = edit.findChild<KPIMTextEdit::RichTextEditorWidget *>(QStringLiteral("notetext"));
    notetext->editor()->setText(QStringLiteral("F"));
    QTest::mouseClick(ok, Qt::LeftButton);
    QCOMPARE(spy.count(), 1);
}

void NoteEditDialogTest::shouldNoteHasCorrectText()
{
    NoteEditDialog edit;
    Akonadi::NoteUtils::NoteMessageWrapper note;
    QString text(QStringLiteral("text"));
    note.setText(text);
    Akonadi::Item item;
    item.setMimeType(Akonadi::NoteUtils::noteMimeType());
    item.setPayload(note.message());

    edit.load(item);
    QSignalSpy spy(&edit, &NoteEditDialog::createNote);
    auto ok = edit.findChild<QPushButton *>(QStringLiteral("save-button"));

    QTest::mouseClick(ok, Qt::LeftButton);
    QCOMPARE(spy.count(), 1);
    Akonadi::NoteUtils::NoteMessageWrapper rNote(spy.at(0).at(0).value<Akonadi::Item>().payload<KMime::Message::Ptr>());
    QCOMPARE(rNote.text(), text);
    auto notetext = edit.findChild<KPIMTextEdit::RichTextEditorWidget *>(QStringLiteral("notetext"));
    QString text2 = QStringLiteral("F");
    notetext->editor()->setText(text2);
    QTest::mouseClick(ok, Qt::LeftButton);
    QCOMPARE(spy.count(), 2);
    Akonadi::NoteUtils::NoteMessageWrapper r2Note(spy.at(1).at(0).value<Akonadi::Item>().payload<KMime::Message::Ptr>());
    QCOMPARE(r2Note.text(), text2);
}

void NoteEditDialogTest::shouldNoteHasCorrectTitle()
{
    NoteEditDialog edit;
    Akonadi::NoteUtils::NoteMessageWrapper note;
    QString text(QStringLiteral("text"));
    note.setTitle(text);
    Akonadi::Item item;
    item.setMimeType(Akonadi::NoteUtils::noteMimeType());
    item.setPayload(note.message());

    edit.load(item);
    QSignalSpy spy(&edit, &NoteEditDialog::createNote);
    auto ok = edit.findChild<QPushButton *>(QStringLiteral("save-button"));

    QTest::mouseClick(ok, Qt::LeftButton);
    QCOMPARE(spy.count(), 1);
    Akonadi::NoteUtils::NoteMessageWrapper rNote(spy.at(0).at(0).value<Akonadi::Item>().payload<KMime::Message::Ptr>());
    QCOMPARE(rNote.title(), text);
    auto notetitle = edit.findChild<QLineEdit *>(QStringLiteral("notetitle"));
    QString text2 = QStringLiteral("F");
    notetitle->setText(text2);
    QTest::mouseClick(ok, Qt::LeftButton);
    QCOMPARE(spy.count(), 2);
    Akonadi::NoteUtils::NoteMessageWrapper r2Note(spy.at(1).at(0).value<Akonadi::Item>().payload<KMime::Message::Ptr>());
    QCOMPARE(r2Note.title(), text2);
}

void NoteEditDialogTest::shouldNoteHasCorrectTextFormat()
{
    NoteEditDialog edit;
    Akonadi::NoteUtils::NoteMessageWrapper note;
    QString text(QStringLiteral("text"));
    note.setText(text);
    Akonadi::Item item;
    item.setMimeType(Akonadi::NoteUtils::noteMimeType());
    item.setPayload(note.message());

    edit.load(item);
    QSignalSpy spy(&edit, &NoteEditDialog::createNote);
    auto ok = edit.findChild<QPushButton *>(QStringLiteral("save-button"));

    QTest::mouseClick(ok, Qt::LeftButton);
    QCOMPARE(spy.count(), 1);
    Akonadi::NoteUtils::NoteMessageWrapper rNote(spy.at(0).at(0).value<Akonadi::Item>().payload<KMime::Message::Ptr>());
    QCOMPARE(rNote.textFormat(), Qt::PlainText);
    auto notetext = edit.findChild<KPIMTextEdit::RichTextEditorWidget *>(QStringLiteral("notetext"));
    notetext->editor()->setAcceptRichText(true);
    QTest::mouseClick(ok, Qt::LeftButton);
    QCOMPARE(spy.count(), 2);
    Akonadi::NoteUtils::NoteMessageWrapper r2Note(spy.at(1).at(0).value<Akonadi::Item>().payload<KMime::Message::Ptr>());
    QCOMPARE(r2Note.textFormat(), Qt::RichText);
}

void NoteEditDialogTest::shouldShouldEnabledSaveEditorButton()
{
    NoteEditDialog edit;
    Akonadi::NoteUtils::NoteMessageWrapper note;
    QString text(QStringLiteral("text"));
    note.setTitle(text);
    Akonadi::Item item;
    item.setMimeType(Akonadi::NoteUtils::noteMimeType());
    item.setPayload(note.message());

    edit.load(item);

    auto ok = edit.findChild<QPushButton *>(QStringLiteral("save-button"));
    auto notetitle = edit.findChild<QLineEdit *>(QStringLiteral("notetitle"));

    QCOMPARE(ok->isEnabled(), true);
    notetitle->clear();

    QCOMPARE(ok->isEnabled(), false);
}

QTEST_MAIN(NoteEditDialogTest)
