//------------------------------------------------------------------------------
// File: EDevDefs.h
//
// Desc: External Device (such as a VCR) control interface parameter and
//       value definitions.  
//
//       Note:  new constants added:  ED_BASE+800L -> ED_BASE+811L
//
// Copyright (c) 1992-2001, Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------


#ifndef __EDEVDEFS__
#define __EDEVDEFS__


#define ED_BASE                     0x1000L

// this is used to tell the device communications object which
// physical communications port to use.
#define DEV_PORT_SIM    1
#define DEV_PORT_COM1   2   // standard serial ports
#define DEV_PORT_COM2   3
#define DEV_PORT_COM3   4
#define DEV_PORT_COM4   5
#define DEV_PORT_DIAQ   6   // Diaquest driver
#define DEV_PORT_ARTI   7   // ARTI driver
#define DEV_PORT_1394   8   // IEEE 1394 Bus
#define DEV_PORT_USB    9   // Universal Serial Bus
#define DEV_PORT_MIN    DEV_PORT_SIM
#define DEV_PORT_MAX    DEV_PORT_USB


//  IAMExtDevice Capability Items:  unless otherwise specified, these items return 
//     OATRUE or OAFALSE.  All return values are in pdwValue unless otherwise specified:

#define ED_DEVCAP_CAN_RECORD            ED_BASE+1L
#define ED_DEVCAP_CAN_RECORD_STROBE     ED_BASE+2L  // for multitrack devices: 
                                                    // switches currently recording tracks off 
                                                    // and selected non-recording tracks into record
#define ED_DEVCAP_HAS_AUDIO         ED_BASE+3L
#define ED_DEVCAP_HAS_VIDEO         ED_BASE+4L
#define ED_DEVCAP_USES_FILES        ED_BASE+5L
#define ED_DEVCAP_CAN_SAVE          ED_BASE+6L

#define ED_DEVCAP_DEVICE_TYPE       ED_BASE+7L      // returns one of the following:
#define ED_DEVTYPE_VCR              ED_BASE+8L
#define ED_DEVTYPE_LASERDISK        ED_BASE+9L
#define ED_DEVTYPE_ATR              ED_BASE+10L
#define ED_DEVTYPE_DDR              ED_BASE+11L
#define ED_DEVTYPE_ROUTER           ED_BASE+12L
#define ED_DEVTYPE_KEYER            ED_BASE+13L
#define ED_DEVTYPE_MIXER_VIDEO      ED_BASE+14L
#define ED_DEVTYPE_DVE              ED_BASE+15L
#define ED_DEVTYPE_WIPEGEN          ED_BASE+16L
#define ED_DEVTYPE_MIXER_AUDIO      ED_BASE+17L
#define ED_DEVTYPE_CG               ED_BASE+18L
#define ED_DEVTYPE_TBC              ED_BASE+19L
#define ED_DEVTYPE_TCG              ED_BASE+20L
#define ED_DEVTYPE_GPI              ED_BASE+21L
#define ED_DEVTYPE_JOYSTICK         ED_BASE+22L
#define ED_DEVTYPE_KEYBOARD         ED_BASE+23L

// returns mfr-specific ID from external device.
#define ED_DEVCAP_EXTERNAL_DEVICE_ID    ED_BASE+24L

#define ED_DEVCAP_TIMECODE_READ     ED_BASE+25L
#define ED_DEVCAP_TIMECODE_WRITE    ED_BASE+26L
//  used for seekable non-timecode enabled devices
#define ED_DEVCAP_CTLTRK_READ       ED_BASE+27L
//  used for seekable non-timecode enabled devices
#define ED_DEVCAP_INDEX_READ        ED_BASE+28L

// returns device preroll time in current time format
#define ED_DEVCAP_PREROLL           ED_BASE+29L
// returns device postroll time in current time format
#define ED_DEVCAP_POSTROLL          ED_BASE+30L

