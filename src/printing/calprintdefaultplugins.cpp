/*
  SPDX-FileCopyrightText: 1998 Preston Brown <pbrown@kde.org>
  SPDX-FileCopyrightText: 2003 Reinhold Kainhofer <reinhold@kainhofer.com>
  SPDX-FileCopyrightText: 2003 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2008 Ron Goodheart <rong.dev@gmail.com>
  SPDX-FileCopyrightText: 2010-2021 Laurent Montel <montel@kde.org>
  SPDX-FileCopyrightText: 2012-2013 Allen Winter <winter@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#include "calprintdefaultplugins.h"
#include "kcalprefs.h"
#include "utils.h"

#include <cmath>

#include <Item>

#include <KCalendarCore/Visitor>

#include <KCalUtils/IncidenceFormatter>
#include <KCalUtils/Stringify>

#include <KConfigGroup>

#include <QPainter>
#include <QPrinter>

using namespace CalendarSupport;

/**************************************************************
 *           Print Incidence
 **************************************************************/

CalPrintIncidence::CalPrintIncidence()
    : CalPrintPluginBase()
{
}

CalPrintIncidence::~CalPrintIncidence()
{
}

QWidget *CalPrintIncidence::createConfigWidget(QWidget *w)
{
    return new CalPrintIncidenceConfig(w);
}

void CalPrintIncidence::readSettingsWidget()
{
    auto cfg = dynamic_cast<CalPrintIncidenceConfig *>((QWidget *)mConfigWidget);
    if (cfg) {
        mUseColors = cfg->mColors->isChecked();
        mPrintFooter = cfg->mPrintFooter->isChecked();
        mShowOptions = cfg->mShowDetails->isChecked();
        mShowSubitemsNotes = cfg->mShowSubitemsNotes->isChecked();
        mShowAttendees = cfg->mShowAttendees->isChecked();
        mShowAttachments = cfg->mShowAttachments->isChecked();
        mShowNoteLines = cfg->mShowNoteLines->isChecked();
    }
}

void CalPrintIncidence::setSettingsWidget()
{
    auto cfg = dynamic_cast<CalPrintIncidenceConfig *>((QWidget *)mConfigWidget);
    if (cfg) {
        cfg->mColors->setChecked(mUseColors);
        cfg->mPrintFooter->setChecked(mPrintFooter);
        cfg->mShowDetails->setChecked(mShowOptions);
        cfg->mShowSubitemsNotes->setChecked(mShowSubitemsNotes);
        cfg->mShowAttendees->setChecked(mShowAttendees);
        cfg->mShowAttachments->setChecked(mShowAttachments);
        cfg->mShowNoteLines->setChecked(mShowNoteLines);
    }
}

void CalPrintIncidence::doLoadConfig()
{
    CalPrintPluginBase::doLoadConfig();
    if (mConfig) {
        KConfigGroup grp(mConfig, groupName());
        mShowOptions = grp.readEntry("Show Options", false);
        mShowSubitemsNotes = grp.readEntry("Show Subitems and Notes", false);
        mShowAttendees = grp.readEntry("Use Attendees", false);
        mShowAttachments = grp.readEntry("Use Attachments", false);
    }
    setSettingsWidget();
}

void CalPrintIncidence::doSaveConfig()
{
    readSettingsWidget();
    if (mConfig) {
        KConfigGroup grp(mConfig, groupName());
        grp.writeEntry("Show Options", mShowOptions);
        grp.writeEntry("Show Subitems and Notes", mShowSubitemsNotes);
        grp.writeEntry("Use Attendees", mShowAttendees);
        grp.writeEntry("Use Attachments", mShowAttachments);
    }
    CalPrintPluginBase::doSaveConfig();
}

class TimePrintStringsVisitor : public KCalendarCore::Visitor
{
public:
    TimePrintStringsVisitor()
    {
    }

    bool act(KCalendarCore::IncidenceBase::Ptr incidence)
    {
        return incidence->accept(*this, incidence);
    }

    QString mStartCaption, mStartString;
    QString mEndCaption, mEndString;
    QString mDurationCaption, mDurationString;

protected:
    bool visit(const KCalendarCore::Event::Ptr &event) override
    {
        if (event->dtStart().isValid()) {
            mStartCaption = i18n("Start date: ");
            mStartString = KCalUtils::IncidenceFormatter::dateTimeToString(event->dtStart(), event->allDay(), false);
        } else {
            mStartCaption = i18n("No start date");
            mStartString.clear();
        }

        if (event->hasEndDate()) {
            mEndCaption = i18n("End date: ");
            mEndString = KCalUtils::IncidenceFormatter::dateTimeToString(event->dtEnd(), event->allDay(), false);
        } else if (event->hasDuration()) {
            mEndCaption = i18n("Duration: ");
            int mins = event->duration().asSeconds() / 60;
            if (mins >= 60) {
                mEndString += i18np("1 hour ", "%1 hours ", mins / 60);
            }
            if (mins % 60 > 0) {
                mEndString += i18np("1 minute ", "%1 minutes ", mins % 60);
            }
        } else {
            mEndCaption = i18n("No end date");
            mEndString.clear();
        }
        return true;
    }

    bool visit(const KCalendarCore::Todo::Ptr &todo) override
    {
        if (todo->hasStartDate()) {
            mStartCaption = i18n("Start date: ");
            mStartString = KCalUtils::IncidenceFormatter::dateTimeToString(todo->dtStart(), todo->allDay(), false);
        } else {
            mStartCaption = i18n("No start date");
            mStartString.clear();
        }

        if (todo->hasDueDate()) {
            mEndCaption = i18n("Due date: ");
            mEndString = KCalUtils::IncidenceFormatter::dateTimeToString(todo->dtDue(), todo->allDay(), false);
        } else {
            mEndCaption = i18n("No due date");
            mEndString.clear();
        }
        return true;
    }

    bool visit(const KCalendarCore::Journal::Ptr &journal) override
    {
        mStartCaption = i18n("Start date: ");
        mStartString = KCalUtils::IncidenceFormatter::dateTimeToString(journal->dtStart(), journal->allDay(), false);
        mEndCaption.clear();
        mEndString.clear();
        return true;
    }

    bool visit(const KCalendarCore::FreeBusy::Ptr &fb) override
    {
        Q_UNUSED(fb)
        return true;
    }
};

int CalPrintIncidence::printCaptionAndText(QPainter &p, QRect box, const QString &caption, const QString &text, const QFont &captionFont, const QFont &textFont)
{
    QFontMetrics captionFM(captionFont);
    int textWd = captionFM.horizontalAdvance(caption);
    QRect textRect(box);

    QFont oldFont(p.font());
    p.setFont(captionFont);
    p.drawText(box, Qt::AlignLeft | Qt::AlignTop | Qt::TextSingleLine, caption);

    if (!text.isEmpty()) {
        textRect.setLeft(textRect.left() + textWd);
        p.setFont(textFont);
        p.drawText(textRect, Qt::AlignLeft | Qt::AlignTop | Qt::TextSingleLine, text);
    }
    p.setFont(oldFont);
    return textRect.bottom();
}

