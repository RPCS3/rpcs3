#ifndef __efxcreative_h_
#define __efxcreative_h_

/**
 *  efx-creative.h - Environmental Audio Extensions
 *            for OpenAL Effects Extension.
 *
 */
#ifdef __cplusplus
extern "C" {
#endif


/**
 * Effect object definitions to be used with alEffect functions.
 *
 * Effect parameter value definitions, ranges, and defaults
 * appear farther down in this file.
 */

/* AL EAXReverb effect parameters. */
#define AL_EAXREVERB_DENSITY                               0x0001
#define AL_EAXREVERB_DIFFUSION                             0x0002
#define AL_EAXREVERB_GAIN                                  0x0003
#define AL_EAXREVERB_GAINHF                                0x0004
#define AL_EAXREVERB_GAINLF                                0x0005
#define AL_EAXREVERB_DECAY_TIME                            0x0006
#define AL_EAXREVERB_DECAY_HFRATIO                         0x0007
#define AL_EAXREVERB_DECAY_LFRATIO                         0x0008
#define AL_EAXREVERB_REFLECTIONS_GAIN                      0x0009
#define AL_EAXREVERB_REFLECTIONS_DELAY                     0x000A
#define AL_EAXREVERB_REFLECTIONS_PAN                       0x000B
#define AL_EAXREVERB_LATE_REVERB_GAIN                      0x000C
#define AL_EAXREVERB_LATE_REVERB_DELAY                     0x000D
#define AL_EAXREVERB_LATE_REVERB_PAN                       0x000E
#define AL_EAXREVERB_ECHO_TIME                             0x000F
#define AL_EAXREVERB_ECHO_DEPTH                            0x0010
#define AL_EAXREVERB_MODULATION_TIME                       0x0011
#define AL_EAXREVERB_MODULATION_DEPTH                      0x0012
#define AL_EAXREVERB_AIR_ABSORPTION_GAINHF                 0x0013 
#define AL_EAXREVERB_HFREFERENCE                           0x0014 
#define AL_EAXREVERB_LFREFERENCE                           0x0015 
#define AL_EAXREVERB_ROOM_ROLLOFF_FACTOR                   0x0016
#define AL_EAXREVERB_DECAY_HFLIMIT                         0x0017

/* Effect type definitions to be used with AL_EFFECT_TYPE. */
#define AL_EFFECT_EAXREVERB                                0x8000



 /**********************************************************
 * Effect parameter structures, value definitions, ranges and defaults.
 */

/**
 * AL reverb effect parameter ranges and defaults
 */
#define AL_EAXREVERB_MIN_DENSITY                           0.0f
#define AL_EAXREVERB_MAX_DENSITY                           1.0f
#define AL_EAXREVERB_DEFAULT_DENSITY                       1.0f

#define AL_EAXREVERB_MIN_DIFFUSION                         0.0f
#define AL_EAXREVERB_MAX_DIFFUSION                         1.0f
#define AL_EAXREVERB_DEFAULT_DIFFUSION                     1.0f

#define AL_EAXREVERB_MIN_GAIN                              0.0f
#define AL_EAXREVERB_MAX_GAIN                              1.0f
#define AL_EAXREVERB_DEFAULT_GAIN                          0.32f

#define AL_EAXREVERB_MIN_GAINHF                            0.0f
#define AL_EAXREVERB_MAX_GAINHF                            1.0f
#define AL_EAXREVERB_DEFAULT_GAINHF                        0.89f

#define AL_EAXREVERB_MIN_GAINLF                            0.0f
#define AL_EAXREVERB_MAX_GAINLF                            1.0f
#define AL_EAXREVERB_DEFAULT_GAINLF                        1.0f

#define AL_EAXREVERB_MIN_DECAY_TIME                        0.1f
#define AL_EAXREVERB_MAX_DECAY_TIME                        20.0f
#define AL_EAXREVERB_DEFAULT_DECAY_TIME                    1.49f

#define AL_EAXREVERB_MIN_DECAY_HFRATIO                     0.1f
#define AL_EAXREVERB_MAX_DECAY_HFRATIO                     2.0f
#define AL_EAXREVERB_DEFAULT_DECAY_HFRATIO                 0.83f

#define AL_EAXREVERB_MIN_DECAY_LFRATIO                     0.1f
#define AL_EAXREVERB_MAX_DECAY_LFRATIO                     2.0f
#define AL_EAXREVERB_DEFAULT_DECAY_LFRATIO                 1.0f

#define AL_EAXREVERB_MIN_REFLECTIONS_GAIN                  0.0f
#define AL_EAXREVERB_MAX_REFLECTIONS_GAIN                  3.16f
#define AL_EAXREVERB_DEFAULT_REFLECTIONS_GAIN              0.05f

#define AL_EAXREVERB_MIN_REFLECTIONS_DELAY                 0.0f
#define AL_EAXREVERB_MAX_REFLECTIONS_DELAY                 0.3f
#define AL_EAXREVERB_DEFAULT_REFLECTIONS_DELAY             0.007f

#define AL_EAXREVERB_DEFAULT_REFLECTIONS_PAN               {0.0f, 0.0f, 0.0f}

#define AL_EAXREVERB_MIN_LATE_REVERB_GAIN                  0.0f
#define AL_EAXREVERB_MAX_LATE_REVERB_GAIN                  10.0f
#define AL_EAXREVERB_DEFAULT_LATE_REVERB_GAIN              1.26f

#define AL_EAXREVERB_MIN_LATE_REVERB_DELAY                 0.0f
#define AL_EAXREVERB_MAX_LATE_REVERB_DELAY                 0.1f
#define AL_EAXREVERB_DEFAULT_LATE_REVERB_DELAY             0.011f

#define AL_EAXREVERB_DEFAULT_LATE_REVERB_PAN               {0.0f, 0.0f, 0.0f}

#define AL_EAXREVERB_MIN_ECHO_TIME                         0.075f
#define AL_EAXREVERB_MAX_ECHO_TIME                         0.25f
#define AL_EAXREVERB_DEFAULT_ECHO_TIME                     0.25f

#define AL_EAXREVERB_MIN_ECHO_DEPTH                        0.0f
#define AL_EAXREVERB_MAX_ECHO_DEPTH                        1.0f
#define AL_EAXREVERB_DEFAULT_ECHO_DEPTH                    0.0f

#define AL_EAXREVERB_MIN_MODULATION_TIME                   0.04f
#define AL_EAXREVERB_MAX_MODULATION_TIME                   4.0f
#define AL_EAXREVERB_DEFAULT_MODULATION_TIME               0.25f

#define AL_EAXREVERB_MIN_MODULATION_DEPTH                  0.0f
#define AL_EAXREVERB_MAX_MODULATION_DEPTH                  1.0f
#define AL_EAXREVERB_DEFAULT_MODULATION_DEPTH              0.0f

#define AL_EAXREVERB_MIN_AIR_ABSORPTION_GAINHF             0.892f
#define AL_EAXREVERB_MAX_AIR_ABSORPTION_GAINHF             1.0f
#define AL_EAXREVERB_DEFAULT_AIR_ABSORPTION_GAINHF         0.994f

#define AL_EAXREVERB_MIN_HFREFERENCE                       1000.0f
#define AL_EAXREVERB_MAX_HFREFERENCE                       20000.0f
#define AL_EAXREVERB_DEFAULT_HFREFERENCE                   5000.0f

#define AL_EAXREVERB_MIN_LFREFERENCE                       20.0f
#define AL_EAXREVERB_MAX_LFREFERENCE                       1000.0f
#define AL_EAXREVERB_DEFAULT_LFREFERENCE                   250.0f

#define AL_EAXREVERB_MIN_ROOM_ROLLOFF_FACTOR               0.0f
#define AL_EAXREVERB_MAX_ROOM_ROLLOFF_FACTOR               10.0f
#define AL_EAXREVERB_DEFAULT_ROOM_ROLLOFF_FACTOR           0.0f

#define AL_EAXREVERB_MIN_DECAY_HFLIMIT                     AL_FALSE
#define AL_EAXREVERB_MAX_DECAY_HFLIMIT                     AL_TRUE
#define AL_EAXREVERB_DEFAULT_DECAY_HFLIMIT                 AL_TRUE


#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* __efxcreative_h_ */
