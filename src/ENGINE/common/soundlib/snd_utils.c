/***
snd_utils.c - sound common tools
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

// [FWGS, 01.06.25]
#include "soundlib.h"
/*if XASH_SDL
include <SDL_audio.h>
endif*/

// [FWGS, 25.12.24] global sound variables
sndlib_t sound;

/***
=============================================================================
XASH3D LOAD SOUND FORMATS
=============================================================================
***/

// [FWGS, 01.04.25]
// ESHQ: внешние библиотеки отключены
static const loadwavfmt_t load_game[] =
	{
#ifndef XASH_DEDICATED
	{ "wav", Sound_LoadWAV },
	{ "mp3", Sound_LoadMPG },
#ifdef OGG_VORBIS
	{ "ogg", Sound_LoadOggVorbis },
#endif
#ifdef OPUS
	{ "opus", Sound_LoadOggOpus },
#endif

#else // we only need extensions
	{ "wav" },
	{ "mp3" },
#ifdef OGG_VORBIS
	{ "ogg" },
#endif
#ifdef OPUS
	{ "opus" },
#endif

#endif
	{ NULL },
	};

/***
=============================================================================
XASH3D PROCESS STREAM FORMATS
=============================================================================
***/

// [FWGS, 01.04.25]
static const streamfmt_t stream_game[] =
	{
#ifndef XASH_DEDICATED
	{ "mp3", Stream_OpenMPG, Stream_ReadMPG, Stream_SetPosMPG, Stream_GetPosMPG, Stream_FreeMPG },
	{ "wav", Stream_OpenWAV, Stream_ReadWAV, Stream_SetPosWAV, Stream_GetPosWAV, Stream_FreeWAV },
#ifdef OGG_VORBIS
	{ "ogg", Stream_OpenOggVorbis, Stream_ReadOggVorbis, Stream_SetPosOggVorbis, Stream_GetPosOggVorbis,
		Stream_FreeOggVorbis },
#endif
#ifdef OPUS
	{ "opus", Stream_OpenOggOpus, Stream_ReadOggOpus, Stream_SetPosOggOpus, Stream_GetPosOggOpus, Stream_FreeOggOpus },
#endif

#else // we only need extensions

	{ "mp3" },
	{ "wav" },
#ifdef OGG_VORBIS
	{ "ogg" },
#endif
#ifdef OPUS
	{ "opus" },
#endif

#endif
	{ NULL },
	};

// [FWGS, 25.12.24]
void Sound_Init (void)
	{
	// init pools
	host.soundpool = Mem_AllocPool ("SoundLib Pool");

	// install sound formats
	sound.loadformats = load_game;
	sound.streamformat = stream_game;

	sound.tempbuffer = NULL;
	}

void Sound_Shutdown (void)
	{
	Mem_Check (); // check for leaks
	Mem_FreePool (&host.soundpool);
	}

// [FWGS, 01.02.25] removed Sound_Copy

uint GAME_EXPORT Sound_GetApproxWavePlayLen (const char *filepath)
	{
	string		name;
	file_t		*f;
	wavehdr_t	wav;
	size_t		filesize;
	uint		msecs;

	Q_strncpy (name, filepath, sizeof (name));
	COM_FixSlashes (name);
	
	f = FS_Open (name, "rb", false);
	if (!f)
		return 0;

	if (FS_Read (f, &wav, sizeof (wav)) != sizeof (wav))
		{
		FS_Close (f);
		return 0;
		}

	filesize = FS_FileLength (f);
	filesize -= 128; // magic number from GoldSrc, seems to be header size

	FS_Close (f);

	// is real wav file ?
	if ((wav.riff_id != RIFFHEADER) || (wav.wave_id != WAVEHEADER) || (wav.fmt_id != FORMHEADER))
		return 0;

	if (wav.nAvgBytesPerSec >= 1000)
		msecs = (uint)((float)filesize / ((float)wav.nAvgBytesPerSec / 1000.0f));
	else
		msecs = (uint)(((float)filesize / (float)wav.nAvgBytesPerSec) * 1000.0f);

	return msecs;
	}

// [FWGS, 01.08.24] removed Sound_ConvertNoResample