// returns indication of device’s synchronization accuracy.
#define ED_DEVCAP_SYNC_ACCURACY     ED_BASE+31L // returns one of the following:
#define ED_SYNCACC_PRECISE          ED_BASE+32L
#define ED_SYNCACC_FRAME            ED_BASE+33L
#define ED_SYNCACC_ROUGH            ED_BASE+34L

// returns device’s normal framerate.
#define ED_DEVCAP_NORMAL_RATE   ED_BASE+35L // returns one of the following:
#define ED_RATE_24              ED_BASE+36L
#define ED_RATE_25              ED_BASE+37L
#define ED_RATE_2997            ED_BASE+38L
#define ED_RATE_30              ED_BASE+39L

#define ED_DEVCAP_CAN_PREVIEW   ED_BASE+40L
#define ED_DEVCAP_CAN_MONITOR_SOURCES   ED_BASE+41L

// indicates implementation allows testing of methods/parameters by
// setting the hi bit of a parm that makes sense - see individual methods
// for details.
#define ED_DEVCAP_CAN_TEST          ED_BASE+42L
    
// indicates device accepts video as an input.
#define ED_DEVCAP_VIDEO_INPUTS      ED_BASE+43L

// indicates device accepts audio as an input.
#define ED_DEVCAP_AUDIO_INPUTS      ED_BASE+44L

#define ED_DEVCAP_NEEDS_CALIBRATING ED_BASE+45L

#define ED_DEVCAP_SEEK_TYPE         ED_BASE+46L // returns one of the following:
#define ED_SEEK_PERFECT             ED_BASE+47L // indicates device can execute seek 
                                                // within 1 video frames without signal 
                                                //  break (like a DDR)
#define ED_SEEK_FAST                ED_BASE+48L // indicates device can move pretty quick 
                                                //  with short break in signal
#define ED_SEEK_SLOW                ED_BASE+49L // seeks like a tape transport

#define ED_POWER_ON                 ED_BASE+50L
#define ED_POWER_OFF                ED_BASE+51L
#define ED_POWER_STANDBY            ED_BASE+52L

#define ED_ACTIVE                   ED_BASE+53L
#define ED_INACTIVE                 ED_BASE+54L
#define ED_ALL                      ED_BASE+55L
#define ED_TEST                     ED_BASE+56L

//  IAMExtTransport Capability Items:  unless otherwise specified, these items return 
//     OATRUE or OAFALSE.  All return values are in pdwValue unless otherwise specified:

#define ED_TRANSCAP_CAN_EJECT           ED_BASE+100L
#define ED_TRANSCAP_CAN_BUMP_PLAY       ED_BASE+101L    // variable speed for synchronizing
#define ED_TRANSCAP_CAN_PLAY_BACKWARDS  ED_BASE+102L    // servo locked for use during an edit
#define ED_TRANSCAP_CAN_SET_EE          ED_BASE+103L    // show device’s input on its output
#define ED_TRANSCAP_CAN_SET_PB          ED_BASE+104L    // show media playback on device’s output
#define ED_TRANSCAP_CAN_DELAY_VIDEO_IN  ED_BASE+105L    // transport can do delayed-in video edits
#define ED_TRANSCAP_CAN_DELAY_VIDEO_OUT ED_BASE+106L    // transport can do delayed-out video edits
#define ED_TRANSCAP_CAN_DELAY_AUDIO_IN  ED_BASE+107L    // transport can do delayed-in audio edits
#define ED_TRANSCAP_CAN_DELAY_AUDIO_OUT ED_BASE+108L    // transport can do delayed-out audio edits
#define ED_TRANSCAP_FWD_VARIABLE_MAX    ED_BASE+109L    // max forward speed (multiple of play speed) 
                                                        //  in pdblValue
#define ED_TRANSCAP_FWD_VARIABLE_MIN    ED_BASE+800L    // min forward speed (multiple of play speed) 
                                                        //  in pdblValue
#define ED_TRANSCAP_REV_VARIABLE_MAX    ED_BASE+110L    // max reverse speed (multiple of play speed) in
                                                        //  pdblValue
