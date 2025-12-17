#pragma once

#include "qt_utils.h"
#include <QString>
#include <QObject>

/**
 * Localized tooltips collection class
 * Due to special characters this file should stay in UTF-8 format
 */
class Tooltips : public QObject
{
	Q_OBJECT

public:

	Tooltips();

	const struct settings
	{
		// advanced

		const QString lle_list                     = tr("These libraries are LLE'd by default (lower list), selection will switch to HLE.\nLLE - \"Low Level Emulated\", function code inside the selected SPRX file will be used for exported firmware functions.\nHLE - \"High Level Emulated\", alternative emulator code will be used instead for exported firmware functions.\nIf chosen wrongly, games will not work! If unsure, leave both lists empty. HLEing all SPRX allows to boot without firmware installed. (experimental)");
		const QString hle_list                     = tr("These libraries are HLE'd by default (upper list), selection will switch to LLE.\nLLE - \"Low Level Emulated\", function code inside the selected SPRX file will be used for exported firmware functions.\nHLE - \"High Level Emulated\", alternative emulator code will be used instead for exported firmware functions.\nIf chosen wrongly, games will not work! If unsure, leave both lists empty. HLEing all SPRX allows to boot without firmware installed. (experimental)");
		const QString lib_default_hle              = tr("Select to LLE. (HLE by default)");
		const QString lib_default_lle              = tr("Select to HLE. (LLE by default)");

		const QString debug_console_mode           = tr("Increases the amount of usable system memory to match a DECR console and more.\nCauses some software to behave differently than on retail hardware.");
		const QString accurate_rsx_access          = tr("Forces RSX pauses on SPU MFC_GETLLAR and SPU MFC_PUTLLUC operations.");
		const QString accurate_spu_dma             = tr("Accurately processes SPU DMA operations.");
		const QString fixup_ppunj                  = tr("Legacy option. Fixup result vector values in Non-Java Mode in PPU LLVM.\nIf unsure, do not modify this setting.");
		const QString accurate_dfma                = tr("Use accurate double-precision FMA instructions in PPU and SPU backends.\nWhile disabling it might give a decent performance boost if your CPU doesn't support FMA, it may also introduce subtle bugs that otherwise do not occur.\nYou shouldn't disable it if your CPU supports FMA.");
		const QString fixup_ppuvnan                = tr("Fixup NaN results in vector instructions in PPU backends.\nIf unsure, do not modify this setting.");
		const QString silence_all_logs             = tr("Stop writing any logs after game startup. Don't use unless you believe it's necessary.");
		const QString read_color                   = tr("Initializes render target memory using vm memory.");
		const QString read_depth                   = tr("Initializes render target memory using vm memory.");
		const QString dump_depth                   = tr("Writes depth buffer values to vm memory.");
		const QString handle_tiled_memory          = tr("Obey RSX memory tiling configuration when writing GPU data to vm memory.\nThis can fix graphics corruption observed when Read Color or Read Depth options are enabled.");
		const QString disable_on_disk_shader_cache = tr("Disables the loading and saving of shaders from and to the shader cache in the data directory.");
		const QString allow_host_labels            = tr("Allows the host GPU to synchronize with CELL directly. This incurs a performance penalty, but exposes the true state of GPU objects to the guest CPU. Can help eliminate visual noise and glitching at the cost of performance. Use with caution.");
		const QString force_hw_MSAA                = tr("Forces MSAA to use the host GPU's resolve capabilities for all sampling operations.\nThis option incurs a performance penalty as well as the risk of visual artifacts but can yield crisper visuals when MSAA is enabled.");
		const QString disable_vertex_cache         = tr("Disables the vertex cache.\nMight resolve missing or flickering graphics output.\nMay degrade performance.");
		const QString disable_async_host_mm        = tr("Force host memory management calls to be inlined instead of handled asynchronously.\nThis can cause severe performance degradation and stuttering in some games.\nThis option is only needed by developers to debug problems with texture cache memory protection.");
		const QString disable_spin_optimization    = tr("Disable SPU GETLLAR spin optimization.\nThis can cause severe performance degradation and stuttering in many games.\nThis option is only needed for a select number of games.");
		const QString enable_spu_events_busy_loop  = tr("Enable SPU RdEventStat spin.\nThis increases CPU usage, this setting is beneficial for high-threaded CPUs (12+) with select number of games.");
		const QString zcull_operation_mode         = tr("Changes ZCULL report synchronization behaviour. Experiment to find the best option for your game. Approximate mode is recommended for most games.\n· Precise is the most accurate to PS3 behaviour. Required for accurate visuals in some titles such as Demon's Souls and The Darkness.\n· Approximate is a much faster way to generate occlusion data which may not always match what the PS3 would generate. Works well with most PS3 games.\n· Relaxed changes the synchronization method completely and can greatly improve performance in some games or completely break others.");
		const QString max_spurs_threads            = tr("Limits the maximum number of SPURS threads in each thread group.\nMay improve performance in some cases, especially on systems with limited number of hardware threads.\nLimiting the number of threads is likely to cause crashes; it's recommended to keep this at the default value.");
		const QString sleep_timers_accuracy        = tr("Changes the sleep period accuracy.\n'As Host' uses default accuracy of the underlying operating system, while 'All Timers' attempts to improve it.\n'Usleep Only' limits the adjustments to usleep syscall only.\nCan affect performance in unexpected ways.");
		const QString rsx_fifo_accuracy            = tr("\"Fast\" is the least accurate setting, RSX does not emulate atomic FIFO buffer.\n\"Atomic\" benefits stability greatly in many games with little performance penalty.\n\"Atomic & Ordered\" is the most accurate but it is the slowest and without much stability benefit in games.");
		const QString vblank_rate                  = tr("Adjusts the frequency of vertical blanking signals that the emulator sends.\nAffects timing of events which rely on these signals.");
		const QString vblank_ntsc_fixup            = tr("Multiplies the rate of VBLANK by 1000/1001 for values like 59.94Hz.\nKnown to fix the rhythm game Space Channel 5 Part 2");
		const QString clocks_scale                 = tr("Changes the scale of emulated system time.\nAffects software which uses system time to calculate things such as dynamic timesteps.");
		const QString wake_up_delay                = tr("Controls how much time it takes for RSX to start processing after waking up by the Cell processor.\nIncreasing wakeup delay improves stability, but very high values can lower RSX/GPU performance.\nIt is recommend to adjust this at 20µs to 40µs increments until the best value for optimal stability is reached.");
		const QString disabled_from_global         = tr("Do not change this setting globally.\nRight-click a game in the game list and choose \"Configure\" instead.");
		const QString vulkan_async_scheduler       = tr("Determines how to schedule GPU async compute jobs when using asynchronous streaming.\nUse 'Safe' mode for more spec compliant behavior at the cost of some CPU overhead. This setting works with all devices.\nUse 'Fast' to use a faster but hacky version. This option is internally disabled for NVIDIA GPUs due to causing GPU hangs.");
		const QString disable_msl_fast_math        = tr("Disables Fast Math for MSL shaders, which may violate the IEEE 754 standard.\nDisabling it may fix some artifacts, especially on Apple GPUs, at the cost of performance.");
		const QString anti_cheat_savestates        = tr("When this mode is on, emulation exits when saving and the savestate file is concealed after its load, preventing reuse by RPCS3.\nThis mode is like hibernation of emulation: if you don't want to be able to cheat using savestates when playing the game, consider using this mode.\nDo note that the savestate file is not gone completely, just ignored by RPCS3. You can manually relaunch it if needed.");
		const QString compatible_savestates        = tr("When this mode is on, SPU emulation prioritizes savestate compatibility, however, it may reduce performance slightly.\nWhen this mode is off, some games may not allow making a savestate and show an SPU pause error in the log.");
		const QString paused_savestates            = tr("When this mode is on, savestates are loaded and paused on the first frame.\nThis allows players to prepare for gameplay without being thrown into the action immediately.");
		const QString spu_profiler                 = tr("When enabled, SPU performance is measured at runtime.\nEnable only at a developer's request because when enabled it reduces performance a bit by itself.");
		const QString use_ReBAR                    = tr("When enabled, Vulkan will try to use PCI-e resizable bar address space for GPU uploads of timing-sensitive data.\nThis yields a massive performance win on NVIDIA cards when the base framerate is low.\nFor games with very high framerates, this option can result in worse performance for all GPU vendors.\n");

		// audio

		const QString audio_out                 = tr("Cubeb uses a cross-platform approach and supports audio buffering, so it is the recommended option.\nXAudio2 uses native Windows sounds system and is the next best alternative.");
		const QString audio_out_linux           = tr("Cubeb uses a cross-platform approach and supports audio buffering, so it is the recommended option.\nIf it's not available, FAudio could be used instead.");
		const QString audio_provider            = tr("Controls which PS3 audio API is used.\nGames use CellAudio, while VSH requires RSXAudio.");
		const QString audio_avport              = tr("Controls which avport is used to sample audio data from.");
		const QString audio_device              = tr("Controls which device is used by audio backend.");
		const QString audio_dump                = tr("Saves all audio as a raw wave file. If unsure, leave this unchecked.");
		const QString convert                   = tr("Uses 16-bit audio samples instead of default 32-bit floating point.\nUse with buggy audio drivers if you have no sound or completely broken sound.");
		const QString audio_format              = tr("Determines the sound format of the emulation.\nConfigure this setting if you want to switch between stereo and surround sound.\nChanging these values requires a restart of the game.\nThe manual setting will use your selected formats while the automatic setting will let the game choose from all available formats.");
		const QString audio_channel_layout      = tr("Determines the sound format of RPCS3.\nUse 'Auto' to let RPCS3 decide the best format based on the audio device and the emulated audio format.");
		const QString master_volume             = tr("Controls the overall volume of the emulation.\nValues above 100% might reduce the audio quality.");
		const QString enable_buffering          = tr("Enables audio buffering, which reduces crackle/stutter but increases audio latency.");
		const QString audio_buffer_duration     = tr("Target buffer duration in milliseconds.\nHigher values make the buffering algorithm's job easier, but may introduce noticeable audio latency.");
		const QString enable_time_stretching    = tr("Enables time stretching - requires buffering to be enabled.\nReduces crackle/stutter further, but may cause a very noticeable reduction in audio quality on slower CPUs.");
		const QString time_stretching_threshold = tr("Buffer fill level (in percentage) below which time stretching will start.");
		const QString microphone                = tr("Standard should be used for most games.\nSingStar emulates a SingStar device and should be used with SingStar games.\nReal SingStar should only be used with a REAL SingStar device with SingStar games.\nRocksmith should be used with a Rocksmith dongle.");

		// cpu

		const QString ppu__static               = tr("Interpreter (slow). Try this if PPU Recompiler (LLVM) doesn't work.");
		const QString ppu_dynamic               = tr("Alternative interpreter (slow). May be faster than static interpreter. Try this if PPU Recompiler (LLVM) doesn't work.");
		const QString ppu_llvm                  = tr("Recompiles and caches the game's PPU code using the LLVM Recompiler once before running it for the first time.\nThis is by far the fastest option and should always be used.\nShould you face compatibility issues, fall back to one of the Interpreters and retry.\nIf unsure, use this option.");
		const QString llvm_precompilation       = tr("Searches the game's directory and precompiles extra PPU and SPU modules during boot.\nIf disabled, these modules will only be compiled when needed. Depending on the game, this might interrupt the gameplay unexpectedly and possibly frequently.\nOnly disable this if you want to get ingame more quickly.");
		const QString spu__static               = tr("Interpreter (slow). Try this if SPU Recompiler (LLVM) doesn't work.");
		const QString spu_dynamic               = tr("Alternative interpreter (slow). May be faster than static interpreter. Try this if SPU Recompiler (LLVM) doesn't work.");
		const QString spu_asmjit                = tr("Recompiles the game's SPU code using the ASMJIT Recompiler.\nThis is the fast option with very good compatibility.\nIf unsure, use this option.");
		const QString spu_llvm                  = tr("Recompiles and caches the game's SPU code using the LLVM Recompiler before running which adds extra start-up time.\nThis is the fastest option with very good compatibility.\nIf you experience issues, use the ASMJIT Recompiler.");
		const QString xfloat                    = tr("Control accuracy to SPU float vectors processing.\nFixes bugs in various games at the cost of performance.\nThis setting is only applied when SPU Decoder is set to Dynamic or LLVM.");
		const QString enable_thread_scheduler   = tr("Control how RPCS3 utilizes the threads of your system.\nEach option heavily depends on the game and on your CPU. It's recommended to try each option to find out which performs the best.\nChanging the thread scheduler is not supported on CPUs with less than 12 threads.");
		const QString spu_loop_detection        = tr("Try to detect loop conditions in SPU kernels and use them as scheduling hints.\nImproves performance and reduces CPU usage.\nMay cause severe audio stuttering in rare cases.");
		const QString spu_block_size            = tr("This option controls the SPU analyser, particularly the size of compiled units. The Mega and Giga modes may improve performance by tying smaller units together, decreasing the number of compiled units but increasing their size.\nUse the Safe mode for maximum compatibility.");
		const QString preferred_spu_threads     = tr("Some SPU stages are sensitive to race conditions and allowing a limited number at a time helps alleviate performance stalls.\nSetting this to a smaller value might improve performance and reduce stuttering in some games.\nLeave this on auto if performance is negatively affected when setting a small value.");
		const QString max_cpu_preempt           = tr("Reduces CPU usage and power consumption, improving battery life on mobile devices. (0 means disabled)\nHigher values cause a more pronounced effect, but may cause audio or performance issues. A value of 50 or less is recommended.\nThis option forces an FPS limit because it's active when framerate is stable.\nThe lighter the game is on the hardware, the more power is saved by it. (until the preemption count barrier is reached)");

		// debug

		const QString start_on_boot                = tr("Leave this enabled unless you are a developer.");
		const QString ppu_debug                    = tr("Creates PPU logs.\nOnly useful to developers.\nNever use this.");
		const QString spu_debug                    = tr("Creates SPU logs.\nOnly useful to developers.\nNever use this.");
		const QString mfc_debug                    = tr("Creates MFC logs.\nOnly useful to developers.\nNever use this.");
		const QString set_daz_and_ftz              = tr("Sets special MXCSR flags to debug errors in SSE operations.\nOnly used in PPU thread when it's not precise.\nOnly useful to developers.\nNever use this.");
		const QString accurate_ppusat              = tr("Accurately set Saturation Bit values in PPU backends.\nIf unsure, do not modify this setting.");
		const QString accurate_ppunj               = tr("Respect Non-Java Mode Bit values for vector ops in PPU backends.\nIf unsure, do not modify this setting.");
		const QString accurate_ppuvnan             = tr("Accurately set NaN results in vector instructions in PPU backends.\nIf unsure, do not modify this setting.");
		const QString accurate_ppufpcc             = tr("Accurately set FPCC Bits in PPU backends.\nIf unsure, do not modify this setting.");
		const QString accurate_cache_line_stores   = tr("Accurately processes PPU DCBZ instruction.\nIn addition, when combined with Accurate SPU DMA, SPU PUT cache line accesses will be processed atomically.");
		const QString mfc_delay_command            = tr("Forces delaying any odd MFC command, waits for at least 2 pending commands to execute them in a random order.\nMust be used with either SPU interpreters currently.\nSeverely degrades performance! If unsure, don't use this option.");
		const QString hook_static_functions        = tr("Allows to hook some functions like 'memcpy' replacing them with high-level implementations. May do nothing or break things. Experimental.");
		const QString renderdoc_compatibility      = tr("Enables use of classic OpenGL buffers which allows capturing tools to work with RPCS3 e.g RenderDoc.\nAlso allows Vulkan to use debug markers for nicer Renderdoc captures.\nIf unsure, don't use this option.");
		const QString force_high_pz                = tr("Only useful when debugging differences in GPU hardware.\nNot necessary for average users.\nIf unsure, don't use this option.");
		const QString debug_output                 = tr("Enables the selected API's inbuilt debugging functionality.\nWill cause severe performance degradation especially with Vulkan.\nOnly useful to developers.\nIf unsure, don't use this option.");
		const QString debug_overlay                = tr("Provides a graphical overlay of various debugging information.\nIf unsure, don't use this option.");
		const QString debug_overlay_io             = tr("Provides a graphical overlay with pad input values for player 1.\nThis is only shown if the debug overlay is disabled.\nIf unsure, don't use this option.");
		const QString debug_overlay_mouse          = tr("Provides a graphical overlay with mouse input values.\nThis is only shown if the other debug overlays are disabled.\nIf unsure, don't use this option.");
		const QString log_shader_programs          = tr("Dump game shaders to file. Only useful to developers.\nIf unsure, don't use this option.");
		const QString disable_occlusion_queries    = tr("Disables running occlusion queries. Minor to moderate performance boost.\nMight introduce issues with broken occlusion e.g missing geometry and extreme pop-in.");
		const QString disable_video_output         = tr("Disables all video output and PS3 graphical rendering.\nIts only use case is to evaluate performance on CELL for development.");
		const QString force_cpu_blit_emulation     = tr("Forces emulation of all blit and image manipulation operations on the CPU.\nRequires 'Write Color Buffers' option to also be enabled in most cases to avoid missing graphics.\nSignificantly degrades performance but is more accurate in some cases.\nThis setting overrides the 'GPU texture scaling' option.");
		const QString disable_vulkan_mem_allocator = tr("Disables the custom Vulkan memory allocator and reverts to direct calls to VkAllocateMemory/VkFreeMemory.");
		const QString disable_fifo_reordering      = tr("Disables RSX FIFO optimizations completely. Draws are processed as they are received by the DMA puller.");
		const QString gpu_texture_scaling          = tr("Force all texture transfer, scaling and conversion operations on the GPU.\nMay cause texture corruption in some cases.");
		const QString strict_texture_flushing      = tr("Forces texture flushing even in situations where it is not necessary/correct. Known to cause visual artifacts, but useful for debugging certain texture cache issues.");
		const QString stereo_render_mode           = tr("Sets the 3D stereo rendering mode (only available in custom configurations with a default resolution of 720p).\nAnaglyph uses different colors for each eye, which can then be filtered with certain glasses.\nSide-by-Side is more commonly supported by VR viewer apps.\nOver-Under is closer to the native stereo output, but less commonly supported.");
		const QString accurate_ppu_128_loop        = tr("When enabled, PPU atomic operations will operate on entire cache line data, as opposed to a single 64bit block of memory when disabled.\nNumerical values control whether or not to enable the accurate version based on the atomic operation's length.");
		const QString enable_performance_report    = tr("Measure certain events and print a chart after the emulator is stopped. Don't enable if not asked to.");
		const QString num_ppu_threads              = tr("Affects maximum amount of PPU threads running concurrently, the value of 1 has very low compatibility with games.\n2 is the default, if unsure do not modify this setting.");

		// emulator

		const QString enable_gamemode              = tr("Activate Feral Interactive's GameMode.\nThis is a series of CPU and GPU optimizations and can potentially benefit game performance on some systems.");
		const QString no_gamemode                  = tr("This requires Feral Interactive's GameMode to be installed.\nGameMode is a series of CPU and GPU optimizations and can potentially benefit game performance on some systems.\nTo install GameMode for your specific Linux distribution, go to the GitHub page:https://github.com/FeralInteractive/gamemode.");
		const QString exit_on_stop                 = tr("Automatically close RPCS3 when closing a game, or when a game closes itself.");
		const QString pause_on_focus_loss          = tr("Automatically pause emulation when RPCS3 loses its focus or the application is inactive in order to save power and reduce CPU usage.\nDo note that emulation pausing in general is not perfect and may not be compatible with all games.\nAlthough it currently also pauses gameplay, it is not recommended to rely on it as this behavior may be changed in the future and it is not the purpose of this setting.");
		const QString start_game_fullscreen        = tr("Automatically puts the game window in fullscreen.\nDouble click on the game window or press Alt+Enter to toggle fullscreen and windowed mode.");
		const QString prevent_display_sleep        = tr("Prevent the display from sleeping while a game is running.\nThis requires the org.freedesktop.ScreenSaver D-Bus service on Linux.\nThis option will be disabled if the current platform does not support display sleep control.");
		const QString game_window_title_format     = tr("Configure the game window title.\nChanging this and/or adding the framerate may cause buggy or outdated recording software to not notice RPCS3.");
		const QString resize_on_boot               = tr("Automatically resizes the game window on boot.\nThis does not change the internal game resolution.");
		const QString show_trophy_popups           = tr("Show trophy pop-ups when a trophy is unlocked.");
		const QString show_rpcn_popups             = tr("Show RPCN friend list pop-ups.");
		const QString disable_mouse                = tr("Disables the activation of fullscreen mode per double-click while the game screen is active.\nCheck this if you want to play with mouse and keyboard (for example with UCR).");
		const QString disable_kb_hotkeys           = tr("Disables keyboard hotkeys such as Ctrl+S, Ctrl+E, Ctrl+R, Ctrl+P while the game screen is active.\nThis does not include Ctrl+L (hide and lock mouse) and Alt+Enter (toggle fullscreen).\nCheck this if you want to play with mouse and keyboard.");
		const QString max_llvm_threads             = tr("Limits the maximum number of threads used for the initial PPU and SPU module compilation.\nLower this in order to increase performance of other open applications.\nThe default uses all available threads.");
		const QString show_mouse_in_fullscreen     = tr("Shows the mouse cursor when the fullscreen mode is active.\nCurrently this may not work every time.");
		const QString lock_mouse_in_fullscreen     = tr("Locks the mouse cursor to the center when the fullscreen mode is active.");
		const QString hide_mouse_on_idle           = tr("Hides the mouse cursor if no mouse movement is detected for the configured time.");
		const QString show_shader_compilation_hint = tr("Shows 'Compiling shaders' hint using the native overlay.");
		const QString show_ppu_compilation_hint    = tr("Shows 'Compiling PPU modules' hint using the native overlay.");
		const QString show_autosave_autoload_hint  = tr("Shows autosave/autoload hint using the native overlay.");
		const QString show_pressure_intensity_toggle_hint = tr("Shows pressure intensity toggle hint using the native overlay.");
		const QString show_analog_limiter_toggle_hint = tr("Shows analog limiter toggle hint using the native overlay.");
		const QString show_mouse_and_keyboard_toggle_hint = tr("Shows mouse and keyboard toggle hint using the native overlay.");
		const QString show_capture_hints           = tr("Shows screenshot and recording hints using the native overlay.");
		const QString use_native_interface         = tr("Enables use of native HUD within the game window that can interact with game controllers.\nWhen disabled, regular Qt dialogs are used instead.\nCurrently, the on-screen keyboard only supports the English key layout.");
		const QString record_with_overlays         = tr("Enables recording with overlays.\nThis also affects screenshots.");
		const QString pause_during_home_menu       = tr("When enabled, opening the home menu will also pause emulation.\nWhile most games pause themselves while the home menu is shown, some do not.\nIn that case it can be helpful to pause the emulation whenever the home menu is open.");

		const QString perf_overlay_enabled                 = tr("Enables or disables the performance overlay.");
		const QString perf_overlay_framerate_graph_enabled = tr("Enables or disables the framerate graph.");
		const QString perf_overlay_frametime_graph_enabled = tr("Enables or disables the frametime graph.");
		const QString perf_overlay_framerate_datapoints    = tr("Sets the amount of datapoints used in the framerate graph.");
		const QString perf_overlay_frametime_datapoints    = tr("Sets the amount of datapoints used in the frametime graph.");
		const QString perf_overlay_position                = tr("Sets the on-screen position (quadrant) of the performance overlay.");
		const QString perf_overlay_detail_level            = tr("Controls the amount of information displayed on the performance overlay.");
		const QString perf_overlay_update_interval         = tr("Sets the time interval in which the performance overlay is being updated (measured in milliseconds).\nSetting this to 16 milliseconds will refresh the performance overlay at roughly 60Hz.\nThe performance overlay refresh rate does not affect the frame graph statistics and can only be as fast as the current game allows.");
		const QString perf_overlay_font_size               = tr("Sets the font size of the performance overlay (measured in pixels).");
		const QString perf_overlay_opacity                 = tr("Sets the opacity of the performance overlay (measured in %).");
		const QString perf_overlay_margin_x                = tr("Sets the horizontal distance to the screen border relative to the screen quadrant (measured in pixels).");
		const QString perf_overlay_margin_y                = tr("Sets the vertical distance to the screen border relative to the screen quadrant (measured in pixels).");
		const QString perf_overlay_center_x                = tr("Centers the performance overlay horizontally and overrides the horizontal margin.");
		const QString perf_overlay_center_y                = tr("Centers the performance overlay vertically and overrides the vertical margin.");

		const QString shader_load_bg_enabled   = tr("Shows a background image during the native shader loading dialog/loading screen.\nBy default the used image will be <gamedir>/PS3_GAME/PIC1.PNG.");
		const QString shader_load_bg_darkening = tr("Changes the background image darkening effect strength of the native shader loading dialog.\nThis may be used to improve readability and/or aesthetics.");
		const QString shader_load_bg_blur      = tr("Changes the background image blur effect strength of the native shader loading dialog.\nThis may be used to improve readability and/or aesthetics.");

		// gpu

		const QString renderer                   = tr("Vulkan is the fastest renderer. OpenGL is the most accurate renderer.\nIf unsure, use Vulkan. Should you have any compatibility issues, fall back to OpenGL.");
		const QString resolution                 = tr("This setting will be ignored if the Resolution Scale is set to anything other than 100%!\nLeave this on 1280x720. Every PS3 game is compatible with this resolution.\nOnly use 1920x1080 if the game supports it.\nRarely due to emulation bugs some games will only render at low resolutions like 480p.");
		const QString graphics_adapter           = tr("On multi GPU systems select which GPU to use in RPCS3 when using Vulkan.\nThis is not needed when using OpenGL.");
		const QString aspect_ratio               = tr("Leave this on 16:9 unless you have a 4:3 monitor.");
		const QString frame_limit                = tr("Off is the fastest option.\nUsing the frame limiter will add extra overhead and slow down the game. However, some games will crash if the framerate is too high.\nPS3 native should only be used if Auto is not working correctly as it can introduce frame-pacing issues.\nInfinite adds a positive feedback loop which adds another vblank signal per frame allowing more games to be fps limitless.\nExperienced users with need of other frame limits should use the setting \"Second Frame Limit\" in the configuration file.");
		const QString anti_aliasing              = tr("Emulate PS3 multisampling layout.\nCan fix some otherwise difficult to solve graphics glitches.\nLow to moderate performance hit depending on your GPU hardware.");
		const QString anisotropic_filter         = tr("Higher values increase sharpness of textures on sloped surfaces at the cost of GPU resources.\nModern GPUs can handle this setting just fine, even at 16x.\nKeep this on Automatic if you want to use the original setting used by a real PS3.");
		const QString resolution_scale           = tr("Scales the game's resolution by the given percentage.\nThe base resolution is always 1280x720.\nSet this value to 100% if you want to use the normal Resolution options.\nValues below 100% will usually not improve performance.");
		const QString minimum_scalable_dimension = tr("Only framebuffers greater than this size will be upscaled.\nIncreasing this value might fix problems with missing graphics when upscaling, especially when Write Color Buffers is enabled.\nIf unsure, don't change this option.");
		const QString dump_color                 = tr("Enable this option if you get missing graphics or broken lighting ingame.\nMight degrade performance and introduce stuttering in some cases.\nRequired for Demon's Souls.");
		const QString vsync                      = tr("By having this off you might obtain a higher framerate at the cost of tearing artifacts in the game.");
		const QString strict_rendering_mode      = tr("Enforces strict compliance to the API specification.\nMight result in degraded performance in some games.\nCan resolve rare cases of missing graphics and flickering.\nIf unsure, don't use this option.");
		const QString stretch_to_display_area    = tr("Overrides the aspect ratio and stretches the image to the full display area.");
		const QString multithreaded_rsx          = tr("Offloads some RSX operations to a secondary thread.\nImproves performance for high-core processors.\nMay cause slowdown in weaker CPUs due to the extra worker thread load.");

		const QString legacy_shader_recompiler        = tr("Disables asynchronous shader compilation.\nFixes missing graphics while shaders are compiling but introduces severe stuttering or lag.\nUse this if you do not want to deal with graphics pop-in, or for testing before filing any bug reports.");
		const QString async_shader_recompiler         = tr("This is the recommended option.\nIf a shader is not found in the cache, nothing will be rendered for this shader until it has compiled.\nYou may experience graphics pop-in.");
		const QString async_with_shader_interpreter   = tr("Hybrid rendering mode.\nIf a shader is not found in the cache, the interpreter will be used to render approximated graphics for this shader until it has compiled.");
		const QString shader_interpreter_only         = tr("All rendering is handled by the interpreter with no attempt to compile native shaders.\nThis mode is very slow and experimental.");
		const QString shader_compiler_threads         = tr("Number of threads to use for the shader compiler backend.\nOnly has an impact when shader mode is set to one of the asynchronous modes.");
		const QString shader_precision                = tr("Controls the precision level of generated shaders. Low precision generates much faster code depending on the hardware, but can sometimes generate minor visual glitches or flicker.");

		const QString async_texture_streaming   = tr("Stream textures to GPU in parallel with 3D rendering using asynchronous compute.\nCan improve performance on more powerful GPUs that have spare headroom.\nOnly works with Vulkan renderer and greatly benefits from having MTRSX enabled if you have a capable CPU.");
		const QString exclusive_fullscreen_mode = tr("Controls which fullscreen mode RPCS3 requests from drivers when using Vulkan renderer.\nAutomatic will let the driver choose an appropriate mode, while the other options will hint the drivers on whether they should use exclusive or borderless fullscreen.\nUsing Prefer borderless fullscreen option can help if you have issues with streaming RPCS3 gameplay or if your system incorrectly enables HDR mode when using fullscreen.");

		const QString output_scaling_mode = tr("Final image filtering. Nearest applies no filtering, Bilinear smooths the image, and FidelityFX Super Resolution enhances upscaled images.\nIf the game is rendering at an internal resolution lower than your window resolution, FidelityFX will handle the upscale.\nFidelityFX can cause visual artifacts.\nFidelityFX does not work with stereo 3D output for now.");
		const QString fsr_rcas_strength   = tr("Control the sharpening strength applied by FidelityFX Super Resolution. Higher values will give sharper output but may introduce artifacts.");

		const QString texture_lod_bias = tr("Changes Texture sampling accuracy. (Small changes have a big effect.)\nAvoid using values outside the range of -12 to +12 if you're unsure.\n-3 to +3 is plenty for most usecases");

		// gui

		const QString log_limit          = tr("Sets the maximum amount of blocks that the log can display.\nThis usually equals the number of lines.\nSet 0 in order to remove the limit.");
		const QString tty_limit          = tr("Sets the maximum amount of blocks that the TTY can display.\nThis usually equals the number of lines.\nSet 0 in order to remove the limit.");
		const QString stylesheets        = tr("Changes the overall look of RPCS3.\nChoose a stylesheet and click Apply to change between styles.");
		const QString show_welcome       = tr("Shows the initial welcome screen upon starting RPCS3.");
		const QString show_exit_game     = tr("Shows a confirmation dialog when the game window is being closed.");
		const QString show_boot_game     = tr("Shows a confirmation dialog when a game was booted while another game is running.");
		const QString show_pkg_install   = tr("Shows a dialog when packages were installed successfully.");
		const QString show_pup_install   = tr("Shows a dialog when firmware was installed successfully.");
		const QString show_obsolete_cfg  = tr("Shows a dialog when obsolete settings were found.");
		const QString show_same_buttons  = tr("Shows a dialog in the game pad configuration when the same button was assigned twice.");
		const QString show_restart_hint  = tr("Shows a dialog when RPCS3 is ready to restart after an update.");
		const QString check_update_start = tr("Checks if an update is available on startup and asks if you want to update.\nIf \"Automatic\" is selected, the update will run automatically without user confirmation.\nIf \"Background\" is selected, the check is done silently in the background and a new download option is shown in the top right corner of the menu if a new version was found.");
		const QString use_rich_presence  = tr("Enables use of Discord Rich Presence to show what game you are playing on Discord.\nRequires a restart of RPCS3 to completely close the connection.");
		const QString discord_state      = tr("Tell your friends what you are doing.");
		const QString custom_colors      = tr("Prioritize custom user interface colors over properties set in stylesheet.");
		const QString uuid               = tr("This is the ID used for hardware statistics.\nIt should only be reset if you change your hardware configuration or if you copied RPCS3 to another PC.");
		const QString pad_navigation     = tr("Use the game pad that is configured for player 1 to navigate in the GUI.");
		const QString global_navigation  = tr("Keep control over pad navigation if RPCS3 is not the active window.");

		// input

		const QString pad_mode          = tr("Single-threaded: All pad handlers run on the same thread sequentially.\nMulti-threaded: Each pad handler has its own thread.\nOnly use multi-threaded if you can spare the extra threads.");
		const QString pad_connection    = tr("Shows all configured pads as always connected ingame even if they are physically disconnected.");
		const QString keyboard_handler  = tr("Some games support native keyboard input.\nBasic will work in these cases.");
		const QString mouse_handler     = tr("Some games support native mouse input.\nBasic or Raw will work in these cases.");
		const QString music_handler     = tr("Currently only used for cellMusic emulation.\nSelect Qt to use the default output device of your operating system.\nThis may not be able to play all audio formats.");
		const QString camera            = tr("Select Qt Camera to use the default camera device of your operating system.");
		const QString camera_type       = tr("Depending on the game, you may need to select a specific camera type.");
		const QString camera_flip       = tr("Flips the camera image either horizontally, vertically, or on both axes.");
		const QString camera_id         = tr("Select the camera that you want to use during gameplay.");
		const QString move              = tr("PlayStation Move support.\nFake: Experimental! This maps Move controls to DS3 controller mappings.\nMouse: Emulate PSMove with Mouse handler.\nRaw Mouse: Emulate PSMove with Raw Mouse handler.");
		const QString buzz              = tr("Buzz! support.\nSelect 1 or 2 controllers if the game requires Buzz! controllers and you don't have real controllers.\nSelect Null if the game has support for DualShock or if you have real Buzz! controllers.");
		const QString turntable         = tr("DJ Hero Turntable controller support.\nSelect 1 or 2 controllers if the game requires DJ Hero Turntable controllers and you don't have real turntable controllers.\nSelect Null if the game has support for DualShock or if you have real turntable controllers.\nA real turntable controller can be used at the same time as an emulated turntable controller.");
		const QString ghltar            = tr("Guitar Hero Live (GHL) Guitar controller support.\nSelect 1 or 2 controllers if the game requires GHL Guitar controllers and you don't have real guitar controllers.\nSelect Null if the game has support for DualShock or if you have real guitar controllers.\nA real guitar controller can be used at the same time as an emulated guitar controller.");
		const QString background_input  = tr("Allows pad and keyboard input while the game window is unfocused.");
		const QString show_move_cursor  = tr("Shows the raw position of the PS Move input.\nThis can be very helpful during calibration screens.");
		const QString midi_devices      = tr("Select up to 3 emulated MIDI devices and their types.");
		const QString sdl_mappings      = tr("Loads the SDL GameController database for improved gamepad compatibility. Only used in the SDL pad handler.");

		const QString lock_overlay_input_to_player_one  = tr("Locks the native overlay input to the first player.");

		// network

		const QString net_status    = tr("If set to Connected, RPCS3 will allow programs to use your internet connection.");
		const QString psn_status    = tr("If set to RPCN, RPCS3 will use the RPCN server as PSN connection if the game is supported.\nIf set to Simulated, RPCS3 will try to fake the PSN connection, but any actual attempt at using the PSN functionality may result in errors or crashes.\nSimulated is only available in custom configurations.");
		const QString dns           = tr("DNS used to resolve hostnames by applications.");
		const QString dns_swap      = tr("DNS Swap List.\nOnly available in custom configurations.");
		const QString bind          = tr("Interface IP Address to bind to.\nOnly available in custom configurations.");
		const QString enable_upnp   = tr("Enable UPNP.\nThis will automatically forward ports bound on 0.0.0.0 if your router has UPNP enabled.");
		const QString psn_country   = tr("Changes the RPCN country.");

		// system

		const QString license_area            = tr("The console region defines the license area of the PS3.\nDepending on the license area, some games may not work.");
		const QString system_language         = tr("Some games may fail to boot if the system language is not available in the game itself.\nOther games will switch language automatically to what is selected here.\nIt is recommended leaving this on a language supported by the game.");
		const QString date_format             = tr("Select the PS3's date format.");
		const QString time_format             = tr("Select the PS3's time format.");
		const QString keyboard_type           = tr("Sets the used keyboard layout.\nCurrently only US, Japanese and German layouts are fully supported at this moment.");
		const QString enter_button_assignment = tr("The button used for enter/accept/confirm in system dialogs.\nChange this to use the Circle button instead, which is the default configuration on Japanese systems and in many Japanese games.\nIn these cases having the cross button assigned can often lead to confusion.");
		const QString enable_host_root        = tr("Required for some Homebrew.\nIf unsure, do not use this option.");
		const QString empty_hdd0_tmp          = tr("Required for some Homebrew or Game Mods.\nIf unsure, do not use this option");
		const QString limit_cache_size        = tr("Automatically removes older files from disk cache on boot if it grows larger than the specified value.\nGames can use the cache folder to temporarily store data outside of system memory. It is not used for long-term storage.\n\nThis setting is only available in the global configuration.");
		const QString console_time_offset     = tr("Sets the time to be used within the console. This will be applied as an offset that tracks wall clock time.\nCan be reset to current wall clock time by clicking \"Set to Now\".");
	} settings;