void CalPrintIncidence::print(QPainter &p, int width, int height)
{
    QFont oldFont(p.font());
    QFont textFont(QStringLiteral("sans-serif"), 11, QFont::Normal);
    QFont captionFont(QStringLiteral("sans-serif"), 11, QFont::Bold);
    p.setFont(textFont);
    int lineHeight = p.fontMetrics().lineSpacing();
    QString cap, txt;

    KCalendarCore::Incidence::List::ConstIterator it;
    for (it = mSelectedIncidences.constBegin(); it != mSelectedIncidences.constEnd(); ++it) {
        // don't do anything on a 0-pointer!
        if (!(*it)) {
            continue;
        }
        if (it != mSelectedIncidences.constBegin()) {
            mPrinter->newPage();
        }

        const bool isJournal = ((*it)->type() == KCalendarCore::Incidence::TypeJournal);

        //  PAGE Layout (same for landscape and portrait! astonishingly, it looks good with both!):
        //  +-----------------------------------+
        //  | Header:  Summary                  |
        //  +===================================+
        //  | start: ______   end: _________    |
        //  | repeats: ___________________      |
        //  | reminder: __________________      |
        //  +-----------------------------------+
        //  | Location: ______________________  |
        //  +------------------------+----------+
        //  | Description:           | Notes or |
        //  |                        | Subitems |
        //  |                        |          |
        //  |                        |          |
        //  |                        |          |
        //  |                        |          |
        //  |                        |          |
        //  |                        |          |
        //  |                        |          |
        //  |                        |          |
        //  +------------------------+----------+
        //  | Attachments:           | Settings |
        //  |                        |          |
        //  +------------------------+----------+
        //  | Attendees:                        |
        //  |                                   |
        //  +-----------------------------------+
        //  | Categories: _____________________ |
        //  +-----------------------------------+

        QRect box(0, 0, width, height);
        QRect titleBox(box);
        titleBox.setHeight(headerHeight());
        QColor headerColor = mUseColors ? categoryBgColor(*it) : QColor();
        // Draw summary as header, no small calendars in title bar, expand height if needed
        int titleBottom = drawHeader(p, (*it)->summary(), QDate(), QDate(), titleBox, true, headerColor);
        titleBox.setBottom(titleBottom);

        QRect timesBox(titleBox);
        timesBox.setTop(titleBox.bottom() + padding());
        timesBox.setHeight(height / 8);

        TimePrintStringsVisitor stringVis;
        int h = timesBox.top();
        if (stringVis.act(*it)) {
            QRect textRect(timesBox.left() + padding(), timesBox.top() + padding(), 0, lineHeight);
            textRect.setRight(timesBox.center().x());
            h = printCaptionAndText(p, textRect, stringVis.mStartCaption, stringVis.mStartString, captionFont, textFont);

            textRect.setLeft(textRect.right());
            textRect.setRight(timesBox.right() - padding());
            h = qMax(printCaptionAndText(p, textRect, stringVis.mEndCaption, stringVis.mEndString, captionFont, textFont), h);
        }

        // Recurrence Printing
        if ((*it)->recurs()) {
            QRect recurBox(timesBox.left() + padding(), h + padding(), timesBox.right() - padding(), lineHeight);
            KCalendarCore::Recurrence *recurs = (*it)->recurrence();
            QString displayString = KCalUtils::IncidenceFormatter::recurrenceString((*it));
            // exception dates
            QString exceptString;
            if (!recurs->exDates().isEmpty()) {
                exceptString = i18nc("except for listed dates", " except");
                for (int i = 0; i < recurs->exDates().size(); ++i) {
                    exceptString.append(QLatin1Char(' '));
                    exceptString.append(QLocale::system().toString(recurs->exDates().at(i), QLocale::ShortFormat));
                }
            }
            displayString.append(exceptString);
            h = qMax(printCaptionAndText(p, recurBox, i18n("Repeats: "), displayString, captionFont, textFont), h);
        }

        if (!isJournal) {
            // Alarms Printing
            QRect alarmBox(timesBox.left() + padding(), h + padding(), timesBox.right() - padding(), lineHeight);
            KCalendarCore::Alarm::List alarms = (*it)->alarms();
            if (alarms.isEmpty()) {
                cap = i18n("No reminders");
                txt.clear();
            } else {
                cap = i18np("Reminder: ", "%1 reminders: ", alarms.count());

                QStringList alarmStrings;
                KCalendarCore::Alarm::List::ConstIterator it;
                alarmStrings.reserve(alarms.count());
                for (it = alarms.constBegin(); it != alarms.constEnd(); ++it) {
                    KCalendarCore::Alarm::Ptr alarm = *it;

                    // Alarm offset, copied from koeditoralarms.cpp:
                    KLocalizedString offsetstr;
                    int offset = 0;
                    if (alarm->hasStartOffset()) {
                        offset = alarm->startOffset().asSeconds();
                        if (offset < 0) {
                            offsetstr = ki18nc("N days/hours/minutes before/after the start/end", "%1 before the start");
                            offset = -offset;
                        } else {
                            offsetstr = ki18nc("N days/hours/minutes before/after the start/end", "%1 after the start");
                        }
                    } else if (alarm->hasEndOffset()) {
                        offset = alarm->endOffset().asSeconds();
                        if (offset < 0) {
                            offsetstr = ki18nc("N days/hours/minutes before/after the start/end", "%1 before the end");
                            offset = -offset;
                        } else {
                            offsetstr = ki18nc("N days/hours/minutes before/after the start/end", "%1 after the end");
                        }
                    }

                    offset = offset / 60; // make minutes
                    int useoffset = 0;

                    if (offset % (24 * 60) == 0 && offset > 0) { // divides evenly into days?
                        useoffset = offset / (24 * 60);
                        offsetstr = offsetstr.subs(i18np("1 day", "%1 days", useoffset));
                    } else if (offset % 60 == 0 && offset > 0) { // divides evenly into hours?
                        useoffset = offset / 60;
                        offsetstr = offsetstr.subs(i18np("1 hour", "%1 hours", useoffset));
                    } else {
                        useoffset = offset;
                        offsetstr = offsetstr.subs(i18np("1 minute", "%1 minutes", useoffset));
                    }
                    alarmStrings << offsetstr.toString();
                }
                txt = alarmStrings.join(i18nc("Spacer for the joined list of categories/tags", ", "));
            }
            h = qMax(printCaptionAndText(p, alarmBox, cap, txt, captionFont, textFont), h);
        }
        QRect organizerBox(timesBox.left() + padding(), h + padding(), timesBox.right() - padding(), lineHeight);
        h = qMax(printCaptionAndText(p, organizerBox, i18n("Organizer: "), (*it)->organizer().fullName(), captionFont, textFont), h);

        // Finally, draw the frame around the time information...
        timesBox.setBottom(qMax(timesBox.bottom(), h + padding()));
        drawBox(p, BOX_BORDER_WIDTH, timesBox);

        QRect locationBox(timesBox);
        locationBox.setTop(timesBox.bottom() + padding());
        locationBox.setHeight(0);
        int locationBottom = 0;
        if (!isJournal) {
            locationBottom = drawBoxWithCaption(p,
                                                locationBox,
                                                i18n("Location: "),
                                                (*it)->location(),
                                                /*sameLine=*/true,
                                                /*expand=*/true,
                                                captionFont,
                                                textFont);
        }
        locationBox.setBottom(locationBottom);

        // Now start constructing the boxes from the bottom:
        QRect footerBox(locationBox);
        footerBox.setBottom(box.bottom());
        footerBox.setTop(footerBox.bottom() - lineHeight - 2 * padding());

        QRect categoriesBox(footerBox);
        categoriesBox.setBottom(footerBox.top());
        categoriesBox.setTop(categoriesBox.bottom() - lineHeight - 2 * padding());
        QRect attendeesBox(box.left(), categoriesBox.top() - padding() - box.height() / 9, box.width(), box.height() / 9);
        QRect attachmentsBox(box.left(), attendeesBox.top() - padding() - box.height() / 9, box.width() * 3 / 4 - padding(), box.height() / 9);
        QRect optionsBox(isJournal ? box.left() : attachmentsBox.right() + padding(), attachmentsBox.top(), 0, 0);
        optionsBox.setRight(box.right());
        optionsBox.setBottom(attachmentsBox.bottom());
        QRect notesBox(optionsBox.left(), isJournal ? (timesBox.bottom() + padding()) : (locationBox.bottom() + padding()), optionsBox.width(), 0);
        notesBox.setBottom(optionsBox.top() - padding());
        QRect descriptionBox(notesBox);
        descriptionBox.setLeft(box.left());
        descriptionBox.setRight(attachmentsBox.right());

        // Adjust boxes depending on the show options...
        if (!mShowSubitemsNotes || isJournal) {
            descriptionBox.setRight(box.right());
        }
        if (!mShowAttachments || !mShowAttendees) {
            descriptionBox.setBottom(attachmentsBox.bottom());
            optionsBox.setTop(attendeesBox.top());
            optionsBox.setBottom(attendeesBox.bottom());
            notesBox.setBottom(attachmentsBox.bottom());
            if (mShowOptions) {
                attendeesBox.setRight(attachmentsBox.right());
            }
            if (!mShowAttachments && !mShowAttendees) {
                if (mShowSubitemsNotes) {
                    descriptionBox.setBottom(attendeesBox.bottom());
                }
                if (!mShowOptions) {
                    descriptionBox.setBottom(attendeesBox.bottom());
                    notesBox.setBottom(attendeesBox.bottom());
                }
            }
        }
        if (mShowAttachments && !isJournal) {
            if (!mShowOptions) {
                attachmentsBox.setRight(box.right());
                attachmentsBox.setRight(box.right());
            }
            if (!mShowAttendees) {
                attachmentsBox.setTop(attendeesBox.top());
                attachmentsBox.setBottom(attendeesBox.bottom());
            }
        }
        int newBottom = drawBoxWithCaption(p,
                                           descriptionBox,
                                           i18n("Description:"),
                                           (*it)->description(),
                                           /*sameLine=*/false,
                                           /*expand=*/false,
                                           captionFont,
                                           textFont,
                                           (*it)->descriptionIsRich());
        if (mShowNoteLines) {
            drawNoteLines(p, descriptionBox, newBottom);
        }

        Akonadi::Item item = mCalendar->item((*it)->uid());
        Akonadi::Item::List relations = mCalendar->childItems(item.id());

        if (mShowSubitemsNotes && !isJournal) {
            if (relations.isEmpty() || (*it)->type() != KCalendarCore::Incidence::TypeTodo) {
                int notesPosition = drawBoxWithCaption(p,
                                                       notesBox,
                                                       i18n("Notes:"),
                                                       QString(),
                                                       /*sameLine=*/false,
                                                       /*expand=*/false,
                                                       captionFont,
                                                       textFont);
                if (mShowNoteLines) {
                    drawNoteLines(p, notesBox, notesPosition);
                }
            } else {
                QString subitemCaption;
                if (relations.isEmpty()) {
                    subitemCaption = i18n("No Subitems");
                    txt.clear();
                } else {
                    subitemCaption = i18np("1 Subitem:", "%1 Subitems:", relations.count());
                }

                QString subitemString;
                QString statusString;
                QString datesString;
                int count = 0;
                for (const Akonadi::Item &item : std::as_const(relations)) {
                    KCalendarCore::Todo::Ptr todo = CalendarSupport::todo(item);
                    ++count;
                    if (!todo) { // defensive, skip any zero pointers
                        continue;
                    }
                    // format the status
                    statusString = KCalUtils::Stringify::incidenceStatus(todo->status());
                    if (statusString.isEmpty()) {
                        if (todo->status() == KCalendarCore::Incidence::StatusNone) {
                            statusString = i18nc("no status", "none");
                        } else {
                            statusString = i18nc("unknown status", "unknown");
                        }
                    }
                    // format the dates if provided
                    datesString.clear();
                    if (todo->dtStart().isValid()) {
                        datesString +=
                            i18nc("subitem start date", "Start Date: %1\n", QLocale().toString(todo->dtStart().toLocalTime().date(), QLocale::ShortFormat));
                        if (!todo->allDay()) {
                            datesString +=
                                i18nc("subitem start time", "Start Time: %1\n", QLocale().toString(todo->dtStart().toLocalTime().time(), QLocale::ShortFormat));
                        }
                    }
                    if (todo->dateTime(KCalendarCore::Incidence::RoleEnd).isValid()) {
                        subitemString +=
                            i18nc("subitem due date",
                                  "Due Date: %1\n",
                                  QLocale().toString(todo->dateTime(KCalendarCore::Incidence ::RoleEnd).toLocalTime().date(), QLocale::ShortFormat));

                        if (!todo->allDay()) {
                            subitemString +=
                                i18nc("subitem due time",
                                      "Due Time: %1\n",
                                      QLocale().toString(todo->dateTime(KCalendarCore::Incidence::RoleEnd).toLocalTime().time(), QLocale::ShortFormat));
                        }
                    }
                    subitemString += i18nc("subitem counter", "%1: ", count);
                    subitemString += todo->summary();
                    subitemString += QLatin1Char('\n');
                    if (!datesString.isEmpty()) {
                        subitemString += datesString;
                        subitemString += QLatin1Char('\n');
                    }
                    subitemString += i18nc("subitem Status: statusString", "Status: %1\n", statusString);
                    subitemString += KCalUtils::IncidenceFormatter::recurrenceString(todo) + QLatin1Char('\n');
                    subitemString += i18nc("subitem Priority: N", "Priority: %1\n", QString::number(todo->priority()));
                    subitemString += i18nc("subitem Secrecy: secrecyString", "Secrecy: %1\n", KCalUtils::Stringify::incidenceSecrecy(todo->secrecy()));
                    subitemString += QLatin1Char('\n');
                }
                drawBoxWithCaption(p,
                                   notesBox,
                                   subitemCaption,
                                   subitemString,
                                   /*sameLine=*/false,
                                   /*expand=*/false,
                                   captionFont,
                                   textFont);
            }
        }

        if (mShowAttachments && !isJournal) {
            const KCalendarCore::Attachment::List attachments = (*it)->attachments();
            QString attachmentCaption;
            if (attachments.isEmpty()) {
                attachmentCaption = i18n("No Attachments");
                txt.clear();
            } else {
                attachmentCaption = i18np("1 Attachment:", "%1 Attachments:", attachments.count());
            }
            QString attachmentString;
            KCalendarCore::Attachment::List::ConstIterator ait = attachments.constBegin();
            for (; ait != attachments.constEnd(); ++ait) {
                if (!attachmentString.isEmpty()) {
                    attachmentString += i18nc("Spacer for list of attachments", "  ");
                }
                attachmentString.append((*ait).label());
            }
            drawBoxWithCaption(p,
                               attachmentsBox,
                               attachmentCaption,
                               attachmentString,
                               /*sameLine=*/false,
                               /*expand=*/false,
                               captionFont,
                               textFont);
        }
        if (mShowAttendees) {
            const KCalendarCore::Attendee::List attendees = (*it)->attendees();
            QString attendeeCaption;
            if (attendees.isEmpty()) {
                attendeeCaption = i18n("No Attendees");
            } else {
                attendeeCaption = i18np("1 Attendee:", "%1 Attendees:", attendees.count());
            }
            QString attendeeString;
            KCalendarCore::Attendee::List::ConstIterator ait = attendees.constBegin();
            for (; ait != attendees.constEnd(); ++ait) {
                if (!attendeeString.isEmpty()) {
                    attendeeString += QLatin1Char('\n');
                }
                attendeeString += i18nc(
                    "Formatting of an attendee: "
                    "'Name (Role): Status', e.g. 'Reinhold Kainhofer "
                    "<reinhold@kainhofer.com> (Participant): Awaiting Response'",
                    "%1 (%2): %3",
                    (*ait).fullName(),
                    KCalUtils::Stringify::attendeeRole((*ait).role()),
                    KCalUtils::Stringify::attendeeStatus((*ait).status()));
            }
            drawBoxWithCaption(p,
                               attendeesBox,
                               attendeeCaption,
                               attendeeString,
                               /*sameLine=*/false,
                               /*expand=*/false,
                               captionFont,
                               textFont);
        }

        if (mShowOptions) {
            QString optionsString;
            if (!KCalUtils::Stringify::incidenceStatus((*it)->status()).isEmpty()) {
                optionsString += i18n("Status: %1", KCalUtils::Stringify::incidenceStatus((*it)->status()));
                optionsString += QLatin1Char('\n');
            }
            if (!KCalUtils::Stringify::incidenceSecrecy((*it)->secrecy()).isEmpty()) {
                optionsString += i18n("Secrecy: %1", KCalUtils::Stringify::incidenceSecrecy((*it)->secrecy()));
                optionsString += QLatin1Char('\n');
            }
            if ((*it)->type() == KCalendarCore::Incidence::TypeEvent) {
                KCalendarCore::Event::Ptr e = (*it).staticCast<KCalendarCore::Event>();
                if (e->transparency() == KCalendarCore::Event::Opaque) {
                    optionsString += i18n("Show as: Busy");
                } else {
                    optionsString += i18n("Show as: Free");
                }
                optionsString += QLatin1Char('\n');
            } else if ((*it)->type() == KCalendarCore::Incidence::TypeTodo) {
                KCalendarCore::Todo::Ptr t = (*it).staticCast<KCalendarCore::Todo>();
                if (t->isOverdue()) {
                    optionsString += i18n("This task is overdue!");
                    optionsString += QLatin1Char('\n');
                }
            } else if ((*it)->type() == KCalendarCore::Incidence::TypeJournal) {
                // TODO: Anything Journal-specific?
            }
            drawBoxWithCaption(p, optionsBox, i18n("Settings: "), optionsString, /*sameLine=*/false, /*expand=*/false, captionFont, textFont);
        }

        drawBoxWithCaption(p,
                           categoriesBox,
                           i18n("Tags: "),
                           (*it)->categories().join(i18nc("Spacer for the joined list of categories/tags", ", ")),
                           /*sameLine=*/true,
                           /*expand=*/false,
                           captionFont,
                           textFont);

        if (mPrintFooter) {
            drawFooter(p, footerBox);
        }
    }
    p.setFont(oldFont);
}

