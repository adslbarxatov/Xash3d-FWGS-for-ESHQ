/*
console.c - developer console
Copyright (C) 2007 Uncle Mike

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
#include "keydefs.h"
#include "protocol.h"		// get the protocol version
#include "con_nprint.h"
#include "qfont.h"
#include "wadfile.h"
#include "input.h"

convar_t *con_notifytime;
convar_t *scr_conspeed;
convar_t *con_fontsize;
convar_t *con_charset;
convar_t *con_fontscale;
convar_t *con_fontnum;
convar_t *con_color;

static int g_codepage = 0;
static qboolean g_utf8 = false;

static qboolean g_messagemode_privileged = true;

#define CON_TIMES		4	// notify lines
#define CON_MAX_TIMES	64	// notify max lines
#define COLOR_DEFAULT	'7'
#define CON_HISTORY		64
#define MAX_DBG_NOTIFY	128
#if XASH_LOW_MEMORY
#define CON_NUMFONTS	1		// do not load different font textures
#define CON_TEXTSIZE	32768	// max scrollback buffer characters in console (32 kb)
#define CON_MAXLINES	2048	// max scrollback buffer lines in console
#else
#define CON_NUMFONTS	3	// maxfonts
#define CON_TEXTSIZE	1048576	// max scrollback buffer characters in console (1 Mb)
#define CON_MAXLINES	16384	// max scrollback buffer lines in console
#endif
#define CON_LINES( i )	(con.lines[(con.lines_first + (i)) % con.maxlines])
#define CON_LINES_COUNT	con.lines_count
#define CON_LINES_LAST()	CON_LINES( CON_LINES_COUNT - 1 )

// console color typeing
rgba_t g_color_table[8] =
	{
	{   0,   0,   0, 255 },	// black
	{ 255,   0,   0, 255 },	// red
	{   0, 255,   0, 255 },	// green
	{ 255, 255,   0, 255 },	// yellow
	{   0,   0, 255, 255 },	// blue
	{   0, 255, 255, 255 },	// cyan
	{ 255,   0, 255, 255 },	// magenta
	{ 240, 180,  24, 255 },	// default color (can be changed by user)
	};

typedef struct
	{
	string		szNotify;
	float		expire;
	rgba_t		color;
	int		key_dest;
	} notify_t;

typedef struct con_lineinfo_s
	{
	char *start;
	size_t		length;
	double		addtime;		// notify stuff
	} con_lineinfo_t;

typedef struct con_history_s
	{
	field_t lines[CON_HISTORY];
	field_t backup;
	int     line; // the line being displayed from history buffer will be <= nextHistoryLine
	int     next; // the last line in the history buffer, not masked
	} con_history_t;

typedef struct
	{
	qboolean		initialized;

	// conbuffer
	char *buffer;		// common buffer for all console lines
	int		bufsize;		// CON_TEXSIZE
	con_lineinfo_t *lines;		// console lines
	int		maxlines;		// CON_MAXLINES

	int		lines_first;	// cyclic buffer
	int		lines_count;
	int		num_times;	// overlay lines count

	// console scroll
	int		backscroll;	// lines up from bottom to display
	int 		linewidth;	// characters across screen

	// console animation
	float		showlines;	// how many lines we should display
	float		vislines;		// in scanlines

	// console images
	int		background;	// console background

	// console fonts
	cl_font_t		chars[CON_NUMFONTS];// fonts.wad/font1.fnt
	cl_font_t *curFont;

	// console input
	field_t		input;

	// chatfiled
	field_t		chat;
	string		chat_cmd;		// can be overrieded by user

	// console history
	con_history_t	history;
	qboolean		historyLoaded;

	notify_t		notify[MAX_DBG_NOTIFY]; // for Con_NXPrintf
	qboolean		draw_notify;	// true if we have NXPrint message

	// console update
	double		lastupdate;
	} console_t;

static console_t		con;

void Con_ClearField (field_t *edit);
void Field_CharEvent (field_t *edit, int ch);

static void Con_LoadHistory (con_history_t *self);
static void Con_SaveHistory (con_history_t *self);

/*
================
Con_Clear_f
================
*/
void Con_Clear_f (void)
	{
	con.lines_count = 0;
	con.backscroll = 0; // go to end
	}

/*
================
Con_SetColor
================
*/
static void Con_SetColor (void)
	{
	vec3_t color;
	int r, g, b;
	int num;

	if (!FBitSet (con_color->flags, FCVAR_CHANGED))
		return;

	num = sscanf (con_color->string, "%i %i %i", &r, &g, &b);

	switch (num)
		{
		case 1:
			Con_DefaultColor (r, r, r);
			break;
		case 3:
			Con_DefaultColor (r, g, b);
			break;
		default:
			Cvar_DirectSet (con_color, con_color->def_string);
			break;
		}

	ClearBits (con_color->flags, FCVAR_CHANGED);
	}

/*
================
Con_ClearNotify
================
*/
void Con_ClearNotify (void)
	{
	int	i;

	for (i = 0; i < CON_LINES_COUNT; i++)
		CON_LINES (i).addtime = 0.0;
	}

/*
================
Con_ClearTyping
================
*/
void Con_ClearTyping (void)
	{
	Con_ClearField (&con.input);
	con.input.widthInChars = con.linewidth;

	Cmd_AutoCompleteClear ();
	}

/* [FWGS, 01.05.23]
============
Con_StringLength

skipped color prefixes
============
int Con_StringLength (const char *string)
	{
	int		len;
	const char *p;

	if (!string) return 0;

	len = 0;
	p = string;

	while (*p)
		{
		if (IsColorString (p))
			{
			p += 2;
			continue;
			}
		len++;
		p++;
		}

	return len;
	}
*/

/*
================
Con_MessageMode_f
================
*/
void Con_MessageMode_f (void)
	{
	g_messagemode_privileged = Cmd_CurrentCommandIsPrivileged ();

	if (Cmd_Argc () == 2)
		Q_strncpy (con.chat_cmd, Cmd_Argv (1), sizeof (con.chat_cmd));
	else Q_strncpy (con.chat_cmd, "say", sizeof (con.chat_cmd));

	Key_SetKeyDest (key_message);
	}

/*
================
Con_MessageMode2_f
================
*/
void Con_MessageMode2_f (void)
	{
	g_messagemode_privileged = Cmd_CurrentCommandIsPrivileged ();

	Q_strncpy (con.chat_cmd, "say_team", sizeof (con.chat_cmd));
	Key_SetKeyDest (key_message);
	}

/*
================
Con_ToggleConsole_f
================
*/
void Con_ToggleConsole_f (void)
	{
	if (!host.allow_console || UI_CreditsActive ())
		return; // disabled

	SCR_EndLoadingPlaque ();

	// show console only in game or by special call from menu
	if ((cls.state != ca_active) || (cls.key_dest == key_menu))
		return;

	Con_ClearTyping ();
	Con_ClearNotify ();

	if (cls.key_dest == key_console)
		{
		if (Cvar_VariableInteger ("sv_background") || Cvar_VariableInteger ("cl_background"))
			UI_SetActiveMenu (true);
		else
			UI_SetActiveMenu (false);
		}
	else
		{
		UI_SetActiveMenu (false);
		Key_SetKeyDest (key_console);
		}
	}

/*
================
Con_SetTimes_f
================
*/
void Con_SetTimes_f (void)
	{
	int	newtimes;

	if (Cmd_Argc () != 2)
		{
		Con_Printf (S_USAGE "contimes <n lines>\n");
		return;
		}

	newtimes = Q_atoi (Cmd_Argv (1));
	con.num_times = bound (CON_TIMES, newtimes, CON_MAX_TIMES);
	}

/*
================
Con_FixTimes

Notifies the console code about the current time
(and shifts back times of other entries when the time
went backwards)
================
*/
void Con_FixTimes (void)
	{
	double	diff;
	int	i;

	if (con.lines_count <= 0) return;

	diff = cl.time - CON_LINES_LAST ().addtime;
	if (diff >= 0.0) return; // nothing to fix

	for (i = 0; i < con.lines_count; i++)
		CON_LINES (i).addtime += diff;
	}

/*
================
Con_DeleteLine

Deletes the first line from the console history.
================
*/
void Con_DeleteLine (void)
	{
	if (con.lines_count == 0)
		return;
	con.lines_count--;
	con.lines_first = (con.lines_first + 1) % con.maxlines;
	}

/*
================
Con_DeleteLastLine

Deletes the last line from the console history.
================
*/
void Con_DeleteLastLine (void)
	{
	if (con.lines_count == 0)
		return;
	con.lines_count--;
	}

/*
================
Con_BytesLeft

Checks if there is space for a line of the given length, and if yes, returns a
pointer to the start of such a space, and NULL otherwise.
================
*/
static char *Con_BytesLeft (int length)
	{
	if (length > con.bufsize)
		return NULL;

	if (con.lines_count == 0)
		{
		return con.buffer;
		}
	else
		{
		char *firstline_start = con.lines[con.lines_first].start;
		char *lastline_onepastend = CON_LINES_LAST ().start + CON_LINES_LAST ().length;

		// the buffer is cyclic, so we first have two cases...
		if (firstline_start < lastline_onepastend) // buffer is contiguous
			{
			// put at end?
			if (length <= con.buffer + con.bufsize - lastline_onepastend)
				return lastline_onepastend;
			// put at beginning?
			else if (length <= firstline_start - con.buffer)
				return con.buffer;

			return NULL;
			}
		else
			{
			// buffer has a contiguous hole
			if (length <= firstline_start - lastline_onepastend)
				return lastline_onepastend;

			return NULL;
			}
		}
	}