	const struct gamepad_settings
	{
		const QString null        = tr("This controller is disabled and will appear as disconnected to software. Choose another handler to enable it.");
		const QString ldd_pad     = tr("This port is currently assigned to a custom controller by the application and can't be changed.");
		const QString keyboard    = tr("While it is possible to use a keyboard as a pad in RPCS3, the use of an actual controller is strongly recommended.<br>To bind mouse movement to a button or joystick, click on the desired button to activate it, then click and hold while dragging the mouse to a direction.");
		const QString ds3_windows = tr("In order to use the DualShock 3 handler, you need to install the official DualShock 3 driver first.<br>See the <a %0 href=\"https://wiki.rpcs3.net/index.php?title=Help:Controller_Configuration\">RPCS3 Wiki</a> for instructions.").arg(gui::utils::get_link_style());
		const QString ds3_linux   = tr("In order to use the DualShock 3 handler, you might need to add udev rules to let RPCS3 access the controller.<br>See the <a %0 href=\"https://wiki.rpcs3.net/index.php?title=Help:Controller_Configuration\">RPCS3 Wiki</a> for instructions.").arg(gui::utils::get_link_style());
		const QString ds3_other   = tr("The DualShock 3 handler is recommended for official DualShock 3 controllers.");
		const QString ds4_windows = tr("If you have any issues with the DualShock 4 handler, it might be caused by third-party tools such as DS4Windows. It's recommended that you disable them while using this handler.");
		const QString ds4_linux   = tr("In order to use the DualShock 4 handler, you might need to add udev rules to let RPCS3 access the controller.<br>See the <a %0 href=\"https://wiki.rpcs3.net/index.php?title=Help:Controller_Configuration\">RPCS3 Wiki</a> for instructions.").arg(gui::utils::get_link_style());
		const QString ds4_other   = tr("The DualShock 4 handler is recommended for official DualShock 4 controllers.");
		const QString dualsense_windows = tr("The DualSense handler is recommended for official DualSense controllers.");
		const QString dualsense_linux   = tr("The DualSense handler is recommended for official DualSense controllers.");
		const QString dualsense_other   = tr("The DualSense handler is recommended for official DualSense controllers.");
		const QString skateboard  = tr("The Skateboard handler is recommended for official RIDE skateboard controllers.");
		const QString move        = tr("The PS Move handler is recommended for official PS Move controllers.");
		const QString xinput      = tr("The XInput handler will work with Xbox controllers and many third-party PC-compatible controllers. Pressure sensitive buttons from SCP are supported when SCP's XInput1_3.dll is placed in the main RPCS3 directory. For more details, see the <a %0 href=\"https://wiki.rpcs3.net/index.php?title=Help:Controller_Configuration\">RPCS3 Wiki</a>.").arg(gui::utils::get_link_style());
		const QString evdev       = tr("The evdev handler should work with any controller that has Linux support.<br>If your joystick is not being centered properly, read the <a %0 href=\"https://wiki.rpcs3.net/index.php?title=Help:Controller_Configuration\">RPCS3 Wiki</a> for instructions.").arg(gui::utils::get_link_style());
		const QString mmjoy       = tr("The MMJoystick handler should work with almost any controller recognized by Windows. However, it is recommended that you use the more specific handlers if you have a controller that supports them.");
		const QString sdl         = tr("The SDL handler supports a variety of controllers across different platforms.");

		const QString orientation_reset  = tr("Resets the sensor orientation when pressed.<br>Toggle the checkbox to enable or disable the orientation feature.<br>Currently only used for PS Move interactions.");
		const QString analog_limiter     = tr("Applies the stick multipliers while this special button is pressed.<br>Enable \"Toggle\" if you want to toggle the analog limiter on button press instead.<br>If no button has been assigned, the stick multipliers are always applied.");
		const QString pressure_intensity = tr("Controls the intensity of pressure sensitive buttons while this special button is pressed.<br>Enable \"Toggle\" if you want to toggle the intensity on button press instead.<br>Use the percentage to change how hard you want to press a button.");
		const QString pressure_deadzone  = tr("Controls the deadzone of pressure sensitive buttons. It determines how far the button has to be pressed until it is recognized by the game. The resulting range will be projected onto the full button sensitivity range.");
		const QString squircle_factor    = tr("The actual DualShock 3's stick range is not circular but formed like a rounded square (or squircle) which represents the maximum range of the emulated sticks. You can use the squircle values to modify the stick input if your sticks can't reach the corners of that range. A value of 0 does not apply any so called squircling. A value of 8000 is usually recommended.");
		const QString stick_multiplier   = tr("The stick multipliers can be used to change the sensitivity of your stick movements.<br>The default setting is 1 and represents normal input.");
		const QString stick_deadzones    = tr("A stick's deadzone determines how far the stick has to be moved until it is fully recognized by the game. The resulting range will be projected onto the full input range in order to give you a smooth experience. Movement inside the deadzone is simulated using the anti-deadzone slider (default is 13%), so don't worry if there is still movement shown in the emulated stick preview.");
		const QString vibration          = tr("The PS3 activates two motors (large and small) to handle controller vibrations.<br>You can enable, disable or even switch these signals for the currently selected pad here.<br>The game sends values from 0-255 to activate the motors.<br>Any value smaller or equal the threshold will be set to 0. This is 63 by default for pad handlers other than DualShock3 in order to emulate the DualShock3's behavior.");
		const QString motion_controls    = tr("Use this to configure the gamepad motion controls.");
		const QString emulated_preview   = tr("The emulated stick values (red dots) in the stick preview represent the actual stick positions as they will be visible to the game. The actual DualShock 3's stick range is not circular but formed like a rounded square (or squircle) which represents the maximum range of the emulated sticks. The blue regular dots represent the raw stick values (including stick multipliers) before they are converted for ingame usage.");
		const QString trigger_deadzones  = tr("A trigger's deadzone determines how far the trigger has to be moved until it is recognized by the game. The resulting range will be projected onto the full input range in order to give you a smooth experience.");
		const QString stick_lerp         = tr("With keyboards, you are inevitably restricted to 8 stick directions (4 straight + 4 diagonal). Furthermore, the stick will jump to the maximum value of the chosen direction immediately when a key is pressed. The stick interpolation can be used to work-around both of these issues by smoothening out these directional changes. The lower the value, the longer you have to press or release a key until the maximum amplitude is reached.");
		const QString mouse_deadzones    = tr("The mouse deadzones represent the games' own deadzones on the x and y axes. Games usually enforce their own deadzones to filter out small unwanted stick movements. In consequence, mouse input feels unintuitive since it relies on immediate responsiveness. You can change these values temporarily during gameplay in order to find out the optimal values for your game (Alt+T and Alt+Y for x, Alt+U and Alt+I for y).");
		const QString mouse_acceleration = tr("The mouse acceleration can be used to amplify your mouse movements on the x and y axes. Increase these values if your mouse movements feel too slow while playing a game. You can change these values temporarily during gameplay in order to find out the optimal values (Alt+G and Alt+H for x, Alt+J and Alt+K for y). Keep in mind that modern mice usually provide different modes and settings that can be used to change mouse movement speeds as well.");
		const QString mouse_movement     = tr("The mouse movement mode determines how the mouse movement is translated to pad input.<br>Use the relative mode for traditional mouse movement.<br>Use the absolute mode to use the mouse's distance to the center of the screen as input value.");
		const QString button_assignment  = tr("Left-click: remap this button.<br>Shift + Left-click: add an additional button mapping.<br>Alt + Left-click: differentiate between trigger press and release (only XInput for now).<br>Right-click: clear this button mapping.");

	} gamepad_settings;
};
