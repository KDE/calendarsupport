# SPDX-FileCopyrightText: Volker Krause <vkrause@kde.org>
# SPDX-License-Identifier: BSD-3-Clause

ecm_add_tests(testtodotooltip.cpp
    LINK_LIBRARIES KPim6CalendarSupport KF6::I18n Qt::Test
)
ecm_add_tests(testextensivedisplaystring.cpp
    LINK_LIBRARIES KPim6CalendarSupport KF6::CalendarCore Qt::Test
)
target_compile_definitions(testextensivedisplaystring PRIVATE TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")
