/***
titles.c - titles.txt file parser
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
#include "common.h"
#include "client.h"
#include "tests.h"

// [FWGS, 05.04.26]
/*define MAX_MESSAGES	2048*/
client_textmessage_t	gMessageParms;

/*define MSGFILE_NAME	0
define MSGFILE_TEXT	1

client_textmessage_t	gMessageParms;

// the string "pText" is assumed to have all whitespace from both ends cut out
static int IsComment (const char *pText)*/

// [FWGS, 05.04.26] removed IsComment, IsStartOfText, IsEndOfText, IsWhiteSpace,
// SkipSpace, SkipText

// [FWGS, 05.04.26]
static qboolean TitleParseDirective (const char *line)
	{
	/*if (pText)*/
	char	directive[64];
	char	*p = COM_ParseFile ((char *)line, directive, sizeof (directive));

	if (!p || (directive[0] != '$'))
		return false;

	int nargs;
	if (!Q_stricmp (directive, "$position"))
		{
		nargs = 2;
		}
	else if (!Q_stricmp (directive, "$effect") || !Q_stricmp (directive, "$fadein") ||
		!Q_stricmp (directive, "$fadeout") || !Q_stricmp (directive, "$holdtime") ||
		!Q_stricmp (directive, "$fxtime"))
		{
		nargs = 1;
		}
	else if (!Q_stricmp (directive, "$color") || !Q_stricmp (directive, "$color2"))
		{
		nargs = 3;
		}
	else
		{
		/*int length = Q_strlen (pText);

		if ((length >= 2) && (pText[0] == '/') && (pText[1] == '/'))
			return 1;

		// no text?
		if (length > 0)
			return 0;*/
		Con_DPrintf (S_ERROR "unknown directive: %s\n", directive);
		return true;
		}

	/*// no text is a comment too
	return 1;
	}

// the string "pText" is assumed to have all whitespace from both ends cut out
static int IsStartOfText (const char *pText)
	{
	if (pText)*/
	float args[3] = { 0 };
	for (int i = 0; i < nargs; i++)
		{
		/*if (pText[0] == '{')
			return 1;*/
		char numstr[32];
		p = COM_ParseFile (p, numstr, sizeof (numstr));

		if (!p)
			return true;	// not enough arguments

		args[i] = Q_atof (numstr);
		}

	/*return 0;
	}

// the string "pText" is assumed to have all whitespace from both ends cut out
static int IsEndOfText (const char *pText)
	{
	if (pText)*/
	if (!Q_stricmp (directive, "$position"))
		{
		/*if (pText[0] == '}')
			return 1;*/
		gMessageParms.x = args[0];
		gMessageParms.y = args[1];
		}
	/*return 0;
	}

static int IsWhiteSpace (char space)
	{
	if ((space == ' ') || (space == '\t') || (space == '\r') || (space == '\n'))
		return 1;
	return 0;
	}

static const char *SkipSpace (const char *pText)
	{
	if (pText)*/
	else if (!Q_stricmp (directive, "$color"))
		{
		/*int pos = 0;
		while (pText[pos] && IsWhiteSpace (pText[pos]))
			pos++;
		return pText + pos;*/
		gMessageParms.r1 = args[0];
		gMessageParms.g1 = args[1];
		gMessageParms.b1 = args[2];
		}
	/*return NULL;
	}

static const char *SkipText (const char *pText)
	{
	if (pText)*/
	else if (!Q_stricmp (directive, "$color2"))
		{
		/*int pos = 0;
		while (pText[pos] && !IsWhiteSpace (pText[pos]))
			pos++;
		return pText + pos;*/
		gMessageParms.r2 = args[0];
		gMessageParms.g2 = args[1];
		gMessageParms.b2 = args[2];
		}
	/*return NULL;*/
	else if (!Q_stricmp (directive, "$effect"))
		{
		gMessageParms.effect = args[0];
		}
	else if (!Q_stricmp (directive, "$fadein"))
		{
		gMessageParms.fadein = args[0];
		}
	else if (!Q_stricmp (directive, "$fadeout"))
		{
		gMessageParms.fadeout = args[0];
		}
	else if (!Q_stricmp (directive, "$holdtime"))
		{
		gMessageParms.holdtime = args[0];
		}
	else if (!Q_stricmp (directive, "$fxtime"))
		{
		gMessageParms.fxtime = args[0];
		}

	return true;
	}

