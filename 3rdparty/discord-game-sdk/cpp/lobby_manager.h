#pragma once

#include "types.h"

namespace discord {

class LobbyManager final {
public:
    ~LobbyManager() = default;

    Result GetLobbyCreateTransaction(LobbyTransaction* transaction);
    Result GetLobbyUpdateTransaction(LobbyId lobbyId, LobbyTransaction* transaction);
    Result GetMemberUpdateTransaction(LobbyId lobbyId,
                                      UserId userId,
                                      LobbyMemberTransaction* transaction);
    void CreateLobby(LobbyTransaction const& transaction,
                     std::function<void(Result, Lobby const&)> callback);
    void UpdateLobby(LobbyId lobbyId,
                     LobbyTransaction const& transaction,
                     std::function<void(Result)> callback);
    void DeleteLobby(LobbyId lobbyId, std::function<void(Result)> callback);
    void ConnectLobby(LobbyId lobbyId,
                      LobbySecret secret,
                      std::function<void(Result, Lobby const&)> callback);
    void ConnectLobbyWithActivitySecret(LobbySecret activitySecret,
                                        std::function<void(Result, Lobby const&)> callback);
    void DisconnectLobby(LobbyId lobbyId, std::function<void(Result)> callback);
    Result GetLobby(LobbyId lobbyId, Lobby* lobby);
    Result GetLobbyActivitySecret(LobbyId lobbyId, char secret[128]);
    Result GetLobbyMetadataValue(LobbyId lobbyId, MetadataKey key, char value[4096]);
    Result GetLobbyMetadataKey(LobbyId lobbyId, std::int32_t index, char key[256]);
    Result LobbyMetadataCount(LobbyId lobbyId, std::int32_t* count);
    Result MemberCount(LobbyId lobbyId, std::int32_t* count);
    Result GetMemberUserId(LobbyId lobbyId, std::int32_t index, UserId* userId);
    Result GetMemberUser(LobbyId lobbyId, UserId userId, User* user);
    Result GetMemberMetadataValue(LobbyId lobbyId,
                                  UserId userId,
                                  MetadataKey key,
                                  char value[4096]);
    Result GetMemberMetadataKey(LobbyId lobbyId, UserId userId, std::int32_t index, char key[256]);
    Result MemberMetadataCount(LobbyId lobbyId, UserId userId, std::int32_t* count);
    void UpdateMember(LobbyId lobbyId,
                      UserId userId,
                      LobbyMemberTransaction const& transaction,
                      std::function<void(Result)> callback);
    void SendLobbyMessage(LobbyId lobbyId,
                          std::uint8_t* data,
                          std::uint32_t dataLength,
                          std::function<void(Result)> callback);
    Result GetSearchQuery(LobbySearchQuery* query);
    void Search(LobbySearchQuery const& query, std::function<void(Result)> callback);
    void LobbyCount(std::int32_t* count);
    Result GetLobbyId(std::int32_t index, LobbyId* lobbyId);
    void ConnectVoice(LobbyId lobbyId, std::function<void(Result)> callback);
    void DisconnectVoice(LobbyId lobbyId, std::function<void(Result)> callback);
    Result ConnectNetwork(LobbyId lobbyId);
    Result DisconnectNetwork(LobbyId lobbyId);
    Result FlushNetwork();
    Result OpenNetworkChannel(LobbyId lobbyId, std::uint8_t channelId, bool reliable);
    Result SendNetworkMessage(LobbyId lobbyId,
                              UserId userId,
                              std::uint8_t channelId,
                              std::uint8_t* data,
                              std::uint32_t dataLength);

    Event<std::int64_t> OnLobbyUpdate;
    Event<std::int64_t, std::uint32_t> OnLobbyDelete;
    Event<std::int64_t, std::int64_t> OnMemberConnect;
    Event<std::int64_t, std::int64_t> OnMemberUpdate;
    Event<std::int64_t, std::int64_t> OnMemberDisconnect;
    Event<std::int64_t, std::int64_t, std::uint8_t*, std::uint32_t> OnLobbyMessage;
    Event<std::int64_t, std::int64_t, bool> OnSpeaking;
    Event<std::int64_t, std::int64_t, std::uint8_t, std::uint8_t*, std::uint32_t> OnNetworkMessage;

private:
    friend class Core;

    LobbyManager() = default;
    LobbyManager(LobbyManager const& rhs) = delete;
    LobbyManager& operator=(LobbyManager const& rhs) = delete;
    LobbyManager(LobbyManager&& rhs) = delete;
    LobbyManager& operator=(LobbyManager&& rhs) = delete;

    IDiscordLobbyManager* internal_;
    static IDiscordLobbyEvents events_;
};

} // namespace discord
