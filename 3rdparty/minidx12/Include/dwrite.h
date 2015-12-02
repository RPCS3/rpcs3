//+--------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Abstract:
//     DirectX Typography Services public API definitions.
//
//----------------------------------------------------------------------------

#ifndef DWRITE_H_INCLUDED
#define DWRITE_H_INCLUDED

#pragma once

#ifndef DWRITE_NO_WINDOWS_H

#include <specstrings.h>
#include <unknwn.h>

#endif // DWRITE_NO_WINDOWS_H

#include <dcommon.h>

#ifndef DWRITE_DECLARE_INTERFACE
#define DWRITE_DECLARE_INTERFACE(iid) DECLSPEC_UUID(iid) DECLSPEC_NOVTABLE
#endif

#ifndef DWRITE_EXPORT
#define DWRITE_EXPORT __declspec(dllimport) WINAPI
#endif

/// <summary>
/// The type of a font represented by a single font file.
/// Font formats that consist of multiple files, e.g. Type 1 .PFM and .PFB, have
/// separate enum values for each of the file type.
/// </summary>
enum DWRITE_FONT_FILE_TYPE
{
    /// <summary>
    /// Font type is not recognized by the DirectWrite font system.
    /// </summary>
    DWRITE_FONT_FILE_TYPE_UNKNOWN,

    /// <summary>
    /// OpenType font with CFF outlines.
    /// </summary>
    DWRITE_FONT_FILE_TYPE_CFF,

    /// <summary>
    /// OpenType font with TrueType outlines.
    /// </summary>
    DWRITE_FONT_FILE_TYPE_TRUETYPE,

    /// <summary>
    /// OpenType font that contains a TrueType collection.
    /// </summary>
    DWRITE_FONT_FILE_TYPE_OPENTYPE_COLLECTION,

    /// <summary>
    /// Type 1 PFM font.
    /// </summary>
    DWRITE_FONT_FILE_TYPE_TYPE1_PFM,

    /// <summary>
    /// Type 1 PFB font.
    /// </summary>
    DWRITE_FONT_FILE_TYPE_TYPE1_PFB,

    /// <summary>
    /// Vector .FON font.
    /// </summary>
    DWRITE_FONT_FILE_TYPE_VECTOR,

    /// <summary>
    /// Bitmap .FON font.
    /// </summary>
    DWRITE_FONT_FILE_TYPE_BITMAP,

    // The following name is obsolete, but kept as an alias to avoid breaking existing code.
    DWRITE_FONT_FILE_TYPE_TRUETYPE_COLLECTION = DWRITE_FONT_FILE_TYPE_OPENTYPE_COLLECTION,
};

/// <summary>
/// The file format of a complete font face.
/// Font formats that consist of multiple files, e.g. Type 1 .PFM and .PFB, have
/// a single enum entry.
/// </summary>
enum DWRITE_FONT_FACE_TYPE
{
    /// <summary>
    /// OpenType font face with CFF outlines.
    /// </summary>
    DWRITE_FONT_FACE_TYPE_CFF,

    /// <summary>
    /// OpenType font face with TrueType outlines.
    /// </summary>
    DWRITE_FONT_FACE_TYPE_TRUETYPE,

    /// <summary>
    /// OpenType font face that is a part of a TrueType or CFF collection.
    /// </summary>
    DWRITE_FONT_FACE_TYPE_OPENTYPE_COLLECTION,

    /// <summary>
    /// A Type 1 font face.
    /// </summary>
    DWRITE_FONT_FACE_TYPE_TYPE1,

    /// <summary>
    /// A vector .FON format font face.
    /// </summary>
    DWRITE_FONT_FACE_TYPE_VECTOR,

    /// <summary>
    /// A bitmap .FON format font face.
    /// </summary>
    DWRITE_FONT_FACE_TYPE_BITMAP,

    /// <summary>
    /// Font face type is not recognized by the DirectWrite font system.
    /// </summary>
    DWRITE_FONT_FACE_TYPE_UNKNOWN,

    /// <summary>
    /// The font data includes only the CFF table from an OpenType CFF font.
    /// This font face type can be used only for embedded fonts (i.e., custom
    /// font file loaders) and the resulting font face object supports only the
    /// minimum functionality necessary to render glyphs.
    /// </summary>
    DWRITE_FONT_FACE_TYPE_RAW_CFF,

    // The following name is obsolete, but kept as an alias to avoid breaking existing code.
    DWRITE_FONT_FACE_TYPE_TRUETYPE_COLLECTION = DWRITE_FONT_FACE_TYPE_OPENTYPE_COLLECTION,
};

/// <summary>
/// Specifies algorithmic style simulations to be applied to the font face.
/// Bold and oblique simulations can be combined via bitwise OR operation.
/// </summary>
enum DWRITE_FONT_SIMULATIONS
{
    /// <summary>
    /// No simulations are performed.
    /// </summary>
    DWRITE_FONT_SIMULATIONS_NONE    = 0x0000,

    /// <summary>
    /// Algorithmic emboldening is performed.
    /// </summary>
    DWRITE_FONT_SIMULATIONS_BOLD    = 0x0001,

    /// <summary>
    /// Algorithmic italicization is performed.
    /// </summary>
    DWRITE_FONT_SIMULATIONS_OBLIQUE = 0x0002
};

#ifdef DEFINE_ENUM_FLAG_OPERATORS
DEFINE_ENUM_FLAG_OPERATORS(DWRITE_FONT_SIMULATIONS);
#endif

/// <summary>
/// The font weight enumeration describes common values for degree of blackness or thickness of strokes of characters in a font.
/// Font weight values less than 1 or greater than 999 are considered to be invalid, and they are rejected by font API functions.
/// </summary>
enum DWRITE_FONT_WEIGHT
{
    /// <summary>
    /// Predefined font weight : Thin (100).
    /// </summary>
    DWRITE_FONT_WEIGHT_THIN = 100,

    /// <summary>
    /// Predefined font weight : Extra-light (200).
    /// </summary>
    DWRITE_FONT_WEIGHT_EXTRA_LIGHT = 200,

    /// <summary>
    /// Predefined font weight : Ultra-light (200).
    /// </summary>
    DWRITE_FONT_WEIGHT_ULTRA_LIGHT = 200,

    /// <summary>
    /// Predefined font weight : Light (300).
    /// </summary>
    DWRITE_FONT_WEIGHT_LIGHT = 300,

    /// <summary>
    /// Predefined font weight : Semi-light (350).
    /// </summary>
    DWRITE_FONT_WEIGHT_SEMI_LIGHT = 350,

    /// <summary>
    /// Predefined font weight : Normal (400).
    /// </summary>
    DWRITE_FONT_WEIGHT_NORMAL = 400,

    /// <summary>
    /// Predefined font weight : Regular (400).
    /// </summary>
    DWRITE_FONT_WEIGHT_REGULAR = 400,

    /// <summary>
    /// Predefined font weight : Medium (500).
    /// </summary>
    DWRITE_FONT_WEIGHT_MEDIUM = 500,

    /// <summary>
    /// Predefined font weight : Demi-bold (600).
    /// </summary>
    DWRITE_FONT_WEIGHT_DEMI_BOLD = 600,

    /// <summary>
    /// Predefined font weight : Semi-bold (600).
    /// </summary>
    DWRITE_FONT_WEIGHT_SEMI_BOLD = 600,

    /// <summary>
    /// Predefined font weight : Bold (700).
    /// </summary>
    DWRITE_FONT_WEIGHT_BOLD = 700,

    /// <summary>
    /// Predefined font weight : Extra-bold (800).
    /// </summary>
    DWRITE_FONT_WEIGHT_EXTRA_BOLD = 800,

    /// <summary>
    /// Predefined font weight : Ultra-bold (800).
    /// </summary>
    DWRITE_FONT_WEIGHT_ULTRA_BOLD = 800,

    /// <summary>
    /// Predefined font weight : Black (900).
    /// </summary>
    DWRITE_FONT_WEIGHT_BLACK = 900,

    /// <summary>
    /// Predefined font weight : Heavy (900).
    /// </summary>
    DWRITE_FONT_WEIGHT_HEAVY = 900,

    /// <summary>
    /// Predefined font weight : Extra-black (950).
    /// </summary>
    DWRITE_FONT_WEIGHT_EXTRA_BLACK = 950,

    /// <summary>
    /// Predefined font weight : Ultra-black (950).
    /// </summary>
    DWRITE_FONT_WEIGHT_ULTRA_BLACK = 950
};

/// <summary>
/// The font stretch enumeration describes relative change from the normal aspect ratio
/// as specified by a font designer for the glyphs in a font.
/// Values less than 1 or greater than 9 are considered to be invalid, and they are rejected by font API functions.
/// </summary>
enum DWRITE_FONT_STRETCH
{
    /// <summary>
    /// Predefined font stretch : Not known (0).
    /// </summary>
    DWRITE_FONT_STRETCH_UNDEFINED = 0,

    /// <summary>
    /// Predefined font stretch : Ultra-condensed (1).
    /// </summary>
    DWRITE_FONT_STRETCH_ULTRA_CONDENSED = 1,

    /// <summary>
    /// Predefined font stretch : Extra-condensed (2).
    /// </summary>
    DWRITE_FONT_STRETCH_EXTRA_CONDENSED = 2,

    /// <summary>
    /// Predefined font stretch : Condensed (3).
    /// </summary>
    DWRITE_FONT_STRETCH_CONDENSED = 3,

    /// <summary>
    /// Predefined font stretch : Semi-condensed (4).
    /// </summary>
    DWRITE_FONT_STRETCH_SEMI_CONDENSED = 4,

    /// <summary>
    /// Predefined font stretch : Normal (5).
    /// </summary>
    DWRITE_FONT_STRETCH_NORMAL = 5,

    /// <summary>
    /// Predefined font stretch : Medium (5).
    /// </summary>
    DWRITE_FONT_STRETCH_MEDIUM = 5,

    /// <summary>
    /// Predefined font stretch : Semi-expanded (6).
    /// </summary>
    DWRITE_FONT_STRETCH_SEMI_EXPANDED = 6,

    /// <summary>
    /// Predefined font stretch : Expanded (7).
    /// </summary>
    DWRITE_FONT_STRETCH_EXPANDED = 7,

    /// <summary>
    /// Predefined font stretch : Extra-expanded (8).
    /// </summary>
    DWRITE_FONT_STRETCH_EXTRA_EXPANDED = 8,

    /// <summary>
    /// Predefined font stretch : Ultra-expanded (9).
    /// </summary>
    DWRITE_FONT_STRETCH_ULTRA_EXPANDED = 9
};

/// <summary>
/// The font style enumeration describes the slope style of a font face, such as Normal, Italic or Oblique.
/// Values other than the ones defined in the enumeration are considered to be invalid, and they are rejected by font API functions.
/// </summary>
enum DWRITE_FONT_STYLE
{
    /// <summary>
    /// Font slope style : Normal.
    /// </summary>
    DWRITE_FONT_STYLE_NORMAL,

    /// <summary>
    /// Font slope style : Oblique.
    /// </summary>
    DWRITE_FONT_STYLE_OBLIQUE,

    /// <summary>
    /// Font slope style : Italic.
    /// </summary>
    DWRITE_FONT_STYLE_ITALIC

};

/// <summary>
/// The informational string enumeration identifies a string in a font.
/// </summary>
enum DWRITE_INFORMATIONAL_STRING_ID
{
    /// <summary>
    /// Unspecified name ID.
    /// </summary>
    DWRITE_INFORMATIONAL_STRING_NONE,

    /// <summary>
    /// Copyright notice provided by the font.
    /// </summary>
    DWRITE_INFORMATIONAL_STRING_COPYRIGHT_NOTICE,

    /// <summary>
    /// String containing a version number.
    /// </summary>
    DWRITE_INFORMATIONAL_STRING_VERSION_STRINGS,

    /// <summary>
    /// Trademark information provided by the font.
    /// </summary>
    DWRITE_INFORMATIONAL_STRING_TRADEMARK,

    /// <summary>
    /// Name of the font manufacturer.
    /// </summary>
    DWRITE_INFORMATIONAL_STRING_MANUFACTURER,

    /// <summary>
    /// Name of the font designer.
    /// </summary>
    DWRITE_INFORMATIONAL_STRING_DESIGNER,

    /// <summary>
    /// URL of font designer (with protocol, e.g., http://, ftp://).
    /// </summary>
    DWRITE_INFORMATIONAL_STRING_DESIGNER_URL,

    /// <summary>
    /// Description of the font. Can contain revision information, usage recommendations, history, features, etc.
    /// </summary>
    DWRITE_INFORMATIONAL_STRING_DESCRIPTION,

    /// <summary>
    /// URL of font vendor (with protocol, e.g., http://, ftp://). If a unique serial number is embedded in the URL, it can be used to register the font.
    /// </summary>
    DWRITE_INFORMATIONAL_STRING_FONT_VENDOR_URL,

    /// <summary>
    /// Description of how the font may be legally used, or different example scenarios for licensed use. This field should be written in plain language, not legalese.
    /// </summary>
    DWRITE_INFORMATIONAL_STRING_LICENSE_DESCRIPTION,

    /// <summary>
    /// URL where additional licensing information can be found.
    /// </summary>
    DWRITE_INFORMATIONAL_STRING_LICENSE_INFO_URL,

    /// <summary>
    /// GDI-compatible family name. Because GDI allows a maximum of four fonts per family, fonts in the same family may have different GDI-compatible family names
    /// (e.g., "Arial", "Arial Narrow", "Arial Black").
    /// </summary>
    DWRITE_INFORMATIONAL_STRING_WIN32_FAMILY_NAMES,

    /// <summary>
    /// GDI-compatible subfamily name.
    /// </summary>
    DWRITE_INFORMATIONAL_STRING_WIN32_SUBFAMILY_NAMES,

    /// <summary>
    /// Family name preferred by the designer. This enables font designers to group more than four fonts in a single family without losing compatibility with
    /// GDI. This name is typically only present if it differs from the GDI-compatible family name.
    /// </summary>
    DWRITE_INFORMATIONAL_STRING_PREFERRED_FAMILY_NAMES,

    /// <summary>
    /// Subfamily name preferred by the designer. This name is typically only present if it differs from the GDI-compatible subfamily name. 
    /// </summary>
    DWRITE_INFORMATIONAL_STRING_PREFERRED_SUBFAMILY_NAMES,

    /// <summary>
    /// Sample text. This can be the font name or any other text that the designer thinks is the best example to display the font in.
    /// </summary>
    DWRITE_INFORMATIONAL_STRING_SAMPLE_TEXT,

    /// <summary>
    /// The full name of the font, e.g. "Arial Bold", from name id 4 in the name table.
    /// </summary>
    DWRITE_INFORMATIONAL_STRING_FULL_NAME,

    /// <summary>
    /// The postscript name of the font, e.g. "GillSans-Bold" from name id 6 in the name table.
    /// </summary>
    DWRITE_INFORMATIONAL_STRING_POSTSCRIPT_NAME,

    /// <summary>
    /// The postscript CID findfont name, from name id 20 in the name table.
    /// </summary>
    DWRITE_INFORMATIONAL_STRING_POSTSCRIPT_CID_NAME,

    /// <summary>
    /// Family name for the weight-width-slope model.
    /// </summary>
    DWRITE_INFORMATIONAL_STRING_WWS_FAMILY_NAME,

    /// <summary>
    /// Script/language tag to identify the scripts or languages that the font was
    /// primarily designed to support. See DWRITE_FONT_PROPERTY_ID_DESIGN_SCRIPT_LANGUAGE_TAG
    /// for a longer description.
    /// </summary>
    DWRITE_INFORMATIONAL_STRING_DESIGN_SCRIPT_LANGUAGE_TAG,

    /// <summary>
    /// Script/language tag to identify the scripts or languages that the font declares
    /// it is able to support.
    /// </summary>
    DWRITE_INFORMATIONAL_STRING_SUPPORTED_SCRIPT_LANGUAGE_TAG,
};


/// <summary>
/// The DWRITE_FONT_METRICS structure specifies the metrics of a font face that
/// are applicable to all glyphs within the font face.
/// </summary>
struct DWRITE_FONT_METRICS
{
    /// <summary>
    /// The number of font design units per em unit.
    /// Font files use their own coordinate system of font design units.
    /// A font design unit is the smallest measurable unit in the em square,
    /// an imaginary square that is used to size and align glyphs.
    /// The concept of em square is used as a reference scale factor when defining font size and device transformation semantics.
    /// The size of one em square is also commonly used to compute the paragraph indentation value.
    /// </summary>
    UINT16 designUnitsPerEm;

    /// <summary>
    /// Ascent value of the font face in font design units.
    /// Ascent is the distance from the top of font character alignment box to English baseline.
    /// </summary>
    UINT16 ascent;

    /// <summary>
    /// Descent value of the font face in font design units.
    /// Descent is the distance from the bottom of font character alignment box to English baseline.
    /// </summary>
    UINT16 descent;

    /// <summary>
    /// Line gap in font design units.
    /// Recommended additional white space to add between lines to improve legibility. The recommended line spacing 
    /// (baseline-to-baseline distance) is thus the sum of ascent, descent, and lineGap. The line gap is usually 
    /// positive or zero but can be negative, in which case the recommended line spacing is less than the height
    /// of the character alignment box.
    /// </summary>
    INT16 lineGap;

    /// <summary>
    /// Cap height value of the font face in font design units.
    /// Cap height is the distance from English baseline to the top of a typical English capital.
    /// Capital "H" is often used as a reference character for the purpose of calculating the cap height value.
    /// </summary>
    UINT16 capHeight;

    /// <summary>
    /// x-height value of the font face in font design units.
    /// x-height is the distance from English baseline to the top of lowercase letter "x", or a similar lowercase character.
    /// </summary>
    UINT16 xHeight;

    /// <summary>
    /// The underline position value of the font face in font design units.
    /// Underline position is the position of underline relative to the English baseline.
    /// The value is usually made negative in order to place the underline below the baseline.
    /// </summary>
    INT16 underlinePosition;

    /// <summary>
    /// The suggested underline thickness value of the font face in font design units.
    /// </summary>
    UINT16 underlineThickness;

    /// <summary>
    /// The strikethrough position value of the font face in font design units.
    /// Strikethrough position is the position of strikethrough relative to the English baseline.
    /// The value is usually made positive in order to place the strikethrough above the baseline.
    /// </summary>
    INT16 strikethroughPosition;

    /// <summary>
    /// The suggested strikethrough thickness value of the font face in font design units.
    /// </summary>
    UINT16 strikethroughThickness;
};

/// <summary>
/// The DWRITE_GLYPH_METRICS structure specifies the metrics of an individual glyph.
/// The units depend on how the metrics are obtained.
/// </summary>
struct DWRITE_GLYPH_METRICS
{
    /// <summary>
    /// Specifies the X offset from the glyph origin to the left edge of the black box.
    /// The glyph origin is the current horizontal writing position.
    /// A negative value means the black box extends to the left of the origin (often true for lowercase italic 'f').
    /// </summary>
    INT32 leftSideBearing;

    /// <summary>
    /// Specifies the X offset from the origin of the current glyph to the origin of the next glyph when writing horizontally.
    /// </summary>
    UINT32 advanceWidth;

    /// <summary>
    /// Specifies the X offset from the right edge of the black box to the origin of the next glyph when writing horizontally.
    /// The value is negative when the right edge of the black box overhangs the layout box.
    /// </summary>
    INT32 rightSideBearing;

    /// <summary>
    /// Specifies the vertical offset from the vertical origin to the top of the black box.
    /// Thus, a positive value adds whitespace whereas a negative value means the glyph overhangs the top of the layout box.
    /// </summary>
    INT32 topSideBearing;

    /// <summary>
    /// Specifies the Y offset from the vertical origin of the current glyph to the vertical origin of the next glyph when writing vertically.
    /// (Note that the term "origin" by itself denotes the horizontal origin. The vertical origin is different.
    /// Its Y coordinate is specified by verticalOriginY value,
    /// and its X coordinate is half the advanceWidth to the right of the horizontal origin).
    /// </summary>
    UINT32 advanceHeight;

    /// <summary>
    /// Specifies the vertical distance from the black box's bottom edge to the advance height.
    /// Positive when the bottom edge of the black box is within the layout box.
    /// Negative when the bottom edge of black box overhangs the layout box.
    /// </summary>
    INT32 bottomSideBearing;

    /// <summary>
    /// Specifies the Y coordinate of a glyph's vertical origin, in the font's design coordinate system.
    /// The y coordinate of a glyph's vertical origin is the sum of the glyph's top side bearing
    /// and the top (i.e. yMax) of the glyph's bounding box.
    /// </summary>
    INT32 verticalOriginY;
};

/// <summary>
/// Optional adjustment to a glyph's position. A glyph offset changes the position of a glyph without affecting
/// the pen position. Offsets are in logical, pre-transform units.
/// </summary>
struct DWRITE_GLYPH_OFFSET
{
    /// <summary>
    /// Offset in the advance direction of the run. A positive advance offset moves the glyph to the right
    /// (in pre-transform coordinates) if the run is left-to-right or to the left if the run is right-to-left.
    /// </summary>
    FLOAT advanceOffset;

    /// <summary>
    /// Offset in the ascent direction, i.e., the direction ascenders point. A positive ascender offset moves
    /// the glyph up (in pre-transform coordinates).
    /// </summary>
    FLOAT ascenderOffset;
};

/// <summary>
/// Specifies the type of DirectWrite factory object.
/// DirectWrite factory contains internal state such as font loader registration and cached font data.
/// In most cases it is recommended to use the shared factory object, because it allows multiple components
/// that use DirectWrite to share internal DirectWrite state and reduce memory usage.
/// However, there are cases when it is desirable to reduce the impact of a component,
/// such as a plug-in from an untrusted source, on the rest of the process by sandboxing and isolating it
/// from the rest of the process components. In such cases, it is recommended to use an isolated factory for the sandboxed
/// component.
/// </summary>
enum DWRITE_FACTORY_TYPE
{
    /// <summary>
    /// Shared factory allow for re-use of cached font data across multiple in process components.
    /// Such factories also take advantage of cross process font caching components for better performance.
    /// </summary>
    DWRITE_FACTORY_TYPE_SHARED,

    /// <summary>
    /// Objects created from the isolated factory do not interact with internal DirectWrite state from other components.
    /// </summary>
    DWRITE_FACTORY_TYPE_ISOLATED
};

// Creates an OpenType tag as a 32bit integer such that
// the first character in the tag is the lowest byte,
// (least significant on little endian architectures)
// which can be used to compare with tags in the font file.
// This macro is compatible with DWRITE_FONT_FEATURE_TAG.
//
// Example: DWRITE_MAKE_OPENTYPE_TAG('c','c','m','p')
// Dword:   0x706D6363
//
#define DWRITE_MAKE_OPENTYPE_TAG(a,b,c,d) ( \
    (static_cast<UINT32>(static_cast<UINT8>(d)) << 24) | \
    (static_cast<UINT32>(static_cast<UINT8>(c)) << 16) | \
    (static_cast<UINT32>(static_cast<UINT8>(b)) << 8)  | \
     static_cast<UINT32>(static_cast<UINT8>(a)))

interface IDWriteFontFileStream;

/// <summary>
/// Font file loader interface handles loading font file resources of a particular type from a key.
/// The font file loader interface is recommended to be implemented by a singleton object.
/// IMPORTANT: font file loader implementations must not register themselves with DirectWrite factory
/// inside their constructors and must not unregister themselves in their destructors, because
/// registration and unregistration operations increment and decrement the object reference count respectively.
/// Instead, registration and unregistration of font file loaders with DirectWrite factory should be performed
/// outside of the font file loader implementation as a separate step.
/// </summary>
interface DWRITE_DECLARE_INTERFACE("727cad4e-d6af-4c9e-8a08-d695b11caa49") IDWriteFontFileLoader : public IUnknown
{
    /// <summary>
    /// Creates a font file stream object that encapsulates an open file resource.
    /// The resource is closed when the last reference to fontFileStream is released.
    /// </summary>
    /// <param name="fontFileReferenceKey">Font file reference key that uniquely identifies the font file resource
    /// within the scope of the font loader being used.</param>
    /// <param name="fontFileReferenceKeySize">Size of font file reference key in bytes.</param>
    /// <param name="fontFileStream">Pointer to the newly created font file stream.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(CreateStreamFromKey)(
        _In_reads_bytes_(fontFileReferenceKeySize) void const* fontFileReferenceKey,
        UINT32 fontFileReferenceKeySize,
        _COM_Outptr_ IDWriteFontFileStream** fontFileStream
        ) PURE;
};