/**************************************************************
 *           Print Timetables
 **************************************************************/

CalPrintTimetable::CalPrintTimetable()
    : CalPrintPluginBase()
{
}

CalPrintTimetable::~CalPrintTimetable()
{
}

void CalPrintTimetable::doLoadConfig()
{
    CalPrintPluginBase::doLoadConfig();
    if (mConfig) {
        KConfigGroup grp(mConfig, groupName());
        QDate dt = QDate::currentDate(); // any valid QDate will do
        QTime tm1(dayStart());
        QDateTime startTm(dt, tm1);
        QDateTime endTm(dt, tm1.addSecs(12 * 60 * 60));
        mStartTime = grp.readEntry("Start time", startTm).time();
        mEndTime = grp.readEntry("End time", endTm).time();
        mIncludeDescription = grp.readEntry("Include description", false);
        mIncludeCategories = grp.readEntry("Include categories", false);
        mIncludeTodos = grp.readEntry("Include todos", false);
        mIncludeAllEvents = grp.readEntry("Include all events", false);
        mSingleLineLimit = grp.readEntry("Single line limit", false);
        mExcludeTime = grp.readEntry("Exclude time", false);
    }
}

void CalPrintTimetable::doSaveConfig()
{
    if (mConfig) {
        KConfigGroup grp(mConfig, groupName());
        QDateTime dt = QDateTime::currentDateTime(); // any valid QDateTime will do
        dt.setTime(mStartTime);
        grp.writeEntry("Start time", dt);
        dt.setTime(mEndTime);
        grp.writeEntry("End time", dt);
        grp.writeEntry("Include description", mIncludeDescription);
        grp.writeEntry("Include categories", mIncludeCategories);
        grp.writeEntry("Include todos", mIncludeTodos);
        grp.writeEntry("Include all events", mIncludeAllEvents);
        grp.writeEntry("Single line limit", mSingleLineLimit);
        grp.writeEntry("Exclude time", mExcludeTime);
    }
    CalPrintPluginBase::doSaveConfig();
}

