#if !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "network_manager.h"

#include "core.h"

#include <cstring>
#include <memory>

namespace discord {

class NetworkEvents final {
public:
    static void DISCORD_CALLBACK OnMessage(void* callbackData,
                                           DiscordNetworkPeerId peerId,
                                           DiscordNetworkChannelId channelId,
                                           uint8_t* data,
                                           uint32_t dataLength)
    {
        auto* core = reinterpret_cast<Core*>(callbackData);
        if (!core) {
            return;
        }

        auto& module = core->NetworkManager();
        module.OnMessage(peerId, channelId, data, dataLength);
    }

    static void DISCORD_CALLBACK OnRouteUpdate(void* callbackData, char const* routeData)
    {
        auto* core = reinterpret_cast<Core*>(callbackData);
        if (!core) {
            return;
        }

        auto& module = core->NetworkManager();
        module.OnRouteUpdate(static_cast<const char*>(routeData));
    }
};

IDiscordNetworkEvents NetworkManager::events_{
  &NetworkEvents::OnMessage,
  &NetworkEvents::OnRouteUpdate,
};

void NetworkManager::GetPeerId(NetworkPeerId* peerId)
{
    if (!peerId) {
        return;
    }

    internal_->get_peer_id(internal_, reinterpret_cast<uint64_t*>(peerId));
}

Result NetworkManager::Flush()
{
    auto result = internal_->flush(internal_);
    return static_cast<Result>(result);
}

Result NetworkManager::OpenPeer(NetworkPeerId peerId, char const* routeData)
{
    auto result = internal_->open_peer(internal_, peerId, const_cast<char*>(routeData));
    return static_cast<Result>(result);
}

Result NetworkManager::UpdatePeer(NetworkPeerId peerId, char const* routeData)
{
    auto result = internal_->update_peer(internal_, peerId, const_cast<char*>(routeData));
    return static_cast<Result>(result);
}

Result NetworkManager::ClosePeer(NetworkPeerId peerId)
{
    auto result = internal_->close_peer(internal_, peerId);
    return static_cast<Result>(result);
}

Result NetworkManager::OpenChannel(NetworkPeerId peerId, NetworkChannelId channelId, bool reliable)
{
    auto result = internal_->open_channel(internal_, peerId, channelId, (reliable ? 1 : 0));
    return static_cast<Result>(result);
}

Result NetworkManager::CloseChannel(NetworkPeerId peerId, NetworkChannelId channelId)
{
    auto result = internal_->close_channel(internal_, peerId, channelId);
    return static_cast<Result>(result);
}

Result NetworkManager::SendMessage(NetworkPeerId peerId,
                                   NetworkChannelId channelId,
                                   std::uint8_t* data,
                                   std::uint32_t dataLength)
{
    auto result = internal_->send_message(
      internal_, peerId, channelId, reinterpret_cast<uint8_t*>(data), dataLength);
    return static_cast<Result>(result);
}

} // namespace discord
