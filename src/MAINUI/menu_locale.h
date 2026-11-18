/***
menu_locale.h - заголовок дл€ обработчика локализации меню
***/

enum
	{
	LOC_Done = 0,
	LOC_SaveAndReturnToSettingsMenu,

	uiAdvControls_crosshair_generic_name,
	uiAdvControls_crosshair_generic_statusText,

	uiAdvControls_invertMouse_generic_name,
	uiAdvControls_invertMouse_generic_statusText,

	uiAdvControls_mouseLook_generic_name,
	uiAdvControls_mouseLook_generic_statusText,

	uiAdvControls_lookSpring_generic_name,
	uiAdvControls_lookSpring_generic_statusText,

	uiAdvControls_lookStrafe_generic_name,
	uiAdvControls_lookStrafe_generic_statusText,

	uiAdvControls_infiniteRun_generic_name,
	uiAdvControls_infiniteRun_generic_statusText,

	uiAdvControls_autoaim_generic_name,
	uiAdvControls_autoaim_generic_statusText,

	uiAdvControls_sensitivity_generic_name,
	uiAdvControls_sensitivity_generic_statusText,

	// Done
	uiAudio_done_generic_statusText,

	uiAudio_soundVolume_generic_name,
	uiAudio_soundVolume_generic_statusText,

	uiAudio_musicVolume_generic_name,
	uiAudio_musicVolume_generic_statusText,

	uiAudio_suitVolume_generic_name,
	uiAudio_suitVolume_generic_statusText,

	uiAudio_lerping_generic_name,
	uiAudio_lerping_generic_statusText,

	uiAudio_noDSP_generic_name,
	uiAudio_noDSP_generic_statusText,

	uiOptions_controls_generic_name,
	uiOptions_controls_generic_statusText,

	uiOptions_audio_generic_name,
	uiOptions_audio_generic_statusText,

	uiOptions_video_generic_name,
	uiOptions_video_generic_statusText,

	uiOptions_update_generic_name,
	uiOptions_update_generic_statusText,

	// Done
	LOC_ReturnToMainMenu,

	uiOptions_updatePrompt_generic_name,

	LOC_OK,
	LOC_Cancel,

	uiControls_hintText_Action,
	uiControls_hintText_Key,
	uiControls_hintText_Variant,

	uiControls_defaults_generic_name,
	uiControls_defaults_generic_statusText,

	uiControls_advanced_generic_name,
	uiControls_advanced_generic_statusText,

	// OK
	uiControls_done_generic_statusText,

	// Cancel
	uiControls_cancel_generic_statusText,

	uiControls_tipMessage_generic_name,
	uiControls_dlgMessage_generic_name,

	uiControls_promptMessage_generic_name,
	// OK
	// Cancel

	LOC_Map,
	uiCreateGame_hintText_Title,

	uiCreateGame_advOptions_generic_name,
	uiCreateGame_advOptions_generic_statusText,

	// OK
	uiCreateGame_done_generic_statusText,

	// Cancel
	uiCreateGame_cancel_generic_statusText,

	uiCreateGame_dedicatedServer_generic_name,
	uiCreateGame_dedicatedServer_generic_statusText,

	uiCreateGame_hostName_generic_name,
	uiCreateGame_maxClients_generic_name,
	uiCreateGame_password_generic_name,

	LOC_WarningNewGame1,
	LOC_WarningNewGame2,
	// OK
	// Cancel

	LOC_Type,
	uiCustomGame_hintText_Name,
	uiCustomGame_hintText_Version,
	uiCustomGame_hintText_Size,

	uiCustomGame_load_generic_name,
	uiCustomGame_load_generic_statusText,

	uiCustomGame_go2url_generic_name,
	uiCustomGame_go2url_generic_statusText,

	// Done
	uiCustomGame_promptMessage_generic_name,

	// Cancel
	LOC_ReturnToSettingsMenu,

	uiGameOptions_maxFPS_generic_statusText,
	uiGameOptions_maxFPSmessage_generic_name,

	uiGameOptions_hand_generic_name,
	uiGameOptions_hand_generic_statusText,

	uiGameOptions_allowDownload_generic_name,
	uiGameOptions_allowDownload_generic_statusText,

	uiInternetGames_hintText_Game,
	uiInternetGames_hintText_Clients,

	uiInternetGames_joinGame_generic_name,
	uiInternetGames_joinGame_generic_statusText,

	uiInternetGames_createGame_generic_name,
	uiInternetGames_createGame_generic_statusText,

	uiInternetGames_gameInfo_generic_name,
	uiInternetGames_gameInfo_generic_statusText,

	uiInternetGames_refresh_generic_name,
	uiInternetGames_refresh_generic_statusText,

	LOC_STRINGSCOUNT	// must be last
	};

extern char *MenuLocStrings[LOC_STRINGSCOUNT];

#define MLOC(n)	(const char *)MenuLocStrings[n]

//pragma pack(push, 1)

union MenuLocStringHeader
	{
	struct MenuLocStringHeaderStr
		{
		char Number[4];	// Ќе задействован - пропускаетс€
		char Length[4];	// »спользуютс€ только первые три байта
		} Str;

	char Ptr[sizeof (struct MenuLocStringHeaderStr)];
	};

STATIC_ASSERT (sizeof (union MenuLocStringHeader) == 8, "Incorrect MenuLocStringHeader size");

//pragma pack(pop)
