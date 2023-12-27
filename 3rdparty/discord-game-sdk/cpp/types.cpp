#if !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "types.h"

#include <cstring>
#include <memory>

namespace discord {

void User::SetId(UserId id)
{
    internal_.id = id;
}

UserId User::GetId() const
{
    return internal_.id;
}

void User::SetUsername(char const* username)
{
    strncpy(internal_.username, username, 256);
    internal_.username[256 - 1] = '\0';
}

char const* User::GetUsername() const
{
    return internal_.username;
}

void User::SetDiscriminator(char const* discriminator)
{
    strncpy(internal_.discriminator, discriminator, 8);
    internal_.discriminator[8 - 1] = '\0';
}

char const* User::GetDiscriminator() const
{
    return internal_.discriminator;
}

void User::SetAvatar(char const* avatar)
{
    strncpy(internal_.avatar, avatar, 128);
    internal_.avatar[128 - 1] = '\0';
}

char const* User::GetAvatar() const
{
    return internal_.avatar;
}

void User::SetBot(bool bot)
{
    internal_.bot = bot;
}

bool User::GetBot() const
{
    return internal_.bot != 0;
}

void OAuth2Token::SetAccessToken(char const* accessToken)
{
    strncpy(internal_.access_token, accessToken, 128);
    internal_.access_token[128 - 1] = '\0';
}

char const* OAuth2Token::GetAccessToken() const
{
    return internal_.access_token;
}

void OAuth2Token::SetScopes(char const* scopes)
{
    strncpy(internal_.scopes, scopes, 1024);
    internal_.scopes[1024 - 1] = '\0';
}

char const* OAuth2Token::GetScopes() const
{
    return internal_.scopes;
}

void OAuth2Token::SetExpires(Timestamp expires)
{
    internal_.expires = expires;
}

Timestamp OAuth2Token::GetExpires() const
{
    return internal_.expires;
}

void ImageHandle::SetType(ImageType type)
{
    internal_.type = static_cast<EDiscordImageType>(type);
}

ImageType ImageHandle::GetType() const
{
    return static_cast<ImageType>(internal_.type);
}

void ImageHandle::SetId(std::int64_t id)
{
    internal_.id = id;
}

std::int64_t ImageHandle::GetId() const
{
    return internal_.id;
}

void ImageHandle::SetSize(std::uint32_t size)
{
    internal_.size = size;
}

std::uint32_t ImageHandle::GetSize() const
{
    return internal_.size;
}

void ImageDimensions::SetWidth(std::uint32_t width)
{
    internal_.width = width;
}

std::uint32_t ImageDimensions::GetWidth() const
{
    return internal_.width;
}

void ImageDimensions::SetHeight(std::uint32_t height)
{
    internal_.height = height;
}

std::uint32_t ImageDimensions::GetHeight() const
{
    return internal_.height;
}

void ActivityTimestamps::SetStart(Timestamp start)
{
    internal_.start = start;
}

Timestamp ActivityTimestamps::GetStart() const
{
    return internal_.start;
}

void ActivityTimestamps::SetEnd(Timestamp end)
{
    internal_.end = end;
}

Timestamp ActivityTimestamps::GetEnd() const
{
    return internal_.end;
}

void ActivityAssets::SetLargeImage(char const* largeImage)
{
    strncpy(internal_.large_image, largeImage, 128);
    internal_.large_image[128 - 1] = '\0';
}

char const* ActivityAssets::GetLargeImage() const
{
    return internal_.large_image;
}

void ActivityAssets::SetLargeText(char const* largeText)
{
    strncpy(internal_.large_text, largeText, 128);
    internal_.large_text[128 - 1] = '\0';
}

char const* ActivityAssets::GetLargeText() const
{
    return internal_.large_text;
}

void ActivityAssets::SetSmallImage(char const* smallImage)
{
    strncpy(internal_.small_image, smallImage, 128);
    internal_.small_image[128 - 1] = '\0';
}

char const* ActivityAssets::GetSmallImage() const
{
    return internal_.small_image;
}

void ActivityAssets::SetSmallText(char const* smallText)
{
    strncpy(internal_.small_text, smallText, 128);
    internal_.small_text[128 - 1] = '\0';
}

char const* ActivityAssets::GetSmallText() const
{
    return internal_.small_text;
}

void PartySize::SetCurrentSize(std::int32_t currentSize)
{
    internal_.current_size = currentSize;
}

std::int32_t PartySize::GetCurrentSize() const
{
    return internal_.current_size;
}

void PartySize::SetMaxSize(std::int32_t maxSize)
{
    internal_.max_size = maxSize;
}

std::int32_t PartySize::GetMaxSize() const
{
    return internal_.max_size;
}

void ActivityParty::SetId(char const* id)
{
    strncpy(internal_.id, id, 128);
    internal_.id[128 - 1] = '\0';
}

char const* ActivityParty::GetId() const
{
    return internal_.id;
}

PartySize& ActivityParty::GetSize()
{
    return reinterpret_cast<PartySize&>(internal_.size);
}

PartySize const& ActivityParty::GetSize() const
{
    return reinterpret_cast<PartySize const&>(internal_.size);
}

void ActivityParty::SetPrivacy(ActivityPartyPrivacy privacy)
{
    internal_.privacy = static_cast<EDiscordActivityPartyPrivacy>(privacy);
}

ActivityPartyPrivacy ActivityParty::GetPrivacy() const
{
    return static_cast<ActivityPartyPrivacy>(internal_.privacy);
}

void ActivitySecrets::SetMatch(char const* match)
{
    strncpy(internal_.match, match, 128);
    internal_.match[128 - 1] = '\0';
}

char const* ActivitySecrets::GetMatch() const
{
    return internal_.match;
}

void ActivitySecrets::SetJoin(char const* join)
{
    strncpy(internal_.join, join, 128);
    internal_.join[128 - 1] = '\0';
}

char const* ActivitySecrets::GetJoin() const
{
    return internal_.join;
}

void ActivitySecrets::SetSpectate(char const* spectate)
{
    strncpy(internal_.spectate, spectate, 128);
    internal_.spectate[128 - 1] = '\0';
}

char const* ActivitySecrets::GetSpectate() const
{
    return internal_.spectate;
}

void Activity::SetType(ActivityType type)
{
    internal_.type = static_cast<EDiscordActivityType>(type);
}

ActivityType Activity::GetType() const
{
    return static_cast<ActivityType>(internal_.type);
}

void Activity::SetApplicationId(std::int64_t applicationId)
{
    internal_.application_id = applicationId;
}

std::int64_t Activity::GetApplicationId() const
{
    return internal_.application_id;
}

void Activity::SetName(char const* name)
{
    strncpy(internal_.name, name, 128);
    internal_.name[128 - 1] = '\0';
}

char const* Activity::GetName() const
{
    return internal_.name;
}

void Activity::SetState(char const* state)
{
    strncpy(internal_.state, state, 128);
    internal_.state[128 - 1] = '\0';
}

char const* Activity::GetState() const
{
    return internal_.state;
}

void Activity::SetDetails(char const* details)
{
    strncpy(internal_.details, details, 128);
    internal_.details[128 - 1] = '\0';
}

char const* Activity::GetDetails() const
{
    return internal_.details;
}

ActivityTimestamps& Activity::GetTimestamps()
{
    return reinterpret_cast<ActivityTimestamps&>(internal_.timestamps);
}

ActivityTimestamps const& Activity::GetTimestamps() const
{
    return reinterpret_cast<ActivityTimestamps const&>(internal_.timestamps);
}

ActivityAssets& Activity::GetAssets()
{
    return reinterpret_cast<ActivityAssets&>(internal_.assets);
}

ActivityAssets const& Activity::GetAssets() const
{
    return reinterpret_cast<ActivityAssets const&>(internal_.assets);
}

ActivityParty& Activity::GetParty()
{
    return reinterpret_cast<ActivityParty&>(internal_.party);
}

ActivityParty const& Activity::GetParty() const
{
    return reinterpret_cast<ActivityParty const&>(internal_.party);
}

ActivitySecrets& Activity::GetSecrets()
{
    return reinterpret_cast<ActivitySecrets&>(internal_.secrets);
}

ActivitySecrets const& Activity::GetSecrets() const
{
    return reinterpret_cast<ActivitySecrets const&>(internal_.secrets);
}

void Activity::SetInstance(bool instance)
{
    internal_.instance = instance;
}

bool Activity::GetInstance() const
{
    return internal_.instance != 0;
}

void Activity::SetSupportedPlatforms(std::uint32_t supportedPlatforms)
{
    internal_.supported_platforms = supportedPlatforms;
}

std::uint32_t Activity::GetSupportedPlatforms() const
{
    return internal_.supported_platforms;
}

void Presence::SetStatus(Status status)
{
    internal_.status = static_cast<EDiscordStatus>(status);
}

Status Presence::GetStatus() const
{
    return static_cast<Status>(internal_.status);
}

Activity& Presence::GetActivity()
{
    return reinterpret_cast<Activity&>(internal_.activity);
}

Activity const& Presence::GetActivity() const
{
    return reinterpret_cast<Activity const&>(internal_.activity);
}

void Relationship::SetType(RelationshipType type)
{
    internal_.type = static_cast<EDiscordRelationshipType>(type);
}

RelationshipType Relationship::GetType() const
{
    return static_cast<RelationshipType>(internal_.type);
}

User& Relationship::GetUser()
{
    return reinterpret_cast<User&>(internal_.user);
}

User const& Relationship::GetUser() const
{
    return reinterpret_cast<User const&>(internal_.user);
}

Presence& Relationship::GetPresence()
{
    return reinterpret_cast<Presence&>(internal_.presence);
}

Presence const& Relationship::GetPresence() const
{
    return reinterpret_cast<Presence const&>(internal_.presence);
}

void Lobby::SetId(LobbyId id)
{
    internal_.id = id;
}

LobbyId Lobby::GetId() const
{
    return internal_.id;
}

void Lobby::SetType(LobbyType type)
{
    internal_.type = static_cast<EDiscordLobbyType>(type);
}

LobbyType Lobby::GetType() const
{
    return static_cast<LobbyType>(internal_.type);
}

void Lobby::SetOwnerId(UserId ownerId)
{
    internal_.owner_id = ownerId;
}

UserId Lobby::GetOwnerId() const
{
    return internal_.owner_id;
}

void Lobby::SetSecret(LobbySecret secret)
{
    strncpy(internal_.secret, secret, 128);
    internal_.secret[128 - 1] = '\0';
}

LobbySecret Lobby::GetSecret() const
{
    return internal_.secret;
}

void Lobby::SetCapacity(std::uint32_t capacity)
{
    internal_.capacity = capacity;
}

std::uint32_t Lobby::GetCapacity() const
{
    return internal_.capacity;
}

void Lobby::SetLocked(bool locked)
{
    internal_.locked = locked;
}

bool Lobby::GetLocked() const
{
    return internal_.locked != 0;
}

void ImeUnderline::SetFrom(std::int32_t from)
{
    internal_.from = from;
}

std::int32_t ImeUnderline::GetFrom() const
{
    return internal_.from;
}

void ImeUnderline::SetTo(std::int32_t to)
{
    internal_.to = to;
}

std::int32_t ImeUnderline::GetTo() const
{
    return internal_.to;
}

void ImeUnderline::SetColor(std::uint32_t color)
{
    internal_.color = color;
}

std::uint32_t ImeUnderline::GetColor() const
{
    return internal_.color;
}

void ImeUnderline::SetBackgroundColor(std::uint32_t backgroundColor)
{
    internal_.background_color = backgroundColor;
}

std::uint32_t ImeUnderline::GetBackgroundColor() const
{
    return internal_.background_color;
}

void ImeUnderline::SetThick(bool thick)
{
    internal_.thick = thick;
}

bool ImeUnderline::GetThick() const
{
    return internal_.thick != 0;
}

void Rect::SetLeft(std::int32_t left)
{
    internal_.left = left;
}

std::int32_t Rect::GetLeft() const
{
    return internal_.left;
}

void Rect::SetTop(std::int32_t top)
{
    internal_.top = top;
}

std::int32_t Rect::GetTop() const
{
    return internal_.top;
}

void Rect::SetRight(std::int32_t right)
{
    internal_.right = right;
}

std::int32_t Rect::GetRight() const
{
    return internal_.right;
}

void Rect::SetBottom(std::int32_t bottom)
{
    internal_.bottom = bottom;
}

std::int32_t Rect::GetBottom() const
{
    return internal_.bottom;
}

void FileStat::SetFilename(char const* filename)
{
    strncpy(internal_.filename, filename, 260);
    internal_.filename[260 - 1] = '\0';
}

char const* FileStat::GetFilename() const
{
    return internal_.filename;
}

void FileStat::SetSize(std::uint64_t size)
{
    internal_.size = size;
}

std::uint64_t FileStat::GetSize() const
{
    return internal_.size;
}

void FileStat::SetLastModified(std::uint64_t lastModified)
{
    internal_.last_modified = lastModified;
}

std::uint64_t FileStat::GetLastModified() const
{
    return internal_.last_modified;
}

void Entitlement::SetId(Snowflake id)
{
    internal_.id = id;
}

Snowflake Entitlement::GetId() const
{
    return internal_.id;
}

void Entitlement::SetType(EntitlementType type)
{
    internal_.type = static_cast<EDiscordEntitlementType>(type);
}

EntitlementType Entitlement::GetType() const
{
    return static_cast<EntitlementType>(internal_.type);
}

void Entitlement::SetSkuId(Snowflake skuId)
{
    internal_.sku_id = skuId;
}

Snowflake Entitlement::GetSkuId() const
{
    return internal_.sku_id;
}

void SkuPrice::SetAmount(std::uint32_t amount)
{
    internal_.amount = amount;
}

std::uint32_t SkuPrice::GetAmount() const
{
    return internal_.amount;
}

void SkuPrice::SetCurrency(char const* currency)
{
    strncpy(internal_.currency, currency, 16);
    internal_.currency[16 - 1] = '\0';
}

char const* SkuPrice::GetCurrency() const
{
    return internal_.currency;
}

void Sku::SetId(Snowflake id)
{
    internal_.id = id;
}

Snowflake Sku::GetId() const
{
    return internal_.id;
}

void Sku::SetType(SkuType type)
{
    internal_.type = static_cast<EDiscordSkuType>(type);
}

SkuType Sku::GetType() const
{
    return static_cast<SkuType>(internal_.type);
}

void Sku::SetName(char const* name)
{
    strncpy(internal_.name, name, 256);
    internal_.name[256 - 1] = '\0';
}

char const* Sku::GetName() const
{
    return internal_.name;
}

SkuPrice& Sku::GetPrice()
{
    return reinterpret_cast<SkuPrice&>(internal_.price);
}

SkuPrice const& Sku::GetPrice() const
{
    return reinterpret_cast<SkuPrice const&>(internal_.price);
}

void InputMode::SetType(InputModeType type)
{
    internal_.type = static_cast<EDiscordInputModeType>(type);
}

InputModeType InputMode::GetType() const
{
    return static_cast<InputModeType>(internal_.type);
}

void InputMode::SetShortcut(char const* shortcut)
{
    strncpy(internal_.shortcut, shortcut, 256);
    internal_.shortcut[256 - 1] = '\0';
}

char const* InputMode::GetShortcut() const
{
    return internal_.shortcut;
}

void UserAchievement::SetUserId(Snowflake userId)
{
    internal_.user_id = userId;
}

Snowflake UserAchievement::GetUserId() const
{
    return internal_.user_id;
}

void UserAchievement::SetAchievementId(Snowflake achievementId)
{
    internal_.achievement_id = achievementId;
}

Snowflake UserAchievement::GetAchievementId() const
{
    return internal_.achievement_id;
}

void UserAchievement::SetPercentComplete(std::uint8_t percentComplete)
{
    internal_.percent_complete = percentComplete;
}

std::uint8_t UserAchievement::GetPercentComplete() const
{
    return internal_.percent_complete;
}

void UserAchievement::SetUnlockedAt(DateTime unlockedAt)
{
    strncpy(internal_.unlocked_at, unlockedAt, 64);
    internal_.unlocked_at[64 - 1] = '\0';
}

DateTime UserAchievement::GetUnlockedAt() const
{
    return internal_.unlocked_at;
}

Result LobbyTransaction::SetType(LobbyType type)
{
    auto result = internal_->set_type(internal_, static_cast<EDiscordLobbyType>(type));
    return static_cast<Result>(result);
}

Result LobbyTransaction::SetOwner(UserId ownerId)
{
    auto result = internal_->set_owner(internal_, ownerId);
    return static_cast<Result>(result);
}

Result LobbyTransaction::SetCapacity(std::uint32_t capacity)
{
    auto result = internal_->set_capacity(internal_, capacity);
    return static_cast<Result>(result);
}

Result LobbyTransaction::SetMetadata(MetadataKey key, MetadataValue value)
{
    auto result =
      internal_->set_metadata(internal_, const_cast<char*>(key), const_cast<char*>(value));
    return static_cast<Result>(result);
}

Result LobbyTransaction::DeleteMetadata(MetadataKey key)
{
    auto result = internal_->delete_metadata(internal_, const_cast<char*>(key));
    return static_cast<Result>(result);
}

Result LobbyTransaction::SetLocked(bool locked)
{
    auto result = internal_->set_locked(internal_, (locked ? 1 : 0));
    return static_cast<Result>(result);
}

Result LobbyMemberTransaction::SetMetadata(MetadataKey key, MetadataValue value)
{
    auto result =
      internal_->set_metadata(internal_, const_cast<char*>(key), const_cast<char*>(value));
    return static_cast<Result>(result);
}

Result LobbyMemberTransaction::DeleteMetadata(MetadataKey key)
{
    auto result = internal_->delete_metadata(internal_, const_cast<char*>(key));
    return static_cast<Result>(result);
}

Result LobbySearchQuery::Filter(MetadataKey key,
                                LobbySearchComparison comparison,
                                LobbySearchCast cast,
                                MetadataValue value)
{
    auto result = internal_->filter(internal_,
                                    const_cast<char*>(key),
                                    static_cast<EDiscordLobbySearchComparison>(comparison),
                                    static_cast<EDiscordLobbySearchCast>(cast),
                                    const_cast<char*>(value));
    return static_cast<Result>(result);
}

Result LobbySearchQuery::Sort(MetadataKey key, LobbySearchCast cast, MetadataValue value)
{
    auto result = internal_->sort(internal_,
                                  const_cast<char*>(key),
                                  static_cast<EDiscordLobbySearchCast>(cast),
                                  const_cast<char*>(value));
    return static_cast<Result>(result);
}

Result LobbySearchQuery::Limit(std::uint32_t limit)
{
    auto result = internal_->limit(internal_, limit);
    return static_cast<Result>(result);
}

Result LobbySearchQuery::Distance(LobbySearchDistance distance)
{
    auto result =
      internal_->distance(internal_, static_cast<EDiscordLobbySearchDistance>(distance));
    return static_cast<Result>(result);
}

} // namespace discord
