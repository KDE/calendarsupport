/*
  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company
  SPDX-FileContributor: Tobias Koenig <tokoe@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QTextBrowser>

namespace CalendarSupport
{
class TextBrowser : public QTextBrowser
{
    Q_OBJECT

public:
    explicit TextBrowser(QWidget *parent = nullptr);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    void setSource(const QUrl &name) override;
#else
    void doSetSource(const QUrl &name, QTextDocument::ResourceType type = QTextDocument::UnknownResource) override;
#endif

Q_SIGNALS:
    void attachmentUrlClicked(const QString &uri);
};
}