// [FWGS, 01.08.24]
#define SOUND_FORMATCONVERT_BOILERPLATE( resamplemacro ) \
	if( inwidth == 1 ) \
		{ \
		const int8_t *data = (const int8_t *)sc->buffer; \
		if( outwidth == 1 ) \
			{ \
			int8_t *outdata = (int8_t *)sound.tempbuffer; \
			resamplemacro( 1 ) \
			} \
		else if( outwidth == 2 ) \
			{ \
			int16_t *outdata = (int16_t *)sound.tempbuffer; \
			resamplemacro( 256 ) \
			} \
		else \
			{ \
			return false; \
			} \
		} \
	else if( inwidth == 2 ) \
		{ \
		const int16_t *data = (const int16_t *)sc->buffer; \
		if( outwidth == 1 ) \
			{ \
			int8_t *outdata = (int8_t *)sound.tempbuffer; \
			resamplemacro( 1 / 256.0 ) \
			} \
		else if( outwidth == 2 ) \
			{ \
			int16_t *outdata = (int16_t *)sound.tempbuffer; \
			resamplemacro( 1 ) \
			} \
		else \
			{ \
			return false; \
			} \
		} \
	else \
		{ \
		return false; \
		} \

// [FWGS, 01.08.24]
#define SOUND_CONVERTNORESAMPLE_BOILERPLATE( multiplier ) \
	if( inchannels == 1 ) \
		{ \
		if( outchannels == 1 ) \
			{ \
			for( i = 0; i < outcount * outchannels; i++ ) \
				outdata[i] = data[i] * ( multiplier ); \
			} \
		else if( outchannels == 2 ) \
			{ \
			for( i = 0; i < outcount; i++ ) \
				{ \
				outdata[i * 2 + 0] = data[i] * ( multiplier ); \
				outdata[i * 2 + 1] = data[i] * ( multiplier ); \
				} \
			} \
		else \
			{ \
			return false; \
			} \
		} \
	else if( inchannels == 2 ) \
		{ \
		if( outchannels == 1 ) \
			{ \
			for( i = 0; i < outcount; i++ ) \
				outdata[i] = ( data[i * 2 + 0] + data[i * 2 + 1] ) * ( multiplier ) / 2; \
			} \
		else if( outchannels == 2 ) \
			{ \
			for( i = 0; i < outcount * outchannels; i++ ) \
				outdata[i] = data[i] * ( multiplier ); \
			} \
		else \
			{ \
			return false; \
			} \
		} \
	else \
		{ \
		return false; \
		} \

// [FWGS, 01.08.24]
#define SOUND_CONVERTDOWNSAMPLE_BOILERPLATE( multiplier ) \
	if( inchannels == 1 ) \
		{ \
		if( outchannels == 1 ) \
			{ \
			for( i = 0; i < outcount; i++ ) \
				{ \
				double j = stepscale * i; \
				outdata[i] = data[(int)j] * ( multiplier ); \
				} \
			} \
		else if( outchannels == 2 ) \
			{ \
			for( i = 0; i < outcount; i++ ) \
				{ \
				double j = stepscale * i;\
				outdata[i * 2 + 0] = data[(int)j] * ( multiplier ); \
				outdata[i * 2 + 1] = data[(int)j] * ( multiplier ); \
				} \
			} \
		else \
			{ \
			return false; \
			} \
		} \
	else if( inchannels == 2 ) \
		{ \
		if( outchannels == 1 ) \
			{ \
			for( i = 0; i < outcount; i++ ) \
				{ \
				double j = stepscale * i; \
				outdata[i] = ( data[((int)j) * 2 + 0] + data[((int)j) * 2 + 1] ) * ( multiplier ) / 2; \
				} \
			} \
		else if( outchannels == 2 ) \
			{ \
			for( i = 0; i < outcount; i++ ) \
				{ \
				double j = stepscale * i; \
				outdata[i * 2 + 0] = data[((int)j) * 2 + 0] * ( multiplier ); \
				outdata[i * 2 + 1] = data[((int)j) * 2 + 1] * ( multiplier ); \
				} \
			} \
		else \
			{ \
			return false; \
			} \
		} \
	else \
		{ \
		return false; \
		} \

