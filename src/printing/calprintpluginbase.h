/*
  SPDX-FileCopyrightText: 1998 Preston Brown <pbrown@kde.org>
  SPDX-FileCopyrightText: 2003 Reinhold Kainhofer <reinhold@kainhofer.com>
  SPDX-FileCopyrightText: 2008 Ron Goodheart <rong.dev@gmail.com>
  SPDX-FileCopyrightText: 2012-2013 Allen Winter <winter@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#pragma once

#include "calendarsupport_export.h"
#include "printplugin.h"

#include <Akonadi/Calendar/ETMCalendar>

#include <KCalendarCore/Event>
#include <KCalendarCore/Journal>
#include <KCalendarCore/Todo>

#include <QDateTime>
#include <QPainter>

class PrintCellItem;
class QWidget;

#define PORTRAIT_HEADER_HEIGHT 72 // header height, for portrait orientation
#define LANDSCAPE_HEADER_HEIGHT 54 // header height, for landscape orientation
#define SUBHEADER_HEIGHT 20 // subheader height, for all orientations
#define PORTRAIT_FOOTER_HEIGHT 16 // footer height, for portrait orientation
#define LANDSCAPE_FOOTER_HEIGHT 14 // footer height, for landscape orientation
#define MARGIN_SIZE 36 // margins, for all orientations
#define PADDING_SIZE 7 // padding between the various top-level boxes
#define BOX_BORDER_WIDTH 2 // width of the border of all top-level boxes
#define EVENT_BORDER_WIDTH 0 // with of the border of all incidence boxes

#define TIMELINE_WIDTH 50 // width of timeline (day and timetable)

namespace CalendarSupport
{
/**
  Base class for Calendar printing classes. Each sub class represents one
  calendar print format.
*/
class CalPrintPluginBase : public PrintPlugin
{
public:
    enum DisplayFlags { Text = 0x0001, TimeBoxes = 0x0002 };

public:
    /**
      Constructor
    */
    CalPrintPluginBase();
    ~CalPrintPluginBase() override;

    /**
      Returns widget for configuring the print format.
    */
    QWidget *createConfigWidget(QWidget *) override;

    /**
      Actually do the printing.

      @param p QPainter the print result is painted to
      @param width Width of printable area
      @param height Height of printable area
    */
    virtual void print(QPainter &p, int width, int height) = 0;
    /**
      Start printing.
    */
    void doPrint(QPrinter *printer) override;

    /**
      Load print format configuration from config file.
    */
    virtual void loadConfig() = 0;
    /**
      Write print format configuration to config file.
    */
    virtual void saveConfig() = 0;

    /**
      Load complete config. This also calls loadConfig() of the derived class.
    */
    void doLoadConfig() override;
    /**
      Save complete config. This also calls saveConfig() of the derived class.
    */
    void doSaveConfig() override;

    /** HELPER FUNCTIONS */
public:
    bool useColors() const;
    void setUseColors(bool useColors);

    bool printFooter() const;
    void setPrintFooter(bool printFooter);

    /**
      Determines the column of the given weekday ( 1=Monday, 7=Sunday ), taking the
      start of the week setting into account as given in kcontrol.
      @param weekday Index of the weekday
    */
    static int weekdayColumn(int weekday);

    QPageLayout::Orientation orientation() const;

    /** Returns the height of the page header. If the height was explicitly
        set using setHeaderHeight, that value is returned, otherwise a
        default value based on the printer orientation.
        @return height of the page header of the printout
    */
    int headerHeight() const;
    void setHeaderHeight(const int height);

    int subHeaderHeight() const;
    void setSubHeaderHeight(const int height);
    /** Returns the height of the page footer. If the height was explicitly
        set using setFooterHeight, that value is returned, otherwise a
        default value based on the printer orientation.
        @return height of the page footer of the printout
    */
    int footerHeight() const;
    void setFooterHeight(const int height);

    int margin() const;
    void setMargin(const int margin);

    int padding() const;
    void setPadding(const int margin);

    int borderWidth() const;
    void setBorderWidth(const int border);