static QString cleanString(const QString &instr)
{
    QString ret = instr;
    return ret.replace(QLatin1Char('\n'), QLatin1Char(' '));
}

void CalPrintTimetable::drawAllDayBox(QPainter &p,
                                      const KCalendarCore::Event::List &eventList,
                                      QDate qd,
                                      QRect box,
                                      const QList<QDate> &workDays)
{
    int lineSpacing = p.fontMetrics().lineSpacing();

    if (!workDays.contains(qd)) {
        drawShadedBox(p, BOX_BORDER_WIDTH, sHolidayBackground, box);
    } else {
        drawBox(p, BOX_BORDER_WIDTH, box);
    }

    QRect eventBox(box);
    eventBox.setTop(box.top() + padding());
    eventBox.setBottom(eventBox.top() + lineSpacing);

    for (const KCalendarCore::Event::Ptr &currEvent : std::as_const(eventList)) {
        if (!currEvent
            || !currEvent->allDay()
            || (mExcludeConfidential && currEvent->secrecy() == KCalendarCore::Incidence::SecrecyConfidential)
            || (mExcludePrivate && currEvent->secrecy() == KCalendarCore::Incidence::SecrecyPrivate)) {
            continue;
        }
        QString str;
        if (currEvent->location().isEmpty()) {
            str = cleanString(currEvent->summary());
        } else {
            str = i18nc("summary, location", "%1, %2", cleanString(currEvent->summary()), cleanString(currEvent->location()));
        }
        if (mIncludeCategories && !currEvent->categoriesStr().isEmpty()) {
                str = i18nc("summary, categories", "%1, %2", str, currEvent->categoriesStr());
        }
        printEventString(p, eventBox, str);
        eventBox.setTop(eventBox.bottom());
        eventBox.setBottom(eventBox.top() + lineSpacing);
    }
}

void CalPrintTimetable::drawTimeTable(QPainter &p,
                                       QDate fromDate,
                                       QDate toDate,
                                       QRect box)
{
    QTime myFromTime = mStartTime;
    QTime myToTime = mEndTime;
    int maxAllDayEvents = 0;
    QDate curDate(fromDate);
    while (curDate <= toDate) {
        KCalendarCore::Event::List eventList = mCalendar->events(curDate, QTimeZone::systemTimeZone());
        const auto holidays = holiday(curDate);
        int allDayEvents = holiday(curDate).isEmpty() ? 0 : 1;
        for (const KCalendarCore::Event::Ptr &event : std::as_const(eventList)) {
            Q_ASSERT(event);
            if (!event
                || (mExcludeConfidential && event->secrecy() == KCalendarCore::Incidence::SecrecyConfidential)
                || (mExcludePrivate && event->secrecy() == KCalendarCore::Incidence::SecrecyPrivate)) {
                continue;
            }
            if (event->allDay()) {
                allDayEvents += 1;
            } else if (mIncludeAllEvents) {
                if (event->dtStart().time() < myFromTime) {
                    myFromTime = event->dtStart().time();
                }
                if (event->dtEnd().time() > myToTime) {
                    myToTime = event->dtEnd().time();
                }
            }
        }
        if (allDayEvents > maxAllDayEvents) {
            maxAllDayEvents = allDayEvents;
        }
        curDate = curDate.addDays(1);
    }

    QFont oldFont(p.font());
    p.setFont(QFont(QStringLiteral("sans-serif"), 11, QFont::Normal));
    const int lineSpacing = p.fontMetrics().lineSpacing();

    int timelineWidth = TIMELINE_WIDTH + padding();

    QRect dowBox(box);
    dowBox.setLeft(box.left() + timelineWidth);
    dowBox.setHeight(mSubHeaderHeight);
    drawDaysOfWeek(p, fromDate, toDate, dowBox);

    int tlTop = dowBox.bottom();

    int alldayHeight = 0;
    if (maxAllDayEvents > 0) {
        // Draw the side bar for all-day events.
        const auto alldayLabel =  i18nc("label for timetable all-day boxes", "All day");
        QFont oldFont(p.font());
        p.setFont(QFont(QStringLiteral("sans-serif"), 9, QFont::Normal));
        const auto labelHeight = p.fontMetrics().horizontalAdvance(alldayLabel) + 2*padding();
        alldayHeight = std::max(maxAllDayEvents*lineSpacing + 2*padding(), labelHeight);
        drawVerticalBox(p,
                        BOX_BORDER_WIDTH,
                        QRect(0, tlTop, TIMELINE_WIDTH, alldayHeight),
                        alldayLabel,
                        Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextWordWrap);
        p.setFont(oldFont);
        tlTop += alldayHeight +padding();
    }

    QRect tlBox(box);
    tlBox.setWidth(TIMELINE_WIDTH);
    tlBox.setTop(tlTop);
    drawTimeLine(p, myFromTime, myToTime, tlBox);

    // draw each day
    curDate = fromDate;
    int i = 0;
    double cellWidth = double(dowBox.width() - 1) / double(fromDate.daysTo(toDate) + 1);
    QRect allDayBox(dowBox.left(), dowBox.bottom(), cellWidth, alldayHeight);
    const QList<QDate> workDays = CalendarSupport::workDays(fromDate, toDate);
    while (curDate <= toDate) {
        KCalendarCore::Event::List eventList =
            mCalendar->events(curDate, QTimeZone::systemTimeZone(), KCalendarCore::EventSortStartDate, KCalendarCore::SortDirectionAscending);

        allDayBox.setLeft(dowBox.left() + int(i * cellWidth));
        allDayBox.setRight(dowBox.left() + int((i + 1) * cellWidth));
        if (maxAllDayEvents > 0) {
            if (const auto h = holidayEvent(curDate)) {
                eventList.prepend(h);
            }
            drawAllDayBox(p, eventList, curDate, allDayBox, workDays);
        }

        QRect dayBox(allDayBox);
        dayBox.setTop(tlTop);
        dayBox.setBottom(box.bottom());
        drawAgendaDayBox(p,
                         eventList,
                         curDate,
                         false,
                         myFromTime,
                         myToTime,
                         dayBox,
                         mIncludeDescription,
                         mIncludeCategories,
                         mExcludeTime,
                         workDays);

        ++i;
        curDate = curDate.addDays(1);
    }
}

/**************************************************************
 *           Print Day
 **************************************************************/

CalPrintDay::CalPrintDay()
    : CalPrintTimetable()
{
}

CalPrintDay::~CalPrintDay()
{
}

QWidget *CalPrintDay::createConfigWidget(QWidget *w)
{
    return new CalPrintDayConfig(w);
}

void CalPrintDay::readSettingsWidget()
{
    auto cfg = dynamic_cast<CalPrintDayConfig *>((QWidget *)mConfigWidget);
    if (cfg) {
        mFromDate = cfg->mFromDate->date();
        mToDate = cfg->mToDate->date();

        if (cfg->mPrintTypeFilofax->isChecked()) {
            mDayPrintType = Filofax;
        } else if (cfg->mPrintTypeTimetable->isChecked()) {
            mDayPrintType = Timetable;
        } else {
            mDayPrintType = SingleTimetable;
        }

        mStartTime = cfg->mFromTime->time();
        mEndTime = cfg->mToTime->time();
        mIncludeAllEvents = cfg->mIncludeAllEvents->isChecked();

        mIncludeDescription = cfg->mIncludeDescription->isChecked();
        mIncludeCategories = cfg->mIncludeCategories->isChecked();
        mSingleLineLimit = cfg->mSingleLineLimit->isChecked();
        mIncludeTodos = cfg->mIncludeTodos->isChecked();
        mUseColors = cfg->mColors->isChecked();
        mPrintFooter = cfg->mPrintFooter->isChecked();
        mShowNoteLines = cfg->mShowNoteLines->isChecked();
        mExcludeTime = cfg->mExcludeTime->isChecked();
        mExcludeConfidential = cfg->mExcludeConfidential->isChecked();
        mExcludePrivate = cfg->mExcludePrivate->isChecked();
    }
}

