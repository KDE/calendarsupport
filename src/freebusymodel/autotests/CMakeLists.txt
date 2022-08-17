# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause
macro(add_freebusymodel_unittest _name)
    ecm_add_test(${_name}.cpp ${_name}.h
        TEST_NAME ${_name}
        NAME_PREFIX "freebusymodel-"
        LINK_LIBRARIES Qt::Test KF5::AkonadiCore KF5::CalendarUtils KF5::CalendarCore KF5::CalendarSupport
    )
endmacro()

add_freebusymodel_unittest(testfreeperiodmodel)
add_freebusymodel_unittest(testfreebusyitemmodel)