#define ED_TRANSCAP_REV_VARIABLE_MIN    ED_BASE+801L    // min reverse speed (multiple of play speed)
                                                        //  in pdblValue
#define ED_TRANSCAP_FWD_SHUTTLE_MAX     ED_BASE+802L    // max forward speed in Shuttle mode (multiple
                                                        //  of play speed) in pdblValue
#define ED_TRANSCAP_FWD_SHUTTLE_MIN     ED_BASE+803L    // min forward speed in Shuttle mode (multiple
                                                        //  of play speed) in pdblValue
#define ED_TRANSCAP_REV_SHUTTLE_MAX     ED_BASE+804L    // max reverse speed in Shuttle mode (multiple
                                                        //  of play speed) in pdblValue
#define ED_TRANSCAP_REV_SHUTTLE_MIN     ED_BASE+805L    // min reverse speed in Shuttle mode (multiple
                                                        //  of play speed) in pdblValue
#define ED_TRANSCAP_NUM_AUDIO_TRACKS    ED_BASE+111L    // returns number of audio tracks
#define ED_TRANSCAP_LTC_TRACK           ED_BASE+112L    // returns track number of LTC timecode track.
                                                        //  ED_ALL means no dedicated timecode track
#define ED_TRANSCAP_NEEDS_TBC           ED_BASE+113L    // device’s output not stable
#define ED_TRANSCAP_NEEDS_CUEING        ED_BASE+114L    // device must be cued prior to performing edit
#define ED_TRANSCAP_CAN_INSERT          ED_BASE+115L
#define ED_TRANSCAP_CAN_ASSEMBLE        ED_BASE+116L
#define ED_TRANSCAP_FIELD_STEP          ED_BASE+117L    // device responds to Frame Advance command by 
                                                        //  advancing one field
#define ED_TRANSCAP_CLOCK_INC_RATE      ED_BASE+118L    // VISCA command - keep for compatibility
#define ED_TRANSCAP_CAN_DETECT_LENGTH   ED_BASE+119L
#define ED_TRANSCAP_CAN_FREEZE          ED_BASE+120L
#define ED_TRANSCAP_HAS_TUNER           ED_BASE+121L
#define ED_TRANSCAP_HAS_TIMER           ED_BASE+122L
#define ED_TRANSCAP_HAS_CLOCK           ED_BASE+123L
#define ED_TRANSCAP_MULTIPLE_EDITS      ED_BASE+806L    // OATRUE means device/filter can support
                                                        //  multiple edit events
#define ED_TRANSCAP_IS_MASTER           ED_BASE+807L    // OATRUE means device is the master clock
                                                        //  for synchronizing (this sets timecode-to-
                                                        //  reference clock offset for editing)
#define ED_TRANSCAP_HAS_DT              ED_BASE+814L    // OATRUE means device has Dynamic Tracking

//  IAMExtTransport Media States
#define ED_MEDIA_SPIN_UP            ED_BASE+130L
#define ED_MEDIA_SPIN_DOWN          ED_BASE+131L
#define ED_MEDIA_UNLOAD             ED_BASE+132L

//  IAMExtTransport Modes
#define ED_MODE_PLAY                ED_BASE+200L
#define ED_MODE_STOP                ED_BASE+201L
#define ED_MODE_FREEZE              ED_BASE+202L    // really "pause"
#define ED_MODE_THAW                ED_BASE+203L
#define ED_MODE_FF                  ED_BASE+204L
#define ED_MODE_REW                 ED_BASE+205L
#define ED_MODE_RECORD              ED_BASE+206L
#define ED_MODE_RECORD_STROBE       ED_BASE+207L
#define ED_MODE_RECORD_FREEZE       ED_BASE+808L    // never "put", only "get"
#define ED_MODE_STEP                ED_BASE+208L    // same as "jog"
#define ED_MODE_STEP_FWD            ED_BASE+208L    // same as ED_MODE_STEP
#define ED_MODE_STEP_REV            ED_BASE+809L
#define ED_MODE_SHUTTLE             ED_BASE+209L
#define ED_MODE_EDIT_CUE            ED_BASE+210L
#define ED_MODE_VAR_SPEED           ED_BASE+211L
#define ED_MODE_PERFORM             ED_BASE+212L    // returned status only
#define ED_MODE_LINK_ON             ED_BASE+280L
#define ED_MODE_LINK_OFF            ED_BASE+281L
#define ED_MODE_NOTIFY_ENABLE       ED_BASE+810L
#define ED_MODE_NOTIFY_DISABLE      ED_BASE+811L
#define ED_MODE_SHOT_SEARCH         ED_BASE+812L