void CalPrintDay::setSettingsWidget()
{
    auto cfg = dynamic_cast<CalPrintDayConfig *>((QWidget *)mConfigWidget);
    if (cfg) {
        cfg->mFromDate->setDate(mFromDate);
        cfg->mToDate->setDate(mToDate);

        cfg->mPrintTypeFilofax->setChecked(mDayPrintType == Filofax);
        cfg->mPrintTypeTimetable->setChecked(mDayPrintType == Timetable);
        cfg->mPrintTypeSingleTimetable->setChecked(mDayPrintType == SingleTimetable);

        cfg->mFromTime->setTime(mStartTime);
        cfg->mToTime->setTime(mEndTime);
        cfg->mIncludeAllEvents->setChecked(mIncludeAllEvents);

        cfg->mIncludeDescription->setChecked(mIncludeDescription);
        cfg->mIncludeCategories->setChecked(mIncludeCategories);
        cfg->mSingleLineLimit->setChecked(mSingleLineLimit);
        cfg->mIncludeTodos->setChecked(mIncludeTodos);
        cfg->mColors->setChecked(mUseColors);
        cfg->mPrintFooter->setChecked(mPrintFooter);
        cfg->mShowNoteLines->setChecked(mShowNoteLines);
        cfg->mExcludeTime->setChecked(mExcludeTime);
        cfg->mExcludeConfidential->setChecked(mExcludeConfidential);
        cfg->mExcludePrivate->setChecked(mExcludePrivate);
    }
}

void CalPrintDay::doLoadConfig()
{
    CalPrintTimetable::doLoadConfig();
    if (mConfig) {
        KConfigGroup grp(mConfig, groupName());
        mDayPrintType = static_cast<eDayPrintType>(grp.readEntry("Print type", static_cast<int>(Timetable)));
    }
    setSettingsWidget();
}

void CalPrintDay::doSaveConfig()
{
    readSettingsWidget();
    if (mConfig) {
        KConfigGroup grp(mConfig, groupName());
        grp.writeEntry("Print type", int(mDayPrintType));
    }
    CalPrintTimetable::doSaveConfig();
}

void CalPrintDay::setDateRange(const QDate &from, const QDate &to)
{
    CalPrintPluginBase::setDateRange(from, to);
    auto cfg = dynamic_cast<CalPrintDayConfig *>((QWidget *)mConfigWidget);
    if (cfg) {
        cfg->mFromDate->setDate(from);
        cfg->mToDate->setDate(to);
    }
}

void CalPrintDay::drawDays(QPainter &p, QRect box)
{
    const int numberOfDays = mFromDate.daysTo(mToDate) + 1;
    int vcells;
    const bool portrait = (box.height() > box.width());
    int cellWidth;
    if (portrait) {
        // 2 columns
        vcells = std::ceil(static_cast<double>(numberOfDays) / 2.0);
        if (numberOfDays > 1) {
            cellWidth = box.width() / 2;
        } else {
            cellWidth = box.width();
        }
    } else {
        // landscape: N columns
        vcells = 1;
        cellWidth = box.width() / numberOfDays;
    }
    const int cellHeight = box.height() / vcells;
    QDate weekDate = mFromDate;
    for (int i = 0; i < numberOfDays; ++i, weekDate = weekDate.addDays(1)) {
        const int hpos = i / vcells;
        const int vpos = i % vcells;
        const QRect dayBox(box.left() + cellWidth * hpos, box.top() + cellHeight * vpos, cellWidth, cellHeight);
        drawDayBox(p,
                   weekDate,
                   mStartTime,
                   mEndTime,
                   dayBox,
                   true,
                   true,
                   true,
                   mSingleLineLimit,
                   mIncludeDescription,
                   mIncludeCategories);
    } // for i through all selected days
}

void CalPrintDay::print(QPainter &p, int width, int height)
{
    QDate curDay(mFromDate);

    QRect headerBox(0, 0, width, headerHeight());
    QRect footerBox(0, height - footerHeight(), width, footerHeight());
    height -= footerHeight();
    QRect daysBox(headerBox);
    daysBox.setTop(headerBox.bottom() + padding());
    daysBox.setBottom(height);

    auto local = QLocale::system();

    switch (mDayPrintType) {
    case Filofax:
    case SingleTimetable: {
        QString line1 = local.toString(mFromDate, QLocale::ShortFormat);
        QString line2 = local.toString(mToDate, QLocale::ShortFormat);
        QString title;
        if (mFromDate == mToDate) {
            title = line1;
        } else {
            title =  i18nc("date from-to", "%1\u2013%2", line1, line2);
        }
        drawHeader(p, title, mFromDate, QDate(), headerBox);
        if (mDayPrintType == Filofax) {
            drawDays(p, daysBox);
        } else if (mDayPrintType == SingleTimetable) {
            drawTimeTable(p, mFromDate, mToDate, daysBox);
        }
        if (mPrintFooter) {
            drawFooter(p, footerBox);
        }
        break;
    }

    case Timetable:
    default:
        do {
            QTime curStartTime(mStartTime);
            QTime curEndTime(mEndTime);

            // For an invalid time range, simply show one hour, starting at the hour
            // before the given start time
            if (curEndTime <= curStartTime) {
                curStartTime = QTime(curStartTime.hour(), 0, 0);
                curEndTime = curStartTime.addSecs(3600);
            }

            drawHeader(p, local.toString(curDay, QLocale::ShortFormat), curDay, QDate(), headerBox);
            drawTimeTable(p, curDay, curDay, daysBox);
            if (mPrintFooter) {
                drawFooter(p, footerBox);
            }

            curDay = curDay.addDays(1);
            if (curDay <= mToDate) {
                mPrinter->newPage();
            }
        } while (curDay <= mToDate);
    } // switch
}

/**************************************************************
 *           Print Week
 **************************************************************/

CalPrintWeek::CalPrintWeek()
    : CalPrintTimetable()
{
}

CalPrintWeek::~CalPrintWeek()
{
}

QWidget *CalPrintWeek::createConfigWidget(QWidget *w)
{
    return new CalPrintWeekConfig(w);
}

void CalPrintWeek::readSettingsWidget()
{
    auto cfg = dynamic_cast<CalPrintWeekConfig *>((QWidget *)mConfigWidget);
    if (cfg) {
        mFromDate = cfg->mFromDate->date();
        mToDate = cfg->mToDate->date();

        if (cfg->mPrintTypeFilofax->isChecked()) {
            mWeekPrintType = Filofax;
        } else if (cfg->mPrintTypeTimetable->isChecked()) {
            mWeekPrintType = Timetable;
        } else if (cfg->mPrintTypeSplitWeek->isChecked()) {
            mWeekPrintType = SplitWeek;
        } else {
            mWeekPrintType = Timetable;
        }

        mStartTime = cfg->mFromTime->time();
        mEndTime = cfg->mToTime->time();
        mIncludeAllEvents = cfg->mIncludeAllEvents->isChecked();

        mShowNoteLines = cfg->mShowNoteLines->isChecked();
        mSingleLineLimit = cfg->mSingleLineLimit->isChecked();
        mIncludeTodos = cfg->mIncludeTodos->isChecked();
        mUseColors = cfg->mColors->isChecked();
        mPrintFooter = cfg->mPrintFooter->isChecked();
        mIncludeDescription = cfg->mIncludeDescription->isChecked();
        mIncludeCategories = cfg->mIncludeCategories->isChecked();
        mExcludeTime = cfg->mExcludeTime->isChecked();
        mExcludeConfidential = cfg->mExcludeConfidential->isChecked();
        mExcludePrivate = cfg->mExcludePrivate->isChecked();
    }
}

void CalPrintWeek::setSettingsWidget()
{
    auto cfg = dynamic_cast<CalPrintWeekConfig *>((QWidget *)mConfigWidget);
    if (cfg) {
        cfg->mFromDate->setDate(mFromDate);
        cfg->mToDate->setDate(mToDate);

        cfg->mPrintTypeFilofax->setChecked(mWeekPrintType == Filofax);
        cfg->mPrintTypeTimetable->setChecked(mWeekPrintType == Timetable);
        cfg->mPrintTypeSplitWeek->setChecked(mWeekPrintType == SplitWeek);

        cfg->mFromTime->setTime(mStartTime);
        cfg->mToTime->setTime(mEndTime);
        cfg->mIncludeAllEvents->setChecked(mIncludeAllEvents);

        cfg->mShowNoteLines->setChecked(mShowNoteLines);
        cfg->mSingleLineLimit->setChecked(mSingleLineLimit);
        cfg->mIncludeTodos->setChecked(mIncludeTodos);
        cfg->mColors->setChecked(mUseColors);
        cfg->mPrintFooter->setChecked(mPrintFooter);
        cfg->mIncludeDescription->setChecked(mIncludeDescription);
        cfg->mIncludeCategories->setChecked(mIncludeCategories);
        cfg->mExcludeTime->setChecked(mExcludeTime);
        cfg->mExcludeConfidential->setChecked(mExcludeConfidential);
        cfg->mExcludePrivate->setChecked(mExcludePrivate);
    }
    CalPrintTimetable::setSettingsWidget();
}

