/*
  SPDX-FileCopyrightText: 1998 Preston Brown <pbrown@kde.org>
  SPDX-FileCopyrightText: 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/
#pragma once

#include "calendarsupport_export.h"
#include "printplugin.h"

#include <QComboBox>
#include <QDialog>
#include <QPushButton>
class QButtonGroup;
class QStackedWidget;

namespace CalendarSupport
{
/**
  CalPrinter is a class for printing Calendars.  It can print in several
  different formats (day, week, month).  It also provides a way for setting
  up the printer and remembering these preferences.
*/
class CALENDARSUPPORT_EXPORT CalPrinter : public QObject, public CalPrinterBase
{
    Q_OBJECT
public:
    enum ePrintOrientation { eOrientPlugin = 0, eOrientPrinter, eOrientPortrait, eOrientLandscape };

public:
    /**
      \param par parent widget for dialogs
      \param cal calendar to be printed
      \param uniqItem if true, indicates the calendar print dialog will only
      provide the option to print an single incidence; else, all possible types
      of print types will be shown
    */
    CalPrinter(QWidget *par, const Akonadi::ETMCalendar::Ptr &calendar, bool uniqItem = false);

    ~CalPrinter() override;

    void init(const Akonadi::ETMCalendar::Ptr &calendar);

    /**
      Set date range to be printed.

      \param start Start date
      \param end   End date
    */
    void setDateRange(QDate start, QDate end);

public Q_SLOTS:
    void updateConfig();

private Q_SLOTS:
    void doPrint(CalendarSupport::PrintPlugin *selectedStyle, CalendarSupport::CalPrinter::ePrintOrientation dlgorientation, bool preview = false);

public:
    void print(int type, QDate fd, QDate td, const KCalendarCore::Incidence::List &selectedIncidences = KCalendarCore::Incidence::List(), bool preview = false);
    Akonadi::ETMCalendar::Ptr calendar() const;
    KConfig *config() const;

protected:
    PrintPlugin::List mPrintPlugins;

private:
    Akonadi::ETMCalendar::Ptr mCalendar;
    QWidget *mParent = nullptr;
    KConfig *mConfig = nullptr;
    const bool mUniqItem;
};

class CalPrintDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CalPrintDialog(int initialPrintType, const PrintPlugin::List &plugins, QWidget *parent = nullptr, bool mUniqItem = false);

    ~CalPrintDialog() override;

    PrintPlugin *selectedPlugin();
    void setOrientation(CalPrinter::ePrintOrientation orientation);
    CalPrinter::ePrintOrientation orientation() const;

public Q_SLOTS:
    void setPrintType(QAbstractButton *button);
    void setPreview(bool);

private:
    void slotOk();
    void changePrintType(int id);
    QButtonGroup *mTypeGroup = nullptr;
    QStackedWidget *mConfigArea = nullptr;
    QMap<int, PrintPlugin *> mPluginIDs;
    QString mPreviewText;
    QComboBox *mOrientationSelection = nullptr;
    QPushButton *mOkButton = nullptr;
    CalPrinter::ePrintOrientation mOrientation;
};
}

