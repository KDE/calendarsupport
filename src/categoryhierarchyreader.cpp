/*
  SPDX-FileCopyrightText: 2005 Rafal Rzepecki <divide@users.sourceforge.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "categoryhierarchyreader.h"
using namespace Qt::Literals::StringLiterals;

#include <QComboBox>

using namespace CalendarSupport;

namespace CategoryConfig
{
static const QLatin1StringView categorySeparator(":");
}

static inline QString &quote(QString &string)
{
    Q_ASSERT(CategoryConfig::categorySeparator != "@"_L1);
    return string.replace(u'@', u"@0"_s).replace(u'\\' + CategoryConfig::categorySeparator, u"@1"_s);
}

static inline QStringList &unquote(QStringList &strings)
{
    return strings.replaceInStrings(u"@1"_s, CategoryConfig::categorySeparator).replaceInStrings(u"@0"_s, QStringLiteral("@"));
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
        QStringList::Iterator jt;
        QStringList::Iterator kt;
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
    spaces.fill(u' ', 2 * mCurrentDepth);
    mBox->addItem(spaces + label, userData);
    mCurrentDepth++;
}

int CategoryHierarchyReaderQComboBox::depth() const
{
    return mCurrentDepth;
}
