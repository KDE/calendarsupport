/*
  This file is part of libkdepim.

  SPDX-FileCopyrightText: 2004 Bram Schoenmakers <bramschoenmakers@kde.nl>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kdatepickerpopup.h"

#include <KDatePicker>
#include <KLocalizedString>

#include <QDateTime>
#include <QWidgetAction>

using namespace CalendarSupport;

class KDatePickerAction : public QWidgetAction
{
public:
    KDatePickerAction(KDatePicker *widget, QObject *parent)
        : QWidgetAction(parent)
        , mDatePicker(widget)
        , mOriginalParent(widget->parentWidget())
    {
    }

protected:
    QWidget *createWidget(QWidget *parent) override
    {
        mDatePicker->setParent(parent);
        return mDatePicker;
    }

    void deleteWidget(QWidget *widget) override
    {
        if (widget != mDatePicker) {
            return;
        }

        mDatePicker->setParent(mOriginalParent);
    }

private:
    KDatePicker *const mDatePicker;
    QWidget *const mOriginalParent;
};

class CalendarSupport::KDatePickerPopupPrivate
{
public:
    explicit KDatePickerPopupPrivate(KDatePickerPopup *qq)
        : q(qq)
    {
    }

    void buildMenu();

    void slotDateChanged(QDate);
    void slotToday();
    void slotTomorrow();
    void slotNextWeek();
    void slotNextMonth();
    void slotNoDate();

    KDatePickerPopup *const q;
    KDatePicker *mDatePicker = nullptr;
    KDatePickerPopup::Modes mModes;
};

void KDatePickerPopupPrivate::buildMenu()
{
    if (q->isVisible()) {
        return;
    }

    q->clear();

    if (mModes & KDatePickerPopup::DatePicker) {
        q->addAction(new KDatePickerAction(mDatePicker, q));

        if ((mModes & KDatePickerPopup::NoDate) || (mModes & KDatePickerPopup::Words)) {
            q->addSeparator();
        }
    }

    if (mModes & KDatePickerPopup::Words) {
        q->addAction(i18nc("@option today", "&Today"), q, [this]() {
            slotToday();
        });
        q->addAction(i18nc("@option tomorrow", "To&morrow"), q, [this]() {
            slotTomorrow();
        });
        q->addAction(i18nc("@option next week", "Next &Week"), q, [this]() {
            slotNextWeek();
        });
        q->addAction(i18nc("@option next month", "Next M&onth"), q, [this]() {
            slotNextMonth();
        });

        if (mModes & KDatePickerPopup::NoDate) {
            q->addSeparator();
        }
    }

    if (mModes & KDatePickerPopup::NoDate) {
        q->addAction(i18nc("@option do not specify a date", "No Date"), q, [this]() {
            slotNoDate();
        });
    }
}

void KDatePickerPopupPrivate::slotDateChanged(QDate date)
{
    Q_EMIT q->dateChanged(date);
    q->hide();
}

void KDatePickerPopupPrivate::slotToday()
{
    Q_EMIT q->dateChanged(QDate::currentDate());
}

void KDatePickerPopupPrivate::slotTomorrow()
{
    Q_EMIT q->dateChanged(QDate::currentDate().addDays(1));
}

void KDatePickerPopupPrivate::slotNoDate()
{
    Q_EMIT q->dateChanged(QDate());
}

void KDatePickerPopupPrivate::slotNextWeek()
{
    Q_EMIT q->dateChanged(QDate::currentDate().addDays(7));
}

void KDatePickerPopupPrivate::slotNextMonth()
{
    Q_EMIT q->dateChanged(QDate::currentDate().addMonths(1));
}

KDatePickerPopup::KDatePickerPopup(Modes modes, QDate date, QWidget *parent)
    : QMenu(parent)
    , d(new KDatePickerPopupPrivate(this))
{
    d->mModes = modes;

    d->mDatePicker = new KDatePicker(this);
    d->mDatePicker->setCloseButton(false);

    connect(d->mDatePicker, &KDatePicker::dateEntered, this, [this](QDate date) {
        d->slotDateChanged(date);
    });
    connect(d->mDatePicker, &KDatePicker::dateSelected, this, [this](QDate date) {
        d->slotDateChanged(date);
    });

    d->mDatePicker->setDate(date);

    d->buildMenu();
}

KDatePickerPopup::~KDatePickerPopup() = default;

KDatePicker *KDatePickerPopup::datePicker() const
{
    return d->mDatePicker;
}

void KDatePickerPopup::setDate(QDate date)
{
    d->mDatePicker->setDate(date);
}

#include "moc_kdatepickerpopup.cpp"
