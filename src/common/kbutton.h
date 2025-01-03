/***
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
***/

// [FWGS, 01.01.24]
#ifndef KBUTTON_H
#define KBUTTON_H

// [FWGS, 01.01.24] a1ba: copied from WinQuake/client.h
//
// cl_input
//
typedef struct
	{
	int		down[2];		// key nums holding it down
	int		state;			// low bit is down state
	} kbutton_t;

// [FWGS, 01.12.24]
/*STATIC_ASSERT (sizeof (kbutton_t) == 12, "kbutton_t isn't 12 bytes!");*/
STATIC_CHECK_SIZEOF (kbutton_t, 12, 12);

#endif