/*
================
Con_AddLine

Appends a given string as a new line to the console.
================
*/
void Con_AddLine (const char *line, int length, qboolean newline)
	{
	char *putpos;
	con_lineinfo_t *p;

	if (!con.initialized || !con.buffer)
		return;

	Con_FixTimes ();
	length++;	// reserve space for term

	ASSERT (length < CON_TEXTSIZE);

	while (!(putpos = Con_BytesLeft (length)) || con.lines_count >= con.maxlines)
		Con_DeleteLine ();

	if (newline)
		{
		memcpy (putpos, line, length);
		putpos[length - 1] = '\0';
		con.lines_count++;

		p = &CON_LINES_LAST ();
		p->start = putpos;
		p->length = length;
		p->addtime = cl.time;
		}
	else
		{
		p = &CON_LINES_LAST ();
		putpos = p->start + Q_strlen (p->start);
		memcpy (putpos, line, length - 1);
		p->length = Q_strlen (p->start);
		putpos[p->length] = '\0';
		p->addtime = cl.time;
		p->length++;
		}
	}

/*
================
Con_CheckResize

If the line width has changed, reformat the buffer.
================
*/
void Con_CheckResize (void)
	{
	int	charWidth = 8;
	int	i, width;

	if (con.curFont && con.curFont->hFontTexture)
		charWidth = con.curFont->charWidths['O'] - 1;

	width = (refState.width / charWidth) - 2;
	if (!ref.initialized) width = (640 / 5);

	if (width == con.linewidth)
		return;

	Con_ClearNotify ();
	con.linewidth = width;
	con.backscroll = 0;

	con.input.widthInChars = con.linewidth;

	for (i = 0; i < CON_HISTORY; i++)
		con.history.lines[i].widthInChars = con.linewidth;
	}

/*
================
Con_PageUp
================
*/
void Con_PageUp (int lines)
	{
	con.backscroll += abs (lines);
	}

/*
================
Con_PageDown
================
*/
void Con_PageDown (int lines)
	{
	con.backscroll -= abs (lines);
	}

/*
================
Con_Top
================
*/
void Con_Top (void)
	{
	con.backscroll = CON_MAXLINES;
	}

/*
================
Con_Bottom
================
*/
void Con_Bottom (void)
	{
	con.backscroll = 0;
	}

/*
================
Con_Visible
================
*/
int GAME_EXPORT Con_Visible (void)
	{
	return (con.vislines > 0);
	}

/*
================
Con_FixedFont [FWGS, 01.04.23]
================
*/
qboolean Con_FixedFont (void)
	{
	/*if (con.curFont && con.curFont->valid && con.curFont->type == FONT_FIXED)
		return true;
	return false;*/
	return CL_FixedFont (con.curFont);
	}

/* [FWGS, 01.04.23]
static qboolean Con_LoadFixedWidthFont (const char *fontname, cl_font_t *font)
	{
	int	i, fontWidth;

	if (font->valid)
		return true; // already loaded

	if (!FS_FileExists (fontname, false))
		return false;

	// keep source to print directly into conback image
	font->hFontTexture = ref.dllFuncs.GL_LoadTexture (fontname, NULL, 0, TF_FONT | TF_KEEP_SOURCE);
	R_GetTextureParms (&fontWidth, NULL, font->hFontTexture);

	if (font->hFontTexture && fontWidth != 0)
		{
		font->charHeight = fontWidth / 16 * con_fontscale->value;
		font->type = FONT_FIXED;

		// build fixed rectangles
		for (i = 0; i < 256; i++)
			{
			font->fontRc[i].left = (i * (fontWidth / 16)) % fontWidth;
			font->fontRc[i].right = font->fontRc[i].left + fontWidth / 16;
			font->fontRc[i].top = (i / 16) * (fontWidth / 16);
			font->fontRc[i].bottom = font->fontRc[i].top + fontWidth / 16;
			font->charWidths[i] = fontWidth / 16 * con_fontscale->value;
			}
		font->valid = true;
		}

	return true;
	}

static qboolean Con_LoadVariableWidthFont (const char *fontname, cl_font_t *font)
	{
	int	i, fontWidth;
	byte *buffer;
	fs_offset_t	length;
	qfont_t *src;

	if (font->valid)
		return true; // already loaded

	if (!FS_FileExists (fontname, false))
		return false;

	font->hFontTexture = ref.dllFuncs.GL_LoadTexture (fontname, NULL, 0, TF_FONT | TF_NEAREST);
	R_GetTextureParms (&fontWidth, NULL, font->hFontTexture);

	// setup consolefont
	if (font->hFontTexture && fontWidth != 0)
		{
		// half-life font with variable chars witdh
		buffer = FS_LoadFile (fontname, &length, false);

		if (buffer && length >= sizeof (qfont_t))
			{
			src = (qfont_t *)buffer;
			font->charHeight = src->rowheight * con_fontscale->value;
			font->type = FONT_VARIABLE;

			// build rectangles
			for (i = 0; i < 256; i++)
				{
				font->fontRc[i].left = (word)src->fontinfo[i].startoffset % fontWidth;
				font->fontRc[i].right = font->fontRc[i].left + src->fontinfo[i].charwidth;
				font->fontRc[i].top = (word)src->fontinfo[i].startoffset / fontWidth;
				font->fontRc[i].bottom = font->fontRc[i].top + src->rowheight;
				font->charWidths[i] = src->fontinfo[i].charwidth * con_fontscale->value;
				}
			font->valid = true;
			}
		if (buffer) Mem_Free (buffer);
		}

	return true;
	}
*/

/*
================
Con_LoadConsoleFont [FWGS, 01.04.23]

INTERNAL RESOURCE
================
*/
static void Con_LoadConsoleFont (int fontNumber, cl_font_t *font)
	{
	/*const char *path = NULL;
	dword crc = 0;*/
	qboolean success = false;
	float scale = con_fontscale->value;

	if (font->valid)
		return;		// already loaded

	// loading conchars
	if (Sys_CheckParm ("-oldfont"))
		{
		success = Con_LoadVariableWidthFont ("gfx/conchars.fnt", font, scale,
			kRenderTransTexture, TF_FONT | TF_NEAREST);
		}
	else
		{
		string path;
		dword crc = 0;

		// replace default fonts.wad textures by current charset's font
		if (!CRC32_File (&crc, "fonts.wad") || (crc == 0x3c0a0029))
			{
			/*const char *path2 = va ("font%i_%s.fnt", fontNumber, Cvar_VariableString ("con_charset"));
			if (FS_FileExists (path2, false))
				path = path2;*/
			if (Q_snprintf (path, sizeof (path),
				"font%i_%s.fnt", fontNumber, Cvar_VariableString ("con_charset")) > 0)
				{
				success = Con_LoadVariableWidthFont (path, font, scale,
					kRenderTransTexture, TF_FONT | TF_NEAREST);
				}
			}

		/* loading conchars
		if (Sys_CheckParm ("-oldfont"))
			Con_LoadVariableWidthFont ("gfx/conchars.fnt", font);
		else*/
		if (!success)
			{
			/*if (!path)
				path = va ("fonts/font%i", fontNumber);

			Con_LoadVariableWidthFont (path, font);*/
			Q_snprintf (path, sizeof (path), "fonts/font%i", fontNumber);
			success = Con_LoadVariableWidthFont (path, font, scale, kRenderTransTexture, TF_FONT | TF_NEAREST);
			}
		}

	/* quake fixed font as fallback
	if (!font->valid)
		Con_LoadFixedWidthFont ("gfx/conchars", font);*/

	if (!success)
		{
		// keep source to print directly into conback image
		if (!Con_LoadFixedWidthFont ("gfx/conchars", font, scale, kRenderTransTexture, TF_FONT | TF_KEEP_SOURCE))
			Con_DPrintf (S_ERROR "failed to load console font\n");
		}
	}

/*
================
Con_LoadConchars
================
*/
static void Con_LoadConchars (void)
	{
	int	i, fontSize;

	// load all the console fonts
	for (i = 0; i < CON_NUMFONTS; i++)
		Con_LoadConsoleFont (i, con.chars + i);

	// select properly fontsize
	if (con_fontnum->value >= 0 && con_fontnum->value <= CON_NUMFONTS - 1)
		fontSize = con_fontnum->value;
	else if (refState.width <= 640)
		fontSize = 0;
	else if (refState.width >= 1280)
		fontSize = 2;
	else
		fontSize = 1;

	if (fontSize > CON_NUMFONTS - 1)
		fontSize = CON_NUMFONTS - 1;

	// [FWGS, 01.04.23] sets the current font
	/*con.lastUsedFont = con.curFont = &con.chars[fontSize];*/
	con.curFont = &con.chars[fontSize];
	}

// CP1251 table
int table_cp1251[64] = {
	0x0402, 0x0403, 0x201A, 0x0453, 0x201E, 0x2026, 0x2020, 0x2021,
	0x20AC, 0x2030, 0x0409, 0x2039, 0x040A, 0x040C, 0x040B, 0x040F,
	0x0452, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
	0x007F, 0x2122, 0x0459, 0x203A, 0x045A, 0x045C, 0x045B, 0x045F,
	0x00A0, 0x040E, 0x045E, 0x0408, 0x00A4, 0x0490, 0x00A6, 0x00A7,
	0x0401, 0x00A9, 0x0404, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x0407,
	0x00B0, 0x00B1, 0x0406, 0x0456, 0x0491, 0x00B5, 0x00B6, 0x00B7,
	0x0451, 0x2116, 0x0454, 0x00BB, 0x0458, 0x0405, 0x0455, 0x0457
	};

