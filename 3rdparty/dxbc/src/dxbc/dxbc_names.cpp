#include "dxbc_names.h"

namespace dxvk {

  std::ostream& operator << (std::ostream& os, DxbcOpcode e) {
    switch (e) {
      ENUM_NAME(DxbcOpcode::Add);
      ENUM_NAME(DxbcOpcode::And);
      ENUM_NAME(DxbcOpcode::Break);
      ENUM_NAME(DxbcOpcode::Breakc);
      ENUM_NAME(DxbcOpcode::Call);
      ENUM_NAME(DxbcOpcode::Callc);
      ENUM_NAME(DxbcOpcode::Case);
      ENUM_NAME(DxbcOpcode::Continue);
      ENUM_NAME(DxbcOpcode::Continuec);
      ENUM_NAME(DxbcOpcode::Cut);
      ENUM_NAME(DxbcOpcode::Default);
      ENUM_NAME(DxbcOpcode::DerivRtx);
      ENUM_NAME(DxbcOpcode::DerivRty);
      ENUM_NAME(DxbcOpcode::Discard);
      ENUM_NAME(DxbcOpcode::Div);
      ENUM_NAME(DxbcOpcode::Dp2);
      ENUM_NAME(DxbcOpcode::Dp3);
      ENUM_NAME(DxbcOpcode::Dp4);
      ENUM_NAME(DxbcOpcode::Else);
      ENUM_NAME(DxbcOpcode::Emit);
      ENUM_NAME(DxbcOpcode::EmitThenCut);
      ENUM_NAME(DxbcOpcode::EndIf);
      ENUM_NAME(DxbcOpcode::EndLoop);
      ENUM_NAME(DxbcOpcode::EndSwitch);
      ENUM_NAME(DxbcOpcode::Eq);
      ENUM_NAME(DxbcOpcode::Exp);
      ENUM_NAME(DxbcOpcode::Frc);
      ENUM_NAME(DxbcOpcode::FtoI);
      ENUM_NAME(DxbcOpcode::FtoU);
      ENUM_NAME(DxbcOpcode::Ge);
      ENUM_NAME(DxbcOpcode::IAdd);
      ENUM_NAME(DxbcOpcode::If);
      ENUM_NAME(DxbcOpcode::IEq);
      ENUM_NAME(DxbcOpcode::IGe);
      ENUM_NAME(DxbcOpcode::ILt);
      ENUM_NAME(DxbcOpcode::IMad);
      ENUM_NAME(DxbcOpcode::IMax);
      ENUM_NAME(DxbcOpcode::IMin);
      ENUM_NAME(DxbcOpcode::IMul);
      ENUM_NAME(DxbcOpcode::INe);
      ENUM_NAME(DxbcOpcode::INeg);
      ENUM_NAME(DxbcOpcode::IShl);
      ENUM_NAME(DxbcOpcode::IShr);
      ENUM_NAME(DxbcOpcode::ItoF);
      ENUM_NAME(DxbcOpcode::Label);
      ENUM_NAME(DxbcOpcode::Ld);
      ENUM_NAME(DxbcOpcode::LdMs);
      ENUM_NAME(DxbcOpcode::Log);
      ENUM_NAME(DxbcOpcode::Loop);
      ENUM_NAME(DxbcOpcode::Lt);
      ENUM_NAME(DxbcOpcode::Mad);
      ENUM_NAME(DxbcOpcode::Min);
      ENUM_NAME(DxbcOpcode::Max);
      ENUM_NAME(DxbcOpcode::CustomData);
      ENUM_NAME(DxbcOpcode::Mov);
      ENUM_NAME(DxbcOpcode::Movc);
      ENUM_NAME(DxbcOpcode::Mul);
      ENUM_NAME(DxbcOpcode::Ne);
      ENUM_NAME(DxbcOpcode::Nop);
      ENUM_NAME(DxbcOpcode::Not);
      ENUM_NAME(DxbcOpcode::Or);
      ENUM_NAME(DxbcOpcode::ResInfo);
      ENUM_NAME(DxbcOpcode::Ret);
      ENUM_NAME(DxbcOpcode::Retc);
      ENUM_NAME(DxbcOpcode::RoundNe);
      ENUM_NAME(DxbcOpcode::RoundNi);
      ENUM_NAME(DxbcOpcode::RoundPi);
      ENUM_NAME(DxbcOpcode::RoundZ);
      ENUM_NAME(DxbcOpcode::Rsq);
      ENUM_NAME(DxbcOpcode::Sample);
      ENUM_NAME(DxbcOpcode::SampleC);
      ENUM_NAME(DxbcOpcode::SampleClz);
      ENUM_NAME(DxbcOpcode::SampleL);
      ENUM_NAME(DxbcOpcode::SampleD);
      ENUM_NAME(DxbcOpcode::SampleB);
      ENUM_NAME(DxbcOpcode::Sqrt);
      ENUM_NAME(DxbcOpcode::Switch);
      ENUM_NAME(DxbcOpcode::SinCos);
      ENUM_NAME(DxbcOpcode::UDiv);
      ENUM_NAME(DxbcOpcode::ULt);
      ENUM_NAME(DxbcOpcode::UGe);
      ENUM_NAME(DxbcOpcode::UMul);
      ENUM_NAME(DxbcOpcode::UMad);
      ENUM_NAME(DxbcOpcode::UMax);
      ENUM_NAME(DxbcOpcode::UMin);
      ENUM_NAME(DxbcOpcode::UShr);
      ENUM_NAME(DxbcOpcode::UtoF);
      ENUM_NAME(DxbcOpcode::Xor);
      ENUM_NAME(DxbcOpcode::DclResource);
      ENUM_NAME(DxbcOpcode::DclConstantBuffer);
      ENUM_NAME(DxbcOpcode::DclSampler);
      ENUM_NAME(DxbcOpcode::DclIndexRange);
      ENUM_NAME(DxbcOpcode::DclGsOutputPrimitiveTopology);
      ENUM_NAME(DxbcOpcode::DclGsInputPrimitive);
      ENUM_NAME(DxbcOpcode::DclMaxOutputVertexCount);
      ENUM_NAME(DxbcOpcode::DclInput);
      ENUM_NAME(DxbcOpcode::DclInputSgv);
      ENUM_NAME(DxbcOpcode::DclInputSiv);
      ENUM_NAME(DxbcOpcode::DclInputPs);
      ENUM_NAME(DxbcOpcode::DclInputPsSgv);
      ENUM_NAME(DxbcOpcode::DclInputPsSiv);
      ENUM_NAME(DxbcOpcode::DclOutput);
      ENUM_NAME(DxbcOpcode::DclOutputSgv);
      ENUM_NAME(DxbcOpcode::DclOutputSiv);
      ENUM_NAME(DxbcOpcode::DclTemps);
      ENUM_NAME(DxbcOpcode::DclIndexableTemp);
      ENUM_NAME(DxbcOpcode::DclGlobalFlags);
      ENUM_NAME(DxbcOpcode::Reserved0);
      ENUM_NAME(DxbcOpcode::Lod);
      ENUM_NAME(DxbcOpcode::Gather4);
      ENUM_NAME(DxbcOpcode::SamplePos);
      ENUM_NAME(DxbcOpcode::SampleInfo);
      ENUM_NAME(DxbcOpcode::Reserved1);
      ENUM_NAME(DxbcOpcode::HsDecls);
      ENUM_NAME(DxbcOpcode::HsControlPointPhase);
      ENUM_NAME(DxbcOpcode::HsForkPhase);
      ENUM_NAME(DxbcOpcode::HsJoinPhase);
      ENUM_NAME(DxbcOpcode::EmitStream);
      ENUM_NAME(DxbcOpcode::CutStream);
      ENUM_NAME(DxbcOpcode::EmitThenCutStream);
      ENUM_NAME(DxbcOpcode::InterfaceCall);
      ENUM_NAME(DxbcOpcode::BufInfo);
      ENUM_NAME(DxbcOpcode::DerivRtxCoarse);
      ENUM_NAME(DxbcOpcode::DerivRtxFine);
      ENUM_NAME(DxbcOpcode::DerivRtyCoarse);
      ENUM_NAME(DxbcOpcode::DerivRtyFine);
      ENUM_NAME(DxbcOpcode::Gather4C);
      ENUM_NAME(DxbcOpcode::Gather4Po);
      ENUM_NAME(DxbcOpcode::Gather4PoC);
      ENUM_NAME(DxbcOpcode::Rcp);
      ENUM_NAME(DxbcOpcode::F32toF16);
      ENUM_NAME(DxbcOpcode::F16toF32);
      ENUM_NAME(DxbcOpcode::UAddc);
      ENUM_NAME(DxbcOpcode::USubb);
      ENUM_NAME(DxbcOpcode::CountBits);
      ENUM_NAME(DxbcOpcode::FirstBitHi);
      ENUM_NAME(DxbcOpcode::FirstBitLo);
      ENUM_NAME(DxbcOpcode::FirstBitShi);
      ENUM_NAME(DxbcOpcode::UBfe);
      ENUM_NAME(DxbcOpcode::IBfe);
      ENUM_NAME(DxbcOpcode::Bfi);
      ENUM_NAME(DxbcOpcode::BfRev);
      ENUM_NAME(DxbcOpcode::Swapc);
      ENUM_NAME(DxbcOpcode::DclStream);
      ENUM_NAME(DxbcOpcode::DclFunctionBody);
      ENUM_NAME(DxbcOpcode::DclFunctionTable);
      ENUM_NAME(DxbcOpcode::DclInterface);
      ENUM_NAME(DxbcOpcode::DclInputControlPointCount);
      ENUM_NAME(DxbcOpcode::DclOutputControlPointCount);
      ENUM_NAME(DxbcOpcode::DclTessDomain);
      ENUM_NAME(DxbcOpcode::DclTessPartitioning);
      ENUM_NAME(DxbcOpcode::DclTessOutputPrimitive);
      ENUM_NAME(DxbcOpcode::DclHsMaxTessFactor);
      ENUM_NAME(DxbcOpcode::DclHsForkPhaseInstanceCount);
      ENUM_NAME(DxbcOpcode::DclHsJoinPhaseInstanceCount);
      ENUM_NAME(DxbcOpcode::DclThreadGroup);
      ENUM_NAME(DxbcOpcode::DclUavTyped);
      ENUM_NAME(DxbcOpcode::DclUavRaw);
      ENUM_NAME(DxbcOpcode::DclUavStructured);
      ENUM_NAME(DxbcOpcode::DclThreadGroupSharedMemoryRaw);
      ENUM_NAME(DxbcOpcode::DclThreadGroupSharedMemoryStructured);
      ENUM_NAME(DxbcOpcode::DclResourceRaw);
      ENUM_NAME(DxbcOpcode::DclResourceStructured);
      ENUM_NAME(DxbcOpcode::LdUavTyped);
      ENUM_NAME(DxbcOpcode::StoreUavTyped);
      ENUM_NAME(DxbcOpcode::LdRaw);
      ENUM_NAME(DxbcOpcode::StoreRaw);
      ENUM_NAME(DxbcOpcode::LdStructured);
      ENUM_NAME(DxbcOpcode::StoreStructured);
      ENUM_NAME(DxbcOpcode::AtomicAnd);
      ENUM_NAME(DxbcOpcode::AtomicOr);
      ENUM_NAME(DxbcOpcode::AtomicXor);
      ENUM_NAME(DxbcOpcode::AtomicCmpStore);
      ENUM_NAME(DxbcOpcode::AtomicIAdd);
      ENUM_NAME(DxbcOpcode::AtomicIMax);
      ENUM_NAME(DxbcOpcode::AtomicIMin);
      ENUM_NAME(DxbcOpcode::AtomicUMax);
      ENUM_NAME(DxbcOpcode::AtomicUMin);
      ENUM_NAME(DxbcOpcode::ImmAtomicAlloc);
      ENUM_NAME(DxbcOpcode::ImmAtomicConsume);
      ENUM_NAME(DxbcOpcode::ImmAtomicIAdd);
      ENUM_NAME(DxbcOpcode::ImmAtomicAnd);
      ENUM_NAME(DxbcOpcode::ImmAtomicOr);
      ENUM_NAME(DxbcOpcode::ImmAtomicXor);
      ENUM_NAME(DxbcOpcode::ImmAtomicExch);
      ENUM_NAME(DxbcOpcode::ImmAtomicCmpExch);
      ENUM_NAME(DxbcOpcode::ImmAtomicIMax);
      ENUM_NAME(DxbcOpcode::ImmAtomicIMin);
      ENUM_NAME(DxbcOpcode::ImmAtomicUMax);
      ENUM_NAME(DxbcOpcode::ImmAtomicUMin);
      ENUM_NAME(DxbcOpcode::Sync);
      ENUM_NAME(DxbcOpcode::DAdd);
      ENUM_NAME(DxbcOpcode::DMax);
      ENUM_NAME(DxbcOpcode::DMin);
      ENUM_NAME(DxbcOpcode::DMul);
      ENUM_NAME(DxbcOpcode::DEq);
      ENUM_NAME(DxbcOpcode::DGe);
      ENUM_NAME(DxbcOpcode::DLt);
      ENUM_NAME(DxbcOpcode::DNe);
      ENUM_NAME(DxbcOpcode::DMov);
      ENUM_NAME(DxbcOpcode::DMovc);
      ENUM_NAME(DxbcOpcode::DtoF);
      ENUM_NAME(DxbcOpcode::FtoD);
      ENUM_NAME(DxbcOpcode::EvalSnapped);
      ENUM_NAME(DxbcOpcode::EvalSampleIndex);
      ENUM_NAME(DxbcOpcode::EvalCentroid);
      ENUM_NAME(DxbcOpcode::DclGsInstanceCount);
      ENUM_DEFAULT(e);
    }
  }
  
  
  std::ostream& operator << (std::ostream& os, DxbcExtOpcode e) {
    switch (e) {
      ENUM_NAME(DxbcExtOpcode::Empty);
      ENUM_NAME(DxbcExtOpcode::SampleControls);
      ENUM_NAME(DxbcExtOpcode::ResourceDim);
      ENUM_NAME(DxbcExtOpcode::ResourceReturnType);
      ENUM_DEFAULT(e);
    }
  }
  
  
  std::ostream& operator << (std::ostream& os, DxbcOperandType e) {
    switch (e) {
      ENUM_NAME(DxbcOperandType::Temp);
      ENUM_NAME(DxbcOperandType::Input);
      ENUM_NAME(DxbcOperandType::Output);
      ENUM_NAME(DxbcOperandType::IndexableTemp);
      ENUM_NAME(DxbcOperandType::Imm32);
      ENUM_NAME(DxbcOperandType::Imm64);
      ENUM_NAME(DxbcOperandType::Sampler);
      ENUM_NAME(DxbcOperandType::Resource);
      ENUM_NAME(DxbcOperandType::ConstantBuffer);
      ENUM_NAME(DxbcOperandType::ImmediateConstantBuffer);
      ENUM_NAME(DxbcOperandType::Label);
      ENUM_NAME(DxbcOperandType::InputPrimitiveId);
      ENUM_NAME(DxbcOperandType::OutputDepth);
      ENUM_NAME(DxbcOperandType::Null);
      ENUM_NAME(DxbcOperandType::Rasterizer);
      ENUM_NAME(DxbcOperandType::OutputCoverageMask);
      ENUM_NAME(DxbcOperandType::Stream);
      ENUM_NAME(DxbcOperandType::FunctionBody);
      ENUM_NAME(DxbcOperandType::FunctionTable);
      ENUM_NAME(DxbcOperandType::Interface);
      ENUM_NAME(DxbcOperandType::FunctionInput);
      ENUM_NAME(DxbcOperandType::FunctionOutput);
      ENUM_NAME(DxbcOperandType::OutputControlPointId);
      ENUM_NAME(DxbcOperandType::InputForkInstanceId);
      ENUM_NAME(DxbcOperandType::InputJoinInstanceId);
      ENUM_NAME(DxbcOperandType::InputControlPoint);
      ENUM_NAME(DxbcOperandType::OutputControlPoint);
      ENUM_NAME(DxbcOperandType::InputPatchConstant);
      ENUM_NAME(DxbcOperandType::InputDomainPoint);
      ENUM_NAME(DxbcOperandType::ThisPointer);
      ENUM_NAME(DxbcOperandType::UnorderedAccessView);
      ENUM_NAME(DxbcOperandType::ThreadGroupSharedMemory);
      ENUM_NAME(DxbcOperandType::InputThreadId);
      ENUM_NAME(DxbcOperandType::InputThreadGroupId);
      ENUM_NAME(DxbcOperandType::InputThreadIdInGroup);
      ENUM_NAME(DxbcOperandType::InputCoverageMask);
      ENUM_NAME(DxbcOperandType::InputThreadIndexInGroup);
      ENUM_NAME(DxbcOperandType::InputGsInstanceId);
      ENUM_NAME(DxbcOperandType::OutputDepthGe);
      ENUM_NAME(DxbcOperandType::OutputDepthLe);
      ENUM_NAME(DxbcOperandType::CycleCounter);
      ENUM_DEFAULT(e);
    }
  }
  
  
  std::ostream& operator << (std::ostream& os, dxvk::DxbcOperandExt e) {
    switch (e) {
      ENUM_NAME(DxbcOperandExt::OperandModifier);
      ENUM_DEFAULT(e);
    }
  }
  
  
  std::ostream& operator << (std::ostream& os, DxbcComponentCount e) {
    switch (e) {
      ENUM_NAME(DxbcComponentCount::Component0);
      ENUM_NAME(DxbcComponentCount::Component1);
      ENUM_NAME(DxbcComponentCount::Component4);
      ENUM_DEFAULT(e);
    }
  }
  
  
  std::ostream& operator << (std::ostream& os, DxbcRegMode e) {
    switch (e) {
      ENUM_NAME(DxbcRegMode::Mask);
      ENUM_NAME(DxbcRegMode::Swizzle);
      ENUM_NAME(DxbcRegMode::Select1);
      ENUM_DEFAULT(e);
    }
  }
  
  
  std::ostream& operator << (std::ostream& os, DxbcOperandIndexRepresentation e) {
    switch (e) {
      ENUM_NAME(DxbcOperandIndexRepresentation::Imm32);
      ENUM_NAME(DxbcOperandIndexRepresentation::Imm64);
      ENUM_NAME(DxbcOperandIndexRepresentation::Relative);
      ENUM_NAME(DxbcOperandIndexRepresentation::Imm32Relative);
      ENUM_NAME(DxbcOperandIndexRepresentation::Imm64Relative);
      ENUM_DEFAULT(e);
    }
  }
  
  
  std::ostream& operator << (std::ostream& os, DxbcResourceDim e) {
    switch (e) {
      ENUM_NAME(DxbcResourceDim::Unknown);
      ENUM_NAME(DxbcResourceDim::Buffer);
      ENUM_NAME(DxbcResourceDim::Texture1D);
      ENUM_NAME(DxbcResourceDim::Texture2D);
      ENUM_NAME(DxbcResourceDim::Texture2DMs);
      ENUM_NAME(DxbcResourceDim::Texture3D);
      ENUM_NAME(DxbcResourceDim::TextureCube);
      ENUM_NAME(DxbcResourceDim::Texture1DArr);
      ENUM_NAME(DxbcResourceDim::Texture2DArr);
      ENUM_NAME(DxbcResourceDim::Texture2DMsArr);
      ENUM_NAME(DxbcResourceDim::TextureCubeArr);
      ENUM_NAME(DxbcResourceDim::RawBuffer);
      ENUM_NAME(DxbcResourceDim::StructuredBuffer);
      ENUM_DEFAULT(e);
    }
  }
  
  
  std::ostream& operator << (std::ostream& os, DxbcResourceReturnType e) {
    switch (e) {
      ENUM_NAME(DxbcResourceReturnType::Unorm);
      ENUM_NAME(DxbcResourceReturnType::Snorm);
      ENUM_NAME(DxbcResourceReturnType::Sint);
      ENUM_NAME(DxbcResourceReturnType::Uint);
      ENUM_NAME(DxbcResourceReturnType::Float);
      ENUM_NAME(DxbcResourceReturnType::Mixed);
      ENUM_NAME(DxbcResourceReturnType::Double);
      ENUM_NAME(DxbcResourceReturnType::Continued);
      ENUM_NAME(DxbcResourceReturnType::Unused);
      ENUM_DEFAULT(e);
    }
  }
  
  
  std::ostream& operator << (std::ostream& os, DxbcRegisterComponentType e) {
    switch (e) {
      ENUM_NAME(DxbcRegisterComponentType::Unknown);
      ENUM_NAME(DxbcRegisterComponentType::Uint32);
      ENUM_NAME(DxbcRegisterComponentType::Sint32);
      ENUM_NAME(DxbcRegisterComponentType::Float32);
      ENUM_DEFAULT(e);
    }
  }
  
  
  std::ostream& operator << (std::ostream& os, DxbcInstructionReturnType e) {
    switch (e) {
      ENUM_NAME(DxbcInstructionReturnType::Float);
      ENUM_NAME(DxbcInstructionReturnType::Uint);
      ENUM_DEFAULT(e);
    }
  }
  
  
  std::ostream& operator << (std::ostream& os, DxbcSystemValue e) {
    switch (e) {
      ENUM_NAME(DxbcSystemValue::None);
      ENUM_NAME(DxbcSystemValue::Position);
      ENUM_NAME(DxbcSystemValue::ClipDistance);
      ENUM_NAME(DxbcSystemValue::CullDistance);
      ENUM_NAME(DxbcSystemValue::RenderTargetId);
      ENUM_NAME(DxbcSystemValue::ViewportId);
      ENUM_NAME(DxbcSystemValue::VertexId);
      ENUM_NAME(DxbcSystemValue::PrimitiveId);
      ENUM_NAME(DxbcSystemValue::InstanceId);
      ENUM_NAME(DxbcSystemValue::IsFrontFace);
      ENUM_NAME(DxbcSystemValue::SampleIndex);
      ENUM_NAME(DxbcSystemValue::FinalQuadUeq0EdgeTessFactor);
      ENUM_NAME(DxbcSystemValue::FinalQuadVeq0EdgeTessFactor);
      ENUM_NAME(DxbcSystemValue::FinalQuadUeq1EdgeTessFactor);
      ENUM_NAME(DxbcSystemValue::FinalQuadVeq1EdgeTessFactor);
      ENUM_NAME(DxbcSystemValue::FinalQuadUInsideTessFactor);
      ENUM_NAME(DxbcSystemValue::FinalQuadVInsideTessFactor);
      ENUM_NAME(DxbcSystemValue::FinalTriUeq0EdgeTessFactor);
      ENUM_NAME(DxbcSystemValue::FinalTriVeq0EdgeTessFactor);
      ENUM_NAME(DxbcSystemValue::FinalTriWeq0EdgeTessFactor);
      ENUM_NAME(DxbcSystemValue::FinalTriInsideTessFactor);
      ENUM_NAME(DxbcSystemValue::FinalLineDetailTessFactor);
      ENUM_NAME(DxbcSystemValue::FinalLineDensityTessFactor);
      ENUM_NAME(DxbcSystemValue::Target);
      ENUM_NAME(DxbcSystemValue::Depth);
      ENUM_NAME(DxbcSystemValue::Coverage);
      ENUM_NAME(DxbcSystemValue::DepthGe);
      ENUM_NAME(DxbcSystemValue::DepthLe);
      ENUM_DEFAULT(e);
    }
  }
  
  
  std::ostream& operator << (std::ostream& os, dxvk::DxbcProgramType e) {
    switch (e) {
      ENUM_NAME(DxbcProgramType::PixelShader);
      ENUM_NAME(DxbcProgramType::VertexShader);
      ENUM_NAME(DxbcProgramType::GeometryShader);
      ENUM_NAME(DxbcProgramType::HullShader);
      ENUM_NAME(DxbcProgramType::DomainShader);
      ENUM_NAME(DxbcProgramType::ComputeShader);
      ENUM_DEFAULT(e);
    }
  }
  
  std::ostream& operator << (std::ostream& os, dxvk::DxbcCustomDataClass e) {
    switch (e) {
      ENUM_NAME(DxbcCustomDataClass::Comment);
      ENUM_NAME(DxbcCustomDataClass::DebugInfo);
      ENUM_NAME(DxbcCustomDataClass::Opaque);
      ENUM_NAME(DxbcCustomDataClass::ImmConstBuf);
      ENUM_DEFAULT(e);
    }
  }

  std::ostream& operator << (std::ostream& os, dxvk::DxbcScalarType e) {
    switch (e) {
      ENUM_NAME(DxbcScalarType::Uint32);
      ENUM_NAME(DxbcScalarType::Uint64);
      ENUM_NAME(DxbcScalarType::Sint32);
      ENUM_NAME(DxbcScalarType::Sint64);
      ENUM_NAME(DxbcScalarType::Float32);
      ENUM_NAME(DxbcScalarType::Float64);
      ENUM_NAME(DxbcScalarType::Bool);
      ENUM_DEFAULT(e);
    }
  }


} //namespace dxvk
