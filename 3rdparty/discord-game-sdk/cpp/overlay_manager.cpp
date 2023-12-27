#if !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "overlay_manager.h"

#include "core.h"

#include <cstring>
#include <memory>

namespace discord {

class OverlayEvents final {
public:
    static void DISCORD_CALLBACK OnToggle(void* callbackData, bool locked)
    {
        auto* core = reinterpret_cast<Core*>(callbackData);
        if (!core) {
            return;
        }

        auto& module = core->OverlayManager();
        module.OnToggle((locked != 0));
    }
};

IDiscordOverlayEvents OverlayManager::events_{
  &OverlayEvents::OnToggle,
};

void OverlayManager::IsEnabled(bool* enabled)
{
    if (!enabled) {
        return;
    }

    internal_->is_enabled(internal_, reinterpret_cast<bool*>(enabled));
}

void OverlayManager::IsLocked(bool* locked)
{
    if (!locked) {
        return;
    }

    internal_->is_locked(internal_, reinterpret_cast<bool*>(locked));
}

void OverlayManager::SetLocked(bool locked, std::function<void(Result)> callback)
{
    static auto wrapper = [](void* callbackData, EDiscordResult result) -> void {
        std::unique_ptr<std::function<void(Result)>> cb(
          reinterpret_cast<std::function<void(Result)>*>(callbackData));
        if (!cb || !(*cb)) {
            return;
        }
        (*cb)(static_cast<Result>(result));
    };
    std::unique_ptr<std::function<void(Result)>> cb{};
    cb.reset(new std::function<void(Result)>(std::move(callback)));
    internal_->set_locked(internal_, (locked ? 1 : 0), cb.release(), wrapper);
}

void OverlayManager::OpenActivityInvite(ActivityActionType type,
                                        std::function<void(Result)> callback)
{
    static auto wrapper = [](void* callbackData, EDiscordResult result) -> void {
        std::unique_ptr<std::function<void(Result)>> cb(
          reinterpret_cast<std::function<void(Result)>*>(callbackData));
        if (!cb || !(*cb)) {
            return;
        }
        (*cb)(static_cast<Result>(result));
    };
    std::unique_ptr<std::function<void(Result)>> cb{};
    cb.reset(new std::function<void(Result)>(std::move(callback)));
    internal_->open_activity_invite(
      internal_, static_cast<EDiscordActivityActionType>(type), cb.release(), wrapper);
}

void OverlayManager::OpenGuildInvite(char const* code, std::function<void(Result)> callback)
{
    static auto wrapper = [](void* callbackData, EDiscordResult result) -> void {
        std::unique_ptr<std::function<void(Result)>> cb(
          reinterpret_cast<std::function<void(Result)>*>(callbackData));
        if (!cb || !(*cb)) {
            return;
        }
        (*cb)(static_cast<Result>(result));
    };
    std::unique_ptr<std::function<void(Result)>> cb{};
    cb.reset(new std::function<void(Result)>(std::move(callback)));
    internal_->open_guild_invite(internal_, const_cast<char*>(code), cb.release(), wrapper);
}

void OverlayManager::OpenVoiceSettings(std::function<void(Result)> callback)
{
    static auto wrapper = [](void* callbackData, EDiscordResult result) -> void {
        std::unique_ptr<std::function<void(Result)>> cb(
          reinterpret_cast<std::function<void(Result)>*>(callbackData));
        if (!cb || !(*cb)) {
            return;
        }
        (*cb)(static_cast<Result>(result));
    };
    std::unique_ptr<std::function<void(Result)>> cb{};
    cb.reset(new std::function<void(Result)>(std::move(callback)));
    internal_->open_voice_settings(internal_, cb.release(), wrapper);
}

Result OverlayManager::InitDrawingDxgi(IDXGISwapChain* swapchain, bool useMessageForwarding)
{
    auto result =
      internal_->init_drawing_dxgi(internal_, swapchain, (useMessageForwarding ? 1 : 0));
    return static_cast<Result>(result);
}

void OverlayManager::OnPresent()
{
    internal_->on_present(internal_);
}

void OverlayManager::ForwardMessage(MSG* message)
{
    internal_->forward_message(internal_, message);
}

void OverlayManager::KeyEvent(bool down, char const* keyCode, KeyVariant variant)
{
    internal_->key_event(internal_,
                         (down ? 1 : 0),
                         const_cast<char*>(keyCode),
                         static_cast<EDiscordKeyVariant>(variant));
}

void OverlayManager::CharEvent(char const* character)
{
    internal_->char_event(internal_, const_cast<char*>(character));
}

void OverlayManager::MouseButtonEvent(std::uint8_t down,
                                      std::int32_t clickCount,
                                      MouseButton which,
                                      std::int32_t x,
                                      std::int32_t y)
{
    internal_->mouse_button_event(
      internal_, down, clickCount, static_cast<EDiscordMouseButton>(which), x, y);
}

void OverlayManager::MouseMotionEvent(std::int32_t x, std::int32_t y)
{
    internal_->mouse_motion_event(internal_, x, y);
}

void OverlayManager::ImeCommitText(char const* text)
{
    internal_->ime_commit_text(internal_, const_cast<char*>(text));
}

void OverlayManager::ImeSetComposition(char const* text,
                                       ImeUnderline* underlines,
                                       std::uint32_t underlinesLength,
                                       std::int32_t from,
                                       std::int32_t to)
{
    internal_->ime_set_composition(internal_,
                                   const_cast<char*>(text),
                                   reinterpret_cast<DiscordImeUnderline*>(underlines),
                                   underlinesLength,
                                   from,
                                   to);
}

void OverlayManager::ImeCancelComposition()
{
    internal_->ime_cancel_composition(internal_);
}

void OverlayManager::SetImeCompositionRangeCallback(
  std::function<void(std::int32_t, std::int32_t, Rect*, std::uint32_t)>
    onImeCompositionRangeChanged)
{
    static auto wrapper = [](void* callbackData,
                             int32_t from,
                             int32_t to,
                             DiscordRect* bounds,
                             uint32_t boundsLength) -> void {
        std::unique_ptr<std::function<void(std::int32_t, std::int32_t, Rect*, std::uint32_t)>> cb(
          reinterpret_cast<std::function<void(std::int32_t, std::int32_t, Rect*, std::uint32_t)>*>(
            callbackData));
        if (!cb || !(*cb)) {
            return;
        }
        (*cb)(from, to, reinterpret_cast<Rect*>(bounds), boundsLength);
    };
    std::unique_ptr<std::function<void(std::int32_t, std::int32_t, Rect*, std::uint32_t)>> cb{};
    cb.reset(new std::function<void(std::int32_t, std::int32_t, Rect*, std::uint32_t)>(
      std::move(onImeCompositionRangeChanged)));
    internal_->set_ime_composition_range_callback(internal_, cb.release(), wrapper);
}

void OverlayManager::SetImeSelectionBoundsCallback(
  std::function<void(Rect, Rect, bool)> onImeSelectionBoundsChanged)
{
    static auto wrapper =
      [](void* callbackData, DiscordRect anchor, DiscordRect focus, bool isAnchorFirst) -> void {
        std::unique_ptr<std::function<void(Rect, Rect, bool)>> cb(
          reinterpret_cast<std::function<void(Rect, Rect, bool)>*>(callbackData));
        if (!cb || !(*cb)) {
            return;
        }
        (*cb)(*reinterpret_cast<Rect const*>(&anchor),
              *reinterpret_cast<Rect const*>(&focus),
              (isAnchorFirst != 0));
    };
    std::unique_ptr<std::function<void(Rect, Rect, bool)>> cb{};
    cb.reset(new std::function<void(Rect, Rect, bool)>(std::move(onImeSelectionBoundsChanged)));
    internal_->set_ime_selection_bounds_callback(internal_, cb.release(), wrapper);
}

bool OverlayManager::IsPointInsideClickZone(std::int32_t x, std::int32_t y)
{
    auto result = internal_->is_point_inside_click_zone(internal_, x, y);
    return (result != 0);
}

} // namespace discord
