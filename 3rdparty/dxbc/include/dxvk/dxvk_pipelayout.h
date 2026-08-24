#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>

#include "dxvk_hash.h"

#include "util_math.h"
#include "util_bit.h"
#include "util_flags.h"

namespace dxvk {

  class DxvkDevice;
  class DxvkPipelineManager;

  /**
   * \brief Order-invariant atomic access operation
   *
   * Information used to optimize barriers when a resource
   * is accessed exlusively via order-invariant stores.
   */
  struct DxvkAccessOp {
    enum OpType : uint16_t {
      None      = 0x0u,
      Or        = 0x1u,
      And       = 0x2u,
      Xor       = 0x3u,
      Add       = 0x4u,
      IMin      = 0x5u,
      IMax      = 0x6u,
      UMin      = 0x7u,
      UMax      = 0x8u,

      StoreF    = 0xdu,
      StoreUi   = 0xeu,
      StoreSi   = 0xfu,
    };

    DxvkAccessOp() = default;
    DxvkAccessOp(OpType t)
    : op(uint16_t(t)) { }

    DxvkAccessOp(OpType t, uint16_t constant)
    : op(uint16_t(t) | (constant << 4u)) { }

    uint16_t op = 0u;

    bool operator == (const DxvkAccessOp& t) const { return op == t.op; }
    bool operator != (const DxvkAccessOp& t) const { return op != t.op; }

    template<typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
    explicit operator T() const { return op; }
  };

  static_assert(sizeof(DxvkAccessOp) == sizeof(uint16_t));

  /**
   * \brief Binding info
   *
   * Stores metadata for a single binding in
   * a given shader, or for the whole pipeline.
   */
  struct DxvkBindingInfo {
    VkDescriptorType      descriptorType  = VK_DESCRIPTOR_TYPE_MAX_ENUM;        ///< Vulkan descriptor type
    uint32_t              resourceBinding = 0u;                                 ///< API binding slot for the resource
    VkImageViewType       viewType        = VK_IMAGE_VIEW_TYPE_MAX_ENUM;        ///< Image view type
    VkShaderStageFlagBits stage           = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM; ///< Shader stage
    VkAccessFlags         access          = 0u;                                 ///< Access mask for the resource
    DxvkAccessOp          accessOp        = DxvkAccessOp::None;                 ///< Order-invariant store type, if any
    bool                  uboSet          = false;                              ///< Whether to include this in the UBO set
    bool                  isMultisampled  = false;                              ///< Multisampled binding

    /**
     * \brief Computes descriptor set index for the given binding
     *
     * This is determines based on the shader stages that use the binding.
     * \returns Descriptor set index
     */
    uint32_t computeSetIndex() const;

    /**
     * \brief Numeric value of the binding
     *
     * Used when sorting bindings.
     * \returns Numeric value
     */
    uint32_t value() const;

    /**
     * \brief Checks for equality
     *
     * \param [in] other Binding to compare to
     * \returns \c true if both bindings are equal
     */
    bool eq(const DxvkBindingInfo& other) const;

    /**
     * \brief Hashes binding info
     * \returns Binding hash
     */
    size_t hash() const;

  };

}