/// <summary>
/// A built-in implementation of IDWriteFontFileLoader interface that operates on local font files
/// and exposes local font file information from the font file reference key.
/// Font file references created using CreateFontFileReference use this font file loader.
/// </summary>
interface DWRITE_DECLARE_INTERFACE("b2d9f3ec-c9fe-4a11-a2ec-d86208f7c0a2") IDWriteLocalFontFileLoader : public IDWriteFontFileLoader
{
    /// <summary>
    /// Obtains the length of the absolute file path from the font file reference key.
    /// </summary>
    /// <param name="fontFileReferenceKey">Font file reference key that uniquely identifies the local font file
    /// within the scope of the font loader being used.</param>
    /// <param name="fontFileReferenceKeySize">Size of font file reference key in bytes.</param>
    /// <param name="filePathLength">Length of the file path string not including the terminated NULL character.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetFilePathLengthFromKey)(
        _In_reads_bytes_(fontFileReferenceKeySize) void const* fontFileReferenceKey,
        UINT32 fontFileReferenceKeySize,
        _Out_ UINT32* filePathLength
        ) PURE;

    /// <summary>
    /// Obtains the absolute font file path from the font file reference key.
    /// </summary>
    /// <param name="fontFileReferenceKey">Font file reference key that uniquely identifies the local font file
    /// within the scope of the font loader being used.</param>
    /// <param name="fontFileReferenceKeySize">Size of font file reference key in bytes.</param>
    /// <param name="filePath">Character array that receives the local file path.</param>
    /// <param name="filePathSize">Size of the filePath array in character count including the terminated NULL character.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetFilePathFromKey)(
        _In_reads_bytes_(fontFileReferenceKeySize) void const* fontFileReferenceKey,
        UINT32 fontFileReferenceKeySize,
        _Out_writes_z_(filePathSize) WCHAR* filePath,
        UINT32 filePathSize
        ) PURE;

    /// <summary>
    /// Obtains the last write time of the file from the font file reference key.
    /// </summary>
    /// <param name="fontFileReferenceKey">Font file reference key that uniquely identifies the local font file
    /// within the scope of the font loader being used.</param>
    /// <param name="fontFileReferenceKeySize">Size of font file reference key in bytes.</param>
    /// <param name="lastWriteTime">Last modified time of the font file.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetLastWriteTimeFromKey)(
        _In_reads_bytes_(fontFileReferenceKeySize) void const* fontFileReferenceKey,
        UINT32 fontFileReferenceKeySize,
        _Out_ FILETIME* lastWriteTime
        ) PURE;
};

/// <summary>
/// The interface for loading font file data.
/// </summary>
interface DWRITE_DECLARE_INTERFACE("6d4865fe-0ab8-4d91-8f62-5dd6be34a3e0") IDWriteFontFileStream : public IUnknown
{
    /// <summary>
    /// Reads a fragment from a file.
    /// </summary>
    /// <param name="fragmentStart">Receives the pointer to the start of the font file fragment.</param>
    /// <param name="fileOffset">Offset of the fragment from the beginning of the font file.</param>
    /// <param name="fragmentSize">Size of the fragment in bytes.</param>
    /// <param name="fragmentContext">The client defined context to be passed to the ReleaseFileFragment.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    /// <remarks>
    /// IMPORTANT: ReadFileFragment() implementations must check whether the requested file fragment
    /// is within the file bounds. Otherwise, an error should be returned from ReadFileFragment.
    /// </remarks>
    STDMETHOD(ReadFileFragment)(
        _Outptr_result_bytebuffer_(fragmentSize) void const** fragmentStart,
        UINT64 fileOffset,
        UINT64 fragmentSize,
        _Out_ void** fragmentContext
        ) PURE;

    /// <summary>
    /// Releases a fragment from a file.
    /// </summary>
    /// <param name="fragmentContext">The client defined context of a font fragment returned from ReadFileFragment.</param>
    STDMETHOD_(void, ReleaseFileFragment)(
        void* fragmentContext
        ) PURE;

    /// <summary>
    /// Obtains the total size of a file.
    /// </summary>
    /// <param name="fileSize">Receives the total size of the file.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    /// <remarks>
    /// Implementing GetFileSize() for asynchronously loaded font files may require
    /// downloading the complete file contents, therefore this method should only be used for operations that
    /// either require complete font file to be loaded (e.g., copying a font file) or need to make
    /// decisions based on the value of the file size (e.g., validation against a persisted file size).
    /// </remarks>
    STDMETHOD(GetFileSize)(
        _Out_ UINT64* fileSize
        ) PURE;

    /// <summary>
    /// Obtains the last modified time of the file. The last modified time is used by DirectWrite font selection algorithms
    /// to determine whether one font resource is more up to date than another one.
    /// </summary>
    /// <param name="lastWriteTime">Receives the last modified time of the file in the format that represents
    /// the number of 100-nanosecond intervals since January 1, 1601 (UTC).</param>
    /// <returns>
    /// Standard HRESULT error code. For resources that don't have a concept of the last modified time, the implementation of
    /// GetLastWriteTime should return E_NOTIMPL.
    /// </returns>
    STDMETHOD(GetLastWriteTime)(
        _Out_ UINT64* lastWriteTime
        ) PURE;
};

/// <summary>
/// The interface that represents a reference to a font file.
/// </summary>
interface DWRITE_DECLARE_INTERFACE("739d886a-cef5-47dc-8769-1a8b41bebbb0") IDWriteFontFile : public IUnknown
{
    /// <summary>
    /// This method obtains the pointer to the reference key of a font file. The pointer is only valid until the object that refers to it is released.
    /// </summary>
    /// <param name="fontFileReferenceKey">Pointer to the font file reference key.
    /// IMPORTANT: The pointer value is valid until the font file reference object it is obtained from is released.</param>
    /// <param name="fontFileReferenceKeySize">Size of font file reference key in bytes.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetReferenceKey)(
        _Outptr_result_bytebuffer_(*fontFileReferenceKeySize) void const** fontFileReferenceKey,
        _Out_ UINT32* fontFileReferenceKeySize
        ) PURE;

    /// <summary>
    /// Obtains the file loader associated with a font file object.
    /// </summary>
    /// <param name="fontFileLoader">The font file loader associated with the font file object.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetLoader)(
        _COM_Outptr_ IDWriteFontFileLoader** fontFileLoader
        ) PURE;

    /// <summary>
    /// Analyzes a file and returns whether it represents a font, and whether the font type is supported by the font system.
    /// </summary>
    /// <param name="isSupportedFontType">TRUE if the font type is supported by the font system, FALSE otherwise.</param>
    /// <param name="fontFileType">The type of the font file. Note that even if isSupportedFontType is FALSE,
    /// the fontFileType value may be different from DWRITE_FONT_FILE_TYPE_UNKNOWN.</param>
    /// <param name="fontFaceType">The type of the font face that can be constructed from the font file.
    /// Note that even if isSupportedFontType is FALSE, the fontFaceType value may be different from
    /// DWRITE_FONT_FACE_TYPE_UNKNOWN.</param>
    /// <param name="numberOfFaces">Number of font faces contained in the font file.</param>
    /// <returns>
    /// Standard HRESULT error code if there was a processing error during analysis.
    /// </returns>
    /// <remarks>
    /// IMPORTANT: certain font file types are recognized, but not supported by the font system.
    /// For example, the font system will recognize a file as a Type 1 font file,
    /// but will not be able to construct a font face object from it. In such situations, Analyze will set
    /// isSupportedFontType output parameter to FALSE.
    /// </remarks>
    STDMETHOD(Analyze)(
        _Out_ BOOL* isSupportedFontType,
        _Out_ DWRITE_FONT_FILE_TYPE* fontFileType,
        _Out_opt_ DWRITE_FONT_FACE_TYPE* fontFaceType,
        _Out_ UINT32* numberOfFaces
        ) PURE;
};

/// <summary>
/// Represents the internal structure of a device pixel (i.e., the physical arrangement of red,
/// green, and blue color components) that is assumed for purposes of rendering text.
/// </summary>
#ifndef DWRITE_PIXEL_GEOMETRY_DEFINED
enum DWRITE_PIXEL_GEOMETRY
{
    /// <summary>
    /// The red, green, and blue color components of each pixel are assumed to occupy the same point.
    /// </summary>
    DWRITE_PIXEL_GEOMETRY_FLAT,

    /// <summary>
    /// Each pixel comprises three vertical stripes, with red on the left, green in the center, and 
    /// blue on the right. This is the most common pixel geometry for LCD monitors.
    /// </summary>
    DWRITE_PIXEL_GEOMETRY_RGB,

    /// <summary>
    /// Each pixel comprises three vertical stripes, with blue on the left, green in the center, and 
    /// red on the right.
    /// </summary>
    DWRITE_PIXEL_GEOMETRY_BGR
};
#define DWRITE_PIXEL_GEOMETRY_DEFINED
#endif

/// <summary>
/// Represents a method of rendering glyphs.
/// </summary>
enum DWRITE_RENDERING_MODE
{
    /// <summary>
    /// Specifies that the rendering mode is determined automatically based on the font and size.
    /// </summary>
    DWRITE_RENDERING_MODE_DEFAULT,

    /// <summary>
    /// Specifies that no antialiasing is performed. Each pixel is either set to the foreground 
    /// color of the text or retains the color of the background.
    /// </summary>
    DWRITE_RENDERING_MODE_ALIASED,

    /// <summary>
    /// Specifies that antialiasing is performed in the horizontal direction and the appearance
    /// of glyphs is layout-compatible with GDI using CLEARTYPE_QUALITY. Use DWRITE_MEASURING_MODE_GDI_CLASSIC 
    /// to get glyph advances. The antialiasing may be either ClearType or grayscale depending on
    /// the text antialiasing mode.
    /// </summary>
    DWRITE_RENDERING_MODE_GDI_CLASSIC,

    /// <summary>
    /// Specifies that antialiasing is performed in the horizontal direction and the appearance
    /// of glyphs is layout-compatible with GDI using CLEARTYPE_NATURAL_QUALITY. Glyph advances
    /// are close to the font design advances, but are still rounded to whole pixels. Use
    /// DWRITE_MEASURING_MODE_GDI_NATURAL to get glyph advances. The antialiasing may be either
    /// ClearType or grayscale depending on the text antialiasing mode.
    /// </summary>
    DWRITE_RENDERING_MODE_GDI_NATURAL,

    /// <summary>
    /// Specifies that antialiasing is performed in the horizontal direction. This rendering
    /// mode allows glyphs to be positioned with subpixel precision and is therefore suitable
    /// for natural (i.e., resolution-independent) layout. The antialiasing may be either
    /// ClearType or grayscale depending on the text antialiasing mode.
    /// </summary>
    DWRITE_RENDERING_MODE_NATURAL,

    /// <summary>
    /// Similar to natural mode except that antialiasing is performed in both the horizontal
    /// and vertical directions. This is typically used at larger sizes to make curves and
    /// diagonal lines look smoother. The antialiasing may be either ClearType or grayscale
    /// depending on the text antialiasing mode.
    /// </summary>
    DWRITE_RENDERING_MODE_NATURAL_SYMMETRIC,

    /// <summary>
    /// Specifies that rendering should bypass the rasterizer and use the outlines directly. 
    /// This is typically used at very large sizes.
    /// </summary>
    DWRITE_RENDERING_MODE_OUTLINE,

    // The following names are obsolete, but are kept as aliases to avoid breaking existing code.
    // Each of these rendering modes may result in either ClearType or grayscale antialiasing 
    // depending on the DWRITE_TEXT_ANTIALIASING_MODE.
    DWRITE_RENDERING_MODE_CLEARTYPE_GDI_CLASSIC         = DWRITE_RENDERING_MODE_GDI_CLASSIC,
    DWRITE_RENDERING_MODE_CLEARTYPE_GDI_NATURAL         = DWRITE_RENDERING_MODE_GDI_NATURAL,
    DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL             = DWRITE_RENDERING_MODE_NATURAL,
    DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL_SYMMETRIC   = DWRITE_RENDERING_MODE_NATURAL_SYMMETRIC
};

/// <summary>
/// The DWRITE_MATRIX structure specifies the graphics transform to be applied
/// to rendered glyphs.
/// </summary>
struct DWRITE_MATRIX
{
    /// <summary>
    /// Horizontal scaling / cosine of rotation
    /// </summary>
    FLOAT m11;

    /// <summary>
    /// Vertical shear / sine of rotation
    /// </summary>
    FLOAT m12;

    /// <summary>
    /// Horizontal shear / negative sine of rotation
    /// </summary>
    FLOAT m21;

    /// <summary>
    /// Vertical scaling / cosine of rotation
    /// </summary>
    FLOAT m22;

    /// <summary>
    /// Horizontal shift (always orthogonal regardless of rotation)
    /// </summary>
    FLOAT dx;

    /// <summary>
    /// Vertical shift (always orthogonal regardless of rotation)
    /// </summary>
    FLOAT dy;
};

/// <summary>
/// The interface that represents text rendering settings for glyph rasterization and filtering.
/// </summary>
interface DWRITE_DECLARE_INTERFACE("2f0da53a-2add-47cd-82ee-d9ec34688e75") IDWriteRenderingParams : public IUnknown
{
    /// <summary>
    /// Gets the gamma value used for gamma correction. Valid values must be
    /// greater than zero and cannot exceed 256.
    /// </summary>
    STDMETHOD_(FLOAT, GetGamma)() PURE;

    /// <summary>
    /// Gets the amount of contrast enhancement. Valid values are greater than
    /// or equal to zero.
    /// </summary>
    STDMETHOD_(FLOAT, GetEnhancedContrast)() PURE;

    /// <summary>
    /// Gets the ClearType level. Valid values range from 0.0f (no ClearType) 
    /// to 1.0f (full ClearType).
    /// </summary>
    STDMETHOD_(FLOAT, GetClearTypeLevel)() PURE;

    /// <summary>
    /// Gets the pixel geometry.
    /// </summary>
    STDMETHOD_(DWRITE_PIXEL_GEOMETRY, GetPixelGeometry)() PURE;

    /// <summary>
    /// Gets the rendering mode.
    /// </summary>
    STDMETHOD_(DWRITE_RENDERING_MODE, GetRenderingMode)() PURE;
};

// Forward declarations of D2D types
interface ID2D1SimplifiedGeometrySink;

typedef ID2D1SimplifiedGeometrySink IDWriteGeometrySink;

/// <summary>
/// The interface that represents an absolute reference to a font face.
/// It contains font face type, appropriate file references and face identification data.
/// Various font data such as metrics, names and glyph outlines is obtained from IDWriteFontFace.
/// </summary>
interface DWRITE_DECLARE_INTERFACE("5f49804d-7024-4d43-bfa9-d25984f53849") IDWriteFontFace : public IUnknown
{
    /// <summary>
    /// Obtains the file format type of a font face.
    /// </summary>
    STDMETHOD_(DWRITE_FONT_FACE_TYPE, GetType)() PURE;

    /// <summary>
    /// Obtains the font files representing a font face.
    /// </summary>
    /// <param name="numberOfFiles">The number of files representing the font face.</param>
    /// <param name="fontFiles">User provided array that stores pointers to font files representing the font face.
    /// This parameter can be NULL if the user is only interested in the number of files representing the font face.
    /// This API increments reference count of the font file pointers returned according to COM conventions, and the client
    /// should release them when finished.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetFiles)(
        _Inout_ UINT32* numberOfFiles,
        _Out_writes_opt_(*numberOfFiles) IDWriteFontFile** fontFiles
        ) PURE;

    /// <summary>
    /// Obtains the zero-based index of the font face in its font file or files. If the font files contain a single face,
    /// the return value is zero.
    /// </summary>
    STDMETHOD_(UINT32, GetIndex)() PURE;

    /// <summary>
    /// Obtains the algorithmic style simulation flags of a font face.
    /// </summary>
    STDMETHOD_(DWRITE_FONT_SIMULATIONS, GetSimulations)() PURE;

    /// <summary>
    /// Determines whether the font is a symbol font.
    /// </summary>
    STDMETHOD_(BOOL, IsSymbolFont)() PURE;

    /// <summary>
    /// Obtains design units and common metrics for the font face.
    /// These metrics are applicable to all the glyphs within a fontface and are used by applications for layout calculations.
    /// </summary>
    /// <param name="fontFaceMetrics">Points to a DWRITE_FONT_METRICS structure to fill in.
    /// The metrics returned by this function are in font design units.</param>
    STDMETHOD_(void, GetMetrics)(
        _Out_ DWRITE_FONT_METRICS* fontFaceMetrics
        ) PURE;

    /// <summary>
    /// Obtains the number of glyphs in the font face.
    /// </summary>
    STDMETHOD_(UINT16, GetGlyphCount)() PURE;

    /// <summary>
    /// Obtains ideal glyph metrics in font design units. Design glyphs metrics are used for glyph positioning.
    /// </summary>
    /// <param name="glyphIndices">An array of glyph indices to compute the metrics for.</param>
    /// <param name="glyphCount">The number of elements in the glyphIndices array.</param>
    /// <param name="glyphMetrics">Array of DWRITE_GLYPH_METRICS structures filled by this function.
    /// The metrics returned by this function are in font design units.</param>
    /// <param name="isSideways">Indicates whether the font is being used in a sideways run.
    /// This can affect the glyph metrics if the font has oblique simulation
    /// because sideways oblique simulation differs from non-sideways oblique simulation.</param>
    /// <returns>
    /// Standard HRESULT error code. If any of the input glyph indices are outside of the valid glyph index range
    /// for the current font face, E_INVALIDARG will be returned.
    /// </returns>
    STDMETHOD(GetDesignGlyphMetrics)(
        _In_reads_(glyphCount) UINT16 const* glyphIndices,
        UINT32 glyphCount,
        _Out_writes_(glyphCount) DWRITE_GLYPH_METRICS* glyphMetrics,
        BOOL isSideways = FALSE
        ) PURE;

    /// <summary>
    /// Returns the nominal mapping of UTF-32 Unicode code points to glyph indices as defined by the font 'cmap' table.
    /// Note that this mapping is primarily provided for line layout engines built on top of the physical font API.
    /// Because of OpenType glyph substitution and line layout character substitution, the nominal conversion does not always correspond
    /// to how a Unicode string will map to glyph indices when rendering using a particular font face.
    /// Also, note that Unicode Variation Selectors provide for alternate mappings for character to glyph.
    /// This call will always return the default variant.
    /// </summary>
    /// <param name="codePoints">An array of UTF-32 code points to obtain nominal glyph indices from.</param>
    /// <param name="codePointCount">The number of elements in the codePoints array.</param>
    /// <param name="glyphIndices">Array of nominal glyph indices filled by this function.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetGlyphIndices)(
        _In_reads_(codePointCount) UINT32 const* codePoints,
        UINT32 codePointCount,
        _Out_writes_(codePointCount) UINT16* glyphIndices
        ) PURE;
 
    /// <summary>
    /// Finds the specified OpenType font table if it exists and returns a pointer to it.
    /// The function accesses the underlying font data via the IDWriteFontFileStream interface
    /// implemented by the font file loader.
    /// </summary>
    /// <param name="openTypeTableTag">Four character tag of table to find.
    ///     Use the DWRITE_MAKE_OPENTYPE_TAG() macro to create it.
    ///     Unlike GDI, it does not support the special TTCF and null tags to access the whole font.</param>
    /// <param name="tableData">
    ///     Pointer to base of table in memory.
    ///     The pointer is only valid so long as the FontFace used to get the font table still exists
    ///     (not any other FontFace, even if it actually refers to the same physical font).
    /// </param>
    /// <param name="tableSize">Byte size of table.</param>
    /// <param name="tableContext">
    ///     Opaque context which must be freed by calling ReleaseFontTable.
    ///     The context actually comes from the lower level IDWriteFontFileStream,
    ///     which may be implemented by the application or DWrite itself.
    ///     It is possible for a NULL tableContext to be returned, especially if
    ///     the implementation directly memory maps the whole file.
    ///     Nevertheless, always release it later, and do not use it as a test for function success.
    ///     The same table can be queried multiple times,
    ///     but each returned context can be different, so release each separately.
    /// </param>
    /// <param name="exists">True if table exists.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// If a table can not be found, the function will not return an error, but the size will be 0, table NULL, and exists = FALSE.
    /// The context does not need to be freed if the table was not found.
    /// </returns>
    /// <remarks>
    /// The context for the same tag may be different for each call,
    /// so each one must be held and released separately.
    /// </remarks>
    STDMETHOD(TryGetFontTable)(
        _In_ UINT32 openTypeTableTag,
        _Outptr_result_bytebuffer_(*tableSize) const void** tableData,
        _Out_ UINT32* tableSize,
        _Out_ void** tableContext,
        _Out_ BOOL* exists
        ) PURE;

    /// <summary>
    /// Releases the table obtained earlier from TryGetFontTable.
    /// </summary>
    /// <param name="tableContext">Opaque context from TryGetFontTable.</param>
    STDMETHOD_(void, ReleaseFontTable)(
        _In_ void* tableContext
        ) PURE;

    /// <summary>
    /// Computes the outline of a run of glyphs by calling back to the outline sink interface.
    /// </summary>
    /// <param name="emSize">Logical size of the font in DIP units. A DIP ("device-independent pixel") equals 1/96 inch.</param>
    /// <param name="glyphIndices">Array of glyph indices.</param>
    /// <param name="glyphAdvances">Optional array of glyph advances in DIPs.</param>
    /// <param name="glyphOffsets">Optional array of glyph offsets.</param>
    /// <param name="glyphCount">Number of glyphs.</param>
    /// <param name="isSideways">If true, specifies that glyphs are rotated 90 degrees to the left and vertical metrics are used.
    /// A client can render a vertical run by specifying isSideways = true and rotating the resulting geometry 90 degrees to the
    /// right using a transform.</param>
    /// <param name="isRightToLeft">If true, specifies that the advance direction is right to left. By default, the advance direction
    /// is left to right.</param>
    /// <param name="geometrySink">Interface the function calls back to draw each element of the geometry.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetGlyphRunOutline)(
        FLOAT emSize,
        _In_reads_(glyphCount) UINT16 const* glyphIndices,
        _In_reads_opt_(glyphCount) FLOAT const* glyphAdvances,
        _In_reads_opt_(glyphCount) DWRITE_GLYPH_OFFSET const* glyphOffsets,
        UINT32 glyphCount,
        BOOL isSideways,
        BOOL isRightToLeft,
        _In_ IDWriteGeometrySink* geometrySink
        ) PURE;

    /// <summary>
    /// Determines the recommended rendering mode for the font given the specified size and rendering parameters.
    /// </summary>
    /// <param name="emSize">Logical size of the font in DIP units. A DIP ("device-independent pixel") equals 1/96 inch.</param>
    /// <param name="pixelsPerDip">Number of physical pixels per DIP. For example, if the DPI of the rendering surface is 96 this 
    /// value is 1.0f. If the DPI is 120, this value is 120.0f/96.</param>
    /// <param name="measuringMode">Specifies measuring mode that will be used for glyphs in the font.
    /// Renderer implementations may choose different rendering modes for given measuring modes, but
    /// best results are seen when the corresponding modes match:
    /// DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL for DWRITE_MEASURING_MODE_NATURAL
    /// DWRITE_RENDERING_MODE_CLEARTYPE_GDI_CLASSIC for DWRITE_MEASURING_MODE_GDI_CLASSIC
    /// DWRITE_RENDERING_MODE_CLEARTYPE_GDI_NATURAL for DWRITE_MEASURING_MODE_GDI_NATURAL
    /// </param>
    /// <param name="renderingParams">Rendering parameters object. This parameter is necessary in case the rendering parameters 
    /// object overrides the rendering mode.</param>
    /// <param name="renderingMode">Receives the recommended rendering mode to use.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetRecommendedRenderingMode)(
        FLOAT emSize,
        FLOAT pixelsPerDip,
        DWRITE_MEASURING_MODE measuringMode,
        IDWriteRenderingParams* renderingParams,
        _Out_ DWRITE_RENDERING_MODE* renderingMode
        ) PURE;

    /// <summary>
    /// Obtains design units and common metrics for the font face.
    /// These metrics are applicable to all the glyphs within a fontface and are used by applications for layout calculations.
    /// </summary>
    /// <param name="emSize">Logical size of the font in DIP units. A DIP ("device-independent pixel") equals 1/96 inch.</param>
    /// <param name="pixelsPerDip">Number of physical pixels per DIP. For example, if the DPI of the rendering surface is 96 this 
    /// value is 1.0f. If the DPI is 120, this value is 120.0f/96.</param>
    /// <param name="transform">Optional transform applied to the glyphs and their positions. This transform is applied after the
    /// scaling specified by the font size and pixelsPerDip.</param>
    /// <param name="fontFaceMetrics">Points to a DWRITE_FONT_METRICS structure to fill in.
    /// The metrics returned by this function are in font design units.</param>
    STDMETHOD(GetGdiCompatibleMetrics)(
        FLOAT emSize,
        FLOAT pixelsPerDip,
        _In_opt_ DWRITE_MATRIX const* transform,
        _Out_ DWRITE_FONT_METRICS* fontFaceMetrics
        ) PURE;

    /// <summary>
    /// Obtains glyph metrics in font design units with the return values compatible with what GDI would produce.
    /// Glyphs metrics are used for positioning of individual glyphs.
    /// </summary>
    /// <param name="emSize">Logical size of the font in DIP units. A DIP ("device-independent pixel") equals 1/96 inch.</param>
    /// <param name="pixelsPerDip">Number of physical pixels per DIP. For example, if the DPI of the rendering surface is 96 this 
    /// value is 1.0f. If the DPI is 120, this value is 120.0f/96.</param>
    /// <param name="transform">Optional transform applied to the glyphs and their positions. This transform is applied after the
    /// scaling specified by the font size and pixelsPerDip.</param>
    /// <param name="useGdiNatural">
    /// When set to FALSE, the metrics are the same as the metrics of GDI aliased text.
    /// When set to TRUE, the metrics are the same as the metrics of text measured by GDI using a font
    /// created with CLEARTYPE_NATURAL_QUALITY.
    /// </param>
    /// <param name="glyphIndices">An array of glyph indices to compute the metrics for.</param>
    /// <param name="glyphCount">The number of elements in the glyphIndices array.</param>
    /// <param name="glyphMetrics">Array of DWRITE_GLYPH_METRICS structures filled by this function.
    /// The metrics returned by this function are in font design units.</param>
    /// <param name="isSideways">Indicates whether the font is being used in a sideways run.
    /// This can affect the glyph metrics if the font has oblique simulation
    /// because sideways oblique simulation differs from non-sideways oblique simulation.</param>
    /// <returns>
    /// Standard HRESULT error code. If any of the input glyph indices are outside of the valid glyph index range
    /// for the current font face, E_INVALIDARG will be returned.
    /// </returns>
    STDMETHOD(GetGdiCompatibleGlyphMetrics)(
        FLOAT emSize,
        FLOAT pixelsPerDip,
        _In_opt_ DWRITE_MATRIX const* transform,
        BOOL useGdiNatural,
        _In_reads_(glyphCount) UINT16 const* glyphIndices,
        UINT32 glyphCount,
        _Out_writes_(glyphCount) DWRITE_GLYPH_METRICS* glyphMetrics,
        BOOL isSideways = FALSE
        ) PURE;
};


