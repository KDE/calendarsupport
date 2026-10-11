/*
  This file is part of the kcalutils library.

  SPDX-FileCopyrightText: 2001 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  SPDX-FileCopyrightText: 2005 Rafal Rzepecki <divide@users.sourceforge.net>
  SPDX-FileCopyrightText: 2009-2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "incidenceformatter_p.h"

#include <calendarsupport_debug.h>

// #include "grantleetemplatemanager_p.h"
#if KCALENDARCORE_VERSION < QT_VERSION_CHECK(6, 30, 0)
#include <KCalUtils/Stringify>
#endif

#include <GrantleeTheme/GrantleeKi18nLocalizer>

#include <KTextTemplate/Engine>
#include <KTextTemplate/FileSystemTemplateLoader>
#include <KTextTemplate/Template>

#include <KCalendarCore/Event>
#include <KCalendarCore/Exceptions>
#include <KCalendarCore/FreeBusy>
#include <KCalendarCore/Journal>
#include <KCalendarCore/Todo>
#include <KCalendarCore/Visitor>
using namespace KCalendarCore;

#include <KIdentityManagementCore/Utils>

#include <ktexttohtml.h>

#if KCOREADDONS_VERSION >= QT_VERSION_CHECK(6, 31, 0)
#include <KFormat>
#endif
#include <KIconLoader>
#include <KLocalizedString>

#include <QLocale>
#include <QTextDocumentFragment>

using namespace Qt::Literals;
using namespace CalendarSupport;

static QString renderTemplate(const QString &templateName, const QVariantHash &data)
{
    KTextTemplate::Engine engine;
    QSharedPointer<KTextTemplate::FileSystemTemplateLoader> loader(new KTextTemplate::FileSystemTemplateLoader);
    loader->setTemplateDirs({u":/"_s});
    engine.addTemplateLoader(loader);
    engine.addDefaultLibrary(QStringLiteral("ktexttemplate_i18ntags"));
    engine.addDefaultLibrary(QStringLiteral("kde_grantlee_plugin"));
    engine.setSmartTrimEnabled(true);

    if (!loader->canLoadTemplate(templateName)) {
        qWarning() << "Cannot load template" << templateName << ", please check your installation";
        return QString();
    }

    KTextTemplate::Template const tpl = loader->loadByName(templateName, &engine);
    if (tpl->error()) {
        return tpl->errorString();
    }

    KTextTemplate::Context ctx;
    ctx.insert(QStringLiteral("incidence"), data);
    QSharedPointer<GrantleeTheme::GrantleeKi18nLocalizer> localizer(new GrantleeTheme::GrantleeKi18nLocalizer);
    localizer->setApplicationDomain("calendarsupport6");
    ctx.setLocalizer(localizer);
    return tpl->render(&ctx);
}

/*******************
 *  General helpers
 *******************/

/*!
  Returns a reminder string list computed for the specified Incidence.
  Each item of the returning QStringList corresponds to a string
  representation of a reminder belonging to this incidence.
  \param incidence a pointer to the Incidence
  \param shortfmt if true, a short version of each reminder is printed; else a longer version
  \return a list of formatted reminder strings
*/
static QStringList reminderStringList(const KCalendarCore::Incidence::Ptr &incidence, bool shortfmt = true);

/*!
  Returns a duration string computed for the specified Incidence.
  \param incidence a pointer to the Incidence
  \return the duration string
*/
static QString durationString(const KCalendarCore::Incidence::Ptr &incidence);

//@cond PRIVATE
static QString cleanHtml(const QString &html)
{
    return QTextDocumentFragment::fromHtml(html).toPlainText();
}

[[nodiscard]] static QString string2HTML(const QString &str)
{
    // use convertToHtml so we get clickable links and other goodies
    return KTextToHTML::convertToHtml(str, KTextToHTML::HighlightText | KTextToHTML::ReplaceSmileys);
}

[[nodiscard]] static bool thatIsMe(const QString &email)
{
    return KIdentityManagementCore::thatIsMe(email);
}

[[nodiscard]] static QString searchName(const QString &email, const QString &name)
{
    const QString printName = name.isEmpty() ? email : name;
    return printName;
}

[[nodiscard]] static bool iamOrganizer(const Incidence::Ptr &incidence)
{
    // Check if the user is the organizer for this incidence

    if (!incidence) {
        return false;
    }

    return thatIsMe(incidence->organizer().email());
}

[[nodiscard]] static bool attendeeIsOrganizer(const Incidence::Ptr &incidence, const Attendee &attendee)
{
    if (incidence && !attendee.isNull() && (incidence->organizer().email() == attendee.email())) {
        return true;
    } else {
        return false;
    }
}

[[nodiscard]] static QString rsvpStatusIconName(Attendee::PartStat status)
{
    switch (status) {
    case Attendee::Accepted:
        return QStringLiteral("dialog-ok-apply");
    case Attendee::Declined:
        return QStringLiteral("dialog-cancel");
    case Attendee::NeedsAction: // NOLINT(bugprone-branch-clone)
        return QStringLiteral("help-about");
    case Attendee::InProcess:
        return QStringLiteral("help-about");
    case Attendee::Tentative:
        return QStringLiteral("dialog-ok");
    case Attendee::Delegated:
        return QStringLiteral("mail-forward");
    case Attendee::Completed:
        return QStringLiteral("mail-mark-read");
    default:
        return QString();
    }
}

//@endcond

/*******************************************************************
 *  Helper functions for the extensive display (display viewer)
 *******************************************************************/

//@cond PRIVATE
[[nodiscard]] static QVariantHash displayViewFormatPerson(const QString &email, const QString &name, const QString &iconName)
{
    QVariantHash personData;
    personData[QStringLiteral("icon")] = iconName;
    personData[QStringLiteral("name")] = name;
    personData[QStringLiteral("email")] = email;

    // Make the mailto link
    if (!email.isEmpty()) {
        Person const person(name, email);
        QString path = person.fullName().simplified();
        if (path.isEmpty() || path.startsWith(u'"')) {
            path = email;
        }
        QUrl mailto;
        mailto.setScheme(QStringLiteral("mailto"));
        mailto.setPath(path);

        personData[QStringLiteral("mailto")] = mailto.url();
    }

    return personData;
}

[[nodiscard]] static QVariantHash displayViewFormatPerson(const QString &email, const QString &name, Attendee::PartStat status)
{
    return displayViewFormatPerson(email, name, rsvpStatusIconName(status));
}