void CalPrintWeek::doLoadConfig()
{
    CalPrintTimetable::doLoadConfig();
    if (mConfig) {
        KConfigGroup grp(mConfig, groupName());
        mWeekPrintType = (eWeekPrintType)(grp.readEntry("Print type", (int)Filofax));
    }
    setSettingsWidget();
}

void CalPrintWeek::doSaveConfig()
{
    readSettingsWidget();
    if (mConfig) {
        KConfigGroup grp(mConfig, groupName());
        grp.writeEntry("Print type", int(mWeekPrintType));
    }
    CalPrintTimetable::doSaveConfig();
}

QPageLayout::Orientation CalPrintWeek::defaultOrientation() const
{
    if (mWeekPrintType == Filofax) {
        return QPageLayout::Portrait;
    } else if (mWeekPrintType == SplitWeek) {
        return QPageLayout::Portrait;
    } else {
        return QPageLayout::Landscape;
    }
}

void CalPrintWeek::setDateRange(const QDate &from, const QDate &to)
{
    CalPrintPluginBase::setDateRange(from, to);
    auto cfg = dynamic_cast<CalPrintWeekConfig *>((QWidget *)mConfigWidget);
    if (cfg) {
        cfg->mFromDate->setDate(from);
        cfg->mToDate->setDate(to);
    }
}

void CalPrintWeek::drawWeek(QPainter &p,
                                  QDate qd,
                                  QRect box)
{
    QDate weekDate = qd;
    const bool portrait = (box.height() > box.width());
    int cellWidth;
    int vcells;
    if (portrait) {
        cellWidth = box.width() / 2;
        vcells = 3;
    } else {
        cellWidth = box.width() / 6;
        vcells = 1;
    }
    const int cellHeight = box.height() / vcells;

    // correct begin of week
    int weekdayCol = weekdayColumn(qd.dayOfWeek());
    weekDate = qd.addDays(-weekdayCol);

    for (int i = 0; i < 7; ++i, weekDate = weekDate.addDays(1)) {
        // Saturday and sunday share a cell, so we have to special-case sunday
        int hpos = ((i < 6) ? i : (i - 1)) / vcells;
        int vpos = ((i < 6) ? i : (i - 1)) % vcells;
        QRect dayBox(box.left() + cellWidth * hpos,
                     box.top() + cellHeight * vpos + ((i == 6) ? (cellHeight / 2) : 0),
                     cellWidth,
                     (i < 5) ? (cellHeight) : (cellHeight / 2));
        drawDayBox(p,
                   weekDate,
                   mStartTime,
                   mEndTime,
                   dayBox,
                   true,
                   true,
                   true,
                   mSingleLineLimit,
                   mIncludeDescription,
                   mIncludeCategories);
    } // for i through all weekdays
}

void CalPrintWeek::print(QPainter &p, int width, int height)
{
    QDate curWeek, fromWeek, toWeek;

    // correct begin and end to first and last day of week
    int weekdayCol = weekdayColumn(mFromDate.dayOfWeek());
    fromWeek = mFromDate.addDays(-weekdayCol);
    weekdayCol = weekdayColumn(mToDate.dayOfWeek());
    toWeek = mToDate.addDays(6 - weekdayCol);

    curWeek = fromWeek.addDays(6);
    auto local = QLocale::system();

    QString line1, line2, title;
    QRect headerBox(0, 0, width, headerHeight());
    QRect footerBox(0, height - footerHeight(), width, footerHeight());
    height -= footerHeight();

    QRect weekBox(headerBox);
    weekBox.setTop(headerBox.bottom() + padding());
    weekBox.setBottom(height);

    switch (mWeekPrintType) {
    case Filofax:
        do {
            line1 = local.toString(curWeek.addDays(-6), QLocale::ShortFormat);
            line2 = local.toString(curWeek, QLocale::ShortFormat);
            title =  i18nc("date from-to", "%1\u2013%2", line1, line2);
            drawHeader(p, title, curWeek.addDays(-6), QDate(), headerBox);

            drawWeek(p, curWeek, weekBox);

            if (mPrintFooter) {
                drawFooter(p, footerBox);
            }

            curWeek = curWeek.addDays(7);
            if (curWeek <= toWeek) {
                mPrinter->newPage();
            }
        } while (curWeek <= toWeek);
        break;

    case Timetable:
    default:
        do {
            line1 = local.toString(curWeek.addDays(-6), QLocale::ShortFormat);
            line2 = local.toString(curWeek, QLocale::ShortFormat);
            if (orientation() == QPageLayout::Landscape) {
                title = i18nc("date from - to (week number)", "%1\u2013%2 (Week %3)", line1, line2, curWeek.weekNumber());
            } else {
                title = i18nc("date from - to\\n(week number)", "%1\u2013%2\n(Week %3)", line1, line2, curWeek.weekNumber());
            }
            drawHeader(p, title, curWeek, QDate(), headerBox);

            drawTimeTable(p, fromWeek, curWeek, weekBox);

            if (mPrintFooter) {
                drawFooter(p, footerBox);
            }

            fromWeek = fromWeek.addDays(7);
            curWeek = fromWeek.addDays(6);
            if (curWeek <= toWeek) {
                mPrinter->newPage();
            }
        } while (curWeek <= toWeek);
        break;

    case SplitWeek: {
        QRect weekBox1(weekBox);
        // On the left side there are four days (mo-th) plus the timeline,
        // on the right there are only three days (fr-su) plus the timeline. Don't
        // use the whole width, but rather give them the same width as on the left.
        weekBox1.setRight(int((width - TIMELINE_WIDTH) * 3. / 4. + TIMELINE_WIDTH));
        do {
            QDate endLeft(fromWeek.addDays(3));
            int hh = headerHeight();

            drawSplitHeaderRight(p, fromWeek, curWeek, QDate(), width, hh);
            drawTimeTable(p, fromWeek, endLeft, weekBox);
            if (mPrintFooter) {
                drawFooter(p, footerBox);
            }
            mPrinter->newPage();
            drawSplitHeaderRight(p, fromWeek, curWeek, QDate(), width, hh);
            drawTimeTable(p, endLeft.addDays(1), curWeek, weekBox1);

            if (mPrintFooter) {
                drawFooter(p, footerBox);
            }

            fromWeek = fromWeek.addDays(7);
            curWeek = fromWeek.addDays(6);
            if (curWeek <= toWeek) {
                mPrinter->newPage();
            }
        } while (curWeek <= toWeek);
        break;
    }
    }
}

/**************************************************************
 *           Print Month
 **************************************************************/

CalPrintMonth::CalPrintMonth()
    : CalPrintPluginBase()
{
}

CalPrintMonth::~CalPrintMonth()
{
}

QWidget *CalPrintMonth::createConfigWidget(QWidget *w)
{
    return new CalPrintMonthConfig(w);
}

void CalPrintMonth::readSettingsWidget()
{
    auto cfg = dynamic_cast<CalPrintMonthConfig *>((QWidget *)mConfigWidget);

    if (cfg) {
        mFromDate = QDate(cfg->mFromYear->value(), cfg->mFromMonth->currentIndex() + 1, 1);
        mToDate = QDate(cfg->mToYear->value(), cfg->mToMonth->currentIndex() + 1, 1);

        mWeekNumbers = cfg->mWeekNumbers->isChecked();
        mRecurDaily = cfg->mRecurDaily->isChecked();
        mRecurWeekly = cfg->mRecurWeekly->isChecked();
        mIncludeTodos = cfg->mIncludeTodos->isChecked();
        mShowNoteLines = cfg->mShowNoteLines->isChecked();
        mSingleLineLimit = cfg->mSingleLineLimit->isChecked();
        mUseColors = cfg->mColors->isChecked();
        mPrintFooter = cfg->mPrintFooter->isChecked();
        mIncludeDescription = cfg->mIncludeDescription->isChecked();
        mIncludeCategories = cfg->mIncludeCategories->isChecked();
        mExcludeConfidential = cfg->mExcludeConfidential->isChecked();
        mExcludePrivate = cfg->mExcludePrivate->isChecked();
    }
}