    /*****************************************************************
     **               PRINTING HELPER FUNCTIONS                     **
     *****************************************************************/
public:
    /**
      Draw a box with given width at the given coordinates.
      @param p The printer to be used
      @param linewidth The border width of the box
      @param rect The rectangle of the box
    */
    static void drawBox(QPainter &p, int linewidth, QRect rect);
    /**
      Draw a shaded box with given width at the given coordinates.
      @param p The printer to be used
      @param linewidth The border width of the box
      @param brush The brush to fill the box
      @param rect The rectangle of the box
    */
    static void drawShadedBox(QPainter &p, int linewidth, const QBrush &brush, QRect rect);

    /**
      Print the given string (event summary) in the given rectangle. Margins
      and justification (centered or not) are automatically adjusted.
      @param p QPainter of the printout
      @param box Coordinates of the surrounding event box
      @param str The text to be printed in the box
    */
    void printEventString(QPainter &p, QRect box, const QString &str, int flags = -1);

    /**
      Print the box for the given event with the given string.
      @param p QPainter of the printout
      @param linewidth is the width of the line used to draw the box, ignored if less than 1.
      @param box Coordinates of the event's box
      @param incidence The incidence (if available), from which the category
                       color will be deduced, if applicable.
      @param str The string to print inside the box
      @param flags is a bitwise OR of Qt::AlignmentFlags and Qt::TextFlags values.
    */
    void showEventBox(QPainter &p, int linewidth, QRect box, const KCalendarCore::Incidence::Ptr &incidence, const QString &str, int flags = -1);

    /**
      Draw a subheader box with a shaded background and the given string
      @param p QPainter of the printout
      @param str Text to be printed inside the box
      @param box Coordinates of the box
    */
    void drawSubHeaderBox(QPainter &p, const QString &str, QRect box);

    /**
      Draw an event box with vertical text.
      @param p QPainter of the printout
      @param linewidth is the width of the line used to draw the box, ignored if less than 1.
      @param box Coordinates of the box
      @param str ext to be printed inside the box
      @param flags is a bitwise OR of Qt::AlignmentFlags and Qt::TextFlags values.
    */
    void drawVerticalBox(QPainter &p, int linewidth, QRect box, const QString &str, int flags = -1);

    /**
      Draw a component box with a heading (printed in bold).
      @param p QPainter of the printout
      @param box Coordinates of the box
      @param caption Caption string to be printed inside the box
      @param contents Normal text contents of the box. If contents.isNull(),
                      then no text will be printed, only the caption.
      @param sameLine Whether the contents should start on the same line as
                      the caption (the space below the caption text will be
                      used as indentation in the subsequent lines) or on the
                      next line (no indentation of the contents)
      @param expand Whether to expand the box vertically to fit the
                    whole text in it.
      @param rickContents Whether contents contains rich text.
      @return The bottom of the printed box. If expand==true, the bottom of
              the drawn box is returned, if expand==false, the vertical
              end of the printed contents inside the box is returned.
              If you want to print some custom graphics or text below
              the contents, use the return value as the top-value of your
              custom contents in that case.
    */
    int drawBoxWithCaption(QPainter &p,
                           QRect box,
                           const QString &caption,
                           const QString &contents,
                           bool sameLine,
                           bool expand,
                           const QFont &captionFont,
                           const QFont &textFont,
                           bool richContents = false);

    /**
      Draw the gray header bar of the printout to the QPainter.
      It prints the given text and optionally one or two small
      month views, as specified by the two QDate. The printed
      text can also contain a line feed.
      If month2 is invalid, only the month that contains month1
      is printed.
      E.g. the filofax week view draws just the current month,
      while the month view draws the previous and the next month.
      @param p QPainter of the printout
      @param title The string printed as the title of the page
                   (e.g. the date, date range or todo list title)
      @param month1 Date specifying the month for the left one of
                    the small month views in the title bar. If left
                    empty, only month2 will be printed (or none,
                    it that is invalid as well).
      @param month2 Date specifying the month for the right one of
                    the small month views in the title bar. If left
                    empty, only month1 will be printed (or none,
                    it that is invalid as well).
      @param box coordinates of the title bar
      @param expand Whether to expand the box vertically to fit the
                    whole title in it.
      @param backColor background color for the header box.
      @return The bottom of the printed box. If expand==false, this
              is box.bottom, otherwise it is larger than box.bottom
              and matches the y-coordinate of the surrounding rectangle.
    */
    int drawHeader(QPainter &p, const QString &title, QDate month1, QDate month2, QRect box, bool expand = false, QColor backColor = QColor());

