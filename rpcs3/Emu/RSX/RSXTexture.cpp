#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "RSXThread.h"
#include "RSXThread.h"
#include "RSXTexture.h"

RSXTexture::RSXTexture()
{
	m_index = 0;
}

RSXTexture::RSXTexture(u8 index)
{
	m_index = index;
}

void RSXTexture::Init()
{
	// Offset
	methodRegisters[NV4097_SET_TEXTURE_OFFSET + (m_index*32)] = 0;

	// Format
	methodRegisters[NV4097_SET_TEXTURE_FORMAT + (m_index*32)] = 0;

	// Address
	methodRegisters[NV4097_SET_TEXTURE_ADDRESS + (m_index*32)] =
		((/*wraps*/1) | ((/*anisoBias*/0) << 4) | ((/*wrapt*/1) << 8) | ((/*unsignedRemap*/0) << 12) | 
		((/*wrapr*/3) << 16) | ((/*gamma*/0) << 20) |((/*signedRemap*/0) << 24) | ((/*zfunc*/0) << 28));

	// Control0
	methodRegisters[NV4097_SET_TEXTURE_CONTROL0 + (m_index*32)] =
		(((/*alphakill*/0) << 2) | (/*maxaniso*/0) << 4) | ((/*maxlod*/0xc00) << 7) | ((/*minlod*/0) << 19) | ((/*enable*/0) << 31);

	// Control1
	methodRegisters[NV4097_SET_TEXTURE_CONTROL1 + (m_index*32)] = 0xE4;

	// Filter
	methodRegisters[NV4097_SET_TEXTURE_FILTER + (m_index*32)] =
		((/*bias*/0) | ((/*conv*/1) << 13) | ((/*min*/5) << 16) | ((/*mag*/2) << 24) 
		       | ((/*as*/0) << 28) | ((/*rs*/0) << 29) | ((/*gs*/0) << 30) | ((/*bs*/0) << 31) );

	// Image Rect
	methodRegisters[NV4097_SET_TEXTURE_IMAGE_RECT + (m_index*32)] = (/*height*/1) | ((/*width*/1) << 16);

	// Border Color
	methodRegisters[NV4097_SET_TEXTURE_BORDER_COLOR + (m_index*32)] = 0;
}

u32 RSXTexture::GetOffset() const
{
	return methodRegisters[NV4097_SET_TEXTURE_OFFSET + (m_index*32)];
}

u8 RSXTexture::GetLocation() const
{
	return (methodRegisters[NV4097_SET_TEXTURE_FORMAT + (m_index*32)] & 0x3) - 1;
}

bool RSXTexture::isCubemap() const
{
	return ((methodRegisters[NV4097_SET_TEXTURE_FORMAT + (m_index*32)] >> 2) & 0x1);
}

u8 RSXTexture::GetBorderType() const
{
	return ((methodRegisters[NV4097_SET_TEXTURE_FORMAT + (m_index*32)] >> 3) & 0x1);
}

u8 RSXTexture::GetDimension() const
{
	return ((methodRegisters[NV4097_SET_TEXTURE_FORMAT + (m_index*32)] >> 4) & 0xf);
}

u8 RSXTexture::GetFormat() const
{
	return ((methodRegisters[NV4097_SET_TEXTURE_FORMAT + (m_index*32)] >> 8) & 0xff);
}

u16 RSXTexture::GetMipmap() const
{
	return ((methodRegisters[NV4097_SET_TEXTURE_FORMAT + (m_index*32)] >> 16) & 0xffff);
}

u8 RSXTexture::GetWrapS() const
{
	return ((methodRegisters[NV4097_SET_TEXTURE_ADDRESS + (m_index*32)]) & 0xf);
}

u8 RSXTexture::GetWrapT() const
{
	return ((methodRegisters[NV4097_SET_TEXTURE_ADDRESS + (m_index*32)] >> 8) & 0xf);
}

u8 RSXTexture::GetWrapR() const
{
	return ((methodRegisters[NV4097_SET_TEXTURE_ADDRESS + (m_index*32)] >> 16) & 0xf);
}

u8 RSXTexture::GetUnsignedRemap() const
{
	return ((methodRegisters[NV4097_SET_TEXTURE_ADDRESS + (m_index*32)] >> 12) & 0xf);
}

