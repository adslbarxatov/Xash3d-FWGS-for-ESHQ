/*
Copyright (C) 1997-2001 Id Software, Inc.

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

// [FWGS, 01.01.24]
#ifndef NET_ADR_H
#define NET_ADR_H

/*
Copyright (c) 1996-2002, Valve LLC. All rights reserved.
	This product contains software technology licensed from Id
	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
	All Rights Reserved.
	Use, distribution, and modification of this source code and/or resulting
	object code is restricted to non-commercial enhancements to products from
	Valve LLC.  All other use, distribution, or modification is prohibited
	without written permission from Valve LLC.

#ifndef NETADR_H
#define NETADR_H

#include "build.h"
*/

#include "xash3d_types.h"		// [FWGS, 01.04.23]
#define STDINT_H <stdint.h>		// [FWGS, 01.04.23]
#include STDINT_H

// [FWGS, 01.01.24]
/*typedef enum*/

// net.h -- quake's interface to the networking layer
// a1ba: copied from Quake-2/qcommon/qcommon.h and modified to support IPv6

#define PORT_ANY -1

typedef enum { NA_LOOPBACK = 1, NA_BROADCAST, NA_IP, NA_IPX, NA_BROADCAST_IPX, NA_IP6, NA_MULTICAST_IP6 } netadrtype_t;

/*
Original Quake-2 structure:
typedef struct
{
	NA_UNUSED = 0,
	NA_LOOPBACK,
	NA_BROADCAST,
	NA_IP,
	NA_IPX,
	NA_BROADCAST_IPX,
	NA_IP6,
	NA_MULTICAST_IP6,	// [FWGS, 01.04.23] all nodes multicast
	} netadrtype_t;

netadrtype_t type;

byte ip[4];
byte ipx[10];

unsigned short port;
	} netadr_t;
*/

#pragma pack(push, 1)	// [FWGS, 01.04.23]

// [FWGS, 01.01.24]
typedef struct netadr_s
	{
	union
		{
		// IPv6 struct
		struct
			{
			/*uint32_t		type;
			union
				{
				uint8_t		ip[4];
				uint32_t	ip4;
				};
			uint8_t			ipx[10];*/
			uint16_t	type6;
			uint8_t		ip6[16];
			};

		struct
			{
			/*#if XASH_LITTLE_ENDIAN
						uint16_t		type6;
						uint8_t			ip6[16];
			#elif XASH_BIG_ENDIAN
						uint8_t			ip6_0[2];
						uint16_t		type6;
						uint8_t			ip6_2[14];
			#endif*/
			uint32_t	type; // must be netadrtype_t but will break with short enums
			union
				{
				uint8_t	ip[4];
				uint32_t	ip4; // for easier conversions
				};
			uint8_t		ipx[10];
			};
		};
	uint16_t	port;
	} netadr_t;

#pragma pack(pop)

/*STATIC_ASSERT (sizeof (netadr_t) == 20, "invalid netadr_t size");*/
STATIC_ASSERT (sizeof (netadr_t) == 20, "netadr_t isn't 20 bytes!");

#endif