//  IAMTimecodeReader/Generator/Display defines
//
// Timecode Generator Mode params and values:
//
#define ED_TCG_TIMECODE_TYPE        ED_BASE+400L    // can be one of the following:
#define ED_TCG_SMPTE_LTC            ED_BASE+401L
#define ED_TCG_SMPTE_VITC           ED_BASE+402L
#define ED_TCG_MIDI_QF              ED_BASE+403L
#define ED_TCG_MIDI_FULL            ED_BASE+404L

#define ED_TCG_FRAMERATE            ED_BASE+405L    // can be one of the following:
#define ED_FORMAT_SMPTE_30          ED_BASE+406L
#define ED_FORMAT_SMPTE_30DROP      ED_BASE+407L
#define ED_FORMAT_SMPTE_25          ED_BASE+408L
#define ED_FORMAT_SMPTE_24          ED_BASE+409L

#define ED_TCG_SYNC_SOURCE          ED_BASE+410L    // can be one of the following:
#define ED_TCG_VIDEO                ED_BASE+411L
#define ED_TCG_READER               ED_BASE+412L
#define ED_TCG_FREE                 ED_BASE+413L

#define ED_TCG_REFERENCE_SOURCE     ED_BASE+414L    // can have one these values:
                                                    //  ED_TCG_FREE || ED_TCG_READER 
                                                    //  (for regen/jamsync)

// TimeCodeReader Mode params and values:
#define ED_TCR_SOURCE           ED_BASE+416L    // can be one of the following:
// ED_TCG (already defined)
#define ED_TCR_LTC              ED_BASE+417L
#define ED_TCR_VITC             ED_BASE+418L
#define ED_TCR_CT               ED_BASE+419L    // Control Track
#define ED_TCR_FTC              ED_BASE+420L    // File TimeCode - for file-based devices
                                                //  that wish they were transports
// ED_MODE_NOTIFY_ENABLE can be OATRUE or OAFALSE (defined in transport mode
//  section of this file).  
#define ED_TCR_LAST_VALUE       ED_BASE+421L    // for notification mode - 
                                                //  successive calls to GetTimecode
                                                //  return the last read value
// TimeCode Display Mode params and values:
//
#define ED_TCD_SOURCE           ED_BASE+422L    // can be one of the following:
#define ED_TCR                  ED_BASE+423L
#define ED_TCG                  ED_BASE+424L

#define ED_TCD_SIZE             ED_BASE+425L    // can be one of the following:
#define ED_SMALL                ED_BASE+426L
#define ED_MED                  ED_BASE+427L
#define ED_LARGE                ED_BASE+428L

#define ED_TCD_POSITION         ED_BASE+429L    // can be one of the following:
#define ED_TOP                  0x0001
#define ED_MIDDLE               0x0002
#define ED_BOTTOM               0x0004          // OR'd with
#define ED_LEFT                 0x0100
#define ED_CENTER               0x0200
#define ED_RIGHT                0x0400

#define ED_TCD_INTENSITY        ED_BASE+436L    // can be one of the following:
#define ED_HIGH                 ED_BASE+437L
#define ED_LOW                  ED_BASE+438L

#define ED_TCD_TRANSPARENCY     ED_BASE+439L    // 0-4, 0 is opaque

#define ED_TCD_INVERT           ED_BASE+440L    // OATRUE=black on white
                                                // OAFALSE=white on black
