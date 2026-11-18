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

	LOC_Controls,
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

	LOC_AdvOptions,
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

	LOC_Game,
	LOC_Clients,

	LOC_JoinGame,
	LOC_JoinGameTip,

	LOC_CreateGame,
	uiInternetGames_createGame_generic_statusText,

	LOC_GameInfo,
	LOC_GameInfoTip,

	LOC_Refresh,
	LOC_RefreshTip,

	// CreateGame
	uiLanGame_createGame_generic_statusText,

	LOC_Time,
	LOC_ElapsedTime,

	uiLoadGame_load_generic_name,
	uiLoadGame_load_generic_statusText,

	LOC_Delete,
	LOC_DeleteTip,

	LOC_WarningDeleteGame,
	// OK
	// Cancel

	uiMain_resumeGame_generic_name,
	uiMain_resumeGame_generic_statusText,

	uiMain_newGame_generic_name,
	uiMain_newGame_generic_statusText,

	uiMain_hazardCourse_generic_name,
	uiMain_hazardCourse_generic_statusText,

	uiMain_saveRestore_generic_name,
	uiMain_saveRestore_generic_statusText,
	LOC_Load,
	LOC_LoadTip,

	uiMain_configuration_generic_name,
	uiMain_configuration_generic_statusText,

	uiMain_multiPlayer_generic_name,
	uiMain_multiPlayer_generic_statusText,

	uiMain_customGame_generic_name,
	uiMain_customGame_generic_statusText,

	uiMain_previews_generic_name,
	uiMain_previews_generic_statusText,

	uiMain_quit_generic_name,
	uiMain_quit_generic_statusText,

	uiMain_console_generic_name,

	uiMain_quitActiveMessage_generic_name,
	uiMain_quitMessage_generic_name,

	uiMain_dlgMessage1_generic_name,
	// OK
	// Cancel

	uiMultiPlayer_internetGames_generic_name,
	uiMultiPlayer_internetGames_generic_statusText,

	uiMultiPlayer_spectateGames_generic_name,
	uiMultiPlayer_spectateGames_generic_statusText,

	uiMultiPlayer_LANGame_generic_name,
	uiMultiPlayer_LANGame_generic_statusText,

	uiMultiPlayer_Customize_generic_name,
	uiMultiPlayer_Customize_generic_statusText,

	// Controls
	uiMultiPlayer_Controls_generic_statusText,

	uiNewGame_easy_generic_name,
	uiNewGame_easy_generic_statusText,

	uiNewGame_medium_generic_name,
	uiNewGame_medium_generic_statusText,

	uiNewGame_hard_generic_name,
	uiNewGame_hard_generic_statusText,

	uiNewGame_dlgMessage1_generic_name,
	// OK
	// Cancel

	// Done
	uiPlayerSetup_done_generic_statusText,

	// Adv. options
	uiPlayerSetup_AdvOptions_generic_statusText,

	uiPlayerSetup_name_generic_statusText,
	uiPlayerSetup_model_generic_statusText,

	uiPlayerSetup_topColor_generic_name,
	uiPlayerSetup_topColor_generic_statusText,

	uiPlayerSetup_bottomColor_generic_name,
	uiPlayerSetup_bottomColor_generic_statusText,

	uiPlayerSetup_showModels_generic_name,
	uiPlayerSetup_showModels_generic_statusText,

	uiPlayerSetup_hiModels_generic_name,
	uiPlayerSetup_hiModels_generic_statusText,

	uiSaveGame_saveDescription_Current,
	uiSaveGame_saveDescription_Save,
	uiSaveGame_saveDescription_New,

	LOC_Save,
	LOC_SaveTip,

	uiSaveLoad_hintText_Save,
	uiSaveLoad_hintText_Load,

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