void CalPrintMonth::setSettingsWidget()
{
    auto cfg = dynamic_cast<CalPrintMonthConfig *>((QWidget *)mConfigWidget);

    if (cfg) {
        setDateRange(mFromDate, mToDate);

        cfg->mWeekNumbers->setChecked(mWeekNumbers);
        cfg->mRecurDaily->setChecked(mRecurDaily);
        cfg->mRecurWeekly->setChecked(mRecurWeekly);
        cfg->mIncludeTodos->setChecked(mIncludeTodos);
        cfg->mShowNoteLines->setChecked(mShowNoteLines);
        cfg->mSingleLineLimit->setChecked(mSingleLineLimit);
        cfg->mColors->setChecked(mUseColors);
        cfg->mPrintFooter->setChecked(mPrintFooter);
        cfg->mIncludeDescription->setChecked(mIncludeDescription);
        cfg->mIncludeCategories->setChecked(mIncludeCategories);
        cfg->mExcludeConfidential->setChecked(mExcludeConfidential);
        cfg->mExcludePrivate->setChecked(mExcludePrivate);
    }
}

void CalPrintMonth::doLoadConfig()
{
    CalPrintPluginBase::doLoadConfig();
    if (mConfig) {
        KConfigGroup grp(mConfig, groupName());
        mWeekNumbers = grp.readEntry("Print week numbers", true);
        mRecurDaily = grp.readEntry("Print daily incidences", true);
        mRecurWeekly = grp.readEntry("Print weekly incidences", true);
        mIncludeTodos = grp.readEntry("Include todos", false);
        mSingleLineLimit = grp.readEntry("Single line limit", false);
        mIncludeDescription = grp.readEntry("Include description", false);
        mIncludeCategories = grp.readEntry("Include categories", false);
    }
    setSettingsWidget();
}

void CalPrintMonth::doSaveConfig()
{
    readSettingsWidget();
    if (mConfig) {
        KConfigGroup grp(mConfig, groupName());
        grp.writeEntry("Print week numbers", mWeekNumbers);
        grp.writeEntry("Print daily incidences", mRecurDaily);
        grp.writeEntry("Print weekly incidences", mRecurWeekly);
        grp.writeEntry("Include todos", mIncludeTodos);
        grp.writeEntry("Single line limit", mSingleLineLimit);
        grp.writeEntry("Include description", mIncludeDescription);
        grp.writeEntry("Include categories", mIncludeCategories);
    }
    CalPrintPluginBase::doSaveConfig();
}

void CalPrintMonth::setDateRange(const QDate &from, const QDate &to)
{
    CalPrintPluginBase::setDateRange(from, to);
    auto cfg = dynamic_cast<CalPrintMonthConfig *>((QWidget *)mConfigWidget);
    if (cfg) {
        cfg->mFromMonth->clear();
        for (int i = 0; i < 12; ++i) {
            cfg->mFromMonth->addItem(QLocale().monthName(i + 1, QLocale::LongFormat));
        }
        cfg->mToMonth->clear();
        for (int i = 0; i < 12; ++i) {
            cfg->mToMonth->addItem(QLocale().monthName(i + 1, QLocale::LongFormat));
        }
        cfg->mFromMonth->setCurrentIndex(from.month() - 1);
        cfg->mFromYear->setValue(to.year());
        cfg->mToMonth->setCurrentIndex(mToDate.month() - 1);
        cfg->mToYear->setValue(mToDate.year());
    }
}

void CalPrintMonth::print(QPainter &p, int width, int height)
{
    QDate curMonth, fromMonth, toMonth;

    fromMonth = mFromDate.addDays(-(mFromDate.day() - 1));
    toMonth = mToDate.addDays(mToDate.daysInMonth() - mToDate.day());

    curMonth = fromMonth;

    QRect headerBox(0, 0, width, headerHeight());
    QRect footerBox(0, height - footerHeight(), width, footerHeight());
    height -= footerHeight();

    QRect monthBox(0, 0, width, height);
    monthBox.setTop(headerBox.bottom() + padding());

    do {
        QString title(i18nc("monthname year", "%1 %2", QLocale::system().monthName(curMonth.month()), QString::number(curMonth.year())));
        QDate tmp(fromMonth);
        int weekdayCol = weekdayColumn(tmp.dayOfWeek());
        tmp = tmp.addDays(-weekdayCol);

        drawHeader(p, title, curMonth.addMonths(-1), curMonth.addMonths(1), headerBox);
        drawMonthTable(p,
                       curMonth,
                       QTime(),
                       QTime(),
                       mWeekNumbers,
                       mRecurDaily,
                       mRecurWeekly,
                       mSingleLineLimit,
                       mIncludeDescription,
                       mIncludeCategories,
                       monthBox);

        if (mPrintFooter) {
            drawFooter(p, footerBox);
        }

        curMonth = curMonth.addDays(curMonth.daysInMonth());
        if (curMonth <= toMonth) {
            mPrinter->newPage();
        }
    } while (curMonth <= toMonth);
}

/**************************************************************
 *           Print Todos
 **************************************************************/

CalPrintTodos::CalPrintTodos()
    : CalPrintPluginBase()
{
    mTodoSortField = TodoFieldUnset;
    mTodoSortDirection = TodoDirectionUnset;
}

CalPrintTodos::~CalPrintTodos()
{
}

QWidget *CalPrintTodos::createConfigWidget(QWidget *w)
{
    return new CalPrintTodoConfig(w);
}

void CalPrintTodos::readSettingsWidget()
{
    auto cfg = dynamic_cast<CalPrintTodoConfig *>((QWidget *)mConfigWidget);

    if (cfg) {
        mPageTitle = cfg->mTitle->text();

        if (cfg->mPrintAll->isChecked()) {
            mTodoPrintType = TodosAll;
        } else if (cfg->mPrintUnfinished->isChecked()) {
            mTodoPrintType = TodosUnfinished;
        } else if (cfg->mPrintDueRange->isChecked()) {
            mTodoPrintType = TodosDueRange;
        } else {
            mTodoPrintType = TodosAll;
        }

        mFromDate = cfg->mFromDate->date();
        mToDate = cfg->mToDate->date();

        mIncludeDescription = cfg->mDescription->isChecked();
        mIncludePriority = cfg->mPriority->isChecked();
        mIncludeCategories = cfg->mCategories->isChecked();
        mIncludeStartDate = cfg->mStartDate->isChecked();
        mIncludeDueDate = cfg->mDueDate->isChecked();
        mIncludePercentComplete = cfg->mPercentComplete->isChecked();
        mConnectSubTodos = cfg->mConnectSubTodos->isChecked();
        mStrikeOutCompleted = cfg->mStrikeOutCompleted->isChecked();
        mExcludeConfidential = cfg->mExcludeConfidential->isChecked();
        mExcludePrivate = cfg->mExcludePrivate->isChecked();

        mTodoSortField = (eTodoSortField)cfg->mSortField->currentIndex();
        mTodoSortDirection = (eTodoSortDirection)cfg->mSortDirection->currentIndex();

        mPrintFooter = cfg->mPrintFooter->isChecked();
    }
}

void CalPrintTodos::setSettingsWidget()
{
    auto cfg = dynamic_cast<CalPrintTodoConfig *>((QWidget *)mConfigWidget);
    if (cfg) {
        cfg->mTitle->setText(mPageTitle);

        cfg->mPrintAll->setChecked(mTodoPrintType == TodosAll);
        cfg->mPrintUnfinished->setChecked(mTodoPrintType == TodosUnfinished);
        cfg->mPrintDueRange->setChecked(mTodoPrintType == TodosDueRange);

        cfg->mFromDate->setDate(mFromDate);
        cfg->mToDate->setDate(mToDate);

        cfg->mDescription->setChecked(mIncludeDescription);
        cfg->mPriority->setChecked(mIncludePriority);
        cfg->mCategories->setChecked(mIncludeCategories);
        cfg->mStartDate->setChecked(mIncludeStartDate);
        cfg->mDueDate->setChecked(mIncludeDueDate);
        cfg->mPercentComplete->setChecked(mIncludePercentComplete);
        cfg->mConnectSubTodos->setChecked(mConnectSubTodos);
        cfg->mStrikeOutCompleted->setChecked(mStrikeOutCompleted);
        cfg->mExcludeConfidential->setChecked(mExcludeConfidential);
        cfg->mExcludePrivate->setChecked(mExcludePrivate);

        if (mTodoSortField != TodoFieldUnset) {
            // do not insert if already done so.
            cfg->mSortField->addItem(i18nc("@option sort by title", "Title"));
            cfg->mSortField->addItem(i18nc("@option sort by start date/time", "Start Date"));
            cfg->mSortField->addItem(i18nc("@option sort by due date/time", "Due Date"));
            cfg->mSortField->addItem(i18nc("@option sort by priority", "Priority"));
            cfg->mSortField->addItem(i18nc("@option sort by percent completed", "Percent Complete"));
            cfg->mSortField->addItem(i18nc("@option sort by tags", "Tags"));
            cfg->mSortField->setCurrentIndex(mTodoSortField);
        }

        if (mTodoSortDirection != TodoDirectionUnset) {
            // do not insert if already done so.
            cfg->mSortDirection->addItem(i18nc("@option sort in increasing order", "Ascending"));
            cfg->mSortDirection->addItem(i18nc("@option sort in descreasing order", "Descending"));
            cfg->mSortDirection->setCurrentIndex(mTodoSortDirection);
        }

        cfg->mPrintFooter->setChecked(mPrintFooter);
    }
}

