#if !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "relationship_manager.h"

#include "core.h"

#include <cstring>
#include <memory>

namespace discord {

class RelationshipEvents final {
public:
    static void DISCORD_CALLBACK OnRefresh(void* callbackData)
    {
        auto* core = reinterpret_cast<Core*>(callbackData);
        if (!core) {
            return;
        }

        auto& module = core->RelationshipManager();
        module.OnRefresh();
    }

    static void DISCORD_CALLBACK OnRelationshipUpdate(void* callbackData,
                                                      DiscordRelationship* relationship)
    {
        auto* core = reinterpret_cast<Core*>(callbackData);
        if (!core) {
            return;
        }

        auto& module = core->RelationshipManager();
        module.OnRelationshipUpdate(*reinterpret_cast<Relationship const*>(relationship));
    }
};

IDiscordRelationshipEvents RelationshipManager::events_{
  &RelationshipEvents::OnRefresh,
  &RelationshipEvents::OnRelationshipUpdate,
};

void RelationshipManager::Filter(std::function<bool(Relationship const&)> filter)
{
    static auto wrapper = [](void* callbackData, DiscordRelationship* relationship) -> bool {
        auto cb(reinterpret_cast<std::function<bool(Relationship const&)>*>(callbackData));
        if (!cb || !(*cb)) {
            return {};
        }
        return (*cb)(*reinterpret_cast<Relationship const*>(relationship));
    };
    std::unique_ptr<std::function<bool(Relationship const&)>> cb{};
    cb.reset(new std::function<bool(Relationship const&)>(std::move(filter)));
    internal_->filter(internal_, cb.get(), wrapper);
}

Result RelationshipManager::Count(std::int32_t* count)
{
    if (!count) {
        return Result::InternalError;
    }

    auto result = internal_->count(internal_, reinterpret_cast<int32_t*>(count));
    return static_cast<Result>(result);
}

Result RelationshipManager::Get(UserId userId, Relationship* relationship)
{
    if (!relationship) {
        return Result::InternalError;
    }

    auto result =
      internal_->get(internal_, userId, reinterpret_cast<DiscordRelationship*>(relationship));
    return static_cast<Result>(result);
}

Result RelationshipManager::GetAt(std::uint32_t index, Relationship* relationship)
{
    if (!relationship) {
        return Result::InternalError;
    }

    auto result =
      internal_->get_at(internal_, index, reinterpret_cast<DiscordRelationship*>(relationship));
    return static_cast<Result>(result);
}

} // namespace discord
