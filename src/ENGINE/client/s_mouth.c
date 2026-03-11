/***
s_mouth.c - animate mouth
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

#include "common.h"
#include "sound.h"
#include "client.h"
#include "const.h"

#define CAVGSAMPLES		10

// [FWGS, 01.03.26] removed SND_InitMouth, SND_CloseMouth
// 
// [FWGS, 01.03.26]
/*void SND_InitMouth (int entnum, int entchannel)*/
static void SND_CommitMouthValue (mouth_t *mouth, int savg, int scount)
	{
	/*if (((entchannel == CHAN_VOICE) || (entchannel == CHAN_STREAM)) && (entnum > 0))
		SND_ForceInitMouth (entnum);
	}*/
	mouth->sndavg += savg;
	mouth->sndcount = (byte)scount;

	/*void SND_CloseMouth (channel_t *ch)
	{
	if ((ch->entchannel == CHAN_VOICE) || (ch->entchannel == CHAN_STREAM))*/
	if (mouth->sndcount >= CAVGSAMPLES)
		{
		/*SND_ForceCloseMouth (ch->entnum);*/
		mouth->mouthopen = mouth->sndavg / CAVGSAMPLES;
		mouth->sndavg = 0;
		mouth->sndcount = 0;
		}
	}

// [FWGS, 01.03.26]
/*void SND_MoveMouth8 (channel_t *ch, wavdata_t *pSource, int count)*/
void SND_MoveMouth8 (mouth_t *mouth, int pos, const wavdata_t *sc, int count, qboolean use_loop)
	{
	/*cl_entity_t *clientEntity;
	signed char *pdata = NULL;
	mouth_t *pMouth = NULL;
	int		scount, pos = 0;*/
	const int8_t	*pdata = NULL;
	int		scount;
	int		savg, data;
	uint 	i;

	/*clientEntity = CL_GetEntityByIndex (ch->entnum);
	if (!clientEntity) return;

	pMouth = &clientEntity->mouth;

	if (ch->isSentence)
		{
		if (ch->currentWord)
			pos = ch->currentWord->sample;
		}
	else pos = ch->pMixer.sample;

	count = S_GetOutputData (pSource, (void **)&pdata, pos, count, ch->use_loop);*/
	// ESHQ: присвоение значения параметру, переданному не через указатель?
	count = S_RetrieveAudioSamples (sc, (const void **)&pdata, pos, count, use_loop);

	if (pdata == NULL)
		return;

	i = 0;
	/*scount = pMouth->sndcount;*/
	scount = mouth->sndcount;
	savg = 0;

	while ((i < count) && (scount < CAVGSAMPLES))
		{
		data = pdata[i];
		savg += abs (data);

		i += 80 + ((byte)data & 0x1F);
		scount++;
		}

	/*pMouth->sndavg += savg;
	pMouth->sndcount = (byte)scount;

	if (pMouth->sndcount >= CAVGSAMPLES)
		{
		pMouth->mouthopen = pMouth->sndavg / CAVGSAMPLES;
		pMouth->sndavg = 0;
		pMouth->sndcount = 0;
		}*/
	SND_CommitMouthValue (mouth, savg, scount);
	}

// [FWGS, 01.03.26]
/*void SND_MoveMouth16 (channel_t *ch, wavdata_t *pSource, int count)*/
void SND_MoveMouth16 (mouth_t *mouth, int pos, const wavdata_t *sc, int count, qboolean use_loop)
	{
	/*cl_entity_t *clientEntity;
	short *pdata = NULL;
	mouth_t *pMouth = NULL;*/
	const int16_t	*pdata = NULL;
	int		savg, data;
	/*int		scount, pos = 0;*/
	int		scount;
	uint 	i;

	/*clientEntity = CL_GetEntityByIndex (ch->entnum);
	if (!clientEntity) return;

	pMouth = &clientEntity->mouth;

	if (ch->isSentence)
		{
		if (ch->currentWord)
			pos = ch->currentWord->sample;
		}
	else pos = ch->pMixer.sample;

	count = S_GetOutputData (pSource, (void **)&pdata, pos, count, ch->use_loop);*/
	// ESHQ: опять?
	count = S_RetrieveAudioSamples (sc, (const void **)&pdata, pos, count, use_loop);

	if (pdata == NULL)
		return;

	i = 0;
	/*scount = pMouth->sndcount;*/
	scount = mouth->sndcount;
	savg = 0;

	while (i < count && scount < CAVGSAMPLES)
		{
		data = pdata[i];
		data = (bound (-32767, data, 0x7ffe) >> 8);
		savg += abs (data);

		i += 80 + ((byte)data & 0x1F);
		scount++;
		}

	/*pMouth->sndavg += savg;
	pMouth->sndcount = (byte)scount;

	if (pMouth->sndcount >= CAVGSAMPLES)
		{
		pMouth->mouthopen = pMouth->sndavg / CAVGSAMPLES;
		pMouth->sndavg = 0;
		pMouth->sndcount = 0;
		}*/
	SND_CommitMouthValue (mouth, savg, scount);
	}

void SND_ForceInitMouth (int entnum)
	{
	cl_entity_t *clientEntity;

	clientEntity = CL_GetEntityByIndex (entnum);
	if (clientEntity)
		{
		clientEntity->mouth.mouthopen = 0;
		clientEntity->mouth.sndavg = 0;
		clientEntity->mouth.sndcount = 0;
		}
	}

void SND_ForceCloseMouth (int entnum)
	{
	cl_entity_t *clientEntity;

	clientEntity = CL_GetEntityByIndex (entnum);
	if (clientEntity)
		clientEntity->mouth.mouthopen = 0;
	}

// [FWGS, 01.03.26]
/*void SND_MoveMouthRaw (rawchan_t *ch, portable_samplepair_t *pData, int count)*/
void SND_MoveMouthRaw (mouth_t *mouth, const portable_samplepair_t *buf, int count)
	{
	/*cl_entity_t *clientEntity;
	mouth_t *pMouth = NULL;*/
	int		savg, data;
	int		scount = 0;
	uint 	i;

	/*clientEntity = CL_GetEntityByIndex (ch->entnum);
	if (!clientEntity) return;

	pMouth = &clientEntity->mouth;

	if (pData == NULL)
		return;*/

	i = 0;
	/*scount = pMouth->sndcount;*/
	scount = mouth->sndcount;
	savg = 0;

	while ((i < count) && (scount < CAVGSAMPLES))
		{
		/*data = pData[i].left; // mono sound anyway*/
		data = buf[i].left;	// mono sound anyway
		data = (bound (-32767, data, 0x7ffe) >> 8);
		savg += abs (data);

		i += 80 + ((byte)data & 0x1F);
		scount++;
		}

	/*pMouth->sndavg += savg;
	pMouth->sndcount = (byte)scount;

	if (pMouth->sndcount >= CAVGSAMPLES)
		{
		pMouth->mouthopen = pMouth->sndavg / CAVGSAMPLES;
		pMouth->sndavg = 0;
		pMouth->sndcount = 0;
		}*/
	SND_CommitMouthValue (mouth, savg, scount);
	}
