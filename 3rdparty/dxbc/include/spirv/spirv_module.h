#pragma once

#include <queue>
#include <unordered_map>
#include <unordered_set>

#include "spirv_code_buffer.h"

namespace dxvk {
  
  struct SpirvPhiLabel {
    uint32_t varId         = 0;
    uint32_t labelId       = 0;
  };
  
  struct SpirvSwitchCaseLabel {
    uint32_t literal       = 0;
    uint32_t labelId       = 0;
  };

  struct SpirvMemoryOperands {
    uint32_t flags         = 0;
    uint32_t alignment     = 0;
    uint32_t makeAvailable = 0;
    uint32_t makeVisible   = 0;
  };
  
  struct SpirvImageOperands {
    uint32_t flags         = 0;
    uint32_t sLodBias      = 0;
    uint32_t sLod          = 0;
    uint32_t sConstOffset  = 0;
    uint32_t sGradX        = 0;
    uint32_t sGradY        = 0;
    uint32_t gOffset       = 0;
    uint32_t gConstOffsets = 0;
    uint32_t sSampleId     = 0;
    uint32_t sMinLod       = 0;
    uint32_t makeAvailable = 0;
    uint32_t makeVisible   = 0;
    bool     sparse        = false;
  };

  constexpr uint32_t spvVersion(uint32_t major, uint32_t minor) {
    return (major << 16) | (minor << 8);
  }
  
  /**
   * \brief SPIR-V module
   * 
   * This class generates a code buffer containing a full
   * SPIR-V shader module. Ensures that the module layout
   * is valid, as defined in the SPIR-V 1.0 specification,
   * section 2.4 "Logical Layout of a Module".
   */
  class SpirvModule {
    
  public:
    
    explicit SpirvModule(uint32_t version);

    ~SpirvModule();
    
    SpirvCodeBuffer compile();

    size_t getInsertionPtr() {
      return m_code.getInsertionPtr();
    }
    
    void beginInsertion(size_t ptr) {
      m_code.beginInsertion(ptr);
    }
    
    void endInsertion() {
      m_code.endInsertion();
    }
    
    uint32_t getBlockId() const {
      return m_blockId;
    }

    uint32_t allocateId();
    
    bool hasCapability(
            spv::Capability         capability);

    void enableCapability(
            spv::Capability         capability);
    
    void enableExtension(
      const char*                   extensionName);
    
    void addEntryPoint(
            uint32_t                entryPointId,
            spv::ExecutionModel     executionModel,
      const char*                   name);
    
    void setMemoryModel(
            spv::AddressingModel    addressModel,
            spv::MemoryModel        memoryModel);
    
    void setExecutionMode(
            uint32_t                entryPointId,
            spv::ExecutionMode      executionMode);
    
    void setExecutionMode(
            uint32_t                entryPointId,
            spv::ExecutionMode      executionMode,
            uint32_t                argCount,
      const uint32_t*               args);
    
    void setInvocations(
            uint32_t                entryPointId,
            uint32_t                invocations);
    
    void setLocalSize(
            uint32_t                entryPointId,
            uint32_t                x,
            uint32_t                y,
            uint32_t                z);
    
    void setOutputVertices(
            uint32_t                entryPointId,
            uint32_t                vertexCount);
    
    uint32_t addDebugString(
      const char*                   string);
    
    void setDebugSource(
            spv::SourceLanguage     language,
            uint32_t                version,
            uint32_t                file,
      const char*                   source);
    
    void setDebugName(
            uint32_t                expressionId,
      const char*                   debugName);
    
    void setDebugMemberName(
            uint32_t                structId,
            uint32_t                memberId,
      const char*                   debugName);
    
    uint32_t constBool(
            bool                    v);
    
    uint32_t consti32(
            int32_t                 v);
    
    uint32_t consti64(
            int64_t                 v);
    
    uint32_t constu32(
            uint32_t                v);
    
    uint32_t constu64(
            uint64_t                v);
    
    uint32_t constf32(
            float                   v);
    
    uint32_t constf64(
            double                  v);
    
    uint32_t constvec4i32(
            int32_t                 x,
            int32_t                 y,
            int32_t                 z,
            int32_t                 w);

    uint32_t constvec4b32(
            bool                    x,
            bool                    y,
            bool                    z,
            bool                    w);
    