interface IDWriteFactory;
interface IDWriteFontFileEnumerator;

/// <summary>
/// The font collection loader interface is used to construct a collection of fonts given a particular type of key.
/// The font collection loader interface is recommended to be implemented by a singleton object.
/// IMPORTANT: font collection loader implementations must not register themselves with a DirectWrite factory
/// inside their constructors and must not unregister themselves in their destructors, because
/// registration and unregistration operations increment and decrement the object reference count respectively.
/// Instead, registration and unregistration of font file loaders with DirectWrite factory should be performed
/// outside of the font file loader implementation as a separate step.
/// </summary>
interface DWRITE_DECLARE_INTERFACE("cca920e4-52f0-492b-bfa8-29c72ee0a468") IDWriteFontCollectionLoader : public IUnknown
{
    /// <summary>
    /// Creates a font file enumerator object that encapsulates a collection of font files.
    /// The font system calls back to this interface to create a font collection.
    /// </summary>
    /// <param name="factory">Factory associated with the loader.</param>
    /// <param name="collectionKey">Font collection key that uniquely identifies the collection of font files within
    /// the scope of the font collection loader being used.</param>
    /// <param name="collectionKeySize">Size of the font collection key in bytes.</param>
    /// <param name="fontFileEnumerator">Pointer to the newly created font file enumerator.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(CreateEnumeratorFromKey)(
        _In_ IDWriteFactory* factory,
        _In_reads_bytes_(collectionKeySize) void const* collectionKey,
        UINT32 collectionKeySize,
        _COM_Outptr_ IDWriteFontFileEnumerator** fontFileEnumerator
        ) PURE;
};

/// <summary>
/// The font file enumerator interface encapsulates a collection of font files. The font system uses this interface
/// to enumerate font files when building a font collection.
/// </summary>
interface DWRITE_DECLARE_INTERFACE("72755049-5ff7-435d-8348-4be97cfa6c7c") IDWriteFontFileEnumerator : public IUnknown
{
    /// <summary>
    /// Advances to the next font file in the collection. When it is first created, the enumerator is positioned
    /// before the first element of the collection and the first call to MoveNext advances to the first file.
    /// </summary>
    /// <param name="hasCurrentFile">Receives the value TRUE if the enumerator advances to a file, or FALSE if
    /// the enumerator advanced past the last file in the collection.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(MoveNext)(
        _Out_ BOOL* hasCurrentFile
        ) PURE;

    /// <summary>
    /// Gets a reference to the current font file.
    /// </summary>
    /// <param name="fontFile">Pointer to the newly created font file object.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetCurrentFontFile)(
        _COM_Outptr_ IDWriteFontFile** fontFile
        ) PURE;
};

/// <summary>
/// Represents a collection of strings indexed by locale name.
/// </summary>
interface DWRITE_DECLARE_INTERFACE("08256209-099a-4b34-b86d-c22b110e7771") IDWriteLocalizedStrings : public IUnknown
{
    /// <summary>
    /// Gets the number of language/string pairs.
    /// </summary>
    STDMETHOD_(UINT32, GetCount)() PURE;

    /// <summary>
    /// Gets the index of the item with the specified locale name.
    /// </summary>
    /// <param name="localeName">Locale name to look for.</param>
    /// <param name="index">Receives the zero-based index of the locale name/string pair.</param>
    /// <param name="exists">Receives TRUE if the locale name exists or FALSE if not.</param>
    /// <returns>
    /// Standard HRESULT error code. If the specified locale name does not exist, the return value is S_OK, 
    /// but *index is UINT_MAX and *exists is FALSE.
    /// </returns>
    STDMETHOD(FindLocaleName)(
        _In_z_ WCHAR const* localeName,
        _Out_ UINT32* index,
        _Out_ BOOL* exists
        ) PURE;

    /// <summary>
    /// Gets the length in characters (not including the null terminator) of the locale name with the specified index.
    /// </summary>
    /// <param name="index">Zero-based index of the locale name.</param>
    /// <param name="length">Receives the length in characters, not including the null terminator.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetLocaleNameLength)(
        UINT32 index,
        _Out_ UINT32* length
        ) PURE;

    /// <summary>
    /// Copies the locale name with the specified index to the specified array.
    /// </summary>
    /// <param name="index">Zero-based index of the locale name.</param>
    /// <param name="localeName">Character array that receives the locale name.</param>
    /// <param name="size">Size of the array in characters. The size must include space for the terminating
    /// null character.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetLocaleName)(
        UINT32 index,
        _Out_writes_z_(size) WCHAR* localeName,
        UINT32 size
        ) PURE;

    /// <summary>
    /// Gets the length in characters (not including the null terminator) of the string with the specified index.
    /// </summary>
    /// <param name="index">Zero-based index of the string.</param>
    /// <param name="length">Receives the length in characters, not including the null terminator.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetStringLength)(
        UINT32 index,
        _Out_ UINT32* length
        ) PURE;

    /// <summary>
    /// Copies the string with the specified index to the specified array.
    /// </summary>
    /// <param name="index">Zero-based index of the string.</param>
    /// <param name="stringBuffer">Character array that receives the string.</param>
    /// <param name="size">Size of the array in characters. The size must include space for the terminating
    /// null character.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetString)(
        UINT32 index,
        _Out_writes_z_(size) WCHAR* stringBuffer,
        UINT32 size
        ) PURE;
};

interface IDWriteFontFamily;
interface IDWriteFont;

/// <summary>
/// The IDWriteFontCollection encapsulates a collection of fonts.
/// </summary>
interface DWRITE_DECLARE_INTERFACE("a84cee02-3eea-4eee-a827-87c1a02a0fcc") IDWriteFontCollection : public IUnknown
{
    /// <summary>
    /// Gets the number of font families in the collection.
    /// </summary>
    STDMETHOD_(UINT32, GetFontFamilyCount)() PURE;

    /// <summary>
    /// Creates a font family object given a zero-based font family index.
    /// </summary>
    /// <param name="index">Zero-based index of the font family.</param>
    /// <param name="fontFamily">Receives a pointer the newly created font family object.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetFontFamily)(
        UINT32 index,
        _COM_Outptr_ IDWriteFontFamily** fontFamily
        ) PURE;

    /// <summary>
    /// Finds the font family with the specified family name.
    /// </summary>
    /// <param name="familyName">Name of the font family. The name is not case-sensitive but must otherwise exactly match a family name in the collection.</param>
    /// <param name="index">Receives the zero-based index of the matching font family if the family name was found or UINT_MAX otherwise.</param>
    /// <param name="exists">Receives TRUE if the family name exists or FALSE otherwise.</param>
    /// <returns>
    /// Standard HRESULT error code. If the specified family name does not exist, the return value is S_OK, but *index is UINT_MAX and *exists is FALSE.
    /// </returns>
    STDMETHOD(FindFamilyName)(
        _In_z_ WCHAR const* familyName,
        _Out_ UINT32* index,
        _Out_ BOOL* exists
        ) PURE;

    /// <summary>
    /// Gets the font object that corresponds to the same physical font as the specified font face object. The specified physical font must belong 
    /// to the font collection.
    /// </summary>
    /// <param name="fontFace">Font face object that specifies the physical font.</param>
    /// <param name="font">Receives a pointer to the newly created font object if successful or NULL otherwise.</param>
    /// <returns>
    /// Standard HRESULT error code. If the specified physical font is not part of the font collection the return value is DWRITE_E_NOFONT.
    /// </returns>
    STDMETHOD(GetFontFromFontFace)(
        _In_ IDWriteFontFace* fontFace,
        _COM_Outptr_ IDWriteFont** font
        ) PURE;
};

/// <summary>
/// The IDWriteFontList interface represents a list of fonts.
/// </summary>
interface DWRITE_DECLARE_INTERFACE("1a0d8438-1d97-4ec1-aef9-a2fb86ed6acb") IDWriteFontList : public IUnknown
{
    /// <summary>
    /// Gets the font collection that contains the fonts.
    /// </summary>
    /// <param name="fontCollection">Receives a pointer to the font collection object.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetFontCollection)(
        _COM_Outptr_ IDWriteFontCollection** fontCollection
        ) PURE;

    /// <summary>
    /// Gets the number of fonts in the font list.
    /// </summary>
    STDMETHOD_(UINT32, GetFontCount)() PURE;

    /// <summary>
    /// Gets a font given its zero-based index.
    /// </summary>
    /// <param name="index">Zero-based index of the font in the font list.</param>
    /// <param name="font">Receives a pointer to the newly created font object.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetFont)(
        UINT32 index, 
        _COM_Outptr_ IDWriteFont** font
        ) PURE;
};

/// <summary>
/// The IDWriteFontFamily interface represents a set of fonts that share the same design but are differentiated
/// by weight, stretch, and style.
/// </summary>
interface DWRITE_DECLARE_INTERFACE("da20d8ef-812a-4c43-9802-62ec4abd7add") IDWriteFontFamily : public IDWriteFontList
{
    /// <summary>
    /// Creates a localized strings object that contains the family names for the font family, indexed by locale name.
    /// </summary>
    /// <param name="names">Receives a pointer to the newly created localized strings object.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetFamilyNames)(
        _COM_Outptr_ IDWriteLocalizedStrings** names
        ) PURE;

    /// <summary>
    /// Gets the font that best matches the specified properties.
    /// </summary>
    /// <param name="weight">Requested font weight.</param>
    /// <param name="stretch">Requested font stretch.</param>
    /// <param name="style">Requested font style.</param>
    /// <param name="matchingFont">Receives a pointer to the newly created font object.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetFirstMatchingFont)(
        DWRITE_FONT_WEIGHT  weight,
        DWRITE_FONT_STRETCH stretch,
        DWRITE_FONT_STYLE   style,
        _COM_Outptr_ IDWriteFont** matchingFont
        ) PURE;

    /// <summary>
    /// Gets a list of fonts in the font family ranked in order of how well they match the specified properties.
    /// </summary>
    /// <param name="weight">Requested font weight.</param>
    /// <param name="stretch">Requested font stretch.</param>
    /// <param name="style">Requested font style.</param>
    /// <param name="matchingFonts">Receives a pointer to the newly created font list object.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetMatchingFonts)(
        DWRITE_FONT_WEIGHT      weight,
        DWRITE_FONT_STRETCH     stretch,
        DWRITE_FONT_STYLE       style,
        _COM_Outptr_ IDWriteFontList** matchingFonts
        ) PURE;
};

/// <summary>
/// The IDWriteFont interface represents a physical font in a font collection.
/// </summary>
interface DWRITE_DECLARE_INTERFACE("acd16696-8c14-4f5d-877e-fe3fc1d32737") IDWriteFont : public IUnknown
{
    /// <summary>
    /// Gets the font family to which the specified font belongs.
    /// </summary>
    /// <param name="fontFamily">Receives a pointer to the font family object.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetFontFamily)(
        _COM_Outptr_ IDWriteFontFamily** fontFamily
        ) PURE;

    /// <summary>
    /// Gets the weight of the specified font.
    /// </summary>
    STDMETHOD_(DWRITE_FONT_WEIGHT, GetWeight)() PURE;

    /// <summary>
    /// Gets the stretch (aka. width) of the specified font.
    /// </summary>
    STDMETHOD_(DWRITE_FONT_STRETCH, GetStretch)() PURE;

    /// <summary>
    /// Gets the style (aka. slope) of the specified font.
    /// </summary>
    STDMETHOD_(DWRITE_FONT_STYLE, GetStyle)() PURE;

    /// <summary>
    /// Returns TRUE if the font is a symbol font or FALSE if not.
    /// </summary>
    STDMETHOD_(BOOL, IsSymbolFont)() PURE;

    /// <summary>
    /// Gets a localized strings collection containing the face names for the font (e.g., Regular or Bold), indexed by locale name.
    /// </summary>
    /// <param name="names">Receives a pointer to the newly created localized strings object.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetFaceNames)(
        _COM_Outptr_ IDWriteLocalizedStrings** names
        ) PURE;

    /// <summary>
    /// Gets a localized strings collection containing the specified informational strings, indexed by locale name.
    /// </summary>
    /// <param name="informationalStringID">Identifies the string to get.</param>
    /// <param name="informationalStrings">Receives a pointer to the newly created localized strings object.</param>
    /// <param name="exists">Receives the value TRUE if the font contains the specified string ID or FALSE if not.</param>
    /// <returns>
    /// Standard HRESULT error code. If the font does not contain the specified string, the return value is S_OK but 
    /// informationalStrings receives a NULL pointer and exists receives the value FALSE.
    /// </returns>
    STDMETHOD(GetInformationalStrings)(
        DWRITE_INFORMATIONAL_STRING_ID informationalStringID,
        _COM_Outptr_result_maybenull_ IDWriteLocalizedStrings** informationalStrings,
        _Out_ BOOL* exists
        ) PURE;

    /// <summary>
    /// Gets a value that indicates what simulation are applied to the specified font.
    /// </summary>
    STDMETHOD_(DWRITE_FONT_SIMULATIONS, GetSimulations)() PURE;

    /// <summary>
    /// Gets the metrics for the font.
    /// </summary>
    /// <param name="fontMetrics">Receives the font metrics.</param>
    STDMETHOD_(void, GetMetrics)(
        _Out_ DWRITE_FONT_METRICS* fontMetrics
        ) PURE;

    /// <summary>
    /// Determines whether the font supports the specified character.
    /// </summary>
    /// <param name="unicodeValue">Unicode (UCS-4) character value.</param>
    /// <param name="exists">Receives the value TRUE if the font supports the specified character or FALSE if not.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(HasCharacter)(
        UINT32 unicodeValue,
        _Out_ BOOL* exists
        ) PURE;

    /// <summary>
    /// Creates a font face object for the font.
    /// </summary>
    /// <param name="fontFace">Receives a pointer to the newly created font face object.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(CreateFontFace)(
        _COM_Outptr_ IDWriteFontFace** fontFace
        ) PURE;
};

/// <summary>
/// Direction for how reading progresses.
/// </summary>
enum DWRITE_READING_DIRECTION
{
    /// <summary>
    /// Reading progresses from left to right.
    /// </summary>
    DWRITE_READING_DIRECTION_LEFT_TO_RIGHT = 0,

    /// <summary>
    /// Reading progresses from right to left.
    /// </summary>
    DWRITE_READING_DIRECTION_RIGHT_TO_LEFT = 1,

    /// <summary>
    /// Reading progresses from top to bottom.
    /// </summary>
    DWRITE_READING_DIRECTION_TOP_TO_BOTTOM = 2,

    /// <summary>
    /// Reading progresses from bottom to top.
    /// </summary>
    DWRITE_READING_DIRECTION_BOTTOM_TO_TOP = 3,
};

/// <summary>
/// Direction for how lines of text are placed relative to one another.
/// </summary>
enum DWRITE_FLOW_DIRECTION
{
    /// <summary>
    /// Text lines are placed from top to bottom.
    /// </summary>
    DWRITE_FLOW_DIRECTION_TOP_TO_BOTTOM = 0,

    /// <summary>
    /// Text lines are placed from bottom to top.
    /// </summary>
    DWRITE_FLOW_DIRECTION_BOTTOM_TO_TOP = 1,

    /// <summary>
    /// Text lines are placed from left to right.
    /// </summary>
    DWRITE_FLOW_DIRECTION_LEFT_TO_RIGHT = 2,

    /// <summary>
    /// Text lines are placed from right to left.
    /// </summary>
    DWRITE_FLOW_DIRECTION_RIGHT_TO_LEFT = 3,
};

/// <summary>
/// Alignment of paragraph text along the reading direction axis relative to 
/// the leading and trailing edge of the layout box.
/// </summary>
enum DWRITE_TEXT_ALIGNMENT
{
    /// <summary>
    /// The leading edge of the paragraph text is aligned to the layout box's leading edge.
    /// </summary>
    DWRITE_TEXT_ALIGNMENT_LEADING,

    /// <summary>
    /// The trailing edge of the paragraph text is aligned to the layout box's trailing edge.
    /// </summary>
    DWRITE_TEXT_ALIGNMENT_TRAILING,

    /// <summary>
    /// The center of the paragraph text is aligned to the center of the layout box.
    /// </summary>
    DWRITE_TEXT_ALIGNMENT_CENTER,

    /// <summary>
    /// Align text to the leading side, and also justify text to fill the lines.
    /// </summary>
    DWRITE_TEXT_ALIGNMENT_JUSTIFIED
};

/// <summary>
/// Alignment of paragraph text along the flow direction axis relative to the
/// flow's beginning and ending edge of the layout box.
/// </summary>
enum DWRITE_PARAGRAPH_ALIGNMENT
{
    /// <summary>
    /// The first line of paragraph is aligned to the flow's beginning edge of the layout box.
    /// </summary>
    DWRITE_PARAGRAPH_ALIGNMENT_NEAR,

    /// <summary>
    /// The last line of paragraph is aligned to the flow's ending edge of the layout box.
    /// </summary>
    DWRITE_PARAGRAPH_ALIGNMENT_FAR,

    /// <summary>
    /// The center of the paragraph is aligned to the center of the flow of the layout box.
    /// </summary>
    DWRITE_PARAGRAPH_ALIGNMENT_CENTER
};

/// <summary>
/// Word wrapping in multiline paragraph.
/// </summary>
enum DWRITE_WORD_WRAPPING
{
    /// <summary>
    /// Words are broken across lines to avoid text overflowing the layout box.
    /// </summary>
    DWRITE_WORD_WRAPPING_WRAP = 0,

    /// <summary>
    /// Words are kept within the same line even when it overflows the layout box.
    /// This option is often used with scrolling to reveal overflow text. 
    /// </summary>
    DWRITE_WORD_WRAPPING_NO_WRAP = 1,

    /// <summary>
    /// Words are broken across lines to avoid text overflowing the layout box.
    /// Emergency wrapping occurs if the word is larger than the maximum width.
    /// </summary>
    DWRITE_WORD_WRAPPING_EMERGENCY_BREAK = 2,

    /// <summary>
    /// Only wrap whole words, never breaking words (emergency wrapping) when the
    /// layout width is too small for even a single word.
    /// </summary>
    DWRITE_WORD_WRAPPING_WHOLE_WORD = 3,

    /// <summary>
    /// Wrap between any valid characters clusters.
    /// </summary>
    DWRITE_WORD_WRAPPING_CHARACTER = 4,
};

/// <summary>
/// The method used for line spacing in layout.
/// </summary>
enum DWRITE_LINE_SPACING_METHOD
{
    /// <summary>
    /// Line spacing depends solely on the content, growing to accommodate the size of fonts and inline objects.
    /// </summary>
    DWRITE_LINE_SPACING_METHOD_DEFAULT,

    /// <summary>
    /// Lines are explicitly set to uniform spacing, regardless of contained font sizes.
    /// This can be useful to avoid the uneven appearance that can occur from font fallback.
    /// </summary>
    DWRITE_LINE_SPACING_METHOD_UNIFORM,

    /// <summary>
    /// Line spacing and baseline distances are proportional to the computed values based on the content, the size of the fonts and inline objects.
    /// </summary>
    DWRITE_LINE_SPACING_METHOD_PROPORTIONAL
};

/// <summary>
/// Text granularity used to trim text overflowing the layout box.
/// </summary>
enum DWRITE_TRIMMING_GRANULARITY
{
    /// <summary>
    /// No trimming occurs. Text flows beyond the layout width.
    /// </summary>
    DWRITE_TRIMMING_GRANULARITY_NONE,

    /// <summary>
    /// Trimming occurs at character cluster boundary.
    /// </summary>
    DWRITE_TRIMMING_GRANULARITY_CHARACTER,
    
    /// <summary>
    /// Trimming occurs at word boundary.
    /// </summary>
    DWRITE_TRIMMING_GRANULARITY_WORD    
};

