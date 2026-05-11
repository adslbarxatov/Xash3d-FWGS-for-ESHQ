/***
s_mix.c - portable code to mix sounds
Copyright (C) 2009 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details
***/

#include "common.h"
#include "sound.h"
#include "client.h"

// ESHQ: было отменено в силу неисправности; вынужденно применено снова из-за фундаментальной
// отмены поддержки механизма
#define DISABLE_UPSAMPLING

// [FWGS, 01.03.26] removed definitions, structures, S_InitScaletable
/*// [FWGS, 01.07.25]
enum
	{
	IPAINTBUFFER = 0,
	IROOMBUFFER,
	ISTREAMBUFFER,
	IVOICEBUFFER,
	CPAINTBUFFERS,
	};

// [FWGS, 01.11.25] ESHQ: удаление постобработки отменено в св€зи с деградацией качества звуков на частоте 22 к√ц
ifndef DISABLE_UPSAMPLING
enum
	{
	FILTERTYPE_NONE = 0,
	FILTERTYPE_LINEAR,
	FILTERTYPE_CUBIC,
	};
endif

#define CCHANVOLUMES	2

// [FWGS, 09.05.24]
define SND_SCALE_BITS		7
define SND_SCALE_SHIFT		( 8 - SND_SCALE_BITS )
define SND_SCALE_LEVELS	( 1 << SND_SCALE_BITS )

// [FWGS, 01.11.25]
ifndef DISABLE_UPSAMPLING
// sound mixing buffer
define CPAINTFILTERMEM		3
define CPAINTFILTERS		4	// maximum number of consecutive upsample passes per paintbuffer
endif

// [FWGS, 09.05.24] fixed point stuff for real-time resampling
define FIX_BITS 28
define FIX_SCALE ( 1 << FIX_BITS )
define FIX_MASK (( 1 << FIX_BITS ) - 1 )
define FIX_FLOAT( a ) ((int)(( a ) * FIX_SCALE ))
define FIX( a ) (((int)( a )) << FIX_BITS )
define FIX_INTPART( a ) (((int)( a )) >> FIX_BITS )
define FIX_FRACPART( a ) (( a ) & FIX_MASK )

// [FWGS, 01.11.25]
typedef struct
	{
	qboolean	factive;			// if true, mix to this paintbuffer using flags
	portable_samplepair_t	*pbuf;	// front stereo mix buffer, for 2 or 4 channel mixing
ifndef DISABLE_UPSAMPLING
	int			ifilter;			// current filter memory buffer to use for upsampling pass
	portable_samplepair_t	fltmem[CPAINTFILTERS][CPAINTFILTERMEM];
endif
	} paintbuffer_t;

// [FWGS, 01.07.25]
static portable_samplepair_t	*g_curpaintbuffer;
static portable_samplepair_t	streambuffer[(PAINTBUFFER_SIZE + 1)];
static portable_samplepair_t	paintbuffer[(PAINTBUFFER_SIZE + 1)];
static portable_samplepair_t	roombuffer[(PAINTBUFFER_SIZE + 1)];
static portable_samplepair_t	voicebuffer[(PAINTBUFFER_SIZE + 1)];
static portable_samplepair_t	temppaintbuffer[(PAINTBUFFER_SIZE + 1)];
static paintbuffer_t			paintbuffers[CPAINTBUFFERS];

static int snd_scaletable[SND_SCALE_LEVELS][256];

void S_InitScaletable (void)
	{
	int	i, j;

	for (i = 0; i < SND_SCALE_LEVELS; i++)*/
static portable_samplepair_t roombuffer[(PAINTBUFFER_SIZE + 1)], paintbuffer[(PAINTBUFFER_SIZE + 1)];

// [FWGS, 01.03.26]
#define S_MakeMixMono( x ) \
	static void S_MixMono ## x( portable_samplepair_t *pbuf, const int *volume, const void *buf, int num_samples ) \
	{ \
	const int##x##_t *data = buf; \
	for( int i = 0; i < num_samples; i++ ) \
	{ \
	pbuf[i].left += ( data[i] * volume[0] ) >> ( x - 8 ); \
	pbuf[i].right += ( data[i] * volume[1] ) >> ( x - 8 ); \
	} \
	} \

// [FWGS, 01.03.26]
#define S_MakeMixStereo( x ) \
	static void S_MixStereo ## x( portable_samplepair_t *pbuf, const int *volume, const void *buf, int num_samples ) \
	{ \
	const int##x##_t *data = buf; \
	for( int i = 0; i < num_samples; i++ ) \
	{ \
	pbuf[i].left += ( data[i * 2 + 0] * volume[0] ) >> ( x - 8 ); \
	pbuf[i].right += ( data[i * 2 + 1] * volume[1] ) >> ( x - 8 ); \
	} \
	} \

