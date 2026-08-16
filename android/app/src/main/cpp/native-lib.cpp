#include "stdafx.h"

#include "Crypto/unpkg.h"
#include "Crypto/unself.h"
#include "Emu/Audio/Cubeb/CubebBackend.h"
#include "Emu/Audio/Null/NullAudioBackend.h"
#include "Emu/Cell/Modules/cellMsgDialog.h"
#include "Emu/Cell/Modules/cellSysutil.h"
#include "Emu/Cell/PPUAnalyser.h"
#include "Emu/Cell/SPURecompiler.h"
#include "Emu/Cell/lv2/sys_sync.h"
#include "Emu/IdManager.h"
#include "Emu/Io/KeyboardHandler.h"
#include "Emu/Io/Null/NullKeyboardHandler.h"
#include "Emu/Io/Null/NullMouseHandler.h"
#include "Emu/Io/Null/null_camera_handler.h"
#include "Emu/Io/Null/null_music_handler.h"
#include "Emu/Io/pad_config_types.h"
#include "Emu/RSX/Null/NullGSRender.h"
#include "Emu/RSX/Overlays/overlay_manager.h"
#include "Emu/RSX/Overlays/overlay_save_dialog.h"
#include "Emu/RSX/RSXThread.h"
#include "Emu/RSX/VK/VKGSRender.h"
#include "Emu/RSX/VK/vkutils/instance.h"
#include "Emu/localized_string_id.h"
#include "Emu/system_config.h"
#include "Emu/system_config_types.h"
#include "Emu/system_progress.hpp"
#include "Utilities/bin_patch.h"
#include "Loader/ISO.h"
#include <climits>
#include "Emu/system_utils.hpp"
#include "Emu/VFS.h"
#include "Emu/vfs_config.h"
#include "Input/ds3_pad_handler.h"
#include "Input/ds4_pad_handler.h"
#include "Input/dualsense_pad_handler.h"
#include "Input/hid_pad_handler.h"
#include "Input/pad_thread.h"
#include "Input/virtual_pad_handler.h"
#include "Loader/PSF.h"
#include "Loader/PUP.h"
#include "Loader/TAR.h"
#include "Utilities/File.h"
#include "Utilities/JIT.h"
#include "Utilities/StrFmt.h"
#include "Utilities/StrUtil.h"
#include "Utilities/Thread.h"
#include "block_dev.hpp"
#include "hidapi_libusb.h"
#include "iso.hpp"
#include "libusb.h"
#include "rpcs3_version.h"
#include "util/asm.hpp"
#include "util/console.h"
#include "util/fixed_typemap.hpp"
#include "util/logs.hpp"
#include "util/serialization.hpp"
#include "util/video_source.h"
#include "util/sysinfo.hpp"
#include <Emu/Cell/Modules/cellSaveData.h>
#include <Emu/Cell/Modules/sceNpTrophy.h>
#include <Emu/Io/pad_config.h>
#include <Emu/RSX/GSFrameBase.h>
#include <Emu/System.h>

#include <algorithm>
#include <set>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <functional>
#include <iterator>
#include <map>
#include <mutex>
#include <memory>
#include <jni.h>
#include <optional>
#include <span>
#include <string>
#include <sys/resource.h>
#include <thread>
#include <pthread.h>
#include <vector>

struct AtExit {
  std::function<void()> cb;
  ~AtExit() { cb(); }
};

static bool g_initialized;
static std::atomic<ANativeWindow *> g_native_window;
static std::atomic<float> g_hud_fps{0.f};
static std::atomic<float> g_hud_frametime{0.f};

extern std::string g_android_executable_dir;
extern std::string g_android_config_dir;
extern std::string g_android_cache_dir;

static std::mutex g_virtual_pad_mutex;
static std::shared_ptr<Pad> g_virtual_pad;

std::string g_input_config_override;
cfg_input_configurations g_cfg_input_configs;

LOG_CHANNEL(rpcs3_android, "ANDROID");

struct LogListener : logs::listener {
  LogListener() { logs::listener::add(this); }

  void log(u64 stamp, const logs::message &msg, std::string_view prefix,
           std::string_view text) override {
    int prio = 0;
    switch (static_cast<logs::level>(msg)) {
    case logs::level::always:
      prio = ANDROID_LOG_INFO;
      break;
    case logs::level::fatal:
      prio = ANDROID_LOG_FATAL;
      break;
    case logs::level::error:
      prio = ANDROID_LOG_ERROR;
      break;
    case logs::level::todo:
      prio = ANDROID_LOG_WARN;
      break;
    case logs::level::success:
      prio = ANDROID_LOG_INFO;
      break;
    case logs::level::warning:
      prio = ANDROID_LOG_WARN;
      break;
    case logs::level::notice:
      prio = ANDROID_LOG_DEBUG;
      break;
    case logs::level::trace:
      prio = ANDROID_LOG_VERBOSE;
      break;
    }

    __android_log_print(prio, "RPCS3", "%.*s", static_cast<int>(text.size()),
                        text.data());
  }
} static g_androidLogListener;

struct GraphicsFrame : GSFrameBase {
  mutable ANativeWindow *activeNativeWindow = nullptr;
  mutable int width = 0;
  mutable int height = 0;

  std::chrono::steady_clock::time_point fpsWindowStart{};
  u32 fpsFrames = 0;

  ~GraphicsFrame() {
    g_hud_fps.store(0.f);
    g_hud_frametime.store(0.f);

    if (activeNativeWindow != nullptr) {
      ANativeWindow_release(activeNativeWindow);
    }
  }

