#if !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "lobby_manager.h"

#include "core.h"

#include <cstring>
#include <memory>

namespace discord {

class LobbyEvents final {
public:
    static void DISCORD_CALLBACK OnLobbyUpdate(void* callbackData, int64_t lobbyId)
    {
        auto* core = reinterpret_cast<Core*>(callbackData);
        if (!core) {
            return;
        }

        auto& module = core->LobbyManager();
        module.OnLobbyUpdate(lobbyId);
    }

    static void DISCORD_CALLBACK OnLobbyDelete(void* callbackData, int64_t lobbyId, uint32_t reason)
    {
        auto* core = reinterpret_cast<Core*>(callbackData);
        if (!core) {
            return;
        }

        auto& module = core->LobbyManager();
        module.OnLobbyDelete(lobbyId, reason);
    }

    static void DISCORD_CALLBACK OnMemberConnect(void* callbackData,
                                                 int64_t lobbyId,
                                                 int64_t userId)
    {
        auto* core = reinterpret_cast<Core*>(callbackData);
        if (!core) {
            return;
        }

        auto& module = core->LobbyManager();
        module.OnMemberConnect(lobbyId, userId);
    }

    static void DISCORD_CALLBACK OnMemberUpdate(void* callbackData, int64_t lobbyId, int64_t userId)
    {
        auto* core = reinterpret_cast<Core*>(callbackData);
        if (!core) {
            return;
        }

        auto& module = core->LobbyManager();
        module.OnMemberUpdate(lobbyId, userId);
    }

    static void DISCORD_CALLBACK OnMemberDisconnect(void* callbackData,
                                                    int64_t lobbyId,
                                                    int64_t userId)
    {
        auto* core = reinterpret_cast<Core*>(callbackData);
        if (!core) {
            return;
        }

        auto& module = core->LobbyManager();
        module.OnMemberDisconnect(lobbyId, userId);
    }

    static void DISCORD_CALLBACK OnLobbyMessage(void* callbackData,
                                                int64_t lobbyId,
                                                int64_t userId,
                                                uint8_t* data,
                                                uint32_t dataLength)
    {
        auto* core = reinterpret_cast<Core*>(callbackData);
        if (!core) {
            return;
        }

        auto& module = core->LobbyManager();
        module.OnLobbyMessage(lobbyId, userId, data, dataLength);
    }

    static void DISCORD_CALLBACK OnSpeaking(void* callbackData,
                                            int64_t lobbyId,
                                            int64_t userId,
                                            bool speaking)
    {
        auto* core = reinterpret_cast<Core*>(callbackData);
        if (!core) {
            return;
        }

        auto& module = core->LobbyManager();
        module.OnSpeaking(lobbyId, userId, (speaking != 0));
    }

    static void DISCORD_CALLBACK OnNetworkMessage(void* callbackData,
                                                  int64_t lobbyId,
                                                  int64_t userId,
                                                  uint8_t channelId,
                                                  uint8_t* data,
                                                  uint32_t dataLength)
    {
        auto* core = reinterpret_cast<Core*>(callbackData);
        if (!core) {
            return;
        }

        auto& module = core->LobbyManager();
        module.OnNetworkMessage(lobbyId, userId, channelId, data, dataLength);
    }
};

IDiscordLobbyEvents LobbyManager::events_{
  &LobbyEvents::OnLobbyUpdate,
  &LobbyEvents::OnLobbyDelete,
  &LobbyEvents::OnMemberConnect,
  &LobbyEvents::OnMemberUpdate,
  &LobbyEvents::OnMemberDisconnect,
  &LobbyEvents::OnLobbyMessage,
  &LobbyEvents::OnSpeaking,
  &LobbyEvents::OnNetworkMessage,
};

Result LobbyManager::GetLobbyCreateTransaction(LobbyTransaction* transaction)
{
    if (!transaction) {
        return Result::InternalError;
    }

    auto result = internal_->get_lobby_create_transaction(internal_, transaction->Receive());
    return static_cast<Result>(result);
}

Result LobbyManager::GetLobbyUpdateTransaction(LobbyId lobbyId, LobbyTransaction* transaction)
{
    if (!transaction) {
        return Result::InternalError;
    }

    auto result =
      internal_->get_lobby_update_transaction(internal_, lobbyId, transaction->Receive());
    return static_cast<Result>(result);
}

Result LobbyManager::GetMemberUpdateTransaction(LobbyId lobbyId,
                                                UserId userId,
                                                LobbyMemberTransaction* transaction)
{
    if (!transaction) {
        return Result::InternalError;
    }

    auto result =
      internal_->get_member_update_transaction(internal_, lobbyId, userId, transaction->Receive());
    return static_cast<Result>(result);
}

void LobbyManager::CreateLobby(LobbyTransaction const& transaction,
                               std::function<void(Result, Lobby const&)> callback)
{
    static auto wrapper =
      [](void* callbackData, EDiscordResult result, DiscordLobby* lobby) -> void {
        std::unique_ptr<std::function<void(Result, Lobby const&)>> cb(
          reinterpret_cast<std::function<void(Result, Lobby const&)>*>(callbackData));
        if (!cb || !(*cb)) {
            return;
        }
        (*cb)(static_cast<Result>(result), *reinterpret_cast<Lobby const*>(lobby));
    };
    std::unique_ptr<std::function<void(Result, Lobby const&)>> cb{};
    cb.reset(new std::function<void(Result, Lobby const&)>(std::move(callback)));
    internal_->create_lobby(
      internal_, const_cast<LobbyTransaction&>(transaction).Internal(), cb.release(), wrapper);
}

void LobbyManager::UpdateLobby(LobbyId lobbyId,
                               LobbyTransaction const& transaction,
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
    internal_->update_lobby(internal_,
                            lobbyId,
                            const_cast<LobbyTransaction&>(transaction).Internal(),
                            cb.release(),
                            wrapper);
}

void LobbyManager::DeleteLobby(LobbyId lobbyId, std::function<void(Result)> callback)
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
    internal_->delete_lobby(internal_, lobbyId, cb.release(), wrapper);
}