    uint32_t constvec4u32(
            uint32_t                x,
            uint32_t                y,
            uint32_t                z,
            uint32_t                w);

    uint32_t constvec2f32(
            float                   x,
            float                   y);

    uint32_t constvec3f32(
            float                   x,
            float                   y,
            float                   z);

    uint32_t constvec4f32(
            float                   x,
            float                   y,
            float                   z,
            float                   w);

    uint32_t constfReplicant(
            float                   replicant,
            uint32_t                count);

    uint32_t constbReplicant(
            bool                    replicant,
            uint32_t                count);

    uint32_t constiReplicant(
            int32_t                 replicant,
            uint32_t                count);

    uint32_t constuReplicant(
            int32_t                 replicant,
            uint32_t                count);
    
    uint32_t constComposite(
            uint32_t                typeId,
            uint32_t                constCount,
      const uint32_t*               constIds);
    
    uint32_t constUndef(
            uint32_t                typeId);
    
    uint32_t constNull(
            uint32_t                typeId);
    
    uint32_t lateConst32(
            uint32_t                typeId);

    void setLateConst(
            uint32_t                constId,
      const uint32_t*               argIds);

    uint32_t specConstBool(
            bool                    v);
    
    uint32_t specConst32(
            uint32_t                typeId,
            uint32_t                value);
    
    void decorate(
            uint32_t                object,
            spv::Decoration         decoration);
    
    void decorateArrayStride(
            uint32_t                object,
            uint32_t                stride);
    
    void decorateBinding(
            uint32_t                object,
            uint32_t                binding);
    
    void decorateBlock(
            uint32_t                object);
    
    void decorateBuiltIn(
            uint32_t                object,
            spv::BuiltIn            builtIn);
    
    void decorateComponent(
            uint32_t                object,
            uint32_t                location);
    
    void decorateDescriptorSet(
            uint32_t                object,
            uint32_t                set);
    
    void decorateIndex(
            uint32_t                object,
            uint32_t                index);
    
    void decorateLocation(
            uint32_t                object,
            uint32_t                location);
    
    void decorateSpecId(
            uint32_t                object,
            uint32_t                specId);
    
    void decorateXfb(
            uint32_t                object,
            uint32_t                streamId,
            uint32_t                bufferId,
            uint32_t                offset,
            uint32_t                stride);
    
    void memberDecorateBuiltIn(
            uint32_t                structId,
            uint32_t                memberId,
            spv::BuiltIn            builtIn);

    void memberDecorate(
            uint32_t                structId,
            uint32_t                memberId,
            spv::Decoration         decoration);

    void memberDecorateMatrixStride(
            uint32_t                structId,
            uint32_t                memberId,
            uint32_t                stride);
    
    void memberDecorateOffset(
            uint32_t                structId,
            uint32_t                memberId,
            uint32_t                offset);
    
    uint32_t defVoidType();
    
    uint32_t defBoolType();
    
    uint32_t defIntType(
            uint32_t                width,
            uint32_t                isSigned);
    
    uint32_t defFloatType(
            uint32_t                width);
    
    uint32_t defVectorType(
            uint32_t                elementType,
            uint32_t                elementCount);
    
    uint32_t defMatrixType(
            uint32_t                columnType,
            uint32_t                columnCount);
    
    uint32_t defArrayType(
            uint32_t                typeId,
            uint32_t                length);
    
    uint32_t defArrayTypeUnique(
            uint32_t                typeId,
            uint32_t                length);
    
    uint32_t defRuntimeArrayType(
            uint32_t                typeId);
    
    uint32_t defRuntimeArrayTypeUnique(
            uint32_t                typeId);
    
    uint32_t defFunctionType(
            uint32_t                returnType,
            uint32_t                argCount,
      const uint32_t*               argTypes);
    
    uint32_t defStructType(
            uint32_t                memberCount,
      const uint32_t*               memberTypes);
    
    uint32_t defStructTypeUnique(
            uint32_t                memberCount,
      const uint32_t*               memberTypes);
    
    uint32_t defPointerType(
            uint32_t                variableType,
            spv::StorageClass       storageClass);
    
    uint32_t defSamplerType();
    
    uint32_t defImageType(
            uint32_t                sampledType,
            spv::Dim                dimensionality,
            uint32_t                depth,
            uint32_t                arrayed,
            uint32_t                multisample,
            uint32_t                sampled,
            spv::ImageFormat        format);
    
