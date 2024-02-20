/*
gamma.c - gamma routines
Copyright (C) 2011 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "common.h"
#include "client.h"
#include "xash3d_mathlib.h"
#include "enginefeatures.h"

//-----------------------------------------------------------------------------
// Gamma conversion support
//-----------------------------------------------------------------------------
static byte	texgammatable[256];
static uint	lightgammatable[1024];
static uint	lineargammatable[1024];
static uint	screengammatable[1024];

static CVAR_DEFINE (v_direct, "direct", "0.9", 0,
	"direct studio lighting");
static CVAR_DEFINE (v_texgamma, "texgamma", "2.0", 0,
	"texgamma amount");
static CVAR_DEFINE (v_lightgamma, "lightgamma", "1.1", 0,
	"lightgamma amount");	// ESHQ
static CVAR_DEFINE (v_brightness, "brightness", "1.0", FCVAR_ARCHIVE,
	"brightness factor");	// ESHQ
static CVAR_DEFINE (v_gamma, "gamma", "1.5", FCVAR_ARCHIVE,
	"gamma amount");		// ESHQ

static void BuildGammaTable (const float gamma, const float brightness, const float texgamma, const float lightgamma)
	{
	float g1, g2, g3;
	int i;

	if (gamma != 0.0)
		g1 = 1.0 / gamma;
	else
		g1 = 0.4;
	g2 = g1 * texgamma;

	if (brightness <= 0.0)
		g3 = 0.125;
	else if (brightness <= 1.0)
		g3 = 0.125 - brightness * brightness * 0.075;
	else
		g3 = 0.05;

	for (i = 0; i < 256; i++)
		{
		double d = pow (i / 255.0, (double)g2);
		int inf = d * 255.0;
		texgammatable[i] = bound (0, inf, 255);
		}

	for (i = 0; i < 1024; i++)
		{
		double d;
		float f = pow (i / 1023.0, (double)lightgamma);
		int inf;

		if (brightness > 1.0)
			f *= brightness;

		if (f <= g3)
			f = (f / g3) * 0.125;
		else
			f = ((f - g3) / (1.0 - g3)) * 0.875 + 0.125;

		d = pow ((double)f, (double)g1); // do not remove the cast, or tests fail
		inf = d * 1023.0;
		lightgammatable[i] = bound (0, inf, 1023);

		// do these calculations in the same loop...
		lineargammatable[i] = pow (i / 1023.0, (double)gamma) * 1023.0;
		screengammatable[i] = pow (i / 1023.0, 1.0 / gamma) * 1023.0;
		}
	}

// [FWGS, 01.02.24]
static void V_ValidateGammaCvars (void)
	{
	/*if (Host_IsLocalGame ())
		return;*/

	// ESHQ: �������� �������� � ������������ � ����� �� ���������
	if (v_gamma.value < 1.5f)
		Cvar_DirectSet (&v_gamma, "1.5");
	else if (v_gamma.value > 3.0f)
		Cvar_DirectSet (&v_gamma, "3");

	if (v_texgamma.value < 1.8f)
		Cvar_DirectSet (&v_texgamma, "1.8");
	else if (v_texgamma.value > 3.0f)
		Cvar_DirectSet (&v_texgamma, "3");

	if (v_lightgamma.value < 1.1f)
		Cvar_DirectSet (&v_lightgamma, "1.1");
	else if (v_lightgamma.value > 3.0f)
		Cvar_DirectSet (&v_lightgamma, "3");

	if (v_brightness.value < 0.5f)
		Cvar_DirectSet (&v_brightness, "0.5");
	else if (v_brightness.value > 2.0f)
		Cvar_DirectSet (&v_brightness, "2");
	}

void V_CheckGamma (void)
	{
	static qboolean dirty = false;
	qboolean notify_refdll = false;

	// because these cvars were defined as archive
	// but wasn't doing anything useful
	// reset them into default values
	// this might be removed after a while
	if ((v_direct.value == 1.0f) || (v_lightgamma.value == 1.0f))
		{
		Cvar_DirectSet (&v_direct, "0.9");
		Cvar_DirectSet (&v_lightgamma, "2.5");
		}

	if ((cls.scrshot_action == scrshot_envshot) || (cls.scrshot_action == scrshot_skyshot))
		{
		dirty = true; // force recalculate next normal frame
		BuildGammaTable (1.8f, 0.0f, 2.0f, 2.5f);
		if (ref.initialized)
			ref.dllFuncs.R_GammaChanged (true);
		return;
		}

	if (dirty || FBitSet (v_texgamma.flags | v_lightgamma.flags | v_brightness.flags | v_gamma.flags, FCVAR_CHANGED))
		{
		V_ValidateGammaCvars ();

		dirty = false;
		BuildGammaTable (v_gamma.value, v_brightness.value, v_texgamma.value, v_lightgamma.value);

		// force refdll to recalculate lightmaps
		notify_refdll = true;

		// unfortunately, recalculating textures isn't possible yet
		ClearBits (v_texgamma.flags, FCVAR_CHANGED);
		ClearBits (v_lightgamma.flags, FCVAR_CHANGED);
		ClearBits (v_brightness.flags, FCVAR_CHANGED);
		ClearBits (v_gamma.flags, FCVAR_CHANGED);
		}

	if (notify_refdll && ref.initialized)
		ref.dllFuncs.R_GammaChanged (false);
	}

void V_Init (void)
	{
	Cvar_RegisterVariable (&v_texgamma);
	Cvar_RegisterVariable (&v_lightgamma);
	Cvar_RegisterVariable (&v_brightness);
	Cvar_RegisterVariable (&v_gamma);
	Cvar_RegisterVariable (&v_direct);

	// force gamma init
	SetBits (v_gamma.flags, FCVAR_CHANGED);
	V_CheckGamma ();
	}

byte TextureToGamma (byte b)
	{
	if (FBitSet (host.features, ENGINE_LINEAR_GAMMA_SPACE))
		return b;

	return texgammatable[b];
	}

byte LightToTexGamma (byte b)
	{
	if (FBitSet (host.features, ENGINE_LINEAR_GAMMA_SPACE))
		return b;

	// 255 << 2 is 1020, impossible to overflow
	return lightgammatable[b << 2] >> 2;
	}

uint LightToTexGammaEx (uint b)
	{
	if (FBitSet (host.features, ENGINE_LINEAR_GAMMA_SPACE))
		return b;

	if (unlikely (b > ARRAYSIZE (lightgammatable)))
		return 0;

	return lightgammatable[b];
	}

