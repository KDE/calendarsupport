/*
 * SPDX-FileCopyrightText: 2014 Sandro Knauß <knauss@kolabsys.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
 */

#pragma once

#include "calendarsupport_export.h"

#include <KMime/Message>

#include <Akonadi/Collection>
#include <Akonadi/Item>

#include <QDialog>

class QAbstractItemModel;
class QLineEdit;

namespace TextCustomEditor
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

    [[nodiscard]] Akonadi::Collection collection() const;
    void setCollection(const Akonadi::Collection &value);

    void load(const Akonadi::Item &item);
    [[nodiscard]] KMime::Message::Ptr note() const;

    // Used for tests
    static QAbstractItemModel *_k_noteEditStubModel;

public Q_SLOTS:
    void accept() override;

private Q_SLOTS:
    CALENDARSUPPORT_NO_EXPORT void slotCollectionChanged(int);
    CALENDARSUPPORT_NO_EXPORT void slotUpdateButtons();

Q_SIGNALS:
    void createNote(const Akonadi::Item &note, const Akonadi::Collection &collection);
    void collectionChanged(const Akonadi::Collection &col);

private:
    CALENDARSUPPORT_NO_EXPORT void readConfig();
    CALENDARSUPPORT_NO_EXPORT void writeConfig();
    Akonadi::Collection mCollection;
    Akonadi::Item mItem;
    QLineEdit *const mNoteTitle;
    QPushButton *mOkButton = nullptr;
    TextCustomEditor::RichTextEditorWidget *mNoteText = nullptr;
    Akonadi::CollectionComboBox *mCollectionCombobox = nullptr;
};
}