    uint32_t defSampledImageType(
            uint32_t                imageType);
    
    uint32_t newVar(
            uint32_t                pointerType,
            spv::StorageClass       storageClass);
    
    uint32_t newVarInit(
            uint32_t                pointerType,
            spv::StorageClass       storageClass,
            uint32_t                initialValue);
    
    void functionBegin(
            uint32_t                returnType,
            uint32_t                functionId,
            uint32_t                functionType,
      spv::FunctionControlMask      functionControl);
    
    uint32_t functionParameter(
            uint32_t                parameterType);
    
    void functionEnd();

    uint32_t opAccessChain(
            uint32_t                resultType,
            uint32_t                composite,
            uint32_t                indexCount,
      const uint32_t*               indexArray);
    
    uint32_t opArrayLength(
            uint32_t                resultType,
            uint32_t                structure,
            uint32_t                memberId);
    
    uint32_t opAny(
            uint32_t                resultType,
            uint32_t                vector);
    
    uint32_t opAll(
            uint32_t                resultType,
            uint32_t                vector);
    
    uint32_t opAtomicLoad(
            uint32_t                resultType,
            uint32_t                pointer,
            uint32_t                scope,
            uint32_t                semantics);
            
    void opAtomicStore(
            uint32_t                pointer,
            uint32_t                scope,
            uint32_t                semantics,
            uint32_t                value);
            
    uint32_t opAtomicExchange(
            uint32_t                resultType,
            uint32_t                pointer,
            uint32_t                scope,
            uint32_t                semantics,
            uint32_t                value);
            
    uint32_t opAtomicCompareExchange(
            uint32_t                resultType,
            uint32_t                pointer,
            uint32_t                scope,
            uint32_t                equal,
            uint32_t                unequal,
            uint32_t                value,
            uint32_t                comparator);
            
    uint32_t opAtomicIIncrement(
            uint32_t                resultType,
            uint32_t                pointer,
            uint32_t                scope,
            uint32_t                semantics);
            
    uint32_t opAtomicIDecrement(
            uint32_t                resultType,
            uint32_t                pointer,
            uint32_t                scope,
            uint32_t                semantics);
            
    uint32_t opAtomicIAdd(
            uint32_t                resultType,
            uint32_t                pointer,
            uint32_t                scope,
            uint32_t                semantics,
            uint32_t                value);
    
    uint32_t opAtomicISub(
            uint32_t                resultType,
            uint32_t                pointer,
            uint32_t                scope,
            uint32_t                semantics,
            uint32_t                value);
    
    uint32_t opAtomicSMin(
            uint32_t                resultType,
            uint32_t                pointer,
            uint32_t                scope,
            uint32_t                semantics,
            uint32_t                value);
    
    uint32_t opAtomicSMax(
            uint32_t                resultType,
            uint32_t                pointer,
            uint32_t                scope,
            uint32_t                semantics,
            uint32_t                value);
    
    uint32_t opAtomicUMin(
            uint32_t                resultType,
            uint32_t                pointer,
            uint32_t                scope,
            uint32_t                semantics,
            uint32_t                value);
    
    uint32_t opAtomicUMax(
            uint32_t                resultType,
            uint32_t                pointer,
            uint32_t                scope,
            uint32_t                semantics,
            uint32_t                value);
    
    uint32_t opAtomicAnd(
            uint32_t                resultType,
            uint32_t                pointer,
            uint32_t                scope,
            uint32_t                semantics,
            uint32_t                value);
    
    uint32_t opAtomicOr(
            uint32_t                resultType,
            uint32_t                pointer,
            uint32_t                scope,
            uint32_t                semantics,
            uint32_t                value);
    
    uint32_t opAtomicXor(
            uint32_t                resultType,
            uint32_t                pointer,
            uint32_t                scope,
            uint32_t                semantics,
            uint32_t                value);
    
    uint32_t opBitcast(
            uint32_t                resultType,
            uint32_t                operand);
            
    uint32_t opBitCount(
            uint32_t                resultType,
            uint32_t                operand);
    
    uint32_t opBitReverse(
            uint32_t                resultType,
            uint32_t                operand);
    
    uint32_t opFindILsb(
            uint32_t                resultType,
            uint32_t                operand);
    