// [FWGS, 01.03.26]
#define S_MakeMixMonoPitch( x ) \
	static void S_MixMonoPitch ## x( portable_samplepair_t *pbuf, const int *volume, const void *buf, double offset_frac, double rate_scale, int num_samples ) \
	{ \
	const int##x##_t *data = buf; \
	uint sample_idx = 0; \
	for( int i = 0; i < num_samples; i++ ) \
	{ \
	pbuf[i].left += ( data[sample_idx] * volume[0] ) >> ( x - 8 ); \
	pbuf[i].right += ( data[sample_idx] * volume[1] ) >> ( x - 8 ); \
	offset_frac += rate_scale; \
	sample_idx += (uint)offset_frac; \
	offset_frac -= (uint)offset_frac; \
	} \
	} \

// [FWGS, 01.03.26]
#define S_MakeMixStereoPitch( x ) \
	static void S_MixStereoPitch ## x( portable_samplepair_t *pbuf, const int *volume, const void *buf, double offset_frac, double rate_scale, int num_samples ) \
	{ \
	const int##x##_t *data = buf; \
	uint sample_idx = 0; \
	for( int i = 0; i < num_samples; i++ ) \
	{ \
	pbuf[i].left += ( data[sample_idx+0] * volume[0] ) >> ( x - 8 ); \
	pbuf[i].right += ( data[sample_idx+1] * volume[1] ) >> ( x - 8 ); \
	offset_frac += rate_scale; \
	sample_idx += (uint)offset_frac << 1; \
	offset_frac -= (uint)offset_frac; \
	} \
	} \

// [FWGS, 01.03.26]
S_MakeMixMono (8)
S_MakeMixMono (16)
S_MakeMixStereo (8)
S_MakeMixStereo (16)
S_MakeMixMonoPitch (8)
S_MakeMixMonoPitch (16)
S_MakeMixStereoPitch (8)
S_MakeMixStereoPitch (16)

// [FWGS, 01.03.26] removed S_TransferPaintBuffer, MIX_ActivatePaintbuffer, MIX_SetCurrentPaintbuffer,
// MIX_GetCurrentPaintbufferIndex, MIX_GetCurrentPaintbufferPtr