// [FWGS, 01.08.24]
#define SOUND_CONVERTUPSAMPLE_BOILERPLATE( multiplier ) \
	if( inchannels == 1 ) \
		{ \
		if( outchannels == 1 ) \
			{ \
			for( i = 0; i < outcount; i++ ) \
				{ \
				double j = stepscale * i; \
				outdata[i] = data[(int)j] * ( multiplier ); \
				if( j != (int)j && j < incount ) \
					{ \
					double frac = ( j - (int)j ) * ( multiplier ); \
					outdata[i] += (data[(int)j+1] - data[(int)j]) * frac; \
					} \
				} \
			} \
		else if( outchannels == 2 ) \
			{ \
			for( i = 0; i < outcount; i++ ) \
				{ \
				double j = stepscale * i; \
				outdata[i * 2 + 0] = data[(int)j] * ( multiplier ); \
				if( j != (int)j && j < incount ) \
					{ \
					double frac = ( j - (int)j ) * ( multiplier ); \
					outdata[i * 2 + 0] += (data[(int)j+1] - data[(int)j]) * frac; \
					} \
				outdata[i * 2 + 1] = outdata[i * 2 + 0]; \
				} \
			} \
		else \
			{ \
			return false; \
			} \
		} \
	else if( inchannels == 2 ) \
		{ \
		if( outchannels == 1 ) \
			{ \
			for( i = 0; i < outcount; i++ ) \
				{ \
				double j = stepscale * i; \
				outdata[i] = ( data[((int)j) * 2 + 0] + data[((int)j) * 2 + 1] ) * ( multiplier ) / 2; \
				if( j != (int)j && j < incount ) \
					{ \
					double frac = ( j - (int)j ) * ( multiplier ) / 2; \
					outdata[i] += (data[((int)j + 1 ) * 2 + 0] - data[((int)j) * 2 + 0]) * frac; \
					outdata[i] += (data[((int)j + 1 ) * 2 + 1] - data[((int)j) * 2 + 1]) * frac; \
					} \
				} \
			} \
		else if( outchannels == 2 ) \
			{ \
			for( i = 0; i < outcount; i++ ) \
				{ \
				double j = stepscale * i; \
				outdata[i*2+0] = data[((int)j)*2+0] * ( multiplier ); \
				outdata[i*2+1] = data[((int)j)*2+1] * ( multiplier ); \
				if( j != (int)j && j < incount ) \
					{ \
					double frac = ( j - (int)j ) * ( multiplier ); \
					outdata[i*2+0] += (data[((int)j+1)*2+0] - data[((int)j)*2+0]) * frac; \
					outdata[i*2+1] += (data[((int)j+1)*2+1] - data[((int)j)*2+1]) * frac; \
					} \
				} \
			} \
		else \
			{ \
			return false; \
			} \
		} \
	else \
		{ \
		return false; \
		} \

// [FWGS, 01.08.24]
static qboolean Sound_ConvertNoResample (wavdata_t *sc, int inwidth, int inchannels, int outwidth,
	int outchannels, int outcount)
	{
	size_t i;

	// I could write a generic case here but I also want to make it easier for compiler
	// to unroll these for-loops

	SOUND_FORMATCONVERT_BOILERPLATE (SOUND_CONVERTNORESAMPLE_BOILERPLATE)

	return true;
	}

// [FWGS, 01.08.24]
static qboolean Sound_ConvertDownsample (wavdata_t *sc, int inwidth, int inchannels, int outwidth,
	int outchannels, int outcount, double stepscale)
	{
	size_t i;

	SOUND_FORMATCONVERT_BOILERPLATE (SOUND_CONVERTDOWNSAMPLE_BOILERPLATE)

	return true;
	}

// [FWGS, 01.08.24]
static qboolean Sound_ConvertUpsample (wavdata_t *sc, int inwidth, int inchannels, int incount, int outwidth,
	int outchannels, int outcount, double stepscale)
	{
	size_t i;

	incount--; // to not go past last sample while interpolating

	SOUND_FORMATCONVERT_BOILERPLATE (SOUND_CONVERTUPSAMPLE_BOILERPLATE)

	return true;
	}

// [FWGS, 01.08.24]
#undef SOUND_FORMATCONVERT_BOILERPLATE
#undef SOUND_CONVERTNORESAMPLE_BOILERPLATE
#undef SOUND_CONVERTDOWNSAMPLE_BOILERPLATE
#undef SOUND_CONVERTUPSAMPLE_BOILERPLATE