// [FWGS, 05.04.26] removed ParseFloats
/*static int ParseFloats (const char *pText, float *pFloat, int count)*/

// [FWGS, 05.04.26] fast first pass: count the number of complete name { text } blocks
static int TitleCountMessages (char *pfile, int fileSize)
	{
	/*const char *pTemp = pText;
	int index = 0;*/
	char	line[512];
	int		filepos = 0, count = 0;
	qboolean	in_text = false;

	/*while (pTemp && (count > 0))*/
	while (Q_memfgets (pfile, fileSize, &filepos, line, sizeof (line)) != NULL)
		{
		/*// skip current token / float
		pTemp = SkipText (pTemp);
		// skip any whitespace in between
		pTemp = SkipSpace (pTemp);*/
		char trim[512];
		COM_TrimSpace (trim, line, sizeof (trim));

		/*if (pTemp)*/
		if (!in_text)
			{
			if (trim[0] == '{')
				in_text = true;
			}
		else if (trim[0] == '}')
			{
			/*// parse a float
			pFloat[index] = Q_atof (pTemp);
			count--;
			index++;*/
			count++;
			in_text = false;
			}
		}

	/*if (count == 0)
		return 1;
	return 0;*/
	return count;
	}

// [FWGS, 05.04.26] removed IsToken, ParseDirective

