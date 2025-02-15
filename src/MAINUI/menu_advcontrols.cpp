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
#include "../cl_dll/kbutton.h"
#include "menu_btnsbmp_table.h"
#include "menu_strings.h"

#define ART_BANNER			"gfx/shell/head_advanced"

#define ID_BACKGROUND		0
#define ID_BANNER			1

#define ID_DONE				2
#define ID_SENSITIVITY		3
#define ID_CROSSHAIR		4
#define ID_INVERTMOUSE		5
#define ID_MOUSELOOK		6
#define ID_LOOKSPRING		7
#define ID_LOOKSTRAFE		8
#define ID_INFINITERUN		9	// ESHQ: ������ ���������
#define ID_AUTOAIM			10

typedef struct
	{
	menuFramework_s	menu;

	menuBitmap_s	background;
	menuBitmap_s	banner;

	menuPicButton_s	done;

	menuCheckBox_s	crosshair;
	menuCheckBox_s	invertMouse;
	menuCheckBox_s	mouseLook;
	menuCheckBox_s	lookSpring;
	menuCheckBox_s	lookStrafe;
	menuCheckBox_s	infiniteRun;	// ESHQ: ����� �������� - ����������� ���
	menuCheckBox_s	autoaim;
	menuSlider_s	sensitivity;
	} uiAdvControls_t;

static uiAdvControls_t	uiAdvControls;

/***
=================
UI_AdvControls_UpdateConfig
=================
***/
static void UI_AdvControls_UpdateConfig (void)
	{
	if (uiAdvControls.invertMouse.enabled && CVAR_GET_FLOAT ("m_pitch") > 0)
		CVAR_SET_FLOAT ("m_pitch", -CVAR_GET_FLOAT ("m_pitch"));
	else if (!uiAdvControls.invertMouse.enabled && CVAR_GET_FLOAT ("m_pitch") < 0)
		CVAR_SET_FLOAT ("m_pitch", fabs (CVAR_GET_FLOAT ("m_pitch")));

	CVAR_SET_FLOAT ("crosshair", uiAdvControls.crosshair.enabled);
	CVAR_SET_FLOAT ("lookspring", uiAdvControls.lookSpring.enabled);
	CVAR_SET_FLOAT ("lookstrafe", uiAdvControls.lookStrafe.enabled);
	
	// ESHQ: ������ ���������
	if (uiAdvControls.infiniteRun.enabled)
		{
		CVAR_SET_FLOAT ("cl_movespeedkey", 0.95f);
		CVAR_SET_FLOAT ("cl_anglespeedkey", 0.95f);
		}
	else 
		{
		CVAR_SET_FLOAT ("cl_movespeedkey", 0.55f);
		CVAR_SET_FLOAT ("cl_anglespeedkey", 0.55f);
		}
	
	CVAR_SET_FLOAT ("sv_aim", uiAdvControls.autoaim.enabled);
	CVAR_SET_FLOAT ("sensitivity", (uiAdvControls.sensitivity.curValue * 20.0f) + 0.2f);

	if (uiAdvControls.mouseLook.enabled)
		{
		uiAdvControls.lookSpring.generic.flags |= QMF_GRAYED;
		uiAdvControls.lookStrafe.generic.flags |= QMF_GRAYED;
		CLIENT_COMMAND (TRUE, "+mlook");
		}
	else
		{
		uiAdvControls.lookSpring.generic.flags &= ~QMF_GRAYED;
		uiAdvControls.lookStrafe.generic.flags &= ~QMF_GRAYED;
		CLIENT_COMMAND (TRUE, "-mlook");
		}
	}

/***
=================
UI_AdvControls_GetConfig
=================
***/
static void UI_AdvControls_GetConfig (void)
	{
	kbutton_t *mlook;

	if (CVAR_GET_FLOAT ("m_pitch") < 0)
		uiAdvControls.invertMouse.enabled = true;

	if (CVAR_GET_FLOAT ("crosshair"))
		uiAdvControls.crosshair.enabled = 1;

	mlook = (kbutton_s *)Key_GetState ("in_mlook");
	if (mlook && mlook->state & 1)
		uiAdvControls.mouseLook.enabled = 1;

	if (CVAR_GET_FLOAT ("lookspring"))
		uiAdvControls.lookSpring.enabled = 1;

	if (CVAR_GET_FLOAT ("lookstrafe"))
		uiAdvControls.lookStrafe.enabled = 1;

	// ESHQ: ������ ���������
	if (CVAR_GET_FLOAT ("cl_movespeedkey") > 0.7f)
		uiAdvControls.infiniteRun.enabled = 1;
	else
		uiAdvControls.infiniteRun.enabled = 0;

	if (CVAR_GET_FLOAT ("sv_aim"))
		uiAdvControls.autoaim.enabled = 1;

	uiAdvControls.sensitivity.curValue = (CVAR_GET_FLOAT ("sensitivity") - 0.2f) / 20.0f;

	if (uiAdvControls.mouseLook.enabled)
		{
		uiAdvControls.lookSpring.generic.flags |= QMF_GRAYED;
		uiAdvControls.lookStrafe.generic.flags |= QMF_GRAYED;
		}
	else
		{
		uiAdvControls.lookSpring.generic.flags &= ~QMF_GRAYED;
		uiAdvControls.lookStrafe.generic.flags &= ~QMF_GRAYED;
		}
	}