/***
================
Sound_ResampleInternal [FWGS, 01.02.25]
================
***/
static qboolean Sound_ResampleInternal (wavdata_t *sc, int outrate, int outwidth, int outchannels)
	{
	const int	inrate = sc->rate;
	const int	inwidth = sc->width;
	const int	inchannels = sc->channels;
	const int	incount = sc->samples;
	const int	insize = sc->size;

	qboolean	handled = false;
	double		stepscale;
	double		t1, t2;
	int			outcount;

	if ((inrate == outrate) && (inwidth == outwidth) && (inchannels == outchannels))
		return false;

	t1 = Sys_DoubleTime ();

	// this is usually 0.5, 1, or 2
	stepscale = (double)inrate / outrate;
	outcount = sc->samples / stepscale;
	sc->size = outcount * outwidth * outchannels;
	sc->channels = outchannels;

	sc->samples = outcount;
	if (FBitSet (sc->flags, SOUND_LOOPED))
		sc->loopStart = sc->loopStart / stepscale;

	sound.tempbuffer = (byte *)Mem_Realloc (host.soundpool, sound.tempbuffer, sc->size);

	// no resampling, just copy data
	if (inrate == outrate)
		handled = Sound_ConvertNoResample (sc, inwidth, inchannels, outwidth, outchannels, outcount);

	// fast case, usually downsample but is also ok for upsampling
	else if (inrate > outrate)
		handled = Sound_ConvertDownsample (sc, inwidth, inchannels, outwidth, outchannels, outcount, stepscale);

	// upsample case, w/ interpolation
	else
		handled = Sound_ConvertUpsample (sc, inwidth, inchannels, incount, outwidth, outchannels, outcount, stepscale);

	if (!handled)
		{
		// restore previously changed data
		sc->rate = inrate;
		sc->width = inwidth;
		sc->channels = inchannels;
		sc->samples = incount;
		sc->size = insize;

		Con_Printf (S_ERROR "%s: unsupported from [%d bit %d Hz %dch] to [%d bit %d Hz %dch]\n",
			__func__, inwidth * 8, inrate, inchannels, outwidth * 8, outrate, outchannels);
		return false;
		}

	t2 = Sys_DoubleTime ();
	sc->rate = outrate;
	sc->width = outwidth;

	// critical, report to mod developer
	if (t2 - t1 > 0.01f)
		Con_Printf (S_WARN "%s: from [%d bit %d Hz %dch] to [%d bit %d Hz %dch] (took %.3fs)\n",
			__func__, inwidth * 8, inrate, inchannels, outwidth * 8, outrate, outchannels, t2 - t1);
	else
		Con_Reportf ("%s: from [%d bit %d Hz %dch] to [%d bit %d Hz %dch] (took %.3fs)\n",
			__func__, inwidth * 8, inrate, inchannels, outwidth * 8, outrate, outchannels, t2 - t1);

	return true;
	}

qboolean Sound_Process (wavdata_t **wav, int rate, int width, int channels, uint flags)
	{
	wavdata_t	*snd = *wav;
	qboolean	result = true;

	// check for buffers
	if (unlikely (!snd || !snd->buffer))
		return false;

	// [FWGS, 01.02.25]
	if (likely (FBitSet (flags, SOUND_RESAMPLE) && ((width > 0) || (rate > 0) || (channels > 0))))
		{
		result = Sound_ResampleInternal (snd, rate, width, channels);
		if (result)
			{
			snd = Mem_Realloc (host.soundpool, snd, sizeof (*snd) + snd->size);
			memcpy (snd->buffer, sound.tempbuffer, snd->size);

			Mem_Free (sound.tempbuffer);
			sound.tempbuffer = NULL;
			}
		}

	*wav = snd;
	return result;
	}

// [FWGS, 01.04.25]
qboolean Sound_SupportedFileFormat (const char *fileext)
	{
	const loadwavfmt_t *format;

	if (COM_CheckStringEmpty (fileext))
		{
		for (format = sound.loadformats; format && format->ext; format++)
			{
			if (!Q_stricmp (format->ext, fileext))
				return true;
			}
		}
	return false;
	}
