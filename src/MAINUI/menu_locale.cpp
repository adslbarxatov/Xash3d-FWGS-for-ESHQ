/***
menu_locale.c - обработчик локализации меню
***/

#include "extdll.h"
#include "utils.h"
#include "menu_locale.h"

#define LOCALE_MAIN		"menu.loc"

char *MenuLocStrings[LOC_STRINGSCOUNT];
static byte *menuLocBuffer;

/***
=================
UI_LoadMenuLocale
=================
***/
void UI_LoadMenuLocale (void)
	{
	// Получение файла
	int		loc_filesize;
	menuLocBuffer = LOAD_FILE (LOCALE_MAIN, &loc_filesize);

	if (!menuLocBuffer || !loc_filesize)
		{
		Con_Printf ("%s: %s not found\n", __func__, LOCALE_MAIN);
		return;
		}

	// Разбор строк
	int pos = 0;
	union MenuLocStringHeader header;
	char sz = sizeof (union MenuLocStringHeader);

	for (int i = 0; i < LOC_STRINGSCOUNT; i++)
		{
		// Защита от чтения вне диапазона
		if (pos + sz >= loc_filesize)
			break;

		memcpy (header.Ptr, menuLocBuffer + pos, sz);
		int len = ((header.Str.Length[0] - '0') * 100 + (header.Str.Length[1] - '0') * 10 +
			header.Str.Length[2] - '0');

		pos += sz;

		// Защита от выхода за диапазон и повреждения файла
		if ((pos + len >= loc_filesize) || (menuLocBuffer[pos + len] != '\n'))
			break;

		MenuLocStrings[i] = (char *)(menuLocBuffer + pos);
		menuLocBuffer[pos + len] = '\0';
		pos += (len + 1);	// \n
		}
	}

/***
=================
UI_UnloadMenuLocale
=================
***/
void UI_UnloadMenuLocale (void)
	{
	FREE_FILE (menuLocBuffer);
	}