void LobbyManager::ConnectLobby(LobbyId lobbyId,
                                LobbySecret secret,
                                std::function<void(Result, Lobby const&)> callback)
{
    static auto wrapper =
      [](void* callbackData, EDiscordResult result, DiscordLobby* lobby) -> void {
        std::unique_ptr<std::function<void(Result, Lobby const&)>> cb(
          reinterpret_cast<std::function<void(Result, Lobby const&)>*>(callbackData));
        if (!cb || !(*cb)) {
            return;
        }
        (*cb)(static_cast<Result>(result), *reinterpret_cast<Lobby const*>(lobby));
    };
    std::unique_ptr<std::function<void(Result, Lobby const&)>> cb{};
    cb.reset(new std::function<void(Result, Lobby const&)>(std::move(callback)));
    internal_->connect_lobby(internal_, lobbyId, const_cast<char*>(secret), cb.release(), wrapper);
}

void LobbyManager::ConnectLobbyWithActivitySecret(
  LobbySecret activitySecret,
  std::function<void(Result, Lobby const&)> callback)
{
    static auto wrapper =
      [](void* callbackData, EDiscordResult result, DiscordLobby* lobby) -> void {
        std::unique_ptr<std::function<void(Result, Lobby const&)>> cb(
          reinterpret_cast<std::function<void(Result, Lobby const&)>*>(callbackData));
        if (!cb || !(*cb)) {
            return;
        }
        (*cb)(static_cast<Result>(result), *reinterpret_cast<Lobby const*>(lobby));
    };
    std::unique_ptr<std::function<void(Result, Lobby const&)>> cb{};
    cb.reset(new std::function<void(Result, Lobby const&)>(std::move(callback)));
    internal_->connect_lobby_with_activity_secret(
      internal_, const_cast<char*>(activitySecret), cb.release(), wrapper);
}

void LobbyManager::DisconnectLobby(LobbyId lobbyId, std::function<void(Result)> callback)
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
    internal_->disconnect_lobby(internal_, lobbyId, cb.release(), wrapper);
}

Result LobbyManager::GetLobby(LobbyId lobbyId, Lobby* lobby)
{
    if (!lobby) {
        return Result::InternalError;
    }

    auto result = internal_->get_lobby(internal_, lobbyId, reinterpret_cast<DiscordLobby*>(lobby));
    return static_cast<Result>(result);
}

Result LobbyManager::GetLobbyActivitySecret(LobbyId lobbyId, char secret[128])
{
    if (!secret) {
        return Result::InternalError;
    }

    auto result = internal_->get_lobby_activity_secret(
      internal_, lobbyId, reinterpret_cast<DiscordLobbySecret*>(secret));
    return static_cast<Result>(result);
}

Result LobbyManager::GetLobbyMetadataValue(LobbyId lobbyId, MetadataKey key, char value[4096])
{
    if (!value) {
        return Result::InternalError;
    }

    auto result = internal_->get_lobby_metadata_value(
      internal_, lobbyId, const_cast<char*>(key), reinterpret_cast<DiscordMetadataValue*>(value));
    return static_cast<Result>(result);
}

Result LobbyManager::GetLobbyMetadataKey(LobbyId lobbyId, std::int32_t index, char key[256])
{
    if (!key) {
        return Result::InternalError;
    }

    auto result = internal_->get_lobby_metadata_key(
      internal_, lobbyId, index, reinterpret_cast<DiscordMetadataKey*>(key));
    return static_cast<Result>(result);
}

Result LobbyManager::LobbyMetadataCount(LobbyId lobbyId, std::int32_t* count)
{
    if (!count) {
        return Result::InternalError;
    }

    auto result =
      internal_->lobby_metadata_count(internal_, lobbyId, reinterpret_cast<int32_t*>(count));
    return static_cast<Result>(result);
}

Result LobbyManager::MemberCount(LobbyId lobbyId, std::int32_t* count)
{
    if (!count) {
        return Result::InternalError;
    }

    auto result = internal_->member_count(internal_, lobbyId, reinterpret_cast<int32_t*>(count));
    return static_cast<Result>(result);
}