uint ScreenGammaTable (uint b)
	{
	if (FBitSet (host.features, ENGINE_LINEAR_GAMMA_SPACE))
		return b;

	if (unlikely (b > ARRAYSIZE (screengammatable)))
		return 0;

	return screengammatable[b];
	}

uint LinearGammaTable (uint b)
	{
	if (FBitSet (host.features, ENGINE_LINEAR_GAMMA_SPACE))
		return b;

	if (unlikely (b > ARRAYSIZE (lineargammatable)))
		return 0;
	return lineargammatable[b];
	}

#if XASH_ENGINE_TESTS
#include "tests.h"

typedef struct precomputed_gamma_tables_s
	{
	float gamma;
	float brightness;
	float texgamma;
	float lightgamma;
	byte  texgammatable[256];
	int   lightgammatable[1024];
	int   lineargammatable[1024];
	int   screengammatable[1024];
	} precomputed_gamma_tables_t;

// put at the end of the file, to not confuse Qt Creator's parser
precomputed_gamma_tables_t *Test_GetGammaTables (int i);

static void Test_PrecomputedGammaTables (void)
	{
	precomputed_gamma_tables_t *data;
	int i = 0;

	while ((data = Test_GetGammaTables (i)))
		{
		int j;

		BuildGammaTable (data->gamma, data->brightness, data->texgamma, data->lightgamma);

		for (j = 0; j < 1024; j++)
			{
			if (j < 256)
				{
				TASSERT_EQi (texgammatable[j], data->texgammatable[j]);
				}

			TASSERT_EQi (lightgammatable[j], data->lightgammatable[j]);
			TASSERT_EQi (lineargammatable[j], data->lineargammatable[j]);
			TASSERT_EQi (screengammatable[j], data->screengammatable[j]);
			}
		i++;
		}
	}

void Test_RunGamma (void)
	{
	TRUN (Test_PrecomputedGammaTables ());
	}

