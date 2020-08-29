#pragma once

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

		const QString libraries_manual             = tr("Allows the user to manually choose the LLE libraries to load.\nIf unsure, don't use this option. Nothing will work if you use this.");
		const QString libraries_both               = tr("Load libsysmodule.sprx and chosen list of libraries. Option for backward compatibility.\nIf unsure, don't use this option.");
		const QString libraries_liblv2both         = tr("Loads liblv2.sprx and chosen list of libraries.\nIf unsure, don't use this option.");
		const QString libraries_liblv2list         = tr("Loads liblv2.sprx and nothing but selected libraries.\nDon't use this option.");
		const QString libraries_liblv2             = tr("This closely emulates how games can load and unload system module files on a real PlayStation 3.\nSome games require this.\nThis is the preferred option.");

		const QString debug_console_mode           = tr("Increases the amount of usable system memory to match a DECR console and more.\nCauses some software to behave differently than on retail hardware.");
		const QString silence_all_logs             = tr("Stop writing any logs after game startup. Don't use unless you believe it's necessary.");
		const QString read_color                   = tr("Initializes render target memory using vm memory.");
		const QString read_depth                   = tr("Initializes render target memory using vm memory.");
		const QString dump_depth                   = tr("Writes depth buffer values to vm memory.");
		const QString disable_on_disk_shader_cache = tr("Disables the loading and saving of shaders from and to the shader cache in the data directory.");
		const QString relaxed_zcull                = tr("Changes ZCULL report synchronization behaviour to be more forgiving to PC architecture.\nCan improve performance in some games but can also cause stability issues or drastically reduced performance in others.\nUse with caution and if performance or stability is compromised, keep this disabled.");
		const QString max_spurs_threads            = tr("Limits the maximum number of SPURS threads in each thread group.\nMay improve performance in some cases, especially on systems with limited number of hardware threads.\nLimiting the number of threads is likely to cause crashes; it's recommended to keep this at default value.");
		const QString sleep_timers_accuracy        = tr("Changes the sleep period accuracy.\n'As Host' uses default accuracy of the underlying operating system, while 'All Timers' attempts to improve it.\n'Usleep Only' limits the adjustments to usleep syscall only.\nCan affect performance in unexpected ways.");
		const QString vblank_rate                  = tr("Adjusts the frequency of vertical blanking signals that the emulator sends.\nAffects timing of events which rely on these signals.");
		const QString clocks_scale                 = tr("Changes the scale of emulated system time.\nAffects software which uses system time to calculate things such as dynamic timesteps.");
		const QString wake_up_delay                = tr("Try fiddling with this setting when encountering unstable games. The higher value, the better stability it may provide.\nIncrements/Decrements for each test should be around 100μs to 200μs until finding the best value for optimal stability.\nValues above 1000μs may cause noticeable performance penalties, use with caution.");
		const QString disabled_from_global         = tr("Do not change this setting globally.\nRight-click the game in game list and choose \"Configure\" instead.");

		// audio

		const QString audio_out                 = tr("XAudio2 is the recommended option and should be used whenever possible.\nOpenAL uses a cross-platform approach and is the next best alternative.");
		const QString audio_out_linux           = tr("OpenAL uses a cross-platform approach and supports audio buffering, so it is the recommended option.\nPulseAudio uses the native Linux sound system, and is the next best alternative. If neither are available, ALSA can be used instead.");
		const QString audio_dump                = tr("Saves all audio as a raw wave file. If unsure, leave this unchecked.");
		const QString convert                   = tr("Uses 16-bit audio samples instead of default 32-bit floating point.\nUse with buggy audio drivers if you have no sound or completely broken sound.");
		const QString downmix                   = tr("Uses chosen audio output instead of default 7.1 surround sound.\nUse downmix to stereo with stereo audio devices. Use 5.1 or higher only if you are using a surround sound audio system.");
		const QString master_volume             = tr("Controls the overall volume of the emulation.\nValues above 100% might reduce the audio quality.");
		const QString enable_buffering          = tr("Enables audio buffering, which reduces crackle/stutter but increases audio latency (requires XAudio2 or OpenAL).");
		const QString audio_buffer_duration     = tr("Target buffer duration in milliseconds.\nHigher values make the buffering algorithm's job easier, but may introduce noticeable audio latency.");
		const QString enable_time_stretching    = tr("Enables time stretching - requires buffering to be enabled.\nReduces crackle/stutter further, but may cause a very noticeable reduction in audio quality on slower CPUs.");
		const QString time_stretching_threshold = tr("Buffer fill level (in percentage) below which time stretching will start.");
		const QString microphone                = tr("Standard should be used for most games.\nSingStar emulates a SingStar device and should be used with SingStar games.\nReal SingStar should only be used with a REAL SingStar device with SingStar games.\nRocksmith should be used with a Rocksmith dongle.");

		// cpu

		const QString ppu_precise               = tr("Interprets PPU code with absolute accuracy.\nThis is the most accurate Interpreter, but very slow to play games with.\nYou may try this as a last resort if you encounter odd bugs or crashes.\nIf unsure, use PPU Interpreter Fast or PPU Recompiler (LLVM).");
		const QString ppu_fast                  = tr("Interprets PPU code with sacrificed accuracy in order to achieve better performance.\nThis is the fastest interpreter.\nIt very rarely breaks games even in comparison to the Precise option.\nTry this if PPU Recompiler (LLVM) fails.");
		const QString ppu_llvm                  = tr("Recompiles and caches the game's PPU code using the LLVM Recompiler once before running it for the first time.\nThis is by far the fastest option and should always be used.\nShould you face compatibility issues, fall back to one of the Interpreters and retry.\nIf unsure, use this option.");
		const QString spu_precise               = tr("Interprets SPU code with absolute accuracy.\nThis is extremely slow but may fix broken graphics in some games.");
		const QString spu_fast                  = tr("Interprets SPU code with sacrificed accuracy in order to achieve better performance.\nThis is slower than the SPU Recompiler but significantly faster than the precise interpreter.\nGames rarely need this however.");
		const QString spu_asmjit                = tr("Recompiles the game's SPU code using the ASMJIT Recompiler.\nThis is the fast option with very good compatibility.\nIf unsure, use this option.");
		const QString spu_llvm                  = tr("Recompiles and caches the game's SPU code using the LLVM Recompiler before running which adds extra start-up time.\nThis is the fastest option with very good compatibility.\nIf you experience issues, use the ASMJIT Recompiler.");
		const QString accurate_xfloat           = tr("Adds extra accuracy to SPU float vectors processing.\nFixes bugs in various games at the cost of performance.\nThis setting is only applied when SPU Decoder is set to Fast or LLVM.");
		const QString spu_cache                 = tr("Caches compiled SPU modules on disc.\nShould normally stay enabled.\nDisable this if the cache becomes too large.\nDisabling it does not remove the existing cache.");
		const QString enable_thread_scheduler   = tr("Allows RPCS3 to manually schedule physical cores to run specific tasks on, instead of letting the OS handle it.\nVery useful on Windows, especially for AMD Ryzen systems where it can give huge performance gains.\nNote: This function is only implemented for AMD Ryzen CPUs.");
		const QString lower_spu_thread_priority = tr("Runs SPU threads with lower priority than PPU threads.\nUsually faster on an i3 or i5, possibly slower or no difference on an i7 or Ryzen.");
		const QString spu_loop_detection        = tr("Try to detect loop conditions in SPU kernels and use them as scheduling hints.\nImproves performance and reduces CPU usage.\nMay cause severe audio stuttering in rare cases.");
		const QString enable_tsx                = tr("Enable usage of TSX instructions.\nNeeds to be forced on some Haswell or Broadwell CPUs.\nForcing this on older Hardware can lead to system instability, use it with caution.");
		const QString spu_block_size            = tr("This option controls the SPU analyser, particularly the size of compiled units. The Mega and Giga modes may improve performance by tying smaller units together, decreasing the number of compiled units but increasing their size.\nUse the Safe mode for maximum compatibility.");
		const QString preferred_spu_threads     = tr("Some SPU stages are sensitive to race conditions and allowing a limited number at a time helps alleviate performance stalls.\nSetting this to a smaller value might improve performance and reduce stuttering in some games.\nLeave this on auto if performance is negatively affected when setting a small value.");

		// debug

		const QString ppu_debug                    = tr("Creates PPU logs.\nOnly useful to developers.\nNever use this.");
		const QString spu_debug                    = tr("Creates SPU logs.\nOnly useful to developers.\nNever use this.");
		const QString set_daz_and_ftz              = tr("Sets special MXCSR flags to debug errors in SSE operations.\nOnly used in PPU thread when it's not precise.\nOnly useful to developers.\nNever use this.");
		const QString accurate_getllar             = tr("Accurately processes SPU MFC_GETLLAR operation.");
		const QString accurate_putlluc             = tr("Accurately processes SPU MFC_PUTLLUC operation.");
		const QString accurate_llvm_dfma           = tr("Provides extra accuracy on FMA instructions at the cost of performance.\nWhile disabling it might give a decent performance boost if your CPU doesn't support FMA, it may also introduce subtle bugs that otherwise do not occur.\nYou can't disable it if your CPU supports FMA.");
		const QString accurate_vector_nan          = tr("Forces the floating point NaN (Not A Number) values outputted from PPU vector instructions to be accurate to the real hardware. (0x7FC00000)");
		const QString accurate_rsx_access          = tr("Forces RSX pauses on SPU MFC_GETLLAR and SPU MFC_PUTLLUC operations.");
		const QString hook_static_functions        = tr("Allows to hook some functions like 'memcpy' replacing them with high-level implementations. May do nothing or break things. Experimental.");
		const QString gl_legacy_buffers            = tr("Enables use of classic OpenGL buffers which allows capturing tools to work with RPCS3 e.g RenderDoc.\nIf unsure, don't use this option.");
		const QString force_high_pz                = tr("Only useful when debugging differences in GPU hardware.\nNot necessary for average users.\nIf unsure, don't use this option.");
		const QString debug_output                 = tr("Enables the selected API's inbuilt debugging functionality.\nWill cause severe performance degradation especially with Vulkan.\nOnly useful to developers.\nIf unsure, don't use this option.");
		const QString debug_overlay                = tr("Provides a graphical overlay of various debugging information.\nIf unsure, don't use this option.");
		const QString log_shader_programs          = tr("Dump game shaders to file. Only useful to developers.\nIf unsure, don't use this option.");
		const QString disable_occlusion_queries    = tr("Disables running occlusion queries. Minor to moderate performance boost.\nMight introduce issues with broken occlusion e.g missing geometry and extreme pop-in.");
		const QString force_cpu_blit_emulation     = tr("Forces emulation of all blit and image manipulation operations on the CPU.\nRequires 'Write Color Buffers' option to also be enabled in most cases to avoid missing graphics.\nSignificantly degrades performance but is more accurate in some cases.\nThis setting overrides the 'GPU texture scaling' option.");
		const QString disable_vulkan_mem_allocator = tr("Disables the custom Vulkan memory allocator and reverts to direct calls to VkAllocateMemory/VkFreeMemory.");
		const QString disable_fifo_reordering      = tr("Disables RSX FIFO optimizations completely. Draws are processed as they are received by the DMA puller.");
		const QString gpu_texture_scaling          = tr("Force all texture transfer, scaling and conversion operations on the GPU.\nMay cause texture corruption in some cases.");
		const QString strict_texture_flushing      = tr("Forces texture flushing even in situations where it is not necessary/correct. Known to cause visual artifacts, but useful for debugging certain texture cache issues.");
		const QString disable_native_fp16          = tr("Disables hardware half-float support which is known to cause problems in some rare cases on some GPUs.");
		const QString enable_3d                    = tr("Enables 3D stereo rendering.\nNote that only anaglyph viewing is supported at the moment.");

		// emulator

		const QString exit_on_stop                 = tr("Automatically close RPCS3 when closing a game, or when a game closes itself.");
		const QString start_on_boot                = tr("Leave this enabled unless you are a developer.");
		const QString start_game_fullscreen        = tr("Automatically puts the game window in fullscreen.\nDouble click on the game window or press alt+enter to toggle fullscreen and windowed mode.");
		const QString prevent_display_sleep        = tr("Prevent the display from sleeping while a game is running.\nThis requires the org.freedesktop.ScreenSaver D-Bus service on Linux.\nThis option will be disabled if the current platform does not support display sleep control.");
		const QString game_window_title_format     = tr("Configure the game window title.\nChanging this and/or adding the framerate may cause buggy or outdated recording software to not notice RPCS3.");
		const QString resize_on_boot               = tr("Automatically resizes the game window on boot.\nThis does not change the internal game resolution.");
		const QString show_trophy_popups           = tr("Show trophy pop-ups when a trophy is unlocked.");
		const QString disable_mouse                = tr("Disables the activation of fullscreen mode per double-click while the game screen is active.\nCheck this if you want to play with mouse and keyboard (for example with UCR).");
		const QString disable_kb_hotkeys           = tr("Disables keyboard hotkeys such as Ctrl-S, Ctrl-E, Ctrl-R, Ctrl-P while the game screen is active.\nCheck this if you want to play with mouse and keyboard.");
		const QString max_llvm_threads             = tr("Limits the maximum number of threads used for the initial PPU and SPU module compilation.\nLower this in order to increase performance of other open applications.\nThe default uses all available threads.");
		const QString show_mouse_in_fullscreen     = tr("Shows the mouse cursor when the fullscreen mode is active.\nCurrently this may not work every time.");
		const QString hide_mouse_on_idle           = tr("Hides the mouse cursor if no mouse movement is detected for the configured time.");
		const QString show_shader_compilation_hint = tr("Shows 'Compiling shaders' hint using the native overlay.");
		const QString use_native_interface         = tr("Enables use of native HUD within the game window that can interact with game controllers.\nWhen disabled, regular Qt dialogs are used instead.\nCurrently, the on-screen keyboard only supports the English key layout.");

		const QString perf_overlay_enabled                 = tr("Enables or disables the performance overlay.");
		const QString perf_overlay_framerate_graph_enabled = tr("Enables or disables the framerate graph.");
		const QString perf_overlay_frametime_graph_enabled = tr("Enables or disables the frametime graph.");
		const QString perf_overlay_position                = tr("Sets the on-screen position (quadrant) of the performance overlay.");
		const QString perf_overlay_detail_level            = tr("Controls the amount of information displayed on the performance overlay.");
		const QString perf_overlay_update_interval         = tr("Sets the time interval in which the performance overlay is being updated (measured in milliseconds).");
		const QString perf_overlay_font_size               = tr("Sets the font size of the performance overlay (measured in pixels).");
		const QString perf_overlay_opacity                 = tr("Sets the opacity of the performance overlay (measured in %).");
		const QString perf_overlay_margin_x                = tr("Sets the horizontal distance to the screen border relative to the screen quadrant (measured in pixels).");
		const QString perf_overlay_margin_y                = tr("Sets the vertical distance to the screen border relative to the screen quadrant (measured in pixels).");
		const QString perf_overlay_center_x                = tr("Centers the performance overlay horizontally and overrides the horizontal margin.");
		const QString perf_overlay_center_y                = tr("Centers the performance overlay vertically and overrides the vertical margin.");

		const QString shader_load_bg_enabled   = tr("Shows a background image during the native shader loading dialog/loading screen.\nBy default the used image will be <gamedir>/PS3_GAME/PIC1.PNG or <gamedir>/PS3_GAME/PIC0.PNG.");
		const QString shader_load_bg_darkening = tr("Changes the background image darkening effect strength of the native shader loading dialog.\nThis may be used to improve readability and/or aesthetics.");
		const QString shader_load_bg_blur      = tr("Changes the background image blur effect strength of the native shader loading dialog.\nThis may be used to improve readability and/or aesthetics.");

		// gpu

		const QString renderer                   = tr("Vulkan is the fastest renderer. OpenGL is the most accurate renderer.\nIf unsure, use Vulkan. Should you have any compatibility issues, fall back to OpenGL.");
		const QString resolution                 = tr("This setting will be ignored if the Resolution Scale is set to anything other than 100%!\nLeave this on 1280x720, every PS3 game is compatible with this resolution.\nOnly use 1920x1080 if the game supports it.\nRarely due to emulation bugs some games will only render at low resolutions like 480p.");
		const QString graphics_adapter           = tr("On multi GPU systems select which GPU to use in RPCS3 when using Vulkan.\nThis is not needed when using OpenGL.");
		const QString aspect_ratio               = tr("Leave this on 16:9 unless you have a 4:3 monitor.");
		const QString frame_limit                = tr("Off is the best option as it performs faster.\nUsing the frame limiter will add extra overhead and slow down the game.\nHowever, some games will crash if the frame rate is too high.\nIf that happens, set value to anything other than Off.");
		const QString anti_aliasing              = tr("Emulate PS3 multisampling layout.\nCan fix some otherwise difficult to solve graphics glitches.\nLow to moderate performance hit depending on your GPU hardware.");
		const QString anisotropic_filter         = tr("Higher values increase sharpness of textures on sloped surfaces at the cost of GPU resources.\nModern GPUs can handle this setting just fine even at 16x.\nKeep this on Automatic if you want to use the original setting used by a real PS3.");
		const QString resolution_scale           = tr("Scales the game's resolution by the given percentage.\nThe base resolution is always 1280x720.\nSet this value to 100% if you want to use the normal Resolution options.");
		const QString minimum_scalable_dimension = tr("Only framebuffers greater than this size will be upscaled.\nIncreasing this value might fix problems with missing graphics when upscaling, especially when Write Color Buffers is enabled.\nIf unsure, don't change this option.");
		const QString dump_color                 = tr("Enable this option if you get missing graphics or broken lighting ingame.\nMight degrade performance and introduce stuttering in some cases.\nRequired for Demon's Souls.");
		const QString vsync                      = tr("By having this off you might obtain a higher frame rate at the cost of tearing artifacts in the game.");
		const QString strict_rendering_mode      = tr("Enforces strict compliance to the API specification.\nMight result in degraded performance in some games.\nCan resolve rare cases of missing graphics and flickering.\nIf unsure, don't use this option.");
		const QString disable_vertex_cache       = tr("Disables the vertex cache.\nMight resolve missing or flickering graphics output.\nMay degrade performance.");
		const QString stretch_to_display_area    = tr("Overrides the aspect ratio and stretches the image to the full display area.");
		const QString multithreaded_rsx          = tr("Offloads some RSX operations to a secondary thread.\nMay improve performance for some high-core processors.\nMay cause slowdown in some situations due to the extra worker thread load.");

		const QString legacy_shader_recompiler        = tr("Disables asynchronous shader compilation.\nFixes missing graphics while shaders are compiling but introduces severe stuttering or lag.\nUse this if you do not want to deal with graphics pop-in, or for testing before filing any bug reports.");
		const QString async_shader_recompiler         = tr("This is the recommended option.\nIf a shader is not found in the cache, nothing will be rendered for this shader until it has compiled.\nYou may experience graphics pop-in.");
		const QString async_with_shader_interpreter   = tr("Hybrid rendering mode.\nIf a shader is not found in the cache, the interpreter will be used to render approximated graphics for this shader until it has compiled.");
		const QString shader_interpreter_only         = tr("All rendering is handled by the interpreter with no attempt to compile native shaders.\nThis mode is very slow and experimental.");

		// gui

		const QString log_limit          = tr("Sets the maximum amount of blocks that the log can display.\nThis usually equals the number of lines.\nSet 0 in order to remove the limit.");
		const QString tty_limit          = tr("Sets the maximum amount of blocks that the tty can display.\nThis usually equals the number of lines.\nSet 0 in order to remove the limit.");
		const QString configs            = tr("Only useful to developers.\nIf unsure, don't use this option.");
		const QString stylesheets        = tr("Changes the overall look of RPCS3.\nChoose a stylesheet and click Apply to change between styles.");
		const QString show_welcome       = tr("Shows the initial welcome screen upon starting RPCS3.");
		const QString show_exit_game     = tr("Shows a confirmation dialog when the game window is being closed.");
		const QString show_boot_game     = tr("Shows a confirmation dialog when a game was booted while another game is running.");
		const QString show_pkg_install   = tr("Shows a dialog when packages were installed successfully.");
		const QString show_pup_install   = tr("Shows a dialog when firmware was installed successfully.");
		const QString check_update_start = tr("Checks if an update is available on startup and asks if you want to update.\nIf \"Background\" is selected, the check is done silently in the background and a new download option is shown in the top right corner of the menu if a new version was found.");
		const QString use_rich_presence  = tr("Enables use of Discord Rich Presence to show what game you are playing on Discord.\nRequires a restart of RPCS3 to completely close the connection.");
		const QString discord_state      = tr("Tell your friends what you are doing.");
		const QString custom_colors      = tr("Prioritize custom user interface colors over properties set in stylesheet.");

		// input

		const QString pad_handler       = tr("If you want to use the keyboard to control, select the Keyboard option.\nIf you have a DualShock 4, select DualShock 4.\nIf you have an Xbox controller, or another compatible device, use XInput.\nOlder controllers such as PS2 controllers with an adapter usually work fine with MMJoystick.\nCheck button mappings in the Windows control panel.");
		const QString pad_handler_linux = tr("If you want to use the keyboard to control, select the Keyboard option.\nIf you have a DualShock 4, select DualShock 4.");
		const QString keyboard_handler  = tr("Some games support native keyboard input.\nBasic will work in these cases.");
		const QString mouse_handler     = tr("Some games support native mouse input.\nBasic will work in these cases.");
		const QString camera            = tr("Camera support is not implemented, leave this on null.");
		const QString camera_type       = tr("Camera support is not implemented, leave this on unknown.");
		const QString move              = tr("PlayStation Move support.\nFake: Experimental! This maps Move controls to DS3 controller mappings.\nMouse: Emulate PSMove with Mouse handler.");

		// network

		const QString net_status    = tr("If set to Connected, RPCS3 will allow programs to use your internet connection.");
		const QString psn_status    = tr("If set to Simulated, RPCS3 will fake PSN connection as best as it can.");
		const QString dns           = tr("DNS used to resolve hostnames by applications.");
		const QString dns_swap      = tr("DNS Swap List.");

		// system

		const QString system_language         = tr("Some games may fail to boot if the system language is not available in the game itself.\nOther games will switch language automatically to what is selected here.\nIt is recommended leaving this on a language supported by the game.");
		const QString keyboard_type           = tr("Sets the used keyboard layout.\nCurrently only US, Japanese and German layouts are fully supported at this moment.");
		const QString enter_button_assignment = tr("The button used for enter/accept/confirm in system dialogs.\nChange this to use the Circle button instead, which is the default configuration on Japanese systems and in many Japanese games.\nIn these cases having the cross button assigned can often lead to confusion.");
		const QString enable_host_root        = tr("Required for some Homebrew.\nIf unsure, don't use this option.");
		const QString limit_cache_size        = tr("Automatically removes older files from disk cache on boot if it grows larger than the specified value.\nGames can use the cache folder to temporarily store data outside of system memory. It is not used for long-term storage.");
	} settings;

	const struct gamepad_settings
	{
		const QString null        = tr("This controller is disabled and will appear as disconnected to software. Choose another handler to enable it.");
		const QString ldd_pad     = tr("This port is currently assigned to a custom controller by the application and can't be changed.");
		const QString keyboard    = tr("While it is possible to use a keyboard as a pad in RPCS3, the use of an actual controller is strongly recommended.<br>To bind mouse movement to a button or joystick, click on the desired button to activate it, then click and hold while dragging the mouse to a direction.");
		const QString ds3_windows = tr("In order to use the DualShock 3 handler, you need to install the official DualShock 3 driver first.<br>See the <a href=\"https://wiki.rpcs3.net/index.php?title=Help:Controller_Configuration\">RPCS3 Wiki</a> for instructions.");
		const QString ds3_linux   = tr("In order to use the DualShock 3 handler, you might need to add udev rules to let RPCS3 access the controller.\nSee the <a href=\"https://wiki.rpcs3.net/index.php?title=Help:Controller_Configuration\">RPCS3 Wiki</a> for instructions.");
		const QString ds3_other   = tr("The DualShock 3 handler is recommended for official DualShock 3 controllers.");
		const QString ds4_windows = tr("If you have any issues with the DualShock 4 handler, it might be caused by third-party tools such as DS4Windows. It's recommended that you disable them while using this handler.");
		const QString ds4_linux   = tr("In order to use the DualShock 4 handler, you might need to add udev rules to let RPCS3 access the controller.\nSee the <a href=\"https://wiki.rpcs3.net/index.php?title=Help:Controller_Configuration\">RPCS3 Wiki</a> for instructions.");
		const QString ds4_other   = tr("The DualShock 4 handler is recommended for official DualShock 4 controllers.");
		const QString xinput      = tr("The XInput handler will work with Xbox controllers and many third-party PC-compatible controllers. Pressure sensitive buttons from SCP are supported when SCP's XInput1_3.dll is placed in the main RPCS3 directory. For more details, see the <a href=\"https://wiki.rpcs3.net/index.php?title=Help:Controller_Configuration\">RPCS3 Wiki</a>.");
		const QString evdev       = tr("The evdev handler should work with any controller that has linux support.<br>If your joystick is not being centered properly, read the <a href=\"https://wiki.rpcs3.net/index.php?title=Help:Controller_Configuration\">RPCS3 Wiki</a> for instructions.");
		const QString mmjoy       = tr("The MMJoystick handler should work with almost any controller recognized by Windows. However, it is recommended that you use the more specific handlers if you have a controller that supports them.");

		const QString squircle_factor    = tr("The actual DualShock 3's stick range is not circular but formed like a rounded square (or squircle) which represents the maximum range of the emulated sticks. You can use the squircle values to modify the stick input if your sticks can't reach the corners of that range. A value of 0 does not apply any so called squircling. A value of 8000 is usually recommended.");
		const QString stick_multiplier   = tr("The stick multipliers can be used to change the sensitivity of your stick movements.<br>The default setting is 1 and represents normal input.");
		const QString stick_deadzones    = tr("A stick's deadzone determines how far the stick has to be moved until it is fully recognized by the game. The resulting range will be projected onto the full input range in order to give you a smooth experience. Movement inside the deadzone is actually simulated as a real DualShock 3's deadzone of ~13%, so don't worry if there is still movement shown in the emulated stick preview.");
		const QString vibration          = tr("The PS3 activates two motors (large and small) to handle controller vibrations.<br>You can enable, disable or even switch these signals for the currently selected pad here.");
		const QString emulated_preview   = tr("The emulated stick values (red dots) in the stick preview represent the actual stick positions as they will be visible to the game. The actual DualShock 3's stick range is not circular but formed like a rounded square (or squircle) which represents the maximum range of the emulated sticks. The blue regular dots represent the raw stick values (including stick multipliers) before they are converted for ingame usage.");
		const QString trigger_deadzones  = tr("A trigger's deadzone determines how far the trigger has to be moved until it is recognized by the game. The resulting range will be projected onto the full input range in order to give you a smooth experience.");
		const QString stick_lerp         = tr("With keyboards you are inevitably restricted to 8 stick directions (4 straight + 4 diagonal). Furthermore when a key is pressed the stick will jump to the maximum value of the chosen direction immediately. The stick interpolation can be used to work-around both of these issues by smoothening out these directional changes. The lower the value, the longer you have to press or release a key until the maximum amplitude is reached.");
		const QString mouse_deadzones    = tr("The mouse deadzones represent the games' own deadzones on the x and y axes. Games usually enforce their own deadzones to filter out small unwanted stick movements. In consequence mouse input feels unintuitive since it relies on immediate responsiveness. You can change these values temporarily during gameplay in order to find out the optimal values for your game (Alt+T and Alt+Y for x, Alt+U and Alt+I for y).");
		const QString mouse_acceleration = tr("The mouse acceleration can be used to amplify your mouse movements on the x and y axes. Increase these values if your mouse movements feel too slow while playing a game. You can change these values temporarily during gameplay in order to find out the optimal values (Alt+G and Alt+H for x, Alt+J and Alt+K for y). Keep in mind that modern mice usually provide different modes and settings that can be used to change mouse movement speeds as well.");

	} gamepad_settings;
};