/*
============================
Con_UtfProcessChar

Convert utf char to current font's single-byte encoding
============================
*/
int Con_UtfProcessCharForce (int in)
	{
	static int m = -1, k = 0; //multibyte state
	static int uc = 0; //unicode char

	if (!in)
		{
		m = -1;
		k = 0;
		uc = 0;
		return 0;
		}

	// Get character length
	if (m == -1)
		{
		uc = 0;
		if (in >= 0xF8)
			return 0;
		else if (in >= 0xF0)
			uc = in & 0x07, m = 3;
		else if (in >= 0xE0)
			uc = in & 0x0F, m = 2;
		else if (in >= 0xC0)
			uc = in & 0x1F, m = 1;
		else if (in <= 0x7F)
			return in; //ascii
		// return 0 if we need more chars to decode one
		k = 0;
		return 0;
		}
	// get more chars
	else if (k <= m)
		{
		uc <<= 6;
		uc += in & 0x3F;
		k++;
		}
	if (in > 0xBF || m < 0)
		{
		m = -1;
		return 0;
		}
	if (k == m)
		{
		k = m = -1;
		if (g_codepage == 1251)
			{
			// cp1251 now
			if (uc >= 0x0410 && uc <= 0x042F)
				return uc - 0x410 + 0xC0;
			if (uc >= 0x0430 && uc <= 0x044F)
				return uc - 0x430 + 0xE0;
			else
				{
				int i;
				for (i = 0; i < 64; i++)
					if (table_cp1251[i] == uc)
						return i + 0x80;
				}
			}
		else if (g_codepage == 1252)
			{
			if (uc < 255)
				return uc;
			}

		// not implemented yet
		return '?';
		}
	return 0;
	}

int GAME_EXPORT Con_UtfProcessChar (int in)
	{
	if (!g_utf8)
		return in;
	else
		return Con_UtfProcessCharForce (in);
	}
/*
=================
Con_UtfMoveLeft

get position of previous printful char
=================
*/
int Con_UtfMoveLeft (char *str, int pos)
	{
	int i, k = 0;
	// int j;
	if (!g_utf8)
		return pos - 1;
	Con_UtfProcessChar (0);
	if (pos == 1) return 0;
	for (i = 0; i < pos - 1; i++)
		if (Con_UtfProcessChar ((unsigned char)str[i]))
			k = i + 1;
	Con_UtfProcessChar (0);
	return k;
	}

/*
=================
Con_UtfMoveRight

get next of previous printful char
=================
*/
int Con_UtfMoveRight (char *str, int pos, int length)
	{
	int i;
	if (!g_utf8)
		return pos + 1;
	Con_UtfProcessChar (0);
	for (i = pos; i <= length; i++)
		{
		if (Con_UtfProcessChar ((unsigned char)str[i]))
			return i + 1;
		}
	Con_UtfProcessChar (0);
	return pos + 1;
	}

static void Con_DrawCharToConback (int num, const byte *conchars, byte *dest)
	{
	int	row, col;
	const byte *source;
	int	drawline;
	int	x;

	row = num >> 4;
	col = num & 15;
	source = conchars + (row << 10) + (col << 3);

	drawline = 8;

	while (drawline--)
		{
		for (x = 0; x < 8; x++)
			if (source[x] != 255)
				dest[x] = 0x60 + source[x];
		source += 128;
		dest += 320;
		}

	}

/*
====================
Con_TextAdjustSize

draw charcters routine
====================
//
static void Con_TextAdjustSize (int *x, int *y, int *w, int *h)
	{
	float	xscale, yscale;

	if (!x && !y && !w && !h) return;

	// scale for screen sizes
	xscale = (float)refState.width / (float)clgame.scrInfo.iWidth;
	yscale = (float)refState.height / (float)clgame.scrInfo.iHeight;

	if (x) *x *= xscale;
	if (y) *y *= yscale;
	if (w) *w *= xscale;
	if (h) *h *= yscale;
	}

//
====================
Con_DrawGenericChar

draw console single character
====================
//
static int Con_DrawGenericChar (int x, int y, int number, rgba_t color)
	{
	int		width, height;
	float		s1, t1, s2, t2;
	wrect_t *rc;

	number &= 255;

	if (!con.curFont || !con.curFont->valid)
		return 0;

	number = Con_UtfProcessChar (number);
	if (!number)
		return 0;

	if (y < -con.curFont->charHeight)
		return 0;

	rc = &con.curFont->fontRc[number];
	R_GetTextureParms (&width, &height, con.curFont->hFontTexture);

	if (!width || !height)
		return con.curFont->charWidths[number];

	// don't apply color to fixed fonts it's already colored
	if (con.curFont->type != FONT_FIXED || REF_GET_PARM (PARM_TEX_GLFORMAT, con.curFont->hFontTexture) == 0x8045) // GL_LUMINANCE8_ALPHA8
		ref.dllFuncs.Color4ub (color[0], color[1], color[2], color[3]);
	else ref.dllFuncs.Color4ub (255, 255, 255, color[3]);

	// calc rectangle
	s1 = (float)rc->left / width;
	t1 = (float)rc->top / height;
	s2 = (float)rc->right / width;
	t2 = (float)rc->bottom / height;
	width = (rc->right - rc->left) * con_fontscale->value;
	height = (rc->bottom - rc->top) * con_fontscale->value;

	if (clgame.ds.adjust_size)
		Con_TextAdjustSize (&x, &y, &width, &height);
	ref.dllFuncs.R_DrawStretchPic (x, y, width, height, s1, t1, s2, t2, con.curFont->hFontTexture);
	ref.dllFuncs.Color4ub (255, 255, 255, 255); // don't forget reset color

	return con.curFont->charWidths[number];
	}

//
====================
Con_SetFont

choose font size
====================
//
void Con_SetFont (int fontNum)
	{
	fontNum = bound (0, fontNum, CON_NUMFONTS - 1);
	con.curFont = &con.chars[fontNum];
	}

//
====================
Con_RestoreFont

restore auto-selected console font
(that based on screen resolution)
====================
//
void Con_RestoreFont (void)
	{
	con.curFont = con.lastUsedFont;
	}

//
====================
Con_DrawCharacter

client version of routine
====================
//
int Con_DrawCharacter (int x, int y, int number, rgba_t color)
	{
	ref.dllFuncs.GL_SetRenderMode (kRenderTransTexture);
	return Con_DrawGenericChar (x, y, number, color);
	}
*/

/*
====================
Con_GetFont [FWGS, 01.04.23]
====================
*/
cl_font_t *Con_GetFont (int num)
	{
	num = bound (0, num, CON_NUMFONTS - 1);
	return &con.chars[num];
	}

/*
====================
Con_DrawCharacterLen

returns character sizes in screen pixels
====================
//
void Con_DrawCharacterLen (int number, int *width, int *height)
	{
	if (width && con.curFont) *width = con.curFont->charWidths[number];
	if (height && con.curFont) *height = con.curFont->charHeight;
	}
	*/

/*
====================
Con_GetCurFont [FWGS, 01.04.23]
====================
*/
cl_font_t *Con_GetCurFont (void)
	{
	return con.curFont;
	}
	
/*
====================
Con_DrawStringLen [FWGS, 01.04.23]

compute string width and height in screen pixels
====================
*/
void GAME_EXPORT Con_DrawStringLen (const char *pText, int *length, int *height)
	{
	/*int	curLength = 0;

	if (!con.curFont)
		return;
	if (height)
		*height = con.curFont->charHeight;
	if (!length)
		return;

	*length = 0;

	while (*pText)
		{
		byte	c = *pText;

		if (*pText == '\n')
			{
			pText++;
			curLength = 0;
			}

		// skip color strings they are not drawing
		if (IsColorString (pText))
			{
			pText += 2;
			continue;
			}


		// Convert to unicode
		c = Con_UtfProcessChar (c);

		if (c)
			curLength += con.curFont->charWidths[c];

		pText++;

		if (curLength > *length)
			*length = curLength;
		}
	}

//
==================
Con_DrawString

Draws a multi-colored string, optionally forcing
to a fixed color.
==================
//
int Con_DrawGenericString (int x, int y, const char *string, rgba_t setColor, qboolean forceColor, int hideChar)
	{
	rgba_t		color;
	int		drawLen = 0;
	int		numDraws = 0;
	const char *s;

	if (!con.curFont) return 0; // no font set

	Con_UtfProcessChar (0);

	// draw the colored text
	memcpy (color, setColor, sizeof (color));
	s = string;

	while (*s)
		{
		if (*s == '\n')
			{
			s++;
			if (!*s) break; // at end the string
			drawLen = 0; // begin new row
			y += con.curFont->charHeight;
			}

		if (IsColorString (s))
			{
			if (!forceColor)
				{
				memcpy (color, g_color_table[ColorIndex (*(s + 1))], sizeof (color));
				color[3] = setColor[3];
				}

			s += 2;
			numDraws++;
			continue;
			}

		// hide char for overstrike mode
		if (hideChar == numDraws)
			drawLen += con.curFont->charWidths[*s];
		else drawLen += Con_DrawCharacter (x + drawLen, y, *s, color);

		numDraws++;
		s++;
		}

	ref.dllFuncs.Color4ub (255, 255, 255, 255);
	return drawLen;*/
	return CL_DrawStringLen (con.curFont, pText, length, height, FONT_DRAW_UTF8);
	}

