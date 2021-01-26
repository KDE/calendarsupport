/*
  SPDX-FileCopyrightText: 2002 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2005 Rafal Rzepecki <divide@users.sourceforge.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "categoryconfig.h"

#include <KConfigGroup>
#include <KCoreConfigSkeleton>
#include <KLocalizedString>
#include <QColor>

using namespace CalendarSupport;

static QStringList categoryDefaults()
{
    QStringList l;
    l << i18nc("incidence category: appointment", "Appointment") << i18nc("incidence category", "Business") << i18nc("incidence category", "Meeting")
      << i18nc("incidence category: phone call", "Phone Call") << i18nc("incidence category", "Education")
      << i18nc(
             "incidence category: "
             "official or unofficial observance of "
             "religious/national/cultural/other significance, "
             "often accompanied by celebrations or festivities",
             "Holiday")
      << i18nc(
             "incidence category: "
             "a lengthy time away from work or school, a trip abroad, "
             "or simply a pleasure trip away from home",
             "Vacation")
      << i18nc(
             "incidence category: "
             "examples: anniversary of historical or personal event; "
             "big date; remembrance, etc",
             "Special Occasion")
      << i18nc("incidence category", "Personal")
      << i18nc(
             "incidence category: "
             "typically associated with leaving home for business, "
             "and not pleasure",
             "Travel")
      << i18nc("incidence category", "Miscellaneous") << i18nc("incidence category", "Birthday");
    return l;
}

class Q_DECL_HIDDEN CategoryConfig::Private
{
public:
    explicit Private(KCoreConfigSkeleton *cfg)
        : config(cfg)
    {
        mDefaultCategoryColor = QColor(151, 235, 121);
    }

    QColor mDefaultCategoryColor;
    KCoreConfigSkeleton *const config;
};

QHash<QString, QColor> CategoryConfig::readColors() const
{
    // Category colors
    QHash<QString, QColor> categoryColors;
    KConfigGroup colorsConfig(d->config->config(), "Category Colors2");
    const QStringList cats = customCategories();
    for (const QString &category : cats) {
        const QColor color = colorsConfig.readEntry(category, d->mDefaultCategoryColor);
        if (color != d->mDefaultCategoryColor) {
            categoryColors.insert(category, color);
        }
    }

    return categoryColors;
}

void CategoryConfig::setColors(const QHash<QString, QColor> &colors)
{
    KConfigGroup colorsConfig(d->config->config(), "Category Colors2");
    QHash<QString, QColor>::const_iterator i = colors.constBegin();
    QHash<QString, QColor>::const_iterator end = colors.constEnd();
    while (i != end) {
        colorsConfig.writeEntry(i.key(), i.value());
        ++i;
    }
}

CategoryConfig::CategoryConfig(KCoreConfigSkeleton *cfg, QObject *parent)
    : QObject(parent)
    , d(new Private(cfg))
{
}

CategoryConfig::~CategoryConfig()
{
    delete d;
}

void CategoryConfig::writeConfig()
{
    d->config->save();
}

QStringList CategoryConfig::customCategories() const
{
    KConfigGroup group(d->config->config(), "General");
    QStringList cats = group.readEntry("Custom Categories", categoryDefaults());
    cats.sort();
    return cats;
}

void CategoryConfig::setCustomCategories(const QStringList &categories)
{
    KConfigGroup group(d->config->config(), "General");
    group.writeEntry("Custom Categories", categories);
}

const QString CategoryConfig::categorySeparator(QLatin1Char(':'));
