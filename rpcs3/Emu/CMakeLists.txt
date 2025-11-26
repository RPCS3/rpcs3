add_library(rpcs3_emu STATIC
    cache_utils.cpp
    games_config.cpp
    IdManager.cpp
    localized_string.cpp
    savestate_utils.cpp
    scoped_progress_dialog.cpp
    stb_image.cpp
    System.cpp
    system_config.cpp
    system_config_types.cpp
    system_progress.cpp
    system_utils.cpp
    vfs_config.cpp
    VFS.cpp
    GDB.cpp
    title.cpp
    perf_meter.cpp
    perf_monitor.cpp
    IPC_config.cpp
    IPC_socket.cpp
)

if(USE_LTO)
    set_target_properties(rpcs3_emu PROPERTIES INTERPROCEDURAL_OPTIMIZATION ON)
endif()

if(USE_ASAN)
    set_source_files_properties(../../Utilities/Thread.cpp PROPERTIES COMPILE_DEFINITIONS USE_ASAN)
endif()

if(NOT USE_SYSTEM_OPENAL)
    target_compile_definitions(rpcs3_emu PUBLIC AL_LIBTYPE_STATIC)
endif()

if(HAS_MEMORY_BREAKPOINTS)
    target_compile_definitions(rpcs3_emu PRIVATE RPCS3_HAS_MEMORY_BREAKPOINTS)
endif()