/// <summary>
/// Typographic feature of text supplied by the font.
/// </summary>
enum DWRITE_FONT_FEATURE_TAG
{
    DWRITE_FONT_FEATURE_TAG_ALTERNATIVE_FRACTIONS               = 0x63726661, // 'afrc'
    DWRITE_FONT_FEATURE_TAG_PETITE_CAPITALS_FROM_CAPITALS       = 0x63703263, // 'c2pc'
    DWRITE_FONT_FEATURE_TAG_SMALL_CAPITALS_FROM_CAPITALS        = 0x63733263, // 'c2sc'
    DWRITE_FONT_FEATURE_TAG_CONTEXTUAL_ALTERNATES               = 0x746c6163, // 'calt'
    DWRITE_FONT_FEATURE_TAG_CASE_SENSITIVE_FORMS                = 0x65736163, // 'case'
    DWRITE_FONT_FEATURE_TAG_GLYPH_COMPOSITION_DECOMPOSITION     = 0x706d6363, // 'ccmp'
    DWRITE_FONT_FEATURE_TAG_CONTEXTUAL_LIGATURES                = 0x67696c63, // 'clig'
    DWRITE_FONT_FEATURE_TAG_CAPITAL_SPACING                     = 0x70737063, // 'cpsp'
    DWRITE_FONT_FEATURE_TAG_CONTEXTUAL_SWASH                    = 0x68777363, // 'cswh'
    DWRITE_FONT_FEATURE_TAG_CURSIVE_POSITIONING                 = 0x73727563, // 'curs'
    DWRITE_FONT_FEATURE_TAG_DEFAULT                             = 0x746c6664, // 'dflt'
    DWRITE_FONT_FEATURE_TAG_DISCRETIONARY_LIGATURES             = 0x67696c64, // 'dlig'
    DWRITE_FONT_FEATURE_TAG_EXPERT_FORMS                        = 0x74707865, // 'expt'
    DWRITE_FONT_FEATURE_TAG_FRACTIONS                           = 0x63617266, // 'frac'
    DWRITE_FONT_FEATURE_TAG_FULL_WIDTH                          = 0x64697766, // 'fwid'
    DWRITE_FONT_FEATURE_TAG_HALF_FORMS                          = 0x666c6168, // 'half'
    DWRITE_FONT_FEATURE_TAG_HALANT_FORMS                        = 0x6e6c6168, // 'haln'
    DWRITE_FONT_FEATURE_TAG_ALTERNATE_HALF_WIDTH                = 0x746c6168, // 'halt'
    DWRITE_FONT_FEATURE_TAG_HISTORICAL_FORMS                    = 0x74736968, // 'hist'
    DWRITE_FONT_FEATURE_TAG_HORIZONTAL_KANA_ALTERNATES          = 0x616e6b68, // 'hkna'
    DWRITE_FONT_FEATURE_TAG_HISTORICAL_LIGATURES                = 0x67696c68, // 'hlig'
    DWRITE_FONT_FEATURE_TAG_HALF_WIDTH                          = 0x64697768, // 'hwid'
    DWRITE_FONT_FEATURE_TAG_HOJO_KANJI_FORMS                    = 0x6f6a6f68, // 'hojo'
    DWRITE_FONT_FEATURE_TAG_JIS04_FORMS                         = 0x3430706a, // 'jp04'
    DWRITE_FONT_FEATURE_TAG_JIS78_FORMS                         = 0x3837706a, // 'jp78'
    DWRITE_FONT_FEATURE_TAG_JIS83_FORMS                         = 0x3338706a, // 'jp83'
    DWRITE_FONT_FEATURE_TAG_JIS90_FORMS                         = 0x3039706a, // 'jp90'
    DWRITE_FONT_FEATURE_TAG_KERNING                             = 0x6e72656b, // 'kern'
    DWRITE_FONT_FEATURE_TAG_STANDARD_LIGATURES                  = 0x6167696c, // 'liga'
    DWRITE_FONT_FEATURE_TAG_LINING_FIGURES                      = 0x6d756e6c, // 'lnum'
    DWRITE_FONT_FEATURE_TAG_LOCALIZED_FORMS                     = 0x6c636f6c, // 'locl'
    DWRITE_FONT_FEATURE_TAG_MARK_POSITIONING                    = 0x6b72616d, // 'mark'
    DWRITE_FONT_FEATURE_TAG_MATHEMATICAL_GREEK                  = 0x6b72676d, // 'mgrk'
    DWRITE_FONT_FEATURE_TAG_MARK_TO_MARK_POSITIONING            = 0x6b6d6b6d, // 'mkmk'
    DWRITE_FONT_FEATURE_TAG_ALTERNATE_ANNOTATION_FORMS          = 0x746c616e, // 'nalt'
    DWRITE_FONT_FEATURE_TAG_NLC_KANJI_FORMS                     = 0x6b636c6e, // 'nlck'
    DWRITE_FONT_FEATURE_TAG_OLD_STYLE_FIGURES                   = 0x6d756e6f, // 'onum'
    DWRITE_FONT_FEATURE_TAG_ORDINALS                            = 0x6e64726f, // 'ordn'
    DWRITE_FONT_FEATURE_TAG_PROPORTIONAL_ALTERNATE_WIDTH        = 0x746c6170, // 'palt'
    DWRITE_FONT_FEATURE_TAG_PETITE_CAPITALS                     = 0x70616370, // 'pcap'
    DWRITE_FONT_FEATURE_TAG_PROPORTIONAL_FIGURES                = 0x6d756e70, // 'pnum'
    DWRITE_FONT_FEATURE_TAG_PROPORTIONAL_WIDTHS                 = 0x64697770, // 'pwid'
    DWRITE_FONT_FEATURE_TAG_QUARTER_WIDTHS                      = 0x64697771, // 'qwid'
    DWRITE_FONT_FEATURE_TAG_REQUIRED_LIGATURES                  = 0x67696c72, // 'rlig'
    DWRITE_FONT_FEATURE_TAG_RUBY_NOTATION_FORMS                 = 0x79627572, // 'ruby'
    DWRITE_FONT_FEATURE_TAG_STYLISTIC_ALTERNATES                = 0x746c6173, // 'salt'
    DWRITE_FONT_FEATURE_TAG_SCIENTIFIC_INFERIORS                = 0x666e6973, // 'sinf'
    DWRITE_FONT_FEATURE_TAG_SMALL_CAPITALS                      = 0x70636d73, // 'smcp'
    DWRITE_FONT_FEATURE_TAG_SIMPLIFIED_FORMS                    = 0x6c706d73, // 'smpl'
    DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_1                     = 0x31307373, // 'ss01'
    DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_2                     = 0x32307373, // 'ss02'
    DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_3                     = 0x33307373, // 'ss03'
    DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_4                     = 0x34307373, // 'ss04'
    DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_5                     = 0x35307373, // 'ss05'
    DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_6                     = 0x36307373, // 'ss06'
    DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_7                     = 0x37307373, // 'ss07'
    DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_8                     = 0x38307373, // 'ss08'
    DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_9                     = 0x39307373, // 'ss09'
    DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_10                    = 0x30317373, // 'ss10'
    DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_11                    = 0x31317373, // 'ss11'
    DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_12                    = 0x32317373, // 'ss12'
    DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_13                    = 0x33317373, // 'ss13'
    DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_14                    = 0x34317373, // 'ss14'
    DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_15                    = 0x35317373, // 'ss15'
    DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_16                    = 0x36317373, // 'ss16'
    DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_17                    = 0x37317373, // 'ss17'
    DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_18                    = 0x38317373, // 'ss18'
    DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_19                    = 0x39317373, // 'ss19'
    DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_20                    = 0x30327373, // 'ss20'
    DWRITE_FONT_FEATURE_TAG_SUBSCRIPT                           = 0x73627573, // 'subs'
    DWRITE_FONT_FEATURE_TAG_SUPERSCRIPT                         = 0x73707573, // 'sups'
    DWRITE_FONT_FEATURE_TAG_SWASH                               = 0x68737773, // 'swsh'
    DWRITE_FONT_FEATURE_TAG_TITLING                             = 0x6c746974, // 'titl'
    DWRITE_FONT_FEATURE_TAG_TRADITIONAL_NAME_FORMS              = 0x6d616e74, // 'tnam'
    DWRITE_FONT_FEATURE_TAG_TABULAR_FIGURES                     = 0x6d756e74, // 'tnum'
    DWRITE_FONT_FEATURE_TAG_TRADITIONAL_FORMS                   = 0x64617274, // 'trad'
    DWRITE_FONT_FEATURE_TAG_THIRD_WIDTHS                        = 0x64697774, // 'twid'
    DWRITE_FONT_FEATURE_TAG_UNICASE                             = 0x63696e75, // 'unic'
    DWRITE_FONT_FEATURE_TAG_VERTICAL_WRITING                    = 0x74726576, // 'vert'
    DWRITE_FONT_FEATURE_TAG_VERTICAL_ALTERNATES_AND_ROTATION    = 0x32747276, // 'vrt2'
    DWRITE_FONT_FEATURE_TAG_SLASHED_ZERO                        = 0x6f72657a, // 'zero'
};

/// <summary>
/// The DWRITE_TEXT_RANGE structure specifies a range of text positions where format is applied.
/// </summary>
struct DWRITE_TEXT_RANGE
{
    /// <summary>
    /// The start text position of the range.
    /// </summary>
    UINT32 startPosition;

    /// <summary>
    /// The number of text positions in the range.
    /// </summary>
    UINT32 length;
};

/// <summary>
/// The DWRITE_FONT_FEATURE structure specifies properties used to identify and execute typographic feature in the font.
/// </summary>
struct DWRITE_FONT_FEATURE
{
    /// <summary>
    /// The feature OpenType name identifier.
    /// </summary>
    DWRITE_FONT_FEATURE_TAG nameTag;

    /// <summary>
    /// Execution parameter of the feature.
    /// </summary>
    /// <remarks>
    /// The parameter should be non-zero to enable the feature.  Once enabled, a feature can't be disabled again within
    /// the same range.  Features requiring a selector use this value to indicate the selector index. 
    /// </remarks>
    UINT32 parameter;
};

/// <summary>
/// Defines a set of typographic features to be applied during shaping.
/// Notice the character range which this feature list spans is specified
/// as a separate parameter to GetGlyphs.
/// </summary>
struct DWRITE_TYPOGRAPHIC_FEATURES
{
    /// <summary>
    /// Array of font features.
    /// </summary>
    _Field_size_(featureCount) DWRITE_FONT_FEATURE* features;

    /// <summary>
    /// The number of features.
    /// </summary>
    UINT32 featureCount;
};

/// <summary>
/// The DWRITE_TRIMMING structure specifies the trimming option for text overflowing the layout box.
/// </summary>
struct DWRITE_TRIMMING
{
    /// <summary>
    /// Text granularity of which trimming applies.
    /// </summary>
    DWRITE_TRIMMING_GRANULARITY granularity;

    /// <summary>
    /// Character code used as the delimiter signaling the beginning of the portion of text to be preserved,
    /// most useful for path ellipsis, where the delimiter would be a slash. Leave this zero if there is no
    /// delimiter.
    /// </summary>
    UINT32 delimiter;

    /// <summary>
    /// How many occurrences of the delimiter to step back. Leave this zero if there is no delimiter.
    /// </summary>
    UINT32 delimiterCount;
};


interface IDWriteTypography;
interface IDWriteInlineObject;

/// <summary>
/// The format of text used for text layout.
/// </summary>
/// <remarks>
/// This object may not be thread-safe and it may carry the state of text format change.
/// </remarks>
interface DWRITE_DECLARE_INTERFACE("9c906818-31d7-4fd3-a151-7c5e225db55a") IDWriteTextFormat : public IUnknown
{
    /// <summary>
    /// Set alignment option of text relative to layout box's leading and trailing edge.
    /// </summary>
    /// <param name="textAlignment">Text alignment option</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(SetTextAlignment)(
        DWRITE_TEXT_ALIGNMENT textAlignment
        ) PURE;

    /// <summary>
    /// Set alignment option of paragraph relative to layout box's top and bottom edge.
    /// </summary>
    /// <param name="paragraphAlignment">Paragraph alignment option</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(SetParagraphAlignment)(
        DWRITE_PARAGRAPH_ALIGNMENT paragraphAlignment
        ) PURE;

    /// <summary>
    /// Set word wrapping option.
    /// </summary>
    /// <param name="wordWrapping">Word wrapping option</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(SetWordWrapping)(
        DWRITE_WORD_WRAPPING wordWrapping
        ) PURE;

    /// <summary>
    /// Set paragraph reading direction.
    /// </summary>
    /// <param name="readingDirection">Text reading direction</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    /// <remarks>
    /// The flow direction must be perpendicular to the reading direction.
    /// Setting both to a vertical direction or both to horizontal yields
    /// DWRITE_E_FLOWDIRECTIONCONFLICTS when calling GetMetrics or Draw.
    /// </remark>
    STDMETHOD(SetReadingDirection)(
        DWRITE_READING_DIRECTION readingDirection
        ) PURE;

    /// <summary>
    /// Set paragraph flow direction.
    /// </summary>
    /// <param name="flowDirection">Paragraph flow direction</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    /// <remarks>
    /// The flow direction must be perpendicular to the reading direction.
    /// Setting both to a vertical direction or both to horizontal yields
    /// DWRITE_E_FLOWDIRECTIONCONFLICTS when calling GetMetrics or Draw.
    /// </remark>
    STDMETHOD(SetFlowDirection)(
        DWRITE_FLOW_DIRECTION flowDirection
        ) PURE;

    /// <summary>
    /// Set incremental tab stop position.
    /// </summary>
    /// <param name="incrementalTabStop">The incremental tab stop value</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(SetIncrementalTabStop)(
        FLOAT incrementalTabStop
        ) PURE;

    /// <summary>
    /// Set trimming options for any trailing text exceeding the layout width
    /// or for any far text exceeding the layout height.
    /// </summary>
    /// <param name="trimmingOptions">Text trimming options.</param>
    /// <param name="trimmingSign">Application-defined omission sign. This parameter may be NULL if no trimming sign is desired.</param>
    /// <remarks>
    /// Any inline object can be used for the trimming sign, but CreateEllipsisTrimmingSign
    /// provides a typical ellipsis symbol. Trimming is also useful vertically for hiding
    /// partial lines.
    /// </remarks>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(SetTrimming)(
        _In_ DWRITE_TRIMMING const* trimmingOptions,
        _In_opt_ IDWriteInlineObject* trimmingSign
        ) PURE;

    /// <summary>
    /// Set line spacing.
    /// </summary>
    /// <param name="lineSpacingMethod">How to determine line height.</param>
    /// <param name="lineSpacing">The line height, or rather distance between one baseline to another.</param>
    /// <param name="baseline">Distance from top of line to baseline. A reasonable ratio to lineSpacing is 80%.</param>
    /// <remarks>
    /// For the default method, spacing depends solely on the content.
    /// For uniform spacing, the given line height will override the content.
    /// </remarks>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(SetLineSpacing)(
        DWRITE_LINE_SPACING_METHOD lineSpacingMethod,
        FLOAT lineSpacing,
        FLOAT baseline
        ) PURE;

    /// <summary>
    /// Get alignment option of text relative to layout box's leading and trailing edge.
    /// </summary>
    STDMETHOD_(DWRITE_TEXT_ALIGNMENT, GetTextAlignment)() PURE;

    /// <summary>
    /// Get alignment option of paragraph relative to layout box's top and bottom edge.
    /// </summary>
    STDMETHOD_(DWRITE_PARAGRAPH_ALIGNMENT, GetParagraphAlignment)() PURE;

    /// <summary>
    /// Get word wrapping option.
    /// </summary>
    STDMETHOD_(DWRITE_WORD_WRAPPING, GetWordWrapping)() PURE;

    /// <summary>
    /// Get paragraph reading direction.
    /// </summary>
    STDMETHOD_(DWRITE_READING_DIRECTION, GetReadingDirection)() PURE;

    /// <summary>
    /// Get paragraph flow direction.
    /// </summary>
    STDMETHOD_(DWRITE_FLOW_DIRECTION, GetFlowDirection)() PURE;

    /// <summary>
    /// Get incremental tab stop position.
    /// </summary>
    STDMETHOD_(FLOAT, GetIncrementalTabStop)() PURE;

    /// <summary>
    /// Get trimming options for text overflowing the layout width.
    /// </summary>
    /// <param name="trimmingOptions">Text trimming options.</param>
    /// <param name="trimmingSign">Trimming omission sign. This parameter may be NULL.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetTrimming)(
        _Out_ DWRITE_TRIMMING* trimmingOptions,
        _COM_Outptr_ IDWriteInlineObject** trimmingSign
        ) PURE;

    /// <summary>
    /// Get line spacing.
    /// </summary>
    /// <param name="lineSpacingMethod">How line height is determined.</param>
    /// <param name="lineSpacing">The line height, or rather distance between one baseline to another.</param>
    /// <param name="baseline">Distance from top of line to baseline.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetLineSpacing)(
        _Out_ DWRITE_LINE_SPACING_METHOD* lineSpacingMethod,
        _Out_ FLOAT* lineSpacing,
        _Out_ FLOAT* baseline
        ) PURE;

    /// <summary>
    /// Get the font collection.
    /// </summary>
    /// <param name="fontCollection">The current font collection.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetFontCollection)(
        _COM_Outptr_ IDWriteFontCollection** fontCollection
        ) PURE;

    /// <summary>
    /// Get the length of the font family name, in characters, not including the terminating NULL character.
    /// </summary>
    STDMETHOD_(UINT32, GetFontFamilyNameLength)() PURE;

    /// <summary>
    /// Get a copy of the font family name.
    /// </summary>
    /// <param name="fontFamilyName">Character array that receives the current font family name</param>
    /// <param name="nameSize">Size of the character array in character count including the terminated NULL character.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetFontFamilyName)(
        _Out_writes_z_(nameSize) WCHAR* fontFamilyName,
        UINT32 nameSize
        ) PURE;

    /// <summary>
    /// Get the font weight.
    /// </summary>
    STDMETHOD_(DWRITE_FONT_WEIGHT, GetFontWeight)() PURE;

    /// <summary>
    /// Get the font style.
    /// </summary>
    STDMETHOD_(DWRITE_FONT_STYLE, GetFontStyle)() PURE;

    /// <summary>
    /// Get the font stretch.
    /// </summary>
    STDMETHOD_(DWRITE_FONT_STRETCH, GetFontStretch)() PURE;

    /// <summary>
    /// Get the font em height.
    /// </summary>
    STDMETHOD_(FLOAT, GetFontSize)() PURE;

    /// <summary>
    /// Get the length of the locale name, in characters, not including the terminating NULL character.
    /// </summary>
    STDMETHOD_(UINT32, GetLocaleNameLength)() PURE;

    /// <summary>
    /// Get a copy of the locale name.
    /// </summary>
    /// <param name="localeName">Character array that receives the current locale name</param>
    /// <param name="nameSize">Size of the character array in character count including the terminated NULL character.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetLocaleName)(
        _Out_writes_z_(nameSize) WCHAR* localeName,
        UINT32 nameSize
        ) PURE;
};


/// <summary>
/// Font typography setting.
/// </summary>
interface DWRITE_DECLARE_INTERFACE("55f1112b-1dc2-4b3c-9541-f46894ed85b6") IDWriteTypography : public IUnknown
{
    /// <summary>
    /// Add font feature.
    /// </summary>
    /// <param name="fontFeature">The font feature to add.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(AddFontFeature)(
        DWRITE_FONT_FEATURE fontFeature
        ) PURE;

    /// <summary>
    /// Get the number of font features.
    /// </summary>
    STDMETHOD_(UINT32, GetFontFeatureCount)() PURE;

    /// <summary>
    /// Get the font feature at the specified index.
    /// </summary>
    /// <param name="fontFeatureIndex">The zero-based index of the font feature to get.</param>
    /// <param name="fontFeature">The font feature.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetFontFeature)(
        UINT32 fontFeatureIndex,
        _Out_ DWRITE_FONT_FEATURE* fontFeature
        ) PURE;
};

enum DWRITE_SCRIPT_SHAPES
{
    /// <summary>
    /// No additional shaping requirement. Text is shaped with the writing system default behavior.
    /// </summary>
    DWRITE_SCRIPT_SHAPES_DEFAULT = 0,

    /// <summary>
    /// Text should leave no visual on display i.e. control or format control characters.
    /// </summary>
    DWRITE_SCRIPT_SHAPES_NO_VISUAL = 1
};

#ifdef DEFINE_ENUM_FLAG_OPERATORS
DEFINE_ENUM_FLAG_OPERATORS(DWRITE_SCRIPT_SHAPES);
#endif

/// <summary>
/// Association of text and its writing system script as well as some display attributes.
/// </summary>
struct DWRITE_SCRIPT_ANALYSIS
{
    /// <summary>
    /// Zero-based index representation of writing system script.
    /// </summary>
    UINT16 script;

    /// <summary>
    /// Additional shaping requirement of text.
    /// </summary>
    DWRITE_SCRIPT_SHAPES shapes;
};

/// <summary>
/// Condition at the edges of inline object or text used to determine
/// line-breaking behavior.
/// </summary>
enum DWRITE_BREAK_CONDITION
{
    /// <summary>
    /// Whether a break is allowed is determined by the condition of the
    /// neighboring text span or inline object.
    /// </summary>
    DWRITE_BREAK_CONDITION_NEUTRAL,

    /// <summary>
    /// A break is allowed, unless overruled by the condition of the
    /// neighboring text span or inline object, either prohibited by a
    /// May Not or forced by a Must.
    /// </summary>
    DWRITE_BREAK_CONDITION_CAN_BREAK,

    /// <summary>
    /// There should be no break, unless overruled by a Must condition from
    /// the neighboring text span or inline object.
    /// </summary>
    DWRITE_BREAK_CONDITION_MAY_NOT_BREAK,

    /// <summary>
    /// The break must happen, regardless of the condition of the adjacent
    /// text span or inline object.
    /// </summary>
    DWRITE_BREAK_CONDITION_MUST_BREAK
};

/// <summary>
/// Line breakpoint characteristics of a character.
/// </summary>
struct DWRITE_LINE_BREAKPOINT
{
    /// <summary>
    /// Breaking condition before the character.
    /// </summary>
    UINT8 breakConditionBefore  : 2;

    /// <summary>
    /// Breaking condition after the character.
    /// </summary>
    UINT8 breakConditionAfter   : 2;

    /// <summary>
    /// The character is some form of whitespace, which may be meaningful
    /// for justification.
    /// </summary>
    UINT8 isWhitespace          : 1;

    /// <summary>
    /// The character is a soft hyphen, often used to indicate hyphenation
    /// points inside words.
    /// </summary>
    UINT8 isSoftHyphen          : 1;

    UINT8 padding               : 2;
};

/// <summary>
/// How to apply number substitution on digits and related punctuation.
/// </summary>
enum DWRITE_NUMBER_SUBSTITUTION_METHOD
{
    /// <summary>
    /// Specifies that the substitution method should be determined based
    /// on LOCALE_IDIGITSUBSTITUTION value of the specified text culture.
    /// </summary>
    DWRITE_NUMBER_SUBSTITUTION_METHOD_FROM_CULTURE,

    /// <summary>
    /// If the culture is Arabic or Farsi, specifies that the number shape
    /// depend on the context. Either traditional or nominal number shape
    /// are used depending on the nearest preceding strong character or (if
    /// there is none) the reading direction of the paragraph.
    /// </summary>
    DWRITE_NUMBER_SUBSTITUTION_METHOD_CONTEXTUAL,

    /// <summary>
    /// Specifies that code points 0x30-0x39 are always rendered as nominal numeral 
    /// shapes (ones of the European number), i.e., no substitution is performed.
    /// </summary>
    DWRITE_NUMBER_SUBSTITUTION_METHOD_NONE,

    /// <summary>
    /// Specifies that number are rendered using the national number shape 
    /// as specified by the LOCALE_SNATIVEDIGITS value of the specified text culture.
    /// </summary>
    DWRITE_NUMBER_SUBSTITUTION_METHOD_NATIONAL,

    /// <summary>
    /// Specifies that number are rendered using the traditional shape
    /// for the specified culture. For most cultures, this is the same as
    /// NativeNational. However, NativeNational results in Latin number
    /// for some Arabic cultures, whereas this value results in Arabic
    /// number for all Arabic cultures.
    /// </summary>
    DWRITE_NUMBER_SUBSTITUTION_METHOD_TRADITIONAL
};

/// <summary>
/// Holds the appropriate digits and numeric punctuation for a given locale.
/// </summary>
interface DWRITE_DECLARE_INTERFACE("14885CC9-BAB0-4f90-B6ED-5C366A2CD03D") IDWriteNumberSubstitution : public IUnknown
{
};

/// <summary>
/// Shaping output properties per input character.
/// </summary>
struct DWRITE_SHAPING_TEXT_PROPERTIES
{
    /// <summary>
    /// This character can be shaped independently from the others
    /// (usually set for the space character).
    /// </summary>
    UINT16  isShapedAlone       : 1;

    /// <summary>
    /// Reserved for use by shaping engine.
    /// </summary>
    UINT16  reserved            : 15;
};

/// <summary>
/// Shaping output properties per output glyph.
/// </summary>
struct DWRITE_SHAPING_GLYPH_PROPERTIES
{
    /// <summary>
    /// Justification class, whether to use spacing, kashidas, or
    /// another method. This exists for backwards compatibility
    /// with Uniscribe's SCRIPT_JUSTIFY enum.
    /// </summary>
    UINT16  justification       : 4;

    /// <summary>
    /// Indicates glyph is the first of a cluster.
    /// </summary>
    UINT16  isClusterStart      : 1;

    /// <summary>
    /// Glyph is a diacritic.
    /// </summary>
    UINT16  isDiacritic         : 1;

    /// <summary>
    /// Glyph has no width, blank, ZWJ, ZWNJ etc.
    /// </summary>
    UINT16  isZeroWidthSpace    : 1;

    /// <summary>
    /// Reserved for use by shaping engine.
    /// </summary>
    UINT16  reserved            : 9;
};