/*
====================
Con_DrawString [FWGS, 01.04.23]

client version of routine
====================
*/
int Con_DrawString (int x, int y, const char *string, rgba_t setColor)
	{
	/*return Con_DrawGenericString (x, y, string, setColor, false, -1);*/
	return CL_DrawString (x, y, string, setColor, con.curFont, FONT_DRAW_UTF8);
	}


/*
================
Con_Init
================
*/
void Con_Init (void)
	{
	int	i;

	if (host.type == HOST_DEDICATED)
		return; // dedicated server already have console

	// must be init before startup video subsystem
	scr_conspeed = Cvar_Get ("scr_conspeed", "600", FCVAR_ARCHIVE, "console moving speed");
	con_notifytime = Cvar_Get ("con_notifytime", "3", FCVAR_ARCHIVE, "notify time to live");
	con_fontsize = Cvar_Get ("con_fontsize", "1", FCVAR_ARCHIVE, "console font number (0, 1 or 2)");
	con_charset = Cvar_Get ("con_charset", "cp1251", FCVAR_ARCHIVE, "console font charset (only cp1251 supported now)");
	con_fontscale = Cvar_Get ("con_fontscale", "1.0", FCVAR_ARCHIVE, "scale font texture");
	con_fontnum = Cvar_Get ("con_fontnum", "-1", FCVAR_ARCHIVE, "console font number (0, 1 or 2), -1 for autoselect");
	con_color = Cvar_Get ("con_color", "240 180 24", FCVAR_ARCHIVE, "set a custom console color");

	// init the console buffer
	con.bufsize = CON_TEXTSIZE;
	con.buffer = (char *)Z_Calloc (con.bufsize);
	con.maxlines = CON_MAXLINES;
	con.lines = (con_lineinfo_t *)Z_Calloc (con.maxlines * sizeof (*con.lines));
	con.lines_first = con.lines_count = 0;
	con.num_times = CON_TIMES; // default as 4

	Con_CheckResize ();

	Con_ClearField (&con.input);
	con.input.widthInChars = con.linewidth;

	Con_ClearField (&con.chat);
	con.chat.widthInChars = con.linewidth;

	Cmd_AddCommand ("toggleconsole", Con_ToggleConsole_f, "opens or closes the console");
	Cmd_AddRestrictedCommand ("clear", Con_Clear_f, "clear console history");
	Cmd_AddCommand ("messagemode", Con_MessageMode_f, "enable message mode \"say\"");
	Cmd_AddCommand ("messagemode2", Con_MessageMode2_f, "enable message mode \"say_team\"");
	Cmd_AddCommand ("contimes", Con_SetTimes_f, "change number of console overlay lines (4-64)");
	con.initialized = true;

	Con_Printf ("Console initialized.\n");
	}

/*
================
Con_Shutdown
================
*/
void Con_Shutdown (void)
	{
	con.initialized = false;

	if (con.buffer)
		Mem_Free (con.buffer);

	if (con.lines)
		Mem_Free (con.lines);

	con.buffer = NULL;
	con.lines = NULL;
	Con_SaveHistory (&con.history);
	}

/*
================
Con_Print

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be displayed
If no console is visible, the notify window will pop up.
================
*/
void Con_Print (const char *txt)
	{
	static int	cr_pending = 0;
	static char	buf[MAX_PRINT_MSG];
	qboolean		norefresh = false;
	static int	lastlength = 0;
	static qboolean	inupdate;
	static int	bufpos = 0;
	int		c, mask = 0;

	// client not running
	if (!con.initialized || !con.buffer)
		return;

	if (txt[0] == 2)
		{
		// go to colored text
		if (Con_FixedFont ())
			mask = 128;
		txt++;
		}

	if (txt[0] == 3)
		{
		norefresh = true;
		txt++;
		}

	for (; *txt; txt++)
		{
		if (cr_pending)
			{
			Con_DeleteLastLine ();
			cr_pending = 0;
			}

		c = *txt;

		switch (c)
			{
			case '\0':
				break;
			case '\r':
				Con_AddLine (buf, bufpos, true);
				lastlength = CON_LINES_LAST ().length;
				cr_pending = 1;
				bufpos = 0;
				break;
			case '\n':
				Con_AddLine (buf, bufpos, true);
				lastlength = CON_LINES_LAST ().length;
				bufpos = 0;
				break;
			default:
				buf[bufpos++] = c | mask;
				if ((bufpos >= sizeof (buf) - 1) || bufpos >= (con.linewidth - 1))
					{
					Con_AddLine (buf, bufpos, true);
					lastlength = CON_LINES_LAST ().length;
					bufpos = 0;
					}
				break;
			}
		}

	if (norefresh) return;

	// custom renderer cause problems while updates screen on-loading
	if (SV_Active () && cls.state < ca_active && !cl.video_prepped && !cls.disable_screen)
		{
		if (bufpos != 0)
			{
			Con_AddLine (buf, bufpos, lastlength != 0);
			lastlength = 0;
			bufpos = 0;
			}

		// pump messages to avoid window hanging
		if (con.lastupdate < Sys_DoubleTime ())
			{
			con.lastupdate = Sys_DoubleTime () + 1.0;
			Host_InputFrame ();
			}

		// FIXME: disable updating screen, because when texture is bound any console print
		// can re-bound it to console font texture
#if 0
		if (!inupdate)
			{
			inupdate = true;
			SCR_UpdateScreen ();
			inupdate = false;
			}
#endif
		}
	}

/*
================
Con_NPrint

Draw a single debug line with specified height
================
*/
void GAME_EXPORT Con_NPrintf (int idx, const char *fmt, ...)
	{
	va_list	args;

	if (idx < 0 || idx >= MAX_DBG_NOTIFY)
		return;

	memset (con.notify[idx].szNotify, 0, MAX_STRING);

	va_start (args, fmt);
	Q_vsnprintf (con.notify[idx].szNotify, MAX_STRING, fmt, args);
	va_end (args);

	// reset values
	con.notify[idx].key_dest = key_game;
	con.notify[idx].expire = host.realtime + 4.0f;
	MakeRGBA (con.notify[idx].color, 255, 255, 255, 255);
	con.draw_notify = true;
	}

/*
================
Con_NXPrint

Draw a single debug line with specified height, color and time to live
================
*/
void GAME_EXPORT Con_NXPrintf (con_nprint_t *info, const char *fmt, ...)
	{
	va_list	args;

	if (!info) return;

	if (info->index < 0 || info->index >= MAX_DBG_NOTIFY)
		return;

	memset (con.notify[info->index].szNotify, 0, MAX_STRING);

	va_start (args, fmt);
	Q_vsnprintf (con.notify[info->index].szNotify, MAX_STRING, fmt, args);
	va_end (args);

	// setup values
	con.notify[info->index].key_dest = key_game;
	con.notify[info->index].expire = host.realtime + info->time_to_live;
	MakeRGBA (con.notify[info->index].color, (byte)(info->color[0] * 255), (byte)(info->color[1] * 255), 
		(byte)(info->color[2] * 255), 255);
	con.draw_notify = true;
	}

/*
================
UI_NPrint

Draw a single debug line with specified height (menu version)
================
*/
void GAME_EXPORT UI_NPrintf (int idx, const char *fmt, ...)
	{
	va_list	args;

	if (idx < 0 || idx >= MAX_DBG_NOTIFY)
		return;

	memset (con.notify[idx].szNotify, 0, MAX_STRING);

	va_start (args, fmt);
	Q_vsnprintf (con.notify[idx].szNotify, MAX_STRING, fmt, args);
	va_end (args);

	// reset values
	con.notify[idx].key_dest = key_menu;
	con.notify[idx].expire = host.realtime + 4.0f;
	MakeRGBA (con.notify[idx].color, 255, 255, 255, 255);
	con.draw_notify = true;
	}

/*
================
UI_NXPrint

Draw a single debug line with specified height, color and time to live (menu version)
================
*/
void GAME_EXPORT UI_NXPrintf (con_nprint_t *info, const char *fmt, ...)
	{
	va_list	args;

	if (!info) return;

	if ((info->index < 0) || (info->index >= MAX_DBG_NOTIFY))
		return;

	memset (con.notify[info->index].szNotify, 0, MAX_STRING);

	va_start (args, fmt);
	Q_vsnprintf (con.notify[info->index].szNotify, MAX_STRING, fmt, args);
	va_end (args);

	// setup values
	con.notify[info->index].key_dest = key_menu;
	con.notify[info->index].expire = host.realtime + info->time_to_live;
	MakeRGBA (con.notify[info->index].color, (byte)(info->color[0] * 255), (byte)(info->color[1] * 255), 
		(byte)(info->color[2] * 255), 255);
	con.draw_notify = true;
	}

/*
=============================================================================
EDIT FIELDS
=============================================================================
*/
/*
================
Con_ClearField
================
*/
void Con_ClearField (field_t *edit)
	{
	memset (edit->buffer, 0, MAX_STRING);
	edit->cursor = 0;
	edit->scroll = 0;
	}

/*
================
Field_Set
================
*/
static void Field_Set (field_t *f, const char *string)
	{
	f->scroll = 0;
	f->cursor = Q_strncpy (f->buffer, string, MAX_STRING);
	}

