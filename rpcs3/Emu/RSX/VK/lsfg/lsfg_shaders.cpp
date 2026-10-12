// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later
// ReX/PS5 adaptation: native SPIR-V validation and explicit device capability checks.
// PS5_RPCS3: without SPIRV-Tools' validator (RPCS3 does not build SPIRV-Tools); the
// checks below of each module's header, instructions, capabilities and interface stay.

#include "lsfg_shaders.h"
#include "util.h"

#include <cstring>
#include <optional>
#include <set>
#include <string_view>
#include "spirv.h"

namespace rex::ui::vulkan::framegen {
namespace {
bool SupportsCapability(u32 capability, const ShaderCapabilities& caps) {
  switch (capability) {
    case SpvCapabilityMatrix:
    case SpvCapabilityShader:
    case SpvCapabilityImageQuery:
    case SpvCapabilityDerivativeControl:
    case SpvCapabilityImageGatherExtended:
      return true;
    // Per-width execution modes are checked below against the physical device.
    case SpvCapabilityDenormPreserve:
    case SpvCapabilityDenormFlushToZero:
    case SpvCapabilitySignedZeroInfNanPreserve:
    case SpvCapabilityRoundingModeRTE:
    case SpvCapabilityRoundingModeRTZ:
      return caps.api_version >= VK_API_VERSION_1_2;
    case SpvCapabilityFloat16: return caps.float16;
    case SpvCapabilityVulkanMemoryModel: return caps.memory_model;
    case SpvCapabilityVulkanMemoryModelDeviceScope: return caps.memory_model && caps.device_scope;
    case SpvCapabilityStorageImageExtendedFormats: return caps.extended_storage_formats;
    case SpvCapabilityStorageImageReadWithoutFormat: return caps.storage_read_without_format;
    case SpvCapabilityStorageImageWriteWithoutFormat: return caps.storage_write_without_format;
    case SpvCapabilityGroupNonUniform:
      return (caps.subgroup_operations & VK_SUBGROUP_FEATURE_BASIC_BIT) != 0;
    case SpvCapabilityGroupNonUniformVote:
      return (caps.subgroup_operations & VK_SUBGROUP_FEATURE_VOTE_BIT) != 0;
    case SpvCapabilityGroupNonUniformArithmetic:
      return (caps.subgroup_operations & VK_SUBGROUP_FEATURE_ARITHMETIC_BIT) != 0;
    case SpvCapabilityGroupNonUniformBallot:
      return (caps.subgroup_operations & VK_SUBGROUP_FEATURE_BALLOT_BIT) != 0;
    case SpvCapabilityGroupNonUniformShuffle:
      return (caps.subgroup_operations & VK_SUBGROUP_FEATURE_SHUFFLE_BIT) != 0;
    case SpvCapabilityGroupNonUniformShuffleRelative:
      return (caps.subgroup_operations & VK_SUBGROUP_FEATURE_SHUFFLE_RELATIVE_BIT) != 0;
    case SpvCapabilityGroupNonUniformClustered:
      return (caps.subgroup_operations & VK_SUBGROUP_FEATURE_CLUSTERED_BIT) != 0;
    case SpvCapabilityGroupNonUniformQuad:
      return (caps.subgroup_operations & VK_SUBGROUP_FEATURE_QUAD_BIT) != 0;
    default: return false;
  }
}

struct Type {
  u32 opcode = 0;
  u32 referenced = 0;
  u32 storage = 0;
  u32 sampled = 0;
  u32 dimension = 0;
  u32 arrayed = 0;
  u32 multisampled = 0;
};
struct Variable { u32 type; u32 storage; };
struct Decorations {
  std::optional<u32> binding;
  std::optional<u32> set;
  bool block = false;
};

bool Reject(std::string& error, std::string reason) {
  error = std::move(reason);
  return false;
}

bool SupportsFloatMode(u32 mode, u32 width, const ShaderCapabilities& caps) {
  const auto& controls = caps.float_controls;
  const auto supported = [width](VkBool32 f16, VkBool32 f32, VkBool32 f64) {
    return width == 16 ? f16 != 0 : width == 32 ? f32 != 0 : width == 64 && f64 != 0;
  };
  switch (mode) {
    case SpvExecutionModeDenormPreserve:
      return supported(controls.shaderDenormPreserveFloat16, controls.shaderDenormPreserveFloat32,
                        controls.shaderDenormPreserveFloat64);
    case SpvExecutionModeDenormFlushToZero:
      return supported(controls.shaderDenormFlushToZeroFloat16, controls.shaderDenormFlushToZeroFloat32,
                        controls.shaderDenormFlushToZeroFloat64);
    case SpvExecutionModeSignedZeroInfNanPreserve:
      return supported(controls.shaderSignedZeroInfNanPreserveFloat16,
                        controls.shaderSignedZeroInfNanPreserveFloat32,
                        controls.shaderSignedZeroInfNanPreserveFloat64);
    case SpvExecutionModeRoundingModeRTE:
      return supported(controls.shaderRoundingModeRTEFloat16, controls.shaderRoundingModeRTEFloat32,
                        controls.shaderRoundingModeRTEFloat64);
    case SpvExecutionModeRoundingModeRTZ:
      return supported(controls.shaderRoundingModeRTZFloat16, controls.shaderRoundingModeRTZFloat32,
                        controls.shaderRoundingModeRTZFloat64);
    default: return true;
  }
}
}  // namespace

bool ValidateShaderModule(std::span<const u32> words, const ShaderCapabilities& capabilities,
                           ShaderInterface& interface, std::string& error) {
  interface = {};
  error.clear();
  if (words.size() < 5 || words.size_bytes() > kMaximumShaderBytes ||
      words[0] != SpvMagicNumber || words[3] > (1u << 22)) {
    return Reject(error, "SPIR-V header or runtime size/ID limit");
  }

  std::map<u32, Type> types;
  std::map<u32, Variable> variables;
  std::map<u32, Decorations> decorations;
  std::map<u32, u32> constants;
  u32 entry = 0;
  std::optional<std::array<u32, 3>> local_size;
  std::optional<std::array<u32, 3>> local_size_ids;
  for (size_t offset = 5; offset < words.size();) {
    const u32 length = words[offset] >> 16;
    const u32 opcode = words[offset] & 0xFFFF;
    if (!length || length > words.size() - offset) return Reject(error, "SPIR-V instruction bounds");
    const u32* instruction = words.data() + offset;
    switch (opcode) {
      case SpvOpCapability:
        if (!SupportsCapability(instruction[1], capabilities)) {
          return Reject(error, "shader needs disabled/unsupported capability " +
                                   std::to_string(instruction[1]));
        }
        break;
      case SpvOpMemoryModel:
        if (instruction[1] != SpvAddressingModelLogical ||
            (instruction[2] != SpvMemoryModelGLSL450 &&
             instruction[2] != SpvMemoryModelVulkan)) {
          return Reject(error, "unsupported shader addressing/memory model");
        }
        break;
      case SpvOpEntryPoint: {
        const std::string_view name(reinterpret_cast<const char*>(instruction + 3),
                                    size_t(length - 3) * sizeof(u32));
        const size_t end = name.find('\0');
        if (entry || instruction[1] != SpvExecutionModelGLCompute || end == std::string_view::npos ||
            name.substr(0, end) != "main") return Reject(error, "expected one compute entry main");
        entry = instruction[2];
        break;
      }
      case SpvOpExecutionMode:
        if (length >= 4 && !SupportsFloatMode(instruction[2], instruction[3], capabilities)) {
          return Reject(error, "unsupported per-width floating-point execution mode");
        }
        if (instruction[2] == SpvExecutionModeLocalSize && length == 6) {
          local_size = std::array<u32, 3>{instruction[3], instruction[4], instruction[5]};
        }
        break;
      case SpvOpExecutionModeId:
        if (instruction[2] == SpvExecutionModeLocalSizeId && length == 6) {
          local_size_ids = std::array<u32, 3>{instruction[3], instruction[4], instruction[5]};
        }
        break;
      case SpvOpDecorate: {
        auto& item = decorations[instruction[1]];
        if (instruction[2] == SpvDecorationBinding && length >= 4) item.binding = instruction[3];
        if (instruction[2] == SpvDecorationDescriptorSet && length >= 4) item.set = instruction[3];
        if (instruction[2] == SpvDecorationBlock) item.block = true;
        break;
      }
      case SpvOpTypePointer:
        types[instruction[1]] = {opcode, instruction[3], instruction[2]};
        break;
      case SpvOpTypeImage:
        types[instruction[1]] = {opcode, instruction[2], 0, instruction[7], instruction[3],
                                  instruction[5], instruction[6]};
        break;
      case SpvOpTypeSampler:
      case SpvOpTypeStruct:
      case SpvOpTypeArray:
      case SpvOpTypeRuntimeArray:
      case SpvOpTypeSampledImage:
        types[instruction[1]] = {opcode};
        break;
      case SpvOpConstant:
        if (length == 4) constants[instruction[2]] = instruction[3];
        break;
      case SpvOpVariable:
        variables[instruction[2]] = {instruction[1], instruction[3]};
        break;
      default: break;
    }
    offset += length;
  }
  if (!entry) return Reject(error, "compute entry point absent");
  if (!local_size && local_size_ids) {
    std::array<u32, 3> resolved{};
    for (size_t axis = 0; axis < 3; ++axis) {
      const auto value = constants.find((*local_size_ids)[axis]);
      if (value == constants.end()) return Reject(error, "specialized workgroup size unsupported");
      resolved[axis] = value->second;
    }
    local_size = resolved;
  }
  if (!local_size) return Reject(error, "static compute workgroup size absent");
  u64 invocations = 1;
  for (size_t axis = 0; axis < 3; ++axis) {
    const u32 value = (*local_size)[axis];
    if (!value || value > capabilities.limits.maxComputeWorkGroupSize[axis]) {
      return Reject(error, "workgroup dimension exceeds device limit");
    }
    invocations *= value;
  }
  if (invocations > capabilities.limits.maxComputeWorkGroupInvocations) {
    return Reject(error, "workgroup invocation count exceeds device limit");
  }
  interface.local_size = *local_size;

  std::map<u32, VkDescriptorType> bindings;
  for (const auto& [id, variable] : variables) {
    const bool uniform_constant = variable.storage == SpvStorageClassUniformConstant;
    if (!uniform_constant && variable.storage != SpvStorageClassUniform) {
      if (variable.storage == SpvStorageClassPushConstant ||
          variable.storage == SpvStorageClassStorageBuffer) {
        return Reject(error, "shader requires a descriptor/push constant interface outside LSFG");
      }
      continue;
    }
    const auto decoration = decorations.find(id);
    if (decoration == decorations.end() || !decoration->second.binding ||
        !decoration->second.set || *decoration->second.set != 0) {
      return Reject(error, "LSFG resource must use descriptor set zero and a binding");
    }
    const auto pointer = types.find(variable.type);
    if (pointer == types.end() || pointer->second.opcode != SpvOpTypePointer ||
        pointer->second.storage != variable.storage) return Reject(error, "invalid resource pointer");
    const auto type = types.find(pointer->second.referenced);
    if (type == types.end()) return Reject(error, "unknown shader resource type");
    VkDescriptorType descriptor;
    if (!uniform_constant && type->second.opcode == SpvOpTypeStruct &&
        decorations[pointer->second.referenced].block) {
      descriptor = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    } else if (uniform_constant && type->second.opcode == SpvOpTypeSampler) {
      descriptor = VK_DESCRIPTOR_TYPE_SAMPLER;
    } else if (uniform_constant && type->second.opcode == SpvOpTypeImage &&
               type->second.dimension == SpvDim2D && !type->second.arrayed &&
               !type->second.multisampled &&
               (type->second.sampled == 1 || type->second.sampled == 2)) {
      descriptor = type->second.sampled == 1 ? VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
                                            : VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    } else {
      return Reject(error, "unsupported LSFG descriptor type/array/image shape");
    }
    if (!bindings.emplace(*decoration->second.binding, descriptor).second) {
      return Reject(error, "duplicate descriptor binding");
    }
  }
  std::array<u32, 4> counts{};
  for (const auto& [binding, type] : bindings) {
    if (binding != interface.bindings.size()) return Reject(error, "non-contiguous LSFG bindings");
    interface.bindings.push_back(type);
    switch (type) {
      case VK_DESCRIPTOR_TYPE_SAMPLER: ++counts[0]; break;
      case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE: ++counts[1]; break;
      case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: ++counts[2]; break;
      case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: ++counts[3]; break;
      default: return Reject(error, "unexpected descriptor type");
    }
  }
  const auto& limits = capabilities.limits;
  if (counts[0] > limits.maxPerStageDescriptorSamplers ||
      counts[1] > limits.maxPerStageDescriptorSampledImages ||
      counts[2] > limits.maxPerStageDescriptorStorageImages ||
      counts[3] > limits.maxPerStageDescriptorUniformBuffers ||
      counts[1] + counts[2] + counts[3] > limits.maxPerStageResources) {
    return Reject(error, "LSFG descriptors exceed device limits");
  }
  return true;
}

LsfgShaders::LsfgShaders(const Device& device, const std::string& path) {
  const auto& enabled = device.Native().properties();
  if (!enabled.vulkanMemoryModel || !enabled.nullDescriptor ||
      enabled.apiVersion < VK_API_VERSION_1_2) {
    throw std::runtime_error("framegen needs enabled Vulkan 1.2 memory model and nullDescriptor");
  }
  ShaderModules code;
  auto result = LoadShaderModules(path, code, enabled.shaderFloat16, enabled.shaderFloat16);
  const auto& subgroups = device.SubgroupProperties();
  const ShaderCapabilities capabilities{
      .api_version = std::min(enabled.apiVersion, device.Native().vulkan_instance()->api_version()),
      .memory_model = enabled.vulkanMemoryModel,
      .device_scope = enabled.vulkanMemoryModelDeviceScope,
      .float16 = enabled.shaderFloat16,
      .extended_storage_formats = enabled.shaderStorageImageExtendedFormats,
      .storage_read_without_format = enabled.shaderStorageImageReadWithoutFormat,
      .storage_write_without_format = enabled.shaderStorageImageWriteWithoutFormat,
      .subgroup_operations = (subgroups.supportedStages & VK_SHADER_STAGE_COMPUTE_BIT)
                                 ? subgroups.supportedOperations : VkSubgroupFeatureFlags(0),
      .limits = device.PhysicalProperties().limits,
      .float_controls = device.FloatControls(),
  };
  const auto validate_all = [&]() -> std::string {
    interfaces_.clear();
    if (result.status != LosslessStatus::Ok) {
      return std::string(LosslessStatusName(result.status)) + ": " + result.detail;
    }
    for (const auto& [id, words] : code) {
      ShaderInterface interface;
      std::string error;
      if (!ValidateShaderModule(words, capabilities, interface, error)) {
        return "SPIR-V resource " + std::to_string(id) + ": " + error;
      }
      interfaces_.emplace(id, std::move(interface));
    }
    return {};
  };
  std::string error = validate_all();
  if (!error.empty() && result.variant == ShaderVariant::NativeFp16) {
    const std::string fp16_error = error;
    result = LoadShaderModules(path, code, false, false);
    error = validate_all();
    if (!error.empty()) error = "FP16: " + fp16_error + "; FP32: " + error;
  }
  if (!error.empty()) throw std::runtime_error(error);
  variant_ = result.variant;
  source_bytes_ = result.source_bytes;
  // All resources validate before the first module is handed to the driver.
  for (const auto& [id, words] : code) {
    modules_.emplace(id, CreateWrappedShaderModule(device, words));
  }
}

VkShaderModule LsfgShaders::Get(u32 shader_id) const {
  const auto found = modules_.find(shader_id);
  return found == modules_.end() ? VK_NULL_HANDLE : *found->second;
}

void LsfgShaders::ValidateInterface(u32 shader_id, std::span<const VkDescriptorType> expected) const {
  const auto found = interfaces_.find(shader_id);
  if (found == interfaces_.end() || found->second.bindings.size() != expected.size() ||
      !std::equal(expected.begin(), expected.end(), found->second.bindings.begin())) {
    throw std::runtime_error("LSFG resource " + std::to_string(shader_id) +
                             " descriptor interface does not match the compute stage");
  }
}
}  // namespace rex::ui::vulkan::framegen