[[nodiscard]] static bool incOrganizerOwnsCalendar(const Incidence::Ptr &incidence)
{
    // PORTME!  Look at e35's CalHelper::incOrganizerOwnsCalendar

    // For now, use iamOrganizer() which is only part of the check
    return iamOrganizer(incidence);
}

[[nodiscard]] static QString displayViewFormatDescription(const Incidence::Ptr &incidence)
{
    const QString description = incidence->description();
    if (!description.isEmpty()) {
        if (!incidence->descriptionIsRich() && !description.startsWith(QLatin1StringView("<!DOCTYPE HTML"))) {
            // cleanHtml first since non rich text might have html tags (like "<p>whatever</p>")
            return string2HTML(cleanHtml(description));
        } else if (!description.startsWith(QLatin1StringView("<!DOCTYPE HTML"))) {
            return incidence->richDescription();
        } else {
            return description;
        }
    }

    return QString();
}

[[nodiscard]] static QVariantList displayViewFormatAttendeeRoleList(const Incidence::Ptr &incidence, Attendee::Role role, bool showStatus)
{
    QVariantList attendeeDataList;
    attendeeDataList.reserve(incidence->attendeeCount());

    const Attendee::List attendees = incidence->attendees();
    for (const auto &a : attendees) {
        if (a.role() != role) {
            // skip this role
            continue;
        }
        if (attendeeIsOrganizer(incidence, a)) {
            // skip attendee that is also the organizer
            continue;
        }
        QVariantHash attendeeData = displayViewFormatPerson(a.email(), a.name(), showStatus ? a.status() : Attendee::None);
        if (!a.delegator().isEmpty()) {
            attendeeData[QStringLiteral("delegator")] = a.delegator();
        }
        if (!a.delegate().isEmpty()) {
            attendeeData[QStringLiteral("delegate")] = a.delegate();
        }
        if (showStatus) {
#if KCALENDARCORE_VERSION < QT_VERSION_CHECK(6, 30, 0)
            attendeeData[QStringLiteral("status")] = KCalUtils::Stringify::attendeeStatus(a.status());
#else
            attendeeData[QStringLiteral("status")] = Attendee::statusName(a.status());
#endif
        }

        attendeeDataList << attendeeData;
    }

    return attendeeDataList;
}

[[nodiscard]] static QVariantHash displayViewFormatOrganizer(const Incidence::Ptr &incidence)
{
    // Add organizer link
    const int attendeeCount = incidence->attendees().count();
    if (attendeeCount > 1 || (attendeeCount == 1 && !attendeeIsOrganizer(incidence, incidence->attendees().at(0)))) {
        return displayViewFormatPerson(incidence->organizer().email(), incidence->organizer().name(), QStringLiteral("meeting-organizer"));
    }

    return QVariantHash();
}

[[nodiscard]] static QVariantList displayViewFormatAttachments(const Incidence::Ptr &incidence)
{
    const Attachment::List as = incidence->attachments();

    QVariantList dataList;
    dataList.reserve(as.count());

    for (auto it = as.cbegin(), end = as.cend(); it != end; ++it) {
        QVariantHash attData;
        if ((*it).isUri()) {
            QString name;
            if ((*it).uri().startsWith(QLatin1StringView("kmail:"))) {
                name = i18n("Show mail");
            } else {
                if ((*it).label().isEmpty()) {
                    name = (*it).uri();
                } else {
                    name = (*it).label();
                }
            }
            attData[QStringLiteral("uri")] = (*it).uri();
            attData[QStringLiteral("label")] = name;
        } else {
            attData[QStringLiteral("uri")] = QStringLiteral("ATTACH:%1").arg(QString::fromUtf8((*it).label().toUtf8().toBase64()));
            attData[QStringLiteral("label")] = (*it).label();
        }
        dataList << attData;
    }
    return dataList;
}

[[nodiscard]] static QVariantHash displayViewFormatBirthday(const Event::Ptr &event)
{
    if (!event) {
        return QVariantHash();
    }

    // It's callees duty to ensure this
    Q_ASSERT(event->customProperty("KABC", "BIRTHDAY") == QLatin1StringView("YES") || event->customProperty("KABC", "ANNIVERSARY") == QLatin1StringView("YES"));

    const QString name_1 = event->customProperty("KABC", "NAME-1");
    const QString email_1 = event->customProperty("KABC", "EMAIL-1");
    const KCalendarCore::Person p = Person::fromFullName(email_1);
    return displayViewFormatPerson(p.email(), name_1, QString());
}

[[nodiscard]] static QVariantHash incidenceTemplateHeader(const Incidence::Ptr &incidence)
{
    QVariantHash incidenceData;
    if (incidence->customProperty("KABC", "BIRTHDAY") == QLatin1StringView("YES")) {
        incidenceData[QStringLiteral("icon")] = QStringLiteral("view-calendar-birthday");
    } else if (incidence->customProperty("KABC", "ANNIVERSARY") == QLatin1StringView("YES")) {
        incidenceData[QStringLiteral("icon")] = QStringLiteral("view-calendar-wedding-anniversary");
    } else {
        incidenceData[QStringLiteral("icon")] = incidence->iconName();
    }

    switch (incidence->type()) {
    case IncidenceBase::IncidenceType::TypeEvent:
        incidenceData[QStringLiteral("alarmIcon")] = QStringLiteral("appointment-reminder");
        incidenceData[QStringLiteral("recursIcon")] = QStringLiteral("appointment-recurring");
        break;
    case IncidenceBase::IncidenceType::TypeTodo:
        incidenceData[QStringLiteral("alarmIcon")] = QStringLiteral("task-reminder");
        incidenceData[QStringLiteral("recursIcon")] = QStringLiteral("task-recurring");
        break;
    default:
        // Others don't repeat and don't have reminders.
        break;
    }

    incidenceData[QStringLiteral("hasEnabledAlarms")] = incidence->hasEnabledAlarms();
    incidenceData[QStringLiteral("recurs")] = incidence->recurs();
    incidenceData[QStringLiteral("isReadOnly")] = incidence->isReadOnly();
    incidenceData[QStringLiteral("summary")] = incidence->summary();
    incidenceData[QStringLiteral("allDay")] = incidence->allDay();

    return incidenceData;
}