/*
================
Field_Paste
================
*/
void Field_Paste (field_t *edit)
	{
	char *cbd;
	int	i, pasteLen;

	cbd = Sys_GetClipboardData ();
	if (!cbd) return;

	// send as if typed, so insert / overstrike works properly
	pasteLen = Q_strlen (cbd);
	for (i = 0; i < pasteLen; i++)
		Field_CharEvent (edit, cbd[i]);
	}

/*
=================
Field_GoTo
=================
*/
static void Field_GoTo (field_t *edit, int pos)
	{
	edit->cursor = pos;
	edit->scroll = Q_max (0, edit->cursor - edit->widthInChars);
	}

/*
=================
Field_KeyDownEvent

Performs the basic line editing functions for the console,
in-game talk, and menu fields

Key events are used for non-printable characters, others are gotten from char events.
=================
*/
void Field_KeyDownEvent (field_t *edit, int key)
	{
	int	len;

	// shift-insert is paste
	if (((key == K_INS) || (key == K_KP_INS)) && Key_IsDown (K_SHIFT))
		{
		Field_Paste (edit);
		return;
		}

	len = Q_strlen (edit->buffer);

	if (key == K_DEL)
		{
		if (edit->cursor < len)
			memmove (edit->buffer + edit->cursor, edit->buffer + edit->cursor + 1, len - edit->cursor);
		return;
		}

	/*if (key == K_BACKSPACE)*/
	if ((key == K_BACKSPACE) || (key == K_X_BUTTON))	// [FWGS, 01.04.23]
		{
		if (edit->cursor > 0)
			{
			int newcursor = Con_UtfMoveLeft (edit->buffer, edit->cursor);
			memmove (edit->buffer + newcursor, edit->buffer + edit->cursor, len - edit->cursor + 1);
			edit->cursor = newcursor;
			if (edit->scroll) edit->scroll--;
			}
		return;
		}

	/*if (key == K_RIGHTARROW)*/
	if ((key == K_RIGHTARROW) || (key == K_DPAD_RIGHT))	// [FWGS, 01.04.23]
		{
		if (edit->cursor < len) edit->cursor = Con_UtfMoveRight (edit->buffer, edit->cursor, edit->widthInChars);
		if (edit->cursor >= edit->scroll + edit->widthInChars && edit->cursor <= len)
			edit->scroll++;
		return;
		}

	/*if (key == K_LEFTARROW)*/
	if ((key == K_LEFTARROW) || (key == K_DPAD_LEFT))	// [FWGS, 01.04.23]
		{
		if (edit->cursor > 0) edit->cursor = Con_UtfMoveLeft (edit->buffer, edit->cursor);
		if (edit->cursor < edit->scroll) edit->scroll--;
		return;
		}

	if ((key == K_HOME) || ((Q_tolower (key) == 'a') && Key_IsDown (K_CTRL)))
		{
		Field_GoTo (edit, 0);
		return;
		}

	if ((key == K_END) || ((Q_tolower (key) == 'e') && Key_IsDown (K_CTRL)))
		{
		Field_GoTo (edit, len);
		return;
		}

	if (key == K_INS)
		{
		host.key_overstrike = !host.key_overstrike;
		return;
		}
	}

/*
==================
Field_CharEvent
==================
*/
void Field_CharEvent (field_t *edit, int ch)
	{
	int	len;

	if (ch == 'v' - 'a' + 1)
		{
		// ctrl-v is paste
		Field_Paste (edit);
		return;
		}

	if (ch == 'c' - 'a' + 1)
		{
		// ctrl-c clears the field
		Con_ClearField (edit);
		return;
		}

	len = Q_strlen (edit->buffer);

	if (ch == 'a' - 'a' + 1)
		{
		// ctrl-a is home
		Field_GoTo (edit, 0);
		return;
		}

	if (ch == 'e' - 'a' + 1)
		{
		// ctrl-e is end
		Field_GoTo (edit, len);
		return;
		}

	// ignore any other non printable chars
	if (ch < 32) return;

	if (host.key_overstrike)
		{
		if (edit->cursor == MAX_STRING - 1) return;
		edit->buffer[edit->cursor] = ch;
		edit->cursor++;
		}
	else
		{
		// insert mode
		if (len == MAX_STRING - 1) return; // all full
		memmove (edit->buffer + edit->cursor + 1, edit->buffer + edit->cursor, len + 1 - edit->cursor);
		edit->buffer[edit->cursor] = ch;
		edit->cursor++;
		}

	if (edit->cursor >= edit->widthInChars) edit->scroll++;
	if (edit->cursor == len + 1) edit->buffer[edit->cursor] = 0;
	}

/*
==================
Field_DrawInputLine
==================
*/
void Field_DrawInputLine (int x, int y, field_t *edit)
	{
	int	len, cursorChar;
	int	drawLen;
	int	prestep, curPos;
	char	str[MAX_SYSPATH];
	byte *colorDefault;

	drawLen = edit->widthInChars;
	len = Q_strlen (edit->buffer) + 1;
	colorDefault = g_color_table[ColorIndex (COLOR_DEFAULT)];

	// guarantee that cursor will be visible
	if (len <= drawLen)
		{
		prestep = 0;
		}
	else
		{
		if (edit->scroll + drawLen > len)
			{
			edit->scroll = len - drawLen;
			if (edit->scroll < 0) edit->scroll = 0;
			}

		prestep = edit->scroll;
		}

	if (prestep + drawLen > len)
		drawLen = len - prestep;

	// extract <drawLen> characters from the field at <prestep>
	drawLen = Q_min (drawLen, MAX_SYSPATH - 1);

	memcpy (str, edit->buffer + prestep, drawLen);
	str[drawLen] = 0;

	// save char for overstrike
	cursorChar = str[edit->cursor - prestep];

	// [FWGS, 01.04.23]
	/*if (host.key_overstrike && cursorChar && !((int)(host.realtime * 4) & 1))
		hideChar = edit->cursor - prestep; // skip this char

	// draw it
	Con_DrawGenericString (x, y, str, colorDefault, false, hideChar);*/
	CL_DrawString (x, y, str, colorDefault, con.curFont, FONT_DRAW_UTF8);

	// draw the cursor
	if ((int)(host.realtime * 4) & 1) 
		return; // off blink

	// calc cursor position
	str[edit->cursor - prestep] = 0;
	/*Con_DrawStringLen (str, &curPos, NULL);
	Con_UtfProcessChar (0);*/
	CL_DrawStringLen (con.curFont, str, &curPos, NULL, FONT_DRAW_UTF8);

	// [FWGS, 01.04.23]
	if (host.key_overstrike)
		/*{
		Con_DrawGenericChar (x + curPos, y, cursorChar, colorDefault);*/
		CL_DrawCharacter (x + curPos, y, '|', colorDefault, con.curFont, 0);
	else
		/*{
		Con_UtfProcessChar (0);
		Con_DrawCharacter (x + curPos, y, '_', colorDefault);*/
		CL_DrawCharacter (x + curPos, y, '_', colorDefault, con.curFont, 0);
	}

/*
=============================================================================
CONSOLE HISTORY HANDLING
=============================================================================
*/
/*
===================
Con_HistoryUp
===================
*/
static void Con_HistoryUp (con_history_t *self, field_t *in)
	{
	if (self->line == self->next)
		self->backup = *in;

	if ((self->next - self->line) < CON_HISTORY)
		self->line = Q_max (0, self->line - 1);

	*in = self->lines[self->line % CON_HISTORY];
	}

/*
===================
Con_HistoryDown

===================
*/
static void Con_HistoryDown (con_history_t *self, field_t *in)
	{
	self->line = Q_min (self->next, self->line + 1);
	if (self->line == self->next)
		*in = self->backup;
	else
		*in = self->lines[self->line % CON_HISTORY];
	}

/*
===================
Con_HistoryAppend
===================
*/
static void Con_HistoryAppend (con_history_t *self, field_t *from)
	{
	int prevLine = Q_max (0, self->line - 1);
	const char *buf = from->buffer;

	// skip backslashes
	if (from->buffer[0] == '\\' || from->buffer[1] == '/')
		buf++;

	// only if non-empty
	if (!from->buffer[0])
		return;

	// skip empty commands
	if (Q_isspace (buf))
		return;

	// if not copy (don't ignore backslashes)
	if (!Q_strcmp (from->buffer, self->lines[prevLine % CON_HISTORY].buffer))
		return;

	self->lines[self->next % CON_HISTORY] = *from;
	self->line = ++self->next;
	}

static void Con_LoadHistory (con_history_t *self)
	{
	const byte *aFile = FS_LoadFile ("console_history.txt", NULL, true);
	const char *pLine, *pFile;
	int i, len;
	field_t *f;

	if (!aFile)
		return;

	for (pFile = pLine = (char *)aFile; *pFile; pFile++)
		{
		if (*pFile != '\n')
			continue;

		Con_ClearField (&self->lines[self->next]);

		len = Q_min (pFile - pLine + 1, sizeof (f->buffer));
		f = &self->lines[self->next % CON_HISTORY];
		f->widthInChars = con.linewidth;
		f->cursor = len - 1;
		Q_strncpy (f->buffer, pLine, len);

		self->next++;

		pLine = pFile + 1;
		}

	for (i = self->next; i < CON_HISTORY; i++)
		{
		Con_ClearField (&self->lines[i]);
		self->lines[i].widthInChars = con.linewidth;
		}

	self->line = self->next;
	}

