/***
voice.c - voice chat implementation
Copyright (C) 2022 Velaron
Copyright (C) 2022 SNMetamorph

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details
***/

#define CUSTOM_MODES 1 // required to correctly link with Opus Custom

#ifdef OPUS
	#include <opus_custom.h>
#endif

#include "common.h"
#include "client.h"
#include "voice.h"

voice_state_t voice = { 0 };

CVAR_DEFINE_AUTO (voice_enable, "1", FCVAR_PRIVILEGED | FCVAR_ARCHIVE,
	"enable voice chat");
CVAR_DEFINE_AUTO (voice_loopback, "0", FCVAR_PRIVILEGED,
	"loopback voice back to the speaker");
CVAR_DEFINE_AUTO (voice_scale, "1.0", FCVAR_PRIVILEGED | FCVAR_ARCHIVE,
	"incoming voice volume scale");

// [FWGS, 01.05.24]
CVAR_DEFINE_AUTO (voice_transmit_scale, "1.0", FCVAR_PRIVILEGED | FCVAR_ARCHIVE,
	"outcoming voice volume scale");

CVAR_DEFINE_AUTO (voice_avggain, "0.5", FCVAR_PRIVILEGED | FCVAR_ARCHIVE,
	"automatic voice gain control (average)");
CVAR_DEFINE_AUTO (voice_maxgain, "5.0", FCVAR_PRIVILEGED | FCVAR_ARCHIVE,
	"automatic voice gain control (maximum)");
CVAR_DEFINE_AUTO (voice_inputfromfile, "0", FCVAR_PRIVILEGED,
	"input voice from voice_input.wav");

// [FWGS, 01.05.24]
static void Voice_ApplyGainAdjust (int16_t *samples, int count, float scale);

// [FWGS, 01.09.24] in case user enabled voice after connection
// can't keep in `voice` struct because it gets zeroed on shutdown
static string voice_codec_init;
static int voice_quality_init;

/***
===============================================================================
OPUS INTEGRATION
===============================================================================
***/

#ifdef OPUS

// [FWGS, 01.05.24]
static qboolean Voice_InitCustomMode (void)
	{
	int err = 0;

	voice.width = sizeof (opus_int16);
	voice.samplerate = VOICE_OPUS_CUSTOM_SAMPLERATE;
	voice.frame_size = VOICE_OPUS_CUSTOM_FRAME_SIZE;

	voice.custom_mode = opus_custom_mode_create (SOUND_44k, voice.frame_size, &err);
	if (!voice.custom_mode)
		{
		Con_Printf (S_ERROR "Can't create Opus Custom mode: %s\n", opus_strerror (err));
		return false;
		}

	return true;
	}

/***
=========================
Voice_InitOpusDecoder [FWGS, 01.05.24]
=========================
***/
static qboolean Voice_InitOpusDecoder (void)
	{
	int err = 0;

	for (int i = 0; i < cl.maxclients; i++)
		{
		voice.decoders[i] = opus_custom_decoder_create (voice.custom_mode, VOICE_PCM_CHANNELS, &err);

		if (!voice.decoders[i])
			{
			Con_Printf (S_ERROR "Can't create Opus decoder for %i: %s\n", i, opus_strerror (err));
			return false;
			}
		}

	return true;
	}

/***
=========================
Voice_InitOpusEncoder [FWGS, 01.05.24]
=========================
***/
static qboolean Voice_InitOpusEncoder (int quality)
	{
	int err = 0;

	voice.encoder = opus_custom_encoder_create (voice.custom_mode, VOICE_PCM_CHANNELS, &err);
	if (!voice.encoder)
		{
		Con_Printf (S_ERROR "Can't create Opus encoder: %s\n", opus_strerror (err));
		return false;
		}

	switch (quality)
		{
		case 1: // 6 kbps
			opus_custom_encoder_ctl (voice.encoder, OPUS_SET_BITRATE (6000));
			break;
		case 2: // 12 kbps
			opus_custom_encoder_ctl (voice.encoder, OPUS_SET_BITRATE (12000));
			break;
		case 4: // 64 kbps
			opus_custom_encoder_ctl (voice.encoder, OPUS_SET_BITRATE (64000));
			break;
		case 5: // 96 kbps
			opus_custom_encoder_ctl (voice.encoder, OPUS_SET_BITRATE (96000));
			break;
		default: // 36 kbps
			opus_custom_encoder_ctl (voice.encoder, OPUS_SET_BITRATE (36000));
			break;
		}

	return true;
	}

