//------------------------------------------------------------------------------
// File: DVDevCod.h
//
// Desc: List of standard DVD-Video event codes and the expected params.
//
// Copyright (c) 1992 - 2001, Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

#ifndef __DVDEVCOD__
#define __DVDEVCOD__


#define EC_DVDBASE                          0x0100

#ifndef EXCLUDE_DVDEVCODE_ENUMS

typedef enum _tagDVD_ERROR {
    DVD_ERROR_Unexpected=1,         // Something unexpected happened, perhaps content
                                    //   is incorrectly authored.  Playback is stopped.
    DVD_ERROR_CopyProtectFail=2,    // Key exchange for DVD copy protection failed. 
                                    //   Playback is stopped.
    DVD_ERROR_InvalidDVD1_0Disc=3,  // DVD-Video disc is incorrectly authored for v1.0 
                                    //   of spec. Playback is stopped.
    DVD_ERROR_InvalidDiscRegion=4,  // The Disc cannot be played because the disc is not
                                    // authored to play in system region.
                                    // The region mismatch may be fixable by 
                                    // changing the system region with dvdrgn.exe
    DVD_ERROR_LowParentalLevel=5,   // Player parental level is lower than the lowest parental
                                    //   level available in the DVD content. Playback is stopped.
    DVD_ERROR_MacrovisionFail=6,    // Macrovision Distribution Failed.
                                    // Playback is stopped.
    DVD_ERROR_IncompatibleSystemAndDecoderRegions=7,
                                    // No discs can be played because the system region
                                    // does not match the decoder region.
    DVD_ERROR_IncompatibleDiscAndDecoderRegions=8
                                    // The disc cannot be played because the disc is
                                    // not authored to be played in the decoder's region
} DVD_ERROR;

typedef enum _tagDVD_WARNING {
    DVD_WARNING_InvalidDVD1_0Disc=1,// DVD-Video disc is incorrectly authored. Playback
                                    //   can continue, but unexpected behavior may occur.
    DVD_WARNING_FormatNotSupported=2,// A decoder would not support the current format.  Playback
                                    //   of a stream (audio, video of SP) may not function.
                                    //   lParam2 contains the stream type (see AM_DVD_STREAM_FLAGS)
    DVD_WARNING_IllegalNavCommand=3,// The internal DVD navigation command processor attempted to 
                                    //   process an illegal command.
    DVD_WARNING_Open = 4,           // File Open Failed
    DVD_WARNING_Seek = 5,           // File Seek Failed
    DVD_WARNING_Read = 6            // File Read Failed
} DVD_WARNING;

typedef enum _tagDVD_PB_STOPPED {
    DVD_PB_STOPPED_Other=0,                 // The navigator stopped the playback (no reason available).
    DVD_PB_STOPPED_NoBranch=1,              // The nav completed the current pgc and there was no more video and
                                            // did not find any other branching instruction for subsequent playback.
    DVD_PB_STOPPED_NoFirstPlayDomain =2,    // The disc does not contain an initial startup program.
    DVD_PB_STOPPED_StopCommand = 3,         // The app issued a stop() command or a stop command was authored on the disc.
    DVD_PB_STOPPED_Reset=4,                 // The navigator was reset to the start of the disc (using ResetOnStop).
    DVD_PB_STOPPED_DiscEjected=5,           // The disc was ejected.
    DVD_PB_STOPPED_IllegalNavCommand = 6,   // An illegal nav command prevented playback from continuing.
    DVD_PB_STOPPED_PlayPeriodAutoStop = 7,  // PlayPeriod completed
    DVD_PB_STOPPED_PlayChapterAutoStop = 8, // PlayChapter completed
    DVD_PB_STOPPED_ParentalFailure = 9,     // A parental level failure prevented playback
    DVD_PB_STOPPED_RegionFailure = 10,      // A region failure prevented playback
    DVD_PB_STOPPED_MacrovisionFailure = 11, // A Macrovision failure prevented playback.
    DVD_PB_STOPPED_DiscReadError = 12,      // A read error prevented playback.
    DVD_PB_STOPPED_CopyProtectFailure = 13  // Copy protection failure.
} DVD_PB_STOPPED;


