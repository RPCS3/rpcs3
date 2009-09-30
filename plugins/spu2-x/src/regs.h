/* SPU2-X, A plugin for Emulating the Sound Processing Unit of the Playstation 2
 * Developed and maintained by the Pcsx2 Development Team.
 * 
 * Original portions from SPU2ghz are (c) 2008 by David Quintana [gigaherz]
 *
 * SPU2-X is free software: you can redistribute it and/or modify it under the terms
 * of the GNU Lesser General Public License as published by the Free Software Found-
 * ation, either version 3 of the License, or (at your option) any later version.
 *
 * SPU2-X is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with SPU2-X.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#define SPU2_CORE0       0x00000000
#define SPU2_CORE1		 0x00000400

#define SPU2_VP(voice)   ((voice) * 16)
#define SPU2_VA(voice)   ((voice) * 12)

#define REG_VP_VOLL      0x0000		// Voice Volume Left
#define REG_VP_VOLR      0x0002		// Voice Volume Right
#define REG_VP_PITCH     0x0004		// Pitch
#define REG_VP_ADSR1     0x0006		// Envelope 1 (Attack-Decay-Sustain-Release)
#define REG_VP_ADSR2     0x0008		// Envelope 2 (Attack-Decay-Sustain-Release)
#define REG_VP_ENVX      0x000A		// Current Envelope
#define REG_VP_VOLXL     0x000C		// Current Voice Volume Left
#define REG_VP_VOLXR     0x000E		// Current Voice Volume Right

// .. repeated for each voice ..

#define REG_S_PMON       0x0180		// Pitch Modulation Spec.
#define REG_S_NON        0x0184		// Alloc Noise Generator
#define REG_S_VMIXL      0x0188		// Voice Output Mix Left  (Dry)
#define REG_S_VMIXEL     0x018C		// Voice Output Mix Left  (Wet)
#define REG_S_VMIXR      0x0190		// Voice Output Mix Right (Dry)
#define REG_S_VMIXER     0x0194		// Voice Output Mix Right (Wet)

#define REG_P_MMIX       0x0198		// Output Spec. After Voice Mix
#define REG_C_ATTR       0x019A		// Core X Attrib
#define REG_A_IRQA       0x019C		// Interrupt Address Spec.

#define REG_S_KON        0x01A0		// Key On 0/1
#define REG_S_KOFF       0x01A4		// Key Off 0/1

#define REG_A_TSA        0x01A8		// Transfer starting address
#define REG__1AC         0x01AC 	// Transfer data
#define REG__1AE         0x01AE 
#define REG_S_ADMAS      0x01B0 	// AutoDMA Status

// 1b2, 1b4, 1b6, 1b8, 1ba, 1bc, 1be are unknown

#define REG_VA_SSA       0x01C0		// Waveform data starting address
#define REG_VA_LSAX      0x01C4		// Loop point address
#define REG_VA_NAX       0x01C8		// Waveform data that should be read next

// .. repeated for each voice ..

#define REG_A_ESA        0x02E0		//Address: Top address of working area for effects processing
#define R_FB_SRC_A       0x02E4		// Feedback Source A
#define R_FB_SRC_B       0x02E8		// Feedback Source B
#define R_IIR_DEST_A0    0x02EC
#define R_IIR_DEST_A1    0x02F0
#define R_ACC_SRC_A0     0x02F4
#define R_ACC_SRC_A1     0x02F8
#define R_ACC_SRC_B0     0x02FC
#define R_ACC_SRC_B1     0x0300
#define R_IIR_SRC_A0     0x0304
#define R_IIR_SRC_A1     0x0308
#define R_IIR_DEST_B0    0x030C
#define R_IIR_DEST_B1    0x0310
#define R_ACC_SRC_C0     0x0314
#define R_ACC_SRC_C1     0x0318
#define R_ACC_SRC_D0     0x031C
#define R_ACC_SRC_D1     0x0320
#define R_IIR_SRC_B1     0x0324
#define R_IIR_SRC_B0     0x0328
#define R_MIX_DEST_A0    0x032C
#define R_MIX_DEST_A1    0x0330
#define R_MIX_DEST_B0    0x0334
#define R_MIX_DEST_B1    0x0338
#define REG_A_EEA        0x033C		// Address: End address of working area for effects processing (upper part of address only!)

#define REG_S_ENDX       0x0340		// End Point passed flag

#define REG_P_STATX      0x0344 	// Status register?

// 0x346 .. 0x3fe are unknown (unused?)

// core 1 has the same registers with 0x400 added.
// core 1 ends at 0x746

// 0x746 .. 0x75e are unknown

// "Different" register area

#define REG_P_MVOLL      0x0760		// Master Volume Left
#define REG_P_MVOLR      0x0762		// Master Volume Right
#define REG_P_EVOLL      0x0764		// Effect Volume Left
#define REG_P_EVOLR      0x0766		// Effect Volume Right
#define REG_P_AVOLL      0x0768		// Core External Input Volume Left  (Only Core 1)
#define REG_P_AVOLR      0x076A		// Core External Input Volume Right (Only Core 1)
#define REG_P_BVOLL      0x076C 	// Sound Data Volume Left
#define REG_P_BVOLR      0x076E		// Sound Data Volume Right
#define REG_P_MVOLXL     0x0770		// Current Master Volume Left
#define REG_P_MVOLXR     0x0772		// Current Master Volume Right

#define R_IIR_ALPHA      0x0774		//IIR alpha (% used)
#define R_ACC_COEF_A     0x0776
#define R_ACC_COEF_B     0x0778
#define R_ACC_COEF_C     0x077A
#define R_ACC_COEF_D     0x077C
#define R_IIR_COEF       0x077E
#define R_FB_ALPHA       0x0780		//feedback alpha (% used)
#define R_FB_X           0x0782		//feedback 
#define R_IN_COEF_L      0x0784
#define R_IN_COEF_R      0x0786

// values repeat for core1

// End OF "Different" register area

// SPDIF interface
#define SPDIF_OUT        0x07C0		// SPDIF Out: OFF/'PCM'/Bitstream/Bypass
#define SPDIF_IRQINFO    0x07C2	
#define SPDIF_MODE       0x07C6			
#define SPDIF_MEDIA      0x07C8		// SPDIF Media: 'CD'/DVD	
#define SPDIF_PROTECT	 0x07CC		// SPDIF Copy Protection


/*********************************************************************
Core attributes (SD_C)
  bit 1         - Unknown (this bit is sometimes set)
  bit 2..3      - Unknown (usually never set)
  bit 4..5      - DMA related
  bit 6         - IRQ? DMA mode? wtf?
  bit 7         - effect enable (reverb enable)
  bit 8			- IRQ enable?
  bit 9..14     - noise clock
  bit 15        - mute
  bit 16        - reset

*********************************************************************/

