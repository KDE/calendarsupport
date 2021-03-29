/*
  SPDX-FileCopyrightText: 2003 Cornelius Schumacher <schumacher@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/
#pragma once

#include "calendarsupport_export.h"

class QString;

namespace CalendarSupport
{
/** URI handler. */
class CALENDARSUPPORT_EXPORT UriHandler
{
public:
    /**
      Process URI (e.g. open mailer, open browser, open incidence viewer etc.).
        @return true if handler handled the URI, otherwise false.
        @param uri The URI of the link that should be handled.
    */
    static bool process(const QString &uri);
};

} // namespace