/***
=========================
Voice_ShutdownOpusDecoder [FWGS, 01.05.24]
=========================
***/
static void Voice_ShutdownOpusDecoder (void)
	{
	for (int i = 0; i < MAX_CLIENTS; i++)
		{
		if (!voice.decoders[i])
			continue;

		opus_custom_decoder_destroy (voice.decoders[i]);
		voice.decoders[i] = NULL;
		}
	}

/***
=========================
Voice_ShutdownOpusEncoder [FWGS, 01.05.24]
=========================
***/
static void Voice_ShutdownOpusEncoder (void)
	{
	if (voice.encoder)
		{
		opus_custom_encoder_destroy (voice.encoder);
		voice.encoder = NULL;
		}
	}

// [FWGS, 01.05.24]
static void Voice_ShutdownCustomMode (void)
	{
	if (voice.custom_mode)
		{
		opus_custom_mode_destroy (voice.custom_mode);
		voice.custom_mode = NULL;
		}
	}

/***
=========================
Voice_GetOpusCompressedData
=========================
***/
static uint Voice_GetOpusCompressedData (byte *out, uint maxsize, uint *frames)
	{
	uint ofs = 0, size = 0;
	uint frame_size_bytes = voice.frame_size * voice.width;

	if (voice.input_file)
		{
		uint numbytes;
		
		// [FWGS, 01.04.23]
		double updateInterval, curtime = Sys_DoubleTime ();

		updateInterval = curtime - voice.start_time;
		voice.start_time = curtime;

		numbytes = updateInterval * voice.samplerate * voice.width * VOICE_PCM_CHANNELS;
		numbytes = Q_min (numbytes, voice.input_file->size - voice.input_file_pos);
		numbytes = Q_min (numbytes, sizeof (voice.input_buffer) - voice.input_buffer_pos);

		memcpy (voice.input_buffer + voice.input_buffer_pos, voice.input_file->buffer + voice.input_file_pos, numbytes);
		voice.input_buffer_pos += numbytes;
		voice.input_file_pos += numbytes;
		}

	if (!voice.input_file)
		VoiceCapture_Lock (true);

	for (ofs = 0; voice.input_buffer_pos - ofs >= frame_size_bytes && ofs <= voice.input_buffer_pos;
		ofs += frame_size_bytes)
		{
		int bytes;

		// [FWGS, 01.05.24] adjust gain before encoding, but only for input from voice
		if (!voice.input_file)
			Voice_ApplyGainAdjust ((opus_int16 *)(voice.input_buffer + ofs), voice.frame_size, voice_transmit_scale.value);

		bytes = opus_custom_encode (voice.encoder, (const opus_int16 *)(voice.input_buffer + ofs),
			voice.frame_size, out + size + sizeof (uint16_t), maxsize);

		if (bytes > 0)
			{
			// write compressed frame size
			*((uint16_t *)&out[size]) = bytes;

			size += bytes + sizeof (uint16_t);
			maxsize -= bytes + sizeof (uint16_t);

			(*frames)++;
			}
		else
			{
			Con_Printf (S_ERROR "%s: failed to encode frame: %s\n", __func__, opus_strerror (bytes));
			}
		}

	// did we compress anything? update counters
	if (ofs)
		{
		fs_offset_t remaining = voice.input_buffer_pos - ofs;

		// move remaining samples to the beginning of buffer
		memmove (voice.input_buffer, voice.input_buffer + ofs, remaining);

		voice.input_buffer_pos = remaining;
		}

	if (!voice.input_file)
		VoiceCapture_Lock (false);

	return size;
	}

#endif

/***
===============================================================================
VOICE CHAT INTEGRATION
===============================================================================
***/