[[nodiscard]] static QString displayViewFormatEvent(const QString &sourceName, const Event::Ptr &event, QDate date)
{
    if (!event) {
        return QString();
    }

    QVariantHash incidence = incidenceTemplateHeader(event);

    incidence[QStringLiteral("calendar")] = sourceName;
    const QString richLocation = event->richLocation();
    if (richLocation.startsWith(QLatin1StringView("http:/")) || richLocation.startsWith(QLatin1StringView("https:/"))) {
        incidence[QStringLiteral("location")] = QStringLiteral("<a href=\"%1\">%1</a>").arg(richLocation);
    } else {
        incidence[QStringLiteral("location")] = richLocation;
    }

    const auto startDts = event->startDateTimesForDate(date, QTimeZone::systemTimeZone());
    QDateTime startDt;
    QDateTime endDt;
    if (startDts.isEmpty()) {
        startDt = event->dtStart().toLocalTime();
        endDt = event->endDateForStart(startDt).toLocalTime();
    } else {
        if (event->recurs()) {
            // timezone is already applied by startDateTimesForDate
            startDt = startDts[0];
        } else {
            startDt = startDts[0].toLocalTime();
        }
        endDt = event->endDateForStart(startDt);
    }
    incidence[QStringLiteral("isAllDay")] = event->allDay();
    incidence[QStringLiteral("isMultiDay")] = event->isMultiDay();
    incidence[QStringLiteral("startDateTime")] = startDt;
    incidence[QStringLiteral("startDate")] = startDt.date();
    incidence[QStringLiteral("endDateTime")] = endDt;
    incidence[QStringLiteral("endDate")] = endDt.date();
    incidence[QStringLiteral("startTime")] = startDt.time();
    incidence[QStringLiteral("endTime")] = endDt.time();
    incidence[QStringLiteral("duration")] = durationString(event);
    incidence[QStringLiteral("isException")] = event->hasRecurrenceId();
#if KCALENDARCORE_VERSION < QT_VERSION_CHECK(6, 30, 0)
    incidence[QStringLiteral("recurrence")] = KCalUtils::Stringify::recurrenceString(event);
#else
    incidence[QStringLiteral("recurrence")] = event->recurrenceDescription();
#endif

    if (event->customProperty("KABC", "BIRTHDAY") == QLatin1StringView("YES")) {
        incidence[QStringLiteral("birthday")] = displayViewFormatBirthday(event);
    }

    if (event->customProperty("KABC", "ANNIVERSARY") == QLatin1StringView("YES")) {
        incidence[QStringLiteral("anniversary")] = displayViewFormatBirthday(event);
    }

    incidence[QStringLiteral("description")] = displayViewFormatDescription(event);
    // TODO: print comments?

    incidence[QStringLiteral("reminders")] = reminderStringList(event);
    incidence[QStringLiteral("organizer")] = displayViewFormatOrganizer(event);
    const bool showStatus = incOrganizerOwnsCalendar(event);
    incidence[QStringLiteral("chair")] = displayViewFormatAttendeeRoleList(event, Attendee::Chair, showStatus);
    incidence[QStringLiteral("requiredParticipants")] = displayViewFormatAttendeeRoleList(event, Attendee::ReqParticipant, showStatus);
    incidence[QStringLiteral("optionalParticipants")] = displayViewFormatAttendeeRoleList(event, Attendee::OptParticipant, showStatus);
    incidence[QStringLiteral("observers")] = displayViewFormatAttendeeRoleList(event, Attendee::NonParticipant, showStatus);
    incidence[QStringLiteral("categories")] = event->categories();
    incidence[QStringLiteral("attachments")] = displayViewFormatAttachments(event);
    incidence[QStringLiteral("creationDate")] = event->created().toLocalTime();
    incidence[QStringLiteral("modificationDate")] = event->lastModified().toLocalTime();
    incidence[QStringLiteral("revision")] = event->revision();

    return renderTemplate(QStringLiteral("org.kde.pim/calendarsupport/event.html"), incidence);
}

[[nodiscard]] static QString displayViewFormatTodo(const QString &sourceName, const Todo::Ptr &todo, QDate ocurrenceDueDate)
{
    if (!todo) {
        qCDebug(CALENDARSUPPORT_LOG) << "IncidenceFormatter::displayViewFormatTodo was called without to-do, quitting";
        return QString();
    }

    QVariantHash incidence = incidenceTemplateHeader(todo);

    incidence[QStringLiteral("calendar")] = sourceName;
    incidence[QStringLiteral("location")] = todo->richLocation();

    const bool hastStartDate = todo->hasStartDate();
    const bool hasDueDate = todo->hasDueDate();

    if (hastStartDate) {
        QDateTime startDt = todo->dtStart(true /**first*/).toLocalTime();
        if (todo->recurs() && ocurrenceDueDate.isValid()) {
            if (hasDueDate) {
                // In kdepim all recurring to-dos have due date.
                const qint64 length = startDt.daysTo(todo->dtDue(true /**first*/));
                if (length >= 0) {
                    startDt.setDate(ocurrenceDueDate.addDays(-length));
                } else {
                    qCritical() << "DTSTART is bigger than DTDUE, todo->uid() is " << todo->uid();
                    startDt.setDate(ocurrenceDueDate);
                }
            } else {
                qCritical() << "To-do is recurring but has no DTDUE set, todo->uid() is " << todo->uid();
                startDt.setDate(ocurrenceDueDate);
            }
        }
        incidence[QStringLiteral("startDate")] = startDt;
    }

    if (hasDueDate) {
        QDateTime dueDt = todo->dtDue().toLocalTime();
        if (todo->recurs()) {
            if (ocurrenceDueDate.isValid()) {
                QDateTime kdt(ocurrenceDueDate, QTime(0, 0, 0), QTimeZone::LocalTime);
                kdt = kdt.addSecs(-1);
                dueDt.setDate(todo->recurrence()->getNextDateTime(kdt).date());
            }
        }
        incidence[QStringLiteral("dueDate")] = dueDt;
    }

    incidence[QStringLiteral("duration")] = durationString(todo);
    incidence[QStringLiteral("isException")] = todo->hasRecurrenceId();
    if (todo->recurs()) {
#if KCALENDARCORE_VERSION < QT_VERSION_CHECK(6, 30, 0)
        incidence[QStringLiteral("recurrence")] = KCalUtils::Stringify::recurrenceString(todo);
#else
        incidence[QStringLiteral("recurrence")] = todo->recurrenceDescription();
#endif
    }

    incidence[QStringLiteral("description")] = displayViewFormatDescription(todo);

    // TODO: print comments?

    incidence[QStringLiteral("reminders")] = reminderStringList(todo);
    incidence[QStringLiteral("organizer")] = displayViewFormatOrganizer(todo);
    const bool showStatus = incOrganizerOwnsCalendar(todo);
    incidence[QStringLiteral("chair")] = displayViewFormatAttendeeRoleList(todo, Attendee::Chair, showStatus);
    incidence[QStringLiteral("requiredParticipants")] = displayViewFormatAttendeeRoleList(todo, Attendee::ReqParticipant, showStatus);
    incidence[QStringLiteral("optionalParticipants")] = displayViewFormatAttendeeRoleList(todo, Attendee::OptParticipant, showStatus);
    incidence[QStringLiteral("observers")] = displayViewFormatAttendeeRoleList(todo, Attendee::NonParticipant, showStatus);
    incidence[QStringLiteral("categories")] = todo->categories();
    incidence[QStringLiteral("priority")] = todo->priority();
    if (todo->isCompleted()) {
        incidence[QStringLiteral("completedDate")] = todo->completed();
    } else {
        incidence[QStringLiteral("percent")] = todo->percentComplete();
    }
    incidence[QStringLiteral("attachments")] = displayViewFormatAttachments(todo);
    incidence[QStringLiteral("creationDate")] = todo->created().toLocalTime();
    incidence[QStringLiteral("modificationDate")] = todo->lastModified().toLocalTime();
    incidence[QStringLiteral("revision")] = todo->revision();

    return renderTemplate(QStringLiteral("org.kde.pim/calendarsupport/todo.html"), incidence);
}

