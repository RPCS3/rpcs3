//------------------------------------------------------------------------------
// File: DVDMedia.h
//
// Desc: Contains typedefs and defines necessary for user mode (ring 3) DVD
//       filters and applications.
//
//       This should be included in the DirectShow SDK for user mode filters.
//       The types defined here should be kept in synch with ksmedia.h WDM
//       DDK for kernel mode filters.
//
// Copyright (c) 1997 - 2001, Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------


#ifndef __DVDMEDIA_H__
#define __DVDMEDIA_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// -----------------------------------------------------------------------
// AC-3 definition for the AM_KSPROPSETID_AC3 property set
// -----------------------------------------------------------------------

typedef enum {
    AM_PROPERTY_AC3_ERROR_CONCEALMENT = 1,
    AM_PROPERTY_AC3_ALTERNATE_AUDIO = 2,
    AM_PROPERTY_AC3_DOWNMIX = 3,
    AM_PROPERTY_AC3_BIT_STREAM_MODE = 4,
    AM_PROPERTY_AC3_DIALOGUE_LEVEL = 5,
    AM_PROPERTY_AC3_LANGUAGE_CODE = 6,
    AM_PROPERTY_AC3_ROOM_TYPE = 7
} AM_PROPERTY_AC3;

typedef struct  {
    BOOL        fRepeatPreviousBlock;
    BOOL        fErrorInCurrentBlock;
} AM_AC3_ERROR_CONCEALMENT, *PAM_AC3_ERROR_CONCEALMENT;

typedef struct {
    BOOL    fStereo;
    ULONG   DualMode;
} AM_AC3_ALTERNATE_AUDIO, *PAM_AC3_ALTERNATE_AUDIO;

#define AM_AC3_ALTERNATE_AUDIO_1     1
#define AM_AC3_ALTERNATE_AUDIO_2     2
#define AM_AC3_ALTERNATE_AUDIO_BOTH  3

typedef struct {
    BOOL        fDownMix;
    BOOL        fDolbySurround;
} AM_AC3_DOWNMIX, *PAM_AC3_DOWNMIX;

typedef struct {
    LONG        BitStreamMode;
} AM_AC3_BIT_STREAM_MODE, *PAM_AC3_BIT_STREAM_MODE;

#define AM_AC3_SERVICE_MAIN_AUDIO            0
#define AM_AC3_SERVICE_NO_DIALOG             1
#define AM_AC3_SERVICE_VISUALLY_IMPAIRED     2
#define AM_AC3_SERVICE_HEARING_IMPAIRED      3
#define AM_AC3_SERVICE_DIALOG_ONLY           4
#define AM_AC3_SERVICE_COMMENTARY            5
#define AM_AC3_SERVICE_EMERGENCY_FLASH       6
#define AM_AC3_SERVICE_VOICE_OVER            7

typedef struct {
    ULONG   DialogueLevel;
} AM_AC3_DIALOGUE_LEVEL, *PAM_AC3_DIALOGUE_LEVEL;

typedef struct {
    BOOL    fLargeRoom;
} AM_AC3_ROOM_TYPE, *PAM_AC3_ROOM_TYPE;


// -----------------------------------------------------------------------
// subpicture definition for the AM_KSPROPSETID_DvdSubPic property set
// -----------------------------------------------------------------------

typedef enum {
    AM_PROPERTY_DVDSUBPIC_PALETTE = 0,
    AM_PROPERTY_DVDSUBPIC_HLI = 1,
    AM_PROPERTY_DVDSUBPIC_COMPOSIT_ON = 2  // TRUE for subpicture is displayed
} AM_PROPERTY_DVDSUBPIC;

typedef struct _AM_DVD_YUV {
    UCHAR   Reserved;
    UCHAR   Y;
    UCHAR   U;
    UCHAR   V;
} AM_DVD_YUV, *PAM_DVD_YUV;

typedef struct _AM_PROPERTY_SPPAL {
    AM_DVD_YUV sppal[16];
} AM_PROPERTY_SPPAL, *PAM_PROPERTY_SPPAL;

typedef struct _AM_COLCON {
    UCHAR emph1col:4;
    UCHAR emph2col:4;
    UCHAR backcol:4;
    UCHAR patcol:4;
    UCHAR emph1con:4;
    UCHAR emph2con:4;
    UCHAR backcon:4;
    UCHAR patcon:4;

} AM_COLCON, *PAM_COLCON;