void CalPrintTodos::doLoadConfig()
{
    CalPrintPluginBase::doLoadConfig();
    if (mConfig) {
        KConfigGroup grp(mConfig, groupName());
        mPageTitle = grp.readEntry("Page title", i18n("To-do list"));
        mTodoPrintType = (eTodoPrintType)grp.readEntry("Print type", static_cast<int>(TodosAll));
        mIncludeDescription = grp.readEntry("Include description", true);
        mIncludePriority = grp.readEntry("Include priority", true);
        mIncludeCategories = grp.readEntry("Include categories", true);
        mIncludeStartDate = grp.readEntry("Include start date", true);
        mIncludeDueDate = grp.readEntry("Include due date", true);
        mIncludePercentComplete = grp.readEntry("Include percentage completed", true);
        mConnectSubTodos = grp.readEntry("Connect subtodos", true);
        mStrikeOutCompleted = grp.readEntry("Strike out completed summaries", true);
        mTodoSortField = (eTodoSortField)grp.readEntry("Sort field", static_cast<int>(TodoFieldSummary));
        mTodoSortDirection = (eTodoSortDirection)grp.readEntry("Sort direction", static_cast<int>(TodoDirectionAscending));
    }
    setSettingsWidget();
}

void CalPrintTodos::doSaveConfig()
{
    readSettingsWidget();
    if (mConfig) {
        KConfigGroup grp(mConfig, groupName());
        grp.writeEntry("Page title", mPageTitle);
        grp.writeEntry("Print type", int(mTodoPrintType));
        grp.writeEntry("Include description", mIncludeDescription);
        grp.writeEntry("Include priority", mIncludePriority);
        grp.writeEntry("Include categories", mIncludeCategories);
        grp.writeEntry("Include start date", mIncludeStartDate);
        grp.writeEntry("Include due date", mIncludeDueDate);
        grp.writeEntry("Include percentage completed", mIncludePercentComplete);
        grp.writeEntry("Connect subtodos", mConnectSubTodos);
        grp.writeEntry("Strike out completed summaries", mStrikeOutCompleted);
        grp.writeEntry("Sort field", static_cast<int>(mTodoSortField));
        grp.writeEntry("Sort direction", static_cast<int>(mTodoSortDirection));
    }
    CalPrintPluginBase::doSaveConfig();
}

void CalPrintTodos::print(QPainter &p, int width, int height)
{
    int possummary = 100;

    QRect headerBox(0, 0, width, headerHeight());
    QRect footerBox(0, height - footerHeight(), width, footerHeight());
    height -= footerHeight();

    // Draw the First Page Header
    drawHeader(p, mPageTitle, mFromDate, QDate(), headerBox);

    // Estimate widths of some data columns.
    QFont oldFont(p.font());
    p.setFont(QFont(QStringLiteral("sans-serif"), 10));
    const int widDate = p.fontMetrics().boundingRect(QLocale::system().toString(QDate(2222, 12, 22), QLocale::ShortFormat)).width();
    const int widPct = p.fontMetrics().boundingRect(i18n("%1%", 100)).width() + 27;

    // Draw the Column Headers
    int mCurrentLinePos = headerHeight() + 5;
    QString outStr;

    p.setFont(QFont(QStringLiteral("sans-serif"), 9, QFont::Bold));
    int lineSpacing = p.fontMetrics().lineSpacing();
    mCurrentLinePos += lineSpacing;
    int pospriority = -1;
    if (mIncludePriority) {
        outStr += i18n("Priority");
        pospriority = 0;
        p.drawText(pospriority, mCurrentLinePos - 2, outStr);
    }

    int posSoFar = width;  // Position of leftmost optional header.

    int posdue = -1;
    if (mIncludeDueDate) {
        outStr.truncate(0);
        outStr += i18nc("@label to-do due date", "Due");
        const int widDue = std::max(p.fontMetrics().boundingRect(outStr).width(), widDate);
        posdue = posSoFar - widDue;
        p.drawText(posdue, mCurrentLinePos - 2, outStr);
        posSoFar = posdue;
    }

    int posStart = -1;
    if (mIncludeStartDate) {
        outStr.truncate(0);
        outStr += i18nc("@label to-do start date", "Start");
        const int widStart = std::max(p.fontMetrics().boundingRect(outStr).width(), widDate);
        posStart = posSoFar - widStart - 5;
        p.drawText(posStart, mCurrentLinePos - 2, outStr);
        posSoFar = posStart;
    }

    int poscomplete = -1;
    if (mIncludePercentComplete) {
        outStr.truncate(0);
        outStr += i18nc("@label to-do percentage complete", "Complete");
        const int widComplete = std::max(p.fontMetrics().boundingRect(outStr).width(), widPct);
        poscomplete = posSoFar - widComplete - 5;
        p.drawText(poscomplete, mCurrentLinePos - 2, outStr);
        posSoFar = poscomplete;
    }

    int posCategories = -1;
    if (mIncludeCategories) {
        outStr.truncate(0);
        outStr += i18nc("@label to-do categories", "Tags");
        const int widCats = std::max(p.fontMetrics().boundingRect(outStr).width(), 100); // Arbitrary!
        posCategories = posSoFar - widCats - 5;
        p.drawText(posCategories, mCurrentLinePos - 2, outStr);
    }

    p.setFont(QFont(QStringLiteral("sans-serif"), 10));

    KCalendarCore::Todo::List todoList;
    KCalendarCore::Todo::List tempList;

    KCalendarCore::SortDirection sortDirection = KCalendarCore::SortDirectionAscending;
    switch (mTodoSortDirection) {
    case TodoDirectionAscending:
        sortDirection = KCalendarCore::SortDirectionAscending;
        break;
    case TodoDirectionDescending:
        sortDirection = KCalendarCore::SortDirectionDescending;
        break;
    case TodoDirectionUnset:
        break;
    }

    KCalendarCore::TodoSortField sortField = KCalendarCore::TodoSortSummary;
    switch (mTodoSortField) {
    case TodoFieldSummary:
        sortField = KCalendarCore::TodoSortSummary;
        break;
    case TodoFieldStartDate:
        sortField = KCalendarCore::TodoSortStartDate;
        break;
    case TodoFieldDueDate:
        sortField = KCalendarCore::TodoSortDueDate;
        break;
    case TodoFieldPriority:
        sortField = KCalendarCore::TodoSortPriority;
        break;
    case TodoFieldPercentComplete:
        sortField = KCalendarCore::TodoSortPercentComplete;
        break;
    case TodoFieldCategories:
        sortField = KCalendarCore::TodoSortCategories;
        break;
    case TodoFieldUnset:
        break;
    }

    // Create list of to-dos which will be printed
    todoList = mCalendar->todos(sortField, sortDirection);
    switch (mTodoPrintType) {
    case TodosAll:
        break;
    case TodosUnfinished:
        for (const KCalendarCore::Todo::Ptr &todo : std::as_const(todoList)) {
            Q_ASSERT(todo);
            if (!todo->isCompleted()) {
                tempList.append(todo);
            }
        }
        todoList = tempList;
        break;
    case TodosDueRange:
        for (const KCalendarCore::Todo::Ptr &todo : std::as_const(todoList)) {
            Q_ASSERT(todo);
            if (todo->hasDueDate()) {
                if (todo->dtDue().date() >= mFromDate && todo->dtDue().date() <= mToDate) {
                    tempList.append(todo);
                }
            } else {
                tempList.append(todo);
            }
        }
        todoList = tempList;
        break;
    }

    // Print to-dos
    int count = 0;
    for (const KCalendarCore::Todo::Ptr &todo : std::as_const(todoList)) {
        if ((mExcludeConfidential && todo->secrecy() == KCalendarCore::Incidence::SecrecyConfidential)
            || (mExcludePrivate && todo->secrecy() == KCalendarCore::Incidence::SecrecyPrivate)) {
            continue;
        }
        // Skip sub-to-dos. They will be printed recursively in drawTodo()
        if (todo->relatedTo().isEmpty()) { // review(AKONADI_PORT)
            count++;
            drawTodo(count,
                     todo,
                     p,
                     sortField,
                     sortDirection,
                     mConnectSubTodos,
                     mStrikeOutCompleted,
                     mIncludeDescription,
                     pospriority,
                     possummary,
                     posCategories,
                     posStart,
                     posdue,
                     poscomplete,
                     0,
                     0,
                     mCurrentLinePos,
                     width,
                     height,
                     todoList,
                     nullptr);
        }
    }

    if (mPrintFooter) {
        drawFooter(p, footerBox);
    }
    p.setFont(oldFont);
}
