cmake_minimum_required(VERSION 2.8.12)

project(calendarsupport)

set(KF5_VERSION "5.28.0")
find_package(ECM ${KF5_VERSION} CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
set(LIBRARY_NAMELINK)
include(GenerateExportHeader)
include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(ECMGeneratePriFile)
include(ECMPackageConfigHelpers)
include(ECMSetupVersion)
include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
include(ECMQtDeclareLoggingCategory)
include(ECMAddTests)

set(PIM_VERSION "5.4.0")

set(CALENDARSUPPORT_LIB_VERSION ${PIM_VERSION})
set(AKONADI_MIMELIB_VERSION "5.3.90")
set(KDEPIM_LIB_VERSION "5.3.90")
set(QT_REQUIRED_VERSION "5.6.0")
set(KMIME_LIB_VERSION "5.3.90")
set(CALENDARUTILS_LIB_VERSION "5.3.90")
set(KCALENDARCORE_LIB_VERSION "5.3.90")
set(IDENTITYMANAGEMENT_LIB_VERSION "5.3.90")
set(KHOLIDAYS_LIB_VERSION "5.3.90")
set(AKONADICALENDAR_LIB_VERSION "5.3.90")
set(PIMCOMMON_LIB_VERSION "5.3.90")
set(AKONADI_VERSION "5.3.90")

find_package(KF5Akonadi ${AKONADI_VERSION} CONFIG REQUIRED)
find_package(Qt5 ${QT_REQUIRED_VERSION} CONFIG REQUIRED Widgets Test UiTools)
find_package(KF5I18n ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5Mime ${KMIME_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5AkonadiMime ${AKONADI_MIMELIB_VERSION} CONFIG REQUIRED)
find_package(KF5Codecs ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5CalendarUtils ${CALENDARUTILS_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5CalendarCore ${KCALENDARCORE_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5IdentityManagement ${IDENTITYMANAGEMENT_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5Holidays ${KHOLIDAYS_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5AkonadiCalendar ${AKONADICALENDAR_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5PimCommon ${PIMCOMMON_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5KdepimDBusInterfaces ${KDEPIM_LIB_VERSION} CONFIG REQUIRED)

ecm_setup_version(${CALENDARSUPPORT_LIB_VERSION} VARIABLE_PREFIX CALENDARSUPPORT
                        VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/calendarsupport_version.h"
                        PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KF5CalendarSupportConfigVersion.cmake"
                        SOVERSION 5
)


########### Targets ###########

add_definitions("-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII")
########### CMake Config Files ###########
set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/KF5CalendarSupport")

ecm_configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/KF5CalendarSupportConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/KF5CalendarSupportConfig.cmake"
  INSTALL_DESTINATION  ${CMAKECONFIG_INSTALL_DIR}
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/KF5CalendarSupportConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/KF5CalendarSupportConfigVersion.cmake"
  DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
  COMPONENT Devel
)

install(EXPORT KF5CalendarSupportTargets DESTINATION "${CMAKECONFIG_INSTALL_DIR}" FILE KF5CalendarSupportTargets.cmake NAMESPACE KF5::)

install(FILES
   ${CMAKE_CURRENT_BINARY_DIR}/calendarsupport_version.h
  DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF5} COMPONENT Devel
)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

add_subdirectory(src)

install( FILES calendarsupport.categories calendarsupport.renamecategories DESTINATION ${KDE_INSTALL_CONFDIR} )