# For stdafx.h
target_include_directories(rpcs3_emu
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_include_directories(rpcs3_emu
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty
)

# Utilities
target_sources(rpcs3_emu PRIVATE
    ../util/atomic.cpp
    ../util/console.cpp
    ../util/emu_utils.cpp
    ../util/media_utils.cpp
    ../util/video_provider.cpp
    ../util/logs.cpp
    ../util/yaml.cpp
    ../util/vm_native.cpp
    ../util/dyn_lib.cpp
    ../util/sysinfo.cpp
    ../util/cpu_stats.cpp
    ../util/serialization_ext.cpp
    ../../Utilities/bin_patch.cpp
    ../../Utilities/cheat_info.cpp
    ../../Utilities/cond.cpp
    ../../Utilities/Config.cpp
    ../../Utilities/date_time.cpp
    ../../Utilities/File.cpp
    ../../Utilities/JITASM.cpp
    ../../Utilities/JITLLVM.cpp
    ../../Utilities/LUrlParser.cpp
    ../../Utilities/mutex.cpp
    ../../Utilities/rXml.cpp
    ../../Utilities/sema.cpp
    ../../Utilities/simple_ringbuf.cpp
    ../../Utilities/stack_trace.cpp
    ../../Utilities/StrFmt.cpp
    ../../Utilities/Thread.cpp
    ../../Utilities/version.cpp
)
if(APPLE)
    target_sources(rpcs3_emu PRIVATE
        ../../darwin/util/sysinfo_darwin.mm
    )
endif()

target_include_directories(rpcs3_emu PUBLIC "${CMAKE_SOURCE_DIR}")

set_source_files_properties("../../Utilities/JITLLVM.cpp" "../../Utilities/JITASM.cpp" PROPERTIES
    COMPILE_FLAGS "$<IF:$<CXX_COMPILER_ID:MSVC>,/GR-,-fno-rtti>"
    SKIP_PRECOMPILE_HEADERS ON
)

set_source_files_properties("../util/yaml.cpp" PROPERTIES
    COMPILE_FLAGS "$<IF:$<CXX_COMPILER_ID:MSVC>,/EHsc,-fexceptions>"
    SKIP_PRECOMPILE_HEADERS ON
)

# Crypto
target_sources(rpcs3_emu PRIVATE
    ../Crypto/aes.cpp
    ../Crypto/aesni.cpp
    ../Crypto/decrypt_binaries.cpp
    ../Crypto/ec.cpp
    ../Crypto/key_vault.cpp
    ../Crypto/lz.cpp
    ../Crypto/md5.cpp
    ../Crypto/sha1.cpp
    ../Crypto/sha256.cpp
    ../Crypto/unedat.cpp
    ../Crypto/unpkg.cpp
    ../Crypto/unself.cpp
    ../Crypto/unzip.cpp
    ../Crypto/utils.cpp
)

# Loader
target_sources(rpcs3_emu PRIVATE
    ../Loader/disc.cpp
    ../Loader/ELF.cpp
    ../Loader/mself.cpp
    ../Loader/PSF.cpp
    ../Loader/PUP.cpp
    ../Loader/TAR.cpp
    ../Loader/TROPUSR.cpp
    ../Loader/TRP.cpp
)

# Audio
target_sources(rpcs3_emu PRIVATE
    Audio/audio_resampler.cpp
    Audio/audio_utils.cpp
    Audio/AudioDumper.cpp
    Audio/AudioBackend.cpp
    Audio/Cubeb/CubebBackend.cpp
    Audio/Cubeb/cubeb_enumerator.cpp
)

if(USE_FAUDIO)
    find_package(SDL3)
    if(SDL3_FOUND AND SDL3_VERSION VERSION_GREATER_EQUAL 3.2.0)
        target_sources(rpcs3_emu PRIVATE
            Audio/FAudio/FAudioBackend.cpp
            Audio/FAudio/faudio_enumerator.cpp
        )
        target_link_libraries(rpcs3_emu PUBLIC 3rdparty::faudio)
    endif()
endif()

if(WIN32)
    if(NOT MSVC)
        target_link_libraries(rpcs3_emu PRIVATE xaudio2_9)
    endif()
    target_sources(rpcs3_emu PRIVATE
        Audio/XAudio2/XAudio2Backend.cpp
        Audio/XAudio2/xaudio2_enumerator.cpp
    )
    target_compile_definitions(rpcs3_emu PRIVATE UNICODE _UNICODE _WIN32_WINNT=0x0A00)
    target_link_libraries(rpcs3_emu PRIVATE pdh bcrypt)
endif()

# Cell
target_sources(rpcs3_emu PRIVATE
    Cell/ErrorCodes.cpp
    Cell/MFC.cpp
    Cell/PPUAnalyser.cpp
    Cell/PPUDisAsm.cpp
    Cell/PPUFunction.cpp
    Cell/PPUInterpreter.cpp
    Cell/PPUModule.cpp
    Cell/PPUThread.cpp
    Cell/PPUTranslator.cpp
    Cell/RawSPUThread.cpp
    Cell/SPUAnalyser.cpp
    Cell/SPUASMJITRecompiler.cpp
    Cell/SPUDisAsm.cpp
    Cell/SPUInterpreter.cpp
    Cell/SPUCommonRecompiler.cpp
    Cell/SPULLVMRecompiler.cpp
    Cell/SPUThread.cpp
    Cell/lv2/lv2.cpp
    Cell/lv2/sys_bdemu.cpp
    Cell/lv2/sys_btsetting.cpp
    Cell/lv2/sys_cond.cpp
    Cell/lv2/sys_console.cpp
    Cell/lv2/sys_crypto_engine.cpp
    Cell/lv2/sys_config.cpp
    Cell/lv2/sys_dbg.cpp
    Cell/lv2/sys_event.cpp
    Cell/lv2/sys_event_flag.cpp
    Cell/lv2/sys_fs.cpp
    Cell/lv2/sys_game.cpp
    Cell/lv2/sys_gamepad.cpp
    Cell/lv2/sys_gpio.cpp
    Cell/lv2/sys_hid.cpp
    Cell/lv2/sys_interrupt.cpp
    Cell/lv2/sys_io.cpp
    Cell/lv2/sys_lwcond.cpp
    Cell/lv2/sys_lwmutex.cpp
    Cell/lv2/sys_memory.cpp
    Cell/lv2/sys_mmapper.cpp
    Cell/lv2/sys_mutex.cpp
    Cell/lv2/sys_net.cpp
    Cell/lv2/sys_net/lv2_socket.cpp
    Cell/lv2/sys_net/lv2_socket_native.cpp
    Cell/lv2/sys_net/lv2_socket_raw.cpp
    Cell/lv2/sys_net/lv2_socket_p2p.cpp
    Cell/lv2/sys_net/lv2_socket_p2ps.cpp
    Cell/lv2/sys_net/network_context.cpp
    Cell/lv2/sys_net/nt_p2p_port.cpp
    Cell/lv2/sys_net/sys_net_helpers.cpp
    Cell/lv2/sys_overlay.cpp
    Cell/lv2/sys_ppu_thread.cpp
    Cell/lv2/sys_process.cpp
    Cell/lv2/sys_prx.cpp
    Cell/lv2/sys_rsx.cpp
    Cell/lv2/sys_rsxaudio.cpp
    Cell/lv2/sys_rwlock.cpp
    Cell/lv2/sys_semaphore.cpp
    Cell/lv2/sys_spu.cpp
    Cell/lv2/sys_sm.cpp
    Cell/lv2/sys_ss.cpp
    Cell/lv2/sys_storage.cpp
    Cell/lv2/sys_time.cpp
    Cell/lv2/sys_timer.cpp
    Cell/lv2/sys_trace.cpp
    Cell/lv2/sys_tty.cpp
    Cell/lv2/sys_uart.cpp
    Cell/lv2/sys_usbd.cpp
    Cell/lv2/sys_vm.cpp
    Cell/Modules/cellAdec.cpp
    Cell/Modules/cellAtrac.cpp
    Cell/Modules/cellAtracMulti.cpp
    Cell/Modules/cellAtracXdec.cpp
    Cell/Modules/cellAudio.cpp
    Cell/Modules/cellAudioOut.cpp
    Cell/Modules/cellAuthDialog.cpp
    Cell/Modules/cellAvconfExt.cpp
    Cell/Modules/cellBgdl.cpp
    Cell/Modules/cellCamera.cpp
    Cell/Modules/cellCelp8Enc.cpp
    Cell/Modules/cellCelpEnc.cpp
    Cell/Modules/cellCrossController.cpp
    Cell/Modules/cellDaisy.cpp
    Cell/Modules/cellDmux.cpp
    Cell/Modules/cellDmuxPamf.cpp
    Cell/Modules/cellDtcpIpUtility.cpp
    Cell/Modules/cellFiber.cpp
    Cell/Modules/cellFont.cpp
    Cell/Modules/cellFontFT.cpp
    Cell/Modules/cell_FreeType2.cpp
    Cell/Modules/cellFs.cpp
    Cell/Modules/cellGame.cpp
    Cell/Modules/cellGameExec.cpp
    Cell/Modules/cellGcmSys.cpp
    Cell/Modules/cellGem.cpp
    Cell/Modules/cellGifDec.cpp
    Cell/Modules/cellHttp.cpp
    Cell/Modules/cellHttpUtil.cpp
    Cell/Modules/cellImeJp.cpp
    Cell/Modules/cellJpgDec.cpp
    Cell/Modules/cellJpgEnc.cpp
    Cell/Modules/cellKb.cpp
    Cell/Modules/cellKey2char.cpp
    Cell/Modules/cellL10n.cpp
    Cell/Modules/cellLibprof.cpp
    Cell/Modules/cellMic.cpp
    Cell/Modules/cellMouse.cpp
    Cell/Modules/cellMsgDialog.cpp
    Cell/Modules/cellMusic.cpp
    Cell/Modules/cellMusicDecode.cpp
    Cell/Modules/cellMusicSelectionContext.cpp
    Cell/Modules/cellMusicExport.cpp
    Cell/Modules/cellNetAoi.cpp
    Cell/Modules/cellNetCtl.cpp
    Cell/Modules/cellOskDialog.cpp
    Cell/Modules/cellOvis.cpp
    Cell/Modules/cellPad.cpp
    Cell/Modules/cellPamf.cpp
    Cell/Modules/cellPesmUtility.cpp
    Cell/Modules/cellPhotoDecode.cpp
    Cell/Modules/cellPhotoExport.cpp
    Cell/Modules/cellPhotoImport.cpp
    Cell/Modules/cellPngDec.cpp
    Cell/Modules/cellPngEnc.cpp
    Cell/Modules/cellPrint.cpp
    Cell/Modules/cellRec.cpp
    Cell/Modules/cellRemotePlay.cpp
    Cell/Modules/cellResc.cpp
    Cell/Modules/cellRtcAlarm.cpp
    Cell/Modules/cellRtc.cpp
    Cell/Modules/cellRudp.cpp
    Cell/Modules/cellSail.cpp
    Cell/Modules/cellSailRec.cpp
    Cell/Modules/cellSaveData.cpp
    Cell/Modules/cellScreenshot.cpp
    Cell/Modules/cellSearch.cpp
    Cell/Modules/cellSheap.cpp
    Cell/Modules/cellSpudll.cpp
    Cell/Modules/cellSpurs.cpp
    Cell/Modules/cellSpursJq.cpp
    Cell/Modules/cellSpursSpu.cpp
    Cell/Modules/cellSsl.cpp
    Cell/Modules/cellStorage.cpp
    Cell/Modules/cellSubDisplay.cpp
    Cell/Modules/cellSync2.cpp
    Cell/Modules/cellSync.cpp
    Cell/Modules/cellSysconf.cpp
    Cell/Modules/cellSysCache.cpp
    Cell/Modules/cellSysmodule.cpp
    Cell/Modules/cellSysutilAp.cpp
    Cell/Modules/cellSysutilAvc2.cpp
    Cell/Modules/cellSysutilAvc.cpp
    Cell/Modules/cellSysutilAvcExt.cpp
    Cell/Modules/cellSysutil.cpp
    Cell/Modules/cellSysutilMisc.cpp
    Cell/Modules/cellSysutilNpEula.cpp
    Cell/Modules/cellUsbd.cpp
    Cell/Modules/cellUsbpspcm.cpp
    Cell/Modules/cellUserInfo.cpp
    Cell/Modules/cellVdec.cpp
    Cell/Modules/cellVideoExport.cpp
    Cell/Modules/cellVideoOut.cpp
    Cell/Modules/cellVideoPlayerUtility.cpp
    Cell/Modules/cellVideoUpload.cpp
    Cell/Modules/cellVoice.cpp
    Cell/Modules/cellVpost.cpp
    Cell/Modules/cellWebBrowser.cpp
    Cell/Modules/HLE_PATCHES.cpp
    Cell/Modules/libad_async.cpp
    Cell/Modules/libad_core.cpp
    Cell/Modules/libmedi.cpp
    Cell/Modules/libmixer.cpp
    Cell/Modules/libsnd3.cpp
    Cell/Modules/libsynth2.cpp
    Cell/Modules/sceNp2.cpp
    Cell/Modules/sceNpClans.cpp
    Cell/Modules/sceNpCommerce2.cpp
    Cell/Modules/sceNp.cpp
    Cell/Modules/sceNpMatchingInt.cpp
    Cell/Modules/sceNpPlus.cpp
    Cell/Modules/sceNpSns.cpp
    Cell/Modules/sceNpTrophy.cpp
    Cell/Modules/sceNpTus.cpp
    Cell/Modules/sceNpUtil.cpp
    Cell/Modules/StaticHLE.cpp
    Cell/Modules/sys_crashdump.cpp
    Cell/Modules/sys_game_.cpp
    Cell/Modules/sys_heap.cpp
    Cell/Modules/sys_io_.cpp
    Cell/Modules/sys_libc_.cpp
    Cell/Modules/sys_libc.cpp
    Cell/Modules/sys_lv2dbg.cpp
    Cell/Modules/sys_lwcond_.cpp
    Cell/Modules/sys_lwmutex_.cpp
    Cell/Modules/sys_mempool.cpp
    Cell/Modules/sys_mmapper_.cpp
    Cell/Modules/sys_net_.cpp
    Cell/Modules/sys_ppu_thread_.cpp
    Cell/Modules/sys_prx_.cpp
    Cell/Modules/sysPrxForUser.cpp
    Cell/Modules/sys_rsxaudio_.cpp
    Cell/Modules/sys_spinlock.cpp
    Cell/Modules/sys_spu_.cpp
    Cell/Modules/libfs_utility_init.cpp
)

if(NOT MSVC)
    set_source_files_properties(Cell/PPUTranslator.cpp PROPERTIES
        COMPILE_FLAGS -fno-rtti
        SKIP_PRECOMPILE_HEADERS ON
    )
endif()

# CPU
target_sources(rpcs3_emu PRIVATE
    CPU/CPUThread.cpp
    CPU/CPUTranslator.cpp
)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64|arm64|aarch64")
    target_sources(rpcs3_emu PRIVATE
        CPU/Backends/AArch64/AArch64ASM.cpp
        CPU/Backends/AArch64/AArch64Common.cpp
        CPU/Backends/AArch64/AArch64JIT.cpp
        CPU/Backends/AArch64/AArch64Signal.cpp
    )
endif()


# Io
target_sources(rpcs3_emu PRIVATE
    Io/Buzz.cpp
    Io/camera_config.cpp
    Io/Dimensions.cpp
    Io/evdev_gun_handler.cpp
    Io/GameTablet.cpp
    Io/GHLtar.cpp
    Io/GunCon3.cpp
    Io/Infinity.cpp
    Io/interception.cpp
    Io/KamenRider.cpp
    Io/KeyboardHandler.cpp
    Io/midi_config_types.cpp
    Io/mouse_config.cpp
    Io/MouseHandler.cpp
    Io/pad_config.cpp
    Io/pad_config_types.cpp
    Io/pad_types.cpp
    Io/PadHandler.cpp
    Io/rb3drums_config.cpp
    Io/RB3MidiDrums.cpp
    Io/RB3MidiGuitar.cpp
    Io/RB3MidiKeyboard.cpp
    Io/recording_config.cpp
    Io/Skylander.cpp
    Io/TopShotElite.cpp
    Io/TopShotFearmaster.cpp
    Io/Turntable.cpp
    Io/usb_device.cpp
    Io/usb_vfs.cpp
    Io/usio.cpp
    Io/LogitechG27.cpp
    Io/LogitechG27Config.cpp
)

# Np
target_sources(rpcs3_emu PRIVATE
    NP/fb_helpers.cpp
    NP/np_cache.cpp
    NP/np_gui_cache.cpp
    NP/np_contexts.cpp
    NP/np_dnshook.cpp
    NP/np_requests_gui.cpp
    NP/np_handler.cpp
    NP/np_helpers.cpp
    NP/np_notifications.cpp
    NP/np_requests.cpp
    NP/signaling_handler.cpp
    NP/np_structs_extra.cpp
    NP/rpcn_client.cpp
    NP/rpcn_config.cpp
    NP/rpcn_countries.cpp
    NP/upnp_config.cpp
    NP/upnp_handler.cpp
    NP/ip_address.cpp
)

# Memory
target_sources(rpcs3_emu PRIVATE
    Memory/vm.cpp
)

# RSX
target_sources(rpcs3_emu PRIVATE
    RSX/Capture/rsx_capture.cpp
    RSX/Capture/rsx_replay.cpp
    RSX/Common/BufferUtils.cpp
    RSX/Common/surface_store.cpp
    RSX/Common/TextureUtils.cpp
    RSX/Common/texture_cache.cpp
    RSX/Common/texture_cache_types.cpp
    RSX/Core/RSXContext.cpp
    RSX/Core/RSXDisplay.cpp
    RSX/Core/RSXDrawCommands.cpp
    RSX/gcm_enums.cpp
    RSX/gcm_printing.cpp
    RSX/GSRender.cpp
    RSX/Host/MM.cpp
    RSX/Host/RSXDMAWriter.cpp
    RSX/Null/NullGSRender.cpp
    RSX/NV47/FW/draw_call.cpp
    RSX/NV47/FW/reg_context.cpp
    RSX/NV47/HW/common.cpp
    RSX/NV47/HW/nv0039.cpp
    RSX/NV47/HW/nv3089.cpp
    RSX/NV47/HW/nv308a.cpp
    RSX/NV47/HW/nv406e.cpp
    RSX/NV47/HW/nv4097.cpp
    RSX/Overlays/FriendsList/overlay_friends_list_dialog.cpp
    RSX/Overlays/HomeMenu/overlay_home_menu.cpp
    RSX/Overlays/HomeMenu/overlay_home_menu_components.cpp
    RSX/Overlays/HomeMenu/overlay_home_menu_main_menu.cpp
    RSX/Overlays/HomeMenu/overlay_home_menu_message_box.cpp
    RSX/Overlays/HomeMenu/overlay_home_menu_page.cpp
    RSX/Overlays/HomeMenu/overlay_home_menu_settings.cpp
    RSX/Overlays/HomeMenu/overlay_home_menu_savestate.cpp
    RSX/Overlays/Network/overlay_recvmessage_dialog.cpp
    RSX/Overlays/Network/overlay_sendmessage_dialog.cpp
    RSX/Overlays/Trophies/overlay_trophy_list_dialog.cpp
    RSX/Overlays/overlays.cpp
    RSX/Overlays/overlay_animated_icon.cpp
    RSX/Overlays/overlay_animation.cpp
    RSX/Overlays/overlay_compile_notification.cpp
    RSX/Overlays/overlay_controls.cpp
    RSX/Overlays/overlay_cursor.cpp
    RSX/Overlays/overlay_debug_overlay.cpp
    RSX/Overlays/overlay_edit_text.cpp
    RSX/Overlays/overlay_fonts.cpp
    RSX/Overlays/overlay_list_view.cpp
    RSX/Overlays/overlay_manager.cpp
    RSX/Overlays/overlay_media_list_dialog.cpp
    RSX/Overlays/overlay_message.cpp
    RSX/Overlays/overlay_message_dialog.cpp
    RSX/Overlays/overlay_osk.cpp
    RSX/Overlays/overlay_osk_panel.cpp
    RSX/Overlays/overlay_perf_metrics.cpp
    RSX/Overlays/overlay_progress_bar.cpp
    RSX/Overlays/overlay_save_dialog.cpp
    RSX/Overlays/overlay_trophy_notification.cpp
    RSX/Overlays/overlay_user_list_dialog.cpp
    RSX/Overlays/overlay_utils.cpp
    RSX/Overlays/overlay_video.cpp
    RSX/Overlays/Shaders/shader_loading_dialog.cpp
    RSX/Overlays/Shaders/shader_loading_dialog_native.cpp
    RSX/Program/Assembler/FPToCFG.cpp
    RSX/Program/CgBinaryProgram.cpp
    RSX/Program/CgBinaryFragmentProgram.cpp
    RSX/Program/CgBinaryVertexProgram.cpp
    RSX/Program/FragmentProgramDecompiler.cpp
    RSX/Program/FragmentProgramRegister.cpp
    RSX/Program/GLSLCommon.cpp
    RSX/Program/ProgramStateCache.cpp
    RSX/Program/program_util.cpp
    RSX/Program/SPIRVCommon.cpp
    RSX/Program/VertexProgramDecompiler.cpp
    RSX/GSFrameBase.cpp
    RSX/RSXDisAsm.cpp
    RSX/RSXFIFO.cpp
    RSX/RSXOffload.cpp
    RSX/RSXTexture.cpp
    RSX/RSXThread.cpp
    RSX/RSXZCULL.cpp
    RSX/rsx_methods.cpp
    RSX/rsx_utils.cpp
    RSX/rsx_vertex_data.cpp
)

if(NOT ANDROID)
    target_sources(rpcs3_emu PRIVATE
        RSX/GL/GLCommonDecompiler.cpp
        RSX/GL/GLCompute.cpp
        RSX/GL/GLDMA.cpp
        RSX/GL/GLDraw.cpp
        RSX/GL/GLFragmentProgram.cpp
        RSX/GL/GLGSRender.cpp
        RSX/GL/GLHelpers.cpp
        RSX/GL/GLOverlays.cpp
        RSX/GL/GLPipelineCompiler.cpp
        RSX/GL/GLPresent.cpp
        RSX/GL/GLRenderTargets.cpp
        RSX/GL/GLResolveHelper.cpp
        RSX/GL/GLShaderInterpreter.cpp
        RSX/GL/GLTexture.cpp
        RSX/GL/GLTextureCache.cpp
        RSX/GL/glutils/blitter.cpp
        RSX/GL/glutils/buffer_object.cpp
        RSX/GL/glutils/capabilities.cpp
        RSX/GL/glutils/common.cpp
        RSX/GL/glutils/fbo.cpp
        RSX/GL/glutils/image.cpp
        RSX/GL/glutils/program.cpp
        RSX/GL/glutils/ring_buffer.cpp
        RSX/GL/glutils/sampler.cpp
        RSX/GL/GLVertexBuffers.cpp
        RSX/GL/GLVertexProgram.cpp
        RSX/GL/OpenGL.cpp
        RSX/GL/upscalers/fsr1/fsr_pass.cpp
    )
endif()

if(TARGET 3rdparty_vulkan)
    target_sources(rpcs3_emu PRIVATE
        RSX/VK/upscalers/fsr1/fsr_pass.cpp
        RSX/VK/vkutils/barriers.cpp
        RSX/VK/vkutils/buffer_object.cpp
        RSX/VK/vkutils/chip_class.cpp
        RSX/VK/vkutils/commands.cpp
        RSX/VK/vkutils/ex.cpp
        RSX/VK/vkutils/data_heap.cpp
        RSX/VK/vkutils/descriptors.cpp
        RSX/VK/vkutils/image.cpp
        RSX/VK/vkutils/image_helpers.cpp
        RSX/VK/vkutils/instance.cpp
        RSX/VK/vkutils/scratch.cpp
        RSX/VK/vkutils/sync.cpp
        RSX/VK/vkutils/swapchain.cpp
        RSX/VK/vkutils/memory.cpp
        RSX/VK/vkutils/device.cpp
        RSX/VK/vkutils/sampler.cpp
        RSX/VK/vkutils/shared.cpp
        RSX/VK/vkutils/unique_resource.cpp
        RSX/VK/VKAsyncScheduler.cpp
        RSX/VK/VKCommandStream.cpp
        RSX/VK/VKCommonDecompiler.cpp
        RSX/VK/VKCommonPipelineLayout.cpp
        RSX/VK/VKCompute.cpp
        RSX/VK/VKDataHeapManager.cpp
        RSX/VK/VKDMA.cpp
        RSX/VK/VKDraw.cpp
        RSX/VK/VKFormats.cpp
        RSX/VK/VKFragmentProgram.cpp
        RSX/VK/VKFramebuffer.cpp
        RSX/VK/VKGSRender.cpp
        RSX/VK/VKHelpers.cpp
        RSX/VK/VKMemAlloc.cpp
        RSX/VK/VKOverlays.cpp
        RSX/VK/VKPipelineCompiler.cpp
        RSX/VK/VKPresent.cpp
        RSX/VK/VKProgramPipeline.cpp
        RSX/VK/VKQueryPool.cpp
        RSX/VK/VKRenderPass.cpp
        RSX/VK/VKRenderTargets.cpp
        RSX/VK/VKResolveHelper.cpp
        RSX/VK/VKResourceManager.cpp
        RSX/VK/VKShaderInterpreter.cpp
        RSX/VK/VKTexture.cpp
        RSX/VK/VKVertexBuffers.cpp
        RSX/VK/VKVertexProgram.cpp
        RSX/VK/VKTextureCache.cpp
        RSX/VK/VulkanAPI.cpp
    )
endif()

find_package(Threads REQUIRED)

target_link_libraries(rpcs3_emu
    PUBLIC
        3rdparty::llvm
        3rdparty::asmjit
        3rdparty::ffmpeg
        3rdparty::sdl3
        3rdparty::opengl
        3rdparty::stblib
        3rdparty::vulkan
        3rdparty::glew
        3rdparty::libusb
        3rdparty::wolfssl
        3rdparty::openal
        3rdparty::cubeb
        3rdparty::soundtouch
        3rdparty::miniupnpc
        3rdparty::libevdev
        3rdparty::flatbuffers
        3rdparty::pugixml
        3rdparty::vulkanmemoryallocator
        Threads::Threads
    PRIVATE
        3rdparty::glslang
        3rdparty::libpng
        3rdparty::rtmidi
        3rdparty::yaml-cpp
        3rdparty::zlib
        3rdparty::zstd
)

if(APPLE)
    check_function_exists(clock_gettime HAVE_CLOCK_GETTIME)
    if (HAVE_CLOCK_GETTIME)
        target_compile_definitions(rpcs3_emu PUBLIC -DHAVE_CLOCK_GETTIME)
    endif()
endif()


if(USE_PRECOMPILED_HEADERS)
    target_precompile_headers(rpcs3_emu PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../stdafx.h")
endif()

if(WAYLAND_FOUND)
    target_link_libraries(rpcs3_emu PRIVATE OpenGL::EGL)
endif()
