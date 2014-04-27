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


#ifndef __REG_H__
#define __REG_H__

////////////////////
// SPU2 Registers //
////////////////////
enum
{
// Volume Registers - currently not implemented in ZeroSPU2, like most volume registers.
 REG_VP_VOLL     			 = 0x0000, // Voice Volume Left
 REG_VP_VOLR    			 = 0x0002, // Voice Volume Right
 REG_VP_PITCH    			 = 0x0004, // Pitch
 REG_VP_ADSR1  			 = 0x0006, // Envelope 1 (Attack-Decay-Sustain-Release)
 REG_VP_ADSR2   			 = 0x0008, // Envelope 2 (Attack-Decay-Sustain-Release)
 REG_VP_ENVX     			 = 0x000A, // Current Envelope
 REG_VP_VOLXL    			 = 0x000C, // Current Voice Volume Left
 REG_VP_VOLXR    			 = 0x000E, // Current Voice Volume Right
// end unimplemented section

 REG_C0_FMOD1    			 = 0x0180, // Pitch Modulation Spec.
 REG_C0_FMOD2    			 = 0x0182,
 REG_S_NON        			 = 0x0184, // Alloc Noise Generator - unimplemented
 REG_C0_VMIXL1   			 = 0x0188, // Voice Output Mix Left  (Dry)
 REG_C0_VMIXL2    			 = 0x018A,
 REG_S_VMIXEL   			 = 0x018C, // Voice Output Mix Left  (Wet) - unimplemented
 REG_C0_VMIXR1   			 = 0x0190, // Voice Output Mix Right (Dry)
 REG_C0_VMIXR2  			 = 0x0192,
 REG_S_VMIXER   			 = 0x0194, // Voice Output Mix Right (Wet) - unimplemented

 REG_C0_MMIX     			 = 0x0198, // Output Spec. After Voice Mix
 REG_C0_CTRL      			 = 0x019A, // Core X Attrib
 REG_C0_IRQA_HI  			 = 0x019C, // Interrupt Address Spec. - Hi
 REG_C0_IRQA_LO 			 = 0x019E, // Interrupt Address Spec. - Lo

 REG_C0_SPUON1 			 = 0x01A0, // Key On 0/1
 REG_C0_SPUON2  			 = 0x01A2,
 REG_C0_SPUOFF1  			 = 0x01A4, // Key Off 0/1
 REG_C0_SPUOFF2  			 = 0x01A6,

 REG_C0_SPUADDR_HI 		 = 0x01A8, // Transfer starting address - hi
 REG_C0_SPUADDR_LO 		 = 0x01AA, // Transfer starting address - lo
 REG_C0_SPUDATA   		 = 0x01AC, // Transfer data
 REG_C0_DMACTRL  			 = 0x01AE, // unimplemented
 REG_C0_ADMAS     			 = 0x01B0, // AutoDMA Status

 // Section Unimplemented
 // Actually, some are implemented but weren't using the constants.
 REG_VA_SSA      			 = 0x01C0, // Waveform data starting address
 REG_VA_LSAX    			 = 0x01C4, // Loop point address
 REG_VA_NAX      			 = 0x01C8, // Waveform data that should be read next
 REG_A_ESA         			 = 0x02E0, //Address: Top address of working area for effects processing
 R_FB_SRC_A       		  	 = 0x02E4, // Feedback Source A
 R_FB_SRC_B       			 = 0x02E8, // Feedback Source B
R_IIR_DEST_A0       			 = 0x02EC,
R_IIR_DEST_A1       			 = 0x02F0,
R_ACC_SRC_A0       			 = 0x02F4,
R_ACC_SRC_A1       			 = 0x02F8,
R_ACC_SRC_B0       			 = 0x02FC,
R_ACC_SRC_B1       			 = 0x0300,
R_IIR_SRC_A0       			 = 0x0304,
R_IIR_SRC_A1       			 = 0x0308,
R_IIR_DEST_B0       			 = 0x030C,
R_IIR_DEST_B1       			 = 0x0310,
R_ACC_SRC_C0       			 = 0x0314,
R_ACC_SRC_C1       			 = 0x0318,
R_ACC_SRC_D0       			 = 0x031C,
R_ACC_SRC_D1       			 = 0x0320,
R_IIR_SRC_B1       			 = 0x0324,
R_IIR_SRC_B0       			 = 0x0328,
R_MIX_DEST_A0       			 = 0x032C,
R_MIX_DEST_A1       			 = 0x0330,
R_MIX_DEST_B0       			 = 0x0334,
R_MIX_DEST_B1       			 = 0x0338,
 REG_A_EEA         			 = 0x033C, // Address: End address of working area for effects processing (upper part of address only!)
 // end unimplemented section

