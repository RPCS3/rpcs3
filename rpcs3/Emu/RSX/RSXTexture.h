#pragma once

class RSXTexture
{
protected:
	u8 m_index;

public:
	u32 m_pitch;
	u16 m_depth;

public:
	RSXTexture();
	RSXTexture(u8 index);
	virtual void Init();

	// Offset
	virtual u32 GetOffset() const;

	// Format
	virtual u8   GetLocation() const;
	virtual bool isCubemap() const;
	virtual u8   GetBorderType() const;
	virtual u8   GetDimension() const;
	virtual u8   GetFormat() const;
	virtual u16  GetMipmap() const;

	// Address
	virtual u8 GetWrapS() const;
	virtual u8 GetWrapT() const;
	virtual u8 GetWrapR() const;
	virtual u8 GetUnsignedRemap() const;
	virtual u8 GetZfunc() const;
	virtual u8 GetGamma() const;
	virtual u8 GetAnisoBias() const;
	virtual u8 GetSignedRemap() const;

	// Control0
	virtual bool IsEnabled() const;
	virtual u16  GetMinLOD() const;
	virtual u16  GetMaxLOD() const;
	virtual u8   GetMaxAniso() const;
	virtual bool IsAlphaKillEnabled() const;

	// Control1
	virtual u32 GetRemap() const;

	// Filter
	virtual u16 GetBias() const;
	virtual u8  GetMinFilter() const;
	virtual u8  GetMagFilter() const;
	virtual u8  GetConvolutionFilter() const;
	virtual bool isASigned() const;
	virtual bool isRSigned() const;
	virtual bool isGSigned() const;
	virtual bool isBSigned() const;

	// Image Rect
	virtual u16 GetWidth() const;
	virtual u16 GetHeight() const;

	// Border Color
	virtual u32 GetBorderColor() const;

	void SetControl3(u16 depth, u32 pitch);
};

class RSXVertexTexture : public RSXTexture
{
public:
	RSXVertexTexture();
	RSXVertexTexture(u8 index);
	void Init();

	// Offset
	u32 GetOffset() const;

	// Format
	u8   GetLocation() const;
	bool isCubemap() const;
	u8   GetBorderType() const;
	u8   GetDimension() const;
	u8   GetFormat() const;
	u16  GetMipmap() const;

	// Address
	u8 GetWrapS() const;
	u8 GetWrapT() const;
	u8 GetWrapR() const;
	u8 GetUnsignedRemap() const;
	u8 GetZfunc() const;
	u8 GetGamma() const;
	u8 GetAnisoBias() const;
	u8 GetSignedRemap() const;

	// Control0
	bool IsEnabled() const;
	u16  GetMinLOD() const;
	u16  GetMaxLOD() const;
	u8   GetMaxAniso() const;
	bool IsAlphaKillEnabled() const;

	// Control1
	u32 GetRemap() const;

	// Filter
	u16 GetBias() const;
	u8  GetMinFilter() const;
	u8  GetMagFilter() const;
	u8  GetConvolutionFilter() const;
	bool isASigned() const;
	bool isRSigned() const;
	bool isGSigned() const;
	bool isBSigned() const;

	// Image Rect
	u16 GetWidth() const;
	u16 GetHeight() const;

	// Border Color
	u32 GetBorderColor() const;
};