//  IAMExtTransport defines
//
// Transport status, params and values
//

// IAMExtTransport Status items and and values:
#define ED_MODE                 ED_BASE+500L    // see ED_MODE_xxx values above
#define ED_ERROR                ED_BASE+501L
#define ED_LOCAL                ED_BASE+502L
#define ED_RECORD_INHIBIT       ED_BASE+503L
#define ED_SERVO_LOCK           ED_BASE+504L
#define ED_MEDIA_PRESENT        ED_BASE+505L
#define ED_MEDIA_LENGTH         ED_BASE+506L
#define ED_MEDIA_SIZE           ED_BASE+507L
#define ED_MEDIA_TRACK_COUNT    ED_BASE+508L
#define ED_MEDIA_TRACK_LENGTH   ED_BASE+509L
#define ED_MEDIA_SIDE           ED_BASE+510L

#define ED_MEDIA_TYPE           ED_BASE+511L    // can be one of the following:
#define ED_MEDIA_VHS            ED_BASE+512L
#define ED_MEDIA_SVHS           ED_BASE+513L
#define ED_MEDIA_HI8            ED_BASE+514L
#define ED_MEDIA_UMATIC         ED_BASE+515L
#define ED_MEDIA_DVC            ED_BASE+516L
#define ED_MEDIA_1_INCH         ED_BASE+517L
#define ED_MEDIA_D1             ED_BASE+518L
#define ED_MEDIA_D2             ED_BASE+519L
#define ED_MEDIA_D3             ED_BASE+520L
#define ED_MEDIA_D5             ED_BASE+521L
#define ED_MEDIA_DBETA          ED_BASE+522L
#define ED_MEDIA_BETA           ED_BASE+523L
#define ED_MEDIA_8MM            ED_BASE+524L
#define ED_MEDIA_DDR            ED_BASE+525L
#define ED_MEDIA_SX             ED_BASE+813L
#define ED_MEDIA_OTHER          ED_BASE+526L
#define ED_MEDIA_CLV            ED_BASE+527L
#define ED_MEDIA_CAV            ED_BASE+528L
#define ED_MEDIA_POSITION       ED_BASE+529L

#define ED_LINK_MODE            ED_BASE+530L    // OATRUE if transport controls
                                                // are linked to graph's RUN, 
                                                // STOP, and PAUSE methods

// IAMExtTransport Basic Parms
#define ED_TRANSBASIC_TIME_FORMAT   ED_BASE+540L    // can be one of the following:
#define ED_FORMAT_MILLISECONDS      ED_BASE+541L
#define ED_FORMAT_FRAMES            ED_BASE+542L
#define ED_FORMAT_REFERENCE_TIME    ED_BASE+543L

#define ED_FORMAT_HMSF              ED_BASE+547L
#define ED_FORMAT_TMSF              ED_BASE+548L

#define ED_TRANSBASIC_TIME_REFERENCE    ED_BASE+549L    // can be one of the following:
#define ED_TIMEREF_TIMECODE         ED_BASE+550L
#define ED_TIMEREF_CONTROL_TRACK    ED_BASE+551L
#define ED_TIMEREF_INDEX            ED_BASE+552L

#define ED_TRANSBASIC_SUPERIMPOSE       ED_BASE+553L    // enable/disable onscreen display
#define ED_TRANSBASIC_END_STOP_ACTION   ED_BASE+554L    // can be one of: ED_MODE_STOP |
                                                        //  ED_MODE_REWIND | ED_MODE_FREEZE
#define ED_TRANSBASIC_RECORD_FORMAT ED_BASE+555L    // can be one of the following:
#define ED_RECORD_FORMAT_SP         ED_BASE+556L
#define ED_RECORD_FORMAT_LP         ED_BASE+557L
#define ED_RECORD_FORMAT_EP         ED_BASE+558L