// [FWGS, 01.03.26]
static void S_MixAudio (portable_samplepair_t *pbuf, const int *pvol, const void *buf, int channels, int width, double offset_frac,
	double rate_scale, int num_samples)
	{
	if (Q_equal (rate_scale, 1.0))
		{
		/*for (j = 0; j < 256; j++)
			snd_scaletable[i][j] = ((signed char)j) * i * (1 << SND_SCALE_SHIFT);
		}
	}

/
===================
S_TransferPaintBuffer [FWGS, 09.05.24]
===================
/
static void S_TransferPaintBuffer (int endtime)
	{
	int		*snd_p, snd_linear_count;
	int		lpos, lpaintedtime;
	int		i, val, sampleMask;
	short	*snd_out;
	dword	*pbuf;

	pbuf = (dword *)dma.buffer;
	snd_p = (int *)g_curpaintbuffer;
	lpaintedtime = paintedtime;
	sampleMask = ((dma.samples >> 1) - 1);

	while (lpaintedtime < endtime)
		{
		// handle recirculating buffer issues
		lpos = lpaintedtime & sampleMask;

		snd_out = (short *)pbuf + (lpos << 1);

		snd_linear_count = (dma.samples >> 1) - lpos;
		if (lpaintedtime + snd_linear_count > endtime)
			snd_linear_count = endtime - lpaintedtime;

		snd_linear_count <<= 1;

		// write a linear blast of samples
		for (i = 0; i < snd_linear_count; i += 2)*/
		if (channels == 1)
			{
			/*val = (snd_p[i + 0] * 256) >> 8;

			if (val > 0x7fff)
				snd_out[i + 0] = 0x7fff;
			else if (val < (short)0x8000)
				snd_out[i + 0] = (short)0x8000;
			else
				snd_out[i + 0] = val;

			val = (snd_p[i + 1] * 256) >> 8;

			if (val > 0x7fff)
				snd_out[i + 1] = 0x7fff;
			else if (val < (short)0x8000)
				snd_out[i + 1] = (short)0x8000;
			else
				snd_out[i + 1] = val;*/
			if (width == 1)
				S_MixMono8 (pbuf, pvol, buf, num_samples);
			else
				S_MixMono16 (pbuf, pvol, buf, num_samples);
			}
		else
			{
			if (width == 1)
				S_MixStereo8 (pbuf, pvol, buf, num_samples);
			else
				S_MixStereo16 (pbuf, pvol, buf, num_samples);
			}

		/*snd_p += snd_linear_count;
		lpaintedtime += (snd_linear_count >> 1);*/
		}
	/*}

// ===============================================================================
// Mix buffer (paintbuffer) management routines
// ===============================================================================

// [FWGS, 09.05.24] Activate a paintbuffer.  All active paintbuffers are mixed in parallel within
// MIX_MixChannelsToPaintbuffer, according to flags
static void MIX_ActivatePaintbuffer (int ipaintbuffer)
	{
	Assert (ipaintbuffer < CPAINTBUFFERS);
	paintbuffers[ipaintbuffer].factive = true;
	}

// [FWGS, 09.05.24]
static void MIX_SetCurrentPaintbuffer (int ipaintbuffer)
	{
	Assert (ipaintbuffer < CPAINTBUFFERS);
	g_curpaintbuffer = paintbuffers[ipaintbuffer].pbuf;
	Assert (g_curpaintbuffer != NULL);
	}

// [FWGS, 09.05.24]
static int MIX_GetCurrentPaintbufferIndex (void)
	{
	int	i;

	for (i = 0; i < CPAINTBUFFERS; i++)*/
	else
		{
		/*if (g_curpaintbuffer == paintbuffers[i].pbuf)
			return i;*/
		if (channels == 1)
			{
			if (width == 1)
				S_MixMonoPitch8 (pbuf, pvol, buf, offset_frac, rate_scale, num_samples);
			else
				S_MixMonoPitch16 (pbuf, pvol, buf, offset_frac, rate_scale, num_samples);
			}
		else
			{
			if (width == 1)
				S_MixStereoPitch8 (pbuf, pvol, buf, offset_frac, rate_scale, num_samples);
			else
				S_MixStereoPitch16 (pbuf, pvol, buf, offset_frac, rate_scale, num_samples);
			}
		}

	/*return 0;
	}

// [FWGS, 09.05.24]
static paintbuffer_t *MIX_GetCurrentPaintbufferPtr (void)
	{
	int	ipaint = MIX_GetCurrentPaintbufferIndex ();

	Assert (ipaint < CPAINTBUFFERS);
	return &paintbuffers[ipaint];*/
	}

// [FWGS, 01.03.26] removed MIX_DeactivateAllPaintbuffers, MIX_ResetPaintbufferFilterCounters,
// MIX_GetPFrontFromIPaint, MIX_GetPPaintFromIPaint, MIX_FreeAllPaintbuffers,
// MIX_InitAllPaintbuffers, S_PaintMonoFrom8, S_PaintStereoFrom8, S_PaintMonoFrom16

// [FWGS, 15.04.26]
static int S_AdjustNumSamples (channel_t *chan, int num_samples, double rate, double timecompress_rate)
	{
	/*if (chan->finished)*/
	if (FBitSet (chan->flags, FL_CHAN_FINISHED))
		return 0;

	// if channel is set to end at specific sample,
	// detect if it's the last mixing pass and truncate
	if (chan->forced_end)
		{
		/*data = pData[i];
		pbuf[i].left += lscale[data];
		pbuf[i].right += rscale[data];
		}
	}

static void S_PaintStereoFrom8 (portable_samplepair_t *pbuf, int *volume, byte *pData, int outCount)
	{
	int		*lscale, *rscale;
	uint	left, right;
	word	*data;
	int		i;

	lscale = snd_scaletable[volume[0] >> SND_SCALE_SHIFT];
	rscale = snd_scaletable[volume[1] >> SND_SCALE_SHIFT];
	data = (word *)pData;*/
		// calculate the last sample position
		double end_sample = chan->sample + rate * num_samples * timecompress_rate;

		/*for (i = 0; i < outCount; i++, data++)
		{
		left = (byte)((*data & 0x00FF));
		right = (byte)((*data & 0xFF00) >> 8);
		pbuf[i].left += lscale[left];
		pbuf[i].right += rscale[right];*/
		if (end_sample >= chan->forced_end)
			{
			/*chan->finished = true;*/
			SetBits (chan->flags, FL_CHAN_FINISHED);
			return floor ((chan->forced_end - chan->sample) / (rate * timecompress_rate));
			}
		}

	/*}

	static void S_PaintMonoFrom16 (portable_samplepair_t *pbuf, int *volume, short *pData, int outCount)
	{
	int	left, right;
	int	i, data;

	for (i = 0; i < outCount; i++)
		{
		data = pData[i];
		left = (data * volume[0]) >> 8;
		right = (data * volume[1]) >> 8;
		pbuf[i].left += left;
		pbuf[i].right += right;
		}*/
	return num_samples;
	}

