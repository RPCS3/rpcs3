/*  ZeroSPU2
 *  Copyright (C) 2006-2010 zerofrog
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef ZEROWORKER_H_INCLUDED
#define ZEROWORKER_H_INCLUDED

extern u32 dwNewChannel2[2]; // keeps track of what channels that have been turned on
extern u32 dwEndChannel2[2]; // keeps track of what channels have ended
extern AUDIOBUFFER s_pAudioBuffers[NSPACKETS];
extern s32 s_nCurBuffer, s_nQueuedBuffers;
extern VOICE_PROCESSED voices[SPU_NUMBER_VOICES+1]; // +1 for modulation
extern u16* pSpuIrq[2];
extern s32 s_buffers[NSSIZE][2]; // left and right buffers
extern bool g_bPlaySound; // if true, will output sound, otherwise no
extern s16* s_pCurOutput;
extern bool s_bThreadExit;
extern s32 s_nDropPacket;
extern u64 s_GlobalTimeStamp;
extern s32 s_nDurations[64];
extern s32 s_nCurDuration, s_nTotalDuration;
extern u32 g_startcount, g_packetcount;

extern void SPU2Worker();
extern void LogPacketSound(void* packet, s32 memsize);
extern void LogRawSound(void* pleft, s32 leftstride, void* pright, s32 rightstride, s32 numsamples);

extern s32 MixADSR(VOICE_PROCESSED* pvoice);
extern void MixChannels(s32 core);
#endif // ZEROWORKER_H_INCLUDED