#endif

// DVD-Video event codes
// ======================
//
// All DVD-Video event are always passed on to the application, and are 
// never processed by the filter graph


#define EC_DVD_DOMAIN_CHANGE                    (EC_DVDBASE + 0x01)
// Parameters: ( DWORD, void ) 
// lParam1 is enum DVD_DOMAIN, and indicates the player's new domain
//
// Raised from following domains: all
//
// Signaled when ever the DVD player changes domains.


#define EC_DVD_TITLE_CHANGE                     (EC_DVDBASE + 0x02)
// Parameters: ( DWORD, void ) 
// lParam1 is the new title number.
//
// Raised from following domains: DVD_DOMAIN_Title
//
// Indicates when the current title number changes.  Title numbers
// range 1 to 99.  This indicates the TTN, which is the title number
// with respect to the whole disc, not the VTS_TTN which is the title
// number with respect to just a current VTS.


#define EC_DVD_CHAPTER_START                   (EC_DVDBASE + 0x03)
// Parameters: ( DWORD, void ) 
// lParam1 is the new chapter number (which is the program number for 
// One_Sequential_PGC_Titles).
//
// Raised from following domains: DVD_DOMAIN_Title
//
// Signales that DVD player started playback of a new program in the Title 
// domain.  This is only signaled for One_Sequential_PGC_Titles.


#define EC_DVD_AUDIO_STREAM_CHANGE              (EC_DVDBASE + 0x04)
// Parameters: ( DWORD, void ) 
// lParam1 is the new user audio stream number.
//
// Raised from following domains: all
//
// Signaled when ever the current user audio stream number changes for the main 
// title.  This can be changed automatically with a navigation command on disc
// as well as through IDVDAnnexJ.
// Audio stream numbers range from 0 to 7.  Stream 0xffffffff
// indicates that no stream is selected.

#define EC_DVD_SUBPICTURE_STREAM_CHANGE         (EC_DVDBASE + 0x05)
// Parameters: ( DWORD, BOOL ) 
// lParam1 is the new user subpicture stream number.
// lParam2 is the subpicture's on/off state (TRUE if on)
//
// Raised from following domains: all
//
// Signaled when ever the current user subpicture stream number changes for the main 
// title.  This can be changed automatically with a navigation command on disc
// as well as through IDVDAnnexJ.  
// Subpicture stream numbers range from 0 to 31.  Stream 0xffffffff
// indicates that no stream is selected.  

#define EC_DVD_ANGLE_CHANGE                     (EC_DVDBASE + 0x06)
// Parameters: ( DWORD, DWORD ) 
// lParam1 is the number of available angles.
// lParam2 is the current user angle number.
//
// Raised from following domains: all
//
// Signaled when ever either 
//   a) the number of available angles changes, or  
//   b) the current user angle number changes.
// Current angle number can be changed automatically with navigation command 
// on disc as well as through IDVDAnnexJ.
// When the number of available angles is 1, the current video is not multiangle.
// Angle numbers range from 1 to 9.


#define EC_DVD_BUTTON_CHANGE                    (EC_DVDBASE + 0x07)
// Parameters: ( DWORD, DWORD ) 
// lParam1 is the number of available buttons.
// lParam2 is the current selected button number.
//
// Raised from following domains: all
//
// Signaled when ever either 
//   a) the number of available buttons changes, or  
//   b) the current selected button number changes.
// The current selected button can be changed automatically with navigation 
// commands on disc as well as through IDVDAnnexJ.  
// Button numbers range from 1 to 36.  Selected button number 0 implies that
// no button is selected.  Note that these button numbers enumerate all 
// available button numbers, and do not always correspond to button numbers
// used for IDVDAnnexJ::ButtonSelectAndActivate since only a subset of buttons
// may be activated with ButtonSelectAndActivate.