/***
=========================
Voice_ApplyGainAdjust [FWGS, 01.05.24]
=========================
***/
static void Voice_ApplyGainAdjust (int16_t *samples, int count, float scale)
	{
	float	gain, modifiedMax;
	int		average, blockOffset = 0;

	for (;;)
		{
		int i, localMax = 0, localSum = 0;
		int blockSize = Q_min (count - (blockOffset + voice.autogain.block_size), voice.autogain.block_size);

		if (blockSize < 1)
			break;

		for (i = 0; i < blockSize; ++i)
			{
			int sample = samples[blockOffset + i];
			int absSample = abs (sample);

			if (absSample > localMax)
				localMax = absSample;

			localSum += absSample;

			gain = voice.autogain.current_gain + i * voice.autogain.gain_multiplier;
			samples[blockOffset + i] = bound (SHRT_MIN, (int)(sample * gain), SHRT_MAX);
			}

		if (blockOffset % voice.autogain.block_size == 0)
			{
			average = localSum / blockSize;
			modifiedMax = average + (localMax - average) * voice_avggain.value;

			voice.autogain.current_gain = voice.autogain.next_gain * scale;
			voice.autogain.next_gain = Q_min ((float)SHRT_MAX / modifiedMax, voice_maxgain.value) * scale;
			voice.autogain.gain_multiplier = (voice.autogain.next_gain - voice.autogain.current_gain) / (blockSize - 1);
			}

		blockOffset += blockSize;
		}
	}

/***
=========================
Voice_Status

Notify user dll aboit voice transmission
=========================
***/
static void Voice_Status (int entindex, qboolean bTalking)
	{
	if (cls.state == ca_active && clgame.dllFuncs.pfnVoiceStatus)
		clgame.dllFuncs.pfnVoiceStatus (entindex, bTalking);
	}

/***
=========================
Voice_StatusTimeout

Waits few milliseconds and if there was no
voice transmission, sends notification
=========================
***/
static void Voice_StatusTimeout (voice_status_t *status, int entindex, double frametime)
	{
	if (status->talking_ack)
		{
		status->talking_timeout += frametime;
		if (status->talking_timeout > 0.2)
			{
			status->talking_ack = false;
			Voice_Status (entindex, false);
			}
		}
	}

/***
=========================
Voice_StatusAck

Sends notification to user dll and
zeroes timeouts for this client
=========================
***/
void Voice_StatusAck (voice_status_t *status, int playerIndex)
	{
	if (!status->talking_ack)
		Voice_Status (playerIndex, true);

	status->talking_ack = true;
	status->talking_timeout = 0.0;
	}

/***
=========================
Voice_IsRecording
=========================
***/
qboolean Voice_IsRecording (void)
	{
	return voice.is_recording;
	}

/***
=========================
Voice_RecordStop
=========================
***/
void Voice_RecordStop (void)
	{
	if (voice.input_file)
		{
		FS_FreeSound (voice.input_file);
		voice.input_file = NULL;
		}

	VoiceCapture_Activate (false);
	voice.is_recording = false;

	Voice_Status (VOICE_LOCALCLIENT_INDEX, false);

	voice.input_buffer_pos = 0;
	memset (voice.input_buffer, 0, sizeof (voice.input_buffer));
	}

/***
=========================
Voice_RecordStart [FWGS, 01.12.24]
=========================
***/
void Voice_RecordStart (void)
	{
	Voice_RecordStop ();

	if (!voice.initialized)
		return;

	if (voice_inputfromfile.value)
		{
		voice.input_file = FS_LoadSound ("voice_input.wav", NULL, 0);
		if (voice.input_file)
			{
			Sound_Process (&voice.input_file, voice.samplerate, voice.width, VOICE_PCM_CHANNELS, SOUND_RESAMPLE);
			voice.input_file_pos = 0;

			voice.start_time = Sys_DoubleTime ();
			voice.is_recording = true;
			}
		else
			{
			FS_FreeSound (voice.input_file);
			voice.input_file = NULL;
			}
		}

	/*if (!Voice_IsRecording ())*/
	if (!Voice_IsRecording () && voice.device_opened)
		voice.is_recording = VoiceCapture_Activate (true);

	if (Voice_IsRecording ())
		Voice_Status (VOICE_LOCALCLIENT_INDEX, true);
	}