[[nodiscard]] static QString displayViewFormatJournal(const QString &sourceName, const Journal::Ptr &journal)
{
    if (!journal) {
        return QString();
    }

    QVariantHash incidence = incidenceTemplateHeader(journal);
    incidence[QStringLiteral("calendar")] = sourceName;
    incidence[QStringLiteral("date")] = journal->dtStart().toLocalTime();
    incidence[QStringLiteral("description")] = displayViewFormatDescription(journal);
    incidence[QStringLiteral("categories")] = journal->categories();
    incidence[QStringLiteral("creationDate")] = journal->created().toLocalTime();
    incidence[QStringLiteral("modificationDate")] = journal->lastModified().toLocalTime();
    incidence[QStringLiteral("revision")] = journal->revision();

    return renderTemplate(QStringLiteral("org.kde.pim/calendarsupport/journal.html"), incidence);
}

[[nodiscard]] static QString displayViewFormatFreeBusy([[maybe_unused]] const QString &sourceName, const FreeBusy::Ptr &fb)
{
    if (!fb) {
        return QString();
    }

    QVariantHash fbData;
    fbData[QStringLiteral("organizer")] = fb->organizer().fullName();
    fbData[QStringLiteral("start")] = fb->dtStart().toLocalTime().date();
    fbData[QStringLiteral("end")] = fb->dtEnd().toLocalTime().date();

    Period::List const periods = fb->busyPeriods();
    QVariantList periodsData;
    periodsData.reserve(periods.size());
    for (auto it = periods.cbegin(), end = periods.cend(); it != end; ++it) {
        const Period &per = *it;
        QVariantHash periodData;
        if (per.hasDuration()) {
            int dur = per.duration().asSeconds();
            QString cont;
            if (dur >= 3600) {
                cont += i18ncp("hours part of duration", "1 hour ", "%1 hours ", dur / 3600);
                dur %= 3600;
            }
            if (dur >= 60) {
                cont += i18ncp("minutes part duration", "1 minute ", "%1 minutes ", dur / 60);
                dur %= 60;
            }
            if (dur > 0) {
                cont += i18ncp("seconds part of duration", "1 second", "%1 seconds", dur);
            }
            periodData[QStringLiteral("dtStart")] = per.start().toLocalTime();
            periodData[QStringLiteral("duration")] = cont;
        } else {
            const QDateTime pStart = per.start().toLocalTime();
            const QDateTime pEnd = per.end().toLocalTime();
            if (per.start().date() == per.end().date()) {
                periodData[QStringLiteral("date")] = pStart.date();
                periodData[QStringLiteral("start")] = pStart.time();
                periodData[QStringLiteral("end")] = pEnd.time();
            } else {
                periodData[QStringLiteral("start")] = pStart;
                periodData[QStringLiteral("end")] = pEnd;
            }
        }

        periodsData << periodData;
    }

    fbData[QStringLiteral("periods")] = periodsData;

    return renderTemplate(QStringLiteral("org.kde.pim/calendarsupport/freebusy.html"), fbData);
}

//@endcond

//@cond PRIVATE
class CalendarSupport::IncidenceFormatter::EventViewerVisitor : public Visitor
{
public:
    EventViewerVisitor() = default;
    ~EventViewerVisitor() override;

    bool act(const QString &sourceName, const IncidenceBase::Ptr &incidence, QDate date)
    {
        mSourceName = sourceName;
        mDate = date;
        mResult = QLatin1StringView("");
        return incidence->accept(*this, incidence);
    }

    [[nodiscard]] const QString &result() const
    {
        return mResult;
    }

protected:
    bool visit(const Event::Ptr &event) override
    {
        mResult = displayViewFormatEvent(mSourceName, event, mDate);
        return !mResult.isEmpty();
    }

    bool visit(const Todo::Ptr &todo) override
    {
        mResult = displayViewFormatTodo(mSourceName, todo, mDate);
        return !mResult.isEmpty();
    }

    bool visit(const Journal::Ptr &journal) override
    {
        mResult = displayViewFormatJournal(mSourceName, journal);
        return !mResult.isEmpty();
    }

    bool visit(const FreeBusy::Ptr &fb) override
    {
        mResult = displayViewFormatFreeBusy(mSourceName, fb);
        return !mResult.isEmpty();
    }

protected:
    QString mSourceName;
    QDate mDate;
    QString mResult;
};
//@endcond

IncidenceFormatter::EventViewerVisitor::~EventViewerVisitor() = default;

QString IncidenceFormatter::extensiveDisplayStr(const QString &sourceName, const IncidenceBase::Ptr &incidence, QDate date)
{
    if (!incidence) {
        return QString();
    }

    EventViewerVisitor v;
    if (v.act(sourceName, incidence, date)) {
        return v.result();
    } else {
        return QString();
    }
}

