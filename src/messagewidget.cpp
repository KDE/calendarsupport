/*
  This file is part of KOrganizer.

  SPDX-FileCopyrightText: 2013 Sérgio Martins <iamsergio@gmail.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#include "messagewidget.h"

#include <QApplication>
#include <QKeyEvent>

using namespace CalendarSupport;

MessageWidget::MessageWidget(QWidget *parent)
    : KMessageWidget(parent)
{
    hide();
    setCloseButtonVisible(false);
    setWordWrap(true);
}

MessageWidget::~MessageWidget()
{
}

void MessageWidget::showEvent(QShowEvent *event)
{
    qApp->installEventFilter(this);
    KMessageWidget::showEvent(event);
}

void MessageWidget::hideEvent(QHideEvent *event)
{
    // No need to spend cycles on an event-filter when this is going to
    // me hidden most of the time
    qApp->removeEventFilter(this);
    KMessageWidget::hideEvent(event);
}

bool MessageWidget::eventFilter(QObject *, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        hide();
    }

    if (event->type() == QEvent::KeyPress) {
        auto ev = static_cast<QKeyEvent *>(event);
        hide();
        if (ev->key() == Qt::Key_Escape) {
            return true; // We eat this one, it's for us
        }
    }

    return false; // we don't want it
}
