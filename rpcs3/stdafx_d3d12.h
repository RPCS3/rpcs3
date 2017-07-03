#pragma once

#ifdef _MSC_VER

// Must be included first
#include <d3dcompiler.h>

#include <d3d12.h>
#include "Emu/RSX/D3D12/D3D12Utils.h"
#include "Emu/RSX/D3D12/D3D12Formats.h"
#include "Emu/RSX/D3D12/D3D12GSRender.h"

#pragma comment(lib, "d2d1")
#pragma comment(lib, "DXGI")
#pragma comment(lib, "Dwrite")

#endif