    uint32_t opFindUMsb(
            uint32_t                resultType,
            uint32_t                operand);
    
    uint32_t opFindSMsb(
            uint32_t                resultType,
            uint32_t                operand);
    
    uint32_t opBitFieldInsert(
            uint32_t                resultType,
            uint32_t                base,
            uint32_t                insert,
            uint32_t                offset,
            uint32_t                count);
    
    uint32_t opBitFieldSExtract(
            uint32_t                resultType,
            uint32_t                base,
            uint32_t                offset,
            uint32_t                count);
    
    uint32_t opBitFieldUExtract(
            uint32_t                resultType,
            uint32_t                base,
            uint32_t                offset,
            uint32_t                count);
    
    uint32_t opBitwiseAnd(
            uint32_t                resultType,
            uint32_t                operand1,
            uint32_t                operand2);
    
    uint32_t opBitwiseOr(
            uint32_t                resultType,
            uint32_t                operand1,
            uint32_t                operand2);
    
    uint32_t opBitwiseXor(
            uint32_t                resultType,
            uint32_t                operand1,
            uint32_t                operand2);
    
    uint32_t opNot(
            uint32_t                resultType,
            uint32_t                operand);
    
    uint32_t opShiftLeftLogical(
            uint32_t                resultType,
            uint32_t                base,
            uint32_t                shift);
    
    uint32_t opShiftRightArithmetic(
            uint32_t                resultType,
            uint32_t                base,
            uint32_t                shift);
    
    uint32_t opShiftRightLogical(
            uint32_t                resultType,
            uint32_t                base,
            uint32_t                shift);
    
    uint32_t opConvertFtoS(
            uint32_t                resultType,
            uint32_t                operand);
    
    uint32_t opConvertFtoU(
            uint32_t                resultType,
            uint32_t                operand);
    
    uint32_t opConvertStoF(
            uint32_t                resultType,
            uint32_t                operand);
    
    uint32_t opConvertUtoF(
            uint32_t                resultType,
            uint32_t                operand);
    
    uint32_t opCompositeConstruct(
            uint32_t                resultType,
            uint32_t                valueCount,
      const uint32_t*               valueArray);
    
    uint32_t opCompositeExtract(
            uint32_t                resultType,
            uint32_t                composite,
            uint32_t                indexCount,
      const uint32_t*               indexArray);
    
    uint32_t opCompositeInsert(
            uint32_t                resultType,
            uint32_t                object,
            uint32_t                composite,
            uint32_t                indexCount,
      const uint32_t*               indexArray);
    
    uint32_t opDpdx(
            uint32_t                resultType,
            uint32_t                operand);
    
    uint32_t opDpdy(
            uint32_t                resultType,
            uint32_t                operand);
    
    uint32_t opDpdxCoarse(
            uint32_t                resultType,
            uint32_t                operand);
    
    uint32_t opDpdyCoarse(
            uint32_t                resultType,
            uint32_t                operand);
    
    uint32_t opDpdxFine(
            uint32_t                resultType,
            uint32_t                operand);
    
    uint32_t opDpdyFine(
            uint32_t                resultType,
            uint32_t                operand);
    
    uint32_t opVectorExtractDynamic(
            uint32_t                resultType,
            uint32_t                vector,
            uint32_t                index);
    
    uint32_t opVectorShuffle(
            uint32_t                resultType,
            uint32_t                vectorLeft,
            uint32_t                vectorRight,
            uint32_t                indexCount,
      const uint32_t*               indexArray);
    
    uint32_t opSNegate(
            uint32_t                resultType,
            uint32_t                operand);
    
    uint32_t opFNegate(
            uint32_t                resultType,
            uint32_t                operand);
    
    uint32_t opSAbs(
            uint32_t                resultType,
            uint32_t                operand);
    
    uint32_t opFAbs(
            uint32_t                resultType,
            uint32_t                operand);

    uint32_t opFSign(
            uint32_t                resultType,
            uint32_t                operand);

    uint32_t opFMix(
            uint32_t                resultType,
            uint32_t                x,
            uint32_t                y,
            uint32_t                a);

  uint32_t opCross(
            uint32_t                resultType,
            uint32_t                x,
            uint32_t                y);
    
    uint32_t opIAdd(
            uint32_t                resultType,
            uint32_t                a,
            uint32_t                b);
    