#define ED_TRANSBASIC_STEP_COUNT    ED_BASE+559L
#define ED_TRANSBASIC_STEP_UNIT     ED_BASE+560L    // can be one of the following:
#define ED_STEP_FIELD               ED_BASE+561L
#define ED_STEP_FRAME               ED_BASE+562L
#define ED_STEP_3_2                 ED_BASE+563L

#define ED_TRANSBASIC_PREROLL       ED_BASE+564L
#define ED_TRANSBASIC_RECPREROLL    ED_BASE+565L
#define ED_TRANSBASIC_POSTROLL      ED_BASE+566L
#define ED_TRANSBASIC_EDIT_DELAY    ED_BASE+567L
#define ED_TRANSBASIC_PLAYTC_DELAY  ED_BASE+568L
#define ED_TRANSBASIC_RECTC_DELAY   ED_BASE+569L
#define ED_TRANSBASIC_EDIT_FIELD    ED_BASE+570L
#define ED_TRANSBASIC_FRAME_SERVO   ED_BASE+571L
#define ED_TRANSBASIC_CF_SERVO      ED_BASE+572L
#define ED_TRANSBASIC_SERVO_REF     ED_BASE+573L    // can be one of the following:
#define ED_REF_EXTERNAL             ED_BASE+574L
#define ED_REF_INPUT                ED_BASE+575L
#define ED_REF_INTERNAL             ED_BASE+576L
#define ED_REF_AUTO                 ED_BASE+577L

#define ED_TRANSBASIC_WARN_GL       ED_BASE+578L
#define ED_TRANSBASIC_SET_TRACKING  ED_BASE+579L    // can be one of the following:
#define ED_TRACKING_PLUS            ED_BASE+580L
#define ED_TRACKING_MINUS           ED_BASE+581L
#define ED_TRACKING_RESET           ED_BASE+582L

#define ED_TRANSBASIC_SET_FREEZE_TIMEOUT    ED_BASE+583L
#define ED_TRANSBASIC_VOLUME_NAME       ED_BASE+584L
#define ED_TRANSBASIC_BALLISTIC_1       ED_BASE+585L    // space for proprietary data
#define ED_TRANSBASIC_BALLISTIC_2       ED_BASE+586L
#define ED_TRANSBASIC_BALLISTIC_3       ED_BASE+587L
#define ED_TRANSBASIC_BALLISTIC_4       ED_BASE+588L
#define ED_TRANSBASIC_BALLISTIC_5       ED_BASE+589L
#define ED_TRANSBASIC_BALLISTIC_6       ED_BASE+590L
#define ED_TRANSBASIC_BALLISTIC_7       ED_BASE+591L
#define ED_TRANSBASIC_BALLISTIC_8       ED_BASE+592L
#define ED_TRANSBASIC_BALLISTIC_9       ED_BASE+593L
#define ED_TRANSBASIC_BALLISTIC_10      ED_BASE+594L
#define ED_TRANSBASIC_BALLISTIC_11      ED_BASE+595L
#define ED_TRANSBASIC_BALLISTIC_12      ED_BASE+596L
#define ED_TRANSBASIC_BALLISTIC_13      ED_BASE+597L
#define ED_TRANSBASIC_BALLISTIC_14      ED_BASE+598L
#define ED_TRANSBASIC_BALLISTIC_15      ED_BASE+599L
#define ED_TRANSBASIC_BALLISTIC_16      ED_BASE+600L
#define ED_TRANSBASIC_BALLISTIC_17      ED_BASE+601L
#define ED_TRANSBASIC_BALLISTIC_18      ED_BASE+602L
#define ED_TRANSBASIC_BALLISTIC_19      ED_BASE+603L
#define ED_TRANSBASIC_BALLISTIC_20      ED_BASE+604L

// consumer VCR items
#define ED_TRANSBASIC_SETCLOCK          ED_BASE+605L
#define ED_TRANSBASIC_SET_COUNTER_FORMAT    ED_BASE+606L    // uses time format flags
#define ED_TRANSBASIC_SET_COUNTER_VALUE ED_BASE+607L