/*******************************************************************
 *  Helper functions for the Incidence tooltips
 *******************************************************************/

//@cond PRIVATE
class CalendarSupport::IncidenceFormatter::ToolTipVisitor : public Visitor
{
public:
    ToolTipVisitor() = default;

    bool act(const QString &location, const IncidenceBase::Ptr &incidence, QDate date = QDate())
    {
        mLocation = location;
        mDate = date;
        mResult = QLatin1StringView("");
        return incidence ? incidence->accept(*this, incidence) : false;
    }

    [[nodiscard]] const QString &result() const
    {
        return mResult;
    }

protected:
    bool visit(const Event::Ptr &event) override;
    bool visit(const Todo::Ptr &todo) override;
    bool visit(const Journal::Ptr &journal) override;
    bool visit(const FreeBusy::Ptr &fb) override;

    [[nodiscard]] QString dateRangeText(const Event::Ptr &event, QDate date);
    [[nodiscard]] QString dateRangeText(const Todo::Ptr &todo, QDate asOfDate);
    [[nodiscard]] QString dateRangeText(const Journal::Ptr &journal);
    [[nodiscard]] QString dateRangeText(const FreeBusy::Ptr &fb);

    [[nodiscard]] QString generateToolTip(const Incidence::Ptr &incidence, const QString &dtRangeText);

protected:
    QString mLocation;
    QDate mDate;
    QString mResult;
};

QString IncidenceFormatter::ToolTipVisitor::dateRangeText(const Event::Ptr &event, QDate date)
{
    // FIXME: support mRichText==false
    QString ret;
    QString tmp;

    const auto startDts = event->startDateTimesForDate(date, QTimeZone::systemTimeZone());
    QDateTime startDt;
    QDateTime endDt;
    if (startDts.isEmpty()) {
        startDt = event->dtStart().toLocalTime();
        endDt = event->endDateForStart(startDt).toLocalTime();
    } else {
        if (event->recurs()) {
            // timezone is already applied by startDateTimesForDate
            startDt = startDts[0];
        } else {
            startDt = startDts[0].toLocalTime();
        }
        endDt = event->endDateForStart(startDt);
    }

    if (event->isMultiDay()) {
        if (event->allDay()) {
            tmp = QLocale().toString(startDt.date(), QLocale::LongFormat);
            ret += QLatin1StringView("<br>") + i18nc("Event start", "<i>From:</i> %1", tmp);
            tmp = QLocale().toString(endDt.date(), QLocale::LongFormat);
            ret += QLatin1StringView("<br>") + i18nc("Event end", "<i>To:</i> %1", tmp);
        } else {
            ret += QLatin1StringView("<br>") + i18nc("datetime range for event", "<i>Date:</i> %1 - %2", dateTimeToString(startDt), dateTimeToString(endDt));
        }
    } else {
        ret += QLatin1StringView("<br>") + i18n("<i>Date:</i> %1", QLocale().toString(startDt.date(), QLocale::LongFormat));
        if (!event->allDay()) {
            const QString dtStartTime = QLocale().toString(startDt.time(), QLocale::ShortFormat);
            const QString dtEndTime = QLocale().toString(endDt.time(), QLocale::ShortFormat);
            if (dtStartTime == dtEndTime) {
                // to prevent 'Time: 17:00 - 17:00'
                tmp = QLatin1StringView("<br>") + i18nc("time for event", "<i>Time:</i> %1", dtStartTime);
            } else {
                tmp = QLatin1StringView("<br>") + i18nc("time range for event", "<i>Time:</i> %1 - %2", dtStartTime, dtEndTime);
            }
            ret += tmp;
        }
    }
    return ret.replace(u' ', QLatin1StringView("&nbsp;"));
}

QString IncidenceFormatter::ToolTipVisitor::dateRangeText(const Todo::Ptr &todo, QDate asOfDate)
{
    // FIXME: support mRichText==false
    // FIXME: doesn't handle to-dos that occur more than once per day.

    QDateTime startDt{todo->dtStart(false)};
    QDateTime dueDt{todo->dtDue(false)};

    if (todo->recurs() && asOfDate.isValid()) {
        const QDateTime limit{asOfDate.addDays(1), QTime(0, 0, 0), QTimeZone::LocalTime};
        startDt = todo->recurrence()->getPreviousDateTime(limit);
        if (startDt.isValid() && todo->hasDueDate()) {
            if (todo->allDay()) {
                // Days, not seconds, because not all days are 24 hours long.
                const auto duration{todo->dtStart(true).daysTo(todo->dtDue(true))};
                dueDt = startDt.addDays(duration);
            } else {
                const auto duration{todo->dtStart(true).secsTo(todo->dtDue(true))};
                dueDt = startDt.addSecs(duration);
            }
        }
    }

    QString ret;
    if (startDt.isValid()) {
        ret = QLatin1StringView("<br>") % i18nc("To-do's start date", "<i>Start:</i> %1", dateTimeToString(startDt, todo->allDay(), false));
    }
    if (dueDt.isValid()) {
        ret += QLatin1StringView("<br>") % i18nc("To-do's due date", "<i>Due:</i> %1", dateTimeToString(dueDt, todo->allDay(), false));
    }

    // Print priority and completed info here, for lack of a better place

    if (todo->priority() > 0) {
        ret += QLatin1StringView("<br>")
            % i18nc("To-do's priority number", "<i>Priority:</i> %1", QString::number(todo->priority())); // krazy:exclude=i18ncheckarg
    }

    ret += QLatin1StringView("<br>");
    if (todo->hasCompletedDate()) {
        ret += i18nc("To-do's completed date", "<i>Completed:</i> %1", dateTimeToString(todo->completed().toLocalTime(), false, false));
    } else {
        int pct = todo->percentComplete();
        if (todo->recurs() && asOfDate.isValid()) {
            const QDate recurrenceDate = todo->dtRecurrence().date();
            if (recurrenceDate < startDt.date()) {
                pct = 0;
            } else if (recurrenceDate > startDt.date()) {
                pct = 100;
            }
        }
        ret += i18nc("To-do's percent complete:", "<i>Percent Done:</i> %1%", pct);
    }

    return ret.replace(u' ', QLatin1StringView("&nbsp;"));
}

