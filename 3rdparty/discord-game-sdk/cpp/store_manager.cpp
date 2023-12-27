#if !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "store_manager.h"

#include "core.h"

#include <cstring>
#include <memory>

namespace discord {

class StoreEvents final {
public:
    static void DISCORD_CALLBACK OnEntitlementCreate(void* callbackData,
                                                     DiscordEntitlement* entitlement)
    {
        auto* core = reinterpret_cast<Core*>(callbackData);
        if (!core) {
            return;
        }

        auto& module = core->StoreManager();
        module.OnEntitlementCreate(*reinterpret_cast<Entitlement const*>(entitlement));
    }

    static void DISCORD_CALLBACK OnEntitlementDelete(void* callbackData,
                                                     DiscordEntitlement* entitlement)
    {
        auto* core = reinterpret_cast<Core*>(callbackData);
        if (!core) {
            return;
        }

        auto& module = core->StoreManager();
        module.OnEntitlementDelete(*reinterpret_cast<Entitlement const*>(entitlement));
    }
};

IDiscordStoreEvents StoreManager::events_{
  &StoreEvents::OnEntitlementCreate,
  &StoreEvents::OnEntitlementDelete,
};

void StoreManager::FetchSkus(std::function<void(Result)> callback)
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
    internal_->fetch_skus(internal_, cb.release(), wrapper);
}

void StoreManager::CountSkus(std::int32_t* count)
{
    if (!count) {
        return;
    }

    internal_->count_skus(internal_, reinterpret_cast<int32_t*>(count));
}

Result StoreManager::GetSku(Snowflake skuId, Sku* sku)
{
    if (!sku) {
        return Result::InternalError;
    }

    auto result = internal_->get_sku(internal_, skuId, reinterpret_cast<DiscordSku*>(sku));
    return static_cast<Result>(result);
}

Result StoreManager::GetSkuAt(std::int32_t index, Sku* sku)
{
    if (!sku) {
        return Result::InternalError;
    }

    auto result = internal_->get_sku_at(internal_, index, reinterpret_cast<DiscordSku*>(sku));
    return static_cast<Result>(result);
}

void StoreManager::FetchEntitlements(std::function<void(Result)> callback)
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
    internal_->fetch_entitlements(internal_, cb.release(), wrapper);
}

void StoreManager::CountEntitlements(std::int32_t* count)
{
    if (!count) {
        return;
    }

    internal_->count_entitlements(internal_, reinterpret_cast<int32_t*>(count));
}

Result StoreManager::GetEntitlement(Snowflake entitlementId, Entitlement* entitlement)
{
    if (!entitlement) {
        return Result::InternalError;
    }

    auto result = internal_->get_entitlement(
      internal_, entitlementId, reinterpret_cast<DiscordEntitlement*>(entitlement));
    return static_cast<Result>(result);
}

Result StoreManager::GetEntitlementAt(std::int32_t index, Entitlement* entitlement)
{
    if (!entitlement) {
        return Result::InternalError;
    }

    auto result = internal_->get_entitlement_at(
      internal_, index, reinterpret_cast<DiscordEntitlement*>(entitlement));
    return static_cast<Result>(result);
}

Result StoreManager::HasSkuEntitlement(Snowflake skuId, bool* hasEntitlement)
{
    if (!hasEntitlement) {
        return Result::InternalError;
    }

    auto result =
      internal_->has_sku_entitlement(internal_, skuId, reinterpret_cast<bool*>(hasEntitlement));
    return static_cast<Result>(result);
}

void StoreManager::StartPurchase(Snowflake skuId, std::function<void(Result)> callback)
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
    internal_->start_purchase(internal_, skuId, cb.release(), wrapper);
}

} // namespace discord
