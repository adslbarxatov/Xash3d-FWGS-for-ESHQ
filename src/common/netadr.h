/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#ifndef NETADR_H
#define NETADR_H

// [Xash3D, 20.03.23]
#include "build.h"
#include STDINT_H

typedef enum
	{
	NA_UNUSED = 0,
	NA_LOOPBACK,
	NA_BROADCAST,
	NA_IP,
	NA_IPX,
	NA_BROADCAST_IPX,
	NA_IP6,
	NA_MULTICAST_IP6,	// [Xash3D, 20.03.23] all nodes multicast
	} netadrtype_t;

#pragma pack(push, 1)	// [Xash3D, 20.03.23]

// [Xash3D, 20.03.23]
typedef struct netadr_s
	{
	/*netadrtype_t	type;
	unsigned char	ip[4];
	unsigned char	ipx[10];
	unsigned short	port;*/

	union
		{
		struct
			{
			uint32_t		type;
			union
				{
				uint8_t		ip[4];
				uint32_t	ip4;
				};
			uint8_t			ipx[10];
			};
		struct
			{
#if XASH_LITTLE_ENDIAN
			uint16_t		type6;
			uint8_t			ip6[16];
#elif XASH_BIG_ENDIAN
			uint8_t			ip6_0[2];
			uint16_t		type6;
			uint8_t			ip6_2[14];
#endif
			};
		};
	uint16_t	port;
	} netadr_t;

#pragma pack(pop)

STATIC_ASSERT (sizeof (netadr_t) == 20, "invalid netadr_t size");

#endif