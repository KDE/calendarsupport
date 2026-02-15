/*
  SPDX-FileCopyrightText: 2003 Cornelius Schumacher <schumacher@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/
#pragma once

#include "calendarsupport_export.h"

class QString;

namespace CalendarSupport
{
/*!
 * \class CalendarSupport::UriHandler
 * \inmodule CalendarSupport
 * \inheaderfile CalendarSupport/UriHandler
 *
 *  URI handler.
 */
class CALENDARSUPPORT_EXPORT UriHandler
{
public:
    /*!
     * Processes a URI (e.g., opens mailer, browser, incidence viewer, etc.).
     * \param uri The URI of the link that should be handled.
     * \return true if the handler handled the URI, false otherwise.
     */
    static bool process(const QString &uri);
};

} // namespace