Result LobbyManager::GetMemberUserId(LobbyId lobbyId, std::int32_t index, UserId* userId)
{
    if (!userId) {
        return Result::InternalError;
    }

    auto result =
      internal_->get_member_user_id(internal_, lobbyId, index, reinterpret_cast<int64_t*>(userId));
    return static_cast<Result>(result);
}

Result LobbyManager::GetMemberUser(LobbyId lobbyId, UserId userId, User* user)
{
    if (!user) {
        return Result::InternalError;
    }

    auto result =
      internal_->get_member_user(internal_, lobbyId, userId, reinterpret_cast<DiscordUser*>(user));
    return static_cast<Result>(result);
}

Result LobbyManager::GetMemberMetadataValue(LobbyId lobbyId,
                                            UserId userId,
                                            MetadataKey key,
                                            char value[4096])
{
    if (!value) {
        return Result::InternalError;
    }

    auto result =
      internal_->get_member_metadata_value(internal_,
                                           lobbyId,
                                           userId,
                                           const_cast<char*>(key),
                                           reinterpret_cast<DiscordMetadataValue*>(value));
    return static_cast<Result>(result);
}

Result LobbyManager::GetMemberMetadataKey(LobbyId lobbyId,
                                          UserId userId,
                                          std::int32_t index,
                                          char key[256])
{
    if (!key) {
        return Result::InternalError;
    }

    auto result = internal_->get_member_metadata_key(
      internal_, lobbyId, userId, index, reinterpret_cast<DiscordMetadataKey*>(key));
    return static_cast<Result>(result);
}

Result LobbyManager::MemberMetadataCount(LobbyId lobbyId, UserId userId, std::int32_t* count)
{
    if (!count) {
        return Result::InternalError;
    }

    auto result = internal_->member_metadata_count(
      internal_, lobbyId, userId, reinterpret_cast<int32_t*>(count));
    return static_cast<Result>(result);
}

void LobbyManager::UpdateMember(LobbyId lobbyId,
                                UserId userId,
                                LobbyMemberTransaction const& transaction,
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
    internal_->update_member(internal_,
                             lobbyId,
                             userId,
                             const_cast<LobbyMemberTransaction&>(transaction).Internal(),
                             cb.release(),
                             wrapper);
}

void LobbyManager::SendLobbyMessage(LobbyId lobbyId,
                                    std::uint8_t* data,
                                    std::uint32_t dataLength,
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
    internal_->send_lobby_message(
      internal_, lobbyId, reinterpret_cast<uint8_t*>(data), dataLength, cb.release(), wrapper);
}

Result LobbyManager::GetSearchQuery(LobbySearchQuery* query)
{
    if (!query) {
        return Result::InternalError;
    }

    auto result = internal_->get_search_query(internal_, query->Receive());
    return static_cast<Result>(result);
}

void LobbyManager::Search(LobbySearchQuery const& query, std::function<void(Result)> callback)
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
    internal_->search(
      internal_, const_cast<LobbySearchQuery&>(query).Internal(), cb.release(), wrapper);
}

void LobbyManager::LobbyCount(std::int32_t* count)
{
    if (!count) {
        return;
    }

    internal_->lobby_count(internal_, reinterpret_cast<int32_t*>(count));
}

Result LobbyManager::GetLobbyId(std::int32_t index, LobbyId* lobbyId)
{
    if (!lobbyId) {
        return Result::InternalError;
    }

    auto result = internal_->get_lobby_id(internal_, index, reinterpret_cast<int64_t*>(lobbyId));
    return static_cast<Result>(result);
}

void LobbyManager::ConnectVoice(LobbyId lobbyId, std::function<void(Result)> callback)
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
    internal_->connect_voice(internal_, lobbyId, cb.release(), wrapper);
}

void LobbyManager::DisconnectVoice(LobbyId lobbyId, std::function<void(Result)> callback)
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
    internal_->disconnect_voice(internal_, lobbyId, cb.release(), wrapper);
}

Result LobbyManager::ConnectNetwork(LobbyId lobbyId)
{
    auto result = internal_->connect_network(internal_, lobbyId);
    return static_cast<Result>(result);
}

Result LobbyManager::DisconnectNetwork(LobbyId lobbyId)
{
    auto result = internal_->disconnect_network(internal_, lobbyId);
    return static_cast<Result>(result);
}

Result LobbyManager::FlushNetwork()
{
    auto result = internal_->flush_network(internal_);
    return static_cast<Result>(result);
}

Result LobbyManager::OpenNetworkChannel(LobbyId lobbyId, std::uint8_t channelId, bool reliable)
{
    auto result =
      internal_->open_network_channel(internal_, lobbyId, channelId, (reliable ? 1 : 0));
    return static_cast<Result>(result);
}

Result LobbyManager::SendNetworkMessage(LobbyId lobbyId,
                                        UserId userId,
                                        std::uint8_t channelId,
                                        std::uint8_t* data,
                                        std::uint32_t dataLength)
{
    auto result = internal_->send_network_message(
      internal_, lobbyId, userId, channelId, reinterpret_cast<uint8_t*>(data), dataLength);
    return static_cast<Result>(result);
}

} // namespace discord