    uint32_t opISub(
            uint32_t                resultType,
            uint32_t                a,
            uint32_t                b);
    
    uint32_t opFAdd(
            uint32_t                resultType,
            uint32_t                a,
            uint32_t                b);
    
    uint32_t opFSub(
            uint32_t                resultType,
            uint32_t                a,
            uint32_t                b);
    
    uint32_t opSDiv(
            uint32_t                resultType,
            uint32_t                a,
            uint32_t                b);
    
    uint32_t opUDiv(
            uint32_t                resultType,
            uint32_t                a,
            uint32_t                b);
    
    uint32_t opSRem(
            uint32_t                resultType,
            uint32_t                a,
            uint32_t                b);
    
    uint32_t opUMod(
            uint32_t                resultType,
            uint32_t                a,
            uint32_t                b);
    
    uint32_t opFDiv(
            uint32_t                resultType,
            uint32_t                a,
            uint32_t                b);
    
    uint32_t opIMul(
            uint32_t                resultType,
            uint32_t                a,
            uint32_t                b);
    
    uint32_t opFMul(
            uint32_t                resultType,
            uint32_t                a,
            uint32_t                b);

    uint32_t opVectorTimesScalar(
            uint32_t                resultType,
            uint32_t                vector,
            uint32_t                scalar);

    uint32_t opMatrixTimesMatrix(
            uint32_t                resultType,
            uint32_t                a,
            uint32_t                b);

    uint32_t opMatrixTimesVector(
            uint32_t                resultType,
            uint32_t                matrix,
            uint32_t                vector);

    uint32_t opVectorTimesMatrix(
            uint32_t                resultType,
            uint32_t                vector,
            uint32_t                matrix);

    uint32_t opTranspose(
            uint32_t                resultType,
            uint32_t                matrix);

    uint32_t opInverse(
            uint32_t                resultType,
            uint32_t                matrix);
    
    uint32_t opFFma(
            uint32_t                resultType,
            uint32_t                a,
            uint32_t                b,
            uint32_t                c);
    
    uint32_t opFMax(
            uint32_t                resultType,
            uint32_t                a,
            uint32_t                b);
    
    uint32_t opFMin(
            uint32_t                resultType,
            uint32_t                a,
            uint32_t                b);
    
    uint32_t opNMax(
            uint32_t                resultType,
            uint32_t                a,
            uint32_t                b);
    
    uint32_t opNMin(
            uint32_t                resultType,
            uint32_t                a,
            uint32_t                b);
    
    uint32_t opSMax(
            uint32_t                resultType,
            uint32_t                a,
            uint32_t                b);
    
    uint32_t opSMin(
            uint32_t                resultType,
            uint32_t                a,
            uint32_t                b);
    
    uint32_t opUMax(
            uint32_t                resultType,
            uint32_t                a,
            uint32_t                b);
    
    uint32_t opUMin(
            uint32_t                resultType,
            uint32_t                a,
            uint32_t                b);
    
    uint32_t opFClamp(
            uint32_t                resultType,
            uint32_t                x,
            uint32_t                minVal,
            uint32_t                maxVal);
    
    uint32_t opNClamp(
            uint32_t                resultType,
            uint32_t                x,
            uint32_t                minVal,
            uint32_t                maxVal);
    
    uint32_t opIEqual(
            uint32_t                resultType,
            uint32_t                vector1,
            uint32_t                vector2);
    
    uint32_t opINotEqual(
            uint32_t                resultType,
            uint32_t                vector1,
            uint32_t                vector2);
    
    uint32_t opSLessThan(
            uint32_t                resultType,
            uint32_t                vector1,
            uint32_t                vector2);
    
    uint32_t opSLessThanEqual(
            uint32_t                resultType,
            uint32_t                vector1,
            uint32_t                vector2);
    
    uint32_t opSGreaterThan(
            uint32_t                resultType,
            uint32_t                vector1,
            uint32_t                vector2);
    
    uint32_t opSGreaterThanEqual(
            uint32_t                resultType,
            uint32_t                vector1,
            uint32_t                vector2);
    
    uint32_t opULessThan(
            uint32_t                resultType,
            uint32_t                vector1,
            uint32_t                vector2);
    
    uint32_t opULessThanEqual(
            uint32_t                resultType,
            uint32_t                vector1,
            uint32_t                vector2);
    