  ANativeWindow *getNativeWindow() const {
    ANativeWindow *result;
    while ((result = g_native_window.load()) == nullptr) [[unlikely]] {
      if (Emu.IsStopped()) {
        return activeNativeWindow;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (result != activeNativeWindow) [[unlikely]] {
      ANativeWindow_acquire(result);

      if (activeNativeWindow != nullptr) {
        ANativeWindow_release(activeNativeWindow);
      }

      activeNativeWindow = result;

      width = ANativeWindow_getWidth(result);
      height = ANativeWindow_getHeight(result);
    }

    return result;
  }

  void close() override {}
  void reset() override {}
  bool shown() override { return true; }
  void hide() override {}
  void show() override {}
  void toggle_fullscreen() override {}

  void delete_context(draw_context_t ctx) override {}
  draw_context_t make_context() override { return nullptr; }
  void set_current(draw_context_t ctx) override {}
  void flip(draw_context_t ctx, bool skip_frame = false) override {
    if (skip_frame) {
      return;
    }

    const auto now = std::chrono::steady_clock::now();

    if (fpsWindowStart.time_since_epoch().count() == 0) {
      fpsWindowStart = now;
      return;
    }

    fpsFrames++;

    const double elapsed =
        std::chrono::duration<double>(now - fpsWindowStart).count();

    if (elapsed >= 0.5) {
      g_hud_fps.store(static_cast<float>(fpsFrames / elapsed));
      g_hud_frametime.store(static_cast<float>(elapsed * 1000.0 / fpsFrames));
      fpsFrames = 0;
      fpsWindowStart = now;
    }
  }
  int client_width() override { return width; }
  int client_height() override { return height; }
  f64 client_display_rate() override { return 30.f; }
  bool has_alpha() override {
    return ANativeWindow_getFormat(getNativeWindow()) ==
           WINDOW_FORMAT_RGBA_8888;
  }

  display_handle_t handle() const override { return getNativeWindow(); }

  bool can_consume_frame() const override { return false; }

  void present_frame(std::vector<u8> &&data, u32 pitch, u32 width, u32 height,
                     bool is_bgra) const override {}

  void take_screenshot(std::vector<u8> &&sshot_data, u32 sshot_width,
                       u32 sshot_height, bool is_bgra) override {}

  void update_title(double fps = 0.0) override {}
};

void jit_announce(uptr, usz, std::string_view);

[[noreturn]] void report_fatal_error(std::string_view _text,
                                     bool is_html = false,
                                     bool include_help_text = true) {
  std::string buf;

  buf = std::string(_text);

  // Check if thread id is in string
  if (_text.find("\nThread id = "sv) == umax && !thread_ctrl::is_main()) {
    // Append thread id if it isn't already, except on main thread
    fmt::append(buf, "\n\nThread id = %u.", thread_ctrl::get_tid());
  }

  if (!g_tls_serialize_name.empty()) {
    fmt::append(buf, "\nSerialized Object: %s", g_tls_serialize_name);
  }

  const system_state state = Emu.GetStatus(false);

  if (state == system_state::stopped) {
    fmt::append(buf, "\nEmulation is stopped");
  } else {
    const std::string &name = Emu.GetTitleAndTitleID();
    fmt::append(buf, "\nTitle: \"%s\" (emulation is %s)",
                name.empty() ? "N/A" : name.data(),
                state == system_state::stopping ? "stopping" : "running");
  }

  fmt::append(buf, "\nBuild: \"%s\"", rpcs3::get_verbose_version());
  fmt::append(buf, "\nDate: \"%s\"", std::chrono::system_clock::now());

  __android_log_write(ANDROID_LOG_FATAL, "RPCS3", buf.c_str());

  jit_announce(0, 0, "");
  utils::trap();
  std::abort();
  std::terminate();
}

void qt_events_aware_op(int repeat_duration_ms,
                        std::function<bool()> wrapped_op) {
  /// ?????
}

static std::string unwrap(JNIEnv *env, jstring string) {
  auto resultBuffer = env->GetStringUTFChars(string, nullptr);
  std::string result(resultBuffer);
  env->ReleaseStringUTFChars(string, resultBuffer);
  return result;
}
static jstring wrap(JNIEnv *env, const std::string &string) {
  return env->NewStringUTF(string.c_str());
}
static jstring wrap(JNIEnv *env, const char *string) {
  return env->NewStringUTF(string);
}

static std::string fix_dir_path(std::string string) {
  if (!string.empty() && !string.ends_with('/')) {
    string += '/';
  }

  return string;
}

enum class FileType {
  Unknown,
  Pup,
  Pkg,
  Edat,
  Rap,
  Iso,
};

static FileType getFileType(const fs::file &file) {
  file.seek(0);
  if (PUPHeader pupHeader; file.read(pupHeader)) {
    if (pupHeader.magic == "SCEUF\0\0\0"_u64) {
      return FileType::Pup;
    }
  }

  file.seek(0);
  if (PKGHeader pkgHeader; file.read(pkgHeader)) {
    if (pkgHeader.pkg_magic == std::bit_cast<le_t<u32>>("\x7FPKG"_u32)) {
      return FileType::Pkg;
    }
  }

  file.seek(0);
  if (NPD_HEADER npdHeader; file.read(npdHeader)) {
    if (npdHeader.magic == "NPD\0"_u32) {
      return FileType::Edat;
    }
  }

  if (file.size() == 16) {
    return FileType::Rap;
  }

  if (iso_fs::open(std::make_unique<file_view_block_dev>(file))) {
    return FileType::Iso;
  }

  return FileType::Unknown;
}

#define MAKE_STRING(id, x) [int(localized_string_id::id)] = {x, U##x}

static std::pair<std::string, std::u32string> g_strings[] = {
#include "localized_strings.inl"
};

enum GameFlags {
  kGameFlagLocked = 1 << 0,
  kGameFlagTrial = 1 << 1,
};

struct GameInfo {
  std::string path;
  std::string name;
  std::string iconPath;
  int flags = 0;
};

class Progress {
  JNIEnv *env;
  jlong progressId;
  jclass progressRepositoryClass;
  jmethodID onProgressEventMethodId;

public:
  Progress(JNIEnv *env, jlong progressId) : env(env), progressId(progressId) {
    progressRepositoryClass =
        ensure(env->FindClass("net/rpcs3/ProgressRepository"));
    onProgressEventMethodId = env->GetStaticMethodID(
        progressRepositoryClass, "onProgressEvent", "(JJJLjava/lang/String;)Z");
  }

  bool report(jlong value, jlong max, const std::string &message = {}) {
    return env->CallStaticBooleanMethod(
        progressRepositoryClass, onProgressEventMethodId, progressId, value,
        max, message.empty() ? nullptr : wrap(env, message));
  }

  void failure(const std::string &message = {}) { report(-1, 0, message); }

  void success(jlong value, const std::string &message = {}) {
    value = std::max<jlong>(value, 1);
    report(value, value, message);
  }

  jlong getProgressId() const { return progressId; }
};

static void sendFirmwareInstalled(JNIEnv *env, std::string version) {
  auto fwRepositoryClass =
      ensure(env->FindClass("net/rpcs3/FirmwareRepository"));
  auto methodId = ensure(env->GetStaticMethodID(
      fwRepositoryClass, "onFirmwareInstalled", "(Ljava/lang/String;)V"));

  env->CallStaticVoidMethod(fwRepositoryClass, methodId, wrap(env, version));
}

static void sendFirmwareCompiled(JNIEnv *env, std::string version) {
  auto fwRepositoryClass =
      ensure(env->FindClass("net/rpcs3/FirmwareRepository"));
  auto methodId = ensure(env->GetStaticMethodID(
      fwRepositoryClass, "onFirmwareCompiled", "(Ljava/lang/String;)V"));

  env->CallStaticVoidMethod(fwRepositoryClass, methodId, wrap(env, version));
}

static void sendGameInfo(JNIEnv *env, jlong progressId,
                         std::span<const GameInfo> infos) {
  auto gameRepositoryClass = ensure(env->FindClass("net/rpcs3/GameRepository"));
  auto addMethodId = ensure(env->GetStaticMethodID(
      gameRepositoryClass, "add", "([Lnet/rpcs3/GameInfo;J)V"));
  auto gameClass = ensure(env->FindClass("net/rpcs3/GameInfo"));

  jmethodID gameConstructor = ensure(env->GetMethodID(
      gameClass, "<init>",
      "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)V"));

  std::vector<jobject> objects;
  objects.reserve(infos.size());

  for (const auto &info : infos) {
    auto path = Emu.GetCallbacks().resolve_path(info.path);
    if (path.ends_with('/')) {
      path.resize(path.size() - 1);
    }

    objects.push_back(env->NewObject(
        gameClass, gameConstructor, wrap(env, path), wrap(env, info.name),
        wrap(env, Emu.GetCallbacks().resolve_path(info.iconPath)),
        jint(info.flags)));
  }

  auto result = env->NewObjectArray(objects.size(), gameClass, nullptr);

  for (std::size_t i = 0; i < objects.size(); ++i) {
    env->SetObjectArrayElement(result, i, objects[i]);
  }

  env->CallStaticVoidMethod(gameRepositoryClass, addMethodId, result,
                            progressId);
}

static void sendEmulationStopped(JNIEnv *env) {
  auto cls = env->FindClass("net/rpcs3/RPCS3");

  if (cls == nullptr) {
    env->ExceptionClear();
    return;
  }

  auto methodId = env->GetStaticMethodID(cls, "notifyEmulationStopped", "()V");

  if (methodId == nullptr) {
    env->ExceptionClear();
    return;
  }

  env->CallStaticVoidMethod(cls, methodId);
}

static void sendVshBootable(JNIEnv *env, jlong progressId) {
  auto dev_flash = g_cfg_vfs.get_dev_flash();

  sendGameInfo(
      env, progressId,
      {{GameInfo{
          .path = dev_flash + "/vsh/module/vsh.self",
          .name = "VSH",
          .iconPath = dev_flash + "vsh/resource/explore/icon/icon_home.png",
      }}});
}

static bool tryUnlockGame(const psf::registry &psf) {
  auto contentId = psf::get_string(psf, "CONTENT_ID");

  if (contentId.empty()) {
    return true;
  }

  const auto licenseDir = fmt::format(
      "%shome/%s/exdata/", rpcs3::utils::get_hdd0_dir(), Emu.GetUsr());

  const auto licenseFile = fmt::format("%s%s", licenseDir, contentId);
  if (std::filesystem::is_regular_file(licenseFile + ".rap")) {
    return true;
  }

  if (std::filesystem::is_regular_file(licenseFile + ".edat")) {
    return true;
  }

  return false;
}

static void collectGamePaths(std::vector<std::string> &paths,
                             const std::string &rootDir) {
  std::error_code ec;
  std::vector<std::filesystem::path> workList;
  workList.reserve(32);
  if (!std::filesystem::is_directory(rootDir)) {
    auto rootPath = std::filesystem::path(rootDir).parent_path();
    if (rootPath.filename() == "USRDIR") {
      rootPath = rootPath.parent_path();
    }
    if (rootPath.filename() == "PS3_GAME") {
      rootPath = rootPath.parent_path();
    }

    workList.push_back(rootPath);
  } else {
    workList.push_back(rootDir);
  }

  while (!workList.empty()) {
    auto dir = std::move(workList.back());
    workList.pop_back();

    for (auto entry : std::filesystem::directory_iterator(dir, ec)) {
      if (entry.is_directory()) {
        if (entry.path().filename() != "C00") {
          workList.push_back(entry.path());
        }

        continue;
      }

      if (entry.is_regular_file() && entry.path().filename() == "PARAM.SFO") {
        paths.push_back(entry.path().parent_path().string());
        continue;
      }
    }
  }
}

static std::string locateEbootPath(const std::string &root) {
  if (std::filesystem::is_regular_file(root)) {
    return root;
  }

  for (auto suffix : {
           "/EBOOT.BIN",
           "/USRDIR/EBOOT.BIN",
           "/USRDIR/ISO.BIN.EDAT",
           "/PS3_GAME/USRDIR/EBOOT.BIN",
       }) {
    std::string tryPath = root + suffix;

    if (std::filesystem::is_regular_file(tryPath)) {
      return tryPath;
    }
  }

  return {};
}

static std::string locateParamSfoPath(const std::string &root) {
  if (std::filesystem::is_regular_file(root)) {
    return root;
  }

  for (auto suffix : {
           "/PARAM.SFO",
           "/PS3_GAME/PARAM.SFO",
       }) {
    std::string tryPath = root + suffix;

    if (std::filesystem::is_regular_file(tryPath)) {
      return tryPath;
    }
  }

  return {};
}

static std::optional<GameInfo>
fetchGameInfo(const psf::registry &psf,
              std::filesystem::path psfRootPath = {}) {
  auto titleId = std::string(psf::get_string(psf, "TITLE_ID"));
  auto name = std::string(psf::get_string(psf, "TITLE"));
  auto bootable = psf::get_integer(psf, "BOOTABLE", 0);
  auto category = psf::get_string(psf, "CATEGORY");

  if (!bootable || titleId.empty()) {
    return {};
  }

  bool isDiscGame = category == "DG";

  std::string path;

  if (!isDiscGame) {
    path = rpcs3::utils::get_hdd0_dir() + "game/" + titleId + "/";
  } else {
    if (psfRootPath.empty()) {
      path = fs::get_config_dir() + "games/" + titleId + "/";
    } else {
      // Locate game root path
      if (psfRootPath.filename() == "USRDIR") {
        psfRootPath = psfRootPath.parent_path();
      }

      if (psfRootPath.filename() == "PS3_GAME") {
        psfRootPath = psfRootPath.parent_path();
      }

      path = psfRootPath;
      if (!path.ends_with('/')) {
        path += '/';
      }
    }
  }

  auto dataPath = isDiscGame ? path + "PS3_GAME/" : path;
  auto iconPath = dataPath + "ICON0.PNG";
  auto moviePath = dataPath + "ICON1.PAM";

  int flags = 0;

  if (!isDiscGame) {
    auto ebootPath = locateEbootPath(path);

    bool isLocked = false;

    if (!ebootPath.empty()) {
      if (fs::file eboot{ebootPath};
          eboot && eboot.size() >= 4 && eboot.read<u32>() == "SCE\0"_u32) {
        isLocked = !decrypt_self(eboot);
      }
    }

    if (isLocked) {
      flags |= kGameFlagLocked;
      rpcs3_android.warning("game %s is locked", path);
    }

    auto c00Path = path + "/C00";

    bool isTrial = std::filesystem::is_directory(c00Path);

    if (isTrial) {
      if (!tryUnlockGame(psf)) {
        flags |= kGameFlagTrial;
        rpcs3_android.warning("game %s is trial", path);
      } else {
        auto c00IconPath = c00Path + "/ICON0.PNG";
        if (std::filesystem::is_regular_file(c00IconPath)) {
          iconPath = c00IconPath;
        }

        auto c00SfoPath = c00Path + "/PARAM.SFO";

        if (std::filesystem::is_regular_file(c00IconPath)) {
          auto c00Sfo = psf::load_object(c00SfoPath);
          titleId = psf::get_string(c00Sfo, "TITLE_ID", titleId);
          name = psf::get_string(c00Sfo, "TITLE", name);
        }
      }
    }
  }

  return GameInfo{
      .path = std::move(path),
      .name = std::move(name),
      .iconPath = std::move(iconPath),
      .flags = flags,
  };
}

static void collectGameInfo(JNIEnv *env, jlong progressId,
                            std::vector<std::string> rootDirs) {
  std::vector<std::string> paths;
  for (auto &&rootDir : rootDirs) {
    collectGamePaths(paths, rootDir);

    rpcs3_android.notice("collectGameInfo: processed %s", rootDir);
  }

  rpcs3_android.notice("collectGameInfo: found %d paths", paths.size());

  Progress progress(env, progressId);
  progress.report(0, paths.size());

  std::vector<GameInfo> gameInfos;
  gameInfos.reserve(10);
  std::size_t processed = 0;

  auto submit = [&] {
    if (gameInfos.empty()) {
      return;
    }

    sendGameInfo(env, progressId, gameInfos);
    progress.report(processed, paths.size());
    gameInfos.clear();
  };

  for (auto &&path : paths) {
    processed++;

    if (!std::filesystem::is_regular_file(path + "/PARAM.SFO")) {
      continue;
    }

    const auto psf = psf::load_object(path + "/PARAM.SFO");

    rpcs3_android.notice("collectGameInfo: sfo at %s", path);

    if (auto gameInfo = fetchGameInfo(psf, path)) {
      gameInfos.push_back(std::move(*gameInfo));

      if (gameInfos.size() >= 10) {
        submit();
      }
    }
  }

  submit();

  progress.success(processed);
}

class MainThreadProcessor {
  std::mutex mutex;
  std::condition_variable cv;
  std::deque<std::pair<std::function<void(JNIEnv *)>, atomic_t<u32> *>> queue;
  std::atomic<std::thread::id> workerId{};

public:
  bool onWorkerThread() const {
    return workerId.load() == std::this_thread::get_id();
  }

  void push(std::function<void(JNIEnv *)> cb, atomic_t<u32> *wakeUp = nullptr) {
    std::lock_guard lock(mutex);
    queue.push_back({std::move(cb), wakeUp});
    cv.notify_one();
  }

  void push(std::function<void()> cb, atomic_t<u32> *wakeUp = nullptr) {
    push([cb = std::move(cb)](JNIEnv *) { cb(); }, wakeUp);
  }

  void process(JNIEnv *env) {
    workerId.store(std::this_thread::get_id());

    while (true) {
      std::function<void(JNIEnv *)> cb;
      atomic_t<u32> *wakeUp = nullptr;

      {
        std::unique_lock lock(mutex);
        if (queue.empty()) {
          cv.wait(lock);
          continue;
        }

        auto item = std::move(queue.front());
        queue.pop_front();

        cb = std::move(item.first);
        wakeUp = item.second;
      }

      cb(env);
      if (wakeUp) {
        *wakeUp = true;
        wakeUp->notify_all();
      }
    }
  }
} static g_mainThreadProcessor;

static void invokeAsync(std::function<void(JNIEnv *)> cb) {
  g_mainThreadProcessor.push(std::move(cb));
}

static void invokeSync(std::function<void(JNIEnv *)> cb) {
  atomic_t<u32> wakeup{false};
  g_mainThreadProcessor.push(std::move(cb), &wakeup);

  while (wakeup.load() == false) {
    wakeup.wait(false);
  }
}

struct ProgressMessageDialog : MsgDialogBase {
  jlong progressId;
  jlong value = 0;
  jlong max = 0;

  ProgressMessageDialog(jlong progressId) : progressId(progressId) {}

  void Create(const std::string &msg, const std::string &title) override {
    rpcs3_android.warning("ProgressMessageDialog::Create(%s, %s)", msg, title);
    max = 100;
    invokeSync([this, &msg](JNIEnv *env) {
      Progress progress(env, progressId);
      progress.report(0, 0, msg);
    });
  }

  jlong getValue() const {
    return value == max && max != 0 ? value - 1 : value;
  }

  void Close(bool success) override {
    rpcs3_android.warning("ProgressMessageDialog::Close(%s)", success);
    invokeSync([this, success](JNIEnv *env) {
      Progress progress(env, progressId);
      progress.report(0, 0);
    });

    //   Progress progress(env, progressId);
    //   if (success) {
    //     progress.success(0);
    //   } else {
    //     progress.failure();
    //   }
    // });
  }

  void SetMsg(const std::string &msg) override {
    rpcs3_android.warning("ProgressMessageDialog::SetMsg(%s)", msg);
    invokeSync([this, msg](JNIEnv *env) {
      Progress(env, progressId).report(getValue(), max, msg);
    });
  }

  void ProgressBarSetMsg(u32 progressBarIndex,
                         const std::string &msg) override {
    rpcs3_android.warning("ProgressMessageDialog::ProgressBarSetMsg(%d, %s)",
                          progressBarIndex, msg);
    if (progressBarIndex != 0) {
      report_fatal_error("Unexpected progress index in progress dialog");
    }

    invokeSync([this, msg](JNIEnv *env) {
      Progress(env, progressId).report(getValue(), max, msg);
    });
  }

  void ProgressBarReset(u32 progressBarIndex) override {
    rpcs3_android.warning("ProgressMessageDialog::ProgressBarReset(%d)",
                          progressBarIndex);

    if (progressBarIndex != 0) {
      report_fatal_error("Unexpected progress index in progress dialog");
    }

    value = 0;
    invokeSync(
        [this](JNIEnv *env) { Progress(env, progressId).report(value, max); });
  }

  void ProgressBarInc(u32 progressBarIndex, u32 delta) override {
    rpcs3_android.warning("ProgressMessageDialog::ProgressBarInc(%d, %d)",
                          progressBarIndex, delta);

    if (progressBarIndex != 0) {
      report_fatal_error("Unexpected progress index in progress dialog");
    }

    value += delta;

    invokeSync([this](JNIEnv *env) {
      Progress(env, progressId).report(getValue(), max);
    });
  }

  void ProgressBarSetValue(u32 progressBarIndex, u32 value) override {
    rpcs3_android.warning("ProgressMessageDialog::ProgressBarSetValue(%d, %d)",
                          progressBarIndex, value);

    if (progressBarIndex != 0) {
      report_fatal_error("Unexpected progress index in progress dialog");
    }

    this->value = value;

    invokeSync([this](JNIEnv *env) {
      Progress(env, progressId).report(getValue(), max);
    });
  }
  void ProgressBarSetLimit(u32 index, u32 limit) override {
    rpcs3_android.warning("ProgressMessageDialog::ProgressBarSetLimit(%d, %d)",
                          index, limit);

    if (index != 0) {
      report_fatal_error("Unexpected progress index in progress dialog");
    }

    max = limit;

    invokeSync([this](JNIEnv *env) {
      Progress(env, progressId).report(getValue(), max);
    });
  }
};

struct UiMessageDialog : MsgDialogBase {
  // FIXME: implement

  void Create(const std::string &msg, const std::string &title) override {}
  void Close(bool success) override {}
  void SetMsg(const std::string &msg) override {}
  void ProgressBarSetMsg(u32 progressBarIndex,
                         const std::string &msg) override {}
  void ProgressBarReset(u32 progressBarIndex) override {}
  void ProgressBarInc(u32 progressBarIndex, u32 delta) override {}
  void ProgressBarSetValue(u32 progressBarIndex, u32 value) override {}
  void ProgressBarSetLimit(u32 index, u32 limit) override {}
};

struct MessageDialog : MsgDialogBase {
  std::unique_ptr<MsgDialogBase> impl = nullptr;

  void Create(const std::string &msg, const std::string &title) override {
    auto progressId = s_pendingProgressId.load();

    rpcs3_android.warning("MessageDialog::Create(%s, %s): source %s, id %d",
                          msg, title, source, progressId);

    if (progressId != -1) {
      impl = std::make_unique<ProgressMessageDialog>(progressId);
    } else {
      impl = std::make_unique<UiMessageDialog>();
    }

    impl->type = type;
    impl->source = source;
    impl->Create(msg, title);
  }

  void Close(bool success) override { impl->Close(success); }

  void SetMsg(const std::string &msg) override { impl->SetMsg(msg); }

  void ProgressBarSetMsg(u32 progressBarIndex,
                         const std::string &msg) override {
    impl->ProgressBarSetMsg(progressBarIndex, msg);
  }

  void ProgressBarReset(u32 progressBarIndex) override {
    impl->ProgressBarReset(progressBarIndex);
  }

  void ProgressBarInc(u32 progressBarIndex, u32 delta) override {
    impl->ProgressBarInc(progressBarIndex, delta);
  }

  void ProgressBarSetValue(u32 progressBarIndex, u32 value) override {
    impl->ProgressBarSetValue(progressBarIndex, value);
  }

  void ProgressBarSetLimit(u32 index, u32 limit) override {
    impl->ProgressBarSetLimit(index, limit);
  }

  static void pushPendingProgressId(jlong id) {
    jlong value = -1;

    while (!s_pendingProgressId.compare_exchange_weak(value, id)) {
      s_pendingProgressId.wait(value);
      value = -1;
    }
  }

  static bool popPendingProgressId(jlong id) {
    return s_pendingProgressId.compare_exchange_strong(id, -1);
  }

private:
  static std::atomic<jlong> s_pendingProgressId;
};

struct OverlaySaveDialog : SaveDialogBase {
  s32 ShowSaveDataList(const std::string &base_dir,
                       std::vector<SaveDataEntry> &save_entries, s32 focused,
                       u32 op, vm::ptr<CellSaveDataListSet> listSet,
                       bool enable_overlay) override {
    rpcs3_android.notice("ShowSaveDataList(save_entries=%d, focused=%d, "
                         "op=0x%x, listSet=*0x%x, enable_overlay=%d)",
                         save_entries.size(), focused, op, listSet,
                         enable_overlay);

    bool use_end = sysutil_send_system_cmd(CELL_SYSUTIL_DRAWING_BEGIN, 0) >= 0;

    auto atExit = AtExit([&] {
      if (use_end) {
        sysutil_send_system_cmd(CELL_SYSUTIL_DRAWING_END, 0);
      }
    });

    if (!use_end) {
      rpcs3_android.error(
          "ShowSaveDataList(): Not able to notify DRAWING_BEGIN callback "
          "because one has already been sent!");
    }

    if (auto manager = g_fxo->try_get<rsx::overlays::display_manager>()) {
      rpcs3_android.notice("ShowSaveDataList: Showing native UI dialog");

      s32 result = manager->create<rsx::overlays::save_dialog>()->show(
          base_dir, save_entries, focused, op, listSet, enable_overlay);

      if (result != rsx::overlays::user_interface::selection_code::error) {
        rpcs3_android.notice(
            "ShowSaveDataList: Native UI dialog returned with selection %d",
            result);

        return result;
      }

      rpcs3_android.error("ShowSaveDataList: Native UI dialog returned error");
    }

    return -2;
  }
};

std::atomic<jlong> MessageDialog::s_pendingProgressId = -1;

struct CompilationWorkload {
  jlong progressId;
  std::string path;
};

extern bool ppu_load_exec(const ppu_exec_object &, bool virtual_load,
                          const std::string &, utils::serial * = nullptr);
extern void spu_load_exec(const spu_exec_object &);
extern void spu_load_rel_exec(const spu_rel_object &);
extern void ppu_precompile(std::vector<std::string> &dir_queue,
                           std::vector<ppu_module<lv2_obj> *> *loaded_modules,
                           bool is_fast_compilation);
extern bool ppu_initialize(const ppu_module<lv2_obj> &, bool check_only = false,
                           u64 file_size = 0);
extern void ppu_finalize(const ppu_module<lv2_obj> &);
extern bool ppu_load_rel_exec(const ppu_rel_object &);

class CompilationQueue {
  std::atomic<std::uint64_t> nextWorkTag{0};
  std::uint64_t lastProcessedTag = 0;
  std::mutex queueMutex;
  std::deque<CompilationWorkload> queue;
  std::timed_mutex emuMutex;
  std::atomic<bool> emuOwned{false};
  std::atomic<int> pendingBootRequests{0};

public:
  bool isOwningEmu() const { return emuOwned.load(); }

  std::unique_lock<std::timed_mutex> acquireEmu() {
    pendingBootRequests.fetch_add(1);

    if (emuOwned.load()) {
      rpcs3_android.warning("Boot requested, asking precompilation to abort");
      Emu.SetState(system_state::stopped);
    }

    std::unique_lock<std::timed_mutex> lock(emuMutex, std::defer_lock);
    const bool locked = lock.try_lock_for(std::chrono::seconds(60));

    pendingBootRequests.fetch_sub(1);

    if (!locked) {
      rpcs3_android.error("Boot timed out waiting for precompilation to abort");
      return {};
    }

    return lock;
  }

  void push(CompilationWorkload workload) {
    {
      std::lock_guard lock(queueMutex);
      queue.push_back(std::move(workload));
    }

    nextWorkTag.fetch_add(1);
  }

  void push(Progress &progress, std::string path) {
    progress.report(0, 0);

    push({
        .progressId = progress.getProgressId(),
        .path = std::move(path),
    });
  }

  void process(JNIEnv *env) {
    while (true) {
      auto nextWorkTagValue = nextWorkTag.load();

      if (nextWorkTagValue == lastProcessedTag) {
        nextWorkTag.wait(lastProcessedTag);
      }

      if (nextWorkTagValue == lastProcessedTag || queue.empty()) {
        continue;
      }

      CompilationWorkload workload;

      {
        std::lock_guard lock(queueMutex);

        if (queue.empty()) {
          continue;
        }

        workload = std::move(queue.front());
        queue.pop_front();
      }

      impl(env, std::move(workload));
      lastProcessedTag++;
    }
  }

private:
  void impl(JNIEnv *env, CompilationWorkload workload) {
    if (workload.path.empty()) {
      Progress(env, workload.progressId).success(0);
      return;
    }

    rpcs3_android.error("Creating cache initiated, state %d",
                        (int)Emu.GetStatus(false));

    std::unique_lock<std::timed_mutex> emuLock;

    while (true) {
      auto state = Emu.GetStatus(false);

      if (state == system_state::stopped && pendingBootRequests.load() == 0) {
        emuLock = std::unique_lock<std::timed_mutex>(emuMutex);

        state = Emu.GetStatus(false);

        if (state == system_state::stopped && pendingBootRequests.load() == 0) {
          break;
        }

        emuLock.unlock();
      }

      rpcs3_android.error("Creating cache wait, state %d", (int)state);
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    bool is_vsh = workload.path.ends_with("/vsh.self");

    emuOwned.store(true);
    AtExit releaseEmuOwnership{[this] { emuOwned.store(false); }};

    Emu.SetState(system_state::running);

    MessageDialog::pushPendingProgressId(workload.progressId);

    g_fxo->init<named_thread<progress_dialog_server>>();
    g_fxo->init<main_ppu_module<lv2_obj>>();
    g_fxo->init(false, nullptr);
    auto rootPath = std::filesystem::path(workload.path);

    if (is_vsh) {
      rootPath = g_cfg_vfs.get_dev_flash() + "sys/external/";
    } else {
      if (!std::filesystem::is_directory(rootPath)) {
        rootPath = rootPath.parent_path();
        if (rootPath.filename() == "USRDIR") {
          rootPath = rootPath.parent_path();
        }
      }
    }

    auto &_main = *ensure(g_fxo->try_get<main_ppu_module<lv2_obj>>());

    if (fs::is_file(workload.path)) {
      if (!is_vsh) {
        auto sfoPath = locateParamSfoPath(rootPath);

        if (!sfoPath.empty()) {
          const auto psf = psf::load_object(sfoPath);
          rpcs3_android.warning("title id is %s",
                                psf::get_string(psf, "TITLE_ID"));

          Emu.SetTitleID(std::string(psf::get_string(psf, "TITLE_ID")));
        } else {
          rpcs3_android.warning("param.sfo not found");
        }
      }

      // Compile binary first
      rpcs3_android.notice("Trying to load binary: %s", workload.path);

      fs::file src{workload.path};
      src = decrypt_self(src);

      const ppu_exec_object obj = src;

      if (obj == elf_error::ok && ppu_load_exec(obj, true, workload.path)) {
        _main.path = workload.path;
      } else {
        rpcs3_android.error("Failed to load binary '%s' (%s)", workload.path,
                            obj.get_error());
      }
    }

    std::vector<std::string> dir_queue;
    dir_queue.push_back(rootPath.string());

    for (auto entry : std::filesystem::recursive_directory_iterator(rootPath)) {
      if (entry.is_directory()) {
        dir_queue.push_back(entry.path().string());
      }
    }

    std::vector<ppu_module<lv2_obj> *> mod_list;
    const bool aborting = pendingBootRequests.load() != 0;

    if (aborting) {
      rpcs3_android.error("Skipping precompilation, boot is pending");
    } else {
      rpcs3_android.error("Going to analyze executable");
    }

    // FIXME: split states
    if (!aborting && !is_vsh) {
      if (_main.analyse(0, _main.elf_entry, _main.seg0_code_end,
                        _main.applied_patches, std::vector<u32>{})) {
        Emu.ConfigurePPUCache();
        Emu.SetTestMode();
        rpcs3_android.error("Going to precompile main PPU module");
        ppu_initialize(_main);
        mod_list.emplace_back(&_main);
      }
    }

    if (!aborting) {
      ppu_precompile(dir_queue, mod_list.empty() ? nullptr : &mod_list, false);
    }

    rpcs3_android.error("Finalization");
    g_fxo->reset();
    Emu.SetState(system_state::stopped);
    emuLock.unlock();

    MessageDialog::popPendingProgressId(workload.progressId);

    Progress(env, workload.progressId).success(0);
  }
} static g_compilationQueue;

struct NullVideoSource : video_source {
  void set_video_path(const std::string &) override {}
  void set_audio_path(const std::string &) override {}
  void set_active(bool) override {}
  bool get_active() const override { return false; }
  bool has_new() const override { return false; }
  void get_image(std::vector<u8> &data, int &w, int &h, int &ch,
                 int &bpp) override {
    data.clear();
    w = 0;
    h = 0;
    ch = 0;
    bpp = 0;
  }
};

static std::string photoPath(std::string_view title) {
  std::string_view extension = ".png";
  if (const auto extension_start = title.find_last_of('.');
      extension_start != umax) {
    extension = title.substr(extension_start);
    title = title.substr(0, extension_start);
  }

  const std::time_t stamp =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::tm now{};
  ::localtime_r(&stamp, &now);

  const int year = now.tm_year + 1900;
  const int month = now.tm_mon + 1;
  const int day = now.tm_mday;

  const std::string path = vfs::get(fmt::format(
      "/dev_hdd0/photo/%04d/%02d/%02d/%s %02d-%02d-%04d %02d-%02d-%02d", year,
      month, day, vfs::escape(title, true), day, month, year, now.tm_hour,
      now.tm_min, now.tm_sec));

  std::string suffix = std::string(extension);
  u32 counter = 0;
  while (!Emu.IsStopped() && fs::is_file(path + suffix)) {
    suffix = fmt::format(" %d%s", ++counter, extension);
  }

  return path + suffix;
}

static void setupRenderers() {
  std::set<video_renderer> supported{video_renderer::null};
  std::string adapter;

  {
    vk::instance device_enum_context;

    if (device_enum_context.create("RPCS3", true)) {
      device_enum_context.bind();

      if (const auto &gpus = device_enum_context.enumerate_devices();
          !gpus.empty()) {
        adapter = gpus.front().get_name();
      }
    }

    device_enum_context.destroy();
  }

  if (!adapter.empty()) {
    rpcs3_android.notice("Default renderer: Vulkan, default GPU: '%s'",
                         adapter);
    supported.insert(video_renderer::vulkan);
    Emu.SetDefaultRenderer(video_renderer::vulkan);
    Emu.SetDefaultGraphicsAdapter(adapter);
  } else {
    rpcs3_android.error("No vulkan capable device found, rendering disabled");
  }

  Emu.SetSupportedRenderers(std::move(supported));
}

static void settings_save_async(std::string data, std::string titleId);

static void setupCallbacks() {
  Emu.SetCallbacks({
      .call_from_main_thread =
          [](std::function<void()> cb, atomic_t<u32> *wake_up) {
            if (g_mainThreadProcessor.onWorkerThread()) {
              cb();
              if (wake_up) {
                *wake_up = true;
                wake_up->notify_all();
              }
              return;
            }

            g_mainThreadProcessor.push(std::move(cb), wake_up);
          },
      .on_run = [](auto...) {},
      .on_pause = [](auto...) {},
      .on_resume = [](auto...) {},
      .on_stop =
          [](auto...) {
            invokeAsync([](JNIEnv *env) { sendEmulationStopped(env); });
          },
      .on_ready = [](auto...) {},
      .on_missing_fw = [](auto...) {},
      .on_emulation_stop_no_response = [](auto...) {},
      .on_save_state_progress = [](auto...) {},
      .enable_disc_eject = [](auto...) {},
      .enable_disc_insert = [](auto...) {},
      .try_to_quit =
          [](bool force_quit, std::function<void()> on_exit) {
            if (!force_quit) {
              return false;
            }

            if (on_exit) {
              on_exit();
            }

            return true;
          },
      .handle_taskbar_progress = [](auto...) {},
      .init_kb_handler =
          [](auto...) {
            ensure(g_fxo->init<KeyboardHandlerBase, NullKeyboardHandler>(
                Emu.DeserialManager()));
          },
      .init_mouse_handler =
          [](auto...) {
            ensure(g_fxo->init<MouseHandlerBase, NullMouseHandler>(
                Emu.DeserialManager()));
          },
      .init_pad_handler =
          [](auto...) {
            ensure(g_fxo->init<named_thread<pad_thread>>(nullptr, nullptr, ""));
          },
      .update_emu_settings = [](auto...) {},
      .save_emu_settings =
          [](auto...) {
            settings_save_async(g_cfg.to_string(), Emu.GetTitleID());
          },
      .close_gs_frame = [](auto...) {},
      .get_gs_frame = [] { return std::make_unique<GraphicsFrame>(); },
      .get_camera_handler =
          [](auto...) { return std::make_shared<null_camera_handler>(); },
      .get_music_handler =
          [](auto...) { return std::make_shared<null_music_handler>(); },
      .init_gs_render =
          [](utils::serial *ar) {
            switch (g_cfg.video.renderer.get()) {
            case video_renderer::null:
              g_fxo->init<rsx::thread, named_thread<NullGSRender>>(ar);
              break;
            case video_renderer::vulkan:
              g_fxo->init<rsx::thread, named_thread<VKGSRender>>(ar);
              break;

            default:
              break;
            }
          },
      .get_audio =
          [](auto...) {
            std::shared_ptr<AudioBackend> result;

            switch (g_cfg.audio.renderer.get()) {
            case audio_renderer::null:
              result = std::make_shared<NullAudioBackend>();
              break;

            case audio_renderer::cubeb:
            default:
              result = std::make_shared<CubebBackend>();
              break;
            }

            if (!result->Initialized()) {
              rpcs3_android.error(
                  "Audio renderer %s could not be initialized, using a Null "
                  "renderer instead. Make sure that no other application is "
                  "running that might block audio access (e.g. Netflix).",
                  result->GetName());
              result = std::make_shared<NullAudioBackend>();
            }
            return result;
          },
      .get_audio_enumerator = [](auto...) { return nullptr; },
      .get_msg_dialog = [] { return std::make_shared<MessageDialog>(); },
      .get_osk_dialog = [](auto...) { return nullptr; },
      .get_save_dialog =
          [](auto...) { return std::make_unique<OverlaySaveDialog>(); },
      .get_sendmessage_dialog = [](auto...) { return nullptr; },
      .get_recvmessage_dialog = [](auto...) { return nullptr; },
      .get_trophy_notification_dialog = [](auto...) { return nullptr; },
      .get_localized_string = [](localized_string_id id,
                                 const char *arg) -> std::string {
        if (int(id) < 0 || usz(id) >= std::size(g_strings)) {
          return "";
        }

        std::string result = g_strings[int(id)].first;

        if (const auto pos = result.find("%0"); pos != std::string::npos) {
          result.replace(pos, 2, arg ? arg : "");
        }

        return result;
      },
      .get_localized_u32string = [](localized_string_id id,
                                    const char *arg) -> std::u32string {
        if (int(id) < 0 || usz(id) >= std::size(g_strings)) {
          return U"";
        }

        std::u32string result = g_strings[int(id)].second;

        if (const auto pos = result.find(U"%0"); pos != std::u32string::npos) {
          const std::string replacement = arg ? arg : "";
          result.replace(pos, 2,
                         std::u32string(replacement.begin(), replacement.end()));
        }

        return result;
      },
      .get_localized_setting = [](const cfg::_base *node,
                                  u32 index) -> std::string {
        if (node == nullptr) {
          return {};
        }

        const auto list = node->to_list();

        if (index < list.size()) {
          return list[index];
        }

        return {};
      },
      .get_photo_path = [](std::string_view title) { return photoPath(title); },
      .play_sound = [](auto...) {},
      .get_image_info = [](auto...) { return false; },
      .get_scaled_image = [](auto...) { return false; },
      .resolve_path =
          [](std::string_view arg) {
            std::error_code ec;
            auto result =
                std::filesystem::weakly_canonical(
                    std::filesystem::path(fmt::replace_all(arg, "\\", "/")), ec)
                    .string();
            return ec ? std::string(arg) : result;
          },
      .get_font_dirs =
          [](auto...) {
            return std::vector<std::string>{
                "/system/fonts/", "/product/fonts/", "/system_ext/fonts/"};
          },
      .on_install_pkgs =
          [](const std::vector<std::string> &pkgs) {
            for (const std::string &pkg : pkgs) {
              if (!rpcs3::utils::install_pkg(pkg)) {
                rpcs3_android.error("cd install pkgs: failed to install %s",
                                    pkg);
                return false;
              }
            }
            return true;
          },
      .add_breakpoint = [](auto...) {},
      .display_sleep_control_supported = [](auto...) { return false; },
      .enable_display_sleep = [](auto...) {},
      .check_microphone_permissions = [](auto...) {},
      .make_video_source =
          [] { return std::make_unique<NullVideoSource>(); },
      .enable_gamemode = [](auto...) {},
      .get_database_config = [](auto...) { return std::string(); },
  });
}

static bool initVirtualPad(const std::shared_ptr<Pad> &pad) {
  u32 pclass_profile = 0;
  pad->Init(CELL_PAD_STATUS_CONNECTED,
            CELL_PAD_CAPABILITY_PS3_CONFORMITY |
                CELL_PAD_CAPABILITY_PRESS_MODE |
                CELL_PAD_CAPABILITY_HP_ANALOG_STICK |
                CELL_PAD_CAPABILITY_ACTUATOR //| CELL_PAD_CAPABILITY_SENSOR_MODE
            ,
            CELL_PAD_DEV_TYPE_STANDARD, CELL_PAD_PCLASS_TYPE_STANDARD,
            pclass_profile, 0, 0, 50);

  pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, std::vector<std::set<u32>>{},
                              CELL_PAD_CTRL_UP);
  pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, std::vector<std::set<u32>>{},
                              CELL_PAD_CTRL_DOWN);
  pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, std::vector<std::set<u32>>{},
                              CELL_PAD_CTRL_LEFT);
  pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, std::vector<std::set<u32>>{},
                              CELL_PAD_CTRL_RIGHT);
  pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, std::vector<std::set<u32>>{},
                              CELL_PAD_CTRL_CROSS);
  pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, std::vector<std::set<u32>>{},
                              CELL_PAD_CTRL_SQUARE);
  pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, std::vector<std::set<u32>>{},
                              CELL_PAD_CTRL_CIRCLE);
  pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, std::vector<std::set<u32>>{},
                              CELL_PAD_CTRL_TRIANGLE);
  pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, std::vector<std::set<u32>>{},
                              CELL_PAD_CTRL_L1);
  pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, std::vector<std::set<u32>>{},
                              CELL_PAD_CTRL_L2);
  pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, std::vector<std::set<u32>>{},
                              CELL_PAD_CTRL_L3);
  pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, std::vector<std::set<u32>>{},
                              CELL_PAD_CTRL_R1);
  pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, std::vector<std::set<u32>>{},
                              CELL_PAD_CTRL_R2);
  pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, std::vector<std::set<u32>>{},
                              CELL_PAD_CTRL_R3);
  pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, std::vector<std::set<u32>>{},
                              CELL_PAD_CTRL_START);
  pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, std::vector<std::set<u32>>{},
                              CELL_PAD_CTRL_SELECT);
  pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, std::vector<std::set<u32>>{},
                              CELL_PAD_CTRL_PS);

  pad->m_sticks[0] = AnalogStick(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X, {}, {});
  pad->m_sticks[1] = AnalogStick(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y, {}, {});
  pad->m_sticks[2] = AnalogStick(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X, {}, {});
  pad->m_sticks[3] = AnalogStick(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y, {}, {});

  pad->m_sensors[0] =
      AnalogSensor(CELL_PAD_BTN_OFFSET_SENSOR_X, 0, 0, 0, DEFAULT_MOTION_X);
  pad->m_sensors[1] =
      AnalogSensor(CELL_PAD_BTN_OFFSET_SENSOR_Y, 0, 0, 0, DEFAULT_MOTION_Y);
  pad->m_sensors[2] =
      AnalogSensor(CELL_PAD_BTN_OFFSET_SENSOR_Z, 0, 0, 0, DEFAULT_MOTION_Z);
  pad->m_sensors[3] =
      AnalogSensor(CELL_PAD_BTN_OFFSET_SENSOR_G, 0, 0, 0, DEFAULT_MOTION_G);

  pad->m_vibrate_motors[0] = VibrateMotor(true);
  pad->m_vibrate_motors[1] = VibrateMotor(false);

  if (pad->m_player_id == 0) {
    std::lock_guard lock(g_virtual_pad_mutex);
    g_virtual_pad = pad;
  }
  return true;
}