typedef struct _AM_PROPERTY_SPHLI {
    USHORT     HLISS;      //
    USHORT     Reserved;
    ULONG      StartPTM;   // start presentation time in x/90000
    ULONG      EndPTM;     // end PTM in x/90000
    USHORT     StartX;
    USHORT     StartY;
    USHORT     StopX;
    USHORT     StopY;
    AM_COLCON  ColCon;     // color contrast description (4 bytes as given in HLI)
} AM_PROPERTY_SPHLI, *PAM_PROPERTY_SPHLI;

typedef BOOL AM_PROPERTY_COMPOSIT_ON, *PAM_PROPERTY_COMPOSIT_ON;



// -----------------------------------------------------------------------
// copy protection definitions
// -----------------------------------------------------------------------

// AM_UseNewCSSKey for the dwTypeSpecificFlags in IMediaSample2 to indicate
// the exact point in a stream after which to start applying a new CSS key.
// This is typically sent on an empty media sample just before attempting
// to renegotiate a CSS key.
#define AM_UseNewCSSKey    0x1

//
// AM_KSPROPSETID_CopyProt property set definitions
//
typedef enum {
    AM_PROPERTY_DVDCOPY_CHLG_KEY = 0x01,
    AM_PROPERTY_DVDCOPY_DVD_KEY1 = 0x02,
    AM_PROPERTY_DVDCOPY_DEC_KEY2 = 0x03,
    AM_PROPERTY_DVDCOPY_TITLE_KEY = 0x04,
    AM_PROPERTY_COPY_MACROVISION = 0x05,
    AM_PROPERTY_DVDCOPY_REGION = 0x06,
    AM_PROPERTY_DVDCOPY_SET_COPY_STATE = 0x07,
    AM_PROPERTY_DVDCOPY_DISC_KEY = 0x80
} AM_PROPERTY_DVDCOPYPROT;

typedef struct _AM_DVDCOPY_CHLGKEY {
    BYTE ChlgKey[10];
    BYTE Reserved[2];
} AM_DVDCOPY_CHLGKEY, *PAM_DVDCOPY_CHLGKEY;

typedef struct _AM_DVDCOPY_BUSKEY {
    BYTE BusKey[5];
    BYTE Reserved[1];
} AM_DVDCOPY_BUSKEY, *PAM_DVDCOPY_BUSKEY;

typedef struct _AM_DVDCOPY_DISCKEY {
    BYTE DiscKey[2048];
} AM_DVDCOPY_DISCKEY, *PAM_DVDCOPY_DISCKEY;

typedef struct AM_DVDCOPY_TITLEKEY {
    ULONG KeyFlags;
    ULONG Reserved1[2];
    UCHAR TitleKey[6];
    UCHAR Reserved2[2];
} AM_DVDCOPY_TITLEKEY, *PAM_DVDCOPY_TITLEKEY;

typedef struct _AM_COPY_MACROVISION {
    ULONG MACROVISIONLevel;
} AM_COPY_MACROVISION, *PAM_COPY_MACROVISION;

typedef struct AM_DVDCOPY_SET_COPY_STATE {
    ULONG DVDCopyState;
} AM_DVDCOPY_SET_COPY_STATE, *PAM_DVDCOPY_SET_COPY_STATE;

typedef enum {
    AM_DVDCOPYSTATE_INITIALIZE = 0,
    AM_DVDCOPYSTATE_INITIALIZE_TITLE = 1,   // indicates we are starting a title
                                        // key copy protection sequence
    AM_DVDCOPYSTATE_AUTHENTICATION_NOT_REQUIRED = 2,
    AM_DVDCOPYSTATE_AUTHENTICATION_REQUIRED = 3,
    AM_DVDCOPYSTATE_DONE = 4
} AM_DVDCOPYSTATE;

typedef enum {
    AM_MACROVISION_DISABLED = 0,
    AM_MACROVISION_LEVEL1 = 1,
    AM_MACROVISION_LEVEL2 = 2,
    AM_MACROVISION_LEVEL3 = 3
} AM_COPY_MACROVISION_LEVEL, *PAM_COPY_MACROVISION_LEVEL;


// CSS region stucture
typedef struct _DVD_REGION {
    UCHAR CopySystem;
    UCHAR RegionData;
    UCHAR SystemRegion;
    UCHAR Reserved;
} DVD_REGION, *PDVD_REGION;

//
// CGMS Copy Protection Flags
//

#define AM_DVD_CGMS_RESERVED_MASK      0x00000078

