# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

########### next target ###############

add_executable(noteeditdialogtest noteeditdialogtest.cpp noteeditdialogtest.h)
if(COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(noteeditdialogtest PROPERTIES UNITY_BUILD ON)
endif()
add_test(NAME noteeditdialogtest COMMAND noteeditdialogtest)
ecm_mark_as_test(noteeditdialogtest)
target_link_libraries(noteeditdialogtest
  Qt::Test
  KPim6::AkonadiWidgets
  KPim6::AkonadiNotes
  KPim6::CalendarSupport
  KF6::TextCustomEditor
)

ecm_add_test(placeitemtest.cpp LINK_LIBRARIES Qt::Test KPim6::CalendarSupport)
