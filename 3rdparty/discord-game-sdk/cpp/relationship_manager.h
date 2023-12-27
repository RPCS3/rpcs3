#pragma once

#include "types.h"

namespace discord {

class RelationshipManager final {
public:
    ~RelationshipManager() = default;

    void Filter(std::function<bool(Relationship const&)> filter);
    Result Count(std::int32_t* count);
    Result Get(UserId userId, Relationship* relationship);
    Result GetAt(std::uint32_t index, Relationship* relationship);

    Event<> OnRefresh;
    Event<Relationship const&> OnRelationshipUpdate;

private:
    friend class Core;

    RelationshipManager() = default;
    RelationshipManager(RelationshipManager const& rhs) = delete;
    RelationshipManager& operator=(RelationshipManager const& rhs) = delete;
    RelationshipManager(RelationshipManager&& rhs) = delete;
    RelationshipManager& operator=(RelationshipManager&& rhs) = delete;

    IDiscordRelationshipManager* internal_;
    static IDiscordRelationshipEvents events_;
};

} // namespace discord