/// <summary>
/// The interface implemented by the text analyzer's client to provide text to
/// the analyzer. It allows the separation between the logical view of text as
/// a continuous stream of characters identifiable by unique text positions,
/// and the actual memory layout of potentially discrete blocks of text in the
/// client's backing store.
///
/// If any of these callbacks returns an error, the analysis functions will
/// stop prematurely and return a callback error. Rather than return E_NOTIMPL,
/// an application should stub the method and return a constant/null and S_OK.
/// </summary>
interface DWRITE_DECLARE_INTERFACE("688e1a58-5094-47c8-adc8-fbcea60ae92b") IDWriteTextAnalysisSource : public IUnknown
{
    /// <summary>
    /// Get a block of text starting at the specified text position.
    /// Returning NULL indicates the end of text - the position is after
    /// the last character. This function is called iteratively for
    /// each consecutive block, tying together several fragmented blocks
    /// in the backing store into a virtual contiguous string.
    /// </summary>
    /// <param name="textPosition">First position of the piece to obtain. All
    ///     positions are in UTF16 code-units, not whole characters, which
    ///     matters when supplementary characters are used.</param>
    /// <param name="textString">Address that receives a pointer to the text block
    ///     at the specified position.</param>
    /// <param name="textLength">Number of UTF16 units of the retrieved chunk.
    ///     The returned length is not the length of the block, but the length
    ///     remaining in the block, from the given position until its end.
    ///     So querying for a position that is 75 positions into a 100
    ///     position block would return 25.</param>
    /// <returns>Pointer to the first character at the given text position.
    /// NULL indicates no chunk available at the specified position, either
    /// because textPosition >= the entire text content length or because the
    /// queried position is not mapped into the app's backing store.</returns>
    /// <remarks>
    /// Although apps can implement sparse textual content that only maps part of
    /// the backing store, the app must map any text that is in the range passed
    /// to any analysis functions.
    /// </remarks>
    STDMETHOD(GetTextAtPosition)(
        UINT32 textPosition,
        _Outptr_result_buffer_(*textLength) WCHAR const** textString,
        _Out_ UINT32* textLength
        ) PURE;

    /// <summary>
    /// Get a block of text immediately preceding the specified position.
    /// </summary>
    /// <param name="textPosition">Position immediately after the last position of the chunk to obtain.</param>
    /// <param name="textString">Address that receives a pointer to the text block
    ///     at the specified position.</param>
    /// <param name="textLength">Number of UTF16 units of the retrieved block.
    ///     The length returned is from the given position to the front of
    ///     the block.</param>
    /// <returns>Pointer to the first character at (textPosition - textLength).
    /// NULL indicates no chunk available at the specified position, either
    /// because textPosition == 0,the textPosition > the entire text content
    /// length, or the queried position is not mapped into the app's backing
    /// store.</returns>
    /// <remarks>
    /// Although apps can implement sparse textual content that only maps part of
    /// the backing store, the app must map any text that is in the range passed
    /// to any analysis functions.
    /// </remarks>
    STDMETHOD(GetTextBeforePosition)(
        UINT32 textPosition,
        _Outptr_result_buffer_(*textLength) WCHAR const** textString,
        _Out_ UINT32* textLength
        ) PURE;

    /// <summary>
    /// Get paragraph reading direction.
    /// </summary>
    STDMETHOD_(DWRITE_READING_DIRECTION, GetParagraphReadingDirection)() PURE;

    /// <summary>
    /// Get locale name on the range affected by it.
    /// </summary>
    /// <param name="textPosition">Position to get the locale name of.</param>
    /// <param name="textLength">Receives the length from the given position up to the
    ///     next differing locale.</param>
    /// <param name="localeName">Address that receives a pointer to the locale
    ///     at the specified position.</param>
    /// <remarks>
    /// The localeName pointer must remain valid until the next call or until
    /// the analysis returns.
    /// </remarks>
    STDMETHOD(GetLocaleName)(
        UINT32 textPosition,
        _Out_ UINT32* textLength,
        _Outptr_result_z_ WCHAR const** localeName
        ) PURE;

    /// <summary>
    /// Get number substitution on the range affected by it.
    /// </summary>
    /// <param name="textPosition">Position to get the number substitution of.</param>
    /// <param name="textLength">Receives the length from the given position up to the
    ///     next differing number substitution.</param>
    /// <param name="numberSubstitution">Address that receives a pointer to the number substitution
    ///     at the specified position.</param>
    /// <remarks>
    /// Any implementation should return the number substitution with an
    /// incremented ref count, and the analysis will release when finished
    /// with it (either before the next call or before it returns). However,
    /// the sink callback may hold onto it after that.
    /// </remarks>
    STDMETHOD(GetNumberSubstitution)(
        UINT32 textPosition,
        _Out_ UINT32* textLength,
        _COM_Outptr_ IDWriteNumberSubstitution** numberSubstitution
        ) PURE;
};

/// <summary>
/// The interface implemented by the text analyzer's client to receive the
/// output of a given text analysis. The Text analyzer disregards any current
/// state of the analysis sink, therefore a Set method call on a range
/// overwrites the previously set analysis result of the same range. 
/// </summary>
interface DWRITE_DECLARE_INTERFACE("5810cd44-0ca0-4701-b3fa-bec5182ae4f6") IDWriteTextAnalysisSink : public IUnknown
{
    /// <summary>
    /// Report script analysis for the text range.
    /// </summary>
    /// <param name="textPosition">Starting position to report from.</param>
    /// <param name="textLength">Number of UTF16 units of the reported range.</param>
    /// <param name="scriptAnalysis">Script analysis of characters in range.</param>
    /// <returns>
    /// A successful code or error code to abort analysis.
    /// </returns>
    STDMETHOD(SetScriptAnalysis)(
        UINT32 textPosition,
        UINT32 textLength,
        _In_ DWRITE_SCRIPT_ANALYSIS const* scriptAnalysis
        ) PURE;

    /// <summary>
    /// Report line-break opportunities for each character, starting from
    /// the specified position.
    /// </summary>
    /// <param name="textPosition">Starting position to report from.</param>
    /// <param name="textLength">Number of UTF16 units of the reported range.</param>
    /// <param name="lineBreakpoints">Breaking conditions for each character.</param>
    /// <returns>
    /// A successful code or error code to abort analysis.
    /// </returns>
    STDMETHOD(SetLineBreakpoints)(
        UINT32 textPosition,
        UINT32 textLength,
        _In_reads_(textLength) DWRITE_LINE_BREAKPOINT const* lineBreakpoints
        ) PURE;

    /// <summary>
    /// Set bidirectional level on the range, called once per each
    /// level run change (either explicit or resolved implicit).
    /// </summary>
    /// <param name="textPosition">Starting position to report from.</param>
    /// <param name="textLength">Number of UTF16 units of the reported range.</param>
    /// <param name="explicitLevel">Explicit level from embedded control codes
    ///     RLE/RLO/LRE/LRO/PDF, determined before any additional rules.</param>
    /// <param name="resolvedLevel">Final implicit level considering the
    ///     explicit level and characters' natural directionality, after all
    ///     Bidi rules have been applied.</param>
    /// <returns>
    /// A successful code or error code to abort analysis.
    /// </returns>
    STDMETHOD(SetBidiLevel)(
        UINT32 textPosition,
        UINT32 textLength,
        UINT8 explicitLevel,
        UINT8 resolvedLevel
        ) PURE;

    /// <summary>
    /// Set number substitution on the range.
    /// </summary>
    /// <param name="textPosition">Starting position to report from.</param>
    /// <param name="textLength">Number of UTF16 units of the reported range.</param>
    /// <param name="numberSubstitution">The number substitution applicable to
    ///     the returned range of text. The sink callback may hold onto it by
    ///     incrementing its ref count.</param>
    /// <returns>
    /// A successful code or error code to abort analysis.
    /// </returns>
    /// <remark>
    /// Unlike script and bidi analysis, where every character passed to the
    /// analyzer has a result, this will only be called for those ranges where
    /// substitution is applicable. For any other range, you will simply not
    /// be called.
    /// </remark>
    STDMETHOD(SetNumberSubstitution)(
        UINT32 textPosition,
        UINT32 textLength,
        _In_ IDWriteNumberSubstitution* numberSubstitution
        ) PURE;
};

/// <summary>
/// Analyzes various text properties for complex script processing.
/// </summary>
interface DWRITE_DECLARE_INTERFACE("b7e6163e-7f46-43b4-84b3-e4e6249c365d") IDWriteTextAnalyzer : public IUnknown
{
    /// <summary>
    /// Analyzes a text range for script boundaries, reading text attributes
    /// from the source and reporting the Unicode script ID to the sink 
    /// callback SetScript.
    /// </summary>
    /// <param name="analysisSource">Source object to analyze.</param>
    /// <param name="textPosition">Starting position within the source object.</param>
    /// <param name="textLength">Length to analyze.</param>
    /// <param name="analysisSink">Callback object.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(AnalyzeScript)(
        _In_ IDWriteTextAnalysisSource* analysisSource,
        UINT32 textPosition,
        UINT32 textLength,
        _In_ IDWriteTextAnalysisSink* analysisSink
        ) PURE;

    /// <summary>
    /// Analyzes a text range for script directionality, reading attributes
    /// from the source and reporting levels to the sink callback SetBidiLevel.
    /// </summary>
    /// <param name="analysisSource">Source object to analyze.</param>
    /// <param name="textPosition">Starting position within the source object.</param>
    /// <param name="textLength">Length to analyze.</param>
    /// <param name="analysisSink">Callback object.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    /// <remarks>
    /// While the function can handle multiple paragraphs, the text range
    /// should not arbitrarily split the middle of paragraphs. Otherwise the
    /// returned levels may be wrong, since the Bidi algorithm is meant to
    /// apply to the paragraph as a whole.
    /// </remarks>
    /// <remarks>
    /// Embedded control codes (LRE/LRO/RLE/RLO/PDF) are taken into account.
    /// </remarks>
    STDMETHOD(AnalyzeBidi)(
        _In_ IDWriteTextAnalysisSource* analysisSource,
        UINT32 textPosition,
        UINT32 textLength,
        _In_ IDWriteTextAnalysisSink* analysisSink
        ) PURE;

    /// <summary>
    /// Analyzes a text range for spans where number substitution is applicable,
    /// reading attributes from the source and reporting substitutable ranges
    /// to the sink callback SetNumberSubstitution.
    /// </summary>
    /// <param name="analysisSource">Source object to analyze.</param>
    /// <param name="textPosition">Starting position within the source object.</param>
    /// <param name="textLength">Length to analyze.</param>
    /// <param name="analysisSink">Callback object.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    /// <remarks>
    /// While the function can handle multiple ranges of differing number
    /// substitutions, the text ranges should not arbitrarily split the
    /// middle of numbers. Otherwise it will treat the numbers separately
    /// and will not translate any intervening punctuation.
    /// </remarks>
    /// <remarks>
    /// Embedded control codes (LRE/LRO/RLE/RLO/PDF) are taken into account.
    /// </remarks>
    STDMETHOD(AnalyzeNumberSubstitution)(
        _In_ IDWriteTextAnalysisSource* analysisSource,
        UINT32 textPosition,
        UINT32 textLength,
        _In_ IDWriteTextAnalysisSink* analysisSink
        ) PURE;

    /// <summary>
    /// Analyzes a text range for potential breakpoint opportunities, reading
    /// attributes from the source and reporting breakpoint opportunities to
    /// the sink callback SetLineBreakpoints.
    /// </summary>
    /// <param name="analysisSource">Source object to analyze.</param>
    /// <param name="textPosition">Starting position within the source object.</param>
    /// <param name="textLength">Length to analyze.</param>
    /// <param name="analysisSink">Callback object.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    /// <remarks>
    /// While the function can handle multiple paragraphs, the text range
    /// should not arbitrarily split the middle of paragraphs, unless the
    /// given text span is considered a whole unit. Otherwise the
    /// returned properties for the first and last characters will
    /// inappropriately allow breaks.
    /// </remarks>
    /// <remarks>
    /// Special cases include the first, last, and surrogate characters. Any
    /// text span is treated as if adjacent to inline objects on either side.
    /// So the rules with contingent-break opportunities are used, where the 
    /// edge between text and inline objects is always treated as a potential
    /// break opportunity, dependent on any overriding rules of the adjacent
    /// objects to prohibit or force the break (see Unicode TR #14).
    /// Surrogate pairs never break between.
    /// </remarks>
    STDMETHOD(AnalyzeLineBreakpoints)(
        _In_ IDWriteTextAnalysisSource* analysisSource,
        UINT32 textPosition,
        UINT32 textLength,
        _In_ IDWriteTextAnalysisSink* analysisSink
        ) PURE;

    /// <summary>
    /// Parses the input text string and maps it to the set of glyphs and associated glyph data
    /// according to the font and the writing system's rendering rules.
    /// </summary>
    /// <param name="textString">The string to convert to glyphs.</param>
    /// <param name="textLength">The length of textString.</param>
    /// <param name="fontFace">The font face to get glyphs from.</param>
    /// <param name="isSideways">Set to true if the text is intended to be
    /// drawn vertically.</param>
    /// <param name="isRightToLeft">Set to TRUE for right-to-left text.</param>
    /// <param name="scriptAnalysis">Script analysis result from AnalyzeScript.</param>
    /// <param name="localeName">The locale to use when selecting glyphs.
    /// e.g. the same character may map to different glyphs for ja-jp vs zh-chs.
    /// If this is NULL then the default mapping based on the script is used.</param>
    /// <param name="numberSubstitution">Optional number substitution which
    /// selects the appropriate glyphs for digits and related numeric characters,
    /// depending on the results obtained from AnalyzeNumberSubstitution. Passing
    /// null indicates that no substitution is needed and that the digits should
    /// receive nominal glyphs.</param>
    /// <param name="features">An array of pointers to the sets of typographic 
    /// features to use in each feature range.</param>
    /// <param name="featureRangeLengths">The length of each feature range, in characters.  
    /// The sum of all lengths should be equal to textLength.</param>
    /// <param name="featureRanges">The number of feature ranges.</param>
    /// <param name="maxGlyphCount">The maximum number of glyphs that can be
    /// returned.</param>
    /// <param name="clusterMap">The mapping from character ranges to glyph 
    /// ranges.</param>
    /// <param name="textProps">Per-character output properties.</param>
    /// <param name="glyphIndices">Output glyph indices.</param>
    /// <param name="glyphProps">Per-glyph output properties.</param>
    /// <param name="actualGlyphCount">The actual number of glyphs returned if
    /// the call succeeds.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    /// <remarks>
    /// Note that the mapping from characters to glyphs is, in general, many-
    /// to-many.  The recommended estimate for the per-glyph output buffers is
    /// (3 * textLength / 2 + 16).  This is not guaranteed to be sufficient.
    ///
    /// The value of the actualGlyphCount parameter is only valid if the call
    /// succeeds.  In the event that maxGlyphCount is not big enough
    /// E_NOT_SUFFICIENT_BUFFER, which is equivalent to HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER),
    /// will be returned.  The application should allocate a larger buffer and try again.
    /// </remarks>
    STDMETHOD(GetGlyphs)(
        _In_reads_(textLength) WCHAR const* textString,
        UINT32 textLength,
        _In_ IDWriteFontFace* fontFace,
        BOOL isSideways,
        BOOL isRightToLeft,
        _In_ DWRITE_SCRIPT_ANALYSIS const* scriptAnalysis,
        _In_opt_z_ WCHAR const* localeName,
        _In_opt_ IDWriteNumberSubstitution* numberSubstitution,
        _In_reads_opt_(featureRanges) DWRITE_TYPOGRAPHIC_FEATURES const** features,
        _In_reads_opt_(featureRanges) UINT32 const* featureRangeLengths,
        UINT32 featureRanges,
        UINT32 maxGlyphCount,
        _Out_writes_(textLength) UINT16* clusterMap,
        _Out_writes_(textLength) DWRITE_SHAPING_TEXT_PROPERTIES* textProps,
        _Out_writes_(maxGlyphCount) UINT16* glyphIndices,
        _Out_writes_(maxGlyphCount) DWRITE_SHAPING_GLYPH_PROPERTIES* glyphProps,
        _Out_ UINT32* actualGlyphCount
        ) PURE;

    /// <summary>
    /// Place glyphs output from the GetGlyphs method according to the font 
    /// and the writing system's rendering rules.
    /// </summary>
    /// <param name="textString">The original string the glyphs came from.</param>
    /// <param name="clusterMap">The mapping from character ranges to glyph 
    /// ranges. Returned by GetGlyphs.</param>
    /// <param name="textProps">Per-character properties. Returned by 
    /// GetGlyphs.</param>
    /// <param name="textLength">The length of textString.</param>
    /// <param name="glyphIndices">Glyph indices. See GetGlyphs</param>
    /// <param name="glyphProps">Per-glyph properties. See GetGlyphs</param>
    /// <param name="glyphCount">The number of glyphs.</param>
    /// <param name="fontFace">The font face the glyphs came from.</param>
    /// <param name="fontEmSize">Logical font size in DIP's.</param>
    /// <param name="isSideways">Set to true if the text is intended to be
    /// drawn vertically.</param>
    /// <param name="isRightToLeft">Set to TRUE for right-to-left text.</param>
    /// <param name="scriptAnalysis">Script analysis result from AnalyzeScript.</param>
    /// <param name="localeName">The locale to use when selecting glyphs.
    /// e.g. the same character may map to different glyphs for ja-jp vs zh-chs.
    /// If this is NULL then the default mapping based on the script is used.</param>
    /// <param name="features">An array of pointers to the sets of typographic 
    /// features to use in each feature range.</param>
    /// <param name="featureRangeLengths">The length of each feature range, in characters.  
    /// The sum of all lengths should be equal to textLength.</param>
    /// <param name="featureRanges">The number of feature ranges.</param>
    /// <param name="glyphAdvances">The advance width of each glyph.</param>
    /// <param name="glyphOffsets">The offset of the origin of each glyph.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetGlyphPlacements)(
        _In_reads_(textLength) WCHAR const* textString,
        _In_reads_(textLength) UINT16 const* clusterMap,
        _In_reads_(textLength) DWRITE_SHAPING_TEXT_PROPERTIES* textProps,
        UINT32 textLength,
        _In_reads_(glyphCount) UINT16 const* glyphIndices,
        _In_reads_(glyphCount) DWRITE_SHAPING_GLYPH_PROPERTIES const* glyphProps,
        UINT32 glyphCount,
        _In_ IDWriteFontFace* fontFace,
        FLOAT fontEmSize,
        BOOL isSideways,
        BOOL isRightToLeft,
        _In_ DWRITE_SCRIPT_ANALYSIS const* scriptAnalysis,
        _In_opt_z_ WCHAR const* localeName,
        _In_reads_opt_(featureRanges) DWRITE_TYPOGRAPHIC_FEATURES const** features,
        _In_reads_opt_(featureRanges) UINT32 const* featureRangeLengths,
        UINT32 featureRanges,
        _Out_writes_(glyphCount) FLOAT* glyphAdvances,
        _Out_writes_(glyphCount) DWRITE_GLYPH_OFFSET* glyphOffsets
        ) PURE;

    /// <summary>
    /// Place glyphs output from the GetGlyphs method according to the font 
    /// and the writing system's rendering rules.
    /// </summary>
    /// <param name="textString">The original string the glyphs came from.</param>
    /// <param name="clusterMap">The mapping from character ranges to glyph 
    /// ranges. Returned by GetGlyphs.</param>
    /// <param name="textProps">Per-character properties. Returned by 
    /// GetGlyphs.</param>
    /// <param name="textLength">The length of textString.</param>
    /// <param name="glyphIndices">Glyph indices. See GetGlyphs</param>
    /// <param name="glyphProps">Per-glyph properties. See GetGlyphs</param>
    /// <param name="glyphCount">The number of glyphs.</param>
    /// <param name="fontFace">The font face the glyphs came from.</param>
    /// <param name="fontEmSize">Logical font size in DIP's.</param>
    /// <param name="pixelsPerDip">Number of physical pixels per DIP. For example, if the DPI of the rendering surface is 96 this 
    /// value is 1.0f. If the DPI is 120, this value is 120.0f/96.</param>
    /// <param name="transform">Optional transform applied to the glyphs and their positions. This transform is applied after the
    /// scaling specified by the font size and pixelsPerDip.</param>
    /// <param name="useGdiNatural">
    /// When set to FALSE, the metrics are the same as the metrics of GDI aliased text.
    /// When set to TRUE, the metrics are the same as the metrics of text measured by GDI using a font
    /// created with CLEARTYPE_NATURAL_QUALITY.
    /// </param>
    /// <param name="isSideways">Set to true if the text is intended to be
    /// drawn vertically.</param>
    /// <param name="isRightToLeft">Set to TRUE for right-to-left text.</param>
    /// <param name="scriptAnalysis">Script analysis result from AnalyzeScript.</param>
    /// <param name="localeName">The locale to use when selecting glyphs.
    /// e.g. the same character may map to different glyphs for ja-jp vs zh-chs.
    /// If this is NULL then the default mapping based on the script is used.</param>
    /// <param name="features">An array of pointers to the sets of typographic 
    /// features to use in each feature range.</param>
    /// <param name="featureRangeLengths">The length of each feature range, in characters.  
    /// The sum of all lengths should be equal to textLength.</param>
    /// <param name="featureRanges">The number of feature ranges.</param>
    /// <param name="glyphAdvances">The advance width of each glyph.</param>
    /// <param name="glyphOffsets">The offset of the origin of each glyph.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetGdiCompatibleGlyphPlacements)(
        _In_reads_(textLength) WCHAR const* textString,
        _In_reads_(textLength) UINT16 const* clusterMap,
        _In_reads_(textLength) DWRITE_SHAPING_TEXT_PROPERTIES* textProps,
        UINT32 textLength,
        _In_reads_(glyphCount) UINT16 const* glyphIndices,
        _In_reads_(glyphCount) DWRITE_SHAPING_GLYPH_PROPERTIES const* glyphProps,
        UINT32 glyphCount,
        _In_ IDWriteFontFace * fontFace,
        FLOAT fontEmSize,
        FLOAT pixelsPerDip,
        _In_opt_ DWRITE_MATRIX const* transform,
        BOOL useGdiNatural,
        BOOL isSideways,
        BOOL isRightToLeft,
        _In_ DWRITE_SCRIPT_ANALYSIS const* scriptAnalysis,
        _In_opt_z_ WCHAR const* localeName,
        _In_reads_opt_(featureRanges) DWRITE_TYPOGRAPHIC_FEATURES const** features,
        _In_reads_opt_(featureRanges) UINT32 const* featureRangeLengths,
        UINT32 featureRanges,
        _Out_writes_(glyphCount) FLOAT* glyphAdvances,
        _Out_writes_(glyphCount) DWRITE_GLYPH_OFFSET* glyphOffsets
        ) PURE;
};

/// <summary>
/// The DWRITE_GLYPH_RUN structure contains the information needed by renderers
/// to draw glyph runs. All coordinates are in device independent pixels (DIPs).
/// </summary>
struct DWRITE_GLYPH_RUN
{
    /// <summary>
    /// The physical font face to draw with.
    /// </summary>
    _Notnull_ IDWriteFontFace* fontFace;

    /// <summary>
    /// Logical size of the font in DIPs, not points (equals 1/96 inch).
    /// </summary>
    FLOAT fontEmSize;

    /// <summary>
    /// The number of glyphs.
    /// </summary>
    UINT32 glyphCount;

    /// <summary>
    /// The indices to render.
    /// </summary>    
    _Field_size_(glyphCount) UINT16 const* glyphIndices;

    /// <summary>
    /// Glyph advance widths.
    /// </summary>
    _Field_size_opt_(glyphCount) FLOAT const* glyphAdvances;

    /// <summary>
    /// Glyph offsets.
    /// </summary>
    _Field_size_opt_(glyphCount) DWRITE_GLYPH_OFFSET const* glyphOffsets;

    /// <summary>
    /// If true, specifies that glyphs are rotated 90 degrees to the left and
    /// vertical metrics are used. Vertical writing is achieved by specifying
    /// isSideways = true and rotating the entire run 90 degrees to the right
    /// via a rotate transform.
    /// </summary>
    BOOL isSideways;

    /// <summary>
    /// The implicit resolved bidi level of the run. Odd levels indicate
    /// right-to-left languages like Hebrew and Arabic, while even levels
    /// indicate left-to-right languages like English and Japanese (when
    /// written horizontally). For right-to-left languages, the text origin
    /// is on the right, and text should be drawn to the left.
    /// </summary>
    UINT32 bidiLevel;
};

/// <summary>
/// The DWRITE_GLYPH_RUN_DESCRIPTION structure contains additional properties
/// related to those in DWRITE_GLYPH_RUN.
/// </summary>
struct DWRITE_GLYPH_RUN_DESCRIPTION
{
    /// <summary>
    /// The locale name associated with this run.
    /// </summary>
    _Field_z_ WCHAR const* localeName;

    /// <summary>
    /// The text associated with the glyphs.
    /// </summary>
    _Field_size_(stringLength) WCHAR const* string;

    /// <summary>
    /// The number of characters (UTF16 code-units).
    /// Note that this may be different than the number of glyphs.
    /// </summary>
    UINT32 stringLength;

