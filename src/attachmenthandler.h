/*
  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

/**
  @file
  This file is part of the API for handling calendar data and provides
  static functions for dealing with calendar incidence attachments.

  @author Allen Winter \<winter@kde.org\>
*/
#pragma once

#include <KCalendarCore/Attachment>
#include <KCalendarCore/Incidence>
#include <KCalendarCore/ScheduleMessage>

#include <QObject>

class KJob;

class QWidget;

namespace CalendarSupport
{
/**
  @brief
  Provides methods to handle incidence attachments.

  Includes functions to view and save attachments.
*/
class AttachmentHandler : public QObject
{
    Q_OBJECT
public:
    /**
     * Constructs an AttachmentHandler.
     * @param parent is the parent widget for the dialogs used by this class.
     */
    explicit AttachmentHandler(QWidget *parent);
    ~AttachmentHandler() override;

    /**
     * Finds the attachment in the user's calendar, by @p attachmentName and @p incidence.
     *
     * @param attachmentName is the name of the attachment
     * @param incidence is a pointer to a valid Incidence object containing the attachment.
     * @return a pointer to the Attachment object located; 0 if no such attachment could be found.
     */
    KCalendarCore::Attachment find(const QString &attachmentName, const KCalendarCore::Incidence::Ptr &incidence);

    /**
     * Finds the attachment in the user's calendar, by @p attachmentName and a scheduler message;
     * in other words, this function is intended to retrieve attachments from calendar invitations.
     *
     * @param attachmentName is the name of the attachment
     * @param message is a pointer to a valid ScheduleMessage object containing the attachment.
     * @return a pointer to the Attachment object located; 0 if no such attachment could be found.
     */
    KCalendarCore::Attachment find(const QString &attachmentName, const KCalendarCore::ScheduleMessage::Ptr &message);

    /**
     * Launches a viewer on the specified attachment.
     *
     * @param attachment is a pointer to a valid Attachment object.
     * @return true if the viewer program successfully launched; false otherwise.
     */
    bool view(const KCalendarCore::Attachment &attachment);

    /**
     * Launches a viewer on the specified attachment.
     *
     * @param attachmentName is the name of the attachment
     * @param incidence is a pointer to a valid Incidence object containing the attachment.
     * @return true if the attachment could be found and the viewer program successfully launched;
     * false otherwise.
     */
    bool view(const QString &attachmentName, const KCalendarCore::Incidence::Ptr &incidence);

    /**
      Launches a viewer on the specified attachment.

      @param attachmentName is the name of the attachment
      @param uid is a QString containing a UID of the incidence containing the attachment.

      This function is async and will return immediately. Listen to signal viewFinished()
      if you're interested on the success of this operation.

    */
    void view(const QString &attachmentName, const QString &uid);

    /**
      Launches a viewer on the specified attachment.

      @param attachmentName is the name of the attachment
      @param message is a pointer to a valid ScheduleMessage object containing the attachment.

      @return true if the attachment could be found and the viewer program successfully launched;
      false otherwise.
    */
    bool view(const QString &attachmentName, const KCalendarCore::ScheduleMessage::Ptr &message);

    /**
      Saves the specified attachment to a file of the user's choice.

      @param attachment is a pointer to a valid Attachment object.

      @return true if the save operation was successful; false otherwise.
    */
    bool saveAs(const KCalendarCore::Attachment &attachment);

    /**
      Saves the specified attachment to a file of the user's choice.

      @param attachmentName is the name of the attachment
      @param incidence is a pointer to a valid Incidence object containing the attachment.

      @return true if the attachment could be found and the save operation was successful;
      false otherwise.
    */
    bool saveAs(const QString &attachmentName, const KCalendarCore::Incidence::Ptr &incidence);

    /**
      Saves the specified attachment to a file of the user's choice.

      @param attachmentName is the name of the attachment
      @param uid is a QString containing a UID of the incidence containing the attachment.

      This function is async, it will return immediately. Listen to signal saveAsFinished()
      if you're interested on the success of this operation.
    */
    void saveAs(const QString &attachmentName, const QString &uid);

    /**
      Saves the specified attachment to a file of the user's choice.

      @param attachmentName is the name of the attachment
      @param message is a pointer to a valid ScheduleMessage object containing the attachment.

      @return true if the attachment could be found and the save operation was successful;
      false otherwise.
    */
    bool saveAs(const QString &attachmentName, const KCalendarCore::ScheduleMessage::Ptr &message);

Q_SIGNALS:
    void viewFinished(const QString &uid, const QString &attachmentName, bool success);
    void saveAsFinished(const QString &uid, const QString &attachmentName, bool success);

private:
    void slotFinishView(KJob *job);
    void slotFinishSaveAs(KJob *job);
    //@cond PRIVATE
    class Private;
    Private *const d;
    //@endcond
}; // class AttachmentHandler
} // namespace CalendarSupport