    /**
      Draw a page footer containing the printing date and possibly
      other things, like a page number.
      @param p QPainter of the printout
      @param box coordinates of the footer
      @return The bottom of the printed box.
    */
    int drawFooter(QPainter &p, QRect box);

    /**
      Draw a small calendar with the days of a month into the given area.
      Used for example in the title bar of the sheet.
      @param p QPainter of the printout
      @param qd Arbitrary Date within the month to be printed.
      @param box coordinates of the small calendar
    */
    void drawSmallMonth(QPainter &p, QDate qd, QRect box);

    /**
      Draw a horizontal bar with the weekday names of the given date range
      in the given area of the painter.
      This is used for the weekday-bar on top of the timetable view and the month view.
      @param p QPainter of the printout
      @param fromDate First date of the printed dates
      @param toDate Last date of the printed dates
      @param box coordinates of the box for the days of the week
    */
    void drawDaysOfWeek(QPainter &p, const QDate &fromDate, QDate toDate, QRect box);

    /**
      Draw a single weekday name in a box inside the given area of the painter.
      This is called in a loop by drawDaysOfWeek.
      @param p QPainter of the printout
      @param qd Date of the printed day
      @param box coordinates of the weekbox
    */
    void drawDaysOfWeekBox(QPainter &p, QDate qd, QRect box);

    /**
      Draw a (vertical) time scale from time fromTime to toTime inside the
      given area of the painter. Every hour will have a one-pixel line over
      the whole width, every half-hour the line will only span the left half
      of the width. This is used in the day and timetable print styles
      @param p QPainter of the printout
      @param fromTime Start time of the time range to display
      @param toTime End time of the time range to display
      @param box coordinates of the timeline
    */
    void drawTimeLine(QPainter &p, QTime fromTime, QTime toTime, QRect box);

    /**
      Draw the all-day box for the agenda print view (the box on top which
      doesn't have a time on the time scale associated).

      Obeys configuration options #mExcludeConfidential, #excludePrivate.
      @param p QPainter of the printout
      @param eventList The list of all-day events that are supposed to be printed
             inside this box
      @param qd The date of the currently printed day
      @param box coordinates of the all day box.
      @param includeCategories Whether to print the event categories (tags) as well.
      @param workDays List of workDays
    */
    void drawAllDayBox(QPainter &p,
                      const KCalendarCore::Event::List &eventList,
                      QDate qd,
                      QRect box,
                      bool includeCategories,
                      const QList<QDate> &workDays);

    /**
      Draw the agenda box for the day print style (the box showing all events of that day).
      Also draws a grid with half-hour spacing of the grid lines.
      Does NOT draw allday events.  Use drawAllDayBox for allday events.

      Obeys configuration options #mExcludeConfidential, #excludePrivate.
      @param p QPainter of the printout
      @param eventList The list of the events that are supposed to be printed
             inside this box
      @param qd The date of the currently printed day
      @param expandable If true, the start and end times are adjusted to include
             the whole range of all events of that day, not just of the given time range.
             The height of the box will not be affected by this (but the height
             of one hour will be scaled down so that the whole range fits into
             the box. fromTime and toTime receive the actual time range printed
             by this function).
      @param fromTime Start of the time range to be printed. Might be adjusted
                      to include all events if expandable==true
      @param toTime End of the time range to be printed. Might be adjusted
                   to include all events if expandable==true
      @param box coordinates of the agenda day box.
      @param includeDescription Whether to print the event description as well.
      @param includeCategories Whether to print the event categories (tags) as well.
      @param excludeTime Whether the time is printed in the detail area.
      @param workDays List of workDays
    */
    void drawAgendaDayBox(QPainter &p,
                          const KCalendarCore::Event::List &eventList,
                          const QDate &qd,
                          bool expandable,
                          const QTime &fromTime,
                          const QTime &toTime,
                          const QRect &box,
                          bool includeDescription,
                          bool includeCategories,
                          bool excludeTime,
                          const QList<QDate> &workDays);

