/***
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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
***/

// [FWGS, 01.01.24]
#ifndef NET_ADR_H
#define NET_ADR_H

#include "xash3d_types.h"
#include <stdint.h>			// ESHQ: ������ �����������

// net.h -- quake's interface to the networking layer
// a1ba: copied from Quake-2/qcommon/qcommon.h and modified to support IPv6

#define PORT_ANY -1

// [FWGS, 01.03.25]
/*typedef enum { NA_LOOPBACK = 1, NA_BROADCAST, NA_IP, NA_IPX, NA_BROADCAST_IPX, NA_IP6, NA_MULTICAST_IP6 } netadrtype_t;*/
typedef enum netadrtype_e
	{
	NA_UNDEFINED = 0,
	NA_LOOPBACK,
	NA_BROADCAST,
	NA_IP,
	NA_IPX,
	NA_BROADCAST_IPX,
	NA_IP6,
	NA_MULTICAST_IP6
	} netadrtype_t;

#pragma pack(push, 1)

// [FWGS, 01.03.25]
typedef struct netadr_s
	{
	// the reason we do this evil thing, is that when this struct contains IPv6
	// address the `type` is 2-byte wide, but when it doesn't `type` must 4-byte
	// wide _and_ ip6_0 must be zeroed, to keep it binary compatible.
#if XASH_LITTLE_ENDIAN
	uint16_t	type;
	uint8_t		ip6_0[2];
#elif XASH_BIG_ENDIAN
	uint8_t		ip6_0[2];
	uint16_t	type;
#else
	#error
#endif

	union
		{
		// IPv6 struct
		uint8_t	ip6_1[14];

		struct
			{
			/*uint16_t	type6;
			uint8_t		ip6[16];
			};

		struct
			{
			uint32_t	type; // must be netadrtype_t but will break with short enums*/
			union
				{
				/*uint8_t	ip[4];
				uint32_t	ip4; // for easier conversions*/
				uint8_t		ip[4];
				uint32_t	ip4;	// for easier conversions
				};
			uint8_t		ipx[10];
			};
		};
	/*uint16_t	port;*/
	uint16_t	port;
	} netadr_t;

#pragma pack(pop)

// [FWGS, 01.03.25]
static inline netadrtype_t NET_NetadrType (const netadr_t *a)
	{
	if ((a->type == NA_IP6) || (a->type == NA_MULTICAST_IP6))
		return (netadrtype_t)a->type;

	if (a->ip6_0[0] || a->ip6_0[1])
		return NA_UNDEFINED;

	return (netadrtype_t)a->type;
	}

// [FWGS, 01.03.25]
static inline void NET_NetadrSetType (netadr_t *a, netadrtype_t type)
	{
	if ((type == NA_IP6) || (type == NA_MULTICAST_IP6))
		{
		a->type = type;
		return;
		}

	a->ip6_0[0] = a->ip6_0[1] = 0;
	a->type = type;
	}

// [FWGS, 01.12.24]
STATIC_CHECK_SIZEOF (netadr_t, 20, 20);

#endif