#define AM_DVD_CGMS_COPY_PROTECT_MASK  0x00000018
#define AM_DVD_CGMS_COPY_PERMITTED     0x00000000
#define AM_DVD_CGMS_COPY_ONCE          0x00000010
#define AM_DVD_CGMS_NO_COPY            0x00000018

#define AM_DVD_COPYRIGHT_MASK          0x00000040
#define AM_DVD_NOT_COPYRIGHTED         0x00000000
#define AM_DVD_COPYRIGHTED             0x00000040

#define AM_DVD_SECTOR_PROTECT_MASK     0x00000020
#define AM_DVD_SECTOR_NOT_PROTECTED    0x00000000
#define AM_DVD_SECTOR_PROTECTED        0x00000020


// -----------------------------------------------------------------------
// video format blocks
// -----------------------------------------------------------------------

enum AM_MPEG2Level {
    AM_MPEG2Level_Low = 1,
    AM_MPEG2Level_Main = 2,
    AM_MPEG2Level_High1440 = 3,
    AM_MPEG2Level_High = 4
};

enum AM_MPEG2Profile {
    AM_MPEG2Profile_Simple = 1,
    AM_MPEG2Profile_Main = 2,
    AM_MPEG2Profile_SNRScalable = 3,
    AM_MPEG2Profile_SpatiallyScalable = 4,
    AM_MPEG2Profile_High = 5
};

#define AMINTERLACE_IsInterlaced            0x00000001  // if 0, other interlace bits are irrelevent
#define AMINTERLACE_1FieldPerSample         0x00000002  // else 2 fields per media sample
#define AMINTERLACE_Field1First             0x00000004  // else Field 2 is first;  top field in PAL is field 1, top field in NTSC is field 2?
#define AMINTERLACE_UNUSED                  0x00000008  //
#define AMINTERLACE_FieldPatternMask        0x00000030  // use this mask with AMINTERLACE_FieldPat*
#define AMINTERLACE_FieldPatField1Only      0x00000000  // stream never contains a Field2
#define AMINTERLACE_FieldPatField2Only      0x00000010  // stream never contains a Field1
#define AMINTERLACE_FieldPatBothRegular     0x00000020  // There will be a Field2 for every Field1 (required for Weave?)
#define AMINTERLACE_FieldPatBothIrregular   0x00000030  // Random pattern of Field1s and Field2s
#define AMINTERLACE_DisplayModeMask         0x000000c0
#define AMINTERLACE_DisplayModeBobOnly      0x00000000
#define AMINTERLACE_DisplayModeWeaveOnly    0x00000040
#define AMINTERLACE_DisplayModeBobOrWeave   0x00000080

#define AMCOPYPROTECT_RestrictDuplication   0x00000001  // duplication of this stream should be restricted

#define AMMPEG2_DoPanScan           0x00000001  //if set, the MPEG-2 video decoder should crop output image
                        //  based on pan-scan vectors in picture_display_extension
                        //  and change the picture aspect ratio accordingly.
#define AMMPEG2_DVDLine21Field1     0x00000002  //if set, the MPEG-2 decoder must be able to produce an output
                        //  pin for DVD style closed caption data found in GOP layer of field 1
#define AMMPEG2_DVDLine21Field2     0x00000004  //if set, the MPEG-2 decoder must be able to produce an output
                        //  pin for DVD style closed caption data found in GOP layer of field 2
#define AMMPEG2_SourceIsLetterboxed 0x00000008  //if set, indicates that black bars have been encoded in the top
                        //  and bottom of the video.
#define AMMPEG2_FilmCameraMode      0x00000010  //if set, indicates "film mode" used for 625/50 content.  If cleared,
                        //  indicates that "camera mode" was used.
#define AMMPEG2_LetterboxAnalogOut  0x00000020  //if set and this stream is sent to an analog output, it should
                        //  be letterboxed.  Streams sent to VGA should be letterboxed only by renderers.
#define AMMPEG2_DSS_UserData        0x00000040  //if set, the MPEG-2 decoder must process DSS style user data
#define AMMPEG2_DVB_UserData        0x00000080  //if set, the MPEG-2 decoder must process DVB style user data
#define AMMPEG2_27MhzTimebase       0x00000100  //if set, the PTS,DTS timestamps advance at 27MHz rather than 90KHz