static void Con_SaveHistory (con_history_t *self)
	{
	int historyStart = self->next - CON_HISTORY, i;
	file_t *f;

	// [FWGS, 01.05.23] do not save history if nothing was executed
	if (self->next == 0)
		return;

	if (historyStart < 0)
		historyStart = 0;

	f = FS_Open ("console_history.txt", "w", true);

	for (i = historyStart; i < self->next; i++)
		FS_Printf (f, "%s\n", self->lines[i % CON_HISTORY].buffer);

	FS_Close (f);
	}


/*
=============================================================================
CONSOLE LINE EDITING
=============================================================================
*/
/*
====================
Key_Console

Handles history and console scrollback
====================
*/
void Key_Console (int key)
	{
	// ctrl-L clears screen
	if ((key == 'l') && Key_IsDown (K_CTRL))
		{
		Cbuf_AddText ("clear\n");
		return;
		}

	// [FWGS, 01.04.23]
	/*// enter finishes the line
	if (key == K_ENTER || key == K_KP_ENTER)*/
	
	// enter or A finish the line
	if ((key == K_ENTER) || (key == K_KP_ENTER) || (key == K_A_BUTTON))
		{
		// backslash text are commands, else chat
		if ((con.input.buffer[0] == '\\') || (con.input.buffer[0] == '/'))
			Cbuf_AddText (con.input.buffer + 1);	// skip backslash
		else 
			Cbuf_AddText (con.input.buffer);		// valid command
		Cbuf_AddText ("\n");

		// echo to console
		Con_Printf (">%s\n", con.input.buffer);

		// copy line to history buffer
		Con_HistoryAppend (&con.history, &con.input);

		Con_ClearField (&con.input);
		con.input.widthInChars = con.linewidth;
		Con_Bottom ();

		if (cls.state == ca_disconnected)
			{
			// force an update, because the command may take some time
			SCR_UpdateScreen ();
			}
		return;
		}

	// [FWGS, 01.04.23] command completion
	/*if (key == K_TAB)*/
	if ((key == K_TAB) || (key == K_L2_BUTTON))
		{
		Con_CompleteCommand (&con.input);
		Con_Bottom ();
		return;
		}

	// command history (ctrl-p ctrl-n for unix style)
	if (((key == K_MWHEELUP) && Key_IsDown (K_SHIFT)) || (key == K_UPARROW) || ((Q_tolower (key) == 'p') &&
		Key_IsDown (K_CTRL)))
		{
		Con_HistoryUp (&con.history, &con.input);
		return;
		}

	if (((key == K_MWHEELDOWN) && Key_IsDown (K_SHIFT)) || (key == K_DOWNARROW) || ((Q_tolower (key) == 'n') && 
		Key_IsDown (K_CTRL)))
		{
		Con_HistoryDown (&con.history, &con.input);
		return;
		}

	// console scrolling
	/*if (key == K_PGUP)*/
	if ((key == K_PGUP) || (key == K_DPAD_UP))	// [FWGS, 01.05.23]
		{
		Con_PageUp (1);
		return;
		}

	/*if (key == K_PGDN)*/
	if ((key == K_PGDN) || (key == K_DPAD_DOWN))	// [FWGS, 01.05.23]
		{
		Con_PageDown (1);
		return;
		}

	if (key == K_MWHEELUP)
		{
		if (Key_IsDown (K_CTRL))
			Con_PageUp (8);
		else 
			Con_PageUp (2);
		return;
		}

	if (key == K_MWHEELDOWN)
		{
		if (Key_IsDown (K_CTRL))
			Con_PageDown (8);
		else
			Con_PageDown (2);
		return;
		}

	// ctrl-home = top of console
	if ((key == K_HOME) && Key_IsDown (K_CTRL))
		{
		Con_Top ();
		return;
		}

	// ctrl-end = bottom of console
	if ((key == K_END) && Key_IsDown (K_CTRL))
		{
		Con_Bottom ();
		return;
		}

	// [FWGS, 01.04.23] enable the OSK with button press
	if (key == K_Y_BUTTON)
		{
		Key_EnableTextInput (true, true);
		return;
		}

	// [FWGS, 01.04.23] exit the console by pressing MINUS on NSwitch
	// or both Back(Select)/Start buttons for everyone else
	if ((key == K_BACK_BUTTON) || (key == K_START_BUTTON))
		{
		if (cls.state == ca_active && !cl.background)
			Key_SetKeyDest (key_game);
		else
			UI_SetActiveMenu (true);
		return;
		}

	// pass to the normal editline routine
	Field_KeyDownEvent (&con.input, key);
	}

/*
================
Key_Message

In game talk message
================
*/
void Key_Message (int key)
	{
	char	buffer[MAX_SYSPATH];

	// [FWGS, 01.04.23]
	/*if (key == K_ESCAPE)*/
	if ((key == K_ESCAPE) || (key == K_BACK_BUTTON))
		{
		Key_SetKeyDest (key_game);
		Con_ClearField (&con.chat);
		return;
		}

	// [FWGS, 01.04.23]
	/*if (key == K_ENTER || key == K_KP_ENTER)*/
	if ((key == K_ENTER) || (key == K_KP_ENTER) || (key == K_A_BUTTON))
		{
		if (con.chat.buffer[0] && (cls.state == ca_active))
			{
			Q_snprintf (buffer, sizeof (buffer), "%s \"%s\"\n", con.chat_cmd, con.chat.buffer);

			if (g_messagemode_privileged)
				Cbuf_AddText (buffer);
			else 
				Cbuf_AddFilteredText (buffer);
			}

		Key_SetKeyDest (key_game);
		Con_ClearField (&con.chat);
		return;
		}

	Field_KeyDownEvent (&con.chat, key);
	}

/*
==============================================================================
DRAWING
==============================================================================
*/
/*
================
Con_DrawInput

The input line scrolls horizontally if typing goes beyond the right edge
================
*/
void Con_DrawInput (int lines)
	{
	int	y;

	// don't draw anything (always draw if not active)
	if ((cls.key_dest != key_console) || !con.curFont)
		return;

	y = lines - (con.curFont->charHeight * 2);
	
	// [FWGS, 01.04.23]
	/*Con_DrawCharacter (con.curFont->charWidths[' '], y, ']', g_color_table[7]);*/
	CL_DrawCharacter (con.curFont->charWidths[' '], y, ']', g_color_table[7], con.curFont, 0);

	Field_DrawInputLine (con.curFont->charWidths[' '] * 2, y, &con.input);
	}

/*
================
Con_DrawDebugLines [FWGS, 01.04.23]

Custom debug messages
================
*/
int Con_DrawDebugLines (void)
	{
	notify_t *notify = con.notify;
	int	i, count = 0;
	int	defaultX;
	int	y = 20;
	int	fontTall;

	if (!con.curFont || !con.curFont->valid)
		return 0;

	defaultX = refState.width / 4;
	fontTall = con.curFont->charHeight + 1;

	/*for (i = 0; i < MAX_DBG_NOTIFY; i++)
		{
		if ((host.realtime < con.notify[i].expire) && (con.notify[i].key_dest == cls.key_dest))*/
	for (i = 0; i < ARRAYSIZE (con.notify); i++, notify++)
		{
		int	x, len;
		/*int	fontTall = 0;
		Con_DrawStringLen (con.notify[i].szNotify, &len, &fontTall);*/

		if (host.realtime > notify->expire)
			continue;

		if (notify->key_dest != cls.key_dest)
			continue;

		Con_DrawStringLen (notify->szNotify, &len, NULL);
		x = refState.width - Q_max (defaultX, len) - 10;
		//fontTall += 1;

		if (y + fontTall > refState.height - 20)
			return count;

		count++;
		/*y = 20 + fontTall * i;
		Con_DrawString (x, y, con.notify[i].szNotify, con.notify[i].color);
		}*/
		y += fontTall;
		CL_DrawString (x, y, notify->szNotify, notify->color, con.curFont, FONT_DRAW_UTF8 | FONT_DRAW_NOLF);
		}

	return count;
	}

/*
================
Con_DrawDebug

Draws the debug messages (not passed to console history)
================
*/
void Con_DrawDebug (void)
	{
	static double	timeStart;
	string		dlstring;
	int		x, y;

	if (scr_download->value != -1.0f)
		{
		Q_snprintf (dlstring, sizeof (dlstring), "Downloading [%d remaining]: ^2%s^7 %5.1f%% time %.f secs",
			host.downloadcount, host.downloadfile, scr_download->value, Sys_DoubleTime () - timeStart);
		x = refState.width - 500;
		y = con.curFont->charHeight * 1.05f;
		Con_DrawString (x, y, dlstring, g_color_table[7]);
		}
	else
		{
		/*timeStart = Sys_DoubleTime ();*/
		timeStart = host.realtime;	// [FWGS, 01.04.23]
		}

	if (!host.allow_console || Cvar_VariableInteger ("cl_background") || Cvar_VariableInteger ("sv_background"))
		return;

	if (con.draw_notify && !Con_Visible ())
		{
		if (Con_DrawDebugLines () == 0)
			con.draw_notify = false;
		}
	}

