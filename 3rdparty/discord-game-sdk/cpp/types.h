#pragma once

#include "ffi.h"
#include "event.h"
#ifdef _WIN32
#include <Windows.h>
#include <dxgi.h>
#endif

namespace discord {

enum class Result {
    Ok = 0,
    ServiceUnavailable = 1,
    InvalidVersion = 2,
    LockFailed = 3,
    InternalError = 4,
    InvalidPayload = 5,
    InvalidCommand = 6,
    InvalidPermissions = 7,
    NotFetched = 8,
    NotFound = 9,
    Conflict = 10,
    InvalidSecret = 11,
    InvalidJoinSecret = 12,
    NoEligibleActivity = 13,
    InvalidInvite = 14,
    NotAuthenticated = 15,
    InvalidAccessToken = 16,
    ApplicationMismatch = 17,
    InvalidDataUrl = 18,
    InvalidBase64 = 19,
    NotFiltered = 20,
    LobbyFull = 21,
    InvalidLobbySecret = 22,
    InvalidFilename = 23,
    InvalidFileSize = 24,
    InvalidEntitlement = 25,
    NotInstalled = 26,
    NotRunning = 27,
    InsufficientBuffer = 28,
    PurchaseCanceled = 29,
    InvalidGuild = 30,
    InvalidEvent = 31,
    InvalidChannel = 32,
    InvalidOrigin = 33,
    RateLimited = 34,
    OAuth2Error = 35,
    SelectChannelTimeout = 36,
    GetGuildTimeout = 37,
    SelectVoiceForceRequired = 38,
    CaptureShortcutAlreadyListening = 39,
    UnauthorizedForAchievement = 40,
    InvalidGiftCode = 41,
    PurchaseError = 42,
    TransactionAborted = 43,
    DrawingInitFailed = 44,
};

enum class CreateFlags {
    Default = 0,
    NoRequireDiscord = 1,
};

enum class LogLevel {
    Error = 1,
    Warn,
    Info,
    Debug,
};

enum class UserFlag {
    Partner = 2,
    HypeSquadEvents = 4,
    HypeSquadHouse1 = 64,
    HypeSquadHouse2 = 128,
    HypeSquadHouse3 = 256,
};

enum class PremiumType {
    None = 0,
    Tier1 = 1,
    Tier2 = 2,
};

enum class ImageType {
    User,
};

enum class ActivityPartyPrivacy {
    Private = 0,
    Public = 1,
};

enum class ActivityType {
    Playing,
    Streaming,
    Listening,
    Watching,
};

enum class ActivityActionType {
    Join = 1,
    Spectate,
};

enum class ActivitySupportedPlatformFlags {
    Desktop = 1,
    Android = 2,
    iOS = 4,
};

enum class ActivityJoinRequestReply {
    No,
    Yes,
    Ignore,
};

enum class Status {
    Offline = 0,
    Online = 1,
    Idle = 2,
    DoNotDisturb = 3,
};

enum class RelationshipType {
    None,
    Friend,
    Blocked,
    PendingIncoming,
    PendingOutgoing,
    Implicit,
};

enum class LobbyType {
    Private = 1,
    Public,
};

enum class LobbySearchComparison {
    LessThanOrEqual = -2,
    LessThan,
    Equal,
    GreaterThan,
    GreaterThanOrEqual,
    NotEqual,
};

enum class LobbySearchCast {
    String = 1,
    Number,
};

enum class LobbySearchDistance {
    Local,
    Default,
    Extended,
    Global,
};

enum class KeyVariant {
    Normal,
    Right,
    Left,
};

enum class MouseButton {
    Left,
    Middle,
    Right,
};

enum class EntitlementType {
    Purchase = 1,
    PremiumSubscription,
    DeveloperGift,
    TestModePurchase,
    FreePurchase,
    UserGift,
    PremiumPurchase,
};

enum class SkuType {
    Application = 1,
    DLC,
    Consumable,
    Bundle,
};

enum class InputModeType {
    VoiceActivity = 0,
    PushToTalk,
};

using ClientId = std::int64_t;
using Version = std::int32_t;
using Snowflake = std::int64_t;
using Timestamp = std::int64_t;
using UserId = Snowflake;
using Locale = char const*;
using Branch = char const*;
using LobbyId = Snowflake;
using LobbySecret = char const*;
using MetadataKey = char const*;
using MetadataValue = char const*;
using NetworkPeerId = std::uint64_t;
using NetworkChannelId = std::uint8_t;
#ifdef __APPLE__
using IDXGISwapChain = void;
#endif
#ifdef __linux__
using IDXGISwapChain = void;
#endif
#ifdef __APPLE__
using MSG = void;
#endif
#ifdef __linux__
using MSG = void;
#endif
using Path = char const*;
using DateTime = char const*;

class User final {
public:
    void SetId(UserId id);
    UserId GetId() const;
    void SetUsername(char const* username);
    char const* GetUsername() const;
    void SetDiscriminator(char const* discriminator);
    char const* GetDiscriminator() const;
    void SetAvatar(char const* avatar);
    char const* GetAvatar() const;
    void SetBot(bool bot);
    bool GetBot() const;

private:
    DiscordUser internal_;
};

class OAuth2Token final {
public:
    void SetAccessToken(char const* accessToken);
    char const* GetAccessToken() const;
    void SetScopes(char const* scopes);
    char const* GetScopes() const;
    void SetExpires(Timestamp expires);
    Timestamp GetExpires() const;

private:
    DiscordOAuth2Token internal_;
};

class ImageHandle final {
public:
    void SetType(ImageType type);
    ImageType GetType() const;
    void SetId(std::int64_t id);
    std::int64_t GetId() const;
    void SetSize(std::uint32_t size);
    std::uint32_t GetSize() const;

private:
    DiscordImageHandle internal_;
};

class ImageDimensions final {
public:
    void SetWidth(std::uint32_t width);
    std::uint32_t GetWidth() const;
    void SetHeight(std::uint32_t height);
    std::uint32_t GetHeight() const;

private:
    DiscordImageDimensions internal_;
};

class ActivityTimestamps final {
public:
    void SetStart(Timestamp start);
    Timestamp GetStart() const;
    void SetEnd(Timestamp end);
    Timestamp GetEnd() const;

private:
    DiscordActivityTimestamps internal_;
};

class ActivityAssets final {
public:
    void SetLargeImage(char const* largeImage);
    char const* GetLargeImage() const;
    void SetLargeText(char const* largeText);
    char const* GetLargeText() const;
    void SetSmallImage(char const* smallImage);
    char const* GetSmallImage() const;
    void SetSmallText(char const* smallText);
    char const* GetSmallText() const;

private:
    DiscordActivityAssets internal_;
};

class PartySize final {
public:
    void SetCurrentSize(std::int32_t currentSize);
    std::int32_t GetCurrentSize() const;
    void SetMaxSize(std::int32_t maxSize);
    std::int32_t GetMaxSize() const;

private:
    DiscordPartySize internal_;
};

class ActivityParty final {
public:
    void SetId(char const* id);
    char const* GetId() const;
    PartySize& GetSize();
    PartySize const& GetSize() const;
    void SetPrivacy(ActivityPartyPrivacy privacy);
    ActivityPartyPrivacy GetPrivacy() const;

private:
    DiscordActivityParty internal_;
};

class ActivitySecrets final {
public:
    void SetMatch(char const* match);
    char const* GetMatch() const;
    void SetJoin(char const* join);
    char const* GetJoin() const;
    void SetSpectate(char const* spectate);
    char const* GetSpectate() const;

private:
    DiscordActivitySecrets internal_;
};

class Activity final {
public:
    void SetType(ActivityType type);
    ActivityType GetType() const;
    void SetApplicationId(std::int64_t applicationId);
    std::int64_t GetApplicationId() const;
    void SetName(char const* name);
    char const* GetName() const;
    void SetState(char const* state);
    char const* GetState() const;
    void SetDetails(char const* details);
    char const* GetDetails() const;
    ActivityTimestamps& GetTimestamps();
    ActivityTimestamps const& GetTimestamps() const;
    ActivityAssets& GetAssets();
    ActivityAssets const& GetAssets() const;
    ActivityParty& GetParty();
    ActivityParty const& GetParty() const;
    ActivitySecrets& GetSecrets();
    ActivitySecrets const& GetSecrets() const;
    void SetInstance(bool instance);
    bool GetInstance() const;
    void SetSupportedPlatforms(std::uint32_t supportedPlatforms);
    std::uint32_t GetSupportedPlatforms() const;

private:
    DiscordActivity internal_;
};

class Presence final {
public:
    void SetStatus(Status status);
    Status GetStatus() const;
    Activity& GetActivity();
    Activity const& GetActivity() const;

private:
    DiscordPresence internal_;
};

class Relationship final {
public:
    void SetType(RelationshipType type);
    RelationshipType GetType() const;
    User& GetUser();
    User const& GetUser() const;
    Presence& GetPresence();
    Presence const& GetPresence() const;

private:
    DiscordRelationship internal_;
};

class Lobby final {
public:
    void SetId(LobbyId id);
    LobbyId GetId() const;
    void SetType(LobbyType type);
    LobbyType GetType() const;
    void SetOwnerId(UserId ownerId);
    UserId GetOwnerId() const;
    void SetSecret(LobbySecret secret);
    LobbySecret GetSecret() const;
    void SetCapacity(std::uint32_t capacity);
    std::uint32_t GetCapacity() const;
    void SetLocked(bool locked);
    bool GetLocked() const;

private:
    DiscordLobby internal_;
};

class ImeUnderline final {
public:
    void SetFrom(std::int32_t from);
    std::int32_t GetFrom() const;
    void SetTo(std::int32_t to);
    std::int32_t GetTo() const;
    void SetColor(std::uint32_t color);
    std::uint32_t GetColor() const;
    void SetBackgroundColor(std::uint32_t backgroundColor);
    std::uint32_t GetBackgroundColor() const;
    void SetThick(bool thick);
    bool GetThick() const;

private:
    DiscordImeUnderline internal_;
};

class Rect final {
public:
    void SetLeft(std::int32_t left);
    std::int32_t GetLeft() const;
    void SetTop(std::int32_t top);
    std::int32_t GetTop() const;
    void SetRight(std::int32_t right);
    std::int32_t GetRight() const;
    void SetBottom(std::int32_t bottom);
    std::int32_t GetBottom() const;

private:
    DiscordRect internal_;
};

class FileStat final {
public:
    void SetFilename(char const* filename);
    char const* GetFilename() const;
    void SetSize(std::uint64_t size);
    std::uint64_t GetSize() const;
    void SetLastModified(std::uint64_t lastModified);
    std::uint64_t GetLastModified() const;

private:
    DiscordFileStat internal_;
};

class Entitlement final {
public:
    void SetId(Snowflake id);
    Snowflake GetId() const;
    void SetType(EntitlementType type);
    EntitlementType GetType() const;
    void SetSkuId(Snowflake skuId);
    Snowflake GetSkuId() const;

private:
    DiscordEntitlement internal_;
};

class SkuPrice final {
public:
    void SetAmount(std::uint32_t amount);
    std::uint32_t GetAmount() const;
    void SetCurrency(char const* currency);
    char const* GetCurrency() const;

private:
    DiscordSkuPrice internal_;
};

class Sku final {
public:
    void SetId(Snowflake id);
    Snowflake GetId() const;
    void SetType(SkuType type);
    SkuType GetType() const;
    void SetName(char const* name);
    char const* GetName() const;
    SkuPrice& GetPrice();
    SkuPrice const& GetPrice() const;

private:
    DiscordSku internal_;
};

class InputMode final {
public:
    void SetType(InputModeType type);
    InputModeType GetType() const;
    void SetShortcut(char const* shortcut);
    char const* GetShortcut() const;

private:
    DiscordInputMode internal_;
};

class UserAchievement final {
public:
    void SetUserId(Snowflake userId);
    Snowflake GetUserId() const;
    void SetAchievementId(Snowflake achievementId);
    Snowflake GetAchievementId() const;
    void SetPercentComplete(std::uint8_t percentComplete);
    std::uint8_t GetPercentComplete() const;
    void SetUnlockedAt(DateTime unlockedAt);
    DateTime GetUnlockedAt() const;

private:
    DiscordUserAchievement internal_;
};

class LobbyTransaction final {
public:
    Result SetType(LobbyType type);
    Result SetOwner(UserId ownerId);
    Result SetCapacity(std::uint32_t capacity);
    Result SetMetadata(MetadataKey key, MetadataValue value);
    Result DeleteMetadata(MetadataKey key);
    Result SetLocked(bool locked);

    IDiscordLobbyTransaction** Receive() { return &internal_; }
    IDiscordLobbyTransaction* Internal() { return internal_; }

private:
    IDiscordLobbyTransaction* internal_;
};

class LobbyMemberTransaction final {
public:
    Result SetMetadata(MetadataKey key, MetadataValue value);
    Result DeleteMetadata(MetadataKey key);

    IDiscordLobbyMemberTransaction** Receive() { return &internal_; }
    IDiscordLobbyMemberTransaction* Internal() { return internal_; }

private:
    IDiscordLobbyMemberTransaction* internal_;
};

class LobbySearchQuery final {
public:
    Result Filter(MetadataKey key,
                  LobbySearchComparison comparison,
                  LobbySearchCast cast,
                  MetadataValue value);
    Result Sort(MetadataKey key, LobbySearchCast cast, MetadataValue value);
    Result Limit(std::uint32_t limit);
    Result Distance(LobbySearchDistance distance);

    IDiscordLobbySearchQuery** Receive() { return &internal_; }
    IDiscordLobbySearchQuery* Internal() { return internal_; }

private:
    IDiscordLobbySearchQuery* internal_;
};

} // namespace discord