#define ED_TRANSBASIC_SETTUNER_CH_UP    ED_BASE+608L
#define ED_TRANSBASIC_SETTUNER_CH_DN    ED_BASE+609L
#define ED_TRANSBASIC_SETTUNER_SK_UP    ED_BASE+610L
#define ED_TRANSBASIC_SETTUNER_SK_DN    ED_BASE+611L
#define ED_TRANSBASIC_SETTUNER_CH       ED_BASE+612L
#define ED_TRANSBASIC_SETTUNER_NUM      ED_BASE+613L

#define ED_TRANSBASIC_SETTIMER_EVENT    ED_BASE+614L
#define ED_TRANSBASIC_SETTIMER_STARTDAY ED_BASE+615L
#define ED_TRANSBASIC_SETTIMER_STARTTIME    ED_BASE+616L
#define ED_TRANSBASIC_SETTIMER_STOPDAY  ED_BASE+617L
#define ED_TRANSBASIC_SETTIMER_STOPTIME ED_BASE+618L

// IAMExtTransport video parameters
#define ED_TRANSVIDEO_SET_OUTPUT    ED_BASE+630L    // can be one of the following:
#define ED_E2E                      ED_BASE+631L
#define ED_PLAYBACK                 ED_BASE+632L
#define ED_OFF                      ED_BASE+633L

#define ED_TRANSVIDEO_SET_SOURCE    ED_BASE+634L

// IAMExtTransport audio parameters
#define ED_TRANSAUDIO_ENABLE_OUTPUT ED_BASE+640L    // can be the following:
#define ED_AUDIO_ALL                0x10000000      //  or any of the following OR'd together
#define ED_AUDIO_1                  0x0000001L
#define ED_AUDIO_2                  0x0000002L
#define ED_AUDIO_3                  0x0000004L
#define ED_AUDIO_4                  0x0000008L
#define ED_AUDIO_5                  0x0000010L
#define ED_AUDIO_6                  0x0000020L
#define ED_AUDIO_7                  0x0000040L
#define ED_AUDIO_8                  0x0000080L
#define ED_AUDIO_9                  0x0000100L
#define ED_AUDIO_10                 0x0000200L
#define ED_AUDIO_11                 0x0000400L
#define ED_AUDIO_12                 0x0000800L
#define ED_AUDIO_13                 0x0001000L
#define ED_AUDIO_14                 0x0002000L
#define ED_AUDIO_15                 0x0004000L
#define ED_AUDIO_16                 0x0008000L
#define ED_AUDIO_17                 0x0010000L
#define ED_AUDIO_18                 0x0020000L
#define ED_AUDIO_19                 0x0040000L
#define ED_AUDIO_20                 0x0080000L
#define ED_AUDIO_21                 0x0100000L
#define ED_AUDIO_22                 0x0200000L
#define ED_AUDIO_23                 0x0400000L
#define ED_AUDIO_24                 0x0800000L
#define ED_VIDEO                    0x2000000L  // for Edit props below

#define ED_TRANSAUDIO_ENABLE_RECORD     ED_BASE+642L
#define ED_TRANSAUDIO_ENABLE_SELSYNC    ED_BASE+643L
#define ED_TRANSAUDIO_SET_SOURCE        ED_BASE+644L
#define ED_TRANSAUDIO_SET_MONITOR       ED_BASE+645L


// Edit Property Set-related defs

// The following values reflect (and control) the state of an 
// edit property set
#define ED_INVALID      ED_BASE+652L
#define ED_EXECUTING    ED_BASE+653L
#define ED_REGISTER     ED_BASE+654L
#define ED_DELETE       ED_BASE+655L

// Edit property set parameters and values
#define ED_EDIT_HEVENT  ED_BASE+656L    // event handle to signal event 
                                        // completion
#define ED_EDIT_TEST    ED_BASE+657L    // returns OAFALSE if filter thinks
                                        //  edit can be done, OATRUE if not