/***
=========================
Voice_Disconnect

We're disconnected from server
stop recording and notify user dlls
=========================
***/
void Voice_Disconnect (void)
	{
	int i;

	Voice_RecordStop ();

	if (voice.local.talking_ack)
		{
		Voice_Status (VOICE_LOOPBACK_INDEX, false);
		voice.local.talking_ack = false;
		}

	for (i = 0; i < MAX_CLIENTS; i++)
		{
		if (voice.players_status[i].talking_ack)
			{
			Voice_Status (i, false);
			voice.players_status[i].talking_ack = false;
			}
		}

	// [FWGS, 01.02.24]
	VoiceCapture_Shutdown ();
	voice.device_opened = false;
	}

/***
=========================
Voice_StartChannel [FWGS, 01.05.24]

Feed the decoded data to engine sound subsystem
=========================
***/
static void Voice_StartChannel (uint samples, byte *data, int entnum)
	{
	SND_ForceInitMouth (entnum);
	S_RawEntSamples (entnum, samples, voice.samplerate, voice.width, VOICE_PCM_CHANNELS, data,
		bound (0, 255 * voice_scale.value, 255));
	}

/***
=========================
Voice_AddIncomingData [FWGS, 01.05.24]

Received encoded voice data, decode it
=========================
***/
void Voice_AddIncomingData (int ent, const byte *data, uint size, uint frames)
	{
	const int playernum = ent - 1;
	int samples = 0;
	int ofs = 0;

	if ((playernum < 0) || (playernum >= cl.maxclients) || (!voice.decoders[playernum]))
		return;

	// decode frame by frame
	for (;;)
		{
		int frame_samples;
		uint16_t compressed_size;

		// no compressed size mark
		if (ofs + sizeof (uint16_t) > size)
			break;

		compressed_size = *(const uint16_t *)(data + ofs);
		ofs += sizeof (uint16_t);

		// no frame data
		if (ofs + compressed_size > size)
			break;

#ifdef OPUS
		frame_samples = opus_custom_decode (voice.decoders[playernum], data + ofs, compressed_size,
			(opus_int16 *)voice.decompress_buffer + samples, voice.frame_size);
#endif

		ofs += compressed_size;
		samples += frame_samples;
		}

	if (samples > 0)
		Voice_StartChannel (samples, voice.decompress_buffer, ent);
	}

/***
=========================
CL_AddVoiceToDatagram [FWGS, 01.05.24]

Encode our voice data and send it to server
=========================
***/
void CL_AddVoiceToDatagram (void)
	{
	uint size = 0, frames = 0;

	if ((cls.state != ca_active) || !Voice_IsRecording () || !voice.encoder)
		return;

#ifdef OPUS
	size = Voice_GetOpusCompressedData (voice.compress_buffer, sizeof (voice.compress_buffer), &frames);
#endif

	if ((size > 0) && (MSG_GetNumBytesLeft (&cls.datagram) >= size + 32))
		{
		MSG_BeginClientCmd (&cls.datagram, clc_voicedata);
		MSG_WriteByte (&cls.datagram, voice_loopback.value != 0);
		MSG_WriteByte (&cls.datagram, frames);
		MSG_WriteShort (&cls.datagram, size);
		MSG_WriteBytes (&cls.datagram, voice.compress_buffer, size);
		}
	}

/***
=========================
Voice_RegisterCvars

Register voice related cvars and commands
=========================
***/
void Voice_RegisterCvars (void)
	{
	Cvar_RegisterVariable (&voice_enable);
	Cvar_RegisterVariable (&voice_loopback);
	Cvar_RegisterVariable (&voice_scale);
	Cvar_RegisterVariable (&voice_transmit_scale);	// [FWGS, 01.05.24]
	Cvar_RegisterVariable (&voice_avggain);
	Cvar_RegisterVariable (&voice_maxgain);
	Cvar_RegisterVariable (&voice_inputfromfile);
	}