extern "C" JNIEXPORT jboolean JNICALL Java_net_rpcs3_RPCS3_overlayPadData(
    JNIEnv *env, jobject, jint digital1, jint digital2, jint leftStickX,
    jint leftStickY, jint rightStickX, jint rightStickY) {

  auto pad = [] {
    std::shared_ptr<Pad> result;
    std::lock_guard lock(g_virtual_pad_mutex);
    result = g_virtual_pad;
    return result;
  }();

  if (pad == nullptr) {
    return false;
  }

  for (auto &btn : pad->m_buttons) {
    if (btn.m_offset == CELL_PAD_BTN_OFFSET_DIGITAL1) {
      btn.m_pressed = (digital1 & btn.m_outKeyCode) != 0;
    } else if (btn.m_offset == CELL_PAD_BTN_OFFSET_DIGITAL2) {
      btn.m_pressed = (digital2 & btn.m_outKeyCode) != 0;
    }

      btn.m_value = btn.m_pressed ? 255 : 0;
  }

  pad->m_sticks[0].m_value = leftStickX;
  pad->m_sticks[1].m_value = leftStickY;
  pad->m_sticks[2].m_value = rightStickX;
  pad->m_sticks[3].m_value = rightStickY;
  return true;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_net_rpcs3_RPCS3_initialize(JNIEnv *env, jobject, jstring rootDir) {
  auto rootDirStr = fix_dir_path(unwrap(env, rootDir));

  if (g_android_executable_dir != rootDirStr) {
    g_android_executable_dir = rootDirStr;
    g_android_config_dir = rootDirStr + "config/";
    g_android_cache_dir = rootDirStr + "cache/";

    std::filesystem::create_directories(g_android_config_dir);
    std::error_code ec;
    // std::filesystem::remove_all(g_android_cache_dir, ec);
    std::filesystem::create_directories(g_android_cache_dir);
  }

  if (g_initialized) {
    return true;
  }

  g_initialized = true;

  if (int r = libusb_set_option(nullptr, LIBUSB_OPTION_NO_DEVICE_DISCOVERY,
                                nullptr);
      r != 0) {
    rpcs3_android.warning(
        "libusb_set_option(LIBUSB_OPTION_NO_DEVICE_DISCOVERY) -> %d", r);
  }

  // Initialize thread pool finalizer // ???
  static_cast<void>(named_thread("", [](int) {}));

  static std::unique_ptr<logs::listener> log_file;
  {
    // Check free space
    fs::device_stat stats{};
    if (!fs::statfs(fs::get_cache_dir(), stats) ||
        stats.avail_free < 128 * 1024 * 1024) {
      std::fprintf(stderr, "Not enough free space for logs (%f KB)",
                   stats.avail_free / 1000000.);
    }

    // preserve old log file
    if (std::filesystem::exists(fs::get_log_dir() + "RPCS3.log")) {
      std::error_code ec;
      std::filesystem::remove(fs::get_log_dir() + "RPCS3.old.log", ec);
      std::filesystem::rename(fs::get_log_dir() + "RPCS3.log",
                              fs::get_log_dir() + "RPCS3.old.log", ec);
    }

    // Limit log size to ~25% of free space
    log_file = logs::make_file_listener(fs::get_log_dir() + "RPCS3.log",
                                        stats.avail_free / 4);
  }

  logs::stored_message ver{rpcs3_android.always()};
  ver.text = fmt::format("RPCS3 v%s", rpcs3::get_verbose_version());

  // Write System information
  logs::stored_message sys{rpcs3_android.always()};
  sys.text = utils::get_system_info();

  // Write OS version
  logs::stored_message os{rpcs3_android.always()};
  os.text = utils::get_OS_version_string();

  // Write current time
  logs::stored_message time{rpcs3_android.always()};
  time.text = fmt::format("Current Time: %s", std::chrono::system_clock::now());

  logs::set_init(
      {std::move(ver), std::move(sys), std::move(os), std::move(time)});

  auto set_rlim = [](int resource, std::uint64_t limit) {
    rlimit64 rlim{};
    if (getrlimit64(resource, &rlim) != 0) {
      rpcs3_android.error("failed to get rlimit for %d", resource);
      return;
    }

    rlim.rlim_cur = std::min<std::size_t>(rlim.rlim_max, limit);
    rpcs3_android.error("rlimit[%d] = %u (requested %u, max %u)", resource,
                        rlim.rlim_cur, limit, rlim.rlim_max);

    if (setrlimit64(resource, &rlim) != 0) {
      rpcs3_android.error("failed to set rlimit for %d", resource);
      return;
    }
  };

  set_rlim(RLIMIT_MEMLOCK, RLIM_INFINITY);
  set_rlim(RLIMIT_NOFILE, RLIM_INFINITY);
  set_rlim(RLIMIT_STACK, 128 * 1024 * 1024);
  set_rlim(RLIMIT_AS, RLIM_INFINITY);

  virtual_pad_handler::set_on_connect_cb(initVirtualPad);
  setupCallbacks();
  setupRenderers();
  Emu.SetHasGui(false);
  Emu.Init();

  g_cfg_input.player1.handler.set(pad_handler::virtual_pad);
  g_cfg_input.player1.device.from_string("Virtual");
  g_cfg_input.save("", g_cfg_input_configs.default_config);

  Emulator::SaveSettings(g_cfg.to_string(), Emu.GetTitleID());
  return true;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_net_rpcs3_RPCS3_processCompilationQueue(JNIEnv *env, jobject) {
  g_compilationQueue.process(env);
  return true;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_net_rpcs3_RPCS3_startMainThreadProcessor(JNIEnv *env, jobject) {
  g_mainThreadProcessor.process(env);
  return true;
}

extern "C" JNIEXPORT jboolean JNICALL Java_net_rpcs3_RPCS3_collectGameInfo(
    JNIEnv *env, jobject, jstring jrootDir, jlong progressId) {

  if (std::filesystem::is_regular_file(g_cfg_vfs.get_dev_flash() +
                                       "/vsh/module/vsh.self")) {
    sendVshBootable(env, progressId);
  }

  collectGameInfo(env, progressId, {unwrap(env, jrootDir)});
  return true;
}

extern "C" JNIEXPORT void JNICALL Java_net_rpcs3_RPCS3_shutdown(JNIEnv *env,
                                                                jobject) {
  Emu.Kill();
}

extern "C" JNIEXPORT jint JNICALL Java_net_rpcs3_RPCS3_boot(JNIEnv *env,
                                                            jobject,
                                                            jstring jpath) {
  auto emuLock = g_compilationQueue.acquireEmu();

  if (!emuLock.owns_lock()) {
    return static_cast<int>(game_boot_result::still_running);
  }

  Emu.SetForceBoot(true);
  auto path = unwrap(env, jpath);
  while (path.ends_with('/')) {
    path.pop_back();
  }

  return static_cast<int>(Emu.BootGame(path, "", false, cfg_mode::custom));
}

static constexpr int kEmulatorStateCompiling = 8;
static_assert(static_cast<int>(system_state::starting) < kEmulatorStateCompiling,
              "kEmulatorStateCompiling collides with a real system_state value, "
              "pick a value above the last system_state enumerator and append a "
              "matching entry to net.rpcs3.EmulatorState");

extern "C" JNIEXPORT jint JNICALL Java_net_rpcs3_RPCS3_getState(JNIEnv *env,
                                                                jobject) {
  if (g_compilationQueue.isOwningEmu()) {
    return kEmulatorStateCompiling;
  }

  return static_cast<int>(Emu.GetStatus(false));
}

extern "C" JNIEXPORT void JNICALL Java_net_rpcs3_RPCS3_kill(JNIEnv *env,
                                                            jobject) {
  Emu.Kill();
}

extern "C" JNIEXPORT void JNICALL Java_net_rpcs3_RPCS3_resume(JNIEnv *env,
                                                              jobject) {
    Emu.Resume();
}

extern "C" JNIEXPORT void JNICALL Java_net_rpcs3_RPCS3_pause(JNIEnv *env,
                                                             jobject) {
    Emu.Pause();
}

extern "C" JNIEXPORT void JNICALL Java_net_rpcs3_RPCS3_openHomeMenu(JNIEnv *env,
                                                                    jobject) {
  if (auto padThread = pad::get_pad_thread(true)) {
    padThread->open_home_menu();
  }
}

extern "C" JNIEXPORT jstring JNICALL
Java_net_rpcs3_RPCS3_getTitleId(JNIEnv *env, jobject) {
  return wrap(env, Emu.GetTitleID());
}

extern "C" JNIEXPORT jstring JNICALL
Java_net_rpcs3_RPCS3_perfMetrics(JNIEnv *env, jobject) {
  u32 rsxLoad = 0;

  if (auto renderer = rsx::get_current_renderer()) {
    rsxLoad = renderer->get_load();
  }

  return wrap(env, fmt::format(R"({"fps":%.2f,"frametime":%.3f,"rsxLoad":%u,)"
                               R"("renderer":"%s"})",
                               g_hud_fps.load(), g_hud_frametime.load(),
                               rsxLoad, g_cfg.video.renderer.to_string()));
}

extern "C" JNIEXPORT jboolean JNICALL Java_net_rpcs3_RPCS3_surfaceEvent(
    JNIEnv *env, jobject, jobject surface, jint event) {
  rpcs3_android.warning("surface event %p, %d", surface, event);

  if (event == 2) {
    auto prevWindow = g_native_window.exchange(nullptr);
    if (prevWindow != nullptr) {
      ANativeWindow_release(prevWindow);
    }

    if (auto padThread = pad::get_pad_thread(true)) {
      padThread->open_home_menu();
    }

    Emu.Pause();
  } else {
    auto newWindow = ANativeWindow_fromSurface(env, surface);

    if (newWindow == nullptr) {
      rpcs3_android.fatal("returned native window is null, surface %p",
                          surface);
      return false;
    }

    auto prevWindow = g_native_window.exchange(newWindow);

    if (newWindow != prevWindow) {
      ANativeWindow_acquire(newWindow);

      if (prevWindow != nullptr) {
        ANativeWindow_release(prevWindow);
      }
    }

    if (event == 0 && Emu.IsPaused()) {
      Emu.Resume();
    }
  }

  return true;
}

extern "C" JNIEXPORT jboolean JNICALL Java_net_rpcs3_RPCS3_usbDeviceEvent(
    JNIEnv *env, jobject, jint fd, jint vendorId, jint productId, jint event) {
  rpcs3_android.warning(
      "usb device event %d fd: %d, vendorId: %d, productId: %d", event, fd,
      vendorId, productId);

  {
    std::lock_guard lock(g_android_usb_devices_mutex);

    if (event == 0) {
      g_android_usb_devices.push_back({
          .fd = int(fd),
          .vendorId = u16(vendorId),
          .productId = u16(productId),
      });
    } else {
      auto filter = [fd](auto device) { return device.fd == fd; };
      if (auto it = std::ranges::find_if(g_android_usb_devices, filter);
          it != g_android_usb_devices.end()) {
        g_android_usb_devices.erase(it);
      }
    }
  }

  {
    auto selectedHandler = g_cfg_input.player1.handler.get();
    std::string selectedDevice;

    std::map<pad_handler, std::pair<std::unique_ptr<PadHandlerBase>,
                                    std::vector<std::string>>>
        handlerToDevices;

    auto collectDevices = [&]<typename T>(T handler) {
      handler->Init();

      std::vector<std::string> devices;
      for (const auto &device : handler->list_connected_devices()) {
        devices.push_back(device.name);
      }

      auto type = handler->m_type;

      handlerToDevices[type] = std::pair{
          std::move(handler),
          std::move(devices),
      };
    };

    collectDevices(std::make_unique<dualsense_pad_handler>());
    collectDevices(std::make_unique<ds4_pad_handler>());
    collectDevices(std::make_unique<ds3_pad_handler>());

    if (handlerToDevices[selectedHandler].second.empty()) {
      selectedHandler = pad_handler::null;
    }

    if (!handlerToDevices[pad_handler::dualsense].second.empty()) {
      selectedHandler = pad_handler::dualsense;
    } else if (!handlerToDevices[pad_handler::ds4].second.empty()) {
      selectedHandler = pad_handler::ds4;
    } else if (!handlerToDevices[pad_handler::ds3].second.empty()) {
      selectedHandler = pad_handler::ds3;
    }

    if (selectedHandler == pad_handler::null) {
      selectedHandler = pad_handler::virtual_pad;
    }

    if (selectedHandler != g_cfg_input.player1.handler.get()) {
      rpcs3_android.warning("install %s pad handler", selectedHandler);

      g_cfg_input.player1.handler.set(selectedHandler);

      if (selectedHandler == pad_handler::null) {
        g_cfg_input.player1.device.from_default();
      } else if (selectedHandler == pad_handler::virtual_pad) {
        g_cfg_input.player1.handler.set(pad_handler::virtual_pad);
        g_cfg_input.player1.device.from_string("Virtual");
      } else {
        g_cfg_input.player1.device.from_string(
            handlerToDevices[selectedHandler].second.front());
        handlerToDevices[selectedHandler].first->init_config(
            &g_cfg_input.player1.config);
        if (selectedHandler != pad_handler::virtual_pad) {
          std::lock_guard lock(g_virtual_pad_mutex);
          g_virtual_pad = nullptr;
        }
      }

      g_cfg_input.save("", g_cfg_input_configs.default_config);

      if (!Emu.IsStopped()) {
        pad::reset(Emu.GetTitleID());
      }
    }
  }

  return true;
}

static bool installPup(JNIEnv *env, fs::file &&pup_f, jlong progressId) {
  Progress progress(env, progressId);

  pup_object pup(std::move(pup_f));
  AtExit atExit{[&] { pup.file().release_handle(); }};

  if (static_cast<pup_error>(pup) == pup_error::hash_mismatch) {
    rpcs3_android.fatal("installFw: invalid PUP");
    progress.failure("Selected file is not firmware update file");
    return false;
  }

  if (static_cast<pup_error>(pup) != pup_error::ok) {
    rpcs3_android.fatal("installFw: invalid PUP");
    progress.failure("Firmware update file is broken");
    return false;
  }

  fs::file update_files_f = pup.get_file(0x300);

  const usz update_files_size = update_files_f ? update_files_f.size() : 0;

  if (!update_files_size) {
    rpcs3_android.fatal("installFw: invalid PUP");
    progress.failure("Firmware update file is broken");
    return false;
  }

  tar_object update_files(update_files_f);

  auto update_filenames = update_files.get_filenames();
  update_filenames.erase(std::remove_if(update_filenames.begin(),
                                        update_filenames.end(),
                                        [](const std::string &s) {
                                          return !s.starts_with("dev_flash_");
                                        }),
                         update_filenames.end());

  if (update_filenames.empty()) {
    rpcs3_android.fatal("installFw: invalid PUP");
    progress.failure("Firmware update file is broken");
    return false;
  }

  std::string version_string;

  if (fs::file version = pup.get_file(0x100)) {
    version_string = version.to_string();
  }

  if (const usz version_pos = version_string.find('\n');
      version_pos != std::string::npos) {
    version_string.erase(version_pos);
  }

  if (version_string.empty()) {
    rpcs3_android.fatal("installFw: invalid PUP");
    progress.failure("Firmware update file is broken");
    return false;
  }

  sendVshBootable(env, progressId);

  jlong processed = 0;
  for (const auto &update_filename : update_filenames) {
    auto update_file_stream = update_files.get_file(update_filename);

    if (update_file_stream->m_file_handler) {
      // Forcefully read all the data
      update_file_stream->m_file_handler->handle_file_op(
          *update_file_stream, 0, update_file_stream->get_size(umax), nullptr);
    }

    fs::file update_file = fs::make_stream(std::move(update_file_stream->data));

    SCEDecrypter self_dec(update_file);
    self_dec.LoadHeaders();
    self_dec.LoadMetadata(SCEPKG_ERK, SCEPKG_RIV);
    self_dec.DecryptData();

    auto dev_flash_tar_f = self_dec.MakeFile();

    if (dev_flash_tar_f.size() < 3) {
      rpcs3_android.error(
          "Firmware installation failed: Firmware could not be decompressed");

      progress.failure("Firmware update file could not be decompressed");
      return false;
    }

    tar_object dev_flash_tar(dev_flash_tar_f[2]);

    if (!dev_flash_tar.extract()) {

      rpcs3_android.error("Error while installing firmware: TAR contents are "
                          "invalid. (package=%s)",
                          update_filename);

      progress.failure(fmt::format("TAR contents are invalid (package=%s)",
                                   update_filename));
      return false;
    }

    if (!progress.report(processed++, update_filenames.size())) {
      // Installation was cancelled
      return false;
    }
  }

  sendFirmwareInstalled(env, utils::get_firmware_version());

  g_compilationQueue.push(progress,
                          g_cfg_vfs.get_dev_flash() + "/vsh/module/vsh.self");
  return true;
}

static bool installPkgs(JNIEnv *env,
                        std::vector<std::pair<std::string, fs::file>> &&files,
                        jlong progressId) {
  Progress progress(env, progressId);

  std::deque<package_reader> readers;
  std::deque<std::string> bootable_paths;
  std::vector<std::string> names;

  for (auto &[name, file] : files) {
    names.push_back(name);
    readers.emplace_back(name, std::move(file));
  }

  AtExit atExit{[&] {
    for (auto &reader : readers) {
      reader.file().release_handle();
    }
  }};

  if (readers.empty()) {
    progress.failure("No packages to install");
    return false;
  }

  for (std::size_t index = 0; index < readers.size(); index++) {
    if (!readers[index].is_valid()) {
      progress.failure("Corrupted package: " + names[index]);
      return false;
    }
  }

  package_install_result result = {};
  named_thread worker("PKG Installer", [&readers, &result, &bootable_paths] {
    result = package_reader::extract_data(readers, bootable_paths);
    return result.error == package_install_result::error_type::no_error;
  });

  for (auto &reader : readers) {
    if (auto gameInfo = fetchGameInfo(reader.get_psf())) {
      sendGameInfo(env, progressId, {{*gameInfo}});
    }
  }

  const jlong maxProgress = 10000;

  while (true) {
    std::uint64_t totalProgress = 0;
    for (auto &reader : readers) {
      if (result.error != package_install_result::error_type::no_error) {
        if (result.error == package_install_result::error_type::app_version) {
          progress.failure(
              "Update cannot be installed on the current version. It expects "
              "version " +
              result.version.expected + ", but version " +
              (result.version.installed.empty() ? std::string("none")
                                                : result.version.installed) +
              " is installed. Install the missing updates in order first.");
        } else {
          progress.failure("Installation failed");
        }

        for (package_reader &reader : readers) {
          reader.abort_extract();
        }
        return false;
      }

      totalProgress += reader.get_progress(maxProgress);
    }

    if (totalProgress == maxProgress * readers.size()) {
      break;
    }

    totalProgress /= readers.size();

    if (!progress.report(totalProgress, maxProgress)) {
      for (package_reader &reader : readers) {
        reader.abort_extract();
      }

      return false;
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));
  }

  if (!worker()) {
    progress.failure("Installation failed");
    return false;
  }

  auto paths = std::vector(bootable_paths.begin(), bootable_paths.end());
  collectGameInfo(env, -1, paths);

  if (paths.empty()) {
    progress.success(maxProgress);
    return true;
  }

  for (auto &path : paths) {
    g_compilationQueue.push(progress, std::move(path));
  }

  return true;
}

static bool installPkg(JNIEnv *env, fs::file &&file, jlong progressId) {
  std::vector<std::pair<std::string, fs::file>> files;
  files.emplace_back("package.pkg", std::move(file));
  return installPkgs(env, std::move(files), progressId);
}

static bool installEdat(JNIEnv *env, fs::file &&file, jlong progressId,
                        std::string rootPath = {}) {
  Progress progress(env, progressId);

  NPD_HEADER npdHeader;
  if (!file.read(npdHeader)) {
    progress.failure("Invalid EDAT file");
    return false;
  }

  if (!rootPath.empty()) {
    auto ebootPath = locateEbootPath(rootPath);
    auto sfoPath = locateParamSfoPath(rootPath);

    if (sfoPath.empty()) {
      progress.failure("Game is broken: PARAM.SFO not found");
      return false;
    }

    auto psf = psf::load_object(sfoPath);
    auto contentId = psf::get_string(psf, "CONTENT_ID");

    if (contentId != npdHeader.content_id) {
      progress.failure(fmt::format("File cannot be used for this game. EDAT "
                                   "content ID missmatch %s vs %s",
                                   contentId, npdHeader.content_id));
      return false;
    }
  }

  const auto licenseFile =
      fmt::format("%shome/%s/exdata/%s.edat", rpcs3::utils::get_hdd0_dir(),
                  Emu.GetUsr(), npdHeader.content_id);

  file.seek(0);

  std::vector<std::uint8_t> bytes(file.size());
  if (!file.read(bytes)) {
    progress.failure("Failed to read key");
    return false;
  }

  if (!fs::write_file(licenseFile, fs::open_mode::create + fs::open_mode::trunc,
                      bytes)) {
    progress.failure(fmt::format("Failed to write EDAT to %s", licenseFile));
    return false;
  }

  if (rootPath.empty()) {
    rootPath = rpcs3::utils::get_hdd0_dir() + "game";
  }

  collectGameInfo(env, progressId, {rootPath});
  return true;
}

static bool installRap(JNIEnv *env, fs::file &&file, jlong progressId,
                       const std::string &rootPath) {
  Progress progress(env, progressId);

  auto ebootPath = locateEbootPath(rootPath);

  std::vector<std::uint8_t> bytes;
  if (!file.read(bytes, 16)) {
    progress.failure("Failed to read key");
    return false;
  }

  SelfAdditionalInfo info;
  decrypt_self(fs::file(ebootPath), nullptr, &info);

  auto npd = [&]() -> NPD_HEADER * {
    for (auto &supplemental : info.supplemental_hdr) {
      if (supplemental.type == 3) {
        return &supplemental.PS3_npdrm_header.npd;
      }
    }

    return nullptr;
  }();

  if (npd == nullptr) {
    progress.failure("Failed to fetch NPDRM of SELF");
    return false;
  }

  const auto licenseFile =
      fmt::format("%shome/%s/exdata/%s.rap", rpcs3::utils::get_hdd0_dir(),
                  Emu.GetUsr(), npd->content_id);

  if (!fs::write_file(licenseFile, fs::open_mode::create + fs::open_mode::trunc,
                      bytes)) {
    progress.failure(fmt::format("Failed to write key to %s", licenseFile));
    return false;
  }

  if (!decrypt_self(fs::file(ebootPath))) {
    progress.failure("Provided key is invalid for selected game");
    fs::remove_file(licenseFile);
    return false;
  }

  collectGameInfo(env, -1, {rootPath});
  g_compilationQueue.push(progress, std::move(ebootPath));
  return true;
}

static bool installRapInExData(JNIEnv *env, fs::file &&file, jlong progressId,
                               const std::string &name) {
  Progress progress(env, progressId);

  const auto baseName = name.substr(name.find_last_of("/\\") + 1);
  const auto contentId = baseName.substr(0, baseName.find_last_of('.'));

  if (contentId.empty()) {
    progress.failure("RAP file name does not contain a content ID");
    return false;
  }

  file.seek(0);

  std::vector<std::uint8_t> bytes(file.size());
  if (!file.read(bytes)) {
    progress.failure("Failed to read key");
    return false;
  }

  if (bytes.size() < 0x10) {
    progress.failure("Not a RAP file");
    return false;
  }

  const auto exdataDir = fmt::format("%shome/%s/exdata/",
                                     rpcs3::utils::get_hdd0_dir(), Emu.GetUsr());

  if (!fs::create_path(exdataDir) && !fs::is_dir(exdataDir)) {
    progress.failure(fmt::format("Failed to create %s", exdataDir));
    return false;
  }

  const auto licenseFile = exdataDir + contentId + ".rap";

  fs::pending_file pending(licenseFile);

  if (!pending.file) {
    progress.failure(fmt::format("Failed to write key to %s", licenseFile));
    return false;
  }

  pending.file.write(bytes);

  if (!pending.commit()) {
    progress.failure(fmt::format("Failed to write key to %s", licenseFile));
    return false;
  }

  collectGameInfo(env, progressId, {rpcs3::utils::get_hdd0_dir() + "game"});
  return true;
}

static bool installIso(JNIEnv *env, fs::file &&file, jlong progressId) {
  auto optIso = iso_fs::open(std::make_unique<file_view_block_dev>(file));
  Progress progress(env, progressId);

  if (!optIso) {
    progress.failure("Failed to read ISO");
    return false;
  }

  auto iso = std::move(*optIso);
  auto sfo_file = iso.open("PS3_GAME/PARAM.SFO");

  if (!sfo_file) {
    progress.failure("Failed to locate PARAM.SFO in ISO");
    return false;
  }

  auto sfo = psf::load_object(sfo_file, "iso://PS3_GAME/PARAM.SFO");
  auto title_id = psf::get_string(sfo, "TITLE_ID");

  if (title_id.empty()) {
    progress.failure("Failed to fetch TITLE_ID from PARAM.SFO in ISO");
    return false;
  }

  if (auto gameInfo = fetchGameInfo(sfo)) {
    sendGameInfo(env, progressId, {{*gameInfo}});
  }

  std::filesystem::path destinationPath =
      fs::get_config_dir() + "games/" + std::string(title_id);
  std::size_t filesCount = 0;

  auto roots = [&] {
    std::vector<std::filesystem::path> result;
    std::vector<std::filesystem::path> workList;
    workList.push_back({});
    result.push_back({});

    while (!workList.empty()) {
      auto path = std::move(workList.back());
      workList.pop_back();

      for (auto entry : iso.open_dir(path)) {
        if (entry.name == "." || entry.name == "..") {
          continue;
        }
        if (entry.name == "PS3_UPDATE" && path.empty()) {
          continue;
        }

        if (entry.is_directory) {
          result.push_back(path / entry.name);
          workList.push_back(path / entry.name);
        } else {
          filesCount++;
        }
      }
    }

    return result;
  }();

  progress.report(0, filesCount);

  std::size_t processedFiles = 0;
  std::error_code ec;

  for (auto &root : roots) {
    auto rootDestPath = root.empty() ? destinationPath : destinationPath / root;

    std::filesystem::create_directories(rootDestPath, ec);
    if (ec) {
      progress.failure(fmt::format("Failed to create dir %s: %s",
                                   rootDestPath.string(), ec.message()));
      return false;
    }

    for (auto entry : iso.open_dir(root)) {
      if (entry.name == "." || entry.name == "..") {
        continue;
      }

      auto entryDestPath = rootDestPath / entry.name;

      if (entry.is_directory) {
        std::filesystem::create_directories(entryDestPath, ec);
        if (ec) {
          progress.failure(fmt::format("Failed to create dir %s: %s",
                                       entryDestPath.string(), ec.message()));
          return false;
        }

        continue;
      }
      if (!iso.extract(root / entry.name, entryDestPath.string())) {
        progress.failure(fmt::format("Failed to extract file: %s, dest %s",
                                     (root / entry.name).string(),
                                     entryDestPath.string()));
        return false;
      }

      progress.report(processedFiles++, filesCount);
    }
  }

  collectGameInfo(env, -1, {destinationPath});
  auto ebootPath = locateEbootPath(destinationPath);
  g_compilationQueue.push(progress, std::move(ebootPath));
  return true;
}

extern "C" JNIEXPORT jboolean JNICALL Java_net_rpcs3_RPCS3_installFw(
    JNIEnv *env, jobject, jint fd, jlong progressId) {
  return installPup(env, fs::file::from_native_handle(fd), progressId);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_net_rpcs3_RPCS3_isInstallableFile(JNIEnv *env, jobject, jint fd) {
  auto file = fs::file::from_native_handle(fd);
  AtExit atExit{[&] { file.release_handle(); }};

  auto type = getFileType(file);
  file.seek(0);
  return type != FileType::Unknown;
}

extern "C" JNIEXPORT jstring JNICALL
Java_net_rpcs3_RPCS3_getDirInstallPath(JNIEnv *env, jobject, jint fd) {
  auto file = fs::file::from_native_handle(fd);
  AtExit atExit{[&] { file.release_handle(); }};

  auto psf = psf::load_object(file, "");
  if (auto gameInfo = fetchGameInfo(psf)) {
    return wrap(env, gameInfo->path);
  }

  return nullptr;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_net_rpcs3_RPCS3_install(JNIEnv *env, jobject, jint fd, jlong progressId,
                             jstring jname) {
  auto file = fs::file::from_native_handle(fd);
  AtExit atExit{[&] { file.release_handle(); }};

  auto type = getFileType(file);
  file.seek(0);

  switch (type) {
  case FileType::Unknown:
    Progress(env, progressId).failure("Unsupported file type");
    return false;

  case FileType::Pup:
    return installPup(env, std::move(file), progressId);

  case FileType::Pkg:
    return installPkg(env, std::move(file), progressId);

  case FileType::Edat:
    return installEdat(env, std::move(file), progressId);

  case FileType::Iso:
    return installIso(env, std::move(file), progressId);

  case FileType::Rap:
    return installRapInExData(env, std::move(file), progressId,
                              jname != nullptr ? unwrap(env, jname)
                                               : std::string());
  }

  return true;
}

extern "C" JNIEXPORT jstring JNICALL
Java_net_rpcs3_RPCS3_gameDetails(JNIEnv *env, jobject, jstring jpath) {
  const std::string path = unwrap(env, jpath);

  std::string titleId;
  std::string title;
  std::string version;
  std::string category;

  const auto readFrom = [&](const psf::registry &psf) {
    titleId = std::string(psf::get_string(psf, "TITLE_ID", ""));
    title = std::string(psf::get_string(psf, "TITLE", ""));
    version = std::string(psf::get_string(psf, "APP_VER", ""));
    category = std::string(psf::get_string(psf, "CATEGORY", ""));
  };

  if (fs::is_dir(path)) {
    const std::string sfoPath = locateParamSfoPath(path);

    if (!sfoPath.empty()) {
      if (const fs::file sfo{sfoPath}) {
        readFrom(psf::load_object(sfo, sfoPath));
      }
    }
  } else if (fs::is_file(path)) {
    fs::file image{path};

    if (image) {
      if (auto optIso = iso_fs::open(std::make_unique<file_view_block_dev>(image))) {
        auto iso = std::move(*optIso);

        if (auto sfoFile = iso.open("PS3_GAME/PARAM.SFO")) {
          readFrom(psf::load_object(sfoFile, "iso://PS3_GAME/PARAM.SFO"));
        }
      }
    }
  }

  std::string baseVersion = version;

  if (!titleId.empty()) {
    const std::string updateSfo =
        rpcs3::utils::get_hdd0_game_dir() + titleId + "/PARAM.SFO";

    if (fs::is_file(updateSfo)) {
      if (const fs::file sfo{updateSfo}) {
        const auto psf = psf::load_object(sfo, updateSfo);
        const auto updateVersion =
            std::string(psf::get_string(psf, "APP_VER", ""));

        if (!updateVersion.empty()) {
          version = updateVersion;
        }
      }
    }
  }

  return wrap(env, nlohmann::json{
                       {"titleId", titleId},
                       {"title", title},
                       {"version", version},
                       {"baseVersion", baseVersion},
                       {"category", category},
                       {"updated", version != baseVersion},
                   }
                       .dump());
}

extern "C" JNIEXPORT jstring JNICALL
Java_net_rpcs3_RPCS3_patchFiles(JNIEnv *env, jobject) {
  auto array = nlohmann::json::array();
  const std::string dir = patch_engine::get_patches_path();

  if (fs::is_dir(dir)) {
    for (auto &&entry : fs::dir(dir)) {
      if (entry.is_directory || !entry.name.ends_with(".yml")) {
        continue;
      }

      patch_engine::patch_map parsed;
      patch_engine::load(parsed, dir + entry.name);

      std::size_t count = 0;
      for (const auto &[hash, container] : parsed) {
        count += container.patch_info_map.size();
      }

      array.push_back({
          {"name", entry.name},
          {"size", static_cast<std::uint64_t>(entry.size)},
          {"patchCount", static_cast<std::uint64_t>(count)},
      });
    }
  }

  return wrap(env, array.dump());
}

extern "C" JNIEXPORT jboolean JNICALL Java_net_rpcs3_RPCS3_patchImport(
    JNIEnv *env, jobject, jint fd, jstring jname) {
  auto file = fs::file::from_native_handle(fd);
  AtExit atExit{[&] { file.release_handle(); }};

  std::string name = unwrap(env, jname);

  if (name.empty() || name.find('/') != std::string::npos) {
    return false;
  }

  if (!name.ends_with(".yml")) {
    name += ".yml";
  }

  const std::string dir = patch_engine::get_patches_path();

  if (!fs::is_dir(dir) && !fs::create_path(dir)) {
    return false;
  }

  const std::string target = dir + name;
  const std::string staging = target + ".part";

  file.seek(0);
  std::vector<u8> data(file.size());

  if (!data.empty() && file.read(data.data(), data.size()) != data.size()) {
    return false;
  }

  {
    fs::file out(staging, fs::rewrite);

    if (!out || out.write(data.data(), data.size()) != data.size()) {
      fs::remove_file(staging);
      return false;
    }
  }

  patch_engine::patch_map parsed;

  if (!patch_engine::load(parsed, staging) || parsed.empty()) {
    fs::remove_file(staging);
    return false;
  }

  if (fs::is_file(target)) {
    fs::remove_file(target);
  }

  return fs::rename(staging, target, true);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_net_rpcs3_RPCS3_patchFileDelete(JNIEnv *env, jobject, jstring jname) {
  const std::string name = unwrap(env, jname);

  if (name.empty() || name.find('/') != std::string::npos) {
    return false;
  }

  return fs::remove_file(patch_engine::get_patches_path() + name);
}

extern "C" JNIEXPORT jstring JNICALL
Java_net_rpcs3_RPCS3_installedUpdates(JNIEnv *env, jobject, jstring jtitleId) {
  const std::string titleId = unwrap(env, jtitleId);
  auto array = nlohmann::json::array();
  const std::string gameDir = rpcs3::utils::get_hdd0_game_dir();

  if (!fs::is_dir(gameDir)) {
    return wrap(env, array.dump());
  }

  for (auto &&entry : fs::dir(gameDir)) {
    if (!entry.is_directory || entry.name == "." || entry.name == "..") {
      continue;
    }

    if (!titleId.empty() && entry.name.find(titleId) == std::string::npos) {
      continue;
    }

    const std::string path = gameDir + entry.name;
    const std::string sfoPath = path + "/PARAM.SFO";

    std::string title;
    std::string category;
    std::string version;

    if (fs::is_file(sfoPath)) {
      if (const fs::file sfo{sfoPath}) {
        const auto psf = psf::load_object(sfo, sfoPath);
        title = std::string(psf::get_string(psf, "TITLE", ""));
        category = std::string(psf::get_string(psf, "CATEGORY", ""));
        version = std::string(psf::get_string(psf, "APP_VER", ""));
      }
    }

    array.push_back({
        {"path", path},
        {"name", entry.name},
        {"title", title},
        {"category", category},
        {"version", version},
        {"size", static_cast<std::uint64_t>(fs::get_dir_size(path, 1))},
    });
  }

  return wrap(env, array.dump());
}

extern "C" JNIEXPORT jboolean JNICALL
Java_net_rpcs3_RPCS3_uninstallUpdate(JNIEnv *env, jobject, jstring jpath) {
  const std::string path = unwrap(env, jpath);
  const std::string gameDir = rpcs3::utils::get_hdd0_game_dir();

  if (path.empty() || !path.starts_with(gameDir) || !fs::is_dir(path)) {
    return false;
  }

  return fs::remove_all(path, true);
}

extern "C" JNIEXPORT jboolean JNICALL Java_net_rpcs3_RPCS3_installPackages(
    JNIEnv *env, jobject, jintArray jfds, jobjectArray jnames,
    jlong progressId) {
  const jsize count = env->GetArrayLength(jfds);

  if (count <= 0) {
    Progress(env, progressId).failure("No packages to install");
    return false;
  }

  std::vector<jint> fds(count);
  env->GetIntArrayRegion(jfds, 0, count, fds.data());

  std::vector<std::pair<std::string, fs::file>> files;
  files.reserve(count);

  for (jsize index = 0; index < count; index++) {
    std::string name = "package.pkg";

    if (jnames != nullptr) {
      if (auto jname = reinterpret_cast<jstring>(
              env->GetObjectArrayElement(jnames, index))) {
        name = unwrap(env, jname);
      }
    }

    files.emplace_back(std::move(name),
                       fs::file::from_native_handle(fds[index]));
  }

  return installPkgs(env, std::move(files), progressId);
}

extern "C" JNIEXPORT jstring JNICALL
Java_net_rpcs3_RPCS3_pkgInfo(JNIEnv *env, jobject, jint fd) {
  auto file = fs::file::from_native_handle(fd);

  auto type = getFileType(file);
  file.seek(0);

  if (type != FileType::Pkg) {
    file.release_handle();

    return wrap(env, nlohmann::json{
                         {"valid", false},
                         {"kind", type == FileType::Rap   ? "rap"
                                  : type == FileType::Edat ? "edat"
                                  : type == FileType::Iso  ? "iso"
                                  : type == FileType::Pup  ? "pup"
                                                           : "unknown"},
                     }
                         .dump());
  }

  package_reader reader("package.pkg", std::move(file));
  AtExit atExit{[&] { reader.file().release_handle(); }};

  if (!reader.is_valid()) {
    return wrap(env, nlohmann::json{{"valid", false}, {"kind", "pkg"}}.dump());
  }

  const psf::registry &psf = reader.get_psf();
  const auto category = std::string(psf::get_string(psf, "CATEGORY", ""));
  const bool isPatch =
      (reader.get_metadata().package_type & pkg_flag::PKG_FLAG_PATCH) != 0;

  return wrap(env, nlohmann::json{
                       {"valid", true},
                       {"kind", "pkg"},
                       {"title", std::string(psf::get_string(psf, "TITLE", ""))},
                       {"titleId",
                        std::string(psf::get_string(psf, "TITLE_ID", ""))},
                       {"category", category},
                       {"appVer",
                        std::string(psf::get_string(psf, "APP_VER", ""))},
                       {"targetAppVer",
                        std::string(psf::get_string(psf, "TARGET_APP_VER", ""))},
                       {"dataSize",
                        static_cast<std::uint64_t>(
                            reader.get_header().data_size.value())},
                       {"isUpdate", category == "GD" && isPatch},
                       {"isDlc", category == "GD" && !isPatch},
                   }
                       .dump());
}

extern "C" JNIEXPORT jboolean JNICALL Java_net_rpcs3_RPCS3_installKey(
    JNIEnv *env, jobject, jint fd, jlong progressId, jstring gamePath) {
  auto file = fs::file::from_native_handle(fd);
  AtExit atExit{[&] { file.release_handle(); }};

  auto type = getFileType(file);
  file.seek(0);

  if (type == FileType::Rap) {
    return installRap(env, std::move(file), progressId, unwrap(env, gamePath));
  }

  if (type == FileType::Edat) {
    return installEdat(env, std::move(file), progressId, unwrap(env, gamePath));
  }

  Progress(env, progressId).failure("Unsupported key type");
  return false;
}

extern "C" JNIEXPORT jstring JNICALL
Java_net_rpcs3_RPCS3_systemInfo(JNIEnv *env, jobject) {
  std::string result;

  fmt::append(result, "%s\n\nLLVM CPU: %s\n\n", utils::get_system_info(), fallback_cpu_detection());

  {
    vk::instance device_enum_context;
    if (device_enum_context.create("RPCS3")) {
      device_enum_context.bind();
      const std::vector<vk::physical_device> &gpus =
          device_enum_context.enumerate_devices();

      for (const auto &gpu : gpus) {
        fmt::append(
                result,
                "GPU: %s\n\nDriver: %s (v%s)\n\nVulkan: %s",
                gpu.get_name(),
                gpu.get_driver_name(),
                gpu.get_driver_version(),
                gpu.get_driver_vk_version());
      }
    }
  }

  return wrap(env, result);
}

static cfg::_base *find_cfg_node(cfg::_base *root, std::string_view path) {
  auto pathList = fmt::split(path, {"@@"});
  std::ranges::reverse(pathList);

  while (!pathList.empty()) {
    auto elem = pathList.back();
    pathList.pop_back();
    if (elem.empty()) {
      continue;
    }

    auto root_node = dynamic_cast<cfg::node *>(root);
    if (root_node == nullptr) {
      return nullptr;
    }

    cfg::_base *child_node = nullptr;

    for (auto node : root_node->get_nodes()) {
      if (node->get_name() == elem) {
        child_node = node;
        break;
      }
    }

    if (child_node == nullptr) {
      return nullptr;
    }

    root = child_node;
  }

  return root;
}

static std::mutex g_settings_mutex;
static std::string g_settings_title;
static std::unique_ptr<cfg_root> g_settings_cfg;

static cfg_root *settings_root_for(const std::string &titleId) {
  if (titleId.empty()) {
    return &g_cfg;
  }

  if (g_settings_cfg && g_settings_title == titleId) {
    return g_settings_cfg.get();
  }

  auto cfg = std::make_unique<cfg_root>();
  cfg->from_default();

  if (fs::file file{fs::get_config_dir(true) + "config.yml"}) {
    cfg->from_string(file.to_string());
  }

  if (const std::string path = rpcs3::utils::get_custom_config_path(titleId);
      !path.empty()) {
    if (fs::file file{path}) {
      cfg->from_string(file.to_string());
    }
  }

  g_settings_cfg = std::move(cfg);
  g_settings_title = titleId;
  return g_settings_cfg.get();
}

static std::mutex g_save_mutex;
static std::condition_variable g_save_cv;
static std::map<std::string, std::string> g_save_pending;
static bool g_save_busy = false;
static bool g_save_now = false;
static std::once_flag g_save_once;

static std::string g_save_failure;

static void settings_save_loop() {
  pthread_setname_np(pthread_self(), "rpcs3-cfgsave");
  std::unique_lock lock(g_save_mutex);

  while (true) {
    g_save_cv.wait(lock, [] { return !g_save_pending.empty(); });
    g_save_cv.wait_for(lock, std::chrono::milliseconds(200),
                       [] { return g_save_now; });

    auto batch = std::move(g_save_pending);
    g_save_pending.clear();
    g_save_busy = true;

    lock.unlock();
    for (const auto &[title, data] : batch) {
      if (!title.empty()) {
        fs::create_path(rpcs3::utils::get_custom_config_dir());
      }

      Emulator::SaveSettings(data, title);

      const std::string written =
          title.empty() ? fs::get_config_dir(true) + "config.yml"
                        : rpcs3::utils::get_custom_config_path(title);

      if (!fs::is_file(written)) {
        rpcs3_android.error("settings save failed: %s", written);
        g_save_failure = written;
      }
    }
    lock.lock();

    g_save_busy = false;
    g_save_cv.notify_all();
  }
}

static void settings_save_async(std::string data, std::string titleId) {
  std::call_once(g_save_once,
                 [] { std::thread(settings_save_loop).detach(); });

  {
    std::lock_guard lock(g_save_mutex);
    g_save_pending[std::move(titleId)] = std::move(data);
  }

  g_save_cv.notify_all();
}

static void settings_save_flush() {
  std::unique_lock lock(g_save_mutex);
  g_save_now = true;
  g_save_cv.notify_all();
  g_save_cv.wait(lock, [] { return g_save_pending.empty() && !g_save_busy; });
  g_save_now = false;
}

extern "C" JNIEXPORT void JNICALL
Java_net_rpcs3_RPCS3_settingsFlush(JNIEnv *, jobject) {
  settings_save_flush();
}

extern "C" JNIEXPORT jstring JNICALL
Java_net_rpcs3_RPCS3_settingsGet(JNIEnv *env, jobject, jstring jpath,
                                 jstring jtitleId) {
  const std::string titleId = unwrap(env, jtitleId);
  std::lock_guard lock(g_settings_mutex);

  auto root = find_cfg_node(settings_root_for(titleId), unwrap(env, jpath));

  if (root == nullptr) {
    return nullptr;
  }

  return wrap(env, root->to_json().dump(4));
}

extern "C" JNIEXPORT jboolean JNICALL Java_net_rpcs3_RPCS3_settingsSet(
    JNIEnv *env, jobject, jstring jpath, jstring jvalue, jstring jtitleId) {
  nlohmann::json value;
  try {
    value = nlohmann::json::parse(unwrap(env, jvalue));
  } catch (...) {
    rpcs3_android.error("settingsSet: node %s passed with invalid json '%s'",
                        unwrap(env, jpath), unwrap(env, jvalue));
    return false;
  }

  const std::string titleId = unwrap(env, jtitleId);
  std::lock_guard lock(g_settings_mutex);

  cfg_root *cfg = settings_root_for(titleId);
  auto root = find_cfg_node(cfg, unwrap(env, jpath));

  if (root == nullptr) {
    rpcs3_android.error("settingsSet: node %s not found", unwrap(env, jpath));
    return false;
  }

  const bool dynamic = titleId.empty() && !Emu.IsStopped();

  if (!root->from_json(value, dynamic)) {
    rpcs3_android.error("settingsSet: node %s not accepts value '%s'",
                        unwrap(env, jpath), value.dump());
    return false;
  }

  if (titleId.empty() && root->get_type() == cfg::type::log) {
    rpcs3::utils::configure_logs(Emu.IsStopped());
  }

  settings_save_async(cfg->to_string(), titleId);
  return true;
}

extern "C" JNIEXPORT jstring JNICALL
Java_net_rpcs3_RPCS3_logChannels(JNIEnv *env, jobject) {
  auto result = nlohmann::json::array();

  for (const auto &name : logs::get_channels()) {
    result.push_back(name);
  }

  return wrap(env, result.dump());
}

extern "C" JNIEXPORT jboolean JNICALL
Java_net_rpcs3_RPCS3_supportsCustomDriverLoading(JNIEnv *env,
                                                 jobject instance) {
  return access("/dev/kgsl-3d0", F_OK) == 0;
}

static patch_engine::patch_map loadAllPatches(const std::string &titleId) {
  patch_engine::patch_map result;

  const std::string dir = patch_engine::get_patches_path();

  if (fs::is_dir(dir)) {
    for (auto &&entry : fs::dir(dir)) {
      if (entry.is_directory || !entry.name.ends_with(".yml")) {
        continue;
      }
      patch_engine::load(result, dir + entry.name, "", false, nullptr, titleId);
    }
  }

  const std::string imported = patch_engine::get_imported_patch_path();

  if (fs::is_file(imported)) {
    patch_engine::load(result, imported, "", false, nullptr, titleId);
  }

  return result;
}

static bool patchAppliesTo(const std::string &serial, const std::string &titleId) {
  return serial == patch_key::all || serial == titleId;
}

extern "C" JNIEXPORT jstring JNICALL Java_net_rpcs3_RPCS3_patchesGet(
    JNIEnv *env, jobject, jstring jtitleId) {
  const std::string titleId = unwrap(env, jtitleId);
  auto array = nlohmann::json::array();

  try {
    auto patches = loadAllPatches(titleId);

    for (const auto &[hash, container] : patches) {
      for (const auto &[description, info] : container.patch_info_map) {
        for (const auto &[title, serials] : info.titles) {
          for (const auto &[serial, app_versions] : serials) {
            if (!patchAppliesTo(serial, titleId)) {
              continue;
            }

            for (const auto &[app_version, values] : app_versions) {
              array.push_back({
                  {"hash", hash},
                  {"description", description},
                  {"title", title},
                  {"serial", serial},
                  {"appVersion", app_version},
                  {"author", info.author},
                  {"notes", info.notes},
                  {"group", info.patch_group},
                  {"patchVersion", info.patch_version},
                  {"enabled", values.enabled},
              });
            }
          }
        }
      }
    }
  } catch (const std::exception &e) {
    rpcs3_android.error("patchesGet failed: %s", e.what());
    return wrap(env, nlohmann::json::array().dump());
  } catch (...) {
    rpcs3_android.error("patchesGet failed");
    return wrap(env, nlohmann::json::array().dump());
  }

  return wrap(env, array.dump());
}

extern "C" JNIEXPORT jstring JNICALL Java_net_rpcs3_RPCS3_patchesAll(
    JNIEnv *env, jobject) {
  auto array = nlohmann::json::array();

  try {
    auto patches = loadAllPatches({});

    for (const auto &[hash, container] : patches) {
      for (const auto &[description, info] : container.patch_info_map) {
        for (const auto &[title, serials] : info.titles) {
          std::string serialList;
          std::string versionList;

          for (const auto &[serial, app_versions] : serials) {
            if (!serialList.empty()) {
              serialList += ", ";
            }
            serialList += serial;

            for (const auto &[app_version, values] : app_versions) {
              if (versionList.find(app_version) != std::string::npos) {
                continue;
              }
              if (!versionList.empty()) {
                versionList += ", ";
              }
              versionList += app_version;
            }
          }

          array.push_back({
              {"hash", hash},
              {"description", description},
              {"title", title},
              {"serials", serialList},
              {"appVersions", versionList},
              {"author", info.author},
              {"notes", info.notes},
              {"group", info.patch_group},
              {"patchVersion", info.patch_version},
          });
        }
      }
    }
  } catch (const std::exception &e) {
    rpcs3_android.error("patchesAll failed: %s", e.what());
    return wrap(env, nlohmann::json::array().dump());
  } catch (...) {
    rpcs3_android.error("patchesAll failed");
    return wrap(env, nlohmann::json::array().dump());
  }

  return wrap(env, array.dump());
}

extern "C" JNIEXPORT jboolean JNICALL Java_net_rpcs3_RPCS3_patchSet(
    JNIEnv *env, jobject, jstring jtitleId, jstring jhash,
    jstring jdescription, jstring jtitle, jstring jserial,
    jstring jappVersion, jboolean enabled) {
  const std::string titleId = unwrap(env, jtitleId);
  const std::string hash = unwrap(env, jhash);
  const std::string description = unwrap(env, jdescription);
  const std::string title = unwrap(env, jtitle);
  const std::string serial = unwrap(env, jserial);
  const std::string appVersion = unwrap(env, jappVersion);

  try {
    auto patches = loadAllPatches(titleId);

    auto container = patches.find(hash);
    if (container == patches.end()) {
      return false;
    }

    auto info = container->second.patch_info_map.find(description);
    if (info == container->second.patch_info_map.end()) {
      return false;
    }

    info->second.titles[title][serial][appVersion].enabled = enabled;
    patch_engine::save_config(patches, titleId);
  } catch (const std::exception &e) {
    rpcs3_android.error("patchSet failed: %s", e.what());
    return false;
  } catch (...) {
    rpcs3_android.error("patchSet failed");
    return false;
  }

  return true;
}


static std::string resolveDescriptorPath(int fd) {
  const std::string procPath = "/proc/self/fd/" + std::to_string(fd);
  char resolved[PATH_MAX]{};
  const ssize_t length = ::readlink(procPath.c_str(), resolved, sizeof(resolved) - 1);

  if (length <= 0) {
    return {};
  }

  const std::string link(resolved, static_cast<std::size_t>(length));
  std::vector<std::string> candidates{link};

  auto reroot = [&](std::string_view prefix, bool dropFirstSegment) {
    if (!link.starts_with(prefix)) {
      return;
    }

    std::string rest = link.substr(prefix.size());

    if (dropFirstSegment) {
      const auto slash = rest.find('/');
      if (slash == std::string::npos) {
        return;
      }
      rest = rest.substr(slash + 1);
    }

    candidates.push_back("/storage/" + rest);
  };

  reroot("/mnt/user/", true);
  reroot("/mnt/runtime/", true);
  reroot("/mnt/androidwritable/", true);
  reroot("/mnt/media_rw/", false);

  for (const auto &candidate : candidates) {
    if (is_iso_file(candidate)) {
      rpcs3_android.notice("resolveDescriptorPath: '%s' -> '%s'", link, candidate);
      return candidate;
    }
  }

  rpcs3_android.error("resolveDescriptorPath: no readable path for '%s'", link);
  return {};
}

extern "C" JNIEXPORT jint JNICALL Java_net_rpcs3_RPCS3_bootIso(JNIEnv *env,
                                                               jobject,
                                                               jint fd) {
  const std::string real = resolveDescriptorPath(fd);

  if (real.empty()) {
    rpcs3_android.error("bootIso: no readable path for descriptor %d", fd);
    return static_cast<int>(game_boot_result::invalid_file_or_folder);
  }

  auto emuLock = g_compilationQueue.acquireEmu();

  if (!emuLock.owns_lock()) {
    return static_cast<int>(game_boot_result::still_running);
  }

  Emu.SetForceBoot(true);
  return static_cast<int>(Emu.BootGame(real, "", false, cfg_mode::custom));
}

extern "C" JNIEXPORT jboolean JNICALL Java_net_rpcs3_RPCS3_addIsoEntry(
    JNIEnv *env, jobject, jint fd, jlong progressId) {
  Progress progress(env, progressId);

  const std::string isoPath = resolveDescriptorPath(fd);

  if (isoPath.empty()) {
    progress.failure("Could not read the disc image. Grant all-files access.");
    return false;
  }

  fs::file image{isoPath};

  if (!image) {
    progress.failure("Could not open the disc image. Grant all-files access.");
    return false;
  }

  auto optIso = iso_fs::open(std::make_unique<file_view_block_dev>(image));

  if (!optIso) {
    progress.failure("Failed to read the disc image");
    return false;
  }

  auto iso = std::move(*optIso);
  auto sfoFile = iso.open("PS3_GAME/PARAM.SFO");

  if (!sfoFile) {
    progress.failure("Failed to locate PARAM.SFO in the disc image");
    return false;
  }

  auto sfo = psf::load_object(sfoFile, "iso://PS3_GAME/PARAM.SFO");
  auto titleId = std::string(psf::get_string(sfo, "TITLE_ID"));

  if (titleId.empty()) {
    progress.failure("Failed to read TITLE_ID from the disc image");
    return false;
  }

  std::string iconPath;

  if (auto icon = iso.open("PS3_GAME/ICON0.PNG")) {
    const std::string iconDir = fs::get_config_dir() + "Icons/iso/";
    fs::create_path(iconDir);

    const std::string target = iconDir + titleId + ".PNG";

    if (fs::file out{target, fs::rewrite}) {
      std::vector<u8> buffer(icon.size());
      icon.read_at(0, buffer.data(), buffer.size());
      out.write(buffer.data(), buffer.size());
      iconPath = target;
    }
  }

  GameInfo info{};
  info.path = isoPath;
  info.name = std::string(psf::get_string(sfo, "TITLE"));
  info.iconPath = iconPath;
  info.flags = 0;

  if (info.name.empty()) {
    info.name = titleId;
  }

  sendGameInfo(env, progressId, {{info}});
  progress.report(1, 1);
  return true;
}

extern "C" JNIEXPORT jstring JNICALL
Java_net_rpcs3_RPCS3_takeSettingsSaveFailure(JNIEnv *env, jobject) {
  std::lock_guard lock(g_save_mutex);

  if (g_save_failure.empty()) {
    return nullptr;
  }

  auto result = wrap(env, g_save_failure);
  g_save_failure.clear();
  return result;
}