 REG_C0_END1     			 = 0x0340, // End Point passed flag
 REG_C0_END2      			 = 0x0342,
 REG_C0_SPUSTAT 			 = 0x0344, // Status register?

 // core 1 has the same registers with 0x400 added, and ends at 0x746.
 REG_C1_FMOD1   			 = 0x0580,
 REG_C1_FMOD2   			 = 0x0582,
 REG_C1_VMIXL1  			 = 0x0588,
 REG_C1_VMIXL2  			 = 0x058A,
 REG_C1_VMIXR1   			 = 0x0590,
 REG_C1_VMIXR2 			 = 0x0592,
 REG_C1_MMIX    			 = 0x0598,
 REG_C1_CTRL      			 = 0x059A,
 REG_C1_IRQA_HI  			 = 0x059C,
 REG_C1_IRQA_LO 			 = 0x059E,
 REG_C1_SPUON1  			 = 0x05A0,
 REG_C1_SPUON2  			 = 0x05A2,
 REG_C1_SPUOFF1 			 = 0x05A4,
 REG_C1_SPUOFF2 			 = 0x05A6,
 REG_C1_SPUADDR_HI 		 = 0x05A8,
 REG_C1_SPUADDR_LO		 = 0x05AA,
 REG_C1_SPUDATA  			 = 0x05AC,
 REG_C1_DMACTRL  			 = 0x05AE, // unimplemented
 REG_C1_ADMAS    			 = 0x05B0,
 REG_C1_END1      			 = 0x0740,
 REG_C1_END2     			 = 0x0742,
 REG_C1_SPUSTAT  			 = 0x0744,

 // Interesting to note that *most* of the volume controls aren't implemented in Zerospu2.
 REG_P_MVOLL     			 = 0x0760, // Master Volume Left - unimplemented
 REG_P_MVOLR    			 = 0x0762, // Master Volume Right - unimplemented
 REG_P_EVOLL     			 = 0x0764, // Effect Volume Left - unimplemented
 REG_P_EVOLR     			 = 0x0766, // Effect Volume Right - unimplemented
 REG_P_AVOLL      			 = 0x0768, // Core External Input Volume Left  (Only Core 1) - unimplemented
 REG_P_AVOLR     			 = 0x076A, // Core External Input Volume Right (Only Core 1) - unimplemented
 REG_C0_BVOLL     			 = 0x076C, // Sound Data Volume Left
 REG_C0_BVOLR     			 = 0x076E, // Sound Data Volume Right
 REG_P_MVOLXL   			 = 0x0770, // Current Master Volume Left - unimplemented
 REG_P_MVOLXR   			 = 0x0772, // Current Master Volume Right - unimplemented

 // Another unimplemented section
 R_IIR_ALPHA   				 = 0x0774, // IIR alpha (% used)
 R_ACC_COEF_A   			 = 0x0776,
 R_ACC_COEF_B   			 = 0x0778,
 R_ACC_COEF_C   			 = 0x077A,
 R_ACC_COEF_D   			 = 0x077C,
 R_IIR_COEF   				 = 0x077E,
 R_FB_ALPHA   				 = 0x0780, // feedback alpha (% used)
 R_FB_X   					 = 0x0782, // feedback
 R_IN_COEF_L   				 = 0x0784,
 R_IN_COEF_R   			 = 0x0786,
  // end unimplemented section

 REG_C1_BVOLL    			 = 0x0794,
 REG_C1_BVOLR   			 = 0x0796,

 SPDIF_OUT          			 = 0x07C0, // SPDIF Out: OFF/'PCM'/Bitstream/Bypass - unimplemented
 REG_IRQINFO     			 = 0x07C2,
 SPDIF_MODE      			 = 0x07C6, // unimplemented
 SPDIF_MEDIA     			 = 0x07C8, // SPDIF Media: 'CD'/DVD - unimplemented
 SPDIF_COPY_PROT     		 = 0x07CC  // SPDIF Copy Protection - unimplemented
 // NOTE: SPDIF_COPY is defined in Linux kernel headers as 0x0004.
};