    uint32_t opUGreaterThan(
            uint32_t                resultType,
            uint32_t                vector1,
            uint32_t                vector2);
    
    uint32_t opUGreaterThanEqual(
            uint32_t                resultType,
            uint32_t                vector1,
            uint32_t                vector2);
    
    uint32_t opFOrdEqual(
            uint32_t                resultType,
            uint32_t                vector1,
            uint32_t                vector2);
    
    uint32_t opFUnordNotEqual(
            uint32_t                resultType,
            uint32_t                vector1,
            uint32_t                vector2);
    
    uint32_t opFOrdLessThan(
            uint32_t                resultType,
            uint32_t                vector1,
            uint32_t                vector2);
    
    uint32_t opFOrdLessThanEqual(
            uint32_t                resultType,
            uint32_t                vector1,
            uint32_t                vector2);
    
    uint32_t opFOrdGreaterThan(
            uint32_t                resultType,
            uint32_t                vector1,
            uint32_t                vector2);
    
    uint32_t opFOrdGreaterThanEqual(
            uint32_t                resultType,
            uint32_t                vector1,
            uint32_t                vector2);
    
    uint32_t opLogicalEqual(
            uint32_t                resultType,
            uint32_t                operand1,
            uint32_t                operand2);
    
    uint32_t opLogicalNotEqual(
            uint32_t                resultType,
            uint32_t                operand1,
            uint32_t                operand2);
    
    uint32_t opLogicalAnd(
            uint32_t                resultType,
            uint32_t                operand1,
            uint32_t                operand2);
    
    uint32_t opLogicalOr(
            uint32_t                resultType,
            uint32_t                operand1,
            uint32_t                operand2);
    
    uint32_t opLogicalNot(
            uint32_t                resultType,
            uint32_t                operand);
    
    uint32_t opDot(
            uint32_t                resultType,
            uint32_t                vector1,
            uint32_t                vector2);
    
    uint32_t opSin(
            uint32_t                resultType,
            uint32_t                vector);
    
    uint32_t opCos(
            uint32_t                resultType,
            uint32_t                vector);
    
    uint32_t opSqrt(
            uint32_t                resultType,
            uint32_t                operand);
    
    uint32_t opInverseSqrt(
            uint32_t                resultType,
            uint32_t                operand);

    uint32_t opNormalize(
            uint32_t                resultType,
            uint32_t                operand);

    uint32_t opRawAccessChain(
            uint32_t                resultType,
            uint32_t                base,
            uint32_t                stride,
            uint32_t                index,
            uint32_t                offset,
            uint32_t                operand);

    uint32_t opReflect(
            uint32_t                resultType,
            uint32_t                incident,
            uint32_t                normal);

    uint32_t opLength(
            uint32_t                resultType,
            uint32_t                operand);
    
    uint32_t opExp2(
            uint32_t                resultType,
            uint32_t                operand);

    uint32_t opExp(
            uint32_t                resultType,
            uint32_t                operand);
    
    uint32_t opLog2(
            uint32_t                resultType,
            uint32_t                operand);

    uint32_t opPow(
            uint32_t                resultType,
            uint32_t                base,
            uint32_t                exponent);
    
    uint32_t opFract(
            uint32_t                resultType,
            uint32_t                operand);
    
    uint32_t opCeil(
            uint32_t                resultType,
            uint32_t                operand);
    
    uint32_t opFloor(
            uint32_t                resultType,
            uint32_t                operand);
    
    uint32_t opRound(
            uint32_t                resultType,
            uint32_t                operand);
    
    uint32_t opRoundEven(
            uint32_t                resultType,
            uint32_t                operand);
    
    uint32_t opTrunc(
            uint32_t                resultType,
            uint32_t                operand);
    
    uint32_t opFConvert(
            uint32_t                resultType,
            uint32_t                operand);
    
    uint32_t opPackHalf2x16(
            uint32_t                resultType,
            uint32_t                operand);
    
    uint32_t opUnpackHalf2x16(
            uint32_t                resultType,
            uint32_t                operand);
    
    uint32_t opSelect(
            uint32_t                resultType,
            uint32_t                condition,
            uint32_t                operand1,
            uint32_t                operand2);

    uint32_t opIsNan(
            uint32_t                resultType,
            uint32_t                operand);
    
    uint32_t opIsInf(
            uint32_t                resultType,
            uint32_t                operand);
    