#define AMMPEG2_WidescreenAnalogOut 0x00000200  //if set and this stream is sent to an analog output, it should
                        //  be in widescreen format (4x3 content should be centered on a 16x9 output).
                        //  Streams sent to VGA should be widescreened only by renderers.

// PRESENT in dwReserved1 field in VIDEOINFOHEADER2
#define AMCONTROL_USED              0x00000001 // Used to test if these flags are supported.  Set and test for AcceptMediaType.
                                                // If rejected, then you cannot use the AMCONTROL flags (send 0 for dwReserved1)
#define AMCONTROL_PAD_TO_4x3        0x00000002 // if set means display the image in a 4x3 area
#define AMCONTROL_PAD_TO_16x9       0x00000004 // if set means display the image in a 16x9 area

typedef struct tagVIDEOINFOHEADER2 {
    RECT                rcSource;
    RECT                rcTarget;
    DWORD               dwBitRate;
    DWORD               dwBitErrorRate;
    REFERENCE_TIME      AvgTimePerFrame;
    DWORD               dwInterlaceFlags;   // use AMINTERLACE_* defines. Reject connection if undefined bits are not 0
    DWORD               dwCopyProtectFlags; // use AMCOPYPROTECT_* defines. Reject connection if undefined bits are not 0
    DWORD               dwPictAspectRatioX; // X dimension of picture aspect ratio, e.g. 16 for 16x9 display
    DWORD               dwPictAspectRatioY; // Y dimension of picture aspect ratio, e.g.  9 for 16x9 display
    union {
        DWORD dwControlFlags;               // use AMCONTROL_* defines, use this from now on
        DWORD dwReserved1;                  // for backward compatiblity (was "must be 0";  connection rejected otherwise)
    };
    DWORD               dwReserved2;        // must be 0; reject connection otherwise
    BITMAPINFOHEADER    bmiHeader;
} VIDEOINFOHEADER2;

typedef struct tagMPEG2VIDEOINFO {
    VIDEOINFOHEADER2    hdr;
    DWORD               dwStartTimeCode;        //  ?? not used for DVD ??
    DWORD               cbSequenceHeader;       // is 0 for DVD (no sequence header)
    DWORD               dwProfile;              // use enum MPEG2Profile
    DWORD               dwLevel;                // use enum MPEG2Level
    DWORD               dwFlags;                // use AMMPEG2_* defines.  Reject connection if undefined bits are not 0
    DWORD               dwSequenceHeader[1];    // DWORD instead of Byte for alignment purposes
                                                //   For MPEG-2, if a sequence_header is included, the sequence_extension
                                                //   should also be included
} MPEG2VIDEOINFO;

#define SIZE_MPEG2VIDEOINFO(pv) (FIELD_OFFSET(MPEG2VIDEOINFO, dwSequenceHeader[0]) + (pv)->cbSequenceHeader)

// do not use
#define MPEG1_SEQUENCE_INFO(pv) ((const BYTE *)(pv)->bSequenceHeader)

// use this macro instead, the previous only works for MPEG1VIDEOINFO structures
#define MPEG2_SEQUENCE_INFO(pv) ((const BYTE *)(pv)->dwSequenceHeader)


//===================================================================================
// flags for dwTypeSpecificFlags in AM_SAMPLE2_PROPERTIES which define type specific
// data in IMediaSample2
//===================================================================================

#define AM_VIDEO_FLAG_FIELD_MASK        0x0003L // use this mask to check whether the sample is field1 or field2 or frame
#define AM_VIDEO_FLAG_INTERLEAVED_FRAME 0x0000L // the sample is a frame (remember to use AM_VIDEO_FLAG_FIELD_MASK when using this)
#define AM_VIDEO_FLAG_FIELD1            0x0001L // the sample is field1 (remember to use AM_VIDEO_FLAG_FIELD_MASK when using this)
#define AM_VIDEO_FLAG_FIELD2            0x0002L // the sample is the field2 (remember to use AM_VIDEO_FLAG_FIELD_MASK when using this)
#define AM_VIDEO_FLAG_FIELD1FIRST       0x0004L // if set means display field1 first, else display field2 first.
                                                // this bit is irrelavant for 1FieldPerSample mode
