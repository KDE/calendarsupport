/*
  This file is part of KOrganizer.

  SPDX-FileCopyrightText: 2013 Sérgio Martins <iamsergio@gmail.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/

#pragma once

#include "calendarsupport_export.h"

#include <KMessageWidget>

namespace CalendarSupport
{
/*!
 * \class CalendarSupport::MessageWidget
 * \inmodule CalendarSupport
 * \inheaderfile CalendarSupport/MessageWidget
 *
 * Even less instrusive message dialog.
 * This one goes away when you click somewhere, doesn't need a close button.
 */
class CALENDARSUPPORT_EXPORT MessageWidget : public KMessageWidget
{
    Q_OBJECT
public:
    /*!
     * Constructs a MessageWidget.
     * \param parent The parent widget.
     */
    explicit MessageWidget(QWidget *parent = nullptr);
    /*!
     * Destroys the MessageWidget.
     */
    ~MessageWidget() override;
    /*!
     * Handles events for watched objects.
     * \param watched The object being watched.
     * \param event The event to handle.
     * \return true if the event was handled, false otherwise.
     */
    [[nodiscard]] bool eventFilter(QObject *watched, QEvent *event) override;
    /*!
     * Handles the show event for the message widget.
     * \param event The show event.
     */
    void showEvent(QShowEvent *event) override;
    /*!
     * Handles the hide event for the message widget.
     * \param event The hide event.
     */
    void hideEvent(QHideEvent *event) override;
};
}
