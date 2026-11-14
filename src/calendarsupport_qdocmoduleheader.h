/*
  SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

// Module header for qdoc PCH
#include "archivedialog.h"
#include "attachmenthandler.h"
#include "calendarsingleton.h"
#include "categoryhierarchyreader.h"
#include "cellitem.h"
#include "collectionselection.h"
#include "eventarchiver.h"
#include "freebusymodel/freebusycalendar.h"
#include "freebusymodel/freebusyitem.h"
#include "freebusymodel/freebusyitemmodel.h"
#include "freebusymodel/freeperiodmodel.h"
#include "identitymanager.h"
#include "incidenceattachmentmodel.h"
#include "incidenceformatter_p.h"
#include "incidenceviewer.h"
#include "incidenceviewer_p.h"
#include "kcalprefs.h"
#include "messagewidget.h"
#include "printing/calprintdefaultplugins.h"
#include "printing/calprinter.h"
#include "printing/calprintpluginbase.h"
#include "printing/journalprint.h"
#include "printing/printplugin.h"
#include "printing/yearprint.h"
#include "urihandler.h"
#include "utils.h"
