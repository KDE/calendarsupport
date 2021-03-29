/*
  This file is part of KOrganizer.

  SPDX-FileCopyrightText: 2013 Sérgio Martins <iamsergio@gmail.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#pragma once

#include "calendarsupport_export.h"

#include <KMessageWidget>

namespace CalendarSupport
{
/**
 * Even less instrusive message dialog.
 * This one goes away when you click somewhere, doesn't need a close button.
 */
class CALENDARSUPPORT_EXPORT MessageWidget : public KMessageWidget
{
    Q_OBJECT
public:
    explicit MessageWidget(QWidget *parent = nullptr);
    ~MessageWidget() override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
};
}

