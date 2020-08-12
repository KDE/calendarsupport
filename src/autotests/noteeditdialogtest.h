/*
 * SPDX-FileCopyrightText: 2014 Sandro Knauß <knauss@kolabsys.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
 */

#ifndef NOTEEDITDIALOGTEST_H
#define NOTEEDITDIALOGTEST_H

#include <QObject>

namespace CalendarSupport {
class NoteEditDialogTest : public QObject
{
    Q_OBJECT
public:
    NoteEditDialogTest();

private Q_SLOTS:
    void shouldHaveDefaultValuesOnCreation();
    void shouldEmitCollectionChanged();
    void shouldNotEmitWhenCollectionIsNotChanged();
    void shouldHaveSameValueAfterSet();
    void shouldHaveFilledText();
    void shouldHaveRichText();
    void shouldDefaultCollectionIsValid();
    void shouldEmitCollectionChangedWhenCurrentCollectionWasChanged();
    void shouldEmitCorrectCollection();

    void shouldNotEmitNoteWhenTitleIsEmpty();
    void shouldNotEmitNoteWhenTextIsEmpty();

    void shouldNoteHasCorrectText();
    void shouldNoteHasCorrectTitle();
    void shouldNoteHasCorrectTextFormat();

    void shouldShouldEnabledSaveEditorButton();
};
}

#endif
