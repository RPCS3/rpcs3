#pragma once

#include <ostream>

#include "dxbc_common.h"
#include "dxbc_enums.h"

namespace dxvk {

  std::ostream& operator << (std::ostream& os, DxbcOpcode e);
  std::ostream& operator << (std::ostream& os, DxbcExtOpcode e);
  std::ostream& operator << (std::ostream& os, DxbcOperandType e);
  std::ostream& operator << (std::ostream& os, DxbcOperandExt e);
  std::ostream& operator << (std::ostream& os, DxbcComponentCount e);
  std::ostream& operator << (std::ostream& os, DxbcRegMode e);
  std::ostream& operator << (std::ostream& os, DxbcOperandIndexRepresentation e);
  std::ostream& operator << (std::ostream& os, DxbcResourceDim e);
  std::ostream& operator << (std::ostream& os, DxbcResourceReturnType e);
  std::ostream& operator << (std::ostream& os, DxbcRegisterComponentType e);
  std::ostream& operator << (std::ostream& os, DxbcInstructionReturnType e);
  std::ostream& operator << (std::ostream& os, DxbcSystemValue e);
  std::ostream& operator << (std::ostream& os, DxbcProgramType e);
  std::ostream& operator << (std::ostream& os, DxbcCustomDataClass e);
  std::ostream& operator << (std::ostream& os, DxbcScalarType e);

} // namespace dxvk