    /// <summary>
    /// An array of indices to the glyph indices array, of the first glyphs of
    /// all the glyph clusters of the glyphs to render. 
    /// </summary>
    _Field_size_opt_(stringLength) UINT16 const* clusterMap;

    /// <summary>
    /// Corresponding text position in the original string
    /// this glyph run came from.
    /// </summary>
    UINT32 textPosition;
};

/// <summary>
/// The DWRITE_UNDERLINE structure contains information about the size and
/// placement of underlines. All coordinates are in device independent
/// pixels (DIPs).
/// </summary>
struct DWRITE_UNDERLINE
{
    /// <summary>
    /// Width of the underline, measured parallel to the baseline.
    /// </summary>
    FLOAT width;

    /// <summary>
    /// Thickness of the underline, measured perpendicular to the
    /// baseline.
    /// </summary>
    FLOAT thickness;

    /// <summary>
    /// Offset of the underline from the baseline.
    /// A positive offset represents a position below the baseline and
    /// a negative offset is above.
    /// </summary>
    FLOAT offset;

    /// <summary>
    /// Height of the tallest run where the underline applies.
    /// </summary>
    FLOAT runHeight;

    /// <summary>
    /// Reading direction of the text associated with the underline.  This 
    /// value is used to interpret whether the width value runs horizontally 
    /// or vertically.
    /// </summary>
    DWRITE_READING_DIRECTION readingDirection;

    /// <summary>
    /// Flow direction of the text associated with the underline.  This value
    /// is used to interpret whether the thickness value advances top to 
    /// bottom, left to right, or right to left.
    /// </summary>
    DWRITE_FLOW_DIRECTION flowDirection;

    /// <summary>
    /// Locale of the text the underline is being drawn under. Can be
    /// pertinent where the locale affects how the underline is drawn.
    /// For example, in vertical text, the underline belongs on the
    /// left for Chinese but on the right for Japanese.
    /// This choice is completely left up to higher levels.
    /// </summary>
    _Field_z_ WCHAR const* localeName;

    /// <summary>
    /// The measuring mode can be useful to the renderer to determine how
    /// underlines are rendered, e.g. rounding the thickness to a whole pixel
    /// in GDI-compatible modes.
    /// </summary>
    DWRITE_MEASURING_MODE measuringMode;
};

/// <summary>
/// The DWRITE_STRIKETHROUGH structure contains information about the size and
/// placement of strikethroughs. All coordinates are in device independent
/// pixels (DIPs).
/// </summary>
struct DWRITE_STRIKETHROUGH
{
    /// <summary>
    /// Width of the strikethrough, measured parallel to the baseline.
    /// </summary>
    FLOAT width;

    /// <summary>
    /// Thickness of the strikethrough, measured perpendicular to the
    /// baseline.
    /// </summary>
    FLOAT thickness;

    /// <summary>
    /// Offset of the strikethrough from the baseline.
    /// A positive offset represents a position below the baseline and
    /// a negative offset is above.
    /// </summary>
    FLOAT offset;

    /// <summary>
    /// Reading direction of the text associated with the strikethrough.  This
    /// value is used to interpret whether the width value runs horizontally 
    /// or vertically.
    /// </summary>
    DWRITE_READING_DIRECTION readingDirection;

    /// <summary>
    /// Flow direction of the text associated with the strikethrough.  This 
    /// value is used to interpret whether the thickness value advances top to
    /// bottom, left to right, or right to left.
    /// </summary>
    DWRITE_FLOW_DIRECTION flowDirection;

    /// <summary>
    /// Locale of the range. Can be pertinent where the locale affects the style.
    /// </summary>
    _Field_z_ WCHAR const* localeName;

    /// <summary>
    /// The measuring mode can be useful to the renderer to determine how
    /// underlines are rendered, e.g. rounding the thickness to a whole pixel
    /// in GDI-compatible modes.
    /// </summary>
    DWRITE_MEASURING_MODE measuringMode;
};

/// <summary>
/// The DWRITE_LINE_METRICS structure contains information about a formatted
/// line of text.
/// </summary>
struct DWRITE_LINE_METRICS
{
    /// <summary>
    /// The number of total text positions in the line.
    /// This includes any trailing whitespace and newline characters.
    /// </summary>
    UINT32 length;

    /// <summary>
    /// The number of whitespace positions at the end of the line.  Newline
    /// sequences are considered whitespace.
    /// </summary>
    UINT32 trailingWhitespaceLength;

    /// <summary>
    /// The number of characters in the newline sequence at the end of the line.
    /// If the count is zero, then the line was either wrapped or it is the
    /// end of the text.
    /// </summary>
    UINT32 newlineLength;

    /// <summary>
    /// Height of the line as measured from top to bottom.
    /// </summary>
    FLOAT height;

    /// <summary>
    /// Distance from the top of the line to its baseline.
    /// </summary>
    FLOAT baseline;

    /// <summary>
    /// The line is trimmed.
    /// </summary>
    BOOL isTrimmed;
};


/// <summary>
/// The DWRITE_CLUSTER_METRICS structure contains information about a glyph cluster.
/// </summary>
struct DWRITE_CLUSTER_METRICS
{
    /// <summary>
    /// The total advance width of all glyphs in the cluster.
    /// </summary>
    FLOAT width;

    /// <summary>
    /// The number of text positions in the cluster.
    /// </summary>
    UINT16 length;

    /// <summary>
    /// Indicate whether line can be broken right after the cluster.
    /// </summary>
    UINT16 canWrapLineAfter : 1;

    /// <summary>
    /// Indicate whether the cluster corresponds to whitespace character.
    /// </summary>
    UINT16 isWhitespace : 1;

    /// <summary>
    /// Indicate whether the cluster corresponds to a newline character.
    /// </summary>
    UINT16 isNewline : 1;

    /// <summary>
    /// Indicate whether the cluster corresponds to soft hyphen character.
    /// </summary>
    UINT16 isSoftHyphen : 1;

    /// <summary>
    /// Indicate whether the cluster is read from right to left.
    /// </summary>
    UINT16 isRightToLeft : 1;

    UINT16 padding : 11;
};


/// <summary>
/// Overall metrics associated with text after layout.
/// All coordinates are in device independent pixels (DIPs).
/// </summary>
struct DWRITE_TEXT_METRICS
{
    /// <summary>
    /// Left-most point of formatted text relative to layout box
    /// (excluding any glyph overhang).
    /// </summary>
    FLOAT left;

    /// <summary>
    /// Top-most point of formatted text relative to layout box
    /// (excluding any glyph overhang).
    /// </summary>
    FLOAT top;

    /// <summary>
    /// The width of the formatted text ignoring trailing whitespace
    /// at the end of each line.
    /// </summary>
    FLOAT width;

    /// <summary>
    /// The width of the formatted text taking into account the
    /// trailing whitespace at the end of each line.
    /// </summary>
    FLOAT widthIncludingTrailingWhitespace;

    /// <summary>
    /// The height of the formatted text. The height of an empty string
    /// is determined by the size of the default font's line height.
    /// </summary>
    FLOAT height;

    /// <summary>
    /// Initial width given to the layout. Depending on whether the text
    /// was wrapped or not, it can be either larger or smaller than the
    /// text content width.
    /// </summary>
    FLOAT layoutWidth;

    /// <summary>
    /// Initial height given to the layout. Depending on the length of the
    /// text, it may be larger or smaller than the text content height.
    /// </summary>
    FLOAT layoutHeight;

    /// <summary>
    /// The maximum reordering count of any line of text, used
    /// to calculate the most number of hit-testing boxes needed.
    /// If the layout has no bidirectional text or no text at all,
    /// the minimum level is 1.
    /// </summary>
    UINT32 maxBidiReorderingDepth;

    /// <summary>
    /// Total number of lines.
    /// </summary>
    UINT32 lineCount;
};


/// <summary>
/// Properties describing the geometric measurement of an
/// application-defined inline object.
/// </summary>
struct DWRITE_INLINE_OBJECT_METRICS
{
    /// <summary>
    /// Width of the inline object.
    /// </summary>
    FLOAT width;

    /// <summary>
    /// Height of the inline object as measured from top to bottom.
    /// </summary>
    FLOAT height;

    /// <summary>
    /// Distance from the top of the object to the baseline where it is lined up with the adjacent text.
    /// If the baseline is at the bottom, baseline simply equals height.
    /// </summary>
    FLOAT baseline;

    /// <summary>
    /// Flag indicating whether the object is to be placed upright or alongside the text baseline
    /// for vertical text.
    /// </summary>
    BOOL  supportsSideways;
};


/// <summary>
/// The DWRITE_OVERHANG_METRICS structure holds how much any visible pixels
/// (in DIPs) overshoot each side of the layout or inline objects.
/// </summary>
/// <remarks>
/// Positive overhangs indicate that the visible area extends outside the layout
/// box or inline object, while negative values mean there is whitespace inside.
/// The returned values are unaffected by rendering transforms or pixel snapping.
/// Additionally, they may not exactly match final target's pixel bounds after
/// applying grid fitting and hinting.
/// </remarks>
struct DWRITE_OVERHANG_METRICS
{
    /// <summary>
    /// The distance from the left-most visible DIP to its left alignment edge.
    /// </summary>
    FLOAT left;

    /// <summary>
    /// The distance from the top-most visible DIP to its top alignment edge.
    /// </summary>
    FLOAT top;

    /// <summary>
    /// The distance from the right-most visible DIP to its right alignment edge.
    /// </summary>
    FLOAT right;

    /// <summary>
    /// The distance from the bottom-most visible DIP to its bottom alignment edge.
    /// </summary>
    FLOAT bottom;
};


/// <summary>
/// Geometry enclosing of text positions.
/// </summary>
struct DWRITE_HIT_TEST_METRICS
{
    /// <summary>
    /// First text position within the geometry.
    /// </summary>
    UINT32 textPosition;

    /// <summary>
    /// Number of text positions within the geometry.
    /// </summary>
    UINT32 length;

    /// <summary>
    /// Left position of the top-left coordinate of the geometry.
    /// </summary>
    FLOAT left;

    /// <summary>
    /// Top position of the top-left coordinate of the geometry.
    /// </summary>
    FLOAT top;

    /// <summary>
    /// Geometry's width.
    /// </summary>
    FLOAT width;

    /// <summary>
    /// Geometry's height.
    /// </summary>
    FLOAT height;

    /// <summary>
    /// Bidi level of text positions enclosed within the geometry.
    /// </summary>
    UINT32 bidiLevel;

    /// <summary>
    /// Geometry encloses text?
    /// </summary>
    BOOL isText;

    /// <summary>
    /// Range is trimmed.
    /// </summary>
    BOOL isTrimmed;
};


interface IDWriteTextRenderer;


/// <summary>
/// The IDWriteInlineObject interface wraps an application defined inline graphic,
/// allowing DWrite to query metrics as if it was a glyph inline with the text.
/// </summary>
interface DWRITE_DECLARE_INTERFACE("8339FDE3-106F-47ab-8373-1C6295EB10B3") IDWriteInlineObject : public IUnknown
{
    /// <summary>
    /// The application implemented rendering callback (IDWriteTextRenderer::DrawInlineObject)
    /// can use this to draw the inline object without needing to cast or query the object
    /// type. The text layout does not call this method directly.
    /// </summary>
    /// <param name="clientDrawingContext">The context passed to IDWriteTextLayout::Draw.</param>
    /// <param name="renderer">The renderer passed to IDWriteTextLayout::Draw as the object's containing parent.</param>
    /// <param name="originX">X-coordinate at the top-left corner of the inline object.</param>
    /// <param name="originY">Y-coordinate at the top-left corner of the inline object.</param>
    /// <param name="isSideways">The object should be drawn on its side.</param>
    /// <param name="isRightToLeft">The object is in an right-to-left context and should be drawn flipped.</param>
    /// <param name="clientDrawingEffect">The drawing effect set in IDWriteTextLayout::SetDrawingEffect.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(Draw)(
        _In_opt_ void* clientDrawingContext,
        _In_ IDWriteTextRenderer* renderer,
        FLOAT originX,
        FLOAT originY,
        BOOL isSideways,
        BOOL isRightToLeft,
        _In_opt_ IUnknown* clientDrawingEffect
        ) PURE;

    /// <summary>
    /// TextLayout calls this callback function to get the measurement of the inline object.
    /// </summary>
    /// <param name="metrics">Returned metrics</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetMetrics)(
        _Out_ DWRITE_INLINE_OBJECT_METRICS* metrics
        ) PURE;

    /// <summary>
    /// TextLayout calls this callback function to get the visible extents (in DIPs) of the inline object.
    /// In the case of a simple bitmap, with no padding and no overhang, all the overhangs will
    /// simply be zeroes.
    /// </summary>
    /// <param name="overhangs">Overshoot of visible extents (in DIPs) outside the object.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    /// <remarks>
    /// The overhangs should be returned relative to the reported size of the object
    /// (DWRITE_INLINE_OBJECT_METRICS::width/height), and should not be baseline
    /// adjusted. If you have an image that is actually 100x100 DIPs, but you want it
    /// slightly inset (perhaps it has a glow) by 20 DIPs on each side, you would
    /// return a width/height of 60x60 and four overhangs of 20 DIPs.
    /// </remarks>
    STDMETHOD(GetOverhangMetrics)(
        _Out_ DWRITE_OVERHANG_METRICS* overhangs
        ) PURE;

    /// <summary>
    /// Layout uses this to determine the line breaking behavior of the inline object
    /// amidst the text.
    /// </summary>
    /// <param name="breakConditionBefore">Line-breaking condition between the object and the content immediately preceding it.</param>
    /// <param name="breakConditionAfter" >Line-breaking condition between the object and the content immediately following it.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetBreakConditions)(
        _Out_ DWRITE_BREAK_CONDITION* breakConditionBefore,
        _Out_ DWRITE_BREAK_CONDITION* breakConditionAfter
        ) PURE;
};

/// <summary>
/// The IDWritePixelSnapping interface defines the pixel snapping properties of a text renderer.
/// </summary>
interface DWRITE_DECLARE_INTERFACE("eaf3a2da-ecf4-4d24-b644-b34f6842024b") IDWritePixelSnapping : public IUnknown
{
    /// <summary>
    /// Determines whether pixel snapping is disabled. The recommended default is FALSE,
    /// unless doing animation that requires subpixel vertical placement.
    /// </summary>
    /// <param name="clientDrawingContext">The context passed to IDWriteTextLayout::Draw.</param>
    /// <param name="isDisabled">Receives TRUE if pixel snapping is disabled or FALSE if it not.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(IsPixelSnappingDisabled)(
        _In_opt_ void* clientDrawingContext,
        _Out_ BOOL* isDisabled
        ) PURE;

    /// <summary>
    /// Gets the current transform that maps abstract coordinates to DIPs,
    /// which may disable pixel snapping upon any rotation or shear.
    /// </summary>
    /// <param name="clientDrawingContext">The context passed to IDWriteTextLayout::Draw.</param>
    /// <param name="transform">Receives the transform.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetCurrentTransform)(
        _In_opt_ void* clientDrawingContext,
        _Out_ DWRITE_MATRIX* transform
        ) PURE;

    /// <summary>
    /// Gets the number of physical pixels per DIP. A DIP (device-independent pixel) is 1/96 inch,
    /// so the pixelsPerDip value is the number of logical pixels per inch divided by 96 (yielding
    /// a value of 1 for 96 DPI and 1.25 for 120).
    /// </summary>
    /// <param name="clientDrawingContext">The context passed to IDWriteTextLayout::Draw.</param>
    /// <param name="pixelsPerDip">Receives the number of physical pixels per DIP.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetPixelsPerDip)(
        _In_opt_ void* clientDrawingContext,
        _Out_ FLOAT* pixelsPerDip
        ) PURE;
};

/// <summary>
/// The IDWriteTextRenderer interface represents a set of application-defined
/// callbacks that perform rendering of text, inline objects, and decorations
/// such as underlines.
/// </summary>
interface DWRITE_DECLARE_INTERFACE("ef8a8135-5cc6-45fe-8825-c5a0724eb819") IDWriteTextRenderer : public IDWritePixelSnapping
{
    /// <summary>
    /// IDWriteTextLayout::Draw calls this function to instruct the client to
    /// render a run of glyphs.
    /// </summary>
    /// <param name="clientDrawingContext">The context passed to 
    /// IDWriteTextLayout::Draw.</param>
    /// <param name="baselineOriginX">X-coordinate of the baseline.</param>
    /// <param name="baselineOriginY">Y-coordinate of the baseline.</param>
    /// <param name="measuringMode">Specifies measuring mode for glyphs in the run.
    /// Renderer implementations may choose different rendering modes for given measuring modes,
    /// but best results are seen when the rendering mode matches the corresponding measuring mode:
    /// DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL for DWRITE_MEASURING_MODE_NATURAL
    /// DWRITE_RENDERING_MODE_CLEARTYPE_GDI_CLASSIC for DWRITE_MEASURING_MODE_GDI_CLASSIC
    /// DWRITE_RENDERING_MODE_CLEARTYPE_GDI_NATURAL for DWRITE_MEASURING_MODE_GDI_NATURAL
    /// </param>
    /// <param name="glyphRun">The glyph run to draw.</param>
    /// <param name="glyphRunDescription">Properties of the characters 
    /// associated with this run.</param>
    /// <param name="clientDrawingEffect">The drawing effect set in
    /// IDWriteTextLayout::SetDrawingEffect.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(DrawGlyphRun)(
        _In_opt_ void* clientDrawingContext,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        DWRITE_MEASURING_MODE measuringMode,
        _In_ DWRITE_GLYPH_RUN const* glyphRun,
        _In_ DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
        _In_opt_ IUnknown* clientDrawingEffect
        ) PURE;

    /// <summary>
    /// IDWriteTextLayout::Draw calls this function to instruct the client to draw
    /// an underline.
    /// </summary>
    /// <param name="clientDrawingContext">The context passed to 
    /// IDWriteTextLayout::Draw.</param>
    /// <param name="baselineOriginX">X-coordinate of the baseline.</param>
    /// <param name="baselineOriginY">Y-coordinate of the baseline.</param>
    /// <param name="underline">Underline logical information.</param>
    /// <param name="clientDrawingEffect">The drawing effect set in
    /// IDWriteTextLayout::SetDrawingEffect.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    /// <remarks>
    /// A single underline can be broken into multiple calls, depending on
    /// how the formatting changes attributes. If font sizes/styles change
    /// within an underline, the thickness and offset will be averaged
    /// weighted according to characters.
    /// To get the correct top coordinate of the underline rect, add underline::offset
    /// to the baseline's Y. Otherwise the underline will be immediately under the text.
    /// The x coordinate will always be passed as the left side, regardless
    /// of text directionality. This simplifies drawing and reduces the
    /// problem of round-off that could potentially cause gaps or a double
    /// stamped alpha blend. To avoid alpha overlap, round the end points
    /// to the nearest device pixel.
    /// </remarks>
    STDMETHOD(DrawUnderline)(
        _In_opt_ void* clientDrawingContext,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        _In_ DWRITE_UNDERLINE const* underline,
        _In_opt_ IUnknown* clientDrawingEffect
        ) PURE;

    /// <summary>
    /// IDWriteTextLayout::Draw calls this function to instruct the client to draw
    /// a strikethrough.
    /// </summary>
    /// <param name="clientDrawingContext">The context passed to 
    /// IDWriteTextLayout::Draw.</param>
    /// <param name="baselineOriginX">X-coordinate of the baseline.</param>
    /// <param name="baselineOriginY">Y-coordinate of the baseline.</param>
    /// <param name="strikethrough">Strikethrough logical information.</param>
    /// <param name="clientDrawingEffect">The drawing effect set in
    /// IDWriteTextLayout::SetDrawingEffect.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    /// <remarks>
    /// A single strikethrough can be broken into multiple calls, depending on
    /// how the formatting changes attributes. Strikethrough is not averaged
    /// across font sizes/styles changes.
    /// To get the correct top coordinate of the strikethrough rect,
    /// add strikethrough::offset to the baseline's Y.
    /// Like underlines, the x coordinate will always be passed as the left side,
    /// regardless of text directionality.
    /// </remarks>
    STDMETHOD(DrawStrikethrough)(
        _In_opt_ void* clientDrawingContext,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        _In_ DWRITE_STRIKETHROUGH const* strikethrough,
        _In_opt_ IUnknown* clientDrawingEffect
        ) PURE;

    /// <summary>
    /// IDWriteTextLayout::Draw calls this application callback when it needs to
    /// draw an inline object.
    /// </summary>
    /// <param name="clientDrawingContext">The context passed to IDWriteTextLayout::Draw.</param>
    /// <param name="originX">X-coordinate at the top-left corner of the inline object.</param>
    /// <param name="originY">Y-coordinate at the top-left corner of the inline object.</param>
    /// <param name="inlineObject">The object set using IDWriteTextLayout::SetInlineObject.</param>
    /// <param name="isSideways">The object should be drawn on its side.</param>
    /// <param name="isRightToLeft">The object is in an right-to-left context and should be drawn flipped.</param>
    /// <param name="clientDrawingEffect">The drawing effect set in
    /// IDWriteTextLayout::SetDrawingEffect.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    /// <remarks>
    /// The right-to-left flag is a hint for those cases where it would look
    /// strange for the image to be shown normally (like an arrow pointing to
    /// right to indicate a submenu).
    /// </remarks>
    STDMETHOD(DrawInlineObject)(
        _In_opt_ void* clientDrawingContext,
        FLOAT originX,
        FLOAT originY,
        _In_ IDWriteInlineObject* inlineObject,
        BOOL isSideways,
        BOOL isRightToLeft,
        _In_opt_ IUnknown* clientDrawingEffect
        ) PURE;
};

