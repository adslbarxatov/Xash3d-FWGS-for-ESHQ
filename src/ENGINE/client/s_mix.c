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

// [FWGS, 01.03.26]
/*// [FWGS, 09.05.24] Don't mix into any paintbuffers
static void MIX_DeactivateAllPaintbuffers (void)*/
static int S_AdjustNumSamples (channel_t *chan, int num_samples, double rate, double timecompress_rate)
	{
	/*int	i;

	for (i = 0; i < CPAINTBUFFERS; i++)
		paintbuffers[i].factive = false;
	}

ifndef DISABLE_UPSAMPLING
// [FWGS: 01.11.25] set upsampling filter indexes back to 0
static void MIX_ResetPaintbufferFilterCounters (void)
	{
	int	i;

	for (i = 0; i < CPAINTBUFFERS; i++)
		paintbuffers[i].ifilter = FILTERTYPE_NONE;
	}
endif

// [FWGS, 09.05.24] return pointer to front paintbuffer pbuf, given index
static portable_samplepair_t *MIX_GetPFrontFromIPaint (int ipaintbuffer)
	{
	Assert (ipaintbuffer < CPAINTBUFFERS);
	return paintbuffers[ipaintbuffer].pbuf;
	}

// [FWGS, 09.05.24]
static paintbuffer_t *MIX_GetPPaintFromIPaint (int ipaint)
	{
	Assert (ipaint < CPAINTBUFFERS);
	return &paintbuffers[ipaint];
	}

void MIX_FreeAllPaintbuffers (void)
	{
	// clear paintbuffer structs
	memset (paintbuffers, 0, CPAINTBUFFERS * sizeof (paintbuffer_t));
	}

// [FWGS, 01.07.25] Initialize paintbuffers array, set current paint buffer to main output buffer IPAINTBUFFER
void MIX_InitAllPaintbuffers (void)
	{
	// clear paintbuffer structs
	memset (paintbuffers, 0, CPAINTBUFFERS * sizeof (paintbuffer_t));

	paintbuffers[IPAINTBUFFER].pbuf = paintbuffer;
	paintbuffers[IROOMBUFFER].pbuf = roombuffer;
	paintbuffers[ISTREAMBUFFER].pbuf = streambuffer;
	paintbuffers[IVOICEBUFFER].pbuf = voicebuffer;

	MIX_SetCurrentPaintbuffer (IPAINTBUFFER);
	}

/
===============================================================================
CHANNEL MIXING
===============================================================================
/
static void S_PaintMonoFrom8 (portable_samplepair_t *pbuf, int *volume, byte *pData, int outCount)
	{
	int	*lscale, *rscale;
	int	i, data;

	lscale = snd_scaletable[volume[0] >> SND_SCALE_SHIFT];
	rscale = snd_scaletable[volume[1] >> SND_SCALE_SHIFT];*/
	if (chan->finished)
		return 0;

	/*for (i = 0; i < outCount; i++)*/
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
			chan->finished = true;
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

// [FWGS, 01.03.26]
/*static void S_PaintStereoFrom16 (portable_samplepair_t *pbuf, int *volume, short *pData, int outCount)*/
static int S_MixChannelToBuffer (portable_samplepair_t *pbuf, channel_t *chan, int num_samples, int out_rate, double pitch, int offset, int timecompress)
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
		/*S_Mix8MonoTimeCompress (pbuf, volume, pData, inputOffset, rateScale, outCount, timecompress);*/
		chan->finished = true;
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
		int available = S_RetrieveAudioSamples (chan->sfx->cache, &audio, chan->sample, request_num_samples, chan->use_loop);

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
		chan->finished = true;

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

// [FWGS, 01.03.26]
/*static void S_MixChannel (channel_t *pChannel, void *pData, int outputOffset, int inputOffset, uint fracRate,
	int outCount, int timecompress)*/
static int VOX_MixChannelToBuffer (portable_samplepair_t *pbuf, channel_t *chan, int num_samples, int out_rate, double pitch)
	{
	/*int				pvol[CCHANVOLUMES];
	paintbuffer_t	*ppaint = MIX_GetCurrentPaintbufferPtr ();
	wavdata_t		*pSource = pChannel->sfx->cache;
	portable_samplepair_t	*pbuf;

	Assert (pSource != NULL);

	pvol[0] = bound (0, pChannel->leftvol, 255);
	pvol[1] = bound (0, pChannel->rightvol, 255);
	pbuf = ppaint->pbuf + outputOffset;

	if (pSource->channels == 1)
		{
		if (pSource->width == 1)
			S_Mix8Mono (pbuf, pvol, pData, inputOffset, fracRate, outCount, timecompress);
		else 
			S_Mix16Mono (pbuf, pvol, (short *)pData, inputOffset, fracRate, outCount);
		}
	else
		{
		if (pSource->width == 1)
			S_Mix8Stereo (pbuf, pvol, pData, inputOffset, fracRate, outCount);
		else 
			S_Mix16Stereo (pbuf, pvol, (short *)pData, inputOffset, fracRate, outCount);
		}
	}*/
	int offset = 0;

	/*int S_MixDataToDevice (channel_t *pChannel, int sampleCount, int outRate, int outOffset, int timeCompress)
	{
	// save this to compute total output
	int	startingOffset = outOffset;
	float	inputRate = (pChannel->pitch * pChannel->sfx->cache->rate);
	float	rate = inputRate / outRate;

	// shouldn't be playing this if finished, but return if we are
	if (pChannel->pMixer.finished)*/
	if (chan->sentence_finished)
		return 0;

	/*// If we are terminating this wave prematurely, then make sure we detect the limit
	if (pChannel->pMixer.forcedEndSample)*/
	while ((num_samples > 0) && !chan->sentence_finished)
		{
		/*// how many total input samples will we need?
		int	samplesRequired = (int)(sampleCount * rate);

		// will this hit the end?
		if (pChannel->pMixer.sample + samplesRequired >= pChannel->pMixer.forcedEndSample)
			{
			// yes, mark finished and truncate the sample request
			pChannel->pMixer.finished = true;
			sampleCount = (int)((pChannel->pMixer.forcedEndSample - pChannel->pMixer.sample) / rate);
			}
		}

	while (sampleCount > 0)
		{
		int	availableSamples, outSampleCount;
		wavdata_t *pSource = pChannel->sfx->cache;
		qboolean	use_loop = pChannel->use_loop;
		void *pData = NULL;
		double	sampleFrac;
		int	i, j;

		// compute number of input samples required
		double	end = pChannel->pMixer.sample + rate * sampleCount;
		int	inputSampleCount = (int)(ceil (end) - floor (pChannel->pMixer.sample));*/
		int outputCount = S_MixChannelToBuffer (pbuf, chan, num_samples, out_rate, pitch, offset,
			chan->words[chan->word_index].timecompress);

		/*availableSamples = S_GetOutputData (pSource, &pData, pChannel->pMixer.sample, inputSampleCount, use_loop);*/
		offset += outputCount;
		num_samples -= outputCount;

		/*// none available, bail out
		if (!availableSamples) break;

		sampleFrac = pChannel->pMixer.sample - floor (pChannel->pMixer.sample);

		if (availableSamples < inputSampleCount)
			{
			// how many samples are there given the number of input samples and the rate
			outSampleCount = (int)ceil ((availableSamples - sampleFrac) / rate);
			}
		else*/
		// if we finished load a next word
		if (chan->finished)
			{
			/*outSampleCount = sampleCount;
			}

			// Verify that we won't get a buffer overrun
			Assert (floor (sampleFrac + rate * (outSampleCount - 1)) <= availableSamples);

			// save current paintbuffer
			j = MIX_GetCurrentPaintbufferIndex ();*/
			VOX_FreeWord (chan);
			chan->word_index++;
			VOX_LoadWord (chan);

			/*for (i = 0; i < CPAINTBUFFERS; i++)
			{
			if (!paintbuffers[i].factive)
				continue;

			// mix chan into all active paintbuffers
			MIX_SetCurrentPaintbuffer (i);

			S_MixChannel (pChannel, pData, outOffset, FIX_FLOAT (sampleFrac), FIX_FLOAT (rate),
				outSampleCount, timeCompress);*/
			if (!chan->sentence_finished)
				chan->sfx = chan->words[chan->word_index].sfx;
			}

		/*MIX_SetCurrentPaintbuffer (j);

		pChannel->pMixer.sample += outSampleCount * rate;
		outOffset += outSampleCount;
		sampleCount -= outSampleCount;
		}

	// [ESHQ: brackets] did we run out of samples? if so, mark finished
	if (sampleCount > 0)
		pChannel->pMixer.finished = true;

	// total number of samples mixed !! at the output clock rate !!
	return outOffset - startingOffset;
	}

static qboolean S_ShouldContinueMixing (channel_t *ch)
	{
	if (ch->isSentence)
		{
		if (ch->currentWord)
			return true;
		return false;*/
		}

	/*return !ch->pMixer.finished;*/
	return offset;
	}

// [FWGS, 01.03.26] removed MIX_MixChannelsToPaintbuffer, S_GetNextpFilter, S_Interpolate2xCubic,
// S_Interpolate2xLinear, S_MixBufferUpsample2x, 

// [FWGS, 01.03.26]
/*// [FWGS, 09.05.24] Mix all channels into active paintbuffers until paintbuffer is full or 'endtime' is reached.
// 
// endtime: time in 44khz samples to mix
// rate: ignore samples which are not natively at this rate (for multipass mixing/filtering)
//   if rate == SOUND_ALL_RATES then mix all samples this pass
// flags: if SOUND_MIX_DRY, then mix only samples with channel flagged as 'dry'
// outputRate: target mix rate for all samples. Note, if outputRate = SOUND_DMA_SPEED, then
//   this routine will fill the paintbuffer to endtime.  Otherwise, fewer samples are mixed.
//   if( endtime - paintedtime ) is not aligned on boundaries of 4,
//   we'll miss data if outputRate < SOUND_DMA_SPEED!
static void MIX_MixChannelsToPaintbuffer (int endtime, int rate, int outputRate)*/
static int S_MixNormalChannels (portable_samplepair_t *dst, int end, int rate)
	{
	/*channel_t	*ch;
	wavdata_t	*pSource;
	int			i, sampleCount;
	qboolean	bZeroVolume;
	qboolean	local = Host_IsLocalGame ();

	// mix each channel into paintbuffer
	ch = channels;*/
	const qboolean	local = Host_IsLocalGame ();
	const qboolean	ingame = CL_IsInGame ();
	const int		num_samples = (end - paintedtime) / (SOUND_DMA_SPEED / rate);

	/*// validate parameters
	Assert (outputRate <= SOUND_DMA_SPEED);*/
	// FWGS feature: make everybody sound like chipmunks when we're going fast
	const float		pitch_mult = (sys_timescale.value + 1) / 2;
	/*// make sure we're not discarding data
	Assert (!((endtime - paintedtime) & 0x3) || (outputRate == SOUND_DMA_SPEED));*/
	int				num_mixed_channels = 0;

	/*// 44k: try to mix this many samples at outputRate
	sampleCount = (endtime - paintedtime) / (SOUND_DMA_SPEED / outputRate);*/
	if (num_samples <= 0)
		return num_mixed_channels;

	/*if (sampleCount <= 0) return;*/
	// no sounds in console with background map
	if (cl.background && (cls.key_dest == key_console))
		return num_mixed_channels;

	/*for (i = 0; i < total_channels; i++, ch++)*/
	for (int i = 0; i < total_channels; i++)
		{
		/*if (!ch->sfx) continue;*/
		channel_t *ch = &channels[i];
		if (!ch->sfx)
			continue;

		/*// NOTE: background map is allow both type sounds: menu and game*/
		if (!cl.background)
			{
			if ((cls.key_dest == key_console) && ch->localsound)
				{
				// play, playvol
				}
			/*else if ((s_listener.inmenu || s_listener.paused) && !ch->localsound && local)*/
			else if (((cls.key_dest == key_menu) || cl.paused) && !ch->localsound && local)
				{
				// play only local sounds, keep pause for other
				continue;
				}
			/*else if (!s_listener.inmenu && !s_listener.active && !ch->staticsound)*/
			else if ((cls.key_dest != key_menu) && !ingame && !ch->staticsound)
				{
				// play only ambient sounds, keep pause for other
				continue;
				}
			}
		/*else if (cls.key_dest == key_console)
			{
			continue;	// silent mode in console
			}

		pSource = S_LoadSound (ch->sfx);

		// Don't mix sound data for sounds with zero volume. If it's a non-looping sound,
		// just remove the sound when its volume goes to zero*/

		/*bZeroVolume = !ch->leftvol && !ch->rightvol;*/
		wavdata_t *sc = S_LoadSound (ch->sfx);
		/*if (!bZeroVolume)*/
		if (!sc)
			{
			/*// this values matched with GoldSrc
			if ((ch->leftvol < 8) && (ch->rightvol < 8))
				bZeroVolume = true;*/
			S_FreeChannel (ch);
			continue;
			}

		/*if (!pSource || (bZeroVolume && !FBitSet (pSource->flags, SOUND_LOOPED)))*/
		// if the sound is unaudible, skip it
		// if it's also not looping, free it
		if ((ch->leftvol < 8) && (ch->rightvol < 8))
			{
			/*if (!pSource)
				{*/
			if (!FBitSet (sc->flags, SOUND_LOOPED) || !ch->use_loop)
				/*S_FreeChannel (ch);*/
				{
				// [FWGS, 05.04.26] fix for random skipping of map startup sounds
				if (ch->inauduble_free_time == 0.0f)
					ch->inauduble_free_time = host.realtime + MAX_CHANNEL_INAUDIBLE_TIME;
				else if (ch->inauduble_free_time > host.realtime)
					S_FreeChannel (ch);
				}
				/*continue;
				}
				}
				else if (bZeroVolume)
				{*/

			continue;
			}

		ch->inauduble_free_time = 0.0f;

		/*// multipass mixing - only mix samples of specified sample rate
		switch (rate)*/
		if (rate != sc->rate)
			continue;

		if ((ch->entchannel == CHAN_VOICE) || (ch->entchannel == CHAN_STREAM))
			{
			/*case SOUND_11k:
			case SOUND_22k:
			case SOUND_44k:
				if (rate != pSource->rate)
					continue;
				break;

			default:
				break;*/
			cl_entity_t *ent = CL_GetEntityByIndex (ch->entnum);

			if (ent != NULL)
				{
				if (sc->width == 1)
					SND_MoveMouth8 (&ent->mouth, ch->sample, sc, num_samples, ch->use_loop);
				else
					SND_MoveMouth16 (&ent->mouth, ch->sample, sc, num_samples, ch->use_loop);
				}
			}

		/*// get playback pitch
		if (ch->isSentence)
			ch->pitch = VOX_ModifyPitch (ch, ch->basePitch * 0.01f);
		else
			ch->pitch = ch->basePitch * 0.01f;*/
		double pitch = VOX_ModifyPitch (ch, ch->basePitch * 0.01) * pitch_mult;

		/*ch->pitch *= (sys_timescale.value + 1) / 2;*/
		num_mixed_channels++;

		/*if (CL_GetEntityByIndex (ch->entnum) && ((ch->entchannel == CHAN_VOICE) || (ch->entchannel == CHAN_STREAM)))*/
		if (ch->is_sentence)
			{
			/*if (pSource->width == 1)
				SND_MoveMouth8 (ch, pSource, sampleCount);
			else
				SND_MoveMouth16 (ch, pSource, sampleCount);
			}*/
			VOX_MixChannelToBuffer (dst, ch, num_samples, rate, pitch);

			/*// mix channel to all active paintbuffers.
			// NOTE: must be called once per channel only - consecutive calls retrieve additional data
			if (ch->isSentence)
				VOX_MixDataToDevice (ch, sampleCount, outputRate, 0);
			else
				S_MixDataToDevice (ch, sampleCount, outputRate, 0, 0);

			if (!S_ShouldContinueMixing (ch))*/
			if (ch->sentence_finished)
				S_FreeChannel (ch);
			}
		else
			{
			/*S_FreeChannel (ch);*/
			S_MixChannelToBuffer (dst, ch, num_samples, rate, pitch, 0, 0);

			if (ch->finished)
				S_FreeChannel (ch);
			}
		}

	/*}

	// [FWGS, 01.11.25]
	ifndef DISABLE_UPSAMPLING

	// pass in index -1...count+2, return pointer to source sample in either paintbuffer or delay buffer
	static portable_samplepair_t *S_GetNextpFilter (int i, portable_samplepair_t *pbuffer, portable_samplepair_t *pfiltermem)
	{
	// The delay buffer is assumed to precede the paintbuffer by 6 duplicated samples
	if (i == -1)
		return (&(pfiltermem[0]));
	if (i == 0)
		return (&(pfiltermem[1]));
	if (i == 1)
		return (&(pfiltermem[2]));

	// return from paintbuffer, where samples are doubled.
	// even samples are to be replaced with interpolated value
	return (&(pbuffer[(i - 2) * 2 + 1]));
	}

	// pass forward over passed in buffer and cubic interpolate all odd samples
	// pbuffer: buffer to filter (in place)
	// prevfilter: filter memory. NOTE: this must match the filtertype ie: filtercubic[] for FILTERTYPE_CUBIC
	//   if NULL then perform no filtering.
	// count: how many samples to upsample. will become count*2 samples in buffer, in place
	static void S_Interpolate2xCubic (portable_samplepair_t *pbuffer, portable_samplepair_t *pfiltermem,
	int cfltmem, int count)
	{
	// implement cubic interpolation on 2x upsampled buffer. Effectively delays buffer contents by 2 samples.
	// pbuffer: contains samples at 0, 2, 4, 6...
	// temppaintbuffer is temp buffer, same size as paintbuffer, used to store processed values
	// count: number of samples to process in buffer ie: how many samples at 0, 2, 4, 6...

	// finpos is the fractional, inpos the integer part.
	// finpos = 0.5 for upsampling by 2x
	// inpos is the position of the sample

	int			i;
	const int	upCount = Q_min (count << 1, PAINTBUFFER_SIZE);
	int			a, b, c;
	int			xm1, x0, x1, x2;
	portable_samplepair_t	*psamp0;
	portable_samplepair_t	*psamp1;
	portable_samplepair_t	*psamp2;
	portable_samplepair_t	*psamp3;
	int			outpos = 0;

	// pfiltermem holds 6 samples from previous buffer pass
	// process 'count' samples
	for (i = 0; i < count; i++)
		{
		// get source sample pointer
		psamp0 = S_GetNextpFilter (i - 1, pbuffer, pfiltermem);
		psamp1 = S_GetNextpFilter (i + 0, pbuffer, pfiltermem);
		psamp2 = S_GetNextpFilter (i + 1, pbuffer, pfiltermem);
		psamp3 = S_GetNextpFilter (i + 2, pbuffer, pfiltermem);

		// write out original sample to interpolation buffer
		temppaintbuffer[outpos++] = *psamp1;

		// get all left samples for interpolation window
		xm1 = psamp0->left;
		x0 = psamp1->left;
		x1 = psamp2->left;
		x2 = psamp3->left;

		// interpolate
		a = (3 * (x0 - x1) - xm1 + x2) / 2;
		b = 2 * x1 + xm1 - (5 * x0 + x2) / 2;
		c = (x1 - xm1) / 2;

		// write out interpolated sample
		temppaintbuffer[outpos].left = a / 8 + b / 4 + c / 2 + x0;

		// get all right samples for window
		xm1 = psamp0->right;
		x0 = psamp1->right;
		x1 = psamp2->right;
		x2 = psamp3->right;

		// interpolate
		a = (3 * (x0 - x1) - xm1 + x2) / 2;
		b = 2 * x1 + xm1 - (5 * x0 + x2) / 2;
		c = (x1 - xm1) / 2;

		// write out interpolated sample, increment output counter
		temppaintbuffer[outpos++].right = a / 8 + b / 4 + c / 2 + x0;

		if (outpos > HLARRAYSIZE (temppaintbuffer))
			break;
		}

	Assert (cfltmem >= 3);

	// save last 3 samples from paintbuffer
	pfiltermem[0] = pbuffer[upCount - 5];
	pfiltermem[1] = pbuffer[upCount - 3];
	pfiltermem[2] = pbuffer[upCount - 1];

	// copy temppaintbuffer back into paintbuffer
	memcpy (pbuffer, temppaintbuffer, sizeof (*pbuffer) * upCount);
	}

	// pass forward over passed in buffer and linearly interpolate all odd samples
	// pbuffer: buffer to filter (in place)
	// prevfilter:  filter memory. NOTE: this must match the filtertype ie: filterlinear[] for FILTERTYPE_LINEAR
	// if NULL then perform no filtering.
	// count: how many samples to upsample. will become count*2 samples in buffer, in place.
	static void S_Interpolate2xLinear (portable_samplepair_t *pbuffer, portable_samplepair_t *pfiltermem,
	int cfltmem, int count)
	{
	int	i, upCount = count << 1;

	Assert (upCount <= PAINTBUFFER_SIZE);
	Assert (cfltmem >= 1);

	// use interpolation value from previous mix
	pbuffer[0].left = (pfiltermem->left + pbuffer[0].left) >> 1;
	pbuffer[0].right = (pfiltermem->right + pbuffer[0].right) >> 1;

	for (i = 2; i < upCount; i += 2)
		{
		// use linear interpolation for upsampling
		pbuffer[i].left = (pbuffer[i].left + pbuffer[i - 1].left) >> 1;
		pbuffer[i].right = (pbuffer[i].right + pbuffer[i - 1].right) >> 1;
		}

	// save last value to be played out in buffer
	*pfiltermem = pbuffer[upCount - 1];
	}

	// upsample by 2x, optionally using interpolation
	// count: how many samples to upsample. will become count*2 samples in buffer, in place.
	// pbuffer: buffer to upsample into (in place)
	// pfiltermem:  filter memory. NOTE: this must match the filtertype ie: filterlinear[] for FILTERTYPE_LINEAR
	// if NULL then perform no filtering.
	// cfltmem: max number of sample pairs filter can use
	// filtertype: FILTERTYPE_NONE, _LINEAR, _CUBIC etc.  Must match prevfilter.
	static void S_MixBufferUpsample2x (int count, portable_samplepair_t *pbuffer, portable_samplepair_t *pfiltermem,
	int cfltmem, int filtertype)

	else

	static void S_MixBufferUpsample2x (int count, portable_samplepair_t *pbuffer)

	endif
	{
	int	upCount = count << 1;
	int	i, j;

	// reverse through buffer, duplicating contents for 'count' samples
	for (i = upCount - 1, j = count - 1; j >= 0; i -= 2, j--)
		{
		pbuffer[i] = pbuffer[j];
		pbuffer[i - 1] = pbuffer[j];
		}*/
	return num_mixed_channels;

	/*// [FWGS, 01.11.25]
	ifndef DISABLE_UPSAMPLING
	// pass forward through buffer, interpolate all even slots
	switch (filtertype)
		{
		case FILTERTYPE_LINEAR:
			S_Interpolate2xLinear (pbuffer, pfiltermem, cfltmem, count);
			break;
		case FILTERTYPE_CUBIC:
			S_Interpolate2xCubic (pbuffer, pfiltermem, cfltmem, count);
			break;
		default:	// no filter
			break;
		}
	endif*/
	}

// [FWGS, 01.03.26] removed MIX_ClearAllPaintBuffers
 
// [FWGS, 01.03.26]
/*// [FWGS, 01.11.25] zero out all paintbuffers
void MIX_ClearAllPaintBuffers (int SampleCount, qboolean clearFilters)*/
static int S_AverageSample (int a, int b)
	{
	/*int	count = Q_min (SampleCount, PAINTBUFFER_SIZE);
	int	i;

	// zero out all paintbuffer data (ignore sampleCount)
	for (i = 0; i < CPAINTBUFFERS; i++)
		{
		if (paintbuffers[i].pbuf != NULL)
			memset (paintbuffers[i].pbuf, 0, (count + 1) * sizeof (portable_samplepair_t));

	ifndef DISABLE_UPSAMPLING
		if (clearFilters)
			{
			memset (paintbuffers[i].fltmem, 0, sizeof (paintbuffers[i].fltmem));
			}
		}

	if (clearFilters)
		{
		MIX_ResetPaintbufferFilterCounters ();
	endif
		}*/
	return (a >> 1) + (b >> 1) + (((a & 1) + (b & 1)) >> 1);
	}

// [FWGS, 01.03.26] removed MIX_MixPaintbuffers

// [FWGS, 01.03.26]
/*// [FWGS, 09.05.24] mixes pbuf1 + pbuf2 into pbuf3, count samples
// fgain is output gain 0-1.0
// NOTE: pbuf3 may equal pbuf1 or pbuf2!
static void MIX_MixPaintbuffers (int ibuf1, int ibuf2, int ibuf3, int count, float fgain)*/
static void S_UpsampleBuffer (portable_samplepair_t *dst, size_t num_samples)
	{
	/*portable_samplepair_t *pbuf1, *pbuf2, *pbuf3;
	int		i, gain;

	gain = 256 * fgain;

	Assert (count <= PAINTBUFFER_SIZE);
	Assert (ibuf1 < CPAINTBUFFERS);
	Assert (ibuf2 < CPAINTBUFFERS);
	Assert (ibuf3 < CPAINTBUFFERS);

	pbuf1 = paintbuffers[ibuf1].pbuf;
	pbuf2 = paintbuffers[ibuf2].pbuf;
	pbuf3 = paintbuffers[ibuf3].pbuf;

	if (!gain)*/
	if (s_lerping.value)
		{
		/*// do not mix buf2 into buf3, just copy
		if (pbuf1 != pbuf3)
			memcpy (pbuf3, pbuf1, sizeof (*pbuf1) * count);
		return;
		}

		// destination buffer stereo - average n chans down to stereo

		// destination 2ch:
		// pb1 2ch + pb2 2ch		-> pb3 2ch
		// pb1 2ch + pb2 (4ch->2ch)		-> pb3 2ch
		// pb1 (4ch->2ch) + pb2 (4ch->2ch)	-> pb3 2ch*/
		// copy even positions and average odd
		for (size_t i = num_samples - 1; i > 0; i--)
			{
			dst[i * 2] = dst[i];

			/*// mix front channels
			for (i = 0; i < count; i++)*/
			dst[i * 2 + 1].left = S_AverageSample (dst[i].left, dst[i - 1].left);
			dst[i * 2 + 1].right = S_AverageSample (dst[i].right, dst[i - 1].right);
			}
		}
	else
		{
		/*pbuf3[i].left = pbuf1[i].left;
		pbuf3[i].right = pbuf1[i].right;
		pbuf3[i].left += (pbuf2[i].left * gain) >> 8;
		pbuf3[i].right += (pbuf2[i].right * gain) >> 8;*/
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
// 
// [FWGS, 01.03.26]
/*static void MIX_CompressPaintbuffer (int ipaint, int count)*/
static int S_MixNormalChannelsToRoombuffer (int end, int count)
	{
	/*portable_samplepair_t	*pbuf;
	paintbuffer_t			*ppaint;
	int		i;*/
	// for room buffer we only support CD rates like 11k, 22k, and 44k
	// TODO: 48k output would require support from platform-specific backends first
	// until there is no real usecase, let's keep it simple
	int num_mixed_channels = S_MixNormalChannels (roombuffer, end, SOUND_11k);

	/*ppaint = MIX_GetPPaintFromIPaint (ipaint);
	pbuf = ppaint->pbuf;

	for (i = 0; i < count; i++, pbuf++)*/
	if (dma.format.speed >= SOUND_22k)
		{
		/*pbuf->left = CLIP (pbuf->left);
		pbuf->right = CLIP (pbuf->right);*/
		if (num_mixed_channels > 0)
			S_UpsampleBuffer (roombuffer, count / (SOUND_22k / SOUND_11k));

		num_mixed_channels += S_MixNormalChannels (roombuffer, end, SOUND_22k);
		}

	/*}

	// [FWGS, 01.11.25]
	static void S_MixUpsample (int sampleCount, int filtertype)
	{
	paintbuffer_t	*ppaint = MIX_GetCurrentPaintbufferPtr ();*/
	if (dma.format.speed >= SOUND_44k)
		{
		if (num_mixed_channels > 0)
			S_UpsampleBuffer (roombuffer, count / (SOUND_44k / SOUND_22k));

		/*ifndef DISABLE_UPSAMPLING
		int		ifilter = ppaint->ifilter;

		Assert (ifilter < CPAINTFILTERS);

		S_MixBufferUpsample2x (sampleCount, ppaint->pbuf, &(ppaint->fltmem[ifilter][0]), CPAINTFILTERMEM, filtertype);
		else
		(void)filtertype;
		endif*/
		num_mixed_channels += S_MixNormalChannels (roombuffer, end, SOUND_44k);
		}

	/*ifndef DISABLE_UPSAMPLING
	// make sure on next upsample pass for this paintbuffer, new filter memory is used
	ppaint->ifilter++;
	else
	S_MixBufferUpsample2x (sampleCount, ppaint->pbuf);
	endif*/
	return num_mixed_channels;
	}

// [FWGS, 01.03.26] removed MIX_MixRawSamplesBuffer

// [FWGS, 01.03.26]
/*// [FWGS, 01.07.25]
static void MIX_MixRawSamplesBuffer (int end)*/
static int S_MixRawChannels (int end)
	{
	/*portable_samplepair_t	*pbuf, *roombuf, *streambuf, *voicebuf;
	uint	i, j, stop;*/
	int num_room_channels = 0;

	/*roombuf = MIX_GetPFrontFromIPaint (IROOMBUFFER);
	streambuf = MIX_GetPFrontFromIPaint (ISTREAMBUFFER);
	voicebuf = MIX_GetPFrontFromIPaint (IVOICEBUFFER);

	if (s_listener.paused) 
		return;*/
	if (cl.paused)
		return 0;

	// paint in the raw channels
	/*for (i = 0; i < MAX_RAW_CHANNELS; i++)*/
	for (size_t i = 0; i < HLARRAYSIZE (raw_channels); i++)
		{
		// copy from the streaming sound source
		rawchan_t *ch = raw_channels[i];
		/*qboolean stream;
		qboolean is_voice;*/

		// background track should be mixing into another buffer
		if (!ch)
			continue;

		// not audible
		if (!ch->leftvol && !ch->rightvol)
			continue;

		/*// [FWGS, 01.11.25]
		is_voice = ((ch->entnum > 0) && (ch->entnum <= MAX_CLIENTS)) ||
			(ch->entnum == VOICE_LOOPBACK_INDEX) ||
			(ch->entnum == VOICE_LOCALCLIENT_INDEX);*/
		qboolean is_voice = CL_IsPlayerIndex (ch->entnum) ||
			(ch->entnum == VOICE_LOOPBACK_INDEX) ||
			(ch->entnum == VOICE_LOCALCLIENT_INDEX);

		/*stream = (ch->entnum == S_RAW_SOUND_BACKGROUNDTRACK) || CL_IsPlayerIndex (ch->entnum);
		if (is_voice)
			pbuf = voicebuf;
		else if (stream)
			pbuf = streambuf;*/
		portable_samplepair_t *pbuf;
		if (is_voice || (ch->entnum == S_RAW_SOUND_BACKGROUNDTRACK))
			{
			// for streams we don't have fancy things like volume controls
			// or DSP processing or upsampling, so paint it directly into result buffer
			pbuf = paintbuffer;
			}
		else
			/*pbuf = roombuf;*/
			{
			pbuf = roombuffer;
			num_room_channels++;
			}

		/*stop = (end < ch->s_rawend) ? end : ch->s_rawend;*/
		uint stop = (end < ch->s_rawend) ? end : ch->s_rawend;
		const uint mask = ch->max_samples - 1;

		/*for (j = paintedtime; j < stop; j++)*/
		for (size_t i = 0, j = paintedtime; j < stop; i++, j++)
			{
			/*pbuf[j - paintedtime].left += (ch->rawsamples[j & (ch->max_samples - 1)].left * ch->leftvol) >> 8;
			pbuf[j - paintedtime].right += (ch->rawsamples[j & (ch->max_samples - 1)].right * ch->rightvol) >> 8;*/
			pbuf[i].left += (ch->rawsamples[j & mask].left * ch->leftvol) >> 8;
			pbuf[i].right += (ch->rawsamples[j & mask].right * ch->rightvol) >> 8;
			}

		if (ch->entnum > 0)
			{
			cl_entity_t *ent = CL_GetEntityByIndex (ch->entnum);
			int pos = paintedtime & (ch->max_samples - 1);
			int count = bound (0, ch->max_samples - pos, stop - paintedtime);

			/*SND_MoveMouthRaw (ch, &ch->rawsamples[pos], bound (0, ch->max_samples - pos, stop - paintedtime));*/
			if (ent)
				SND_MoveMouthRaw (&ent->mouth, &ch->rawsamples[pos], count);
			}
		}

	return num_room_channels;
	}

// [FWGS, 01.03.26] removed MIX_UpsampleAllPaintbuffers

// [FWGS, 01.03.26]
/*// upsample and mix sounds into final 44khz versions of:
// IROOMBUFFER, IFACINGBUFFER, IFACINGAWAY
// dsp fx are then applied to these buffers by the caller.
// caller also remixes all into final IPAINTBUFFER output
static void MIX_UpsampleAllPaintbuffers (int end, int count)*/
static void S_MixBufferWithGain (portable_samplepair_t *dst, const portable_samplepair_t *src, size_t count, int gain)
	{
	/*// 11khz sounds are mixed into 3 buffers based on distance from listener, and facing direction
	// These buffers are facing, facingaway, room
	// These 3 mixed buffers are then each upsampled to 22khz

	// 22khz sounds are mixed into the 3 buffers based on distance from listener, and facing direction
	// These 3 mixed buffers are then each upsampled to 44khz*/
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
/*// 44khz sounds are mixed into the 3 buffers based on distance from listener, and facing direction*/
static void S_WriteLinearBlastStereo16 (short *snd_out, const int *snd_p, size_t count)
	{
	for (size_t i = 0; i < count; i += 2)
		{
		snd_out[i + 0] = CLIP16 (snd_p[i + 0]);
		snd_out[i + 1] = CLIP16 (snd_p[i + 1]);
		}
	}

// [FWGS, 01.03.26]
/*MIX_DeactivateAllPaintbuffers ();*/
static void S_TransferPaintBuffer (const portable_samplepair_t *src, int endtime)
	{
	const int	*snd_p = (const int *)src;
	const int	sampleMask = ((dma.samples >> 1) - 1);
	int			lpaintedtime = paintedtime;

	/*// [FWGS, 01.11.25]
	ifndef DISABLE_UPSAMPLING
	// set paintbuffer upsample filter indices to 0
	MIX_ResetPaintbufferFilterCounters ();
	endif

	// only mix to roombuffer if dsp fx are on KDB: perf
	MIX_ActivatePaintbuffer (IROOMBUFFER);	// operates on MIX_MixChannelsToPaintbuffer*/
	SNDDMA_BeginPainting ();

	/*// mix 11khz sounds:
	MIX_MixChannelsToPaintbuffer (end, SOUND_11k, SOUND_11k);*/
	while (lpaintedtime < endtime)
		{
		// handle recirculating buffer issues
		int lpos = lpaintedtime & sampleMask;

		/*if SOUND_DMA_SPEED >= SOUND_22k
		// upsample all 11khz buffers by 2x
		// only upsample roombuffer if dsp fx are on KDB: perf
		MIX_SetCurrentPaintbuffer (IROOMBUFFER); // operates on MixUpSample
		S_MixUpsample (count / (SOUND_DMA_SPEED / SOUND_11k), s_lerping.value);*/
		short *snd_out = (short *)dma.buffer + (lpos << 1);

		/*// mix 22khz sounds:
		MIX_MixChannelsToPaintbuffer (end, SOUND_22k, SOUND_22k);
		endif*/
		int snd_linear_count = (dma.samples >> 1) - lpos;
		if (lpaintedtime + snd_linear_count > endtime)
			snd_linear_count = endtime - lpaintedtime;

		/*if SOUND_DMA_SPEED >= SOUND_44k
		// upsample all 22khz buffers by 2x
		// only upsample roombuffer if dsp fx are on KDB: perf
		MIX_SetCurrentPaintbuffer (IROOMBUFFER);
		S_MixUpsample (count / (SOUND_DMA_SPEED / SOUND_22k), s_lerping.value);*/
		snd_linear_count <<= 1;

		/*// mix all 44khz sounds to all active paintbuffers
		MIX_MixChannelsToPaintbuffer (end, SOUND_44k, SOUND_DMA_SPEED);
		endif*/
		// write a linear blast of samples
		S_WriteLinearBlastStereo16 (snd_out, snd_p, snd_linear_count);

		/*// mix raw samples from the video streams
		MIX_MixRawSamplesBuffer (end);*/
		snd_p += snd_linear_count;
		lpaintedtime += (snd_linear_count >> 1);
		}

	/*MIX_DeactivateAllPaintbuffers ();
	MIX_SetCurrentPaintbuffer (IPAINTBUFFER);*/
	SNDDMA_Submit ();
	}

// [FWGS, 01.03.26] removed MIX_PaintChannels

// [FWGS, 01.03.26]
/*void MIX_PaintChannels (int endtime)*/
void S_ClearBuffers (int num_samples)
	{
	/*int	end, count;*/
	const size_t num_bytes = (num_samples + 1) * sizeof (portable_samplepair_t);

	/*CheckNewDspPresets ();*/
	memset (roombuffer, 0, num_bytes);
	memset (paintbuffer, 0, num_bytes);
	}

// [FWGS, 01.03.26]
void S_PaintChannels (int endtime)
	{
	int gain = S_GetMasterVolume () * 256;

	while (paintedtime < endtime)
		{
		// if paintbuffer is smaller than DMA buffer
		/*end = endtime;
		if (endtime - paintedtime > PAINTBUFFER_SIZE)*/
		int end = endtime;
		if (end - paintedtime > PAINTBUFFER_SIZE)
			end = paintedtime + PAINTBUFFER_SIZE;

		/*// number of 44khz samples to mix into paintbuffer, up to paintbuffer size
		count = end - paintedtime;

		// clear the all mix buffers
		MIX_ClearAllPaintBuffers (count, false);*/
		const int num_samples = end - paintedtime;

		/*MIX_UpsampleAllPaintbuffers (end, count);*/
		S_ClearBuffers (num_samples);

		/*// [FWGS, 01.01.24] process all sounds with DSP
		if (cls.key_dest != key_menu)
			DSP_Process (MIX_GetPFrontFromIPaint (IROOMBUFFER), count);

		// add music or soundtrack from movie (no dsp)
		MIX_MixPaintbuffers (IPAINTBUFFER, IROOMBUFFER, IPAINTBUFFER, count, S_GetMasterVolume ());*/
		int room_channels = S_MixNormalChannelsToRoombuffer (end, num_samples);

		/*// [FWGS, 01.07.25] add music or soundtrack from movie (no dsp)
		MIX_MixPaintbuffers (IPAINTBUFFER, ISTREAMBUFFER, IPAINTBUFFER, count, 1.0f);*/
		room_channels += S_MixRawChannels (end);

		/*// [FWGS, 01.07.25] voice chat
		MIX_MixPaintbuffers (IPAINTBUFFER, IVOICEBUFFER, IPAINTBUFFER, count, 1.0f);

		// clip all values > 16 bit down to 16 bit
		MIX_CompressPaintbuffer (IPAINTBUFFER, count);*/
		// now process DSP and mix result into paintbuffer
		if (cls.key_dest != key_menu)
			SX_RoomFX (roombuffer, num_samples);

		/*// transfer IPAINTBUFFER paintbuffer out to DMA buffer
		MIX_SetCurrentPaintbuffer (IPAINTBUFFER);*/
		if ((room_channels > 0) || (cls.key_dest != key_menu))
			S_MixBufferWithGain (paintbuffer, roombuffer, num_samples, gain);

		// transfer out according to DMA format
		/*S_TransferPaintBuffer (end);*/
		S_TransferPaintBuffer (paintbuffer, end);
		paintedtime = end;
		}
	}