precomputed_gamma_tables_t *Test_GetGammaTables (int i)
	{
	static precomputed_gamma_tables_t precomputed_data[] = {
	{
		.gamma = 2.5,
		.brightness = 0.0,
		.texgamma = 2.0,
		.lightgamma = 2.5,
		.texgammatable = {
			0, 3, 5, 7, 9, 10, 12, 14, 15, 17, 19, 20, 22, 23, 25, 26, 27, 29, 30, 31, 33, 34, 35, 37, 38, 39, 41, 42, 43, 44, 46, 47, 48, 49, 50, 52, 53, 54, 55, 56, 57, 59, 60, 61, 62, 63, 64, 65, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 79, 80, 81, 82, 83, 84,
			85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 132, 133, 134, 135, 136, 137, 138,
			139, 140, 141, 142, 143, 144, 145, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 165, 166, 167, 168, 169, 170, 171, 172, 172, 173, 174, 175, 176, 177, 178, 179, 180, 180, 181, 182, 183, 184, 185,
			186, 186, 187, 188, 189, 190, 191, 192, 192, 193, 194, 195, 196, 197, 198, 198, 199, 200, 201, 202, 203, 204, 204, 205, 206, 207, 208, 209, 209, 210, 211, 212, 213, 214, 214, 215, 216, 217, 218, 219, 219, 220, 221, 222, 223, 224, 224, 225, 226, 227, 228, 229,
			229, 230, 231, 232, 233, 233, 234, 235, 236, 237, 238, 238, 239, 240, 241, 242, 242, 243, 244, 245, 246, 246, 247, 248, 249, 250, 250, 251, 252, 253, 254, 255
		},
		.lightgammatable = {
			0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64,
			65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123,
			124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
			176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227,
			228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255, 256, 257, 258, 259, 260, 261, 262, 263, 264, 265, 266, 267, 268, 269, 270, 271, 272, 273, 274, 275, 276, 277, 278, 279,
			280, 281, 282, 283, 284, 285, 286, 287, 288, 289, 290, 291, 292, 293, 294, 295, 296, 297, 298, 299, 300, 301, 302, 303, 304, 305, 306, 307, 308, 309, 310, 311, 312, 313, 314, 315, 316, 317, 318, 319, 320, 321, 322, 323, 324, 325, 326, 327, 328, 329, 330, 331,
			332, 333, 334, 335, 336, 337, 338, 339, 340, 341, 343, 343, 344, 345, 346, 347, 348, 349, 350, 351, 353, 353, 354, 355, 356, 357, 358, 359, 360, 361, 362, 363, 364, 365, 366, 367, 368, 369, 370, 371, 372, 373, 374, 376, 376, 377, 379, 379, 380, 381, 382, 383,
			384, 385, 386, 387, 388, 389, 390, 391, 392, 393, 394, 395, 396, 397, 398, 399, 400, 401, 402, 403, 404, 405, 406, 407, 408, 409, 410, 411, 412, 413, 414, 415, 416, 417, 418, 419, 420, 421, 422, 423, 424, 425, 426, 427, 428, 429, 430, 431, 432, 433, 434, 435,
			436, 437, 438, 439, 440, 441, 442, 443, 444, 446, 446, 447, 449, 449, 450, 451, 452, 453, 455, 455, 456, 457, 458, 459, 460, 461, 462, 463, 464, 465, 466, 468, 469, 470, 470, 472, 472, 473, 474, 475, 476, 477, 478, 479, 480, 481, 482, 483, 484, 485, 486, 487,
			488, 489, 490, 491, 493, 493, 494, 495, 496, 498, 498, 499, 500, 502, 502, 503, 504, 505, 507, 508, 508, 509, 510, 511, 512, 513, 514, 515, 516, 517, 518, 520, 521, 521, 522, 523, 524, 525, 526, 527, 528, 530, 530, 531, 532, 533, 534, 536, 536, 537, 538, 539,
			540, 541, 542, 543, 544, 545, 546, 547, 548, 549, 550, 552, 552, 553, 554, 555, 556, 557, 558, 559, 560, 561, 562, 564, 564, 565, 566, 567, 568, 570, 571, 571, 572, 573, 574, 576, 576, 577, 578, 579, 580, 581, 583, 583, 584, 585, 586, 587, 588, 590, 591, 591,
			593, 593, 594, 596, 596, 597, 598, 600, 600, 601, 602, 603, 604, 605, 606, 607, 608, 609, 611, 611, 612, 613, 615, 615, 616, 618, 618, 620, 621, 621, 622, 624, 625, 626, 627, 627, 628, 629, 630, 632, 633, 633, 634, 635, 636, 638, 638, 639, 641, 642, 643, 643,
			644, 645, 646, 648, 648, 650, 651, 651, 652, 654, 655, 656, 656, 658, 658, 660, 660, 662, 662, 664, 664, 665, 666, 668, 668, 669, 670, 671, 672, 673, 674, 675, 676, 677, 678, 679, 680, 682, 682, 683, 684, 685, 686, 687, 688, 690, 690, 691, 692, 693, 695, 696,
			696, 697, 698, 699, 700, 701, 702, 703, 705, 706, 706, 707, 708, 709, 710, 711, 712, 713, 715, 716, 716, 717, 718, 719, 721, 721, 722, 724, 724, 725, 727, 727, 728, 729, 730, 731, 733, 733, 734, 735, 736, 737, 738, 740, 740, 741, 743, 744, 744, 745, 747, 747,
			748, 749, 750, 752, 753, 753, 755, 755, 757, 757, 758, 759, 760, 761, 762, 764, 764, 766, 766, 767, 769, 770, 770, 771, 772, 773, 774, 775, 776, 777, 778, 779, 780, 782, 783, 784, 784, 785, 786, 788, 789, 790, 791, 792, 792, 794, 794, 796, 796, 797, 799, 800,
			801, 801, 802, 803, 804, 805, 806, 807, 809, 810, 810, 811, 813, 813, 815, 815, 816, 817, 819, 819, 820, 822, 823, 823, 824, 825, 826, 827, 829, 829, 830, 832, 832, 833, 835, 835, 837, 837, 839, 840, 840, 841, 842, 843, 844, 845, 847, 847, 849, 849, 850, 851,
			852, 854, 854, 856, 856, 857, 859, 859, 860, 861, 862, 864, 864, 865, 867, 867, 868, 870, 870, 872, 872, 874, 875, 875, 876, 877, 879, 880, 881, 881, 882, 883, 885, 885, 886, 887, 888, 890, 891, 891, 893, 893, 894, 895, 897, 898, 899, 900, 901, 902, 902, 903,
			904, 905, 907, 907, 908, 910, 910, 911, 912, 914, 914, 915, 917, 918, 919, 920, 921, 922, 922, 923, 925, 925, 926, 927, 928, 930, 931, 932, 932, 933, 935, 936, 936, 937, 939, 940, 940, 942, 942, 944, 944, 946, 947, 948, 949, 949, 950, 952, 952, 953, 955, 956,
			957, 958, 959, 959, 960, 962, 962, 964, 965, 965, 967, 967, 969, 970, 971, 972, 973, 973, 975, 975, 976, 977, 978, 979, 980, 982, 982, 984, 984, 986, 986, 988, 988, 990, 991, 992, 993, 993, 994, 995, 996, 998, 999, 999, 1000, 1002, 1002, 1004, 1005, 1005,
			1006, 1007, 1008, 1010, 1011, 1011, 1013, 1014, 1015, 1016, 1017, 1018, 1018, 1019, 1020, 1021, 1023
		},
		.lineargammatable = {
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
			2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 11, 11,
			11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 14, 14, 14, 14, 14, 15, 15, 15, 15, 15, 16, 16, 16, 16, 16, 17, 17, 17, 17, 17, 18, 18, 18, 18, 19, 19, 19, 19, 19, 20, 20, 20, 20, 21, 21, 21, 21, 22, 22, 22, 22, 23, 23, 23, 23, 24, 24, 24, 25, 25,
			25, 25, 26, 26, 26, 26, 27, 27, 27, 28, 28, 28, 29, 29, 29, 29, 30, 30, 30, 31, 31, 31, 32, 32, 32, 32, 33, 33, 33, 34, 34, 34, 35, 35, 35, 36, 36, 36, 37, 37, 37, 38, 38, 39, 39, 39, 40, 40, 40, 41, 41, 41, 42, 42, 43, 43, 43, 44, 44, 44, 45, 45, 46, 46, 46,
			47, 47, 48, 48, 48, 49, 49, 50, 50, 50, 51, 51, 52, 52, 52, 53, 53, 54, 54, 55, 55, 55, 56, 56, 57, 57, 58, 58, 59, 59, 60, 60, 60, 61, 61, 62, 62, 63, 63, 64, 64, 65, 65, 66, 66, 67, 67, 68, 68, 69, 69, 70, 70, 71, 71, 72, 72, 73, 73, 74, 74, 75, 75, 76, 76,
			77, 77, 78, 78, 79, 79, 80, 81, 81, 82, 82, 83, 83, 84, 84, 85, 86, 86, 87, 87, 88, 88, 89, 90, 90, 91, 91, 92, 92, 93, 94, 94, 95, 95, 96, 97, 97, 98, 99, 99, 100, 100, 101, 102, 102, 103, 104, 104, 105, 105, 106, 107, 107, 108, 109, 109, 110, 111, 111, 112,
			113, 113, 114, 115, 115, 116, 117, 117, 118, 119, 119, 120, 121, 122, 122, 123, 124, 124, 125, 126, 126, 127, 128, 129, 129, 130, 131, 132, 132, 133, 134, 134, 135, 136, 137, 137, 138, 139, 140, 140, 141, 142, 143, 144, 144, 145, 146, 147, 147, 148, 149, 150,
			151, 151, 152, 153, 154, 155, 155, 156, 157, 158, 159, 159, 160, 161, 162, 163, 164, 164, 165, 166, 167, 168, 169, 169, 170, 171, 172, 173, 174, 175, 176, 176, 177, 178, 179, 180, 181, 182, 183, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 193, 194,
			195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246,
			247, 248, 249, 250, 251, 252, 254, 255, 256, 257, 258, 259, 260, 261, 262, 263, 265, 266, 267, 268, 269, 270, 271, 272, 274, 275, 276, 277, 278, 279, 280, 282, 283, 284, 285, 286, 287, 289, 290, 291, 292, 293, 294, 296, 297, 298, 299, 300, 302, 303, 304, 305,
			306, 308, 309, 310, 311, 312, 314, 315, 316, 317, 319, 320, 321, 322, 324, 325, 326, 327, 329, 330, 331, 333, 334, 335, 336, 338, 339, 340, 342, 343, 344, 345, 347, 348, 349, 351, 352, 353, 355, 356, 357, 359, 360, 361, 363, 364, 365, 367, 368, 369, 371, 372,
			373, 375, 376, 378, 379, 380, 382, 383, 384, 386, 387, 389, 390, 391, 393, 394, 396, 397, 399, 400, 401, 403, 404, 406, 407, 409, 410, 411, 413, 414, 416, 417, 419, 420, 422, 423, 425, 426, 428, 429, 431, 432, 434, 435, 437, 438, 440, 441, 443, 444, 446, 447,
			449, 450, 452, 453, 455, 456, 458, 459, 461, 462, 464, 466, 467, 469, 470, 472, 473, 475, 477, 478, 480, 481, 483, 485, 486, 488, 489, 491, 493, 494, 496, 497, 499, 501, 502, 504, 506, 507, 509, 511, 512, 514, 515, 517, 519, 520, 522, 524, 525, 527, 529, 531,
			532, 534, 536, 537, 539, 541, 542, 544, 546, 548, 549, 551, 553, 554, 556, 558, 560, 561, 563, 565, 567, 568, 570, 572, 574, 575, 577, 579, 581, 583, 584, 586, 588, 590, 592, 593, 595, 597, 599, 601, 602, 604, 606, 608, 610, 612, 613, 615, 617, 619, 621, 623,
			625, 626, 628, 630, 632, 634, 636, 638, 639, 641, 643, 645, 647, 649, 651, 653, 655, 657, 659, 660, 662, 664, 666, 668, 670, 672, 674, 676, 678, 680, 682, 684, 686, 688, 690, 692, 694, 696, 698, 700, 702, 704, 706, 708, 710, 712, 714, 716, 718, 720, 722, 724,
			726, 728, 730, 732, 734, 736, 738, 740, 742, 744, 746, 748, 750, 753, 755, 757, 759, 761, 763, 765, 767, 769, 771, 773, 776, 778, 780, 782, 784, 786, 788, 791, 793, 795, 797, 799, 801, 803, 806, 808, 810, 812, 814, 816, 819, 821, 823, 825, 827, 830, 832, 834,
			836, 839, 841, 843, 845, 847, 850, 852, 854, 856, 859, 861, 863, 865, 868, 870, 872, 874, 877, 879, 881, 884, 886, 888, 890, 893, 895, 897, 900, 902, 904, 907, 909, 911, 914, 916, 918, 921, 923, 925, 928, 930, 932, 935, 937, 940, 942, 944, 947, 949, 952, 954,
			956, 959, 961, 964, 966, 968, 971, 973, 976, 978, 981, 983, 985, 988, 990, 993, 995, 998, 1000, 1003, 1005, 1008, 1010, 1013, 1015, 1018, 1020, 1023
		},
		.screengammatable = {
			0, 63, 84, 99, 111, 121, 130, 139, 146, 154, 160, 166, 172, 178, 183, 188, 193, 198, 203, 207, 212, 216, 220, 224, 228, 231, 235, 239, 242, 245, 249, 252, 255, 259, 262, 265, 268, 271, 274, 276, 279, 282, 285, 287, 290, 293, 295, 298, 300, 303, 305, 308,
			310, 313, 315, 317, 320, 322, 324, 326, 328, 331, 333, 335, 337, 339, 341, 343, 345, 347, 349, 351, 353, 355, 357, 359, 361, 363, 365, 367, 369, 370, 372, 374, 376, 378, 379, 381, 383, 385, 386, 388, 390, 392, 393, 395, 397, 398, 400, 401, 403, 405, 406, 408,
			409, 411, 413, 414, 416, 417, 419, 420, 422, 423, 425, 426, 428, 429, 431, 432, 434, 435, 436, 438, 439, 441, 442, 444, 445, 446, 448, 449, 450, 452, 453, 455, 456, 457, 459, 460, 461, 463, 464, 465, 466, 468, 469, 470, 472, 473, 474, 475, 477, 478, 479, 480,
			482, 483, 484, 485, 487, 488, 489, 490, 491, 493, 494, 495, 496, 497, 499, 500, 501, 502, 503, 504, 505, 507, 508, 509, 510, 511, 512, 513, 515, 516, 517, 518, 519, 520, 521, 522, 523, 524, 526, 527, 528, 529, 530, 531, 532, 533, 534, 535, 536, 537, 538, 539,
			540, 541, 543, 544, 545, 546, 547, 548, 549, 550, 551, 552, 553, 554, 555, 556, 557, 558, 559, 560, 561, 562, 563, 564, 565, 566, 567, 568, 568, 569, 570, 571, 572, 573, 574, 575, 576, 577, 578, 579, 580, 581, 582, 583, 584, 585, 585, 586, 587, 588, 589, 590,
			591, 592, 593, 594, 595, 595, 596, 597, 598, 599, 600, 601, 602, 603, 603, 604, 605, 606, 607, 608, 609, 610, 610, 611, 612, 613, 614, 615, 616, 616, 617, 618, 619, 620, 621, 622, 622, 623, 624, 625, 626, 627, 627, 628, 629, 630, 631, 632, 632, 633, 634, 635,
			636, 637, 637, 638, 639, 640, 641, 641, 642, 643, 644, 645, 645, 646, 647, 648, 649, 649, 650, 651, 652, 652, 653, 654, 655, 656, 656, 657, 658, 659, 659, 660, 661, 662, 663, 663, 664, 665, 666, 666, 667, 668, 669, 669, 670, 671, 672, 672, 673, 674, 675, 675,
			676, 677, 678, 678, 679, 680, 681, 681, 682, 683, 684, 684, 685, 686, 686, 687, 688, 689, 689, 690, 691, 692, 692, 693, 694, 694, 695, 696, 697, 697, 698, 699, 699, 700, 701, 701, 702, 703, 704, 704, 705, 706, 706, 707, 708, 708, 709, 710, 711, 711, 712, 713,
			713, 714, 715, 715, 716, 717, 717, 718, 719, 719, 720, 721, 721, 722, 723, 723, 724, 725, 725, 726, 727, 727, 728, 729, 729, 730, 731, 731, 732, 733, 733, 734, 735, 735, 736, 737, 737, 738, 739, 739, 740, 741, 741, 742, 743, 743, 744, 745, 745, 746, 746, 747,
			748, 748, 749, 750, 750, 751, 752, 752, 753, 753, 754, 755, 755, 756, 757, 757, 758, 758, 759, 760, 760, 761, 762, 762, 763, 763, 764, 765, 765, 766, 767, 767, 768, 768, 769, 770, 770, 771, 771, 772, 773, 773, 774, 774, 775, 776, 776, 777, 778, 778, 779, 779,
			780, 781, 781, 782, 782, 783, 784, 784, 785, 785, 786, 786, 787, 788, 788, 789, 789, 790, 791, 791, 792, 792, 793, 794, 794, 795, 795, 796, 796, 797, 798, 798, 799, 799, 800, 801, 801, 802, 802, 803, 803, 804, 805, 805, 806, 806, 807, 807, 808, 809, 809, 810,
			810, 811, 811, 812, 813, 813, 814, 814, 815, 815, 816, 816, 817, 818, 818, 819, 819, 820, 820, 821, 821, 822, 823, 823, 824, 824, 825, 825, 826, 826, 827, 828, 828, 829, 829, 830, 830, 831, 831, 832, 832, 833, 834, 834, 835, 835, 836, 836, 837, 837, 838, 838,
			839, 839, 840, 841, 841, 842, 842, 843, 843, 844, 844, 845, 845, 846, 846, 847, 848, 848, 849, 849, 850, 850, 851, 851, 852, 852, 853, 853, 854, 854, 855, 855, 856, 856, 857, 857, 858, 859, 859, 860, 860, 861, 861, 862, 862, 863, 863, 864, 864, 865, 865, 866,
			866, 867, 867, 868, 868, 869, 869, 870, 870, 871, 871, 872, 872, 873, 873, 874, 874, 875, 875, 876, 876, 877, 877, 878, 878, 879, 879, 880, 880, 881, 881, 882, 882, 883, 883, 884, 884, 885, 885, 886, 886, 887, 887, 888, 888, 889, 889, 890, 890, 891, 891, 892,
			892, 893, 893, 894, 894, 895, 895, 896, 896, 897, 897, 898, 898, 899, 899, 900, 900, 901, 901, 902, 902, 903, 903, 904, 904, 904, 905, 905, 906, 906, 907, 907, 908, 908, 909, 909, 910, 910, 911, 911, 912, 912, 913, 913, 914, 914, 915, 915, 915, 916, 916, 917,
			917, 918, 918, 919, 919, 920, 920, 921, 921, 922, 922, 922, 923, 923, 924, 924, 925, 925, 926, 926, 927, 927, 928, 928, 929, 929, 929, 930, 930, 931, 931, 932, 932, 933, 933, 934, 934, 935, 935, 935, 936, 936, 937, 937, 938, 938, 939, 939, 940, 940, 940, 941,
			941, 942, 942, 943, 943, 944, 944, 944, 945, 945, 946, 946, 947, 947, 948, 948, 949, 949, 949, 950, 950, 951, 951, 952, 952, 953, 953, 953, 954, 954, 955, 955, 956, 956, 957, 957, 957, 958, 958, 959, 959, 960, 960, 961, 961, 961, 962, 962, 963, 963, 964, 964,
			964, 965, 965, 966, 966, 967, 967, 968, 968, 968, 969, 969, 970, 970, 971, 971, 971, 972, 972, 973, 973, 974, 974, 974, 975, 975, 976, 976, 977, 977, 977, 978, 978, 979, 979, 980, 980, 980, 981, 981, 982, 982, 983, 983, 983, 984, 984, 985, 985, 986, 986, 986,
			987, 987, 988, 988, 988, 989, 989, 990, 990, 991, 991, 991, 992, 992, 993, 993, 993, 994, 994, 995, 995, 996, 996, 996, 997, 997, 998, 998, 998, 999, 999, 1000, 1000, 1001, 1001, 1001, 1002, 1002, 1003, 1003, 1003, 1004, 1004, 1005, 1005, 1005, 1006, 1006,
			1007, 1007, 1008, 1008, 1008, 1009, 1009, 1010, 1010, 1010, 1011, 1011, 1012, 1012, 1012, 1013, 1013, 1014, 1014, 1014, 1015, 1015, 1016, 1016, 1016, 1017, 1017, 1018, 1018, 1018, 1019, 1019, 1020, 1020, 1020, 1021, 1021, 1022, 1022, 1023
		},
	}, {
		.gamma = 2.2,
		.brightness = 1.0,
		.texgamma = 2.2,
		.lightgamma = 2.4,
		.texgammatable = {
			0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65,
			66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123,
			124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
			176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227,
			228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255
		},
		.lightgammatable = {
			0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 12, 13, 14, 15, 16, 17, 18, 20, 21, 22, 23, 24, 25, 27, 28, 29, 30, 31, 33, 34, 35, 36, 37, 39, 40, 41, 42, 43, 45, 46, 47, 48, 50, 51, 52, 53, 55, 56, 57, 58, 60, 61, 62, 63, 65, 66, 67, 69, 70, 71, 72, 74, 75, 76,
			78, 79, 80, 81, 83, 84, 85, 87, 88, 89, 91, 92, 93, 94, 96, 97, 98, 100, 101, 102, 104, 105, 106, 108, 109, 110, 112, 113, 114, 116, 117, 118, 120, 121, 122, 124, 125, 126, 128, 129, 130, 132, 133, 134, 136, 137, 138, 140, 141, 142, 144, 145, 147, 148, 149,
			151, 152, 153, 155, 156, 157, 159, 160, 162, 163, 164, 166, 167, 168, 170, 171, 173, 174, 175, 177, 178, 179, 181, 182, 184, 185, 186, 188, 189, 191, 192, 193, 195, 196, 198, 199, 200, 202, 203, 204, 206, 207, 209, 210, 211, 213, 214, 216, 217, 219, 220, 221,
			223, 224, 226, 227, 228, 230, 231, 233, 234, 235, 237, 238, 240, 241, 243, 244, 245, 247, 248, 250, 251, 252, 254, 255, 257, 258, 260, 261, 262, 264, 265, 267, 268, 270, 271, 272, 274, 275, 277, 278, 280, 281, 282, 284, 285, 287, 288, 290, 291, 293, 294, 295,
			297, 298, 300, 301, 303, 304, 306, 307, 308, 310, 311, 313, 314, 316, 317, 319, 320, 321, 323, 324, 326, 327, 329, 330, 332, 333, 335, 336, 337, 339, 340, 342, 343, 345, 346, 348, 349, 351, 352, 354, 355, 356, 358, 359, 361, 362, 364, 365, 367, 368, 370, 371,
			373, 374, 376, 377, 378, 380, 381, 383, 384, 386, 387, 389, 390, 392, 393, 395, 396, 397, 398, 398, 399, 399, 400, 401, 401, 402, 402, 403, 403, 404, 404, 405, 406, 406, 407, 407, 408, 408, 409, 410, 410, 411, 411, 412, 413, 413, 414, 414, 415, 416, 416, 417,
			417, 418, 419, 419, 420, 420, 421, 422, 422, 423, 423, 424, 425, 425, 426, 427, 427, 428, 428, 429, 430, 430, 431, 432, 432, 433, 433, 434, 435, 435, 436, 437, 437, 438, 439, 439, 440, 441, 441, 442, 443, 443, 444, 445, 445, 446, 447, 447, 448, 449, 449, 450,
			451, 451, 452, 453, 453, 454, 455, 455, 456, 457, 457, 458, 459, 459, 460, 461, 461, 462, 463, 464, 464, 465, 466, 466, 467, 468, 468, 469, 470, 471, 471, 472, 473, 473, 474, 475, 476, 476, 477, 478, 478, 479, 480, 481, 481, 482, 483, 483, 484, 485, 486, 486,
			487, 488, 489, 489, 490, 491, 492, 492, 493, 494, 495, 495, 496, 497, 498, 498, 499, 500, 501, 501, 502, 503, 504, 504, 505, 506, 507, 507, 508, 509, 510, 510, 511, 512, 513, 513, 514, 515, 516, 517, 517, 518, 519, 520, 520, 521, 522, 523, 524, 524, 525, 526,
			527, 527, 528, 529, 530, 531, 531, 532, 533, 534, 535, 535, 536, 537, 538, 538, 539, 540, 541, 542, 542, 543, 544, 545, 546, 546, 547, 548, 549, 550, 550, 551, 552, 553, 554, 555, 555, 556, 557, 558, 559, 559, 560, 561, 562, 563, 564, 564, 565, 566, 567, 568,
			568, 569, 570, 571, 572, 573, 573, 574, 575, 576, 577, 578, 578, 579, 580, 581, 582, 583, 583, 584, 585, 586, 587, 588, 588, 589, 590, 591, 592, 593, 593, 594, 595, 596, 597, 598, 599, 599, 600, 601, 602, 603, 604, 605, 605, 606, 607, 608, 609, 610, 611, 611,
			612, 613, 614, 615, 616, 617, 617, 618, 619, 620, 621, 622, 623, 623, 624, 625, 626, 627, 628, 629, 629, 630, 631, 632, 633, 634, 635, 636, 636, 637, 638, 639, 640, 641, 642, 643, 643, 644, 645, 646, 647, 648, 649, 650, 651, 651, 652, 653, 654, 655, 656, 657,
			658, 658, 659, 660, 661, 662, 663, 664, 665, 666, 666, 667, 668, 669, 670, 671, 672, 673, 674, 675, 675, 676, 677, 678, 679, 680, 681, 682, 683, 684, 684, 685, 686, 687, 688, 689, 690, 691, 692, 693, 693, 694, 695, 696, 697, 698, 699, 700, 701, 702, 703, 703,
			704, 705, 706, 707, 708, 709, 710, 711, 712, 713, 713, 714, 715, 716, 717, 718, 719, 720, 721, 722, 723, 724, 724, 725, 726, 727, 728, 729, 730, 731, 732, 733, 734, 735, 736, 736, 737, 738, 739, 740, 741, 742, 743, 744, 745, 746, 747, 748, 749, 749, 750, 751,
			752, 753, 754, 755, 756, 757, 758, 759, 760, 761, 762, 763, 763, 764, 765, 766, 767, 768, 769, 770, 771, 772, 773, 774, 775, 776, 777, 778, 778, 779, 780, 781, 782, 783, 784, 785, 786, 787, 788, 789, 790, 791, 792, 793, 794, 795, 795, 796, 797, 798, 799, 800,
			801, 802, 803, 804, 805, 806, 807, 808, 809, 810, 811, 812, 813, 814, 814, 815, 816, 817, 818, 819, 820, 821, 822, 823, 824, 825, 826, 827, 828, 829, 830, 831, 832, 833, 834, 835, 836, 836, 837, 838, 839, 840, 841, 842, 843, 844, 845, 846, 847, 848, 849, 850,
			851, 852, 853, 854, 855, 856, 857, 858, 859, 860, 861, 862, 863, 863, 864, 865, 866, 867, 868, 869, 870, 871, 872, 873, 874, 875, 876, 877, 878, 879, 880, 881, 882, 883, 884, 885, 886, 887, 888, 889, 890, 891, 892, 893, 894, 895, 896, 897, 898, 899, 900, 900,
			901, 902, 903, 904, 905, 906, 907, 908, 909, 910, 911, 912, 913, 914, 915, 916, 917, 918, 919, 920, 921, 922, 923, 924, 925, 926, 927, 928, 929, 930, 931, 932, 933, 934, 935, 936, 937, 938, 939, 940, 941, 942, 943, 944, 945, 946, 947, 948, 949, 950, 951, 952,
			953, 954, 955, 956, 957, 958, 959, 960, 961, 962, 963, 964, 965, 966, 967, 968, 969, 970, 971, 972, 973, 974, 974, 975, 976, 977, 978, 979, 980, 981, 982, 983, 984, 985, 986, 987, 988, 989, 990, 991, 992, 993, 994, 995, 996, 997, 998, 999, 1000, 1001, 1002,
			1003, 1004, 1005, 1006, 1007, 1008, 1009, 1010, 1011, 1012, 1013, 1014, 1015, 1016, 1017, 1018, 1019, 1020, 1021, 1023
		},
		.lineargammatable = {
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4,
			4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 14, 14, 14, 14, 14, 15, 15, 15, 15, 16, 16, 16, 16, 17,
			17, 17, 17, 17, 18, 18, 18, 18, 19, 19, 19, 19, 20, 20, 20, 21, 21, 21, 21, 22, 22, 22, 22, 23, 23, 23, 24, 24, 24, 24, 25, 25, 25, 26, 26, 26, 26, 27, 27, 27, 28, 28, 28, 29, 29, 29, 30, 30, 30, 31, 31, 31, 32, 32, 32, 33, 33, 33, 34, 34, 34, 35, 35, 35, 36,
			36, 36, 37, 37, 38, 38, 38, 39, 39, 39, 40, 40, 40, 41, 41, 42, 42, 42, 43, 43, 44, 44, 44, 45, 45, 46, 46, 46, 47, 47, 48, 48, 48, 49, 49, 50, 50, 51, 51, 51, 52, 52, 53, 53, 54, 54, 55, 55, 55, 56, 56, 57, 57, 58, 58, 59, 59, 60, 60, 61, 61, 61, 62, 62, 63,
			63, 64, 64, 65, 65, 66, 66, 67, 67, 68, 68, 69, 69, 70, 70, 71, 71, 72, 72, 73, 73, 74, 75, 75, 76, 76, 77, 77, 78, 78, 79, 79, 80, 80, 81, 82, 82, 83, 83, 84, 84, 85, 86, 86, 87, 87, 88, 88, 89, 90, 90, 91, 91, 92, 93, 93, 94, 94, 95, 96, 96, 97, 97, 98, 99,
			99, 100, 100, 101, 102, 102, 103, 104, 104, 105, 105, 106, 107, 107, 108, 109, 109, 110, 111, 111, 112, 113, 113, 114, 115, 115, 116, 117, 117, 118, 119, 119, 120, 121, 121, 122, 123, 123, 124, 125, 126, 126, 127, 128, 128, 129, 130, 131, 131, 132, 133, 133,
			134, 135, 136, 136, 137, 138, 139, 139, 140, 141, 142, 142, 143, 144, 145, 145, 146, 147, 148, 148, 149, 150, 151, 151, 152, 153, 154, 155, 155, 156, 157, 158, 159, 159, 160, 161, 162, 163, 163, 164, 165, 166, 167, 167, 168, 169, 170, 171, 172, 172, 173, 174,
			175, 176, 177, 177, 178, 179, 180, 181, 182, 183, 183, 184, 185, 186, 187, 188, 189, 190, 190, 191, 192, 193, 194, 195, 196, 197, 198, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221,
			222, 223, 224, 225, 226, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255, 257, 258, 259, 260, 261, 262, 263, 264, 265, 266, 267, 268, 269, 270, 271, 272, 273,
			274, 276, 277, 278, 279, 280, 281, 282, 283, 284, 285, 286, 288, 289, 290, 291, 292, 293, 294, 295, 296, 298, 299, 300, 301, 302, 303, 304, 305, 307, 308, 309, 310, 311, 312, 313, 315, 316, 317, 318, 319, 320, 322, 323, 324, 325, 326, 328, 329, 330, 331, 332,
			333, 335, 336, 337, 338, 339, 341, 342, 343, 344, 346, 347, 348, 349, 350, 352, 353, 354, 355, 357, 358, 359, 360, 362, 363, 364, 365, 367, 368, 369, 370, 372, 373, 374, 375, 377, 378, 379, 381, 382, 383, 384, 386, 387, 388, 390, 391, 392, 393, 395, 396, 397,
			399, 400, 401, 403, 404, 405, 407, 408, 409, 411, 412, 413, 415, 416, 417, 419, 420, 421, 423, 424, 426, 427, 428, 430, 431, 432, 434, 435, 437, 438, 439, 441, 442, 443, 445, 446, 448, 449, 450, 452, 453, 455, 456, 458, 459, 460, 462, 463, 465, 466, 468, 469,
			470, 472, 473, 475, 476, 478, 479, 481, 482, 483, 485, 486, 488, 489, 491, 492, 494, 495, 497, 498, 500, 501, 503, 504, 506, 507, 509, 510, 512, 513, 515, 516, 518, 519, 521, 522, 524, 525, 527, 528, 530, 532, 533, 535, 536, 538, 539, 541, 542, 544, 545, 547,
			549, 550, 552, 553, 555, 556, 558, 560, 561, 563, 564, 566, 568, 569, 571, 572, 574, 576, 577, 579, 580, 582, 584, 585, 587, 589, 590, 592, 593, 595, 597, 598, 600, 602, 603, 605, 607, 608, 610, 612, 613, 615, 617, 618, 620, 622, 623, 625, 627, 628, 630, 632,
			633, 635, 637, 639, 640, 642, 644, 645, 647, 649, 650, 652, 654, 656, 657, 659, 661, 663, 664, 666, 668, 670, 671, 673, 675, 677, 678, 680, 682, 684, 685, 687, 689, 691, 692, 694, 696, 698, 700, 701, 703, 705, 707, 709, 710, 712, 714, 716, 718, 719, 721, 723,
			725, 727, 729, 730, 732, 734, 736, 738, 740, 741, 743, 745, 747, 749, 751, 753, 754, 756, 758, 760, 762, 764, 766, 767, 769, 771, 773, 775, 777, 779, 781, 783, 785, 786, 788, 790, 792, 794, 796, 798, 800, 802, 804, 806, 808, 809, 811, 813, 815, 817, 819, 821,
			823, 825, 827, 829, 831, 833, 835, 837, 839, 841, 843, 845, 847, 849, 851, 853, 855, 857, 859, 861, 863, 865, 867, 869, 871, 873, 875, 877, 879, 881, 883, 885, 887, 889, 891, 893, 895, 897, 899, 901, 903, 905, 907, 910, 912, 914, 916, 918, 920, 922, 924, 926,
			928, 930, 932, 934, 937, 939, 941, 943, 945, 947, 949, 951, 953, 956, 958, 960, 962, 964, 966, 968, 970, 973, 975, 977, 979, 981, 983, 985, 988, 990, 992, 994, 996, 998, 1001, 1003, 1005, 1007, 1009, 1012, 1014, 1016, 1018, 1020, 1023
		},
		.screengammatable = {
			0, 43, 60, 72, 82, 91, 98, 106, 112, 118, 124, 130, 135, 140, 145, 150, 154, 158, 163, 167, 171, 174, 178, 182, 185, 189, 192, 196, 199, 202, 205, 208, 211, 214, 217, 220, 223, 226, 228, 231, 234, 237, 239, 242, 244, 247, 249, 252, 254, 257, 259, 261,
			264, 266, 268, 270, 273, 275, 277, 279, 281, 283, 286, 288, 290, 292, 294, 296, 298, 300, 302, 304, 306, 308, 310, 311, 313, 315, 317, 319, 321, 323, 324, 326, 328, 330, 331, 333, 335, 337, 338, 340, 342, 343, 345, 347, 348, 350, 352, 353, 355, 357, 358, 360,
			361, 363, 365, 366, 368, 369, 371, 372, 374, 375, 377, 378, 380, 381, 383, 384, 386, 387, 389, 390, 392, 393, 394, 396, 397, 399, 400, 401, 403, 404, 406, 407, 408, 410, 411, 412, 414, 415, 416, 418, 419, 420, 422, 423, 424, 426, 427, 428, 430, 431, 432, 433,
			435, 436, 437, 438, 440, 441, 442, 443, 445, 446, 447, 448, 450, 451, 452, 453, 454, 456, 457, 458, 459, 460, 462, 463, 464, 465, 466, 467, 469, 470, 471, 472, 473, 474, 475, 477, 478, 479, 480, 481, 482, 483, 484, 486, 487, 488, 489, 490, 491, 492, 493, 494,
			495, 497, 498, 499, 500, 501, 502, 503, 504, 505, 506, 507, 508, 509, 510, 511, 512, 513, 514, 516, 517, 518, 519, 520, 521, 522, 523, 524, 525, 526, 527, 528, 529, 530, 531, 532, 533, 534, 535, 536, 537, 538, 539, 540, 541, 542, 543, 544, 545, 545, 546, 547,
			548, 549, 550, 551, 552, 553, 554, 555, 556, 557, 558, 559, 560, 561, 562, 563, 563, 564, 565, 566, 567, 568, 569, 570, 571, 572, 573, 574, 574, 575, 576, 577, 578, 579, 580, 581, 582, 583, 583, 584, 585, 586, 587, 588, 589, 590, 591, 591, 592, 593, 594, 595,
			596, 597, 598, 598, 599, 600, 601, 602, 603, 604, 604, 605, 606, 607, 608, 609, 609, 610, 611, 612, 613, 614, 615, 615, 616, 617, 618, 619, 620, 620, 621, 622, 623, 624, 624, 625, 626, 627, 628, 629, 629, 630, 631, 632, 633, 633, 634, 635, 636, 637, 637, 638,
			639, 640, 641, 641, 642, 643, 644, 645, 645, 646, 647, 648, 649, 649, 650, 651, 652, 652, 653, 654, 655, 656, 656, 657, 658, 659, 659, 660, 661, 662, 663, 663, 664, 665, 666, 666, 667, 668, 669, 669, 670, 671, 672, 672, 673, 674, 675, 675, 676, 677, 678, 678,
			679, 680, 681, 681, 682, 683, 684, 684, 685, 686, 686, 687, 688, 689, 689, 690, 691, 692, 692, 693, 694, 694, 695, 696, 697, 697, 698, 699, 700, 700, 701, 702, 702, 703, 704, 705, 705, 706, 707, 707, 708, 709, 709, 710, 711, 712, 712, 713, 714, 714, 715, 716,
			716, 717, 718, 719, 719, 720, 721, 721, 722, 723, 723, 724, 725, 725, 726, 727, 728, 728, 729, 730, 730, 731, 732, 732, 733, 734, 734, 735, 736, 736, 737, 738, 738, 739, 740, 740, 741, 742, 742, 743, 744, 744, 745, 746, 746, 747, 748, 748, 749, 750, 750, 751,
			752, 752, 753, 754, 754, 755, 756, 756, 757, 758, 758, 759, 759, 760, 761, 761, 762, 763, 763, 764, 765, 765, 766, 767, 767, 768, 769, 769, 770, 770, 771, 772, 772, 773, 774, 774, 775, 776, 776, 777, 777, 778, 779, 779, 780, 781, 781, 782, 782, 783, 784, 784,
			785, 786, 786, 787, 787, 788, 789, 789, 790, 791, 791, 792, 792, 793, 794, 794, 795, 795, 796, 797, 797, 798, 799, 799, 800, 800, 801, 802, 802, 803, 803, 804, 805, 805, 806, 806, 807, 808, 808, 809, 809, 810, 811, 811, 812, 812, 813, 814, 814, 815, 815, 816,
			817, 817, 818, 818, 819, 820, 820, 821, 821, 822, 823, 823, 824, 824, 825, 825, 826, 827, 827, 828, 828, 829, 830, 830, 831, 831, 832, 833, 833, 834, 834, 835, 835, 836, 837, 837, 838, 838, 839, 839, 840, 841, 841, 842, 842, 843, 843, 844, 845, 845, 846, 846,
			847, 847, 848, 849, 849, 850, 850, 851, 851, 852, 853, 853, 854, 854, 855, 855, 856, 857, 857, 858, 858, 859, 859, 860, 860, 861, 862, 862, 863, 863, 864, 864, 865, 865, 866, 867, 867, 868, 868, 869, 869, 870, 870, 871, 872, 872, 873, 873, 874, 874, 875, 875,
			876, 876, 877, 878, 878, 879, 879, 880, 880, 881, 881, 882, 882, 883, 884, 884, 885, 885, 886, 886, 887, 887, 888, 888, 889, 889, 890, 891, 891, 892, 892, 893, 893, 894, 894, 895, 895, 896, 896, 897, 898, 898, 899, 899, 900, 900, 901, 901, 902, 902, 903, 903,
			904, 904, 905, 905, 906, 906, 907, 908, 908, 909, 909, 910, 910, 911, 911, 912, 912, 913, 913, 914, 914, 915, 915, 916, 916, 917, 917, 918, 918, 919, 920, 920, 921, 921, 922, 922, 923, 923, 924, 924, 925, 925, 926, 926, 927, 927, 928, 928, 929, 929, 930, 930,
			931, 931, 932, 932, 933, 933, 934, 934, 935, 935, 936, 936, 937, 937, 938, 938, 939, 939, 940, 940, 941, 941, 942, 942, 943, 943, 944, 944, 945, 945, 946, 946, 947, 947, 948, 948, 949, 949, 950, 950, 951, 951, 952, 952, 953, 953, 954, 954, 955, 955, 956, 956,
			957, 957, 958, 958, 959, 959, 960, 960, 961, 961, 962, 962, 963, 963, 964, 964, 965, 965, 966, 966, 967, 967, 968, 968, 969, 969, 969, 970, 970, 971, 971, 972, 972, 973, 973, 974, 974, 975, 975, 976, 976, 977, 977, 978, 978, 979, 979, 980, 980, 981, 981, 982,
			982, 982, 983, 983, 984, 984, 985, 985, 986, 986, 987, 987, 988, 988, 989, 989, 990, 990, 991, 991, 991, 992, 992, 993, 993, 994, 994, 995, 995, 996, 996, 997, 997, 998, 998, 999, 999, 999, 1000, 1000, 1001, 1001, 1002, 1002, 1003, 1003, 1004, 1004, 1005,
			1005, 1006, 1006, 1006, 1007, 1007, 1008, 1008, 1009, 1009, 1010, 1010, 1011, 1011, 1012, 1012, 1012, 1013, 1013, 1014, 1014, 1015, 1015, 1016, 1016, 1017, 1017, 1017, 1018, 1018, 1019, 1019, 1020, 1020, 1021, 1021, 1022, 1022, 1023
		},
	}
		};

	if (i < 0 || i >= ARRAYSIZE (precomputed_data))
		return NULL;

	return &precomputed_data[i];
	}
#endif
