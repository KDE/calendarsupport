/*
  Copyright (c) 2005 Rafal Rzepecki <divide@users.sourceforge.net>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/
#ifndef CALENDARSUPPORT_CATEGORYHIERARCHYREADER_H
#define CALENDARSUPPORT_CATEGORYHIERARCHYREADER_H

#include "calendarsupport_export.h"

#include <QVariant>

class KComboBox;

class QTreeWidget;
class QTreeWidgetItem;

namespace CalendarSupport {
class CALENDARSUPPORT_EXPORT CategoryHierarchyReader
{
public:
    void read(const QStringList &categories);
    virtual ~CategoryHierarchyReader()
    {
    }

    static QStringList path(QString string);

protected:
    CategoryHierarchyReader()
    {
    }

    virtual void clear() = 0;
    virtual void goUp() = 0;
    virtual void addChild(const QString &label, const QVariant &userData = QVariant()) = 0;
    virtual int depth() const = 0;
};

class CALENDARSUPPORT_EXPORT CategoryHierarchyReaderQComboBox : public CategoryHierarchyReader
{
public:
    explicit CategoryHierarchyReaderQComboBox(KComboBox *box) : mBox(box)
    {
    }

    ~CategoryHierarchyReaderQComboBox() override
    {
    }

protected:
    void clear() override;
    void goUp() override;
    void addChild(const QString &label, const QVariant &userData = QVariant()) override;
    int depth() const override;

private:
    KComboBox *mBox = nullptr;
    int mCurrentDepth = 0;
};

#ifndef QT_NO_TREEWIDGET
class CALENDARSUPPORT_EXPORT CategoryHierarchyReaderQTreeWidget : public CategoryHierarchyReader
{
public:
    explicit CategoryHierarchyReaderQTreeWidget(QTreeWidget *tree)
        : mTree(tree)
        , mItem(nullptr)
        , mCurrentDepth(0)
    {
    }

    ~CategoryHierarchyReaderQTreeWidget() override
    {
    }

protected:
    void clear() override;
    void goUp() override;
    void addChild(const QString &label, const QVariant &userData = QVariant()) override;
    int depth() const override;

private:
    QTreeWidget *mTree = nullptr;
    QTreeWidgetItem *mItem = nullptr;
    int mCurrentDepth = 0;
};
#endif
}

#endif