QString IncidenceFormatter::ToolTipVisitor::dateRangeText(const Journal::Ptr &journal)
{
    // FIXME: support mRichText==false
    QString ret;
    if (journal->dtStart().isValid()) {
        ret += QLatin1StringView("<br>") + i18n("<i>Date:</i> %1", QLocale().toString(journal->dtStart().toLocalTime().date(), QLocale::LongFormat));
    }
    return ret.replace(u' ', QLatin1StringView("&nbsp;"));
}

QString IncidenceFormatter::ToolTipVisitor::dateRangeText(const FreeBusy::Ptr &fb)
{
    // FIXME: support mRichText==false
    QString ret = QLatin1StringView("<br>") + i18n("<i>Period start:</i> %1", QLocale().toString(fb->dtStart(), QLocale::ShortFormat));
    ret += QLatin1StringView("<br>") + i18n("<i>Period end:</i> %1", QLocale().toString(fb->dtEnd(), QLocale::ShortFormat));
    return ret.replace(u' ', QLatin1StringView("&nbsp;"));
}

bool IncidenceFormatter::ToolTipVisitor::visit(const Event::Ptr &event)
{
    mResult = generateToolTip(event, dateRangeText(event, mDate));
    return !mResult.isEmpty();
}

bool IncidenceFormatter::ToolTipVisitor::visit(const Todo::Ptr &todo)
{
    mResult = generateToolTip(todo, dateRangeText(todo, mDate));
    return !mResult.isEmpty();
}

bool IncidenceFormatter::ToolTipVisitor::visit(const Journal::Ptr &journal)
{
    mResult = generateToolTip(journal, dateRangeText(journal));
    return !mResult.isEmpty();
}

bool IncidenceFormatter::ToolTipVisitor::visit(const FreeBusy::Ptr &fb)
{
    // FIXME: support mRichText==false
    mResult = QLatin1StringView("<qt><b>") + i18n("Free/Busy information for %1", fb->organizer().fullName()) + QLatin1StringView("</b>");
    mResult += dateRangeText(fb);
    mResult += QLatin1StringView("</qt>");
    return !mResult.isEmpty();
}

[[nodiscard]] static QString tooltipPerson(const QString &email, const QString &name, Attendee::PartStat status)
{
    // Search for a new print name, if needed.
    const QString printName = searchName(email, name);

    // Get the icon corresponding to the attendee participation status.
    const QString iconPath = KIconLoader::global()->iconPath(rsvpStatusIconName(status), KIconLoader::Small);

    // Make the return string.
    QString personString;
    if (!iconPath.isEmpty()) {
        personString += QLatin1StringView(R"(<img valign="top" src=")") + iconPath + QLatin1StringView("\">") + QLatin1StringView("&nbsp;");
    }
    if (status != Attendee::None) {
#if KCALENDARCORE_VERSION < QT_VERSION_CHECK(6, 30, 0)
        personString +=
            i18nc("attendee name (attendee status)", "%1 (%2)", printName.isEmpty() ? email : printName, KCalUtils::Stringify::attendeeStatus(status));
#else
        personString += i18nc("attendee name (attendee status)", "%1 (%2)", printName.isEmpty() ? email : printName, Attendee::statusName(status));
#endif
    } else {
        personString += i18n("%1", printName.isEmpty() ? email : printName);
    }
    return personString;
}

[[nodiscard]] static QString tooltipFormatOrganizer(const QString &email, const QString &name)
{
    // Search for a new print name, if needed
    const QString printName = searchName(email, name);

    // Get the icon for organizer
    // TODO fixme laurent: use another icon. It doesn't exist in breeze.
    const QString iconPath = KIconLoader::global()->iconPath(QStringLiteral("meeting-organizer"), KIconLoader::Small, true);

    // Make the return string.
    QString personString;
    if (!iconPath.isEmpty()) {
        personString += QLatin1StringView(R"(<img valign="top" src=")") + iconPath + QLatin1StringView("\">") + QLatin1StringView("&nbsp;");
    }
    personString += (printName.isEmpty() ? email : printName);
    return personString;
}

[[nodiscard]] static QString tooltipFormatAttendeeRoleList(const Incidence::Ptr &incidence, Attendee::Role role, bool showStatus)
{
    int const maxNumAtts = 8; // maximum number of people to print per attendee role
    const QString etc = i18nc("ellipsis", "...");

    int i = 0;
    QString tmpStr;
    const Attendee::List attendees = incidence->attendees();
    for (const auto &a : attendees) {
        if (a.role() != role) {
            // skip not this role
            continue;
        }
        if (attendeeIsOrganizer(incidence, a)) {
            // skip attendee that is also the organizer
            continue;
        }
        if (i == maxNumAtts) {
            tmpStr += QLatin1StringView("&nbsp;&nbsp;") + etc;
            break;
        }
        tmpStr += QLatin1StringView("&nbsp;&nbsp;") + tooltipPerson(a.email(), a.name(), showStatus ? a.status() : Attendee::None);
        if (!a.delegator().isEmpty()) {
            tmpStr += i18n(" (delegated by %1)", a.delegator());
        }
        if (!a.delegate().isEmpty()) {
            tmpStr += i18n(" (delegated to %1)", a.delegate());
        }
        tmpStr += QLatin1StringView("<br>");
        i++;
    }
    if (tmpStr.endsWith(QLatin1StringView("<br>"))) {
        tmpStr.chop(4);
    }
    return tmpStr;
}

[[nodiscard]] static QString tooltipFormatAttendees(const Incidence::Ptr &incidence)
{
    QString tmpStr;
    QString str;

    // Add organizer link
    const auto attendees = incidence->attendees();
    const int attendeeCount = attendees.count();
    if (attendeeCount > 1 || (attendeeCount == 1 && !attendeeIsOrganizer(incidence, attendees.at(0)))) {
        tmpStr += QLatin1StringView("<i>") + i18n("Organizer:") + QLatin1StringView("</i>") + QLatin1StringView("<br>");
        tmpStr += QLatin1StringView("&nbsp;&nbsp;") + tooltipFormatOrganizer(incidence->organizer().email(), incidence->organizer().name());
    }

    // Show the attendee status if the incidence's organizer owns the resource calendar,
    // which means they are running the show and have all the up-to-date response info.
    const bool showStatus = attendeeCount > 0 && incOrganizerOwnsCalendar(incidence);

    // Add "chair"
    str = tooltipFormatAttendeeRoleList(incidence, Attendee::Chair, showStatus);
    if (!str.isEmpty()) {
        tmpStr += QLatin1StringView("<br><i>") + i18n("Chair:") + QLatin1StringView("</i>") + QLatin1StringView("<br>");
        tmpStr += str;
    }

    // Add required participants
    str = tooltipFormatAttendeeRoleList(incidence, Attendee::ReqParticipant, showStatus);
    if (!str.isEmpty()) {
        tmpStr += QLatin1StringView("<br><i>") + i18n("Required Participants:") + QLatin1StringView("</i>") + QLatin1StringView("<br>");
        tmpStr += str;
    }

    // Add optional participants
    str = tooltipFormatAttendeeRoleList(incidence, Attendee::OptParticipant, showStatus);
    if (!str.isEmpty()) {
        tmpStr += QLatin1StringView("<br><i>") + i18n("Optional Participants:") + QLatin1StringView("</i>") + QLatin1StringView("<br>");
        tmpStr += str;
    }

    // Add observers
    str = tooltipFormatAttendeeRoleList(incidence, Attendee::NonParticipant, showStatus);
    if (!str.isEmpty()) {
        tmpStr += QLatin1StringView("<br><i>") + i18n("Observers:") + QLatin1StringView("</i>") + QLatin1StringView("<br>");
        tmpStr += str;
    }

    return tmpStr;
}

