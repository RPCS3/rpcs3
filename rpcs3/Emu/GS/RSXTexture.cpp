#include "stdafx.h"
#include "RSXThread.h"

RSXTexture::RSXTexture()
  : m_width(0)
  , m_height(0)
  , m_enabled(false)
  , m_minlod(0)
  , m_maxlod(1000)
  , m_maxaniso(0)
  , m_wraps(1)
  , m_wrapt(1)
  , m_wrapr(3)
  , m_zfunc(0)
  , m_gamma(0)
  , m_bias(0)
  , m_remap(0xE4)
  , m_min_filter(1)
  , m_mag_filter(2)
{
  m_index = 0;
}

RSXTexture::RSXTexture(u8 index)
  : m_width(0)
  , m_height(0)
  , m_enabled(false)
  , m_minlod(0)
  , m_maxlod(1000)
  , m_maxaniso(0)
  , m_wraps(1)
  , m_wrapt(1)
  , m_wrapr(3)
  , m_zfunc(0)
  , m_gamma(0)
  , m_bias(0)
  , m_remap(0xE4)
  , m_min_filter(1)
  , m_mag_filter(2)
{
  m_index = index;
}

u32 RSXTexture::GetOffset() const
{
  return methodRegisters[NV4097_SET_TEXTURE_OFFSET + (m_index*4)];
}

u8 RSXTexture::GetLocation() const
{
  return (methodRegisters[NV4097_SET_TEXTURE_FORMAT + (m_index*4)] & 0x3) - 1;
}

bool RSXTexture::isCubemap() const
{
  return ((methodRegisters[NV4097_SET_TEXTURE_FORMAT + (m_index*4)] >> 2) & 0x1);
}

u8 RSXTexture::GetBorderType() const
{
  return ((methodRegisters[NV4097_SET_TEXTURE_FORMAT + (m_index*4)] >> 3) & 0x1);
}

u8 RSXTexture::GetDimension() const
{
  return ((methodRegisters[NV4097_SET_TEXTURE_FORMAT + (m_index*4)] >> 4) & 0xf);
}

u8 RSXTexture::GetFormat() const
{
  return ((methodRegisters[NV4097_SET_TEXTURE_FORMAT + (m_index*4)] >> 8) & 0xff);
}

u16 RSXTexture::Getmipmap() const
{
  return ((methodRegisters[NV4097_SET_TEXTURE_FORMAT + (m_index*4)] >> 16) & 0xffff);
}

void RSXTexture::SetRect(const u32 width, const u32 height)
{
  m_width = width;
  m_height = height;
}

void RSXTexture::SetAddress(u8 wraps, u8 wrapt, u8 wrapr, u8 unsigned_remap, u8 zfunc, u8 gamma, u8 aniso_bias, u8 signed_remap)
{
  m_wraps = wraps;
  m_wrapt = wrapt;
  m_wrapr = wrapr;
  m_unsigned_remap = unsigned_remap;
  m_zfunc = zfunc;
  m_gamma = gamma;
  m_aniso_bias = aniso_bias;
  m_signed_remap = signed_remap;
}

void RSXTexture::SetControl0(const bool enable, const u16 minlod, const u16 maxlod, const u8 maxaniso)
{
  m_enabled = enable;
  m_minlod = minlod;
  m_maxlod = maxlod;
  m_maxaniso = maxaniso;
}

void RSXTexture::SetControl1(u32 remap)
{
  m_remap = remap;
}

void RSXTexture::SetControl3(u16 depth, u32 pitch)
{
  m_depth = depth;
  m_pitch = pitch;
}

void RSXTexture::SetFilter(u16 bias, u8 min, u8 mag, u8 conv, u8 a_signed, u8 r_signed, u8 g_signed, u8 b_signed)
{
  m_bias = bias;
  m_min_filter = min;
  m_mag_filter = mag;
  m_conv = conv;
  m_a_signed = a_signed;
  m_r_signed = r_signed;
  m_g_signed = g_signed;
  m_b_signed = b_signed;
}

wxSize RSXTexture::GetRect() const
{
  return wxSize(m_width, m_height);
}

bool RSXTexture::IsEnabled() const
{
  return m_enabled;
}