// [FWGS, 05.04.26]
/*static int IsToken (const char *pText, const char *pTokenName)*/
client_textmessage_t *CL_TextMessageParse (poolhandle_t mempool, char *pfile, int fileSize, int *numTitles)
	{
	/*if (!pText || !pTokenName)
		return 0;*/
	int total = TitleCountMessages (pfile, fileSize);
	if (!total)
		{
		*numTitles = 0;
		return NULL;
		}

	/*if (!Q_strnicmp (pText + 1, pTokenName, Q_strlen (pTokenName)))
		return 1;*/
	size_t	bufsize = total * sizeof (client_textmessage_t);
	client_textmessage_t *out = Mem_Calloc (mempool, bufsize);

	/*return 0;
	}*/
	char	line[512], curname[512];
	int		filepos = 0, linenum = 0, count = 0, textsegstart = 0;
	qboolean	in_text = false;

	/*static int ParseDirective (const char *pText)
	{
	if (pText && (pText[0] == '$'))*/
	while (1)
		{
		/*float	tempFloat[8];*/
		int curlinestart = filepos;

		/*if (IsToken (pText, "position"))
			{
			if (ParseFloats (pText, tempFloat, 2))
				{
				gMessageParms.x = tempFloat[0];
				gMessageParms.y = tempFloat[1];
				}
			}
		else if (IsToken (pText, "effect"))
			{
			if (ParseFloats (pText, tempFloat, 1))
				{
				gMessageParms.effect = (int)tempFloat[0];
				}
			}
		else if (IsToken (pText, "fxtime"))
			{
			if (ParseFloats (pText, tempFloat, 1))
				{
				gMessageParms.fxtime = tempFloat[0];
				}
			}
		else if (IsToken (pText, "color2"))
			{
			if (ParseFloats (pText, tempFloat, 3))
				{
				gMessageParms.r2 = (int)tempFloat[0];
				gMessageParms.g2 = (int)tempFloat[1];
				gMessageParms.b2 = (int)tempFloat[2];
				}
			}
		else if (IsToken (pText, "color"))
			{
			if (ParseFloats (pText, tempFloat, 3))
				{
				gMessageParms.r1 = (int)tempFloat[0];
				gMessageParms.g1 = (int)tempFloat[1];
				gMessageParms.b1 = (int)tempFloat[2];
				}
			}
		else if (IsToken (pText, "fadein"))
			{
			if (ParseFloats (pText, tempFloat, 1))
				{
				gMessageParms.fadein = tempFloat[0];
				}
			}
		else if (IsToken (pText, "fadeout"))
			{
			if (ParseFloats (pText, tempFloat, 3))
				{
				gMessageParms.fadeout = tempFloat[0];
				}
			}
		else if (IsToken (pText, "holdtime"))
			{
			if (ParseFloats (pText, tempFloat, 3))
				{
				gMessageParms.holdtime = tempFloat[0];
				}
			}
		else
			{
			Con_DPrintf (S_ERROR "unknown token: %s\n", pText);
			}
		return 1;
		}
	return 0;
	}*/
		if (Q_memfgets (pfile, fileSize, &filepos, line, sizeof (line)) == NULL)
			break;

		/*// [FWGS, 01.03.26]
client_textmessage_t *CL_TextMessageParse (poolhandle_t mempool, byte *pMemFile, int fileSize, int *numTitles)
	{
	char	buf[512], trim[512], currentName[512];
	char	*pCurrentText = NULL, *pNameHeap;
	char	nameHeap[32768];		// g-cont. i will scale up heap to handle all TFC messages
	int		mode = MSGFILE_NAME;	// searching for a message name
	int		lineNumber, filePos, lastLinePos;
	client_textmessage_t	textMessages[MAX_MESSAGES];
	int		i, nameHeapSize, textHeapSize, messageSize, nameOffset;
	int		messageCount, lastNamePos;
	size_t	textHeapSizeRemaining;

	lastNamePos = 0;
	lineNumber = 0;
	filePos = 0;
	lastLinePos = 0;
	messageCount = 0;

	while (Q_memfgets (pMemFile, fileSize, &filePos, buf, 512) != NULL)
		{
		COM_TrimSpace (trim, buf, sizeof (trim));*/
		linenum++;

		char trim[512];
		COM_TrimSpace (trim, line, sizeof (trim));

		/*switch (mode)*/
		if (!in_text)
			{
			/*case MSGFILE_NAME:
				// skip comment lines
				if (IsComment (trim))
					break;*/
			if (COM_StringEmpty (trim) || ((trim[0] == '/') && (trim[1] == '/')))
				continue;

			/*// Is this a directive "$command"?, if so parse it and break
				if (ParseDirective (trim))
					break;*/
			if (TitleParseDirective (trim))
				continue;

			/*if (IsStartOfText (trim))*/
			if (trim[0] == '{')
				{
				/*mode = MSGFILE_TEXT;
					pCurrentText = (char *)(pMemFile + filePos);
					break;*/
				in_text = true;
				textsegstart = filepos;
				continue;
				}

			/*if (IsEndOfText (trim))*/
			if (trim[0] == '}')
				{
				/*Con_Reportf ("%s: unexpected '}' found, line %d\n", __func__, lineNumber);*/
				Con_Reportf ("%s: unexpected '}' at line %d\n", __func__, linenum);
				Mem_Free (out);
				return NULL;
				}

			/*Q_strncpy (currentName, trim, sizeof (currentName));
				break;

			case MSGFILE_TEXT:
				if (IsEndOfText (trim))
					{
					int length = Q_strlen (currentName);

					// save name on name heap
					if (lastNamePos + length > 32768)
						{
						Con_Reportf ("%s: error while parsing!\n", __func__);
						return NULL;
						}

					Q_strncpy (nameHeap + lastNamePos, currentName, sizeof (nameHeap) - lastNamePos);

					// terminate text in-place in the memory file
					// (it's temporary memory that will be deleted)
					pMemFile[lastLinePos - 1] = 0;

					// Save name/text on heap
					textMessages[messageCount] = gMessageParms;
					textMessages[messageCount].pName = nameHeap + lastNamePos;
					lastNamePos += length + 1;
					textMessages[messageCount].pMessage = pCurrentText;
					messageCount++;

					// reset parser to search for names
					mode = MSGFILE_NAME;
					break;
					}
				if (IsStartOfText (trim))*/
			Q_strncpy (curname, trim, sizeof (curname));
			}
		else
			{
			if (trim[0] == '{')
				{
				/*Con_Reportf ("%s: unexpected '{' found, line %d\n", __func__, lineNumber);*/
				Con_Reportf ("%s: unexpected '{' at line %d\n", __func__, linenum);
				Mem_Free (out);
				return NULL;
				}
			/*break;
			}

		lineNumber++;
		lastLinePos = filePos;*/

			/*if (messageCount >= MAX_MESSAGES)
		{
		Con_Printf (S_WARN "Too many messages in titles.txt, max is %d\n", MAX_MESSAGES);
		break;*/
			if (trim[0] == '}')
				{
				size_t textlen = Q_max (curlinestart - 1 - textsegstart, 0);
				size_t namelen = Q_strlen (curname);
				size_t nameoffset = bufsize;
				size_t textoffset = nameoffset + namelen + 1;

				bufsize = textoffset + textlen + 1;
				out = Mem_Realloc (mempool, out, bufsize);

				char *p = (char *)out;

				memcpy (&p[nameoffset], curname, namelen + 1);
				memcpy (&p[textoffset], &pfile[textsegstart], textlen);
				p[textoffset + textlen] = '\0';

				// store offsets as pointers temporarily
				out[count] = gMessageParms;
				out[count].pName = (char *)nameoffset;
				out[count].pMessage = (char *)textoffset;
				count++;
				in_text = false;
				}
			}
		}

	/*Con_Reportf ("%s: parsed %d text messages\n", __func__, messageCount);
nameHeapSize = lastNamePos;
textHeapSize = 0;

for (i = 0; i < messageCount; i++)
	textHeapSize += Q_strlen (textMessages[i].pMessage) + 1;
messageSize = (messageCount * sizeof (client_textmessage_t));

if ((textHeapSize + nameHeapSize + messageSize) <= 0)
	{
	*numTitles = 0;
	return NULL;
	}

// must malloc because we need to be able to clear it after initialization
client_textmessage_t *out = Mem_Calloc (mempool, textHeapSize + nameHeapSize + messageSize);

// copy table over
memcpy (out, textMessages, messageSize);*/
	if (unlikely (count != total))
		Con_DPrintf (S_ERROR "%s: expected %d messages, parsed %d\n", __func__, total, count);

	/*// copy Name heap
pNameHeap = ((char *)out) + messageSize;
memcpy (pNameHeap, nameHeap, nameHeapSize);

// copy text & fixup pointers
textHeapSizeRemaining = textHeapSize;
pCurrentText = pNameHeap + nameHeapSize;

for (i = 0; i < messageCount; i++)*/
	// fix up offsets
	for (int i = 0; i < count; i++)
		{
		/*size_t currentTextSize = Q_strlen (out[i].pMessage) + 1;

	out[i].pName = pNameHeap;	// adjust name pointer (parallel buffer)
	Q_strncpy (pCurrentText, out[i].pMessage, textHeapSizeRemaining);	// copy text over
	out[i].pMessage = pCurrentText;

	pNameHeap += Q_strlen (pNameHeap) + 1;
	pCurrentText += currentTextSize;
	textHeapSizeRemaining -= currentTextSize;*/
		out[i].pName = (char *)out + (uintptr_t)out[i].pName;
		out[i].pMessage = (char *)out + (uintptr_t)out[i].pMessage;
		}

	/*if ((pCurrentText - (char *)out) != (textHeapSize + nameHeapSize + messageSize))
		Con_DPrintf (S_ERROR "%s: overflow text message buffer!\n", __func__);

	clgame.numTitles = messageCount;
	*numTitles = messageCount;*/
	Con_Reportf ("%s: parsed %d text messages\n", __func__, count);
	*numTitles = count;

	return out;
	}