/***
=================
UI_AdvControls_Callback
=================
***/
static void UI_AdvControls_Callback (void *self, int event)
	{
	menuCommon_s *item = (menuCommon_s *)self;

	switch (item->id)
		{
		case ID_CROSSHAIR:
		case ID_INVERTMOUSE:
		case ID_MOUSELOOK:
		case ID_LOOKSPRING:
		case ID_LOOKSTRAFE:
		case ID_INFINITERUN:	// ESHQ: ������ ���������
		case ID_AUTOAIM:
			if (event == QM_PRESSED)
				((menuCheckBox_s *)self)->focusPic = UI_CHECKBOX_PRESSED;
			else 
				((menuCheckBox_s *)self)->focusPic = UI_CHECKBOX_FOCUS;
			break;
		}

	if (event == QM_CHANGED)
		{
		UI_AdvControls_UpdateConfig ();
		return;
		}

	if (event != QM_ACTIVATED)
		return;

	switch (item->id)
		{
		case ID_DONE:
			UI_PopMenu ();
			break;
		}
	}

/***
=================
UI_AdvControls_Init
=================
***/
static void UI_AdvControls_Init (void)
	{
	memset (&uiAdvControls, 0, sizeof (uiAdvControls_t));

	uiAdvControls.menu.vidInitFunc = UI_AdvControls_Init;

	uiAdvControls.background.generic.id = ID_BACKGROUND;
	uiAdvControls.background.generic.type = QMTYPE_BITMAP;
	uiAdvControls.background.generic.flags = QMF_INACTIVE;
	uiAdvControls.background.generic.x = 0;
	uiAdvControls.background.generic.y = 0;
	uiAdvControls.background.generic.width = 1024;
	uiAdvControls.background.generic.height = 768;
	uiAdvControls.background.pic = ART_BACKGROUND;

	uiAdvControls.banner.generic.id = ID_BANNER;
	uiAdvControls.banner.generic.type = QMTYPE_BITMAP;
	uiAdvControls.banner.generic.flags = QMF_INACTIVE | QMF_DRAW_ADDITIVE;
	uiAdvControls.banner.generic.x = UI_BANNER_POSX;
	uiAdvControls.banner.generic.y = UI_BANNER_POSY;
	uiAdvControls.banner.generic.width = UI_BANNER_WIDTH;
	uiAdvControls.banner.generic.height = UI_BANNER_HEIGHT;
	uiAdvControls.banner.pic = ART_BANNER;

	uiAdvControls.done.generic.id = ID_DONE;
	uiAdvControls.done.generic.type = QMTYPE_BM_BUTTON;
	uiAdvControls.done.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_DROPSHADOW;
	uiAdvControls.done.generic.x = 72;
	uiAdvControls.done.generic.y = 680;
	uiAdvControls.done.generic.name = "Done";
	uiAdvControls.done.generic.callback = UI_AdvControls_Callback;
#ifdef RU
	uiAdvControls.done.generic.statusText = "��������� ��������� � ��������� � ���� ���������";
#else
	uiAdvControls.done.generic.statusText = "Save changes and go back to the customizing menu";
#endif

	UI_UtilSetupPicButton (&uiAdvControls.done, PC_DONE);

	uiAdvControls.crosshair.generic.id = ID_CROSSHAIR;
	uiAdvControls.crosshair.generic.type = QMTYPE_CHECKBOX;
	uiAdvControls.crosshair.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_NOTIFY | QMF_ACT_ONRELEASE | 
		QMF_MOUSEONLY | QMF_DROPSHADOW;
	uiAdvControls.crosshair.generic.x = 72;
	uiAdvControls.crosshair.generic.y = 230;
	uiAdvControls.crosshair.generic.callback = UI_AdvControls_Callback;
#ifdef RU
	uiAdvControls.crosshair.generic.name = "�����������";
	uiAdvControls.crosshair.generic.statusText = "��������� ����� ������� ������";
#else
	uiAdvControls.crosshair.generic.name = "Crosshair";
	uiAdvControls.crosshair.generic.statusText = "Enable the weapon aiming crosshair";
#endif

	uiAdvControls.invertMouse.generic.id = ID_INVERTMOUSE;
	uiAdvControls.invertMouse.generic.type = QMTYPE_CHECKBOX;
	uiAdvControls.invertMouse.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_NOTIFY | QMF_ACT_ONRELEASE | QMF_MOUSEONLY | QMF_DROPSHADOW;
	uiAdvControls.invertMouse.generic.x = 72;
	uiAdvControls.invertMouse.generic.y = 280;
	uiAdvControls.invertMouse.generic.name = MenuStrings[HINT_REVERSE_MOUSE];
	uiAdvControls.invertMouse.generic.callback = UI_AdvControls_Callback;
#ifdef RU
	uiAdvControls.invertMouse.generic.statusText = "����������� ������������ ��� ����";
#else
	uiAdvControls.invertMouse.generic.statusText = "Reverse vertical mouse axis";
#endif

	uiAdvControls.mouseLook.generic.id = ID_MOUSELOOK;
	uiAdvControls.mouseLook.generic.type = QMTYPE_CHECKBOX;
	uiAdvControls.mouseLook.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_NOTIFY | QMF_ACT_ONRELEASE | QMF_MOUSEONLY | QMF_DROPSHADOW;
	uiAdvControls.mouseLook.generic.x = 72;
	uiAdvControls.mouseLook.generic.y = 330;
	uiAdvControls.mouseLook.generic.callback = UI_AdvControls_Callback;
#ifdef RU
	uiAdvControls.mouseLook.generic.name = "����� �����";
	uiAdvControls.mouseLook.generic.statusText = "������������ ���� ��� ������ ������ �����������";
#else
	uiAdvControls.mouseLook.generic.name = "Mouse look";
	uiAdvControls.mouseLook.generic.statusText = "Use the mouse to look around instead of using the mouse to move";
#endif

	uiAdvControls.lookSpring.generic.id = ID_LOOKSPRING;
	uiAdvControls.lookSpring.generic.type = QMTYPE_CHECKBOX;
	uiAdvControls.lookSpring.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_NOTIFY | QMF_ACT_ONRELEASE | QMF_MOUSEONLY | QMF_DROPSHADOW;
	uiAdvControls.lookSpring.generic.x = 72;
	uiAdvControls.lookSpring.generic.y = 380;
	uiAdvControls.lookSpring.generic.callback = UI_AdvControls_Callback;
#ifdef RU
	uiAdvControls.lookSpring.generic.name = "���������� �����";
	uiAdvControls.lookSpring.generic.statusText = "���������� ����� '���������' �����, ����� ��������\n�����, ����� �� ���������� �����";
#else
	uiAdvControls.lookSpring.generic.name = "Look spring";
	uiAdvControls.lookSpring.generic.statusText = "Causes the screen to 'spring' back to looking straight\nahead when you move forward";
#endif

	uiAdvControls.lookStrafe.generic.id = ID_LOOKSTRAFE;
	uiAdvControls.lookStrafe.generic.type = QMTYPE_CHECKBOX;
	uiAdvControls.lookStrafe.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_NOTIFY | QMF_ACT_ONRELEASE | QMF_MOUSEONLY | QMF_DROPSHADOW;
	uiAdvControls.lookStrafe.generic.x = 72;
	uiAdvControls.lookStrafe.generic.y = 430;
	uiAdvControls.lookStrafe.generic.callback = UI_AdvControls_Callback;
#ifdef RU
	uiAdvControls.lookStrafe.generic.name = "����� ������";
	uiAdvControls.lookStrafe.generic.statusText = "� ��������� � ������������� ������ ���� ����������\n��������� �����-������ ������ ��������";
#else
	uiAdvControls.lookStrafe.generic.name = "Look strafe";
	uiAdvControls.lookStrafe.generic.statusText = "In combination with your mouse look modifier, causes left-right\nmovements to strafe instead of turn";
#endif

	// ESHQ: ������ ��������� �� ����������� ���
	uiAdvControls.infiniteRun.generic.id = ID_INFINITERUN;
	uiAdvControls.infiniteRun.generic.type = QMTYPE_CHECKBOX;
	uiAdvControls.infiniteRun.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_NOTIFY | QMF_ACT_ONRELEASE |
		QMF_MOUSEONLY | QMF_DROPSHADOW;
	uiAdvControls.infiniteRun.generic.x = 72;
	uiAdvControls.infiniteRun.generic.y = 480;
	uiAdvControls.infiniteRun.generic.callback = UI_AdvControls_Callback;
#ifdef RU
	uiAdvControls.infiniteRun.generic.name = "���������� ���";
	uiAdvControls.infiniteRun.generic.statusText = "���������� ��� ������, �� ��������� �������\n��������� �������";
#else
	uiAdvControls.infiniteRun.generic.name = "Permanent run";
	uiAdvControls.infiniteRun.generic.statusText = "Constantly running player without special keys";
#endif

	uiAdvControls.autoaim.generic.id = ID_AUTOAIM;
	uiAdvControls.autoaim.generic.type = QMTYPE_CHECKBOX;
	uiAdvControls.autoaim.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_NOTIFY | QMF_ACT_ONRELEASE | QMF_MOUSEONLY | QMF_DROPSHADOW;
	uiAdvControls.autoaim.generic.x = 72;
	uiAdvControls.autoaim.generic.y = 530;
	uiAdvControls.autoaim.generic.callback = UI_AdvControls_Callback;
#ifdef RU
	uiAdvControls.autoaim.generic.name = "����������";
	uiAdvControls.autoaim.generic.statusText = "����� ���� ������� ��� �������� �� ������";
#else
	uiAdvControls.autoaim.generic.name = "Autoaim";
	uiAdvControls.autoaim.generic.statusText = "Let game to help you aim at enemies";
#endif

	uiAdvControls.sensitivity.generic.id = ID_SENSITIVITY;
	uiAdvControls.sensitivity.generic.type = QMTYPE_SLIDER;
	uiAdvControls.sensitivity.generic.flags = QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiAdvControls.sensitivity.generic.name = MenuStrings[HINT_MOUSE_SENSE];
	uiAdvControls.sensitivity.generic.x = 72;
	uiAdvControls.sensitivity.generic.y = 625;
	uiAdvControls.sensitivity.minValue = 0.0;
	uiAdvControls.sensitivity.maxValue = 1.0;
	uiAdvControls.sensitivity.range = 0.01f;
	uiAdvControls.sensitivity.generic.callback = UI_AdvControls_Callback;
#ifdef RU
	uiAdvControls.sensitivity.generic.statusText = "��������� ���������������� ���� � ����";
#else
	uiAdvControls.sensitivity.generic.statusText = "Set in-game mouse sensitivity";
#endif

	UI_AdvControls_GetConfig ();

	UI_AddItem (&uiAdvControls.menu, (void *)&uiAdvControls.background);
	UI_AddItem (&uiAdvControls.menu, (void *)&uiAdvControls.banner);
	UI_AddItem (&uiAdvControls.menu, (void *)&uiAdvControls.done);
	UI_AddItem (&uiAdvControls.menu, (void *)&uiAdvControls.crosshair);
	UI_AddItem (&uiAdvControls.menu, (void *)&uiAdvControls.invertMouse);
	UI_AddItem (&uiAdvControls.menu, (void *)&uiAdvControls.mouseLook);
	UI_AddItem (&uiAdvControls.menu, (void *)&uiAdvControls.lookSpring);
	UI_AddItem (&uiAdvControls.menu, (void *)&uiAdvControls.lookStrafe);
	UI_AddItem (&uiAdvControls.menu, (void *)&uiAdvControls.infiniteRun);	// ESHQ: ������ ���������
	UI_AddItem (&uiAdvControls.menu, (void *)&uiAdvControls.autoaim);
	UI_AddItem (&uiAdvControls.menu, (void *)&uiAdvControls.sensitivity);
	}

/***
=================
UI_AdvControls_Precache
=================
***/
void UI_AdvControls_Precache (void)
	{
	PIC_Load (ART_BACKGROUND);
	PIC_Load (ART_BANNER);
	}

/***
=================
UI_AdvControls_Menu
=================
***/
void UI_AdvControls_Menu (void)
	{
	UI_AdvControls_Precache ();
	UI_AdvControls_Init ();

	UI_AdvControls_UpdateConfig ();
	UI_PushMenu (&uiAdvControls.menu);
	}