// [FWGS, 01.03.26] removed S_PaintStereoFrom16, S_Mix8MonoTimeCompress, S_Mix8Mono, S_Mix8Stereo, S_Mix16Mono,
// S_Mix16Stereo

// [FWGS, 15.04.26]
static int S_MixChannelToBuffer (portable_samplepair_t *pbuf, channel_t *chan, int num_samples, int out_rate,
	double pitch, int offset, int timecompress)
	{
	/*uint	*data;
	int		left, right;
	int		i;

	data = (uint *)pData;

	for (i = 0; i < outCount; i++, data++)*/
	const int initial_offset = offset;
	const int pvol[2] =
		{
		/*left = (signed short)((*data & 0x0000FFFF));
		right = (signed short)((*data & 0xFFFF0000) >> 16);

		left = (left * volume[0]) >> 8;
		right = (right * volume[1]) >> 8;

		pbuf[i].left += left;
		pbuf[i].right += right;
		}
	}

static void S_Mix8MonoTimeCompress (portable_samplepair_t *pbuf, int *volume, byte *pData, int inputOffset,
	uint rateScale, int outCount, int timecompress)
	{
	}

static void S_Mix8Mono (portable_samplepair_t *pbuf, int *volume, byte *pData, int inputOffset,
	uint rateScale, int outCount, int timecompress)
	{
	int		i, sampleIndex = 0;
	uint	sampleFrac = inputOffset;
	int		*lscale, *rscale;*/
		bound (0, chan->leftvol, 255),
		bound (0, chan->rightvol, 255),
		};
	double rate = pitch * chan->sfx->cache->rate / (double)out_rate;

	/*if (timecompress != 0)*/
	// timecompress at 100% is skipping the entire sfx, so mark as finished and exit
	if (timecompress >= 100)
		{
		/*S_Mix8MonoTimeCompress (pbuf, volume, pData, inputOffset, rateScale, outCount, timecompress);
		chan->finished = true;*/
		SetBits (chan->flags, FL_CHAN_FINISHED);
		return 0;
		}

	/*// Not using pitch shift?
	if (rateScale == FIX (1))
		{
		S_PaintMonoFrom8 (pbuf, volume, pData, outCount);
		return;
		}*/
	double timecompress_rate = 1 / (1 - timecompress / 100.0);

	/*lscale = snd_scaletable[volume[0] >> SND_SCALE_SHIFT];
	rscale = snd_scaletable[volume[1] >> SND_SCALE_SHIFT];*/
	num_samples = S_AdjustNumSamples (chan, num_samples, rate, timecompress_rate);
	if (num_samples == 0)
		return 0;

	/*for (i = 0; i < outCount; i++)*/
	while (num_samples > 0)
		{
		/*pbuf[i].left += lscale[pData[sampleIndex]];
		pbuf[i].right += rscale[pData[sampleIndex]];
		sampleFrac += rateScale;
		sampleIndex += FIX_INTPART (sampleFrac);
		sampleFrac = FIX_FRACPART (sampleFrac);
		}
		}*/
		// calculate the last sample position
		double end_sample = chan->sample + rate * num_samples * timecompress_rate;

		/*static void S_Mix8Stereo (portable_samplepair_t *pbuf, int *volume, byte *pData, int inputOffset,
		uint rateScale, int outCount)
		{
		int		i, sampleIndex = 0;
		uint	sampleFrac = inputOffset;
		int		*lscale, *rscale;*/
		// and get total amount of samples we want
		int request_num_samples = (int)(ceil (end_sample) - floor (chan->sample));

		/*// Not using pitch shift?
		if (rateScale == FIX (1))
		{
		S_PaintStereoFrom8 (pbuf, volume, pData, outCount);
		return;
		}*/
		// get sample pointer and also amount of samples available
		const void *audio = NULL;
		/*int available = S_RetrieveAudioSamples (chan->sfx->cache, &audio, chan->sample, request_num_samples, chan->use_loop);*/
		int available = S_RetrieveAudioSamples (chan->sfx->cache, &audio, chan->sample, request_num_samples,
			FBitSet (chan->flags, FL_CHAN_USE_LOOP));

		/*lscale = snd_scaletable[volume[0] >> SND_SCALE_SHIFT];
		rscale = snd_scaletable[volume[1] >> SND_SCALE_SHIFT];*/
		// no samples available, exit
		if (!available)
			break;

		/*for (i = 0; i < outCount; i++)
		{
		pbuf[i].left += lscale[pData[sampleIndex + 0]];
		pbuf[i].right += rscale[pData[sampleIndex + 1]];
		sampleFrac += rateScale;
		sampleIndex += FIX_INTPART (sampleFrac) << 1;
		sampleFrac = FIX_FRACPART (sampleFrac);
		}
		}*/
		double sample_frac = chan->sample - floor (chan->sample);

		/*static void S_Mix16Mono (portable_samplepair_t *pbuf, int *volume, short *pData, int inputOffset,
		uint rateScale, int outCount)
		{
		int		i, sampleIndex = 0;
		uint	sampleFrac = inputOffset;*/
		// this is how much data we output
		int out_count = num_samples;
		if (request_num_samples > available) // but we can't write more than we have
			out_count = (int)ceil ((available - sample_frac) / (rate));

		/*// Not using pitch shift?
		if (rateScale == FIX (1))
		{
		S_PaintMonoFrom16 (pbuf, volume, pData, outCount);
		return;
		}*/
		const wavdata_t *wav = chan->sfx->cache;
		S_MixAudio (pbuf + offset, pvol, audio, wav->channels, wav->width, sample_frac, rate, out_count);

		/*for (i = 0; i < outCount; i++)
		{
		pbuf[i].left += (volume[0] * (int)(pData[sampleIndex])) >> 8;
		pbuf[i].right += (volume[1] * (int)(pData[sampleIndex])) >> 8;
		sampleFrac += rateScale;
		sampleIndex += FIX_INTPART (sampleFrac);
		sampleFrac = FIX_FRACPART (sampleFrac);*/
		chan->sample += out_count * rate * timecompress_rate;
		offset += out_count;
		num_samples -= out_count;
		}
	/*}

	static void S_Mix16Stereo (portable_samplepair_t *pbuf, int *volume, short *pData, int inputOffset,
	uint rateScale, int outCount)
	{
	int		i, sampleIndex = 0;
	uint	sampleFrac = inputOffset;*/

	/*// Not using pitch shift?
	if (rateScale == FIX (1))
		{
		S_PaintStereoFrom16 (pbuf, volume, pData, outCount);
		return;
		}*/
	// samples couldn't be retrieved, mark as finished
	if (num_samples > 0)
		/*chan->finished = true;*/
		SetBits (chan->flags, FL_CHAN_FINISHED);

	/*for (i = 0; i < outCount; i++)
		{
		pbuf[i].left += (volume[0] * (int)(pData[sampleIndex + 0])) >> 8;
		pbuf[i].right += (volume[1] * (int)(pData[sampleIndex + 1])) >> 8;
		sampleFrac += rateScale;
		sampleIndex += FIX_INTPART (sampleFrac) << 1;
		sampleFrac = FIX_FRACPART (sampleFrac);
		}*/
	// total amount of samples mixed
	return offset - initial_offset;
	}

