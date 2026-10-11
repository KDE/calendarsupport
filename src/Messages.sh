#!/bin/sh
# SPDX-License-Identifier: CC0-1.0
# SPDX-FileCopyrightText: none
$EXTRACT_GRANTLEE_TEMPLATE_STRINGS `find templates -name \*.html` >> rc.cpp
$EXTRACTRC `find . -name "*.ui" -o -name "*.kcfg"` >> rc.cpp || exit 11
$XGETTEXT `find . -name "*.cpp" -o -name "*.h" | grep -v '/tests/'` -o $podir/calendarsupport6.pot
rm -f rc.cpp