/// <summary>
/// The IDWriteTextLayout interface represents a block of text after it has
/// been fully analyzed and formatted.
///
/// All coordinates are in device independent pixels (DIPs).
/// </summary>
interface DWRITE_DECLARE_INTERFACE("53737037-6d14-410b-9bfe-0b182bb70961") IDWriteTextLayout : public IDWriteTextFormat
{
    /// <summary>
    /// Set layout maximum width
    /// </summary>
    /// <param name="maxWidth">Layout maximum width</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(SetMaxWidth)(
        FLOAT maxWidth
        ) PURE;

    /// <summary>
    /// Set layout maximum height
    /// </summary>
    /// <param name="maxHeight">Layout maximum height</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(SetMaxHeight)(
        FLOAT maxHeight
        ) PURE;

    /// <summary>
    /// Set the font collection.
    /// </summary>
    /// <param name="fontCollection">The font collection to set</param>
    /// <param name="textRange">Text range to which this change applies.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(SetFontCollection)(
        _In_ IDWriteFontCollection* fontCollection,
        DWRITE_TEXT_RANGE textRange
        ) PURE;

    /// <summary>
    /// Set null-terminated font family name.
    /// </summary>
    /// <param name="fontFamilyName">Font family name</param>
    /// <param name="textRange">Text range to which this change applies.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(SetFontFamilyName)(
        _In_z_ WCHAR const* fontFamilyName,
        DWRITE_TEXT_RANGE textRange
        ) PURE;

    /// <summary>
    /// Set font weight.
    /// </summary>
    /// <param name="fontWeight">Font weight</param>
    /// <param name="textRange">Text range to which this change applies.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(SetFontWeight)(
        DWRITE_FONT_WEIGHT fontWeight,
        DWRITE_TEXT_RANGE textRange
        ) PURE;

    /// <summary>
    /// Set font style.
    /// </summary>
    /// <param name="fontStyle">Font style</param>
    /// <param name="textRange">Text range to which this change applies.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(SetFontStyle)(
        DWRITE_FONT_STYLE fontStyle,
        DWRITE_TEXT_RANGE textRange
        ) PURE;

    /// <summary>
    /// Set font stretch.
    /// </summary>
    /// <param name="fontStretch">font stretch</param>
    /// <param name="textRange">Text range to which this change applies.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(SetFontStretch)(
        DWRITE_FONT_STRETCH fontStretch,
        DWRITE_TEXT_RANGE textRange
        ) PURE;

    /// <summary>
    /// Set font em height.
    /// </summary>
    /// <param name="fontSize">Font em height</param>
    /// <param name="textRange">Text range to which this change applies.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(SetFontSize)(
        FLOAT fontSize,
        DWRITE_TEXT_RANGE textRange
        ) PURE;

    /// <summary>
    /// Set underline.
    /// </summary>
    /// <param name="hasUnderline">The Boolean flag indicates whether underline takes place</param>
    /// <param name="textRange">Text range to which this change applies.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(SetUnderline)(
        BOOL hasUnderline,
        DWRITE_TEXT_RANGE textRange
        ) PURE;

    /// <summary>
    /// Set strikethrough.
    /// </summary>
    /// <param name="hasStrikethrough">The Boolean flag indicates whether strikethrough takes place</param>
    /// <param name="textRange">Text range to which this change applies.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(SetStrikethrough)(
        BOOL hasStrikethrough,
        DWRITE_TEXT_RANGE textRange
        ) PURE;

    /// <summary>
    /// Set application-defined drawing effect.
    /// </summary>
    /// <param name="drawingEffect">Pointer to an application-defined drawing effect.</param>
    /// <param name="textRange">Text range to which this change applies.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    /// <remarks>
    /// This drawing effect is associated with the specified range and will be passed back
    /// to the application via the callback when the range is drawn at drawing time.
    /// </remarks>
    STDMETHOD(SetDrawingEffect)(
        IUnknown* drawingEffect,
        DWRITE_TEXT_RANGE textRange
        ) PURE;

    /// <summary>
    /// Set inline object.
    /// </summary>
    /// <param name="inlineObject">Pointer to an application-implemented inline object.</param>
    /// <param name="textRange">Text range to which this change applies.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    /// <remarks>
    /// This inline object applies to the specified range and will be passed back
    /// to the application via the DrawInlineObject callback when the range is drawn.
    /// Any text in that range will be suppressed.
    /// </remarks>
    STDMETHOD(SetInlineObject)(
        _In_ IDWriteInlineObject* inlineObject,
        DWRITE_TEXT_RANGE textRange
        ) PURE;

    /// <summary>
    /// Set font typography features.
    /// </summary>
    /// <param name="typography">Pointer to font typography setting.</param>
    /// <param name="textRange">Text range to which this change applies.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(SetTypography)(
        _In_ IDWriteTypography* typography,
        DWRITE_TEXT_RANGE textRange
        ) PURE;

    /// <summary>
    /// Set locale name.
    /// </summary>
    /// <param name="localeName">Locale name</param>
    /// <param name="textRange">Text range to which this change applies.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(SetLocaleName)(
        _In_z_ WCHAR const* localeName,
        DWRITE_TEXT_RANGE textRange
        ) PURE;

    /// <summary>
    /// Get layout maximum width
    /// </summary>
    STDMETHOD_(FLOAT, GetMaxWidth)() PURE;

    /// <summary>
    /// Get layout maximum height
    /// </summary>
    STDMETHOD_(FLOAT, GetMaxHeight)() PURE;

    /// <summary>
    /// Get the font collection where the current position is at.
    /// </summary>
    /// <param name="currentPosition">The current text position.</param>
    /// <param name="fontCollection">The current font collection</param>
    /// <param name="textRange">Text range to which this change applies.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetFontCollection)(
        UINT32 currentPosition,
        _COM_Outptr_ IDWriteFontCollection** fontCollection,
        _Out_opt_ DWRITE_TEXT_RANGE* textRange = NULL
        ) PURE;

    /// <summary>
    /// Get the length of the font family name where the current position is at.
    /// </summary>
    /// <param name="currentPosition">The current text position.</param>
    /// <param name="nameLength">Size of the character array in character count not including the terminated NULL character.</param>
    /// <param name="textRange">The position range of the current format.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetFontFamilyNameLength)(
        UINT32 currentPosition,
        _Out_ UINT32* nameLength,
        _Out_opt_ DWRITE_TEXT_RANGE* textRange = NULL
        ) PURE;

    /// <summary>
    /// Copy the font family name where the current position is at.
    /// </summary>
    /// <param name="currentPosition">The current text position.</param>
    /// <param name="fontFamilyName">Character array that receives the current font family name</param>
    /// <param name="nameSize">Size of the character array in character count including the terminated NULL character.</param>
    /// <param name="textRange">The position range of the current format.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetFontFamilyName)(
        UINT32 currentPosition,
        _Out_writes_z_(nameSize) WCHAR* fontFamilyName,
        UINT32 nameSize,
        _Out_opt_ DWRITE_TEXT_RANGE* textRange = NULL
        ) PURE;

    /// <summary>
    /// Get the font weight where the current position is at.
    /// </summary>
    /// <param name="currentPosition">The current text position.</param>
    /// <param name="fontWeight">The current font weight</param>
    /// <param name="textRange">The position range of the current format.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetFontWeight)(
        UINT32 currentPosition,
        _Out_ DWRITE_FONT_WEIGHT* fontWeight,
        _Out_opt_ DWRITE_TEXT_RANGE* textRange = NULL
        ) PURE;

    /// <summary>
    /// Get the font style where the current position is at.
    /// </summary>
    /// <param name="currentPosition">The current text position.</param>
    /// <param name="fontStyle">The current font style</param>
    /// <param name="textRange">The position range of the current format.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetFontStyle)(
        UINT32 currentPosition,
        _Out_ DWRITE_FONT_STYLE* fontStyle,
        _Out_opt_ DWRITE_TEXT_RANGE* textRange = NULL
        ) PURE;

    /// <summary>
    /// Get the font stretch where the current position is at.
    /// </summary>
    /// <param name="currentPosition">The current text position.</param>
    /// <param name="fontStretch">The current font stretch</param>
    /// <param name="textRange">The position range of the current format.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetFontStretch)(
        UINT32 currentPosition,
        _Out_ DWRITE_FONT_STRETCH* fontStretch,
        _Out_opt_ DWRITE_TEXT_RANGE* textRange = NULL
        ) PURE;

    /// <summary>
    /// Get the font em height where the current position is at.
    /// </summary>
    /// <param name="currentPosition">The current text position.</param>
    /// <param name="fontSize">The current font em height</param>
    /// <param name="textRange">The position range of the current format.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetFontSize)(
        UINT32 currentPosition,
        _Out_ FLOAT* fontSize,
        _Out_opt_ DWRITE_TEXT_RANGE* textRange = NULL
        ) PURE;

    /// <summary>
    /// Get the underline presence where the current position is at.
    /// </summary>
    /// <param name="currentPosition">The current text position.</param>
    /// <param name="hasUnderline">The Boolean flag indicates whether text is underlined.</param>
    /// <param name="textRange">The position range of the current format.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetUnderline)(
        UINT32 currentPosition,
        _Out_ BOOL* hasUnderline,
        _Out_opt_ DWRITE_TEXT_RANGE* textRange = NULL
        ) PURE;

    /// <summary>
    /// Get the strikethrough presence where the current position is at.
    /// </summary>
    /// <param name="currentPosition">The current text position.</param>
    /// <param name="hasStrikethrough">The Boolean flag indicates whether text has strikethrough.</param>
    /// <param name="textRange">The position range of the current format.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetStrikethrough)(
        UINT32 currentPosition,
        _Out_ BOOL* hasStrikethrough,
        _Out_opt_ DWRITE_TEXT_RANGE* textRange = NULL
        ) PURE;

    /// <summary>
    /// Get the application-defined drawing effect where the current position is at.
    /// </summary>
    /// <param name="currentPosition">The current text position.</param>
    /// <param name="drawingEffect">The current application-defined drawing effect.</param>
    /// <param name="textRange">The position range of the current format.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetDrawingEffect)(
        UINT32 currentPosition,
        _COM_Outptr_ IUnknown** drawingEffect,
        _Out_opt_ DWRITE_TEXT_RANGE* textRange = NULL
        ) PURE;

    /// <summary>
    /// Get the inline object at the given position.
    /// </summary>
    /// <param name="currentPosition">The given text position.</param>
    /// <param name="inlineObject">The inline object.</param>
    /// <param name="textRange">The position range of the current format.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetInlineObject)(
        UINT32 currentPosition,
        _COM_Outptr_ IDWriteInlineObject** inlineObject,
        _Out_opt_ DWRITE_TEXT_RANGE* textRange = NULL
        ) PURE;

    /// <summary>
    /// Get the typography setting where the current position is at.
    /// </summary>
    /// <param name="currentPosition">The current text position.</param>
    /// <param name="typography">The current typography setting.</param>
    /// <param name="textRange">The position range of the current format.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetTypography)(
        UINT32 currentPosition,
        _COM_Outptr_ IDWriteTypography** typography,
        _Out_opt_ DWRITE_TEXT_RANGE* textRange = NULL
        ) PURE;

    /// <summary>
    /// Get the length of the locale name where the current position is at.
    /// </summary>
    /// <param name="currentPosition">The current text position.</param>
    /// <param name="nameLength">Size of the character array in character count not including the terminated NULL character.</param>
    /// <param name="textRange">The position range of the current format.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetLocaleNameLength)(
        UINT32 currentPosition,
        _Out_ UINT32* nameLength,
        _Out_opt_ DWRITE_TEXT_RANGE* textRange = NULL
        ) PURE;

    /// <summary>
    /// Get the locale name where the current position is at.
    /// </summary>
    /// <param name="currentPosition">The current text position.</param>
    /// <param name="localeName">Character array that receives the current locale name</param>
    /// <param name="nameSize">Size of the character array in character count including the terminated NULL character.</param>
    /// <param name="textRange">The position range of the current format.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetLocaleName)(
        UINT32 currentPosition,
        _Out_writes_z_(nameSize) WCHAR* localeName,
        UINT32 nameSize,
        _Out_opt_ DWRITE_TEXT_RANGE* textRange = NULL
        ) PURE;

    /// <summary>
    /// Initiate drawing of the text.
    /// </summary>
    /// <param name="clientDrawingContext">An application defined value
    /// included in rendering callbacks.</param>
    /// <param name="renderer">The set of application-defined callbacks that do
    /// the actual rendering.</param>
    /// <param name="originX">X-coordinate of the layout's left side.</param>
    /// <param name="originY">Y-coordinate of the layout's top side.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(Draw)(
        _In_opt_ void* clientDrawingContext,
        _In_ IDWriteTextRenderer* renderer,
        FLOAT originX,
        FLOAT originY
        ) PURE;

    /// <summary>
    /// GetLineMetrics returns properties of each line.
    /// </summary>
    /// <param name="lineMetrics">The array to fill with line information.</param>
    /// <param name="maxLineCount">The maximum size of the lineMetrics array.</param>
    /// <param name="actualLineCount">The actual size of the lineMetrics
    /// array that is needed.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    /// <remarks>
    /// If maxLineCount is not large enough E_NOT_SUFFICIENT_BUFFER, 
    /// which is equivalent to HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER),
    /// is returned and *actualLineCount is set to the number of lines
    /// needed.
    /// </remarks>
    STDMETHOD(GetLineMetrics)(
        _Out_writes_opt_(maxLineCount) DWRITE_LINE_METRICS* lineMetrics,
        UINT32 maxLineCount,
        _Out_ UINT32* actualLineCount
        ) PURE;

    /// <summary>
    /// GetMetrics retrieves overall metrics for the formatted string.
    /// </summary>
    /// <param name="textMetrics">The returned metrics.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    /// <remarks>
    /// Drawing effects like underline and strikethrough do not contribute
    /// to the text size, which is essentially the sum of advance widths and
    /// line heights. Additionally, visible swashes and other graphic
    /// adornments may extend outside the returned width and height.
    /// </remarks>
    STDMETHOD(GetMetrics)(
        _Out_ DWRITE_TEXT_METRICS* textMetrics
        ) PURE;

    /// <summary>
    /// GetOverhangMetrics returns the overhangs (in DIPs) of the layout and all
    /// objects contained in it, including text glyphs and inline objects.
    /// </summary>
    /// <param name="overhangs">Overshoots of visible extents (in DIPs) outside the layout.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    /// <remarks>
    /// Any underline and strikethrough do not contribute to the black box
    /// determination, since these are actually drawn by the renderer, which
    /// is allowed to draw them in any variety of styles.
    /// </remarks>
    STDMETHOD(GetOverhangMetrics)(
        _Out_ DWRITE_OVERHANG_METRICS* overhangs
        ) PURE;

    /// <summary>
    /// Retrieve logical properties and measurement of each cluster.
    /// </summary>
    /// <param name="clusterMetrics">The array to fill with cluster information.</param>
    /// <param name="maxClusterCount">The maximum size of the clusterMetrics array.</param>
    /// <param name="actualClusterCount">The actual size of the clusterMetrics array that is needed.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    /// <remarks>
    /// If maxClusterCount is not large enough E_NOT_SUFFICIENT_BUFFER, 
    /// which is equivalent to HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER), 
    /// is returned and *actualClusterCount is set to the number of clusters
    /// needed.
    /// </remarks>
    STDMETHOD(GetClusterMetrics)(
        _Out_writes_opt_(maxClusterCount) DWRITE_CLUSTER_METRICS* clusterMetrics,
        UINT32 maxClusterCount,
        _Out_ UINT32* actualClusterCount
        ) PURE;

    /// <summary>
    /// Determines the minimum possible width the layout can be set to without
    /// emergency breaking between the characters of whole words.
    /// </summary>
    /// <param name="minWidth">Minimum width.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(DetermineMinWidth)(
        _Out_ FLOAT* minWidth
        ) PURE;

    /// <summary>
    /// Given a coordinate (in DIPs) relative to the top-left of the layout box,
    /// this returns the corresponding hit-test metrics of the text string where
    /// the hit-test has occurred. This is useful for mapping mouse clicks to caret
    /// positions. When the given coordinate is outside the text string, the function
    /// sets the output value *isInside to false but returns the nearest character
    /// position.
    /// </summary>
    /// <param name="pointX">X coordinate to hit-test, relative to the top-left location of the layout box.</param>
    /// <param name="pointY">Y coordinate to hit-test, relative to the top-left location of the layout box.</param>
    /// <param name="isTrailingHit">Output flag indicating whether the hit-test location is at the leading or the trailing
    ///     side of the character. When the output *isInside value is set to false, this value is set according to the output
    ///     *position value to represent the edge closest to the hit-test location. </param>
    /// <param name="isInside">Output flag indicating whether the hit-test location is inside the text string.
    ///     When false, the position nearest the text's edge is returned.</param>
    /// <param name="hitTestMetrics">Output geometry fully enclosing the hit-test location. When the output *isInside value
    ///     is set to false, this structure represents the geometry enclosing the edge closest to the hit-test location.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(HitTestPoint)(
        FLOAT pointX,
        FLOAT pointY,
        _Out_ BOOL* isTrailingHit,
        _Out_ BOOL* isInside,
        _Out_ DWRITE_HIT_TEST_METRICS* hitTestMetrics
        ) PURE;

    /// <summary>
    /// Given a text position and whether the caret is on the leading or trailing
    /// edge of that position, this returns the corresponding coordinate (in DIPs)
    /// relative to the top-left of the layout box. This is most useful for drawing
    /// the caret's current position, but it could also be used to anchor an IME to the
    /// typed text or attach a floating menu near the point of interest. It may also be
    /// used to programmatically obtain the geometry of a particular text position
    /// for UI automation.
    /// </summary>
    /// <param name="textPosition">Text position to get the coordinate of.</param>
    /// <param name="isTrailingHit">Flag indicating whether the location is of the leading or the trailing side of the specified text position. </param>
    /// <param name="pointX">Output caret X, relative to the top-left of the layout box.</param>
    /// <param name="pointY">Output caret Y, relative to the top-left of the layout box.</param>
    /// <param name="hitTestMetrics">Output geometry fully enclosing the specified text position.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    /// <remarks>
    /// When drawing a caret at the returned X,Y, it should be centered on X
    /// and drawn from the Y coordinate down. The height will be the size of the
    /// hit-tested text (which can vary in size within a line).
    /// Reading direction also affects which side of the character the caret is drawn.
    /// However, the returned X coordinate will be correct for either case.
    /// You can get a text length back that is larger than a single character.
    /// This happens for complex scripts when multiple characters form a single cluster,
    /// when diacritics join their base character, or when you test a surrogate pair.
    /// </remarks>
    STDMETHOD(HitTestTextPosition)(
        UINT32 textPosition,
        BOOL isTrailingHit,
        _Out_ FLOAT* pointX,
        _Out_ FLOAT* pointY,
        _Out_ DWRITE_HIT_TEST_METRICS* hitTestMetrics
        ) PURE;

    /// <summary>
    /// The application calls this function to get a set of hit-test metrics
    /// corresponding to a range of text positions. The main usage for this
    /// is to draw highlighted selection of the text string.
    ///
    /// The function returns E_NOT_SUFFICIENT_BUFFER, which is equivalent to 
    /// HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER), when the buffer size of
    /// hitTestMetrics is too small to hold all the regions calculated by the
    /// function. In such situation, the function sets the output value
    /// *actualHitTestMetricsCount to the number of geometries calculated.
    /// The application is responsible to allocate a new buffer of greater
    /// size and call the function again.
    ///
    /// A good value to use as an initial value for maxHitTestMetricsCount may
    /// be calculated from the following equation:
    ///     maxHitTestMetricsCount = lineCount * maxBidiReorderingDepth
    ///
    /// where lineCount is obtained from the value of the output argument
    /// *actualLineCount from the function IDWriteTextLayout::GetLineMetrics,
    /// and the maxBidiReorderingDepth value from the DWRITE_TEXT_METRICS
    /// structure of the output argument *textMetrics from the function
    /// IDWriteFactory::CreateTextLayout.
    /// </summary>
    /// <param name="textPosition">First text position of the specified range.</param>
    /// <param name="textLength">Number of positions of the specified range.</param>
    /// <param name="originX">Offset of the X origin (left of the layout box) which is added to each of the hit-test metrics returned.</param>
    /// <param name="originY">Offset of the Y origin (top of the layout box) which is added to each of the hit-test metrics returned.</param>
    /// <param name="hitTestMetrics">Pointer to a buffer of the output geometry fully enclosing the specified position range.</param>
    /// <param name="maxHitTestMetricsCount">Maximum number of distinct metrics it could hold in its buffer memory.</param>
    /// <param name="actualHitTestMetricsCount">Actual number of metrics returned or needed.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    /// <remarks>
    /// There are no gaps in the returned metrics. While there could be visual gaps,
    /// depending on bidi ordering, each range is contiguous and reports all the text,
    /// including any hidden characters and trimmed text.
    /// The height of each returned range will be the same within each line, regardless
    /// of how the font sizes vary.
    /// </remarks>
    STDMETHOD(HitTestTextRange)(
        UINT32 textPosition,
        UINT32 textLength,
        FLOAT originX,
        FLOAT originY,
        _Out_writes_opt_(maxHitTestMetricsCount) DWRITE_HIT_TEST_METRICS* hitTestMetrics,
        UINT32 maxHitTestMetricsCount,
        _Out_ UINT32* actualHitTestMetricsCount
        ) PURE;

    using IDWriteTextFormat::GetFontCollection;
    using IDWriteTextFormat::GetFontFamilyNameLength;
    using IDWriteTextFormat::GetFontFamilyName;
    using IDWriteTextFormat::GetFontWeight;
    using IDWriteTextFormat::GetFontStyle;
    using IDWriteTextFormat::GetFontStretch;
    using IDWriteTextFormat::GetFontSize;
    using IDWriteTextFormat::GetLocaleNameLength;
    using IDWriteTextFormat::GetLocaleName;
};


/// <summary>
/// Encapsulates a 32-bit device independent bitmap and device context, which can be used for rendering glyphs.
/// </summary>
interface DWRITE_DECLARE_INTERFACE("5e5a32a3-8dff-4773-9ff6-0696eab77267") IDWriteBitmapRenderTarget : public IUnknown
{
    /// <summary>
    /// Draws a run of glyphs to the bitmap.
    /// </summary>
    /// <param name="baselineOriginX">Horizontal position of the baseline origin, in DIPs, relative to the upper-left corner of the DIB.</param>
    /// <param name="baselineOriginY">Vertical position of the baseline origin, in DIPs, relative to the upper-left corner of the DIB.</param>
    /// <param name="measuringMode">Specifies measuring mode for glyphs in the run.
    /// Renderer implementations may choose different rendering modes for different measuring modes, for example
    /// DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL for DWRITE_MEASURING_MODE_NATURAL,
    /// DWRITE_RENDERING_MODE_CLEARTYPE_GDI_CLASSIC for DWRITE_MEASURING_MODE_GDI_CLASSIC, and
    /// DWRITE_RENDERING_MODE_CLEARTYPE_GDI_NATURAL for DWRITE_MEASURING_MODE_GDI_NATURAL.
    /// </param>
    /// <param name="glyphRun">Structure containing the properties of the glyph run.</param>
    /// <param name="renderingParams">Object that controls rendering behavior.</param>
    /// <param name="textColor">Specifies the foreground color of the text.</param>
    /// <param name="blackBoxRect">Optional rectangle that receives the bounding box (in pixels not DIPs) of all the pixels affected by 
    /// drawing the glyph run. The black box rectangle may extend beyond the dimensions of the bitmap.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(DrawGlyphRun)(
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        DWRITE_MEASURING_MODE measuringMode,
        _In_ DWRITE_GLYPH_RUN const* glyphRun,
        _In_ IDWriteRenderingParams* renderingParams,
        COLORREF textColor,
        _Out_opt_ RECT* blackBoxRect = NULL
        ) PURE;

    /// <summary>
    /// Gets a handle to the memory device context.
    /// </summary>
    /// <returns>
    /// Returns the device context handle.
    /// </returns>
    /// <remarks>
    /// An application can use the device context to draw using GDI functions. An application can obtain the bitmap handle
    /// (HBITMAP) by calling GetCurrentObject. An application that wants information about the underlying bitmap, including
    /// a pointer to the pixel data, can call GetObject to fill in a DIBSECTION structure. The bitmap is always a 32-bit 
    /// top-down DIB.
    /// </remarks>
    STDMETHOD_(HDC, GetMemoryDC)() PURE;

    /// <summary>
    /// Gets the number of bitmap pixels per DIP. A DIP (device-independent pixel) is 1/96 inch so this value is the number
    /// if pixels per inch divided by 96.
    /// </summary>
    /// <returns>
    /// Returns the number of bitmap pixels per DIP.
    /// </returns>
    STDMETHOD_(FLOAT, GetPixelsPerDip)() PURE;

    /// <summary>
    /// Sets the number of bitmap pixels per DIP. A DIP (device-independent pixel) is 1/96 inch so this value is the number
    /// if pixels per inch divided by 96.
    /// </summary>
    /// <param name="pixelsPerDip">Specifies the number of pixels per DIP.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(SetPixelsPerDip)(
        FLOAT pixelsPerDip
        ) PURE;

    /// <summary>
    /// Gets the transform that maps abstract coordinate to DIPs. By default this is the identity 
    /// transform. Note that this is unrelated to the world transform of the underlying device
    /// context.
    /// </summary>
    /// <param name="transform">Receives the transform.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetCurrentTransform)(
        _Out_ DWRITE_MATRIX* transform
        ) PURE;

    /// <summary>
    /// Sets the transform that maps abstract coordinate to DIPs. This does not affect the world
    /// transform of the underlying device context.
    /// </summary>
    /// <param name="transform">Specifies the new transform. This parameter can be NULL, in which
    /// case the identity transform is implied.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(SetCurrentTransform)(
        _In_opt_ DWRITE_MATRIX const* transform
        ) PURE;

    /// <summary>
    /// Gets the dimensions of the bitmap.
    /// </summary>
    /// <param name="size">Receives the size of the bitmap in pixels.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetSize)(
        _Out_ SIZE* size
        ) PURE;

    /// <summary>
    /// Resizes the bitmap.
    /// </summary>
    /// <param name="width">New bitmap width, in pixels.</param>
    /// <param name="height">New bitmap height, in pixels.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(Resize)(
        UINT32 width,
        UINT32 height
        ) PURE;
};

