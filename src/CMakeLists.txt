# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause
add_definitions(-DTRANSLATION_DOMAIN=\"calendarsupport6\")

########### next target ###############
add_library(KPim6CalendarSupport)
add_library(KPim6::CalendarSupport ALIAS KPim6CalendarSupport)
target_sources(
    KPim6CalendarSupport
    PRIVATE
        archivedialog.cpp
        attachmenthandler.cpp
        calendarsingleton.cpp
        categoryhierarchyreader.cpp
        cellitem.cpp
        collectionselection.cpp
        eventarchiver.cpp
        identitymanager.cpp
        incidenceattachmentmodel.cpp
        kcalprefs.cpp
        messagewidget.cpp
        utils.cpp
        urihandler.cpp
        printing/calprintpluginbase.cpp
        printing/calprintdefaultplugins.cpp
        printing/calprinter.cpp
        printing/journalprint.cpp
        printing/yearprint.cpp
        next/incidenceviewer.cpp
        freebusymodel/freeperiodmodel.cpp
        freebusymodel/freebusyitem.cpp
        freebusymodel/freebusyitemmodel.cpp
        freebusymodel/freebusycalendar.cpp
        next/incidenceviewer.h
        next/incidenceviewer_p.h
        categoryhierarchyreader.h
        calendarsingleton.h
        utils.h
        archivedialog.h
        cellitem.h
        identitymanager.h
        attachmenthandler.h
        eventarchiver.h
        printing/printplugin.h
        printing/calprintpluginbase.h
        printing/journalprint.h
        printing/calprintdefaultplugins.h
        printing/yearprint.h
        printing/calprinter.h
        kcalprefs.h
        urihandler.h
        incidenceattachmentmodel.h
        freebusymodel/freeperiodmodel.h
        freebusymodel/freebusyitemmodel.h
        freebusymodel/freebusycalendar.h
        freebusymodel/freebusyitem.h
        collectionselection.h
        messagewidget.h
)

ki18n_wrap_ui(KPim6CalendarSupport
  printing/calprintdayconfig_base.ui
  printing/calprintincidenceconfig_base.ui
  printing/calprintjournalconfig_base.ui
  printing/calprintmonthconfig_base.ui
  printing/calprinttodoconfig_base.ui
  printing/calprintweekconfig_base.ui
  printing/calprintyearconfig_base.ui
)
ecm_qt_declare_logging_category(KPim6CalendarSupport HEADER calendarsupport_debug.h IDENTIFIER CALENDARSUPPORT_LOG CATEGORY_NAME org.kde.pim.calendarsupport
        OLD_CATEGORY_NAMES log_calendarsupport
        DESCRIPTION "calendarsupport (calendarsupport)" EXPORT CALENDARSUPPORT
)

kconfig_add_kcfg_files(KPim6CalendarSupport kcalprefs_base.kcfgc)

if(COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(
        KPim6CalendarSupport
        PROPERTIES
            UNITY_BUILD
                ON
    )
endif()
ecm_generate_export_header(
    KPim6CalendarSupport
    BASE_NAME calendarsupport
    VERSION ${PIM_VERSION}
    DEPRECATION_VERSIONS "5.24"
)

target_include_directories(
    KPim6CalendarSupport
    INTERFACE
        "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}/KPim6/CalendarSupport/>"
)
target_include_directories(
    KPim6CalendarSupport
    PUBLIC
        "$<BUILD_INTERFACE:${calendarsupport_SOURCE_DIR}/src;${calendarsupport_BINARY_DIR}/src>"
)

target_link_libraries(
    KPim6CalendarSupport
    PUBLIC
        KPim6::Mime
        KPim6::IdentityManagementCore
        KPim6::AkonadiCalendar
        KF6::ConfigGui
        Qt::PrintSupport
    PRIVATE
        KF6::TextCustomEditor
        KF6::I18n
        KF6::Completion
        KF6::KIOWidgets
        KF6::GuiAddons
        KPim6::AkonadiWidgets
        KF6::Holidays
        KF6::Codecs
        KPim6::AkonadiCore
        KF6::CalendarCore
        KPim6::CalendarUtils
)

set_target_properties(
    KPim6CalendarSupport
    PROPERTIES
        VERSION
            ${CALENDARSUPPORT_VERSION}
        SOVERSION
            ${CALENDARSUPPORT_SOVERSION}
        EXPORT_NAME
            CalendarSupport
)

install(TARGETS KPim6CalendarSupport EXPORT KPim6CalendarSupportTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

ecm_generate_headers(CalendarSupport_CamelCase_HEADERS
  HEADER_NAMES
  Utils
  CellItem
  CollectionSelection
  KCalPrefs
  IdentityManager
  EventArchiver
  CategoryHierarchyReader
  CalendarSingleton
  MessageWidget
  ArchiveDialog
  UriHandler
  REQUIRED_HEADERS CalendarSupport_HEADERS
  PREFIX CalendarSupport
)

ecm_generate_headers(CalendarSupport_CamelCaseprinter_HEADERS
  HEADER_NAMES
  PrintPlugin
  CalPrinter
  REQUIRED_HEADERS CalendarSupport_printer_HEADERS
  PREFIX CalendarSupport
  RELATIVE printing
)

ecm_generate_headers(CalendarSupport_CamelCasenext_HEADERS
  HEADER_NAMES
  IncidenceViewer
  REQUIRED_HEADERS CalendarSupport_next_HEADERS
  PREFIX CalendarSupport
  RELATIVE next
)

ecm_generate_headers(CalendarSupport_CamelCasefreebusy_HEADERS
  HEADER_NAMES
  FreeBusyItem
  FreeBusyItemModel
  FreeBusyCalendar
  FreePeriodModel
  REQUIRED_HEADERS CalendarSupport_freebusy_HEADERS
  PREFIX CalendarSupport
  RELATIVE freebusymodel
)

install(
    FILES
        ${CalendarSupport_CamelCase_HEADERS}
        ${CalendarSupport_CamelCaseprinter_HEADERS}
        ${CalendarSupport_CamelCasenext_HEADERS}
        ${CalendarSupport_CamelCasefreebusy_HEADERS}
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KPim6/CalendarSupport/CalendarSupport
    COMPONENT Devel
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/calendarsupport_export.h
        ${CMAKE_CURRENT_BINARY_DIR}/ui_calprintincidenceconfig_base.h
        ${CMAKE_CURRENT_BINARY_DIR}/ui_calprintdayconfig_base.h
        ${CMAKE_CURRENT_BINARY_DIR}/ui_calprintweekconfig_base.h
        ${CMAKE_CURRENT_BINARY_DIR}/ui_calprintmonthconfig_base.h
        ${CMAKE_CURRENT_BINARY_DIR}/ui_calprinttodoconfig_base.h
        ${CMAKE_CURRENT_BINARY_DIR}/kcalprefs_base.h
        ${CalendarSupport_HEADERS}
        ${CalendarSupport_printer_HEADERS}
        ${CalendarSupport_next_HEADERS}
        ${CalendarSupport_freebusy_HEADERS}
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KPim6/CalendarSupport/calendarsupport
    COMPONENT Devel
)
if(BUILD_TESTING)
    add_subdirectory(freebusymodel/autotests)
endif()

ecm_qt_install_logging_categories(EXPORT CALENDARSUPPORT FILE calendarsupport.categories DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})
ecm_generate_qdoc(KPim6CalendarSupport calendarsupport.qdocconf)