#define SPDIF_OUT_OFF				0x0000		// no spdif output
#define SPDIF_OUT_PCM				0x0020		// encode spdif from spu2 pcm output
#define SPDIF_OUT_BYPASS			0x0100		// bypass spu2 processing

#define SPDIF_MODE_BYPASS_BITSTREAM	0x0002		// bypass mode for digital bitstream data
#define SPDIF_MODE_BYPASS_PCM		0x0000		// bypass mode for pcm data (using analog output)

#define SPDIF_MODE_MEDIA_CD			0x0800		// source media is a CD
#define SPDIF_MODE_MEDIA_DVD		0x0000		// source media is a DVD

#define SPDIF_MEDIA_CDVD			0x0200
#define SPDIF_MEDIA_400				0x0000

#define SPDIF_PROTECT_NORMAL		0x0000		// spdif stream is not protected
#define SPDIF_PROTECT_PROHIBIT		0x8000		// spdif stream can't be copied

/********************************************************************/

#define VOICE_PARAM_VOLL     0x0		// Voice Volume Left
#define VOICE_PARAM_VOLR     0x2		// Voice Volume Right
#define VOICE_PARAM_PITCH    0x4		// Pitch
#define VOICE_PARAM_ADSR1    0x6		// Envelope 1 (Attack-Delay-Sustain-Release)
#define VOICE_PARAM_ADSR2    0x8		// Envelope 2 (Attack-Delay-Sustain-Release)
#define VOICE_PARAM_ENVX     0xA		// Current Envelope
#define VOICE_PARAM_VOLXL    0xC		// Current Voice Volume Left
#define VOICE_PARAM_VOLXR    0xE		// Current Voice Volume Right

/********************************************************************/

#define VOICE_ADDR_SSA		 0x0		// Waveform data starting address
#define VOICE_ADDR_LSAX      0x4		// Loop point address
#define VOICE_ADDR_NAX       0x8		// Waveform data that should be read next



// --------------------------------------------------------------------------------------
//  SPU2-X Register Table LUT
// --------------------------------------------------------------------------------------

#define U16P(x)		( (u16*)&(x) )

// Returns the hiword of a 32 bit integer.
#define U16P_HI(x)	( ((u16*)&(x))+1 )

extern u16* regtable[0x401];