/*
================
Con_DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
void Con_DrawNotify (void)
	{
	double	time = cl.time;
	int	i, x, y = 0;

	if (!con.curFont) return;

	x = con.curFont->charWidths[' ']; // offset one space at left screen side

	if (host.allow_console && (!Cvar_VariableInteger ("cl_background") && !Cvar_VariableInteger ("sv_background")))
		{
		for (i = CON_LINES_COUNT - con.num_times; i < CON_LINES_COUNT; i++)
			{
			con_lineinfo_t *l = &CON_LINES (i);

			if (l->addtime < (time - con_notifytime->value))
				continue;

			Con_DrawString (x, y, l->start, g_color_table[7]);
			y += con.curFont->charHeight;
			}
		}

	if (cls.key_dest == key_message)
		{
		string	buf;
		int	len;

		// update chatline position from client.dll
		if (clgame.dllFuncs.pfnChatInputPosition)
			clgame.dllFuncs.pfnChatInputPosition (&x, &y);

		Q_snprintf (buf, sizeof (buf), "%s: ", con.chat_cmd);

		Con_DrawStringLen (buf, &len, NULL);
		Con_DrawString (x, y, buf, g_color_table[7]);

		Field_DrawInputLine (x + len, y, &con.chat);
		}

	ref.dllFuncs.Color4ub (255, 255, 255, 255);
	}

/*
================
Con_DrawConsoleLine

Draws a line of the console; returns its height in lines.
If alpha is 0, the line is not drawn, but still wrapped and its height
returned.
================
*/
int Con_DrawConsoleLine (int y, int lineno)
	{
	con_lineinfo_t *li = &CON_LINES (lineno);

	if (!li || !li->start || (*li->start == '\1'))
		return 0;	// this string will be shown only at notify

	// [FWGS, 01.04.23]
	if (y >= con.curFont->charHeight)
		{
		float x = con.curFont->charWidths[' '];
		CL_DrawString (x, y, li->start, g_color_table[7], con.curFont, FONT_DRAW_UTF8);
		}
	/*Con_DrawGenericString (con.curFont->charWidths[' '], y, li->start, g_color_table[7], false, -1);*/

	return con.curFont->charHeight;
	}

/*
================
Con_LastVisibleLine

Calculates the last visible line index and how much to show
of it based on con.backscroll.
================
*/
static void Con_LastVisibleLine (int *lastline)
	{
	int	i, lines_seen = 0;

	con.backscroll = Q_max (0, con.backscroll);
	*lastline = 0;

	// now count until we saw con_backscroll actual lines
	for (i = CON_LINES_COUNT - 1; i >= 0; i--)
		{
		// line is the last visible line?
		*lastline = i;

		if (lines_seen + 1 > con.backscroll && lines_seen <= con.backscroll)
			return;

		lines_seen += 1;
		}

	// if we get here, no line was on screen - scroll so that one line is visible then.
	con.backscroll = lines_seen - 1;
	}

/*
================
Con_DrawConsole

Draws the console with the solid background
================
*/
void Con_DrawSolidConsole (int lines)
	{
	int	i, x, y;
	float	fraction;
	int	start;
	int	stringLen, width = 0, charH;
	string	curbuild;
	byte	color[4];

	if (lines <= 0) return;

	// draw the background
	ref.dllFuncs.GL_SetRenderMode (kRenderNormal);
	ref.dllFuncs.Color4ub (255, 255, 255, 255); // to prevent grab color from screenfade
	if (refState.width * 3 / 4 < refState.height && lines >= refState.height)
		ref.dllFuncs.R_DrawStretchPic (0, lines - refState.height, refState.width,
			refState.height - refState.width * 3 / 4, 0, 0, 1, 1,
			R_GetBuiltinTexture (REF_BLACK_TEXTURE));
	ref.dllFuncs.R_DrawStretchPic (0, lines - refState.width * 3 / 4, refState.width,
		refState.width * 3 / 4, 0, 0, 1, 1, con.background);

	if (!con.curFont || !host.allow_console)
		return; // nothing to draw

	// draw current version
	memcpy (color, g_color_table[7], sizeof (color));

	// [FWGS, 01.04.23]
	/*Q_snprintf (curbuild, MAX_STRING, "%s %i/%s (%s-%s build %i)", XASH_ENGINE_NAME, PROTOCOL_VERSION,
		XASH_VERSION, Q_buildos (), Q_buildarch (), Q_buildnum ());*/
	Q_snprintf (curbuild, MAX_STRING, XASH_ENGINE_NAME " %i/" XASH_VERSION " (%s-%s build %i)", 
		PROTOCOL_VERSION, Q_buildos (), Q_buildarch (), Q_buildnum ());

	Con_DrawStringLen (curbuild, &stringLen, &charH);
	
	/*start = refState.width - stringLen;
	stringLen = Con_StringLength (curbuild);*/
	start = refState.width - stringLen;

	fraction = lines / (float)refState.height;
	color[3] = Q_min (fraction * 2.0f, 1.0f) * 255; // fadeout version number

	/*for (i = 0; i < stringLen; i++)
		width += Con_DrawCharacter (start + width, 0, curbuild[i], color);*/
	Con_DrawString (start, 0, curbuild, color);

	// draw the text
	if (CON_LINES_COUNT > 0)
		{
		int	ymax = lines - (con.curFont->charHeight * 2.0f);
		int	lastline;

		Con_LastVisibleLine (&lastline);
		y = ymax - con.curFont->charHeight;

		if (con.backscroll)
			{
			start = con.curFont->charWidths[' ']; // offset one space at left screen side

			// [FWGS, 01.04.23] draw red arrows to show the buffer is backscrolled
			for (x = 0; x < con.linewidth; x += 4)
				CL_DrawCharacter ((x + 1) * start, y, '^', g_color_table[1], con.curFont, 0);
			/*Con_DrawCharacter ((x + 1) * start, y, '^', g_color_table[1]);*/
			
			y -= con.curFont->charHeight;
			}
		x = lastline;

		while (1)
			{
			y -= Con_DrawConsoleLine (y, x);

			// top of console buffer or console window
			if ((x == 0) || (y < con.curFont->charHeight))
				break;
			x--;
			}
		}

	// draw the input prompt, user text, and cursor if desired
	Con_DrawInput (lines);

	y = lines - (con.curFont->charHeight * 1.2f);
	SCR_DrawFPS (Q_max (y, 4)); // to avoid to hide fps counter

	ref.dllFuncs.Color4ub (255, 255, 255, 255);
	}

/*
==================
Con_DrawConsole
==================
*/
void Con_DrawConsole (void)
	{
	// never draw console when changelevel in-progress
	if ((cls.state != ca_disconnected) && (cls.changelevel || cls.changedemo))
		return;

	// check for console width changes from a vid mode change
	Con_CheckResize ();

	if ((cls.state == ca_connecting) || (cls.state == ca_connected))
		{
		if (!cl_allow_levelshots->value)
			{
			if ((Cvar_VariableInteger ("cl_background") || Cvar_VariableInteger ("sv_background")) && 
				(cls.key_dest != key_console))
				con.vislines = con.showlines = 0;
			else 
				con.vislines = con.showlines = refState.height;
			}
		else
			{
			con.showlines = 0;

			if (host_developer.value >= DEV_EXTENDED)
				Con_DrawNotify (); // draw notify lines
			}
		}

	// if disconnected, render console full screen
	switch (cls.state)
		{
		case ca_disconnected:
			if (cls.key_dest != key_menu)
				{
				Con_DrawSolidConsole (refState.height);
				Key_SetKeyDest (key_console);
				}
			break;
		case ca_connecting:
		case ca_connected:
		case ca_validate:
			// force to show console always for -dev 3 and higher
			Con_DrawSolidConsole (con.vislines);
			break;
		case ca_active:
		case ca_cinematic:
			if (Cvar_VariableInteger ("cl_background") || Cvar_VariableInteger ("sv_background"))
				{
				if (cls.key_dest == key_console)
					Con_DrawSolidConsole (refState.height);
				}
			else
				{
				if (con.vislines)
					Con_DrawSolidConsole (con.vislines);
				else if (cls.state == ca_active && (cls.key_dest == key_game || cls.key_dest == key_message))
					Con_DrawNotify (); // draw notify lines
				}
			break;
		}

	if (!Con_Visible ())
		SCR_DrawFPS (4);
	}

/*
==================
Con_DrawVersion [FWGS, 01.04.23]

Used by menu
==================
*/
void Con_DrawVersion (void)
	{
	// draws the current build
	byte *color = g_color_table[7];
	
	/*int	i, stringLen, width = 0, charH = 0;*/
	int	stringLen, charH = 0;
	
	int	start, height = refState.height;
	qboolean	draw_version = false;
	string	curbuild;

	switch (cls.scrshot_action)
		{
		case scrshot_normal:
		case scrshot_snapshot:
			draw_version = true;
			break;
		}

	if (!host.force_draw_version)
		{
		if ((cls.key_dest != key_menu && !draw_version) || (CL_IsDevOverviewMode () == 2) || net_graph->value)
			return;
		}

	if (host.force_draw_version_time > host.realtime)
		host.force_draw_version = false;

	// [FWGS, 01.04.23]
	if (host.force_draw_version || draw_version)
		Q_snprintf (curbuild, MAX_STRING, XASH_ENGINE_NAME " v%i/" XASH_VERSION " (%s-%s build %i)",
			PROTOCOL_VERSION, Q_buildos (), Q_buildarch (), Q_buildnum ());
	else 
		Q_snprintf (curbuild, MAX_STRING, "v%i/" XASH_VERSION " (%s-%s build %i)", PROTOCOL_VERSION,
			Q_buildos (), Q_buildarch (), Q_buildnum ());

	/*Q_snprintf (curbuild, MAX_STRING, "%s v%i/%s (%s-%s build %i)", XASH_ENGINE_NAME, PROTOCOL_VERSION, XASH_VERSION, Q_buildos (), Q_buildarch (), Q_buildnum ());
	else Q_snprintf (curbuild, MAX_STRING, "v%i/%s (%s-%s build %i)", PROTOCOL_VERSION, XASH_VERSION, Q_buildos (), Q_buildarch (), Q_buildnum ());*/

	Con_DrawStringLen (curbuild, &stringLen, &charH);
	start = refState.width - stringLen * 1.05f;
	/*stringLen = Con_StringLength (curbuild);*/	// [FWGS, 01.05.23]
	height -= charH * 1.05f;

	/*for (i = 0; i < stringLen; i++)
		width += Con_DrawCharacter (start + width, height, curbuild[i], color);*/
	Con_DrawString (start, height, curbuild, color);
	}

