/*
 * SPDX-FileCopyrightText: 2014 Sandro Knauß <knauss@kolabsys.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
 */

#pragma once

#include "calendarsupport_export.h"

#include <KMime/KMimeMessage>

#include <AkonadiCore/Collection>
#include <AkonadiCore/Item>

#include <QDialog>

class QAbstractItemModel;
class QLineEdit;

namespace KPIMTextEdit
{
class RichTextEditorWidget;
}

namespace Akonadi
{
class CollectionComboBox;
}

namespace CalendarSupport
{
class CALENDARSUPPORT_EXPORT NoteEditDialog : public QDialog
{
    Q_OBJECT
public:
    explicit NoteEditDialog(QWidget *parent = nullptr);
    ~NoteEditDialog() override;

    Q_REQUIRED_RESULT Akonadi::Collection collection() const;
    void setCollection(const Akonadi::Collection &value);

    void load(const Akonadi::Item &item);
    Q_REQUIRED_RESULT KMime::Message::Ptr note() const;

    // Used for tests
    static QAbstractItemModel *_k_noteEditStubModel;

public Q_SLOTS:
    void accept() override;

private Q_SLOTS:
    void slotCollectionChanged(int);
    void slotUpdateButtons();

Q_SIGNALS:
    void createNote(const Akonadi::Item &note, const Akonadi::Collection &collection);
    void collectionChanged(const Akonadi::Collection &col);

private:
    void readConfig();
    void writeConfig();
    Akonadi::Collection mCollection;
    Akonadi::Item mItem;
    QLineEdit *mNoteTitle = nullptr;
    QPushButton *mOkButton = nullptr;
    KPIMTextEdit::RichTextEditorWidget *mNoteText = nullptr;
    Akonadi::CollectionComboBox *mCollectionCombobox = nullptr;
};
}