// [FWGS, 01.03.26] removed S_MixChannel, S_MixDataToDevice, S_ShouldContinueMixing

// [FWGS, 15.04.26]
static int VOX_MixChannelToBuffer (portable_samplepair_t *pbuf, channel_t *chan, int num_samples, int out_rate, double pitch)
	{
	int offset = 0;

	/*if (chan->sentence_finished)*/
	if (FBitSet (chan->flags, FL_CHAN_SENTENCE_FINISHED))
		return 0;

	/*while ((num_samples > 0) && !chan->sentence_finished)*/
	while ((num_samples > 0) && !FBitSet (chan->flags, FL_CHAN_SENTENCE_FINISHED))
		{
		int outputCount = S_MixChannelToBuffer (pbuf, chan, num_samples, out_rate, pitch, offset,
			chan->words[chan->word_index].timecompress);

		/*availableSamples = S_GetOutputData (pSource, &pData, pChannel->pMixer.sample, inputSampleCount, use_loop);*/
		offset += outputCount;
		num_samples -= outputCount;

		// if we finished load a next word
		/*if (chan->finished)*/
		if (FBitSet (chan->flags, FL_CHAN_FINISHED))
			{
			VOX_FreeWord (chan);
			chan->word_index++;
			VOX_LoadWord (chan);

			/*if (!chan->sentence_finished)*/
			if (!FBitSet (chan->flags, FL_CHAN_SENTENCE_FINISHED))
				chan->sfx = chan->words[chan->word_index].sfx;
			}
		}

	/*return !ch->pMixer.finished;*/
	return offset;
	}