    void drawAgendaItem(PrintCellItem *item,
                        QPainter &p,
                        const QDateTime &startPrintDate,
                        const QDateTime &endPrintDate,
                        float minlen,
                        const QRect &box,
                        bool includeDescription,
                        bool includeCategories,
                        bool excludeTime);

    /**
      Draw the box containing a list of all events of the given day (with their times,
      of course). Used in the Filofax and the month print style.

      Obeys configuration options #mExcludeConfidential, #excludePrivate.
      @param p QPainter of the printout
      @param qd The date of the currently printed day. All events of the calendar
                that appear on that day will be printed.
      @param fromTime Start time of the time range to display
      @param toTime End time of the time range to display
      @param box coordinates of the day box.
      @param fullDate Whether the title bar of the box should contain the full
                      date string or just a short.
      @param printRecurDaily Whether daily recurring incidences should be printed.
      @param printRecurWeekly Whether weekly recurring incidences should be printed.
      @param singleLineLimit Whether Incidence text wraps or truncates.
      @param showNoteLines Whether note lines are printed.
      @param includeDescription Whether to print the event description as well.
      @param includeCategories Whether to print the event categories (tags) as well.
      @param useColors Whether to use  event category colors to draw the events.
    */
    void drawDayBox(QPainter &p,
                    const QDate &qd,
                    const QTime &fromTime,
                    const QTime &toTime,
                    const QRect &box,
                    bool fullDate = false,
                    bool printRecurDaily = true,
                    bool printRecurWeekly = true,
                    bool singleLineLimit = true,
                    bool showNoteLines = false,
                    bool includeDescription = false,
                    bool includeCategories = false,
                    bool useColors = true);

    /**
      Draw the week (filofax) table of the week containing the date qd. The first
      three days of the week will be shown in the first column (using drawDayBox),
      the remaining four in the second column, where the last two days of the week
      (typically Saturday and Sunday) only get half the height of the other day boxes.

      Obeys configuration options #mExcludeConfidential, #excludePrivate.
      @param p QPainter of the printout
      @param qd Arbitrary date within the week to be printed.
      @param fromTime Start time of the displayed time range
      @param toTime End time of the displayed time range
      @param box coordinates of the week box.
      @param singleLineLimit Whether Incidence text wraps or truncates.
      @param showNoteLines Whether note lines are printed.
      @param includeDescription Whether to print the event description as well.
      @param includeCategories Whether to print the event categories (tags) as well.
      @param useColors Whether to use  event category colors to draw the events.
    */
    void drawWeek(QPainter &p,
                  const QDate &qd,
                  const QTime &fromTime,
                  const QTime &toTime,
                  const QRect &box,
                  bool singleLineLimit,
                  bool showNoteLines,
                  bool includeDescription,
                  bool includeCategories,
                  bool useColors);

    /**
      Draw the (filofax) table for a bunch of days, using drawDayBox.

      Obeys configuration options #mExcludeConfidential, #excludePrivate.
      @param p QPainter of the printout
      @param start Start date
      @param end End date
      @param fromTime Start time of the displayed time range
      @param toTime End time of the displayed time range
      @param box coordinates of the week box.
      @param singleLineLimit Whether Incidence text wraps or truncates.
      @param showNoteLines Whether note lines are printed.
      @param includeDescription Whether to print the event description as well.
      @param includeCategories Whether to print the event categories (tags) as well.
      @param useColors Whether to use  event category colors to draw the events.
    */
    void drawDays(QPainter &p,
                  const QDate &start,
                  const QDate &end,
                  const QTime &fromTime,
                  const QTime &toTime,
                  const QRect &box,
                  bool singleLineLimit,
                  bool showNoteLines,
                  bool includeDescription,
                  bool includeCategories,
                  bool useColors);

