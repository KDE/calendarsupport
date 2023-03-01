# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause
macro(add_freebusymodel_unittest _name)
    ecm_add_test(${_name}.cpp ${_name}.h
        TEST_NAME ${_name}
        NAME_PREFIX "freebusymodel-"
        LINK_LIBRARIES Qt::Test KPim${KF_MAJOR_VERSION}::AkonadiCore KPim${KF_MAJOR_VERSION}::CalendarUtils KF${KF_MAJOR_VERSION}::CalendarCore KPim${KF_MAJOR_VERSION}::CalendarSupport
    )
endmacro()

add_freebusymodel_unittest(testfreeperiodmodel)
add_freebusymodel_unittest(testfreebusyitemmodel)