QString IncidenceFormatter::ToolTipVisitor::generateToolTip(const Incidence::Ptr &incidence, const QString &dtRangeText)
{
    // FIXME: support mRichText==false
    if (!incidence) {
        return QString();
    }

    QString tmp = QStringLiteral("<qt>");

    // header
    tmp += QLatin1StringView("<b>") + incidence->richSummary() + QLatin1StringView("</b>");
    tmp += QLatin1StringView("<hr>");

    const QString calStr = mLocation;
    if (!calStr.isEmpty()) {
        tmp += QLatin1StringView("<i>") + i18n("Calendar:") + QLatin1StringView("</i>") + QLatin1StringView("&nbsp;");
        tmp += calStr;
    }

    tmp += dtRangeText;

    if (!incidence->location().isEmpty()) {
        tmp += QLatin1StringView("<br>");
        tmp += QLatin1StringView("<i>") + i18n("Location:") + QLatin1StringView("</i>") + QLatin1StringView("&nbsp;");
        tmp += incidence->richLocation();
    }

    QString const durStr = durationString(incidence);
    if (!durStr.isEmpty()) {
        tmp += QLatin1StringView("<br>");
        tmp += QLatin1StringView("<i>") + i18n("Duration:") + QLatin1StringView("</i>") + QLatin1StringView("&nbsp;");
        tmp += durStr;
    }

    if (incidence->recurs()) {
        tmp += QLatin1StringView("<br>");
        tmp += QLatin1StringView("<i>") + i18n("Recurrence:") + QLatin1StringView("</i>") + QLatin1StringView("&nbsp;");
#if KCALENDARCORE_VERSION < QT_VERSION_CHECK(6, 30, 0)
        tmp += KCalUtils::Stringify::recurrenceString(incidence);
#else
        tmp += incidence->recurrenceDescription();
#endif
    }

    if (incidence->hasRecurrenceId()) {
        tmp += QLatin1StringView("<br>");
        tmp += QLatin1StringView("<i>") + i18n("Recurrence:") + QLatin1StringView("</i>") + QLatin1StringView("&nbsp;");
        tmp += i18n("Exception");
    }

    if (!incidence->description().isEmpty()) {
        QString desc(incidence->description());
        if (!incidence->descriptionIsRich()) {
            int const maxDescLen = 120; // maximum description chars to print (before ellipsis)
            if (desc.length() > maxDescLen) {
                desc = desc.left(maxDescLen) + i18nc("ellipsis", "...");
            }
            // cleanHtml first since non rich text might have html tags (like "<p>whatever</p>")
            desc = cleanHtml(desc).replace(u'\n', QLatin1StringView("<br>"));
        } else {
            // TODO: truncate the description when it's rich text
        }
        tmp += QLatin1StringView("<hr>");
        tmp += QLatin1StringView("<i>") + i18n("Description:") + QLatin1StringView("</i>") + QLatin1StringView("<br>");
        tmp += desc;
    }

    bool needAnHorizontalLine = true;
    const int reminderCount = incidence->alarms().count();
    if (reminderCount > 0 && incidence->hasEnabledAlarms()) {
        /* cppcheck-suppress knownConditionTrueFalse */
        if (needAnHorizontalLine) {
            tmp += QLatin1StringView("<hr>");
            needAnHorizontalLine = false;
        } else {
            tmp += QLatin1StringView("<br>");
        }
        tmp += QLatin1StringView("<i>") + i18np("Reminder:", "Reminders:", reminderCount) + QLatin1StringView("</i>") + QLatin1StringView("&nbsp;");
        if (reminderCount > 1) {
            tmp += QLatin1StringView("<br> * ");
            tmp += reminderStringList(incidence).join(QLatin1StringView("<br> * "));
        } else {
            tmp += reminderStringList(incidence).join(QLatin1StringView("<br>"));
        }
    }

    const QString attendees = tooltipFormatAttendees(incidence);
    if (!attendees.isEmpty()) {
        if (needAnHorizontalLine) {
            tmp += QLatin1StringView("<hr>");
            needAnHorizontalLine = false;
        } else {
            tmp += QLatin1StringView("<br>");
        }
        tmp += attendees;
    }

    const QStringList categories = incidence->categories();
    int const categoryCount = categories.count();
    if (categoryCount > 0) {
        if (needAnHorizontalLine) {
            tmp += QLatin1StringView("<hr>");
        } else {
            tmp += QLatin1StringView("<br>");
        }
        tmp += QLatin1StringView("<i>") + i18np("Tag:", "Tags:", categoryCount) + QLatin1StringView("</i>") + QLatin1StringView("&nbsp;");
        tmp += categories.join(QLatin1StringView(", "));
    }

    tmp += QLatin1StringView("</qt>");
    return tmp;
}

//@endcond

QString IncidenceFormatter::toolTipStr(const QString &sourceName, const IncidenceBase::Ptr &incidence, QDate date)
{
    ToolTipVisitor v;
    if (incidence && v.act(sourceName, incidence, date)) {
        return v.result();
    } else {
        return QString();
    }
}