// Not sure if this is a good idea, but I don't feel like retyping it, so I'll leave it in for now.
 /*struct core_registers
 {
	 u16 pad[179];

	 u16 fmod1; u16 pad1;
	 u16 fmod2; u16 pad2;
	 u16 s_non; u16 pad3[3];
	 u16 vmix_l1; u16 pad4;
	 u16 vmix_l2; u16 pad5;
	 u16 vmixel; u16 pad6[3];
	 u16 vmixr1; u16 pad7;
	 u16 vmixr2; u16 pad8;
	 u16 vmixer; u16 pad9[3];
	 u16 mmix; u16 pad10;
	 u16 attr; u16 pad11;
	 u16 irqa_hi; u16 pad12;
	 u16 irqa_lo; u16 pad13;
	 u16 spu_on_1; u16 pad14;
	 u16 spu_on_2; u16 pad15;
	 u16 spu_off_1; u16 pad16;
	 u16 spu_off_2; u16 pad17;
	 u16 spu_addr_hi; u16 pad18;
	 u16 spu_addr_lo; u16 pad19;
	 u16 spu_data; u16 pad20;
	 u16 dma_ctrl; u16 pad21;
	 u16 admas; u16 pad22[16];

	 // Not complete past this point (not padded properly)
	 u16 va_ssa; u16 pad23[3];	//0x01C0, // Waveform data starting address
 	 u16 va_lsax; u16 pad24[3];	//0x01C4, // Loop point address
 	 u16 va_nax; u16 pad25;		//0x01C8, // Waveform data that should be read next
	 u16  a_esa; u16 pad26;		//0x02E0, //Address: Top address of working area for effects processing
	 u16  fb_src_a; u16 pad27;	//0x02E4, // Feedback Source A
	 u16  fb_sra_b; u16 pad28;	//0x02E8, // Feedback Source B
	 u16 iir_dest_a0; u16 pad29;	//0x02EC,
	 u16 iir_dest_a1; u16 pad30;	//0x02F0,
	 u16 acc_src_a0; u16 pad31;	//0x02F4,
	 u16 acc_src_a1; u16 pad32;	//0x02F8,
	 u16 acc_src_b0; u16 pad33;	//0x02FC,
	 u16 acc_src_b1; u16 pad34;	//0x0300,
	 u16 iir_src_a0; u16 pad35;	//0x0304,
	 u16 iir_src_a1; u16 pad36;	//0x0308,
	 u16 iir_dest_b0; u16 pad37;	//0x030C,
	 u16 iir_dest_b1; u16 pad38; 	//0x0310,
	 u16 acc_src_c0; u16 pad39;	//0x0314,
	 u16 acc_src_c1; u16 pad40;	//0x0318,
	 u16 acc_src_d0; u16 pad41;	//0x031C,
	 u16 acc_src_d1; u16 pad42;	// 0x0320,
	 u16 iir_src_b1; u16 pad43;	//0x0324,
	 u16 iir_src_b0; u16 pad44;	//0x0328,
	 u16 mix_dest_a0; u16 pad45;	//0x032C,
	 u16 mix_dest_a1; u16 pad46;	//0x0330,
	 u16 mix_dest_b0; u16 pad47;	//0x0334,
	 u16 mix_dest_b1;  u16 pad48;	// 0x0338,
 	 u16 a_eea; u16 pad49;	//0x033C,
	 u16 end1; u16 pad 50;	//0x0340, // End Point passed flag
	 u16 end2; u16 pad 51; 	//0x0342,
	 u16 statx; //0x0344, // Status register?
 };*/

// These SPDIF defines aren't used yet - swiped from spu2ghz, like a number of the registers I added in.
// -- arcum42
#define SPDIF_OUT_OFF        0x0000		//no spdif output
#define SPDIF_OUT_PCM        0x0020		//encode spdif from spu2 pcm output
#define SPDIF_OUT_BYPASS     0x0100		//bypass spu2 processing

#define SPDIF_MODE_BYPASS_BITSTREAM 0x0002	//bypass mode for digital bitstream data
#define SPDIF_MODE_BYPASS_PCM       0x0000	//bypass mode for pcm data (using analog output)

#define SPDIF_MODE_MEDIA_CD  0x0800		//source media is a CD
#define SPDIF_MODE_MEDIA_DVD 0x0000		//source media is a DVD

#define SPDIF_MEDIA_CDVD     0x0200
#define SPDIF_MEDIA_400      0x0000

#define SPDIF_COPY_NORMAL      0x0000	// spdif stream is not protected
#define SPDIF_COPY_PROHIBIT    0x8000	// spdif stream can't be copied

#define SPU_AUTODMA_ONESHOT 0  //spu2
#define SPU_AUTODMA_LOOP 1   //spu2
#define SPU_AUTODMA_START_ADDR (1 << 1)   //spu2

#endif