// [FWGS, 01.03.26] removed MIX_MixChannelsToPaintbuffer, S_GetNextpFilter, S_Interpolate2xCubic,
// S_Interpolate2xLinear, S_MixBufferUpsample2x, 

// [FWGS, 15.04.26]
static int S_MixNormalChannels (portable_samplepair_t *dst, int end, int rate)
	{
	const qboolean	local = Host_IsLocalGame ();
	const qboolean	ingame = CL_IsInGame ();
	/*const int		num_samples = (end - paintedtime) / (SOUND_DMA_SPEED / rate);*/
	const int		num_samples = (end - snd.paintedtime) / (SOUND_DMA_SPEED / rate);

	// FWGS feature: make everybody sound like chipmunks when we're going fast
	const float		pitch_mult = (sys_timescale.value + 1) / 2;
	int				num_mixed_channels = 0;

	if (num_samples <= 0)
		return num_mixed_channels;

	/*if (sampleCount <= 0) return;*/
	// no sounds in console with background map
	if (cl.background && (cls.key_dest == key_console))
		return num_mixed_channels;

	/*for (int i = 0; i < total_channels; i++)*/
	for (int i = 0; i < snd.total_channels; i++)
		{
		/*channel_t *ch = &channels[i];*/
		channel_t *ch = &snd.channels[i];
		if (!ch->sfx)
			continue;

		if (!cl.background)
			{
			/*if ((cls.key_dest == key_console) && ch->localsound)*/
			if ((cls.key_dest == key_console) && FBitSet (ch->flags, FL_CHAN_LOCAL_SOUND))
				{
				// play, playvol
				}
			/*else if (((cls.key_dest == key_menu) || cl.paused) && !ch->localsound && local)*/
			else if (((cls.key_dest == key_menu) || cl.paused) && !FBitSet (ch->flags, FL_CHAN_LOCAL_SOUND) && local)
				{
				// play only local sounds, keep pause for other
				continue;
				}
			/*else if ((cls.key_dest != key_menu) && !ingame && !ch->staticsound)*/
			else if ((cls.key_dest != key_menu) && !ingame && !FBitSet (ch->flags, FL_CHAN_STATIC_SOUND))
				{
				// play only ambient sounds, keep pause for other
				continue;
				}
			}

		wavdata_t *sc = S_LoadSound (ch->sfx);
		if (!sc)
			{
			S_FreeChannel (ch);
			continue;
			}

		// if the sound is unaudible, skip it
		// if it's also not looping, free it
		if ((ch->leftvol < 8) && (ch->rightvol < 8))
			{
			/*if (!FBitSet (sc->flags, SOUND_LOOPED) || !ch->use_loop)*/
			if (!FBitSet (sc->flags, SOUND_LOOPED) || !FBitSet (ch->flags, FL_CHAN_USE_LOOP))
				{
				// fix for random skipping of map startup sounds
				if (ch->inauduble_free_time == 0.0f)
					ch->inauduble_free_time = host.realtime + MAX_CHANNEL_INAUDIBLE_TIME;
				else if (ch->inauduble_free_time > host.realtime)
					S_FreeChannel (ch);
				}

			continue;
			}

		ch->inauduble_free_time = 0.0f;

		if (rate != sc->rate)
			continue;

		if ((ch->entchannel == CHAN_VOICE) || (ch->entchannel == CHAN_STREAM))
			{
			cl_entity_t *ent = CL_GetEntityByIndex (ch->entnum);

			if (ent != NULL)
				{
				if (sc->width == 1)
					/*SND_MoveMouth8 (&ent->mouth, ch->sample, sc, num_samples, ch->use_loop);*/
					SND_MoveMouth8 (&ent->mouth, ch->sample, sc, num_samples, FBitSet (ch->flags, FL_CHAN_USE_LOOP));
				else
					/*SND_MoveMouth16 (&ent->mouth, ch->sample, sc, num_samples, ch->use_loop);*/
					SND_MoveMouth16 (&ent->mouth, ch->sample, sc, num_samples, FBitSet (ch->flags, FL_CHAN_USE_LOOP));
				}
			}

		double pitch = VOX_ModifyPitch (ch, ch->basePitch * 0.01) * pitch_mult;
		num_mixed_channels++;

		/*if (ch->is_sentence)*/
		if (ch->words)
			{
			VOX_MixChannelToBuffer (dst, ch, num_samples, rate, pitch);

			/*if (ch->sentence_finished)*/
			if (FBitSet (ch->flags, FL_CHAN_SENTENCE_FINISHED))
				S_FreeChannel (ch);
			}
		else
			{
			S_MixChannelToBuffer (dst, ch, num_samples, rate, pitch, 0, 0);

			/*if (ch->finished)*/
			if (FBitSet (ch->flags, FL_CHAN_FINISHED))
				S_FreeChannel (ch);
			}
		}

	return num_mixed_channels;
	}