#define EC_DVD_VALID_UOPS_CHANGE                (EC_DVDBASE + 0x08)
// Parameters: ( DWORD, void ) 
// lParam1 is a VALID_UOP_SOMTHING_OR_OTHER bit-field stuct which indicates
//   which IDVDAnnexJ commands are explicitly disable by the DVD disc.
//
// Raised from following domains: all
//
// Signaled when ever the available set of IDVDAnnexJ methods changes.  This
// only indicates which operations are explicited disabled by the content on 
// the DVD disc, and does not guarentee that it is valid to call methods 
// which are not disabled.  For example, if no buttons are currently present,
// IDVDAnnexJ::ButtonActivate() won't work, even though the buttons are not
// explicitly disabled. 


#define EC_DVD_STILL_ON                         (EC_DVDBASE + 0x09)
// Parameters: ( BOOL, DWORD ) 
// lParam1 == 0  -->  buttons are available, so StillOff won't work
// lParam1 == 1  -->  no buttons available, so StillOff will work
// lParam2 indicates the number of seconds the still will last, with 0xffffffff 
//   indicating an infinite still (wait till button or StillOff selected).
//
// Raised from following domains: all
//
// Signaled at the beginning of any still: PGC still, Cell Still, or VOBU Still.
// Note that all combinations of buttons and still are possible (buttons on with
// still on, buttons on with still off, button off with still on, button off
// with still off).

#define EC_DVD_STILL_OFF                         (EC_DVDBASE + 0x0a)
// Parameters: ( void, void ) 
//
//   Indicating that any still that is currently active
//   has been released.
//
// Raised from following domains: all
//
// Signaled at the end of any still: PGC still, Cell Still, or VOBU Still.
//

#define EC_DVD_CURRENT_TIME                     (EC_DVDBASE + 0x0b)
// Parameters: ( DWORD, BOOL ) 
// lParam1 is a DVD_TIMECODE which indicates the current 
//   playback time code in a BCD HH:MM:SS:FF format.
// lParam2 == 0  -->  time code is 25 frames/sec
// lParam2 == 1  -->  time code is 30 frames/sec (non-drop).
// lParam2 == 2  -->  time code is invalid (current playback time 
//                    cannot be determined for current title)
//
// Raised from following domains: DVD_DOMAIN_Title
//
// Signaled at the beginning of every VOBU, which occurs every .4 to 1.0 sec.
// This is only signaled for One_Sequential_PGC_Titles.


#define EC_DVD_ERROR                            (EC_DVDBASE + 0x0c)
// Parameters: ( DWORD, void) 
// lParam1 is an enum DVD_ERROR which notifies the app of some error condition.
//
// Raised from following domains: all
//

#define EC_DVD_WARNING                           (EC_DVDBASE + 0x0d)
// Parameters: ( DWORD, DWORD) 
// lParam1 is an enum DVD_WARNING which notifies the app of some warning condition.
// lParam2 contains more specific information about the warning (warning dependent)
//
// Raised from following domains: all
//

#define EC_DVD_CHAPTER_AUTOSTOP                  (EC_DVDBASE + 0x0e)
// Parameters: (BOOL, void)
// lParam1 is a BOOL which indicates the reason for the cancellation of ChapterPlayAutoStop
// lParam1 == 0 indicates successful completion of ChapterPlayAutoStop
// lParam1 == 1 indicates that ChapterPlayAutoStop is being cancelled as a result of another
//            IDVDControl call or the end of content has been reached & no more chapters
//            can be played.
//  Indicating that playback is stopped as a result of a call
//  to IDVDControl::ChapterPlayAutoStop()
//
// Raised from following domains : DVD_DOMAIN_TITLE
//

#define EC_DVD_NO_FP_PGC                         (EC_DVDBASE + 0x0f)
//  Parameters : (void, void)
//
//  Raised from the following domains : FP_DOM
//
//  Indicates that the DVD disc does not have a FP_PGC (First Play Program Chain)
//  and the DVD Navigator will not automatically load any PGC and start playback.
//