#define AM_VIDEO_FLAG_WEAVE             0x0008L // if set use bob display mode else weave
#define AM_VIDEO_FLAG_IPB_MASK          0x0030L // use this mask to check whether the sample is I, P or B
#define AM_VIDEO_FLAG_I_SAMPLE          0x0000L // I Sample (remember to use AM_VIDEO_FLAG_IPB_MASK when using this)
#define AM_VIDEO_FLAG_P_SAMPLE          0x0010L // P Sample (remember to use AM_VIDEO_FLAG_IPB_MASK when using this)
#define AM_VIDEO_FLAG_B_SAMPLE          0x0020L // B Sample (remember to use AM_VIDEO_FLAG_IPB_MASK when using this)
#define AM_VIDEO_FLAG_REPEAT_FIELD      0x0040L // if set means display the field which has been displayed first again after displaying
                                                // both fields first. This bit is irrelavant for 1FieldPerSample mode

// -----------------------------------------------------------------------
// AM_KSPROPSETID_DvdKaraoke property set definitions
// -----------------------------------------------------------------------

typedef struct tagAM_DvdKaraokeData
{
    DWORD   dwDownmix;              // bitwise OR of AM_DvdKaraoke_Downmix flags
    DWORD   dwSpeakerAssignment;    // AM_DvdKaraoke_SpeakerAssignment
} AM_DvdKaraokeData;

typedef enum {
    AM_PROPERTY_DVDKARAOKE_ENABLE = 0,  // BOOL
    AM_PROPERTY_DVDKARAOKE_DATA = 1,
} AM_PROPERTY_DVDKARAOKE;

// -----------------------------------------------------------------------
// AM_KSPROPSETID_TSRateChange property set definitions for time stamp
// rate changes.
// -----------------------------------------------------------------------

typedef enum {
    AM_RATE_SimpleRateChange = 1,    // rw, use AM_SimpleRateChange
    AM_RATE_ExactRateChange  = 2,    // rw, use AM_ExactRateChange
    AM_RATE_MaxFullDataRate  = 3,    // r,  use AM_MaxFullDataRate
    AM_RATE_Step             = 4,    // w,  use AM_Step
    AM_RATE_UseRateVersion   = 5,       //  w, use WORD
    AM_RATE_QueryFullFrameRate =6,      //  r, use AM_QueryRate
    AM_RATE_QueryLastRateSegPTS =7,     //  r, use REFERENCE_TIME
    AM_RATE_CorrectTS        = 8     // w,  use LONG
} AM_PROPERTY_TS_RATE_CHANGE;

// -------------------------------------------------------------------
// AM_KSPROPSETID_DVD_RateChange property set definitions for new DVD
// rate change scheme.
// -------------------------------------------------------------------

typedef enum {
    AM_RATE_ChangeRate       = 1,    // w,  use AM_DVD_ChangeRate
    AM_RATE_FullDataRateMax  = 2,    // r,  use AM_MaxFullDataRate
    AM_RATE_ReverseDecode    = 3,    // r,  use LONG
    AM_RATE_DecoderPosition  = 4,    // r,  use AM_DVD_DecoderPosition
    AM_RATE_DecoderVersion   = 5     // r,  use LONG
} AM_PROPERTY_DVD_RATE_CHANGE;

typedef struct {
    // this is the simplest mechanism to set a time stamp rate change on
    // a filter (simplest for the person setting the rate change, harder
    // for the filter doing the rate change).
    REFERENCE_TIME  StartTime;  //stream time at which to start this rate
    LONG        Rate;       //new rate * 10000 (decimal)
} AM_SimpleRateChange;

typedef struct {
    LONG    lMaxForwardFullFrame ;          //  rate * 10000
    LONG    lMaxReverseFullFrame ;          //  rate * 10000
} AM_QueryRate ;

typedef struct {
    REFERENCE_TIME  OutputZeroTime; //input TS that maps to zero output TS
    LONG        Rate;       //new rate * 10000 (decimal)
} AM_ExactRateChange;

typedef LONG AM_MaxFullDataRate; //rate * 10000 (decimal)

typedef DWORD AM_Step; // number of frame to step

// New rate change property set, structs. enums etc.
typedef struct {
    REFERENCE_TIME  StartInTime;   // stream time (input) at which to start decoding at this rate
    REFERENCE_TIME  StartOutTime;  // reference time (output) at which to start showing at this rate
    LONG            Rate;          // new rate * 10000 (decimal)
} AM_DVD_ChangeRate ;

typedef LONGLONG  AM_DVD_DecoderPosition ;

typedef enum {
    DVD_DIR_FORWARD  = 0,
    DVD_DIR_BACKWARD = 1
} DVD_PLAY_DIRECTION ;

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // __DVDMEDIA_H__
