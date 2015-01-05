#pragma once
#include "Emu/RSX/GSRender.h"

class NullGSRender
	: public GSRender
{
public:

	NullGSRender() {}
	virtual ~NullGSRender() {}

private:
	virtual void OnInit() {}
	virtual void OnInitThread() {}
	virtual void OnExitThread() {}
	virtual void OnReset() {}
	virtual void Enable(u32 cmd, u32 enable) {}
	virtual void ClearColor(u32 a, u32 r, u32 g, u32 b) {}
	virtual void ClearStencil(u32 stencil) {}
	virtual void ClearDepth(u32 depth) {}
	virtual void ClearSurface(u32 mask) {}
	virtual void ColorMask(bool a, bool r, bool g, bool b) {}
	virtual void ExecCMD() {}
	virtual void AlphaFunc(u32 func, float ref) {}
	virtual void DepthFunc(u32 func) {}
	virtual void DepthMask(u32 flag) {}
	virtual void PolygonMode(u32 face, u32 mode) {}
	virtual void PointSize(float size) {}
	virtual void LogicOp(u32 opcode) {}
	virtual void LineWidth(float width) {}
	virtual void LineStipple(u16 factor, u16 pattern) {}
	virtual void PolygonStipple(u32 pattern) {}
	virtual void PrimitiveRestartIndex(u32 index) {}
	virtual void CullFace(u32 mode) {}
	virtual void FrontFace(u32 mode) {}
	virtual void Fogi(u32 mode) {}
	virtual void Fogf(float start, float end) {}
	virtual void PolygonOffset(float factor, float bias) {}
	virtual void DepthRangef(float min, float max) {}
	virtual void BlendEquationSeparate(u16 rgb, u16 a) {}
	virtual void BlendFuncSeparate(u16 srcRGB, u16 dstRGB, u16 srcAlpha, u16 dstAlpha) {}
	virtual void BlendColor(u8 r, u8 g, u8 b, u8 a) {}
	virtual void LightModeli(u32 enable) {}
	virtual void ShadeModel(u32 mode) {}
	virtual void DepthBoundsEXT(float min, float max) {}
	virtual void Scissor(u16 x, u16 y, u16 width, u16 height) {}
	virtual void StencilOp(u32 fail, u32 zfail, u32 zpass) {}
	virtual void StencilMask(u32 mask) {}
	virtual void StencilFunc(u32 func, u32 ref, u32 mask) {}
	virtual void StencilOpSeparate(u32 mode, u32 fail, u32 zfail, u32 zpass) {}
	virtual void StencilMaskSeparate(u32 mode, u32 mask) {}
	virtual void StencilFuncSeparate(u32 mode, u32 func, u32 ref, u32 mask) {}
	virtual void Flip() {}
	virtual void Close() {}
};
