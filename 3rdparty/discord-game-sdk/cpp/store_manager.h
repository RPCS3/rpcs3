#pragma once

#include "types.h"

namespace discord {

class StoreManager final {
public:
    ~StoreManager() = default;

    void FetchSkus(std::function<void(Result)> callback);
    void CountSkus(std::int32_t* count);
    Result GetSku(Snowflake skuId, Sku* sku);
    Result GetSkuAt(std::int32_t index, Sku* sku);
    void FetchEntitlements(std::function<void(Result)> callback);
    void CountEntitlements(std::int32_t* count);
    Result GetEntitlement(Snowflake entitlementId, Entitlement* entitlement);
    Result GetEntitlementAt(std::int32_t index, Entitlement* entitlement);
    Result HasSkuEntitlement(Snowflake skuId, bool* hasEntitlement);
    void StartPurchase(Snowflake skuId, std::function<void(Result)> callback);

    Event<Entitlement const&> OnEntitlementCreate;
    Event<Entitlement const&> OnEntitlementDelete;

private:
    friend class Core;

    StoreManager() = default;
    StoreManager(StoreManager const& rhs) = delete;
    StoreManager& operator=(StoreManager const& rhs) = delete;
    StoreManager(StoreManager&& rhs) = delete;
    StoreManager& operator=(StoreManager&& rhs) = delete;

    IDiscordStoreManager* internal_;
    static IDiscordStoreEvents events_;
};

} // namespace discord