/// <summary>
/// The GDI interop interface provides interoperability with GDI.
/// </summary>
interface DWRITE_DECLARE_INTERFACE("1edd9491-9853-4299-898f-6432983b6f3a") IDWriteGdiInterop : public IUnknown
{
    /// <summary>
    /// Creates a font object that matches the properties specified by the LOGFONT structure
    /// in the system font collection (GetSystemFontCollection).
    /// </summary>
    /// <param name="logFont">Structure containing a GDI-compatible font description.</param>
    /// <param name="font">Receives a newly created font object if successful, or NULL in case of error.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(CreateFontFromLOGFONT)(
        _In_ LOGFONTW const* logFont,
        _COM_Outptr_ IDWriteFont** font
        ) PURE;

    /// <summary>
    /// Initializes a LOGFONT structure based on the GDI-compatible properties of the specified font.
    /// </summary>
    /// <param name="font">Specifies a font.</param>
    /// <param name="logFont">Structure that receives a GDI-compatible font description.</param>
    /// <param name="isSystemFont">Contains TRUE if the specified font object is part of the system font collection
    /// or FALSE otherwise.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(ConvertFontToLOGFONT)(
        _In_ IDWriteFont* font,
        _Out_ LOGFONTW* logFont,
        _Out_ BOOL* isSystemFont
        ) PURE;

    /// <summary>
    /// Initializes a LOGFONT structure based on the GDI-compatible properties of the specified font.
    /// </summary>
    /// <param name="font">Specifies a font face.</param>
    /// <param name="logFont">Structure that receives a GDI-compatible font description.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(ConvertFontFaceToLOGFONT)(
        _In_ IDWriteFontFace* font,
        _Out_ LOGFONTW* logFont
        ) PURE;

    /// <summary>
    /// Creates a font face object that corresponds to the currently selected HFONT.
    /// </summary>
    /// <param name="hdc">Handle to a device context into which a font has been selected. It is assumed that the client
    /// has already performed font mapping and that the font selected into the DC is the actual font that would be used 
    /// for rendering glyphs.</param>
    /// <param name="fontFace">Contains the newly created font face object, or NULL in case of failure.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(CreateFontFaceFromHdc)(
        HDC hdc,
        _COM_Outptr_ IDWriteFontFace** fontFace
        ) PURE;

    /// <summary>
    /// Creates an object that encapsulates a bitmap and memory DC which can be used for rendering glyphs.
    /// </summary>
    /// <param name="hdc">Optional device context used to create a compatible memory DC.</param>
    /// <param name="width">Width of the bitmap.</param>
    /// <param name="height">Height of the bitmap.</param>
    /// <param name="renderTarget">Receives a pointer to the newly created render target.</param>
    STDMETHOD(CreateBitmapRenderTarget)(
        _In_opt_ HDC hdc,
        UINT32 width,
        UINT32 height,
        _COM_Outptr_ IDWriteBitmapRenderTarget** renderTarget
        ) PURE;
};

/// <summary>
/// The DWRITE_TEXTURE_TYPE enumeration identifies a type of alpha texture. An alpha texture is a bitmap of alpha values, each
/// representing the darkness (i.e., opacity) of a pixel or subpixel.
/// </summary>
enum DWRITE_TEXTURE_TYPE
{
    /// <summary>
    /// Specifies an alpha texture for aliased text rendering (i.e., bi-level, where each pixel is either fully opaque or fully transparent),
    /// with one byte per pixel.
    /// </summary>
    DWRITE_TEXTURE_ALIASED_1x1,

    /// <summary>
    /// Specifies an alpha texture for ClearType text rendering, with three bytes per pixel in the horizontal dimension and 
    /// one byte per pixel in the vertical dimension.
    /// </summary>
    DWRITE_TEXTURE_CLEARTYPE_3x1
};

/// <summary>
/// Maximum alpha value in a texture returned by IDWriteGlyphRunAnalysis::CreateAlphaTexture.
/// </summary>
#define DWRITE_ALPHA_MAX 255

/// <summary>
/// Interface that encapsulates information used to render a glyph run.
/// </summary>
interface DWRITE_DECLARE_INTERFACE("7d97dbf7-e085-42d4-81e3-6a883bded118") IDWriteGlyphRunAnalysis : public IUnknown
{
    /// <summary>
    /// Gets the bounding rectangle of the physical pixels affected by the glyph run.
    /// </summary>
    /// <param name="textureType">Specifies the type of texture requested. If a bi-level texture is requested, the
    /// bounding rectangle includes only bi-level glyphs. Otherwise, the bounding rectangle includes only anti-aliased
    /// glyphs.</param>
    /// <param name="textureBounds">Receives the bounding rectangle, or an empty rectangle if there are no glyphs
    /// if the specified type.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetAlphaTextureBounds)(
        DWRITE_TEXTURE_TYPE textureType,
        _Out_ RECT* textureBounds
        ) PURE;

    /// <summary>
    /// Creates an alpha texture of the specified type.
    /// </summary>
    /// <param name="textureType">Specifies the type of texture requested. If a bi-level texture is requested, the
    /// texture contains only bi-level glyphs. Otherwise, the texture contains only anti-aliased glyphs.</param>
    /// <param name="textureBounds">Specifies the bounding rectangle of the texture, which can be different than
    /// the bounding rectangle returned by GetAlphaTextureBounds.</param>
    /// <param name="alphaValues">Receives the array of alpha values.</param>
    /// <param name="bufferSize">Size of the alphaValues array. The minimum size depends on the dimensions of the
    /// rectangle and the type of texture requested.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(CreateAlphaTexture)(
        DWRITE_TEXTURE_TYPE textureType,
        _In_ RECT const* textureBounds,
        _Out_writes_bytes_(bufferSize) BYTE* alphaValues,
        UINT32 bufferSize
        ) PURE;

    /// <summary>
    /// Gets properties required for ClearType blending.
    /// </summary>
    /// <param name="renderingParams">Rendering parameters object. In most cases, the values returned in the output
    /// parameters are based on the properties of this object. The exception is if a GDI-compatible rendering mode
    /// is specified.</param>
    /// <param name="blendGamma">Receives the gamma value to use for gamma correction.</param>
    /// <param name="blendEnhancedContrast">Receives the enhanced contrast value.</param>
    /// <param name="blendClearTypeLevel">Receives the ClearType level.</param>
    STDMETHOD(GetAlphaBlendParams)(
        _In_ IDWriteRenderingParams* renderingParams,
        _Out_ FLOAT* blendGamma,
        _Out_ FLOAT* blendEnhancedContrast,
        _Out_ FLOAT* blendClearTypeLevel
        ) PURE;
};

/// <summary>
/// The root factory interface for all DWrite objects.
/// </summary>
interface DWRITE_DECLARE_INTERFACE("b859ee5a-d838-4b5b-a2e8-1adc7d93db48") IDWriteFactory : public IUnknown
{
    /// <summary>
    /// Gets a font collection representing the set of installed fonts.
    /// </summary>
    /// <param name="fontCollection">Receives a pointer to the system font collection object, or NULL in case of failure.</param>
    /// <param name="checkForUpdates">If this parameter is nonzero, the function performs an immediate check for changes to the set of
    /// installed fonts. If this parameter is FALSE, the function will still detect changes if the font cache service is running, but
    /// there may be some latency. For example, an application might specify TRUE if it has itself just installed a font and wants to 
    /// be sure the font collection contains that font.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetSystemFontCollection)(
        _COM_Outptr_ IDWriteFontCollection** fontCollection,
        BOOL checkForUpdates = FALSE
        ) PURE;

    /// <summary>
    /// Creates a font collection using a custom font collection loader.
    /// </summary>
    /// <param name="collectionLoader">Application-defined font collection loader, which must have been previously
    /// registered using RegisterFontCollectionLoader.</param>
    /// <param name="collectionKey">Key used by the loader to identify a collection of font files.</param>
    /// <param name="collectionKeySize">Size in bytes of the collection key.</param>
    /// <param name="fontCollection">Receives a pointer to the system font collection object, or NULL in case of failure.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(CreateCustomFontCollection)(
        _In_ IDWriteFontCollectionLoader* collectionLoader,
        _In_reads_bytes_(collectionKeySize) void const* collectionKey,
        UINT32 collectionKeySize,
        _COM_Outptr_ IDWriteFontCollection** fontCollection
        ) PURE;

    /// <summary>
    /// Registers a custom font collection loader with the factory object.
    /// </summary>
    /// <param name="fontCollectionLoader">Application-defined font collection loader.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(RegisterFontCollectionLoader)(
        _In_ IDWriteFontCollectionLoader* fontCollectionLoader
        ) PURE;

    /// <summary>
    /// Unregisters a custom font collection loader that was previously registered using RegisterFontCollectionLoader.
    /// </summary>
    /// <param name="fontCollectionLoader">Application-defined font collection loader.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(UnregisterFontCollectionLoader)(
        _In_ IDWriteFontCollectionLoader* fontCollectionLoader
        ) PURE;

    /// <summary>
    /// CreateFontFileReference creates a font file reference object from a local font file.
    /// </summary>
    /// <param name="filePath">Absolute file path. Subsequent operations on the constructed object may fail
    /// if the user provided filePath doesn't correspond to a valid file on the disk.</param>
    /// <param name="lastWriteTime">Last modified time of the input file path. If the parameter is omitted,
    /// the function will access the font file to obtain its last write time, so the clients are encouraged to specify this value
    /// to avoid extra disk access. Subsequent operations on the constructed object may fail
    /// if the user provided lastWriteTime doesn't match the file on the disk.</param>
    /// <param name="fontFile">Contains newly created font file reference object, or NULL in case of failure.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(CreateFontFileReference)(
        _In_z_ WCHAR const* filePath,
        _In_opt_ FILETIME const* lastWriteTime,
        _COM_Outptr_ IDWriteFontFile** fontFile
        ) PURE;

    /// <summary>
    /// CreateCustomFontFileReference creates a reference to an application specific font file resource.
    /// This function enables an application or a document to use a font without having to install it on the system.
    /// The fontFileReferenceKey has to be unique only in the scope of the fontFileLoader used in this call.
    /// </summary>
    /// <param name="fontFileReferenceKey">Font file reference key that uniquely identifies the font file resource
    /// during the lifetime of fontFileLoader.</param>
    /// <param name="fontFileReferenceKeySize">Size of font file reference key in bytes.</param>
    /// <param name="fontFileLoader">Font file loader that will be used by the font system to load data from the file identified by
    /// fontFileReferenceKey.</param>
    /// <param name="fontFile">Contains the newly created font file object, or NULL in case of failure.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    /// <remarks>
    /// This function is provided for cases when an application or a document needs to use a font
    /// without having to install it on the system. fontFileReferenceKey has to be unique only in the scope
    /// of the fontFileLoader used in this call.
    /// </remarks>
    STDMETHOD(CreateCustomFontFileReference)(
        _In_reads_bytes_(fontFileReferenceKeySize) void const* fontFileReferenceKey,
        UINT32 fontFileReferenceKeySize,
        _In_ IDWriteFontFileLoader* fontFileLoader,
        _COM_Outptr_ IDWriteFontFile** fontFile
        ) PURE;

    /// <summary>
    /// Creates a font face object.
    /// </summary>
    /// <param name="fontFaceType">The file format of the font face.</param>
    /// <param name="numberOfFiles">The number of font files required to represent the font face.</param>
    /// <param name="fontFiles">Font files representing the font face. Since IDWriteFontFace maintains its own references
    /// to the input font file objects, it's OK to release them after this call.</param>
    /// <param name="faceIndex">The zero based index of a font face in cases when the font files contain a collection of font faces.
    /// If the font files contain a single face, this value should be zero.</param>
    /// <param name="fontFaceSimulationFlags">Font face simulation flags for algorithmic emboldening and italicization.</param>
    /// <param name="fontFace">Contains the newly created font face object, or NULL in case of failure.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(CreateFontFace)(
        DWRITE_FONT_FACE_TYPE fontFaceType,
        UINT32 numberOfFiles,
        _In_reads_(numberOfFiles) IDWriteFontFile* const* fontFiles,
        UINT32 faceIndex,
        DWRITE_FONT_SIMULATIONS fontFaceSimulationFlags,
        _COM_Outptr_ IDWriteFontFace** fontFace
        ) PURE;

    /// <summary>
    /// Creates a rendering parameters object with default settings for the primary monitor.
    /// </summary>
    /// <param name="renderingParams">Holds the newly created rendering parameters object, or NULL in case of failure.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(CreateRenderingParams)(
        _COM_Outptr_ IDWriteRenderingParams** renderingParams
        ) PURE;

    /// <summary>
    /// Creates a rendering parameters object with default settings for the specified monitor.
    /// </summary>
    /// <param name="monitor">The monitor to read the default values from.</param>
    /// <param name="renderingParams">Holds the newly created rendering parameters object, or NULL in case of failure.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(CreateMonitorRenderingParams)(
        HMONITOR monitor,
        _COM_Outptr_ IDWriteRenderingParams** renderingParams
        ) PURE;

    /// <summary>
    /// Creates a rendering parameters object with the specified properties.
    /// </summary>
    /// <param name="gamma">The gamma value used for gamma correction, which must be greater than zero and cannot exceed 256.</param>
    /// <param name="enhancedContrast">The amount of contrast enhancement, zero or greater.</param>
    /// <param name="clearTypeLevel">The degree of ClearType level, from 0.0f (no ClearType) to 1.0f (full ClearType).</param>
    /// <param name="pixelGeometry">The geometry of a device pixel.</param>
    /// <param name="renderingMode">Method of rendering glyphs. In most cases, this should be DWRITE_RENDERING_MODE_DEFAULT to automatically use an appropriate mode.</param>
    /// <param name="renderingParams">Holds the newly created rendering parameters object, or NULL in case of failure.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(CreateCustomRenderingParams)(
        FLOAT gamma,
        FLOAT enhancedContrast,
        FLOAT clearTypeLevel,
        DWRITE_PIXEL_GEOMETRY pixelGeometry,
        DWRITE_RENDERING_MODE renderingMode,
        _COM_Outptr_ IDWriteRenderingParams** renderingParams
        ) PURE;

    /// <summary>
    /// Registers a font file loader with DirectWrite.
    /// </summary>
    /// <param name="fontFileLoader">Pointer to the implementation of the IDWriteFontFileLoader for a particular file resource type.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    /// <remarks>
    /// This function registers a font file loader with DirectWrite.
    /// Font file loader interface handles loading font file resources of a particular type from a key.
    /// The font file loader interface is recommended to be implemented by a singleton object.
    /// A given instance can only be registered once.
    /// Succeeding attempts will return an error that it has already been registered.
    /// IMPORTANT: font file loader implementations must not register themselves with DirectWrite
    /// inside their constructors and must not unregister themselves in their destructors, because
    /// registration and unregistration operations increment and decrement the object reference count respectively.
    /// Instead, registration and unregistration of font file loaders with DirectWrite should be performed
    /// outside of the font file loader implementation as a separate step.
    /// </remarks>
    STDMETHOD(RegisterFontFileLoader)(
        _In_ IDWriteFontFileLoader* fontFileLoader
        ) PURE;

    /// <summary>
    /// Unregisters a font file loader that was previously registered with the DirectWrite font system using RegisterFontFileLoader.
    /// </summary>
    /// <param name="fontFileLoader">Pointer to the file loader that was previously registered with the DirectWrite font system using RegisterFontFileLoader.</param>
    /// <returns>
    /// This function will succeed if the user loader is requested to be removed.
    /// It will fail if the pointer to the file loader identifies a standard DirectWrite loader,
    /// or a loader that is never registered or has already been unregistered.
    /// </returns>
    /// <remarks>
    /// This function unregisters font file loader callbacks with the DirectWrite font system.
    /// The font file loader interface is recommended to be implemented by a singleton object.
    /// IMPORTANT: font file loader implementations must not register themselves with DirectWrite
    /// inside their constructors and must not unregister themselves in their destructors, because
    /// registration and unregistration operations increment and decrement the object reference count respectively.
    /// Instead, registration and unregistration of font file loaders with DirectWrite should be performed
    /// outside of the font file loader implementation as a separate step.
    /// </remarks>
    STDMETHOD(UnregisterFontFileLoader)(
        _In_ IDWriteFontFileLoader* fontFileLoader
        ) PURE;

    /// <summary>
    /// Create a text format object used for text layout.
    /// </summary>
    /// <param name="fontFamilyName">Name of the font family</param>
    /// <param name="fontCollection">Font collection. NULL indicates the system font collection.</param>
    /// <param name="fontWeight">Font weight</param>
    /// <param name="fontStyle">Font style</param>
    /// <param name="fontStretch">Font stretch</param>
    /// <param name="fontSize">Logical size of the font in DIP units. A DIP ("device-independent pixel") equals 1/96 inch.</param>
    /// <param name="localeName">Locale name</param>
    /// <param name="textFormat">Contains newly created text format object, or NULL in case of failure.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(CreateTextFormat)(
        _In_z_ WCHAR const* fontFamilyName,
        _In_opt_ IDWriteFontCollection* fontCollection,
        DWRITE_FONT_WEIGHT fontWeight,
        DWRITE_FONT_STYLE fontStyle,
        DWRITE_FONT_STRETCH fontStretch,
        FLOAT fontSize,
        _In_z_ WCHAR const* localeName,
        _COM_Outptr_ IDWriteTextFormat** textFormat
        ) PURE;

    /// <summary>
    /// Create a typography object used in conjunction with text format for text layout.
    /// </summary>
    /// <param name="typography">Contains newly created typography object, or NULL in case of failure.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(CreateTypography)(
        _COM_Outptr_ IDWriteTypography** typography
        ) PURE;

    /// <summary>
    /// Create an object used for interoperability with GDI.
    /// </summary>
    /// <param name="gdiInterop">Receives the GDI interop object if successful, or NULL in case of failure.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetGdiInterop)(
        _COM_Outptr_ IDWriteGdiInterop** gdiInterop
        ) PURE;

    /// <summary>
    /// CreateTextLayout takes a string, format, and associated constraints
    /// and produces an object representing the fully analyzed
    /// and formatted result.
    /// </summary>
    /// <param name="string">The string to layout.</param>
    /// <param name="stringLength">The length of the string.</param>
    /// <param name="textFormat">The format to apply to the string.</param>
    /// <param name="maxWidth">Width of the layout box.</param>
    /// <param name="maxHeight">Height of the layout box.</param>
    /// <param name="textLayout">The resultant object.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(CreateTextLayout)(
        _In_reads_(stringLength) WCHAR const* string,
        UINT32 stringLength,
        _In_ IDWriteTextFormat* textFormat,
        FLOAT maxWidth,
        FLOAT maxHeight,
        _COM_Outptr_ IDWriteTextLayout** textLayout
        ) PURE;

    /// <summary>
    /// CreateGdiCompatibleTextLayout takes a string, format, and associated constraints
    /// and produces and object representing the result formatted for a particular display resolution
    /// and measuring mode. The resulting text layout should only be used for the intended resolution,
    /// and for cases where text scalability is desired, CreateTextLayout should be used instead.
    /// </summary>
    /// <param name="string">The string to layout.</param>
    /// <param name="stringLength">The length of the string.</param>
    /// <param name="textFormat">The format to apply to the string.</param>
    /// <param name="layoutWidth">Width of the layout box.</param>
    /// <param name="layoutHeight">Height of the layout box.</param>
    /// <param name="pixelsPerDip">Number of physical pixels per DIP. For example, if rendering onto a 96 DPI device then pixelsPerDip
    /// is 1. If rendering onto a 120 DPI device then pixelsPerDip is 120/96.</param>
    /// <param name="transform">Optional transform applied to the glyphs and their positions. This transform is applied after the
    /// scaling specified the font size and pixelsPerDip.</param>
    /// <param name="useGdiNatural">
    /// When set to FALSE, instructs the text layout to use the same metrics as GDI aliased text.
    /// When set to TRUE, instructs the text layout to use the same metrics as text measured by GDI using a font
    /// created with CLEARTYPE_NATURAL_QUALITY.
    /// </param>
    /// <param name="textLayout">The resultant object.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(CreateGdiCompatibleTextLayout)(
        _In_reads_(stringLength) WCHAR const* string,
        UINT32 stringLength,
        _In_ IDWriteTextFormat* textFormat,
        FLOAT layoutWidth,
        FLOAT layoutHeight,
        FLOAT pixelsPerDip,
        _In_opt_ DWRITE_MATRIX const* transform,
        BOOL useGdiNatural,
        _COM_Outptr_ IDWriteTextLayout** textLayout
        ) PURE;

    /// <summary>
    /// The application may call this function to create an inline object for trimming, using an ellipsis as the omission sign.
    /// The ellipsis will be created using the current settings of the format, including base font, style, and any effects.
    /// Alternate omission signs can be created by the application by implementing IDWriteInlineObject.
    /// </summary>
    /// <param name="textFormat">Text format used as a template for the omission sign.</param>
    /// <param name="trimmingSign">Created omission sign.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(CreateEllipsisTrimmingSign)(
        _In_ IDWriteTextFormat* textFormat,
        _COM_Outptr_ IDWriteInlineObject** trimmingSign
        ) PURE;

    /// <summary>
    /// Return an interface to perform text analysis with.
    /// </summary>
    /// <param name="textAnalyzer">The resultant object.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(CreateTextAnalyzer)(
        _COM_Outptr_ IDWriteTextAnalyzer** textAnalyzer
        ) PURE;

    /// <summary>
    /// Creates a number substitution object using a locale name,
    /// substitution method, and whether to ignore user overrides (uses NLS
    /// defaults for the given culture instead).
    /// </summary>
    /// <param name="substitutionMethod">Method of number substitution to use.</param>
    /// <param name="localeName">Which locale to obtain the digits from.</param>
    /// <param name="ignoreUserOverride">Ignore the user's settings and use the locale defaults</param>
    /// <param name="numberSubstitution">Receives a pointer to the newly created object.</param>
    STDMETHOD(CreateNumberSubstitution)(
        _In_ DWRITE_NUMBER_SUBSTITUTION_METHOD substitutionMethod,
        _In_z_ WCHAR const* localeName,
        _In_ BOOL ignoreUserOverride,
        _COM_Outptr_ IDWriteNumberSubstitution** numberSubstitution
        ) PURE;

    /// <summary>
    /// Creates a glyph run analysis object, which encapsulates information
    /// used to render a glyph run.
    /// </summary>
    /// <param name="glyphRun">Structure specifying the properties of the glyph run.</param>
    /// <param name="pixelsPerDip">Number of physical pixels per DIP. For example, if rendering onto a 96 DPI bitmap then pixelsPerDip
    /// is 1. If rendering onto a 120 DPI bitmap then pixelsPerDip is 120/96.</param>
    /// <param name="transform">Optional transform applied to the glyphs and their positions. This transform is applied after the
    /// scaling specified by the emSize and pixelsPerDip.</param>
    /// <param name="renderingMode">Specifies the rendering mode, which must be one of the raster rendering modes (i.e., not default
    /// and not outline).</param>
    /// <param name="measuringMode">Specifies the method to measure glyphs.</param>
    /// <param name="baselineOriginX">Horizontal position of the baseline origin, in DIPs.</param>
    /// <param name="baselineOriginY">Vertical position of the baseline origin, in DIPs.</param>
    /// <param name="glyphRunAnalysis">Receives a pointer to the newly created object.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(CreateGlyphRunAnalysis)(
        _In_ DWRITE_GLYPH_RUN const* glyphRun,
        FLOAT pixelsPerDip,
        _In_opt_ DWRITE_MATRIX const* transform,
        DWRITE_RENDERING_MODE renderingMode,
        DWRITE_MEASURING_MODE measuringMode,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        _COM_Outptr_ IDWriteGlyphRunAnalysis** glyphRunAnalysis
        ) PURE;

}; // interface IDWriteFactory


/// <summary>
/// Creates a DirectWrite factory object that is used for subsequent creation of individual DirectWrite objects.
/// </summary>
/// <param name="factoryType">Identifies whether the factory object will be shared or isolated.</param>
/// <param name="iid">Identifies the DirectWrite factory interface, such as __uuidof(IDWriteFactory).</param>
/// <param name="factory">Receives the DirectWrite factory object.</param>
/// <returns>
/// Standard HRESULT error code.
/// </returns>
/// <remarks>
/// Obtains DirectWrite factory object that is used for subsequent creation of individual DirectWrite classes.
/// DirectWrite factory contains internal state such as font loader registration and cached font data.
/// In most cases it is recommended to use the shared factory object, because it allows multiple components
/// that use DirectWrite to share internal DirectWrite state and reduce memory usage.
/// However, there are cases when it is desirable to reduce the impact of a component,
/// such as a plug-in from an untrusted source, on the rest of the process by sandboxing and isolating it
/// from the rest of the process components. In such cases, it is recommended to use an isolated factory for the sandboxed
/// component.
/// </remarks>
EXTERN_C HRESULT DWRITE_EXPORT DWriteCreateFactory(
    _In_ DWRITE_FACTORY_TYPE factoryType,
    _In_ REFIID iid,
    _COM_Outptr_ IUnknown **factory
    );

// Macros used to define DirectWrite error codes.
#define FACILITY_DWRITE 0x898
#define DWRITE_ERR_BASE 0x5000
#define MAKE_DWRITE_HR(severity, code) MAKE_HRESULT(severity, FACILITY_DWRITE, (DWRITE_ERR_BASE + code))
#define MAKE_DWRITE_HR_ERR(code) MAKE_DWRITE_HR(SEVERITY_ERROR, code)

// DWrite errors have moved to winerror.h


#endif /* DWRITE_H_INCLUDED */
