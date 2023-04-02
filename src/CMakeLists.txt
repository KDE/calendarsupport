# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause
add_definitions(-DTRANSLATION_DOMAIN=\"calendarsupport\")

########### next target ###############
add_library(KPim${KF_MAJOR_VERSION}CalendarSupport)
add_library(KPim${KF_MAJOR_VERSION}::CalendarSupport ALIAS KPim${KF_MAJOR_VERSION}CalendarSupport)
target_sources(KPim${KF_MAJOR_VERSION}CalendarSupport PRIVATE
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
  noteeditdialog.cpp
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
  noteeditdialog.h
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

ki18n_wrap_ui(KPim${KF_MAJOR_VERSION}CalendarSupport
  printing/calprintdayconfig_base.ui
  printing/calprintincidenceconfig_base.ui
  printing/calprintjournalconfig_base.ui
  printing/calprintmonthconfig_base.ui
  printing/calprinttodoconfig_base.ui
  printing/calprintweekconfig_base.ui
  printing/calprintyearconfig_base.ui
)
ecm_qt_declare_logging_category(KPim${KF_MAJOR_VERSION}CalendarSupport HEADER calendarsupport_debug.h IDENTIFIER CALENDARSUPPORT_LOG CATEGORY_NAME org.kde.pim.calendarsupport
        OLD_CATEGORY_NAMES log_calendarsupport
        DESCRIPTION "calendarsupport (calendarsupport)" EXPORT CALENDARSUPPORT)

kconfig_add_kcfg_files(KPim${KF_MAJOR_VERSION}CalendarSupport kcalprefs_base.kcfgc)

if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(KPim${KF_MAJOR_VERSION}CalendarSupport PROPERTIES UNITY_BUILD ON)
endif()
generate_export_header(KPim${KF_MAJOR_VERSION}CalendarSupport BASE_NAME calendarsupport)


target_include_directories(KPim${KF_MAJOR_VERSION}CalendarSupport INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}/KPim${KF_MAJOR_VERSION}/CalendarSupport/>")
target_include_directories(KPim${KF_MAJOR_VERSION}CalendarSupport PUBLIC "$<BUILD_INTERFACE:${calendarsupport_SOURCE_DIR}/src;${calendarsupport_BINARY_DIR}/src>")

target_link_libraries(
  KPim${KF_MAJOR_VERSION}CalendarSupport
PUBLIC
  KPim${KF_MAJOR_VERSION}::Mime
  KPim${KF_MAJOR_VERSION}::IdentityManagement
  KPim${KF_MAJOR_VERSION}::AkonadiCalendar
  KF${KF_MAJOR_VERSION}::ConfigGui
  Qt::PrintSupport
PRIVATE
  KPim${KF_MAJOR_VERSION}::PimTextEdit
  KF${KF_MAJOR_VERSION}::I18n
  KF${KF_MAJOR_VERSION}::Completion
  KF${KF_MAJOR_VERSION}::KIOWidgets
  KF${KF_MAJOR_VERSION}::GuiAddons
  KPim${KF_MAJOR_VERSION}::AkonadiWidgets
  KPim${KF_MAJOR_VERSION}::AkonadiNotes
  KF${KF_MAJOR_VERSION}::Holidays
  KF${KF_MAJOR_VERSION}::Codecs
  KPim${KF_MAJOR_VERSION}::AkonadiCore
  KF${KF_MAJOR_VERSION}::CalendarCore
  KPim${KF_MAJOR_VERSION}::CalendarUtils
)

set_target_properties(KPim${KF_MAJOR_VERSION}CalendarSupport PROPERTIES
    VERSION ${CALENDARSUPPORT_VERSION}
    SOVERSION ${CALENDARSUPPORT_SOVERSION}
    EXPORT_NAME CalendarSupport
)

install(TARGETS
    KPim${KF_MAJOR_VERSION}CalendarSupport
    EXPORT KPim${KF_MAJOR_VERSION}CalendarSupportTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
)

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
  NoteEditDialog
  UriHandler
  REQUIRED_HEADERS CalendarSupport_HEADERS
  PREFIX CalendarSupport
)

ecm_generate_headers(CalendarSupport_CamelCaseprinter_HEADERS
  HEADER_NAMES
  PrintPlugin
  CalPrinter
  CalPrintDefaultPlugins
  CalPrintPluginBase
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

ecm_generate_pri_file(BASE_NAME CalendarSupport
    LIB_NAME KPim${KF_MAJOR_VERSION}CalendarSupport
    DEPS "KMime KIdentityManagement AkonadiCalendar" FILENAME_VAR PRI_FILENAME INCLUDE_INSTALL_DIR ${KDE_INSTALL_INCLUDEDIR}/KPim${KF_MAJOR_VERSION}/CalendarSupport
)

install(FILES
    ${CalendarSupport_CamelCase_HEADERS}
    ${CalendarSupport_CamelCaseprinter_HEADERS}
    ${CalendarSupport_CamelCasenext_HEADERS}
    ${CalendarSupport_CamelCasefreebusy_HEADERS}
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KPim${KF_MAJOR_VERSION}/CalendarSupport/CalendarSupport
    COMPONENT Devel
)

install(FILES
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
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KPim${KF_MAJOR_VERSION}/CalendarSupport/calendarsupport
    COMPONENT Devel
)
install(FILES
    ${PRI_FILENAME}
    DESTINATION ${ECM_MKSPECS_INSTALL_DIR})

if(BUILD_TESTING)
    add_subdirectory(autotests)
    add_subdirectory(freebusymodel/autotests)
endif()

ecm_qt_install_logging_categories(EXPORT CALENDARSUPPORT FILE calendarsupport.categories DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})

if (BUILD_QCH)
    ecm_add_qch(
        KPim${KF_MAJOR_VERSION}CalendarSupport_QCH
        NAME KPim${KF_MAJOR_VERSION}CalendarSupport
        BASE_NAME KPim${KF_MAJOR_VERSION}CalendarSupport
        VERSION ${PIM_VERSION}
        ORG_DOMAIN org.kde
        SOURCES # using only public headers, to cover only public API
        ${CalendarSupport_HEADERS}
        ${CalendarSupport_printer_HEADERS}
        ${CalendarSupport_next_HEADERS}
        ${CalendarSupport_freebusy_HEADERS}
        MD_MAINPAGE "${CMAKE_SOURCE_DIR}/README.md"
        #IMAGE_DIRS "${CMAKE_SOURCE_DIR}/docs/pics"
        LINK_QCHS
            Qt${QT_MAJOR_VERSION}Core_QCH
            Qt${QT_MAJOR_VERSION}Gui_QCH
            Qt${QT_MAJOR_VERSION}Widgets_QCH
        INCLUDE_DIRS
            ${CMAKE_CURRENT_BINARY_DIR}
        BLANK_MACROS
            KSIEVEUI_EXPORT
        TAGFILE_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        QCH_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        COMPONENT Devel
    )
endif()