    uint32_t opFunctionCall(
            uint32_t                resultType,
            uint32_t                functionId,
            uint32_t                argCount,
      const uint32_t*               argIds);
    
    void opLabel(
            uint32_t                labelId);
    
    uint32_t opLoad(
            uint32_t                typeId,
            uint32_t                pointerId);

    uint32_t opLoad(
            uint32_t                typeId,
            uint32_t                pointerId,
      const SpirvMemoryOperands&    operands);

    void opStore(
            uint32_t                pointerId,
            uint32_t                valueId);

    void opStore(
            uint32_t                pointerId,
            uint32_t                valueId,
      const SpirvMemoryOperands&    operands);

    uint32_t opInterpolateAtCentroid(
            uint32_t                resultType,
            uint32_t                interpolant);
    
    uint32_t opInterpolateAtSample(
            uint32_t                resultType,
            uint32_t                interpolant,
            uint32_t                sample);
    
    uint32_t opInterpolateAtOffset(
            uint32_t                resultType,
            uint32_t                interpolant,
            uint32_t                offset);

    uint32_t opImage(
            uint32_t                resultType,
            uint32_t                sampledImage);
    
    uint32_t opImageRead(
            uint32_t                resultType,
            uint32_t                image,
            uint32_t                coordinates,
      const SpirvImageOperands&     operands);
    
    void opImageWrite(
            uint32_t                image,
            uint32_t                coordinates,
            uint32_t                texel,
      const SpirvImageOperands&     operands);

    uint32_t opImageSparseTexelsResident(
            uint32_t                resultType,
            uint32_t                residentCode);

    uint32_t opImageTexelPointer(
            uint32_t                resultType,
            uint32_t                image,
            uint32_t                coordinates,
            uint32_t                sample);
    
    uint32_t opSampledImage(
            uint32_t                resultType,
            uint32_t                image,
            uint32_t                sampler);
    
    uint32_t opImageQuerySizeLod(
            uint32_t                resultType,
            uint32_t                image,
            uint32_t                lod);
    
    uint32_t opImageQuerySize(
            uint32_t                resultType,
            uint32_t                image);
    
    uint32_t opImageQueryLevels(
            uint32_t                resultType,
            uint32_t                image);
    
    uint32_t opImageQueryLod(
            uint32_t                resultType,
            uint32_t                sampledImage,
            uint32_t                coordinates);
    
    uint32_t opImageQuerySamples(
            uint32_t                resultType,
            uint32_t                image);
    
    uint32_t opImageFetch(
            uint32_t                resultType,
            uint32_t                image,
            uint32_t                coordinates,
      const SpirvImageOperands&     operands);
    
    uint32_t opImageGather(
            uint32_t                resultType,
            uint32_t                sampledImage,
            uint32_t                coordinates,
            uint32_t                component,
      const SpirvImageOperands&     operands);
    
    uint32_t opImageDrefGather(
            uint32_t                resultType,
            uint32_t                sampledImage,
            uint32_t                coordinates,
            uint32_t                reference,
      const SpirvImageOperands&     operands);
    
    uint32_t opImageSampleImplicitLod(
            uint32_t                resultType,
            uint32_t                sampledImage,
            uint32_t                coordinates,
      const SpirvImageOperands&     operands);
    
    uint32_t opImageSampleExplicitLod(
            uint32_t                resultType,
            uint32_t                sampledImage,
            uint32_t                coordinates,
      const SpirvImageOperands&     operands);

    uint32_t opImageSampleProjImplicitLod(
            uint32_t                resultType,
            uint32_t                sampledImage,
            uint32_t                coordinates,
      const SpirvImageOperands&     operands);

    uint32_t opImageSampleProjExplicitLod(
            uint32_t                resultType,
            uint32_t                sampledImage,
            uint32_t                coordinates,
      const SpirvImageOperands&     operands);
    
    uint32_t opImageSampleDrefImplicitLod(
            uint32_t                resultType,
            uint32_t                sampledImage,
            uint32_t                coordinates,
            uint32_t                reference,
      const SpirvImageOperands&     operands);
    
    uint32_t opImageSampleDrefExplicitLod(
            uint32_t                resultType,
            uint32_t                sampledImage,
            uint32_t                coordinates,
            uint32_t                reference,
      const SpirvImageOperands&     operands);

