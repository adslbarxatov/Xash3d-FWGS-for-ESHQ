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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
***/

#include "extdll.h"
#include "basemenu.h"
#include "utils.h"
#include "menu_btnsbmp_table.h"
#include "menu_strings.h"

#define ART_BANNER		"gfx/shell/head_vidmodes"

#define ID_BACKGROUND	0
#define ID_BANNER		1
#define ID_APPLY		2
#define ID_DONE			3
#define ID_VIDMODELIST	4
#define ID_FULLSCREEN	5
#define ID_VERTICALSYNC	6
#define ID_TABLEHINT	7

#define MAX_VIDMODES	65

typedef struct
	{
	const char *videoModesPtr[MAX_VIDMODES];

	menuFramework_s	menu;

	menuBitmap_s	background;
	menuBitmap_s	banner;
	menuPicButton_s	ok;
	menuPicButton_s	cancel;
	menuCheckBox_s	windowed;
	menuCheckBox_s	vsync;

	menuScrollList_s	vidList;
	menuAction_s	listCaption;
	} uiVidModes_t;

static uiVidModes_t	uiVidModes;

/***
=================
UI_VidModes_GetModesList
=================
***/
static void UI_VidModes_GetConfig (void)
	{
	int i;
	for (i = 0; i < MAX_VIDMODES - 1; i++)
		{
		uiVidModes.videoModesPtr[i] = VID_GET_MODE (i);
		if (!uiVidModes.videoModesPtr[i])
			break; // end of list
		}

	uiVidModes.videoModesPtr[i] = NULL;	// terminator

	uiVidModes.vidList.itemNames = uiVidModes.videoModesPtr;
	uiVidModes.vidList.curItem = CVAR_GET_FLOAT ("vid_mode");

	if (!CVAR_GET_FLOAT ("fullscreen"))
		uiVidModes.windowed.enabled = 1;

	if (CVAR_GET_FLOAT ("gl_vsync"))
		uiVidModes.vsync.enabled = 1;
	}

/***
=================
UI_VidModes_SetConfig
=================
***/
static void UI_VidOptions_SetConfig (void)
	{
	CVAR_SET_FLOAT ("vid_mode", uiVidModes.vidList.curItem);
	CVAR_SET_FLOAT ("fullscreen", !uiVidModes.windowed.enabled);
	CVAR_SET_FLOAT ("gl_vsync", uiVidModes.vsync.enabled);
	}

/***
=================
UI_VidModes_UpdateConfig
=================
***/
static void UI_VidOptions_UpdateConfig (void)
	{
	CVAR_SET_FLOAT ("gl_vsync", uiVidModes.vsync.enabled);
	}

/***
=================
UI_VidModes_Callback
=================
***/
static void UI_VidModes_Callback (void *self, int event)
	{
	menuCommon_s *item = (menuCommon_s *)self;

	switch (item->id)
		{
		case ID_FULLSCREEN:
		case ID_VERTICALSYNC:
			if (event == QM_PRESSED)
				((menuCheckBox_s *)self)->focusPic = UI_CHECKBOX_PRESSED;
			else 
				((menuCheckBox_s *)self)->focusPic = UI_CHECKBOX_FOCUS;
			break;
		}

	if (event == QM_CHANGED)
		{
		UI_VidOptions_UpdateConfig ();
		return;
		}

	if (event != QM_ACTIVATED)
		return;

	switch (item->id)
		{
		case ID_DONE:
			UI_PopMenu ();
			break;
		case ID_APPLY:
			UI_VidOptions_SetConfig ();
			UI_PopMenu ();
			break;
		}
	}