// [FWGS, 01.03.26] removed MIX_ClearAllPaintBuffers
 
// [FWGS, 01.03.26]
static int S_AverageSample (int a, int b)
	{
	return (a >> 1) + (b >> 1) + (((a & 1) + (b & 1)) >> 1);
	}

// [FWGS, 01.03.26] removed MIX_MixPaintbuffers

// [FWGS, 01.03.26]
static void S_UpsampleBuffer (portable_samplepair_t *dst, size_t num_samples)
	{
	if (s_lerping.value)
		{
		// copy even positions and average odd
		for (size_t i = num_samples - 1; i > 0; i--)
			{
			dst[i * 2] = dst[i];

			dst[i * 2 + 1].left = S_AverageSample (dst[i].left, dst[i - 1].left);
			dst[i * 2 + 1].right = S_AverageSample (dst[i].right, dst[i - 1].right);
			}
		}
	else
		{
		// copy into even and odd positions
		for (size_t i = num_samples - 1; i > 0; i--)
			{
			dst[i * 2] = dst[i];
			dst[i * 2 + 1] = dst[i];
			}
		}

	dst[1] = dst[0];
	}

// [FWGS, 01.03.26] removed MIX_CompressPaintbuffer, S_MixUpsample

// [FWGS, 15.04.26]
static int S_MixNormalChannelsToRoombuffer (int end, int count)
	{
	// for room buffer we only support CD rates like 11k, 22k, and 44k
	// TODO: 48k output would require support from platform-specific backends first
	// until there is no real usecase, let's keep it simple
	int num_mixed_channels = S_MixNormalChannels (roombuffer, end, SOUND_11k);

	/*if (dma.format.speed >= SOUND_22k)*/
	if (snd.format.speed >= SOUND_22k)
		{
		if (num_mixed_channels > 0)
			S_UpsampleBuffer (roombuffer, count / (SOUND_22k / SOUND_11k));

		num_mixed_channels += S_MixNormalChannels (roombuffer, end, SOUND_22k);
		}

	/*if (dma.format.speed >= SOUND_44k)*/
	if (snd.format.speed >= SOUND_44k)
		{
		if (num_mixed_channels > 0)
			S_UpsampleBuffer (roombuffer, count / (SOUND_44k / SOUND_22k));

		num_mixed_channels += S_MixNormalChannels (roombuffer, end, SOUND_44k);
		}

	return num_mixed_channels;
	}

// [FWGS, 01.03.26] removed MIX_MixRawSamplesBuffer

// [FWGS, 01.03.26]
static int S_MixRawChannels (int end)
	{
	int num_room_channels = 0;

	if (cl.paused)
		return 0;

	// paint in the raw channels
	/*for (size_t i = 0; i < HLARRAYSIZE (raw_channels); i++)*/
	for (size_t i = 0; i < snd.max_raw_channels; i++)
		{
		// copy from the streaming sound source
		/*rawchan_t *ch = raw_channels[i];*/
		rawchan_t *ch = snd.raw_channels[i];

		// background track should be mixing into another buffer
		if (!ch)
			continue;

		// not audible
		if (!ch->leftvol && !ch->rightvol)
			continue;

		qboolean is_voice = CL_IsPlayerIndex (ch->entnum) ||
			(ch->entnum == VOICE_LOOPBACK_INDEX) ||
			(ch->entnum == VOICE_LOCALCLIENT_INDEX);

		portable_samplepair_t *pbuf;
		if (is_voice || (ch->entnum == S_RAW_SOUND_BACKGROUNDTRACK))
			{
			// for streams we don't have fancy things like volume controls
			// or DSP processing or upsampling, so paint it directly into result buffer
			pbuf = paintbuffer;
			}
		else
			{
			pbuf = roombuffer;
			num_room_channels++;
			}

		uint stop = (end < ch->s_rawend) ? end : ch->s_rawend;
		const uint mask = ch->max_samples - 1;

		/*for (size_t i = 0, j = paintedtime; j < stop; i++, j++)*/
		for (size_t i = 0, j = snd.paintedtime; j < stop; i++, j++)
			{
			pbuf[i].left += (ch->rawsamples[j & mask].left * ch->leftvol) >> 8;
			pbuf[i].right += (ch->rawsamples[j & mask].right * ch->rightvol) >> 8;
			}

		if (ch->entnum > 0)
			{
			cl_entity_t *ent = CL_GetEntityByIndex (ch->entnum);
			/*int pos = paintedtime & (ch->max_samples - 1);
			int count = bound (0, ch->max_samples - pos, stop - paintedtime);*/
			int pos = snd.paintedtime & (ch->max_samples - 1);
			int count = bound (0, ch->max_samples - pos, stop - snd.paintedtime);

			if (ent)
				SND_MoveMouthRaw (&ent->mouth, &ch->rawsamples[pos], count);
			}
		}

	return num_room_channels;
	}