/*
==================
Con_RunConsole

Scroll it up or down
==================
*/
void Con_RunConsole (void)
	{
	float	lines_per_frame;

	Con_SetColor ();

	// decide on the destination height of the console
	if (host.allow_console && (cls.key_dest == key_console))
		{
		if ((cls.state < ca_active) || cl.first_frame)
			con.showlines = refState.height;	// full screen
		else 
			con.showlines = (refState.height >> 1);	// half screen
		}
	else
		{
		con.showlines = 0; // none visible
		}

	lines_per_frame = fabs (scr_conspeed->value) * host.realframetime;

	if (con.showlines < con.vislines)
		{
		con.vislines -= lines_per_frame;
		if (con.showlines > con.vislines)
			con.vislines = con.showlines;
		}
	else if (con.showlines > con.vislines)
		{
		con.vislines += lines_per_frame;
		if (con.showlines < con.vislines)
			con.vislines = con.showlines;
		}

	if (FBitSet (con_charset->flags, FCVAR_CHANGED) ||
		FBitSet (con_fontscale->flags, FCVAR_CHANGED) ||
		FBitSet (con_fontnum->flags, FCVAR_CHANGED) ||
		FBitSet (cl_charset->flags, FCVAR_CHANGED))
		{
		// update codepage parameters
		if (!Q_stricmp (con_charset->string, "cp1251"))
			{
			g_codepage = 1251;
			}
		else if (!Q_stricmp (con_charset->string, "cp1252"))
			{
			g_codepage = 1252;
			}
		else
			{
			Con_Printf (S_WARN "Unknown charset %s, defaulting to cp1252", con_charset->string);

			Cvar_DirectSet (con_charset, "cp1252");
			g_codepage = 1252;
			}

		g_utf8 = !Q_stricmp (cl_charset->string, "utf-8");
		Con_InvalidateFonts ();
		Con_LoadConchars ();
		
		// [FWGS, 01.04.23]
		/*cls.creditsFont.valid = false;
		SCR_LoadCreditsFont ();*/

		ClearBits (con_charset->flags, FCVAR_CHANGED);
		ClearBits (con_fontnum->flags, FCVAR_CHANGED);
		ClearBits (con_fontscale->flags, FCVAR_CHANGED);
		ClearBits (cl_charset->flags, FCVAR_CHANGED);
		}
	}

/*
==============================================================================

CONSOLE INTERFACE

==============================================================================
*/
/*
================
Con_CharEvent

Console input
================
*/
void Con_CharEvent (int key)
	{
	// distribute the key down event to the apropriate handler
	if (cls.key_dest == key_console)
		{
		Field_CharEvent (&con.input, key);
		}
	else if (cls.key_dest == key_message)
		{
		Field_CharEvent (&con.chat, key);
		}
	}

/*
=========
Con_VidInit

INTERNAL RESOURCE
=========
*/
void Con_VidInit (void)
	{
	if (!con.historyLoaded)
		{
		Con_LoadHistory (&con.history);
		con.historyLoaded = true;
		}

	Con_LoadConchars ();
	Con_CheckResize ();
#if XASH_LOW_MEMORY
	con.background = R_GetBuiltinTexture (REF_BLACK_TEXTURE);
#else
	// loading console image
	// ESHQ: �� ���������
	/*if (host.allow_console)
		{
		// trying to load truecolor image first
		if (FS_FileExists ("gfx/shell/conback.bmp", false) || FS_FileExists ("gfx/shell/conback.tga", false))
			con.background = ref.dllFuncs.GL_LoadTexture ("gfx/shell/conback", NULL, 0, TF_IMAGE);

		if (!con.background)
			{
			if (FS_FileExists ("cached/conback640", false))
				con.background = ref.dllFuncs.GL_LoadTexture ("cached/conback640", NULL, 0, TF_IMAGE);
			else if (FS_FileExists ("cached/conback", false))
				con.background = ref.dllFuncs.GL_LoadTexture ("cached/conback", NULL, 0, TF_IMAGE);
			}
		}
	else
		{
		// trying to load truecolor image first
		if (FS_FileExists ("gfx/shell/loading.bmp", false) || FS_FileExists ("gfx/shell/loading.tga", false))
			con.background = ref.dllFuncs.GL_LoadTexture ("gfx/shell/loading", NULL, 0, TF_IMAGE);

		if (!con.background)
			{
			if (FS_FileExists ("cached/loading640", false))
				con.background = ref.dllFuncs.GL_LoadTexture ("cached/loading640", NULL, 0, TF_IMAGE);
			else if (FS_FileExists ("cached/loading", false))
				con.background = ref.dllFuncs.GL_LoadTexture ("cached/loading", NULL, 0, TF_IMAGE);
			}
		}

	if (!con.background) // last chance - quake conback image
		{
		qboolean		draw_to_console = false;
		fs_offset_t		length = 0;
		const byte *buf;

		// NOTE: only these games want to draw build number into console background
		if (!Q_stricmp (FS_Gamedir (), "id1"))
			draw_to_console = true;

		if (!Q_stricmp (FS_Gamedir (), "hipnotic"))
			draw_to_console = true;

		if (!Q_stricmp (FS_Gamedir (), "rogue"))
			draw_to_console = true;

		if (draw_to_console && con.curFont &&
			(buf = ref.dllFuncs.R_GetTextureOriginalBuffer (con.curFont->hFontTexture)) != NULL)
			{
			lmp_t *cb = (lmp_t *)FS_LoadFile ("gfx/conback.lmp", &length, false);
			char	ver[64];
			byte *dest;
			int	x, y, len;

			if (cb && cb->width == 320 && cb->height == 200)
				{
				len = Q_snprintf (ver, 64, "%i", Q_buildnum ()); // can store only buildnum
				dest = (byte *)(cb + 1) + 320 * 186 + 320 - 11 - 8 * len;
				y = len;
				for (x = 0; x < y; x++)
					Con_DrawCharToConback (ver[x], buf, dest + (x << 3));
				con.background = ref.dllFuncs.GL_LoadTexture ("#gfx/conback.lmp", (byte *)cb, length, TF_IMAGE);
				}
			if (cb) Mem_Free (cb);
			}

		if (!con.background) // trying the load unmodified conback
			con.background = ref.dllFuncs.GL_LoadTexture ("gfx/conback.lmp", NULL, 0, TF_IMAGE);
		}*/

	// missed console image will be replaced as gray background like X-Ray or Crysis
	if ((con.background == R_GetBuiltinTexture (REF_DEFAULT_TEXTURE)) || (con.background == 0))
		// ESHQ: ���� ���� �������
		con.background = R_GetBuiltinTexture (REF_BLACK_TEXTURE);
#endif
	}

/*
=========
Con_InvalidateFonts [FWGS, 01.04.23]
=========
*/
void Con_InvalidateFonts (void)
	{
	/*memset (con.chars, 0, sizeof (con.chars));
	con.curFont = con.lastUsedFont = NULL;*/
	int i;
	for (i = 0; i < ARRAYSIZE (con.chars); i++)
		CL_FreeFont (&con.chars[i]);
	con.curFont = NULL;
	}

/*
=========
Con_FastClose

immediately close the console
=========
*/
void Con_FastClose (void)
	{
	Con_ClearField (&con.input);
	Con_ClearNotify ();
	con.showlines = 0;
	con.vislines = 0;
	}

/*
=========
Con_DefaultColor

called from MainUI
=========
*/
void GAME_EXPORT Con_DefaultColor (int r, int g, int b)
	{
	r = bound (0, r, 255);
	g = bound (0, g, 255);
	b = bound (0, b, 255);
	MakeRGBA (g_color_table[7], r, g, b, 255);
	}

#if XASH_ENGINE_TESTS
#include "tests.h"

static void Test_RunConHistory (void)
	{
	con_history_t hist = { 0 };
	field_t input = { 0 };
	const char *strs1[] = { "map t0a0", "quit", "wtf", "wtf", "", "nyan" };
	const char *strs2[] = { "nyan", "wtf", "quit", "map t0a0" };
	const char *testbackup = "unfinished_edit";
	int i;

	for (i = 0; i < ARRAYSIZE (strs1); i++)
		{
		Field_Set (&input, strs1[i]);
		Con_HistoryAppend (&hist, &input);
		}

	Field_Set (&input, testbackup);

	for (i = 0; i < ARRAYSIZE (strs2); i++)
		{
		Con_HistoryUp (&hist, &input);
		TASSERT_STR (input.buffer, strs2[i]);
		}

	// check for overrun
	Con_HistoryUp (&hist, &input);

	for (i = ARRAYSIZE (strs2) - 1; i >= 0; i--)
		{
		TASSERT_STR (input.buffer, strs2[i]);
		Con_HistoryDown (&hist, &input);
		}

	TASSERT_STR (input.buffer, testbackup);
	}

void Test_RunCon (void)
	{
	TRUN (Test_RunConHistory ());
	}

#endif /* XASH_ENGINE_TESTS */