/***
build_vcs.c - info from VCS
Copyright (C) 2025 Alibek Omarov

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details
***/

#include "buildenums.h"	// ESHQ: const fix

const char *g_buildcommit = XASH_BUILD_COMMIT;
const char *g_buildbranch = XASH_BUILD_BRANCH;

// [FWGS, 01.03.25]
const char *g_buildcommit_date = "01.03.2025";
const char *g_build_date = __DATE__;