    /**
      Draw the timetable view of the given time range from fromDate to toDate.
      On the left side the time scale is printed (using drawTimeLine), then each
      day gets one column (printed using drawAgendaDayBox),
      and the events are displayed as boxes (like in korganizer's day/week view).
      The first cell of each column contains the all-day events (using
      drawAllDayBox with expandable=false).

      Obeys configuration options #mExcludeConfidential, #excludePrivate.
      @param p QPainter of the printout
      @param fromDate First day to be included in the page
      @param toDate Last day to be included in the page
      @param expandable If true, the start and end times are adjusted to include
      the whole range of all events of that day, not just of the given time range.
      @param fromTime Start time of the displayed time range
      @param toTime End time of the displayed time range
      @param box coordinates of the time table.
      @param includeDescription Whether to print the event description as well.
      @param includeCategories Whether to print the event categories (tags) as well.
      @param excludeTime Whether the time is printed in the detail area.
    */
    void drawTimeTable(QPainter &p,
                       const QDate &fromDate,
                       const QDate &toDate,
                       bool expandable,
                       const QTime &fromTime,
                       const QTime &toTime,
                       const QRect &box,
                       bool includeDescription,
                       bool includeCategories,
                       bool excludeTime);

    /**
      Draw the month table of the month containing the date qd. Each day gets one
      box (using drawDayBox) that contains a list of all events on that day. They are arranged
      in a matrix, with the first column being the first day of the
      week (so it might display some days of the previous and the next month).
      Above the matrix there is a bar showing the weekdays (drawn using drawDaysOfWeek).

      Obeys configuration options #mExcludeConfidential, #excludePrivate.
      @param p QPainter of the printout
      @param qd Arbitrary date within the month to be printed.
      @param fromTime Start time of the displayed time range
      @param toTime End time of the displayed time range
      @param weeknumbers Whether the week numbers are printed left of each row of the matrix
      @param recurDaily Whether daily recurring incidences should be printed.
      @param recurWeekly Whether weekly recurring incidences should be printed.
      @param singleLineLimit Whether Incidence text wraps or truncates.
      @param showNoteLines Whether note lines are printed.
      @param includeDescription Whether descriptions are printed.
      @param includeCategories Whether to print the event categories (tags) as well.
      @param useColors Whether to use  event category colors to draw the events.
      @param box coordinates of the month.
    */
    void drawMonthTable(QPainter &p,
                        const QDate &qd,
                        const QTime &fromTime,
                        const QTime &toTime,
                        bool weeknumbers,
                        bool recurDaily,
                        bool recurWeekly,
                        bool singleLineLimit,
                        bool showNoteLines,
                        bool includeDescription,
                        bool includeCategories,
                        bool useColors,
                        const QRect &box);

    /**
      Draw a vertical representation of the month containing the date dt. Each
      day gets one line.

      Obeys configuration options #mExcludeConfidential, #excludePrivate.
      @param p QPainter of the printout
      @param dt Arbitrary date within the month to be printed
      @param box coordinates of the box reserved for the month
      @param maxdays Days to print. If a value of -1 is given, the number of days
                     is deduced from the month. If maxdays is larger than the
                     number of days in the month, the remaining boxes are
                     shaded to indicate they are not days of the month.
      @param subDailyFlags Bitfield consisting of DisplayFlags flags to determine
                           how events that do not cross midnight should be printed.
      @param holidaysFlags Bitfield consisting of DisplayFlags flags to determine
                           how holidays should be printed.
    */
    void drawMonth(QPainter &p,
                   const QDate &dt,
                   const QRect &box,
                   int maxdays = -1,
                   int subDailyFlags = TimeBoxes,
                   int holidaysFlags = Text);

    /**
      Internal class representing the start of a todo.
    */
    class TodoParentStart;

