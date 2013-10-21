/*
* Copyright (c) 2011-2013 by naehrwert
* This file is released under the GPLv2.
*/

#ifndef _CONFIG_H_
#define _CONFIG_H_

/*! scetool base version. */
#define SCETOOL_VERSION_BASE "0.2.9"

/*! Private build. */
//#define CONFIG_PRIVATE_BUILD
#define BUILD_FOR "naehrwert"
//#define BUILD_FOR "unicorns"

/*! scetool version. */
#ifdef CONFIG_PRIVATE_BUILD
	#ifdef BUILD_FOR
		#define SCETOOL_VERSION SCETOOL_VERSION_BASE " <PRIVATE BUILD:" BUILD_FOR ">"
	#else
		#error Specify a name in BUILD_FOR.
	#endif
#else
	#define SCETOOL_VERSION SCETOOL_VERSION_BASE " <public build>"
#endif

/*! Private build options. */
#ifdef CONFIG_PRIVATE_BUILD
	#define CONFIG_CUSTOM_INDIV_SEED
	#define CONFIG_DUMP_INDIV_SEED
#endif

#if 0
/*! scetool API. */
#define CONFIG_EXPORTS
#ifdef CONFIG_EXPORTS
#define SCETOOL_API __declspec(dllexport)
#else
#define SCETOOL_API __declspec(dllimport)
#endif
#endif

/*! NPDRM watermark text (16 bytes exactly). */
//"I like kittens !"
#define CONFIG_NPDRM_WATERMARK "watermarktrololo"

/*! Environment variables. */
#define CONFIG_ENV_PS3 "PS3"

/*! Path configurations. */
#define CONFIG_KEYS_FILE "keys"
#define CONFIG_KEYS_PATH "./data"
#define CONFIG_CURVES_FILE "ldr_curves"
#define CONFIG_CURVES_PATH "./data"
#define CONFIG_VSH_CURVES_FILE "vsh_curves"
#define CONFIG_VSH_CURVES_PATH "./data"
#define CONFIG_IDPS_FILE "idps"
#define CONFIG_IDPS_PATH "./data"
#define CONFIG_ACT_DAT_FILE "act.dat"
#define CONFIG_ACT_DAT_PATH "./data"
#define CONFIG_RIF_FILE_EXT ".rif"
#define CONFIG_RIF_PATH "./rifs"
#define CONFIG_RAP_FILE_EXT ".rap"
#define CONFIG_RAP_PATH "./raps"

/*! Key names. */
#define CONFIG_NP_TID_KNAME "NP_tid"
#define CONFIG_NP_CI_KNAME "NP_ci"
#define CONFIG_NP_KLIC_FREE_KNAME "NP_klic_free"
#define CONFIG_NP_KLIC_KEY_KNAME "NP_klic_key"
#define CONFIG_NP_IDPS_CONST_KNAME "NP_idps_const"
#define CONFIG_NP_RIF_KEY_KNAME "NP_rif_key"
#define CONFIG_NP_SIG_KNAME "NP_sig"

#endif
