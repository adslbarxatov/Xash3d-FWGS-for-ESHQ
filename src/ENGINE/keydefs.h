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

// [FWGS, 01.02.25]
/*ifndef KEYDEFS_H
define KEYDEFS_H*/

/*//
// these are the key numbers that should be passed to Key_Event
//
define K_TAB			9
define K_ENTER			13
define K_ESCAPE		27
define K_SPACE			32
define K_SCROLLOCK		70

// normal keys should be passed as lowercased ascii*/

// [FWGS, 01.02.25]
/*define K_BACKSPACE		127
define K_UPARROW		128
define K_DOWNARROW		129
define K_LEFTARROW		130
define K_RIGHTARROW	131*/

/*define K_ALT		132
define K_CTRL		133
define K_SHIFT		134
define K_F1		135
define K_F2		136
define K_F3		137
define K_F4		138
define K_F5		139
define K_F6		140
define K_F7		141
define K_F8		142
define K_F9		143
define K_F10		144
define K_F11		145
define K_F12		146
define K_INS		147
define K_DEL		148
define K_PGDN		149
define K_PGUP		150
define K_HOME		151
define K_END		152*/

// taken from Quake-2 source code and modified for GoldSrc compatibility

// [FWGS, 01.02.25]
/*define K_KP_HOME		160
define K_KP_UPARROW	161
define K_KP_PGUP		162
define K_KP_LEFTARROW	163
define K_KP_5			164*/

//
// these are the key numbers that should be passed to Key_Event
// normal keys should be passed as lowercased ascii
//
#define K_TAB			9
#define K_ENTER			13
#define K_ESCAPE		27
#define K_SPACE			32
#define K_SCROLLLOCK	70
#define K_BACKSPACE		127
#define K_UPARROW		128
#define K_DOWNARROW		129
#define K_LEFTARROW		130
#define K_RIGHTARROW	131
#define K_ALT		132
#define K_CTRL		133
#define K_SHIFT		134
#define K_F1		135
#define K_F2		136
#define K_F3		137
#define K_F4		138
#define K_F5		139
#define K_F6		140
#define K_F7		141
#define K_F8		142
#define K_F9		143
#define K_F10		144
#define K_F11		145
#define K_F12		146
#define K_INS		147
#define K_DEL		148
#define K_PGDN		149
#define K_PGUP		150
#define K_HOME		151
#define K_END		152

#define K_KP_HOME		160
#define K_KP_UPARROW	161
#define K_KP_PGUP		162
#define K_KP_LEFTARROW	163
#define K_KP_5			164
#define K_KP_RIGHTARROW	165

// [FWGS, 01.02.25]
/*define K_KP_END		166
define K_KP_DOWNARROW	167
define K_KP_PGDN		168
define K_KP_ENTER		169
define K_KP_INS   		170
define K_KP_DEL		171
define K_KP_SLASH		172
define K_KP_MINUS		173
define K_KP_PLUS		174
define K_CAPSLOCK		175

// FWGS
define K_KP_MUL		176
define K_WIN			177
define K_KP_NUMLOCK	178*/
#define K_KP_END		166
#define K_KP_DOWNARROW	167
#define K_KP_PGDN		168
#define K_KP_ENTER		169
#define K_KP_INS		170
#define K_KP_DEL		171
#define K_KP_SLASH		172
#define K_KP_MINUS		173
#define K_KP_PLUS		174
#define K_CAPSLOCK		175
#define K_KP_MUL		176
#define K_WIN			177
#define K_KP_NUMLOCK	178

//
// joystick buttons [FWGS, 01.02.25]
//
/*define K_JOY1		203	// FWGS: LTRIGGER (L2)
define K_JOY2		204	// FWGS: RTRIGGER (R2)
define K_JOY3		205
define K_JOY4		206*/
#define K_JOY1		203
#define K_JOY2		204
#define K_JOY3		205
#define K_JOY4		206

// FWGS
// aux keys are for multi-buttoned joysticks to generate so they can use
// the normal binding process