#define EC_DVD_PLAYBACK_RATE_CHANGE              (EC_DVDBASE + 0x10)
//  Parameters : (LONG, void)
//  lParam1 is a LONG indicating the new playback rate.
//  lParam1 < 0 indicates reverse playback mode.
//  lParam1 > 0 indicates forward playback mode
//  Value of lParam1 is the actual playback rate multiplied by 10000.
//  i.e. lParam1 = rate * 10000
//
//  Raised from the following domains : TT_DOM
//
//  Indicates that a rate change in playback has been initiated and the parameter
//  lParam1 indicates the new playback rate that is being used.
//

#define EC_DVD_PARENTAL_LEVEL_CHANGE            (EC_DVDBASE + 0x11)
//  Parameters : (LONG, void)
//  lParam1 is a LONG indicating the new parental level.
//
//  Raised from the following domains : VMGM_DOM
//
//  Indicates that an authored Nav command has changed the parental level
//  setting in the player.
//

#define EC_DVD_PLAYBACK_STOPPED                 (EC_DVDBASE + 0x12)
//  Parameters : (DWORD, void)
//
//  Raised from the following domains : All Domains
//
// Indicates that playback has been stopped as the Navigator has completed
// playback of the pgc and did not find any other branching instruction for
// subsequent playback.
//
//  The DWORD returns the reason for the completion of the playback.  See
//  The DVD_PB_STOPPED enumeration for details.
//

#define EC_DVD_ANGLES_AVAILABLE                 (EC_DVDBASE + 0x13)
//  Parameters : (BOOL, void)
//  lParam1 == 0 indicates that playback is not in an angle block and angles are
//             not available
//  lParam1 == 1 indicates that an angle block is being played back and angle changes
//             can be performed.
//
//  Indicates whether an angle block is being played and if angle changes can be 
//  performed. However, angle changes are not restricted to angle blocks and the
//  manifestation of the angle change can be seen only in an angle block.

#define EC_DVD_PLAYPERIOD_AUTOSTOP              (EC_DVDBASE + 0x14)
// Parameters: (void, void)
// Sent when the PlayPeriodInTitle completes or is cancelled
//
// Raised from following domains : DVD_DOMAIN_TITLE
//

#define EC_DVD_BUTTON_AUTO_ACTIVATED                 (EC_DVDBASE + 0x15)
// Parameters: (DWORD button, void)
// Sent when a button is automatically activated
//
// Raised from following domains : DVD_DOMAIN_MENU
//

#define EC_DVD_CMD_START                 (EC_DVDBASE + 0x16)
// Parameters: (CmdID, HRESULT)
// Sent when a command begins
//

#define EC_DVD_CMD_END                 (EC_DVDBASE + 0x17)
// Parameters: (CmdID, HRESULT)
// Sent when a command completes
//

#define EC_DVD_DISC_EJECTED                (EC_DVDBASE + 0x18)
// Parameters: none
// Sent when the nav detects that a disc was ejected and stops the playback
// The app does not need to take any action to stop the playback.
//

#define EC_DVD_DISC_INSERTED                (EC_DVDBASE + 0x19)
// Parameters: none
// Sent when the nav detects that a disc was inserted and the nav begins playback
// The app does not need to take any action to start the playback.
//

#define EC_DVD_CURRENT_HMSF_TIME                     (EC_DVDBASE + 0x1a)
// Parameters: ( ULONG, ULONG ) 
// lParam2 contains a union of the DVD_TIMECODE_FLAGS
// lParam1 contains a DVD_HMSF_TIMECODE.  Assign lParam1 to a ULONG then cast the
// ULONG as a DVD_HMSF_TIMECODE to use its values.
//
// Raised from following domains: DVD_DOMAIN_Title
//
// Signaled at the beginning of every VOBU, which occurs every .4 to 1.0 sec.

#define EC_DVD_KARAOKE_MODE                     (EC_DVDBASE + 0x1b)
// Parameters: ( BOOL, reserved ) 
// lParam1 is either TRUE (a karaoke track is being played) or FALSE (no karaoke data is being played).
//
#endif // __DVDEVCOD__
