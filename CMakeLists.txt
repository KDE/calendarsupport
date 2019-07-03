cmake_minimum_required(VERSION 3.5)
set(PIM_VERSION "5.11.40")

project(calendarsupport VERSION ${PIM_VERSION})

set(KF5_MIN_VERSION "5.59.0")

find_package(ECM ${KF5_MIN_VERSION} CONFIG REQUIRED)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
set(LIBRARY_NAMELINK)
include(GenerateExportHeader)
include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(ECMGeneratePriFile)

include(ECMSetupVersion)
include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
include(ECMQtDeclareLoggingCategory)
include(ECMAddTests)


set(CALENDARSUPPORT_LIB_VERSION ${PIM_VERSION})
set(AKONADI_MIMELIB_VERSION "5.11.40")
set(KDEPIM_LIB_VERSION "5.11.40")
set(QT_REQUIRED_VERSION "5.11.0")
set(KMIME_LIB_VERSION "5.11.40")
set(CALENDARUTILS_LIB_VERSION "5.11.40")
set(KCALENDARCORE_LIB_VERSION "5.11.45")
set(IDENTITYMANAGEMENT_LIB_VERSION "5.11.40")
set(AKONADICALENDAR_LIB_VERSION "5.11.40")
set(PIMCOMMON_LIB_VERSION "5.11.40")
set(AKONADI_VERSION "5.11.40")
find_package(KF5Akonadi ${AKONADI_VERSION} CONFIG REQUIRED)
find_package(Qt5 ${QT_REQUIRED_VERSION} CONFIG REQUIRED Widgets Test UiTools PrintSupport)
find_package(KF5I18n ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5IconThemes ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5GuiAddons ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5KIO ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5Mime ${KMIME_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5AkonadiMime ${AKONADI_MIMELIB_VERSION} CONFIG REQUIRED)
find_package(KF5Codecs ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5CalendarUtils ${CALENDARUTILS_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5CalendarCore ${KCALENDARCORE_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5IdentityManagement ${IDENTITYMANAGEMENT_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5Holidays ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5AkonadiCalendar ${AKONADICALENDAR_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5PimCommon ${PIMCOMMON_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5KdepimDBusInterfaces ${KDEPIM_LIB_VERSION} CONFIG REQUIRED)

ecm_setup_version(PROJECT VARIABLE_PREFIX CALENDARSUPPORT
                        VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/calendarsupport_version.h"
                        PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KF5CalendarSupportConfigVersion.cmake"
                        SOVERSION 5
)

########### Targets ###########


add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x060000)
########### CMake Config Files ###########
set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/KF5CalendarSupport")

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/KF5CalendarSupportConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/KF5CalendarSupportConfig.cmake"
  INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/KF5CalendarSupportConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/KF5CalendarSupportConfigVersion.cmake"
  DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
  COMPONENT Devel
)

install(EXPORT KF5CalendarSupportTargets DESTINATION "${CMAKECONFIG_INSTALL_DIR}" FILE KF5CalendarSupportTargets.cmake NAMESPACE KF5::)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/calendarsupport_version.h
  DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF5} COMPONENT Devel
)

add_definitions(-DQT_NO_FOREACH)


add_subdirectory(src)

install(FILES calendarsupport.categories calendarsupport.renamecategories DESTINATION  ${KDE_INSTALL_LOGGINGCATEGORIESDIR})


feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