// [FWGS, 01.02.25]
/*define K_AUX1		207
define K_A_BUTTON	K_AUX1
define K_AUX2		208
define K_B_BUTTON	K_AUX2
define K_AUX3		209
define K_X_BUTTON	K_AUX3
define K_AUX4		210
define K_Y_BUTTON	K_AUX4
define K_AUX5		211
define K_L1_BUTTON	K_AUX5
define K_AUX6		212
define K_R1_BUTTON	K_AUX6
define K_AUX7		213
define K_BACK_BUTTON	K_AUX7
define K_AUX8		214
define K_MODE_BUTTON	K_AUX8
define K_AUX9		215
define K_START_BUTTON	K_AUX9
define K_AUX10		216
define K_LSTICK	K_AUX10
define K_AUX11		217
define K_RSTICK	K_AUX11*/
#define K_AUX1		207
#define K_AUX2		208
#define K_AUX3		209
#define K_AUX4		210
#define K_AUX5		211
#define K_AUX6		212
#define K_AUX7		213
#define K_AUX8		214
#define K_AUX9		215
#define K_AUX10		216
#define K_AUX11		217
#define K_AUX12		218
#define K_AUX13		219
#define K_AUX14		220
#define K_AUX15		221
#define K_AUX16		222
#define K_AUX17		223
#define K_AUX18		224
#define K_AUX19		225
#define K_AUX20		226
#define K_AUX21		227
#define K_AUX22		228
#define K_AUX23		229
#define K_AUX24		230
#define K_AUX25		231
#define K_AUX26		232
#define K_AUX27		233
#define K_AUX28		234
#define K_AUX29		235
#define K_AUX30		236
#define K_AUX31		237
#define K_AUX32		238

// [FWGS, 01.02.25]
/*define K_AUX12		218
define K_L2_BUTTON	K_AUX12
define K_AUX13		219
define K_R2_BUTTON	K_AUX13
define K_AUX14		220
define K_C_BUTTON	K_AUX14
define K_AUX15		221
define K_Z_BUTTON	K_AUX15
define K_AUX16		222
define K_DPAD_UP	K_AUX16
define K_AUX17		223
define K_DPAD_DOWN	K_AUX17
define K_AUX18		224
define K_DPAD_LEFT	K_AUX18
define K_AUX19		225
define K_DPAD_RIGHT	K_AUX19

// [FWGS, 01.04.23: Xbox Series, PS5, NSPro]
define K_AUX20		226
define K_MISC_BUTTON		K_AUX20 // Xbox Series X share button, PS5 microphone button, Nintendo Switch Pro capture button
define K_AUX21		227
define K_PADDLE1_BUTTON	K_AUX21 // Xbox Elite paddle P1-P4
define K_AUX22		228*/

//
// button names for game pads
//
#define K_A_BUTTON			K_AUX1
#define K_B_BUTTON			K_AUX2
#define K_X_BUTTON			K_AUX3
#define K_Y_BUTTON			K_AUX4
#define K_L1_BUTTON			K_AUX5
#define K_R1_BUTTON			K_AUX6
#define K_BACK_BUTTON		K_AUX7
#define K_MODE_BUTTON		K_AUX8
#define K_START_BUTTON		K_AUX9
#define K_LSTICK			K_AUX10
#define K_RSTICK			K_AUX11
#define K_L2_BUTTON			K_AUX12
#define K_R2_BUTTON			K_AUX13
#define K_C_BUTTON			K_AUX14
#define K_Z_BUTTON			K_AUX15
#define K_DPAD_UP			K_AUX16
#define K_DPAD_DOWN			K_AUX17
#define K_DPAD_LEFT			K_AUX18
#define K_DPAD_RIGHT		K_AUX19
#define K_MISC_BUTTON		K_AUX20
#define K_PADDLE1_BUTTON	K_AUX21
#define K_PADDLE2_BUTTON	K_AUX22
/*define K_AUX23		229*/
#define K_PADDLE3_BUTTON	K_AUX23
/*define K_AUX24		230*/
#define K_PADDLE4_BUTTON	K_AUX24
/*define K_AUX25		231
define K_TOUCHPAD			K_AUX25 // PS4/PS5 touchpad button
define K_AUX26		232
define K_AUX27		233
define K_AUX28		234
define K_AUX29		235
define K_AUX30		236
define K_AUX31		237
define K_AUX32		238
define K_MWHEELDOWN	239
define K_MWHEELUP		240
define K_PAUSE		255*/
#define K_TOUCHPAD			K_AUX25

//
// mouse buttons generate virtual keys [FWGS, 01.02.25]
//
/*define K_MOUSE1		241
define K_MOUSE2		242
define K_MOUSE3		243
define K_MOUSE4		244
define K_MOUSE5		245
endif*/
#define K_MWHEELDOWN	239
#define K_MWHEELUP		240
#define K_MOUSE1		241
#define K_MOUSE2		242
#define K_MOUSE3		243
#define K_MOUSE4		244
#define K_MOUSE5		245

#define K_PAUSE			255