    /**
      Draws single to-do and its (indented) sub-to-dos, optionally connects them
      by a tree-like line, and optionally shows due date, summary, description
      and priority.
      @param count The number of the currently printed to-do (count will be
      incremented for each to-do drawn)
      @param todo The to-do to be printed. It's sub-to-dos are recursively drawn,
      so drawTodo should only be called on the to-dos of the highest level.
      @param p QPainter of the printout
      @param sortField Specifies on which attribute of the todo you want to sort.
      @param sortDir Specifies if you want to sort ascending or descending.
      @param connectSubTodos Whether sub-to-dos shall be connected with
      their parent by a line (tree-like).
      @param strikeoutCompleted Whether completed to-dos should be printed with
      strike-out summaries.
      @param desc Whether to print the whole description of the to-do
      (the summary is always printed).
      @param posPriority x-coordinate where the priority is supposed to be
      printed. If negative, no priority will be printed.
      @param posSummary x-coordinate where the summary of the to-do is supposed
      to be printed.
      @param posCategories x-coordinate where the categories (tags) should be
      printed. If negative, no categories will be printed.
      @param posStartDt x-coordinate where the due date is supposed to the be
      printed. If negative, no start date will be printed.
      @param posDueDt x-coordinate where the due date is supposed to the be
      printed. If negative, no due date will be printed.
      @param posPercentComplete x-coordinate where the percentage complete is
      supposed to be printed. If negative, percentage complete will not be printed.
      @param level Level of the current to-do in the to-do hierarchy (0 means
      highest level of printed to-dos, 1 are their sub-to-dos, etc.)
      @param x x-coordinate of the upper left coordinate of the first to-do.
      @param y y-coordinate of the upper left coordinate of the first to-do.
      @param width width of the whole to-do list.
      @param pageHeight Total height allowed for the to-do list on a page.
      If an to-do would be below that line, a new page is started.
      @param todoList Contains a list of sub-todos for the specified @p todo .
      @param r Internal (used when printing sub-to-dos to give information
      about its parent)
    */
    void drawTodo(int &count,
                  const KCalendarCore::Todo::Ptr &todo,
                  QPainter &p,
                  KCalendarCore::TodoSortField sortField,
                  KCalendarCore::SortDirection sortDir,
                  bool connectSubTodos,
                  bool strikeoutCompleted,
                  bool desc,
                  int posPriority,
                  int posSummary,
                  int posCategories,
                  int posStartDt,
                  int posDueDt,
                  int posPercentComplete,
                  int level,
                  int x,
                  int &y,
                  int width,
                  int pageHeight,
                  const KCalendarCore::Todo::List &todoList,
                  TodoParentStart *r);

    /**
      Draws text lines splitting on page boundaries.
      @param p QPainter of the printout
      @param x x-coordinate of the upper left coordinate of the first item
      @param y y-coordinate of the upper left coordinate of the first item
      @param width width of the whole list
      @param pageHeight size of the page. A new page is started when the
             text reaches the end of the page.
    */
    void drawTextLines(QPainter &p, const QString &entry, int x, int &y, int width, int pageHeight, bool richTextEntry);

    void drawSplitHeaderRight(QPainter &p, QDate fd, QDate td, QDate cd, int width, int height);

    /**
      Draws dotted lines for notes in a box.
      @param p QPainter of the printout
      @param box coordinates of the box where the lines will be placed
      @param startY starting y-coordinate for the first line
    */
    void drawNoteLines(QPainter &p, QRect box, int startY);

protected:
    QTime dayStart() const;
    QColor categoryBgColor(const KCalendarCore::Incidence::Ptr &incidence) const;

    void drawIncidence(QPainter &p,
                       const QRect &dayBox,
                       const QString &time,
                       const QString &summary,
                       const QString &description,
                       int &textY,
                       bool singleLineLimit,
                       bool includeDescription,
                       bool richDescription);
    QString toPlainText(const QString &htmlText);
    void drawTodoLines(QPainter &p,
                       const QString &entry,
                       int x,
                       int &y,
                       int width,
                       int pageHeight,
                       bool richTextEntry,
                       QList<TodoParentStart *> &startPoints,
                       bool connectSubTodos);

protected:
    bool mUseColors;
    bool mPrintFooter;
    bool mShowNoteLines;
    bool mExcludeConfidential;
    bool mExcludePrivate;
    int mHeaderHeight;
    int mSubHeaderHeight;
    int mFooterHeight;
    int mMargin;
    int mPadding;
    int mBorder;

private:
    QColor categoryColor(const QStringList &categories) const;

    /**
     * Sets the QPainter's brush and pen color according to the Incidence's category.
     */
    void setColorsByIncidenceCategory(QPainter &p, const KCalendarCore::Incidence::Ptr &incidence) const;

    QString holidayString(QDate date) const;

    KCalendarCore::Event::Ptr holidayEvent(QDate date) const;

    /**
     * Returns a nice QColor for text, give the input color &c.
     */
    QColor getTextColor(const QColor &c) const;
};
}