// [FWGS, 01.03.26]
#if XASH_ENGINE_TESTS

void Test_RunTitles (void)
	{
	poolhandle_t mempool = Mem_AllocPool (__func__);
	client_textmessage_t *tmessages;
	client_textmessage_t *null;
	int num_titles = 0, num_null_titles = 0;
	char titles[] =
		"// this is a comment\n"
		"$effect 2\n"
		"$color 1 2 3\n"
		"$color2 4 5 6\n"
		"$position 7 8\n"
		"$fadein 0.1\n"
		"$fadeout 0.5\n"
		"$holdtime 321\n"
		"$fxtime 123\n"
		"TITLE\n"
		"{\n"
		"Hello to anybody reading test data\n"
		"Hope you have a good time\n"
		"}\n"
		"\n"
		"TITLE2\n"
		"{\n"
		"Still reading that nonsense huh?\n"
		"}\n"
		"// let's override some things\n"
		"$cats are\n"
		"$cute\n"
		"$color 10 10 10\n"
		"$position -1 -1\n"
		"UwU\n"
		"{\n"
		"OwO\n"
		"}\n"
		"Technically titles can have spaces and // can't have comments\n"
		"{\n"
		"Oh yeah!\n"
		"}\n";
	char broken_titles[] = "}\n";
	char broken_titles2[] = "{\n{\n";

	null = CL_TextMessageParse (mempool, broken_titles, Q_strlen (broken_titles), &num_null_titles);
	TASSERT_EQi (num_null_titles, 0);
	TASSERT_EQp (null, NULL);

	null = CL_TextMessageParse (mempool, broken_titles2, Q_strlen (broken_titles2), &num_null_titles);
	TASSERT_EQi (num_null_titles, 0);
	TASSERT_EQp (null, NULL);

	tmessages = CL_TextMessageParse (mempool, titles, Q_strlen (titles), &num_titles);
	TASSERT_EQi (num_titles, 4);
	TASSERT_NEQp (tmessages, NULL);

	for (int i = 0; i < 4; i++)
		{
		TASSERT_EQi (tmessages[i].effect, 2);
		TASSERT_EQi (tmessages[i].r1, i >= 2 ? 10 : 1);
		TASSERT_EQi (tmessages[i].g1, i >= 2 ? 10 : 2);
		TASSERT_EQi (tmessages[i].b1, i >= 2 ? 10 : 3);
		TASSERT_EQi (tmessages[i].a1, 0);
		TASSERT_EQi (tmessages[i].r2, 4);
		TASSERT_EQi (tmessages[i].g2, 5);
		TASSERT_EQi (tmessages[i].b2, 6);
		TASSERT_EQi (tmessages[i].a2, 0);
		TASSERT_EQi (tmessages[i].x, i >= 2 ? -1 : 7);
		TASSERT_EQi (tmessages[i].y, i >= 2 ? -1 : 8);
		TASSERT_EQi (tmessages[i].fadein, 0.1f);
		TASSERT_EQi (tmessages[i].fadeout, 0.5f);
		TASSERT_EQi (tmessages[i].holdtime, 321.f);
		TASSERT_EQi (tmessages[i].fxtime, 123.f);
		}

	TASSERT_STR (tmessages[0].pName, "TITLE");
	TASSERT_STR (tmessages[1].pName, "TITLE2");
	TASSERT_STR (tmessages[2].pName, "UwU");
	TASSERT_STR (tmessages[3].pName, "Technically titles can have spaces and // can't have comments");

	TASSERT_STR (tmessages[0].pMessage, "Hello to anybody reading test data\nHope you have a good time");
	TASSERT_STR (tmessages[1].pMessage, "Still reading that nonsense huh?");
	TASSERT_STR (tmessages[2].pMessage, "OwO");
	TASSERT_STR (tmessages[3].pMessage, "Oh yeah!");

	Mem_Free (tmessages);
	Mem_FreePool (&mempool);
	}

#endif