#define ED_EDIT_IMMEDIATE   ED_BASE+658L    // OATRUE means start put the 
                                        // device into edit mode (editing
                                        // "on the fly") immediately upon
                                        //  execution of Mode(ED_MODE_EDIT_CUE)
#define ED_EDIT_MODE    ED_BASE+659L
// can be one of the following values:
#define ED_EDIT_MODE_ASSEMBLE       ED_BASE+660L
#define ED_EDIT_MODE_INSERT         ED_BASE+661L
#define ED_EDIT_MODE_CRASH_RECORD   ED_BASE+662L
#define ED_EDIT_MODE_BOOKMARK_TIME  ED_BASE+663L        // these two are for
#define ED_EDIT_MODE_BOOKMARK_CHAPTER   ED_BASE+664L    // laserdisks

#define ED_EDIT_MASTER  ED_BASE+666L    // OATRUE causes device 
                                        //  not to synchronize

#define ED_EDIT_TRACK       ED_BASE+667L
// can be one of the following possible OR'd values:
//  ED_VIDEO, ED_AUDIO_1 thru ED_AUDIO_24 (or ED_AUDIO_ALL)

#define ED_EDIT_SRC_INPOINT     ED_BASE+668L    // in current time format
#define ED_EDIT_SRC_OUTPOINT    ED_BASE+669L    // in current time format
#define ED_EDIT_REC_INPOINT     ED_BASE+670L    // in current time format
#define ED_EDIT_REC_OUTPOINT    ED_BASE+671L    // in current time format

#define ED_EDIT_REHEARSE_MODE   ED_BASE+672L
// can be one of the following possible values:
#define ED_EDIT_BVB     ED_BASE+673L    // means rehearse the edit with 
                                        //  "black-video-black"
#define ED_EDIT_VBV     ED_BASE+674L
#define ED_EDIT_VVV     ED_BASE+675L
#define ED_EDIT_PERFORM ED_BASE+676L    // means perform the edit with no 
                                        //  rehearsal.

// Set this property to OATRUE to kill the edit if in progress
#define ED_EDIT_ABORT   ED_BASE+677L
// how long to wait for edit to complete
#define ED_EDIT_TIMEOUT ED_BASE+678L        // in current time format

// This property causes the device to seek to a point specified by
// ED_EDIT_SEEK_MODE (see below).  NOTE: Only one event at a time can seek.
#define ED_EDIT_SEEK        ED_BASE+679L    // OATRUE means do it now.  
#define ED_EDIT_SEEK_MODE   ED_BASE+680L
//possible values:
#define ED_EDIT_SEEK_EDIT_IN    ED_BASE+681L    // seek to edit's inpoint
#define ED_EDIT_SEEK_EDIT_OUT   ED_BASE+682L    // seek to edit's outpoint
#define ED_EDIT_SEEK_PREROLL    ED_BASE+683L    // seek to edit's 
                                                //  inpoint-preroll
#define ED_EDIT_SEEK_PREROLL_CT ED_BASE+684L    // seek to preroll point 
                            // using control track (used for tapes with 
                            // discontinuoustimecode before edit point: seek
                            // to inpoint using timecode, then backup to 
                            // preroll point using control track)
#define ED_EDIT_SEEK_BOOKMARK   ED_BASE+685L    // seek to bookmark (just like 
                                                //  timecode search)
// This property is used for multiple-VCR systems where each machine must
//  cue to a different location relative to the graph's reference clock.  The
//  basic idea is that an edit event is setup with an ED_EDIT_OFFSET property
//  that tells the VCR what offset to maintain between it's timecode (converted
//  to reference clock units) and the reference clock.
#define ED_EDIT_OFFSET  ED_BASE+686L        // in current time format

#define ED_EDIT_PREREAD ED_BASE+815L        // OATRUE means device supports
                                            //  pre-read (recorder can also be
                                            //  player

//
// Some error codes:
// 
// device could be in local mode
#define ED_ERR_DEVICE_NOT_READY     ED_BASE+700L

#endif // __EDEVDEFS__

// eof edevdefs.h