// [FWGS, 01.03.26] removed MIX_UpsampleAllPaintbuffers

// [FWGS, 01.03.26]
static void S_MixBufferWithGain (portable_samplepair_t *dst, const portable_samplepair_t *src, size_t count, int gain)
	{
	if (gain == 256)
		{
		for (size_t i = 0; i < count; i++)
			{
			dst[i].left += src[i].left;
			dst[i].right += src[i].right;
			}
		}
	else
		{
		for (size_t i = 0; i < count; i++)
			{
			dst[i].left += (src[i].left * gain) >> 8;
			dst[i].right += (src[i].right * gain) >> 8;
			}
		}
	}

// [FWGS, 01.03.26]
static void S_WriteLinearBlastStereo16 (short *snd_out, const int *snd_p, size_t count)
	{
	for (size_t i = 0; i < count; i += 2)
		{
		snd_out[i + 0] = CLIP16 (snd_p[i + 0]);
		snd_out[i + 1] = CLIP16 (snd_p[i + 1]);
		}
	}

// [FWGS, 15.04.26]
static void S_TransferPaintBuffer (const portable_samplepair_t *src, int endtime)
	{
	const int	*snd_p = (const int *)src;
	/*const int	sampleMask = ((dma.samples >> 1) - 1);
	int			lpaintedtime = paintedtime;*/
	const int	sampleMask = ((snd.samples >> 1) - 1);
	int			lpaintedtime = snd.paintedtime;

	SNDDMA_BeginPainting ();

	while (lpaintedtime < endtime)
		{
		// handle recirculating buffer issues
		int		lpos = lpaintedtime & sampleMask;
		/*short	*snd_out = (short *)dma.buffer + (lpos << 1);*/
		short	*snd_out = (short *)snd.buffer + (lpos << 1);
		/*int		snd_linear_count = (dma.samples >> 1) - lpos;*/
		int snd_linear_count = (snd.samples >> 1) - lpos;

		if (lpaintedtime + snd_linear_count > endtime)
			snd_linear_count = endtime - lpaintedtime;

		snd_linear_count <<= 1;

		// write a linear blast of samples
		S_WriteLinearBlastStereo16 (snd_out, snd_p, snd_linear_count);

		snd_p += snd_linear_count;
		lpaintedtime += (snd_linear_count >> 1);
		}

	SNDDMA_Submit ();
	}

// [FWGS, 01.03.26] removed MIX_PaintChannels

// [FWGS, 01.03.26]
void S_ClearBuffers (int num_samples)
	{
	const size_t num_bytes = (num_samples + 1) * sizeof (portable_samplepair_t);

	memset (roombuffer, 0, num_bytes);
	memset (paintbuffer, 0, num_bytes);
	}

// [FWGS, 15.04.26]
void S_PaintChannels (int endtime)
	{
	int gain = S_GetMasterVolume () * 256;

	/*while (paintedtime < endtime)*/
	while (snd.paintedtime < endtime)
		{
		// if paintbuffer is smaller than DMA buffer
		int end = endtime;
		/*if (end - paintedtime > PAINTBUFFER_SIZE)
			end = paintedtime + PAINTBUFFER_SIZE;*/
		if (end - snd.paintedtime > PAINTBUFFER_SIZE)
			end = snd.paintedtime + PAINTBUFFER_SIZE;

		/*const int num_samples = end - paintedtime;*/
		const int num_samples = end - snd.paintedtime;

		S_ClearBuffers (num_samples);

		int room_channels = S_MixNormalChannelsToRoombuffer (end, num_samples);
		room_channels += S_MixRawChannels (end);

		// now process DSP and mix result into paintbuffer
		if (cls.key_dest != key_menu)
			SX_RoomFX (roombuffer, num_samples);

		if ((room_channels > 0) || (cls.key_dest != key_menu))
			S_MixBufferWithGain (paintbuffer, roombuffer, num_samples, gain);

		// transfer out according to DMA format
		S_TransferPaintBuffer (paintbuffer, end);
		/*paintedtime = end;*/
		snd.paintedtime = end;
		}
	}
