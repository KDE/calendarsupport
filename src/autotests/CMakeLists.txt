# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

########### next target ###############

add_executable(noteeditdialogtest noteeditdialogtest.cpp)
if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(noteeditdialogtest PROPERTIES UNITY_BUILD ON)
endif()
add_test(NAME noteeditdialogtest COMMAND noteeditdialogtest)
ecm_mark_as_test(noteeditdialogtest)
target_link_libraries(noteeditdialogtest
  Qt${QT_MAJOR_VERSION}::Test
  KF5::AkonadiWidgets
  KF5::AkonadiNotes
  KF5::CalendarSupport
  KF5::PimTextEdit
)

ecm_add_test(placeitemtest.cpp LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test KF5::CalendarSupport)
