/***
build.c - returns a engine build number
Copyright (C) 2010 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details
***/

// [FWGS, 01.03.26]
#include <stdio.h>
#include "crtlib.h"

// [FWGS, 01.09.25]
static const char mond[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

// [FWGS, 01.09.25] removed Q_buildnum_date

// [FWGS, 01.07.26]
int Q_buildnum_iso (const char *date)
	{
	/*int y, m, d, b, i;*/
	int y, m, d;

	if (sscanf (date, "%d-%d-%d", &y, &m, &d) != 3 || (y <= 1900) || (m <= 0) || (d <= 0))
		return -1;

	// fixup day and month
	m--;
	d--;

	/*for (i = 0; i < m; i++)*/
	for (int i = 0; i < m; i++)
		d += mond[i];

	y -= 1900;
	/*b = d + (int)((y - 1) * 365.25f);*/
	int b = d + (int)((y - 1) * 365.25f);

	if (((y % 4) == 0) && m > 1)
		b += 1;
	b -= 41728;	// Apr 1 2015

	return b;
	}

/***
===============
Q_buildnum [FWGS, 01.09.25]

returns days since Apr 1 2015
===============
***/
int Q_buildnum (void)
	{
	static int b = 0;

	if (b)
		return b;

	b = Q_buildnum_iso (g_buildcommit_date);
	return b;
	}

// [FWGS, 01.03.26] removed Q_buildnum_compat, Q_PlatformStringByID, Q_buildos,
// Q_ArchitectureStringByID, Q_buildarch
// [FWGS, 01.02.25] removed Q_buildcommit, Q_buildbranch