    uint32_t opImageSampleProjDrefImplicitLod(
            uint32_t                resultType,
            uint32_t                sampledImage,
            uint32_t                coordinates,
            uint32_t                reference,
      const SpirvImageOperands&     operands);

    uint32_t opImageSampleProjDrefExplicitLod(
            uint32_t                resultType,
            uint32_t                sampledImage,
            uint32_t                coordinates,
            uint32_t                reference,
      const SpirvImageOperands&     operands);

    uint32_t opGroupNonUniformBallot(
            uint32_t                resultType,
            uint32_t                execution,
            uint32_t                predicate);
    
    uint32_t opGroupNonUniformBallotBitCount(
            uint32_t                resultType,
            uint32_t                execution,
            uint32_t                operation,
            uint32_t                ballot);
    
    uint32_t opGroupNonUniformElect(
            uint32_t                resultType,
            uint32_t                execution);
    
    uint32_t opGroupNonUniformBroadcastFirst(
            uint32_t                resultType,
            uint32_t                execution,
            uint32_t                value);
    
    void opControlBarrier(
            uint32_t                execution,
            uint32_t                memory,
            uint32_t                semantics);
    
    void opMemoryBarrier(
            uint32_t                memory,
            uint32_t                semantics);
    
    void opLoopMerge(
            uint32_t                mergeBlock,
            uint32_t                continueTarget,
            uint32_t                loopControl);
    
    void opSelectionMerge(
            uint32_t                mergeBlock,
            uint32_t                selectionControl);
    
    void opBranch(
            uint32_t                label);
    
    void opBranchConditional(
            uint32_t                condition,
            uint32_t                trueLabel,
            uint32_t                falseLabel);
    
    void opSwitch(
            uint32_t                selector,
            uint32_t                jumpDefault,
            uint32_t                caseCount,
      const SpirvSwitchCaseLabel*   caseLabels);
    
    uint32_t opPhi(
            uint32_t                resultType,
            uint32_t                sourceCount,
      const SpirvPhiLabel*          sourceLabels);
    
    void opReturn();
    
    void opDemoteToHelperInvocation();
    
    void opEmitVertex(
            uint32_t                streamId);
    
    void opEndPrimitive(
            uint32_t                streamId);

    void opBeginInvocationInterlock();

    void opEndInvocationInterlock();

    uint32_t opSinCos(
            uint32_t                x,
            bool                    useBuiltIn);

  private:
    
    uint32_t m_version;
    uint32_t m_id             = 1;
    uint32_t m_instExtGlsl450 = 0;
    uint32_t m_blockId        = 0;
    
    SpirvCodeBuffer m_capabilities;
    SpirvCodeBuffer m_extensions;
    SpirvCodeBuffer m_instExt;
    SpirvCodeBuffer m_memoryModel;
    SpirvCodeBuffer m_entryPoints;
    SpirvCodeBuffer m_execModeInfo;
    SpirvCodeBuffer m_debugNames;
    SpirvCodeBuffer m_annotations;
    SpirvCodeBuffer m_typeConstDefs;
    SpirvCodeBuffer m_variables;
    SpirvCodeBuffer m_code;

    std::unordered_set<uint32_t> m_lateConsts;

    std::vector<uint32_t> m_interfaceVars;

    uint32_t defType(
            spv::Op                 op, 
            uint32_t                argCount,
      const uint32_t*               argIds);
    
    uint32_t defConst(
            spv::Op                 op,
            uint32_t                typeId,
            uint32_t                argCount,
      const uint32_t*               argIds);
    
    void instImportGlsl450();
    
    uint32_t getMemoryOperandWordCount(
      const SpirvMemoryOperands&    op) const;

    void putMemoryOperands(
      const SpirvMemoryOperands&    op);

    uint32_t getImageOperandWordCount(
      const SpirvImageOperands&     op) const;
    
    void putImageOperands(
      const SpirvImageOperands&     op);

    bool isInterfaceVar(
            spv::StorageClass       sclass) const;

    void classifyBlocks(
            std::unordered_set<uint32_t>& reachableBlocks,
            std::unordered_set<uint32_t>& mergeBlocks);

    static constexpr double sincosTaylorFactor(uint32_t power) {
      double result = 1.0;

      for (uint32_t i = 1; i <= power; i++)
        result *= pi * 0.25f / double(i);

      return result;
    }

  };
  
}
