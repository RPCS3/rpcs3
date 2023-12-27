#pragma once

#include <functional>
#include <vector>

namespace discord {

template <typename... Args>
class Event final {
public:
    using Token = int;

    Event() { slots_.reserve(4); }

    Event(Event const&) = default;
    Event(Event&&) = default;
    ~Event() = default;

    Event& operator=(Event const&) = default;
    Event& operator=(Event&&) = default;

    template <typename EventHandler>
    Token Connect(EventHandler slot)
    {
        slots_.emplace_back(Slot{nextToken_, std::move(slot)});
        return nextToken_++;
    }

    void Disconnect(Token token)
    {
        for (auto& slot : slots_) {
            if (slot.token == token) {
                slot = slots_.back();
                slots_.pop_back();
                break;
            }
        }
    }

    void DisconnectAll() { slots_ = {}; }

    void operator()(Args... args)
    {
        for (auto const& slot : slots_) {
            slot.fn(std::forward<Args>(args)...);
        }
    }

private:
    struct Slot {
        Token token;
        std::function<void(Args...)> fn;
    };

    Token nextToken_{};
    std::vector<Slot> slots_{};
};

} // namespace discord
