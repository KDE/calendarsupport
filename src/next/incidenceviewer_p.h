/*
  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company
  SPDX-FileContributor: Tobias Koenig <tokoe@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CALENDARSUPPORT_INCIDENCEVIEWER_P_H
#define CALENDARSUPPORT_INCIDENCEVIEWER_P_H

#include <QTextBrowser>

namespace CalendarSupport
{
class TextBrowser : public QTextBrowser
{
    Q_OBJECT

public:
    explicit TextBrowser(QWidget *parent = nullptr);

    void setSource(const QUrl &name) override;

Q_SIGNALS:
    void attachmentUrlClicked(const QString &uri);
};
}

#endif