/***
=========================
Voice_Shutdown [FWGS, 01.05.24]

Completely shutdown the voice subsystem
=========================
***/
static void Voice_Shutdown (void)
	{
	int i;

	Voice_RecordStop ();

#ifdef OPUS
	Voice_ShutdownOpusDecoder ();
	Voice_ShutdownOpusEncoder ();
	Voice_ShutdownCustomMode ();
#endif

	VoiceCapture_Shutdown ();

	if (voice.local.talking_ack)
		Voice_Status (VOICE_LOOPBACK_INDEX, false);

	for (i = 0; i < MAX_CLIENTS; i++)
		{
		if (voice.players_status[i].talking_ack)
			Voice_Status (i, false);
		}

	memset (&voice, 0, sizeof (voice));
	}

/***
=========================
Voice_Idle [FWGS, 01.09.24]

Run timeout for all clients
=========================
***/
void Voice_Idle (double frametime)
	{
	int i;

	if (FBitSet (voice_enable.flags, FCVAR_CHANGED))
		{
		ClearBits (voice_enable.flags, FCVAR_CHANGED);

		if (voice_enable.value)
			{
			if ((cls.state == ca_active) && COM_CheckString (voice_codec_init) && (voice_quality_init != 0))
				Voice_Init (voice_codec_init, voice_quality_init, false);
			}
		else
			{
			Voice_Shutdown ();
			}
		}

	// update local player status first
	Voice_StatusTimeout (&voice.local, VOICE_LOOPBACK_INDEX, frametime);

	for (i = 0; i < MAX_CLIENTS; i++)
		Voice_StatusTimeout (&voice.players_status[i], i, frametime);
	}

/***
=========================
Voice_Init [FWGS, 01.12.24]

Initialize the voice subsystem
=========================
***/
qboolean Voice_Init (const char *pszCodecName, int quality, qboolean preinit)
	{
	if (Q_strcmp (pszCodecName, VOICE_OPUS_CUSTOM_CODEC))
		{
		/*Con_Printf (S_ERROR "Server requested unsupported codec: %s\n", pszCodecName);*/
		if (COM_CheckStringEmpty (pszCodecName))
			Con_Printf (S_ERROR "Server requested unsupported codec: %s\n", pszCodecName);

		// reset saved codec name, we won't enable voice for this connection
		voice_codec_init[0] = 0;
		voice_quality_init = 0;
		return false;
		}

	Q_strncpy (voice_codec_init, pszCodecName, sizeof (voice_codec_init));
	voice_quality_init = quality;

	if (!voice_enable.value)
		return false;

	// reinitialize only if codec parameters are different
	if (Q_strcmp (pszCodecName, voice.codec) || (voice.quality != quality))
		{
		Voice_Shutdown ();
		voice.autogain.block_size = 128;

#if OPUS
		if (!Voice_InitCustomMode ())
			{
			// no reason to init encoder and open audio device
			// if we can't hear other players
			Voice_Shutdown ();
			return false;
			}
#else
		return false;
#endif

		// we can hear others players, so it's fine to fail now
		voice.initialized = true;
		Q_strncpy (voice.codec, pszCodecName, sizeof (voice.codec));

#if OPUS
		if (!Voice_InitOpusEncoder (quality))
#endif
			{
			Con_Printf (S_WARN "Other players will not be able to hear you.\n");
			return false;
			}

		voice.quality = quality;
		}

	if (!preinit)
		{
#if OPUS
		Voice_ShutdownOpusDecoder ();
		if (!Voice_InitOpusDecoder ())
#endif
			{
			// no reason to init encoder and open audio device
			// if we can't hear other players
			Con_Printf (S_ERROR "Can't create decoders, voice chat is disabled.\n");
			Voice_Shutdown ();
			return false;
			}

		voice.device_opened = VoiceCapture_Init ();
		if (!voice.device_opened)
			Con_Printf (S_WARN "No microphone is available.\n");
		}

	return true;
	}