u8 RSXTexture::GetZfunc() const
{
	return ((methodRegisters[NV4097_SET_TEXTURE_ADDRESS + (m_index*32)] >> 28) & 0xf);
}

u8 RSXTexture::GetGamma() const
{
	return ((methodRegisters[NV4097_SET_TEXTURE_ADDRESS + (m_index*32)] >> 20) & 0xf);
}

u8 RSXTexture::GetAnisoBias() const
{
	return ((methodRegisters[NV4097_SET_TEXTURE_ADDRESS + (m_index*32)] >> 4) & 0xf);
}

u8 RSXTexture::GetSignedRemap() const
{
	return ((methodRegisters[NV4097_SET_TEXTURE_ADDRESS + (m_index*32)] >> 24) & 0xf);
}

bool RSXTexture::IsEnabled() const
{
	return ((methodRegisters[NV4097_SET_TEXTURE_CONTROL0 + (m_index*32)] >> 31) & 0x1);
}

u16  RSXTexture::GetMinLOD() const
{
	return ((methodRegisters[NV4097_SET_TEXTURE_CONTROL0 + (m_index*32)] >> 19) & 0xfff);
}

u16  RSXTexture::GetMaxLOD() const
{
	return ((methodRegisters[NV4097_SET_TEXTURE_CONTROL0 + (m_index*32)] >> 7) & 0xfff);
}

u8   RSXTexture::GetMaxAniso() const
{
	return ((methodRegisters[NV4097_SET_TEXTURE_CONTROL0 + (m_index*32)] >> 4) & 0x7);
}

bool RSXTexture::IsAlphaKillEnabled() const
{
	return ((methodRegisters[NV4097_SET_TEXTURE_CONTROL0 + (m_index*32)] >> 2) & 0x1);
}

u32 RSXTexture::GetRemap() const
{
	return (methodRegisters[NV4097_SET_TEXTURE_CONTROL1 + (m_index*32)]);
}

u16 RSXTexture::GetBias() const
{
	return ((methodRegisters[NV4097_SET_TEXTURE_FILTER + (m_index*32)]) & 0x1fff);
}

u8  RSXTexture::GetMinFilter() const
{
	return ((methodRegisters[NV4097_SET_TEXTURE_FILTER + (m_index*32)] >> 16) & 0x7);
}

u8  RSXTexture::GetMagFilter() const
{
	return ((methodRegisters[NV4097_SET_TEXTURE_FILTER + (m_index*32)] >> 24) & 0x7);
}

u8 RSXTexture::GetConvolutionFilter() const
{
	return ((methodRegisters[NV4097_SET_TEXTURE_FILTER + (m_index*32)] >> 13) & 0xf);
}

bool RSXTexture::isASigned() const
{
	return ((methodRegisters[NV4097_SET_TEXTURE_FILTER + (m_index*32)] >> 28) & 0x1);
}

bool RSXTexture::isRSigned() const
{
	return ((methodRegisters[NV4097_SET_TEXTURE_FILTER + (m_index*32)] >> 29) & 0x1);
}

bool RSXTexture::isGSigned() const
{
	return ((methodRegisters[NV4097_SET_TEXTURE_FILTER + (m_index*32)] >> 30) & 0x1);
}

bool RSXTexture::isBSigned() const
{
	return ((methodRegisters[NV4097_SET_TEXTURE_FILTER + (m_index*32)] >> 31) & 0x1);
}

u16 RSXTexture::GetWidth() const
{
	return ((methodRegisters[NV4097_SET_TEXTURE_IMAGE_RECT + (m_index*32)] >> 16) & 0xffff);
}

u16 RSXTexture::GetHeight() const
{
	return ((methodRegisters[NV4097_SET_TEXTURE_IMAGE_RECT + (m_index*32)]) & 0xffff);
}

u32 RSXTexture::GetBorderColor() const
{
	return methodRegisters[NV4097_SET_TEXTURE_BORDER_COLOR + (m_index*32)];
}

void RSXTexture::SetControl3(u16 depth, u32 pitch)
{
	m_depth = depth;
	m_pitch = pitch;
}
