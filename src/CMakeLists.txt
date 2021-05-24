
add_definitions(-DTRANSLATION_DOMAIN=\"calendarsupport\")

########### next target ###############
add_library(KF5CalendarSupport)
add_library(KF5::CalendarSupport ALIAS KF5CalendarSupport)
target_sources(KF5CalendarSupport PRIVATE
  archivedialog.cpp
  attachmenthandler.cpp
  calendarsingleton.cpp
  calendarutils.cpp
  categoryconfig.cpp
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
  tagcache.cpp
  plugin.cpp
  urihandler.cpp
  kdatepickerpopup.cpp

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
)

ki18n_wrap_ui(KF5CalendarSupport
  printing/calprintdayconfig_base.ui
  printing/calprintincidenceconfig_base.ui
  printing/calprintjournalconfig_base.ui
  printing/calprintmonthconfig_base.ui
  printing/calprinttodoconfig_base.ui
  printing/calprintweekconfig_base.ui
  printing/calprintyearconfig_base.ui
)
ecm_qt_declare_logging_category(KF5CalendarSupport HEADER calendarsupport_debug.h IDENTIFIER CALENDARSUPPORT_LOG CATEGORY_NAME org.kde.pim.calendarsupport
        OLD_CATEGORY_NAMES log_calendarsupport
        DESCRIPTION "calendarsupport (calendarsupport)" EXPORT CALENDARSUPPORT)

kconfig_add_kcfg_files(KF5CalendarSupport kcalprefs_base.kcfgc)

if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(KF5CalendarSupport PROPERTIES UNITY_BUILD ON)
endif()
generate_export_header(KF5CalendarSupport BASE_NAME calendarsupport)


target_include_directories(KF5CalendarSupport INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR_KF5}/CalendarSupport/;${KDE_INSTALL_INCLUDEDIR_KF5}/calendarsupport>")
target_include_directories(KF5CalendarSupport PUBLIC "$<BUILD_INTERFACE:${calendarsupport_SOURCE_DIR}/src;${calendarsupport_BINARY_DIR}/src>")

target_link_libraries(
  KF5CalendarSupport
PUBLIC
  KF5::Mime
  KF5::IdentityManagement
  KF5::AkonadiCalendar
  Qt::PrintSupport
PRIVATE
  KF5::I18n
  KF5::Completion
  KF5::KIOWidgets
  KF5::GuiAddons
  KF5::AkonadiWidgets
  KF5::AkonadiNotes
  KF5::Holidays
  KF5::Codecs
  KF5::AkonadiCore
  KF5::CalendarCore
  KF5::CalendarUtils
  KF5::PimCommon
)

set_target_properties(KF5CalendarSupport PROPERTIES
    VERSION ${CALENDARSUPPORT_VERSION}
    SOVERSION ${CALENDARSUPPORT_SOVERSION}
    EXPORT_NAME CalendarSupport
)

install(TARGETS
    KF5CalendarSupport
    EXPORT KF5CalendarSupportTargets ${KF5_INSTALL_TARGETS_DEFAULT_ARGS}
)

ecm_generate_headers(CalendarSupport_CamelCase_HEADERS
  HEADER_NAMES
  Utils
  CategoryConfig
  CellItem
  CollectionSelection
  KCalPrefs
  IdentityManager
  EventArchiver
  Plugin
  CategoryHierarchyReader
  CalendarSingleton
  MessageWidget
  ArchiveDialog
  NoteEditDialog
  KDatePickerPopup
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
    LIB_NAME KF5CalendarSupport
    DEPS "KMime KIdentityManagement AkonadiCalendar" FILENAME_VAR PRI_FILENAME INCLUDE_INSTALL_DIR ${KDE_INSTALL_INCLUDEDIR_KF5}/CalendarSupport
)

install(FILES
    ${CalendarSupport_CamelCase_HEADERS}
    ${CalendarSupport_CamelCaseprinter_HEADERS}
    ${CalendarSupport_CamelCasenext_HEADERS}
    ${CalendarSupport_CamelCasefreebusy_HEADERS}
    DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF5}/CalendarSupport
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
    DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF5}/calendarsupport
    COMPONENT Devel
)
install(FILES
    ${PRI_FILENAME}
    DESTINATION ${ECM_MKSPECS_INSTALL_DIR})

install(FILES calendarplugin.desktop DESTINATION ${KDE_INSTALL_KSERVICETYPES5DIR})

if(BUILD_TESTING)
    add_subdirectory(autotests)
    add_subdirectory(freebusymodel/autotests)
endif()

ecm_qt_install_logging_categories(EXPORT CALENDARSUPPORT FILE calendarsupport.categories DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})
