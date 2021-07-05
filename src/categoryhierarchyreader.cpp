/*
  SPDX-FileCopyrightText: 2005 Rafal Rzepecki <divide@users.sourceforge.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "categoryhierarchyreader.h"
#include "categoryconfig.h"

#include <QComboBox>

#include <QTreeWidget>

using namespace CalendarSupport;

inline QString &quote(QString &string)
{
    Q_ASSERT(CategoryConfig::categorySeparator != QLatin1String("@"));
    return string.replace(QLatin1Char('@'), QStringLiteral("@0")).replace(QLatin1Char('\\') + CategoryConfig::categorySeparator, QStringLiteral("@1"));
}

inline QStringList &unquote(QStringList &strings)
{
    return strings.replaceInStrings(QStringLiteral("@1"), CategoryConfig::categorySeparator).replaceInStrings(QStringLiteral("@0"), QStringLiteral("@"));
}

QStringList CategoryHierarchyReader::path(QString string)
{
    QStringList _path = quote(string).split(CategoryConfig::categorySeparator, Qt::SkipEmptyParts);
    return unquote(_path);
}

void CategoryHierarchyReader::read(const QStringList &categories)
{
    clear();

    // case insensitive sort
    QMap<QString, QString> sortedCategories;
    for (const QString &str : categories) {
        sortedCategories.insert(str.toLower(), str);
    }

    QStringList last_path;
    for (const QString &category : std::as_const(sortedCategories)) {
        QStringList _path = path(category);

        // we need to figure out where last item and the new one differ
        QStringList::Iterator jt, kt;
        int split_level = 0;
        QStringList new_path = _path; // save it for later
        for (jt = _path.begin(), kt = last_path.begin(); jt != _path.end() && kt != last_path.end(); ++jt, ++kt) {
            if (*jt == *kt) {
                split_level++;
            } else {
                break; // now we have first non_equal component in the iterators
            }
        }

        // make a path relative to the shared ancestor
        if (jt != _path.begin()) {
            _path.erase(_path.begin(), jt);
        }
        last_path = new_path;

        if (_path.isEmpty()) {
            // something is wrong, we already have this node
            continue;
        }

        // find that ancestor
        while (split_level < depth()) {
            goUp();
        }
        Q_ASSERT(split_level == depth());

        // make the node and any non-existent ancestors
        while (!_path.isEmpty()) {
            addChild(_path.first(), QVariant(category));
            _path.pop_front();
        }
    }
}

void CategoryHierarchyReaderQComboBox::clear()
{
    mBox->clear();
}

void CategoryHierarchyReaderQComboBox::goUp()
{
    mCurrentDepth--;
}

void CategoryHierarchyReaderQComboBox::addChild(const QString &label, const QVariant &userData)
{
    QString spaces;
    spaces.fill(QLatin1Char(' '), 2 * mCurrentDepth);
    mBox->addItem(spaces + label, userData);
    mCurrentDepth++;
}

int CategoryHierarchyReaderQComboBox::depth() const
{
    return mCurrentDepth;
}

#ifndef QT_NO_TREEWIDGET

void CategoryHierarchyReaderQTreeWidget::clear()
{
    mTree->clear();
}

void CategoryHierarchyReaderQTreeWidget::goUp()
{
    Q_ASSERT(mItem);
    mItem = mItem->parent();
    --mCurrentDepth;
}

void CategoryHierarchyReaderQTreeWidget::addChild(const QString &label, const QVariant &userData)
{
    Q_UNUSED(userData)

    if (mItem) {
        mItem = new QTreeWidgetItem(mItem, QStringList() << label);
    } else {
        mItem = new QTreeWidgetItem(mTree, QStringList() << label);
    }

    mItem->setExpanded(true);
    ++mCurrentDepth;
}

int CategoryHierarchyReaderQTreeWidget::depth() const
{
    return mCurrentDepth;
}

#endif