QString IncidenceFormatter::dateTimeToString(const QDateTime &date, bool dateOnly, bool shortfmt)
{
    if (dateOnly) {
        return QLocale().toString(date.toLocalTime().date(), shortfmt ? QLocale::ShortFormat : QLocale::LongFormat);
    }

#if KCOREADDONS_VERSION < QT_VERSION_CHECK(6, 31, 0)
    return QLocale().toString(date.toLocalTime(), (shortfmt ? QLocale::ShortFormat : QLocale::LongFormat));
#else
    const KFormat format;
    return format.formatDateTime(date.toLocalTime(), (shortfmt ? QLocale::ShortFormat : QLocale::LongFormat), KFormat::AddTimezoneAbbreviationIfNeeded);
#endif
}

static QString secs2Duration(qint64 secs)
{
    QString tmp;
    qint64 const days = secs / 86400;
    if (days > 0) {
        tmp += i18np("1 day", "%1 days", days);
        tmp += u' ';
        secs -= (days * 86400);
    }
    qint64 const hours = secs / 3600;
    if (hours > 0) {
        tmp += i18np("1 hour", "%1 hours", hours);
        tmp += u' ';
        secs -= (hours * 3600);
    }
    qint64 const mins = secs / 60;
    if (mins > 0) {
        tmp += i18np("1 minute", "%1 minutes", mins);
    }
    return tmp;
}

QString durationString(const Incidence::Ptr &incidence)
{
    QString tmp;
    if (incidence->type() == Incidence::TypeEvent) {
        Event::Ptr const event = incidence.staticCast<Event>();
        if (event->hasEndDate()) {
            if (!event->allDay()) {
                tmp = secs2Duration(event->dtStart().secsTo(event->dtEnd()));
            } else {
                tmp = i18np("1 day", "%1 days", event->dtStart().date().daysTo(event->dtEnd().date()) + 1);
            }
        } else {
            tmp = i18n("forever");
        }
    } else if (incidence->type() == Incidence::TypeTodo) {
        Todo::Ptr const todo = incidence.staticCast<Todo>();
        if (todo->hasDueDate()) {
            if (todo->hasStartDate()) {
                if (!todo->allDay()) {
                    tmp = secs2Duration(todo->dtStart().secsTo(todo->dtDue()));
                } else {
                    tmp = i18np("1 day", "%1 days", todo->dtStart().date().daysTo(todo->dtDue().date()) + 1);
                }
            }
        }
    }
    return tmp;
}

QStringList reminderStringList(const Incidence::Ptr &incidence, [[maybe_unused]] bool shortfmt)
{
    // TODO: implement shortfmt=false
    QStringList reminderStringList;

    if (incidence) {
        Alarm::List const alarms = incidence->alarms();
        Alarm::List::ConstIterator it;
        const Alarm::List::ConstIterator end(alarms.constEnd());
        reminderStringList.reserve(alarms.count());
        for (it = alarms.constBegin(); it != end; ++it) {
            const Alarm::Ptr &alarm = *it;
            int offset = 0;
            QString remStr;
            QString atStr;
            QString offsetStr;
            if (alarm->hasTime()) {
                offset = 0;
                if (alarm->time().isValid()) {
                    atStr = QLocale().toString(alarm->time().toLocalTime(), QLocale::ShortFormat);
                }
            } else if (alarm->hasStartOffset()) {
                offset = alarm->startOffset().asSeconds();
                if (offset < 0) {
                    offset = -offset;
                    offsetStr = i18nc("N days/hours/minutes before the start datetime", "%1 before the start", secs2Duration(offset));
                } else if (offset > 0) {
                    offsetStr = i18nc("N days/hours/minutes after the start datetime", "%1 after the start", secs2Duration(offset));
                } else { // offset is 0
                    if (incidence->dtStart().isValid()) {
                        atStr = QLocale().toString(incidence->dtStart().toLocalTime(), QLocale::ShortFormat);
                    }
                }
            } else if (alarm->hasEndOffset()) {
                offset = alarm->endOffset().asSeconds();
                if (offset < 0) {
                    offset = -offset;
                    if (incidence->type() == Incidence::TypeTodo) {
                        offsetStr = i18nc("N days/hours/minutes before the due datetime", "%1 before the to-do is due", secs2Duration(offset));
                    } else {
                        offsetStr = i18nc("N days/hours/minutes before the end datetime", "%1 before the end", secs2Duration(offset));
                    }
                } else if (offset > 0) {
                    if (incidence->type() == Incidence::TypeTodo) {
                        offsetStr = i18nc("N days/hours/minutes after the due datetime", "%1 after the to-do is due", secs2Duration(offset));
                    } else {
                        offsetStr = i18nc("N days/hours/minutes after the end datetime", "%1 after the end", secs2Duration(offset));
                    }
                } else { // offset is 0
                    if (incidence->type() == Incidence::TypeTodo) {
                        Todo::Ptr const t = incidence.staticCast<Todo>();
                        if (t->dtDue().isValid()) {
                            atStr = QLocale().toString(t->dtDue().toLocalTime(), QLocale::ShortFormat);
                        }
                    } else {
                        Event::Ptr const e = incidence.staticCast<Event>();
                        if (e->dtEnd().isValid()) {
                            atStr = QLocale().toString(e->dtEnd().toLocalTime(), QLocale::ShortFormat);
                        }
                    }
                }
            }
            if (offset == 0) {
                if (!atStr.isEmpty()) {
                    remStr = i18nc("reminder occurs at datetime", "at %1", atStr);
                }
            } else {
                remStr = std::move(offsetStr);
            }

            if (alarm->repeatCount() > 0) {
                QString const countStr = i18np("repeats once", "repeats %1 times", alarm->repeatCount());
                QString const intervalStr = i18nc("interval is N days/hours/minutes", "interval is %1", secs2Duration(alarm->snoozeTime().asSeconds()));
                QString repeatStr = i18nc("(repeat string, interval string)", "(%1, %2)", countStr, intervalStr);
                remStr = remStr + u' ' + repeatStr;
            }
            QStringList types;
            if (!alarm->enabled()) {
                types << i18nc("alarm is disabled", "disabled");
            }
            // FYI: we no longer support email or procedure alarms. and no need to pollute the output with "display"
            if (alarm->type() == KCalendarCore::Alarm::Audio) {
                types << i18nc("alarm will play a sound", "audio");
            }
            if (!types.isEmpty()) {
                remStr = i18nc("the reminder string with its types list", "%1 (%2)", remStr, types.join(QLatin1StringView(", ")));
            }
            reminderStringList << std::move(remStr);
        }
    }

    return reminderStringList;
}