/***
=================
UI_VidModes_Init
=================
***/
static void UI_VidModes_Init (void)
	{
	memset (&uiVidModes, 0, sizeof (uiVidModes_t));

	uiVidModes.menu.vidInitFunc = UI_VidModes_Init;

	uiVidModes.background.generic.id = ID_BACKGROUND;
	uiVidModes.background.generic.type = QMTYPE_BITMAP;
	uiVidModes.background.generic.flags = QMF_INACTIVE;
	uiVidModes.background.generic.x = 0;
	uiVidModes.background.generic.y = 0;
	uiVidModes.background.generic.width = 1024;
	uiVidModes.background.generic.height = 768;
	uiVidModes.background.pic = ART_BACKGROUND;

	uiVidModes.banner.generic.id = ID_BANNER;
	uiVidModes.banner.generic.type = QMTYPE_BITMAP;
	uiVidModes.banner.generic.flags = QMF_INACTIVE | QMF_DRAW_ADDITIVE;
	uiVidModes.banner.generic.x = UI_BANNER_POSX;
	uiVidModes.banner.generic.y = UI_BANNER_POSY;
	uiVidModes.banner.generic.width = UI_BANNER_WIDTH;
	uiVidModes.banner.generic.height = UI_BANNER_HEIGHT;
	uiVidModes.banner.pic = ART_BANNER;

	uiVidModes.ok.generic.id = ID_APPLY;
	uiVidModes.ok.generic.type = QMTYPE_BM_BUTTON;
	uiVidModes.ok.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_DROPSHADOW;
	uiVidModes.ok.generic.x = 72;
	uiVidModes.ok.generic.y = 230;
	uiVidModes.ok.generic.name = "Apply";
	uiVidModes.ok.generic.callback = UI_VidModes_Callback;
	UI_UtilSetupPicButton (&uiVidModes.ok, PC_OK);
#ifdef RU
	uiVidModes.ok.generic.statusText = "��������� ���������";
#else
	uiVidModes.ok.generic.statusText = "Apply changes";
#endif

	uiVidModes.cancel.generic.id = ID_DONE;
	uiVidModes.cancel.generic.type = QMTYPE_BM_BUTTON;
	uiVidModes.cancel.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_DROPSHADOW;
	uiVidModes.cancel.generic.x = 72;
	uiVidModes.cancel.generic.y = 280;
	uiVidModes.cancel.generic.name = "Done";
	uiVidModes.cancel.generic.callback = UI_VidModes_Callback;
	UI_UtilSetupPicButton (&uiVidModes.cancel, PC_CANCEL);
#ifdef RU
	uiVidModes.cancel.generic.statusText = "��������� � ���������� ����";
#else
	uiVidModes.cancel.generic.statusText = "Return back to previous menu";
#endif

	uiVidModes.listCaption.generic.id = ID_TABLEHINT;
	uiVidModes.listCaption.generic.type = QMTYPE_BM_BUTTON;
	uiVidModes.listCaption.generic.flags = QMF_INACTIVE | QMF_SMALLFONT;
	uiVidModes.listCaption.generic.color = uiColorHelp;
	uiVidModes.listCaption.generic.name = MenuStrings[HINT_DISPLAYMODE];
	uiVidModes.listCaption.generic.x = 400;
	uiVidModes.listCaption.generic.y = 270;

	uiVidModes.vidList.generic.id = ID_VIDMODELIST;
	uiVidModes.vidList.generic.type = QMTYPE_SCROLLLIST;
	uiVidModes.vidList.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_SMALLFONT;
	uiVidModes.vidList.generic.x = 400;
	uiVidModes.vidList.generic.y = 300;
	uiVidModes.vidList.generic.width = 560;
	uiVidModes.vidList.generic.height = 300;
	uiVidModes.vidList.generic.callback = UI_VidModes_Callback;

	uiVidModes.windowed.generic.id = ID_FULLSCREEN;
	uiVidModes.windowed.generic.type = QMTYPE_CHECKBOX;
	uiVidModes.windowed.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_ACT_ONRELEASE | QMF_MOUSEONLY | QMF_DROPSHADOW;
	uiVidModes.windowed.generic.x = 400;
	uiVidModes.windowed.generic.y = 620;
	uiVidModes.windowed.generic.callback = UI_VidModes_Callback;
#ifdef RU
	uiVidModes.windowed.generic.name = "��������� � ����";
	uiVidModes.windowed.generic.statusText = "��������� ���� � ������� ������";
#else
	uiVidModes.windowed.generic.name = "Run in a window";
	uiVidModes.windowed.generic.statusText = "Run game in window mode";
#endif

	uiVidModes.vsync.generic.id = ID_VERTICALSYNC;
	uiVidModes.vsync.generic.type = QMTYPE_CHECKBOX;
	uiVidModes.vsync.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_ACT_ONRELEASE | QMF_MOUSEONLY | QMF_DROPSHADOW;
	uiVidModes.vsync.generic.x = 400;
	uiVidModes.vsync.generic.y = 670;
	uiVidModes.vsync.generic.callback = UI_VidModes_Callback;
#ifdef RU
	uiVidModes.vsync.generic.name = "����. �������������";
	uiVidModes.vsync.generic.statusText = "������������� ������������ �������������";
#else
	uiVidModes.vsync.generic.name = "Vertical sync";
	uiVidModes.vsync.generic.statusText = "Enable the vertical synchronization";
#endif

	UI_VidModes_GetConfig ();

	UI_AddItem (&uiVidModes.menu, (void *)&uiVidModes.background);
	UI_AddItem (&uiVidModes.menu, (void *)&uiVidModes.banner);
	UI_AddItem (&uiVidModes.menu, (void *)&uiVidModes.ok);
	UI_AddItem (&uiVidModes.menu, (void *)&uiVidModes.cancel);
	UI_AddItem (&uiVidModes.menu, (void *)&uiVidModes.windowed);
	UI_AddItem (&uiVidModes.menu, (void *)&uiVidModes.vsync);
	UI_AddItem (&uiVidModes.menu, (void *)&uiVidModes.listCaption);
	UI_AddItem (&uiVidModes.menu, (void *)&uiVidModes.vidList);
	}

/***
=================
UI_VidModes_Precache
=================
***/
void UI_VidModes_Precache (void)
	{
	PIC_Load (ART_BACKGROUND);
	PIC_Load (ART_BANNER);
	}

/***
=================
UI_VidModes_Menu
=================
***/
void UI_VidModes_Menu (void)
	{
	UI_VidModes_Precache ();
	UI_VidModes_Init ();

	UI_PushMenu (&uiVidModes.menu);
	}
