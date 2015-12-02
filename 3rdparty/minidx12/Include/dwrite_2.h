//+--------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Abstract:
//     DirectX Typography Services public API definitions.
//
//----------------------------------------------------------------------------

#ifndef DWRITE_2_H_INCLUDED
#define DWRITE_2_H_INCLUDED

#pragma once

#include <DWrite_1.h>


interface IDWriteFontFallback;


/// <summary>
/// How to align glyphs to the margin.
/// </summary>
enum DWRITE_OPTICAL_ALIGNMENT
{
    /// <summary>
    /// Align to the default metrics of the glyph.
    /// </summary>
    DWRITE_OPTICAL_ALIGNMENT_NONE,

    /// <summary>
    /// Align glyphs to the margins. Without this, some small whitespace
    /// may be present between the text and the margin from the glyph's side
    /// bearing values. Note that glyphs may still overhang outside the
    /// margin, such as flourishes or italic slants.
    /// </summary>
    DWRITE_OPTICAL_ALIGNMENT_NO_SIDE_BEARINGS,
};


/// <summary>
/// Whether to enable grid-fitting of glyph outlines (a.k.a. hinting).
/// </summary>
enum DWRITE_GRID_FIT_MODE
{
    /// <summary>
    /// Choose grid fitting base on the font's gasp table information.
    /// </summary>
    DWRITE_GRID_FIT_MODE_DEFAULT,

    /// <summary>
    /// Always disable grid fitting, using the ideal glyph outlines.
    /// </summary>
    DWRITE_GRID_FIT_MODE_DISABLED,

    /// <summary>
    /// Enable grid fitting, adjusting glyph outlines for device pixel display.
    /// </summary>
    DWRITE_GRID_FIT_MODE_ENABLED
};


/// <summary>
/// Overall metrics associated with text after layout.
/// All coordinates are in device independent pixels (DIPs).
/// </summary>
struct DWRITE_TEXT_METRICS1 : DWRITE_TEXT_METRICS
{
    /// <summary>
    /// The height of the formatted text taking into account the
    /// trailing whitespace at the end of each line, which will
    /// matter for vertical reading directions.
    /// </summary>
    FLOAT heightIncludingTrailingWhitespace;
};


/// <summary>
/// The text renderer interface represents a set of application-defined
/// callbacks that perform rendering of text, inline objects, and decorations
/// such as underlines.
/// </summary>
interface DWRITE_DECLARE_INTERFACE("D3E0E934-22A0-427E-AAE4-7D9574B59DB1") IDWriteTextRenderer1 : public IDWriteTextRenderer
{
    /// <summary>
    /// IDWriteTextLayout::Draw calls this function to instruct the client to
    /// render a run of glyphs.
    /// </summary>
    /// <param name="clientDrawingContext">The context passed to 
    ///     IDWriteTextLayout::Draw.</param>
    /// <param name="baselineOriginX">X-coordinate of the baseline.</param>
    /// <param name="baselineOriginY">Y-coordinate of the baseline.</param>
    /// <param name="orientationAngle">Orientation of the glyph run.</param>
    /// <param name="measuringMode">Specifies measuring method for glyphs in
    ///     the run. Renderer implementations may choose different rendering
    ///     modes for given measuring methods, but best results are seen when
    ///     the rendering mode matches the corresponding measuring mode:
    ///     DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL for DWRITE_MEASURING_MODE_NATURAL
    ///     DWRITE_RENDERING_MODE_CLEARTYPE_GDI_CLASSIC for DWRITE_MEASURING_MODE_GDI_CLASSIC
    ///     DWRITE_RENDERING_MODE_CLEARTYPE_GDI_NATURAL for DWRITE_MEASURING_MODE_GDI_NATURAL
    /// </param>
    /// <param name="glyphRun">The glyph run to draw.</param>
    /// <param name="glyphRunDescription">Properties of the characters 
    ///     associated with this run.</param>
    /// <param name="clientDrawingEffect">The drawing effect set in
    ///     IDWriteTextLayout::SetDrawingEffect.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    /// <remarks>
    /// If a non-identity orientation is passed, the glyph run should be
    /// rotated around the given baseline x and y coordinates. The function
    /// IDWriteAnalyzer2::GetGlyphOrientationTransform will return the
    /// necessary transform for you, which can be combined with any existing
    /// world transform on the drawing context.
    /// </remarks>
    STDMETHOD(DrawGlyphRun)(
        _In_opt_ void* clientDrawingContext,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        DWRITE_GLYPH_ORIENTATION_ANGLE orientationAngle,
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
    /// <param name="orientationAngle">Orientation of the underline.</param>
    /// <param name="underline">Underline logical information.</param>
    /// <param name="clientDrawingEffect">The drawing effect set in
    ///     IDWriteTextLayout::SetDrawingEffect.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    /// <remarks>
    /// A single underline can be broken into multiple calls, depending on
    /// how the formatting changes attributes. If font sizes/styles change
    /// within an underline, the thickness and offset will be averaged
    /// weighted according to characters.
    ///
    /// To get the correct top coordinate of the underline rect, add
    /// underline::offset to the baseline's Y. Otherwise the underline will
    /// be immediately under the text. The x coordinate will always be passed
    /// as the left side, regardless of text directionality. This simplifies
    /// drawing and reduces the problem of round-off that could potentially
    /// cause gaps or a double stamped alpha blend. To avoid alpha overlap,
    /// round the end points to the nearest device pixel.
    /// </remarks>
    STDMETHOD(DrawUnderline)(
        _In_opt_ void* clientDrawingContext,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        DWRITE_GLYPH_ORIENTATION_ANGLE orientationAngle,
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
    /// <param name="orientationAngle">Orientation of the strikethrough.</param>
    /// <param name="strikethrough">Strikethrough logical information.</param>
    /// <param name="clientDrawingEffect">The drawing effect set in
    ///     IDWriteTextLayout::SetDrawingEffect.</param>
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
        DWRITE_GLYPH_ORIENTATION_ANGLE orientationAngle,
        _In_ DWRITE_STRIKETHROUGH const* strikethrough,
        _In_opt_ IUnknown* clientDrawingEffect
        ) PURE;

    /// <summary>
    /// IDWriteTextLayout::Draw calls this application callback when it needs to
    /// draw an inline object.
    /// </summary>
    /// <param name="clientDrawingContext">The context passed to
    ///     IDWriteTextLayout::Draw.</param>
    /// <param name="originX">X-coordinate at the top-left corner of the
    ///     inline object.</param>
    /// <param name="originY">Y-coordinate at the top-left corner of the
    ///     inline object.</param>
    /// <param name="orientationAngle">Orientation of the inline object.</param>
    /// <param name="inlineObject">The object set using IDWriteTextLayout::SetInlineObject.</param>
    /// <param name="isSideways">The object should be drawn on its side.</param>
    /// <param name="isRightToLeft">The object is in an right-to-left context
    ///     and should be drawn flipped.</param>
    /// <param name="clientDrawingEffect">The drawing effect set in
    ///     IDWriteTextLayout::SetDrawingEffect.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    /// <remarks>
    /// The right-to-left flag is a hint to draw the appropriate visual for
    /// that reading direction. For example, it would look strange to draw an
    /// arrow pointing to the right to indicate a submenu. The sideways flag
    /// similarly hints that the object is drawn in a different orientation.
    /// If a non-identity orientation is passed, the top left of the inline
    /// object should be rotated around the given x and y coordinates.
    /// IDWriteAnalyzer2::GetGlyphOrientationTransform returns the necessary
    /// transform for this.
    /// </remarks>
    STDMETHOD(DrawInlineObject)(
        _In_opt_ void* clientDrawingContext,
        FLOAT originX,
        FLOAT originY,
        DWRITE_GLYPH_ORIENTATION_ANGLE orientationAngle,
        _In_ IDWriteInlineObject* inlineObject,
        BOOL isSideways,
        BOOL isRightToLeft,
        _In_opt_ IUnknown* clientDrawingEffect
        ) PURE;

    using IDWriteTextRenderer::DrawGlyphRun;
    using IDWriteTextRenderer::DrawUnderline;
    using IDWriteTextRenderer::DrawStrikethrough;
    using IDWriteTextRenderer::DrawInlineObject;
};


/// <summary>
/// The format of text used for text layout.
/// </summary>
/// <remarks>
/// This object may not be thread-safe and it may carry the state of text format change.
/// </remarks>
interface DWRITE_DECLARE_INTERFACE("5F174B49-0D8B-4CFB-8BCA-F1CCE9D06C67") IDWriteTextFormat1 : public IDWriteTextFormat
{
    /// <summary>
    /// Set the preferred orientation of glyphs when using a vertical reading direction.
    /// </summary>
    /// <param name="glyphOrientation">Preferred glyph orientation.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(SetVerticalGlyphOrientation)(
        DWRITE_VERTICAL_GLYPH_ORIENTATION glyphOrientation
        ) PURE;

    /// <summary>
    /// Get the preferred orientation of glyphs when using a vertical reading
    /// direction.
    /// </summary>
    STDMETHOD_(DWRITE_VERTICAL_GLYPH_ORIENTATION, GetVerticalGlyphOrientation)() PURE;

    /// <summary>
    /// Set whether or not the last word on the last line is wrapped.
    /// </summary>
    /// <param name="isLastLineWrappingEnabled">Line wrapping option.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(SetLastLineWrapping)(
        BOOL isLastLineWrappingEnabled
        ) PURE;

    /// <summary>
    /// Get whether or not the last word on the last line is wrapped.
    /// </summary>
    STDMETHOD_(BOOL, GetLastLineWrapping)() PURE;

    /// <summary>
    /// Set how the glyphs align to the edges the margin. Default behavior is
    /// to align glyphs using their default glyphs metrics which include side
    /// bearings.
    /// </summary>
    /// <param name="opticalAlignment">Optical alignment option.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(SetOpticalAlignment)(
        DWRITE_OPTICAL_ALIGNMENT opticalAlignment
        ) PURE;

    /// <summary>
    /// Get how the glyphs align to the edges the margin.
    /// </summary>
    STDMETHOD_(DWRITE_OPTICAL_ALIGNMENT, GetOpticalAlignment)() PURE;

    /// <summary>
    /// Apply a custom font fallback onto layout. If none is specified,
    /// layout uses the system fallback list.
    /// </summary>
    /// <param name="fontFallback">Custom font fallback created from
    ///     IDWriteFontFallbackBuilder::CreateFontFallback or from
    ///     IDWriteFactory2::GetSystemFontFallback.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(SetFontFallback)(
        IDWriteFontFallback* fontFallback
        ) PURE;

    /// <summary>
    /// Get the current font fallback object.
    /// </summary>
    STDMETHOD(GetFontFallback)(
        __out IDWriteFontFallback** fontFallback
        ) PURE;
};


/// <summary>
/// The text layout interface represents a block of text after it has
/// been fully analyzed and formatted.
///
/// All coordinates are in device independent pixels (DIPs).
/// </summary>
interface DWRITE_DECLARE_INTERFACE("1093C18F-8D5E-43F0-B064-0917311B525E") IDWriteTextLayout2 : public IDWriteTextLayout1
{
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
        _Out_ DWRITE_TEXT_METRICS1* textMetrics
        ) PURE;

    using IDWriteTextLayout::GetMetrics;

    /// <summary>
    /// Set the preferred orientation of glyphs when using a vertical reading direction.
    /// </summary>
    /// <param name="glyphOrientation">Preferred glyph orientation.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(SetVerticalGlyphOrientation)(
        DWRITE_VERTICAL_GLYPH_ORIENTATION glyphOrientation
        ) PURE;

    /// <summary>
    /// Get the preferred orientation of glyphs when using a vertical reading
    /// direction.
    /// </summary>
    STDMETHOD_(DWRITE_VERTICAL_GLYPH_ORIENTATION, GetVerticalGlyphOrientation)() PURE;

    /// <summary>
    /// Set whether or not the last word on the last line is wrapped.
    /// </summary>
    /// <param name="isLastLineWrappingEnabled">Line wrapping option.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(SetLastLineWrapping)(
        BOOL isLastLineWrappingEnabled
        ) PURE;

    /// <summary>
    /// Get whether or not the last word on the last line is wrapped.
    /// </summary>
    STDMETHOD_(BOOL, GetLastLineWrapping)() PURE;

    /// <summary>
    /// Set how the glyphs align to the edges the margin. Default behavior is
    /// to align glyphs using their default glyphs metrics which include side
    /// bearings.
    /// </summary>
    /// <param name="opticalAlignment">Optical alignment option.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(SetOpticalAlignment)(
        DWRITE_OPTICAL_ALIGNMENT opticalAlignment
        ) PURE;

    /// <summary>
    /// Get how the glyphs align to the edges the margin.
    /// </summary>
    STDMETHOD_(DWRITE_OPTICAL_ALIGNMENT, GetOpticalAlignment)() PURE;

    /// <summary>
    /// Apply a custom font fallback onto layout. If none is specified,
    /// layout uses the system fallback list.
    /// </summary>
    /// <param name="fontFallback">Custom font fallback created from
    ///     IDWriteFontFallbackBuilder::CreateFontFallback or
    ///     IDWriteFactory2::GetSystemFontFallback.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(SetFontFallback)(
        IDWriteFontFallback* fontFallback
        ) PURE;

    /// <summary>
    /// Get the current font fallback object.
    /// </summary>
    STDMETHOD(GetFontFallback)(
        __out IDWriteFontFallback** fontFallback
        ) PURE;
};


/// <summary>
/// The text analyzer interface represents a set of application-defined
/// callbacks that perform rendering of text, inline objects, and decorations
/// such as underlines.
/// </summary>
interface DWRITE_DECLARE_INTERFACE("553A9FF3-5693-4DF7-B52B-74806F7F2EB9") IDWriteTextAnalyzer2 : public IDWriteTextAnalyzer1
{
    /// <summary>
    /// Returns 2x3 transform matrix for the respective angle to draw the
    /// glyph run or other object.
    /// </summary>
    /// <param name="glyphOrientationAngle">The angle reported to one of the application callbacks,
    ///     including IDWriteTextAnalysisSink1::SetGlyphOrientation and IDWriteTextRenderer1::Draw*.</param>
    /// <param name="isSideways">Whether the run's glyphs are sideways or not.</param>
    /// <param name="originX">X origin of the element, be it a glyph run or underline or other.</param>
    /// <param name="originY">Y origin of the element, be it a glyph run or underline or other.</param>
    /// <param name="transform">Returned transform.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    /// <remarks>
    /// This rotates around the given origin x and y, returning a translation component
    /// such that the glyph run, text decoration, or inline object is drawn with the
    /// right orientation at the expected coordinate.
    /// </remarks>
    STDMETHOD(GetGlyphOrientationTransform)(
        DWRITE_GLYPH_ORIENTATION_ANGLE glyphOrientationAngle,
        BOOL isSideways,
        FLOAT originX,
        FLOAT originY,
        _Out_ DWRITE_MATRIX* transform
        ) PURE;

    /// <summary>
    /// Returns a list of typographic feature tags for the given script and language.
    /// </summary>
    /// <param name="fontFace">The font face to get features from.</param>
    /// <param name="scriptAnalysis">Script analysis result from AnalyzeScript.</param>
    /// <param name="localeName">The locale to use when selecting the feature,
    ///     such en-us or ja-jp.</param>
    /// <param name="maxTagCount">Maximum tag count.</param>
    /// <param name="actualTagCount">Actual tag count. If greater than
    ///     maxTagCount, E_NOT_SUFFICIENT_BUFFER is returned, and the call
    ///     should be retried with a larger buffer.</param>
    /// <param name="tags">Feature tag list.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetTypographicFeatures)(
        IDWriteFontFace* fontFace,
        DWRITE_SCRIPT_ANALYSIS scriptAnalysis,
        _In_opt_z_ WCHAR const* localeName,
        UINT32 maxTagCount,
        _Out_ UINT32* actualTagCount,
        _Out_writes_(maxTagCount) DWRITE_FONT_FEATURE_TAG* tags
        ) PURE;

    /// <summary>
    /// Returns an array of which glyphs are affected by a given feature.
    /// </summary>
    /// <param name="fontFace">The font face to read glyph information from.</param>
    /// <param name="scriptAnalysis">Script analysis result from AnalyzeScript.</param>
    /// <param name="localeName">The locale to use when selecting the feature,
    ///     such en-us or ja-jp.</param>
    /// <param name="featureTag">OpenType feature name to use, which may be one
    ///     of the DWRITE_FONT_FEATURE_TAG values or a custom feature using
    ///     DWRITE_MAKE_OPENTYPE_TAG.</param>
    /// <param name="glyphCount">Number of glyph indices to check.</param>
    /// <param name="glyphIndices">Glyph indices to check for feature application.</param>
    /// <param name="featureApplies">Output of which glyphs are affected by the
    ///     feature, where for each glyph affected, the respective array index
    ///     will be 1. The result is returned per-glyph without regard to
    ///     neighboring context of adjacent glyphs.</param>
    /// </remarks>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(CheckTypographicFeature)(
        IDWriteFontFace* fontFace,
        DWRITE_SCRIPT_ANALYSIS scriptAnalysis,
        _In_opt_z_ WCHAR const* localeName,
        DWRITE_FONT_FEATURE_TAG featureTag,
        UINT32 glyphCount,
        _In_reads_(glyphCount) UINT16 const* glyphIndices,
        _Out_writes_(glyphCount) UINT8* featureApplies
        ) PURE;

    using IDWriteTextAnalyzer1::GetGlyphOrientationTransform;
};


/// <summary>
/// A font fallback definition used for mapping characters to fonts capable of
/// supporting them.
/// </summary>
interface DWRITE_DECLARE_INTERFACE("EFA008F9-F7A1-48BF-B05C-F224713CC0FF") IDWriteFontFallback : public IUnknown
{
    /// <summary>
    /// Determines an appropriate font to use to render the range of text.
    /// </summary>
    /// <param name="source">The text source implementation holds the text and
    ///     locale.</param>
    /// <param name="textLength">Length of the text to analyze.</param>
    /// <param name="baseFontCollection">Default font collection to use.</param>
    /// <param name="baseFont">Base font to check (optional).</param>
    /// <param name="baseFamilyName">Family name of the base font. If you pass
    ///     null, no matching will be done against the family.</param>
    /// <param name="baseWeight">Desired weight.</param>
    /// <param name="baseStyle">Desired style.</param>
    /// <param name="baseStretch">Desired stretch.</param>
    /// <param name="mappedLength">Length of text mapped to the mapped font.
    ///     This will always be less or equal to the input text length and
    ///     greater than zero (if the text length is non-zero) so that the
    ///     caller advances at least one character each call.</param>
    /// <param name="mappedFont">The font that should be used to render the
    ///     first mappedLength characters of the text. If it returns NULL,
    ///     then no known font can render the text, and mappedLength is the
    ///     number of unsupported characters to skip.</param>
    /// <param name="scale">Scale factor to multiply the em size of the
    ///     returned font by.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(MapCharacters)(
        IDWriteTextAnalysisSource* analysisSource,
        UINT32 textPosition,
        UINT32 textLength,
        _In_opt_ IDWriteFontCollection* baseFontCollection,
        _In_opt_z_ wchar_t const* baseFamilyName,
        DWRITE_FONT_WEIGHT baseWeight,
        DWRITE_FONT_STYLE baseStyle,
        DWRITE_FONT_STRETCH baseStretch,
        _Out_range_(0, textLength) UINT32* mappedLength,
        _COM_Outptr_result_maybenull_ IDWriteFont** mappedFont,
        _Out_ FLOAT* scale
        ) PURE;
};


/// <summary>
/// Builder used to create a font fallback definition by appending a series of
/// fallback mappings, followed by a creation call.
/// </summary>
/// <remarks>
/// This object may not be thread-safe.
/// </remarks>
interface DWRITE_DECLARE_INTERFACE("FD882D06-8ABA-4FB8-B849-8BE8B73E14DE") IDWriteFontFallbackBuilder : public IUnknown
{
    /// <summary>
    /// Appends a single mapping to the list. Call this once for each additional mapping.
    /// </summary>
    /// <param name="ranges">Unicode ranges that apply to this mapping.</param>
    /// <param name="rangesCount">Number of Unicode ranges.</param>
    /// <param name="localeName">Locale of the context (e.g. document locale).</param>
    /// <param name="baseFamilyName">Base family name to match against, if applicable.</param>
    /// <param name="fontCollection">Explicit font collection for this mapping (optional).</param>
    /// <param name="targetFamilyNames">List of target family name strings.</param>
    /// <param name="targetFamilyNamesCount">Number of target family names.</param>
    /// <param name="scale">Scale factor to multiply the result target font by.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(AddMapping)(
        _In_reads_(rangesCount) DWRITE_UNICODE_RANGE const* ranges,
        UINT32 rangesCount,
        _In_reads_(targetFamilyNamesCount) WCHAR const** targetFamilyNames,
        UINT32 targetFamilyNamesCount,
        _In_opt_ IDWriteFontCollection* fontCollection = NULL,
        _In_opt_z_ WCHAR const* localeName = NULL,
        _In_opt_z_ WCHAR const* baseFamilyName = NULL,
        FLOAT scale = 1.0f
        ) PURE;

    /// <summary>
    /// Appends all the mappings from an existing font fallback object.
    /// </summary>
    /// <param name="fontFallback">Font fallback to read mappings from.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(AddMappings)(
        IDWriteFontFallback* fontFallback
        ) PURE;

    /// <summary>
    /// Creates the finalized fallback object from the mappings added.
    /// </summary>
    /// <param name="fontFallback">Created fallback list.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
   STDMETHOD(CreateFontFallback)(
        _COM_Outptr_ IDWriteFontFallback** fontFallback
        ) PURE;
};

/// <summary>
/// DWRITE_COLOR_F
/// </summary>
#ifndef D3DCOLORVALUE_DEFINED

typedef struct _D3DCOLORVALUE {
    union {
    FLOAT r;
    FLOAT dvR;
    };
    union {
    FLOAT g;
    FLOAT dvG;
    };
    union {
    FLOAT b;
    FLOAT dvB;
    };
    union {
    FLOAT a;
    FLOAT dvA;
    };
} D3DCOLORVALUE;

#define D3DCOLORVALUE_DEFINED
#endif // D3DCOLORVALUE_DEFINED

typedef D3DCOLORVALUE DWRITE_COLOR_F;

/// <summary>
/// The IDWriteFont interface represents a physical font in a font collection.
/// </summary>
interface DWRITE_DECLARE_INTERFACE("29748ed6-8c9c-4a6a-be0b-d912e8538944") IDWriteFont2 : public IDWriteFont1
{
    /// <summary>
    /// Returns TRUE if the font contains tables that can provide color information
    /// (including COLR, CPAL, SVG, CBDT, sbix  tables), or FALSE if not. Note that
    /// TRUE is returned even in the case when the font tables contain only grayscale
    /// images.
    /// </summary>
    STDMETHOD_(BOOL, IsColorFont)() PURE;
};

/// <summary>
/// The interface that represents an absolute reference to a font face.
/// It contains font face type, appropriate file references and face identification data.
/// Various font data such as metrics, names and glyph outlines is obtained from IDWriteFontFace.
/// </summary>
interface DWRITE_DECLARE_INTERFACE("d8b768ff-64bc-4e66-982b-ec8e87f693f7") IDWriteFontFace2 : public IDWriteFontFace1
{
    /// <summary>
    /// Returns TRUE if the font contains tables that can provide color information
    /// (including COLR, CPAL, SVG, CBDT, sbix  tables), or FALSE if not. Note that
    /// TRUE is returned even in the case when the font tables contain only grayscale
    /// images.
    /// </summary>
    STDMETHOD_(BOOL, IsColorFont)() PURE;

    /// <summary>
    /// Returns the number of color palettes defined by the font. The return
    /// value is zero if the font has no color information. Color fonts must
    /// have at least one palette, with palette index zero being the default.
    /// </summary>
    STDMETHOD_(UINT32, GetColorPaletteCount)() PURE;

    /// <summary>
    /// Returns the number of entries in each color palette. All color palettes
    /// in a font have the same number of palette entries. The return value is 
    /// zero if the font has no color information.
    /// </summary>
    STDMETHOD_(UINT32, GetPaletteEntryCount)() PURE;

    /// <summary>
    /// Reads color values from the font's color palette.
    /// </summary>
    /// <param name="colorPaletteIndex">Zero-based index of the color palette. If the
    /// font does not have a palette with the specified index, the method returns 
    /// DWRITE_E_NOCOLOR.<param>
    /// <param name="firstEntryIndex">Zero-based index of the first palette entry
    /// to read.</param>
    /// <param name="entryCount">Number of palette entries to read.</param>
    /// <param name="paletteEntries">Array that receives the color values.<param>
    /// <returns>
    /// Standard HRESULT error code.
    /// The return value is E_INVALIDARG if firstEntryIndex + entryCount is greater
    /// than the actual number of palette entries as returned by GetPaletteEntryCount.
    /// The return value is DWRITE_E_NOCOLOR if the font does not have a palette
    /// with the specified palette index.
    /// </returns>
    STDMETHOD(GetPaletteEntries)(
        UINT32 colorPaletteIndex,
        UINT32 firstEntryIndex,
        UINT32 entryCount,
        _Out_writes_(entryCount) DWRITE_COLOR_F* paletteEntries
        ) PURE;

    /// <summary>
    /// Determines the recommended text rendering and grid-fit mode to be used based on the
    /// font, size, world transform, and measuring mode.
    /// </summary>
    /// <param name="fontEmSize">Logical font size in DIPs.</param>
    /// <param name="dpiX">Number of pixels per logical inch in the horizontal direction.</param>
    /// <param name="dpiY">Number of pixels per logical inch in the vertical direction.</param>
    /// <param name="transform">Specifies the world transform.</param>
    /// <param name="outlineThreshold">Specifies the quality of the graphics system's outline rendering,
    /// affects the size threshold above which outline rendering is used.</param>
    /// <param name="measuringMode">Specifies the method used to measure during text layout. For proper
    /// glyph spacing, the function returns a rendering mode that is compatible with the specified 
    /// measuring mode.</param>
    /// <param name="renderingParams">Rendering parameters object. This parameter is necessary in case the rendering parameters 
    /// object overrides the rendering mode.</param>
    /// <param name="renderingMode">Receives the recommended rendering mode.</param>
    /// <param name="gridFitMode">Receives the recommended grid-fit mode.</param>
    /// <remarks>
    /// This method should be used to determine the actual rendering mode in cases where the rendering 
    /// mode of the rendering params object is DWRITE_RENDERING_MODE_DEFAULT, and the actual grid-fit
    /// mode when the rendering params object is DWRITE_GRID_FIT_MODE_DEFAULT.
    /// </remarks>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetRecommendedRenderingMode)(
        FLOAT fontEmSize,
        FLOAT dpiX,
        FLOAT dpiY,
        _In_opt_ DWRITE_MATRIX const* transform,
        BOOL isSideways,
        DWRITE_OUTLINE_THRESHOLD outlineThreshold,
        DWRITE_MEASURING_MODE measuringMode,
        _In_opt_ IDWriteRenderingParams* renderingParams,
        _Out_ DWRITE_RENDERING_MODE* renderingMode,
        _Out_ DWRITE_GRID_FIT_MODE* gridFitMode
        ) PURE;

    using IDWriteFontFace1::GetRecommendedRenderingMode;
};

/// <summary>
/// Represents a color glyph run. The IDWriteFactory2::TranslateColorGlyphRun
/// method returns an ordered collection of color glyph runs, which can be
/// layered on top of each other to produce a color representation of the
/// given base glyph run.
/// </summary>
struct DWRITE_COLOR_GLYPH_RUN
{
    /// <summary>
    /// Glyph run to render.
    /// </summary>
    DWRITE_GLYPH_RUN glyphRun;

    /// <summary>
    /// Optional glyph run description.
    /// </summary>
    _Maybenull_ DWRITE_GLYPH_RUN_DESCRIPTION* glyphRunDescription;

    /// <summary>
    /// Location at which to draw this glyph run.
    /// </summary>
    FLOAT baselineOriginX;
    FLOAT baselineOriginY;

    /// <summary>
    /// Color to use for this layer, if any. This is the same color that
    /// IDWriteFontFace2::GetPaletteEntries would return for the current
    /// palette index if the paletteIndex member is less than 0xFFFF. If
    /// the paletteIndex member is 0xFFFF then there is no associated
    /// palette entry, this member is set to { 0, 0, 0, 0 }, and the client
    /// should use the current foreground brush.
    /// </summary>
    DWRITE_COLOR_F runColor;

    /// <summary>
    /// Zero-based index of this layer's color entry in the current color
    /// palette, or 0xFFFF if this layer is to be rendered using 
    /// the current foreground brush.
    /// </summary>
    UINT16 paletteIndex;
};

/// <summary>
/// Enumerator for an ordered collection of color glyph runs.
/// </summary>
interface DWRITE_DECLARE_INTERFACE("d31fbe17-f157-41a2-8d24-cb779e0560e8") IDWriteColorGlyphRunEnumerator : public IUnknown
{
    /// <summary>
    /// Advances to the first or next color run. The runs are enumerated
    /// in order from back to front.
    /// </summary>
    /// <param name="hasRun">Receives TRUE if there is a current run or
    /// FALSE if the end of the sequence has been reached.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(MoveNext)(
        _Out_ BOOL* hasRun
        ) PURE;

    /// <summary>
    /// Gets the current color glyph run.
    /// </summary>
    /// <param name="colorGlyphRun">Receives a pointer to the color
    /// glyph run. The pointer remains valid until the next call to
    /// MoveNext or until the interface is released.</param>
    /// <returns>
    /// Standard HRESULT error code. An error is returned if there is
    /// no current glyph run, i.e., if MoveNext has not yet been called
    /// or if the end of the sequence has been reached.
    /// </returns>
    STDMETHOD(GetCurrentRun)(
        _Outptr_ DWRITE_COLOR_GLYPH_RUN const** colorGlyphRun
        ) PURE;
};

/// <summary>
/// The interface that represents text rendering settings for glyph rasterization and filtering.
/// </summary>
interface DWRITE_DECLARE_INTERFACE("F9D711C3-9777-40AE-87E8-3E5AF9BF0948") IDWriteRenderingParams2 : public IDWriteRenderingParams1
{
    /// <summary>
    /// Gets the grid fitting mode.
    /// </summary>
    STDMETHOD_(DWRITE_GRID_FIT_MODE, GetGridFitMode)() PURE;
};

/// <summary>
/// The root factory interface for all DWrite objects.
/// </summary>
interface DWRITE_DECLARE_INTERFACE("0439fc60-ca44-4994-8dee-3a9af7b732ec") IDWriteFactory2 : public IDWriteFactory1
{
    /// <summary>
    /// Get the system-appropriate font fallback mapping list.
    /// </summary>
    /// <param name="fontFallback">The system fallback list.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(GetSystemFontFallback)(
        _COM_Outptr_ IDWriteFontFallback** fontFallback
        ) PURE;

    /// <summary>
    /// Create a custom font fallback builder.
    /// </summary>
    /// <param name="fontFallbackBuilder">Empty font fallback builder.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(CreateFontFallbackBuilder)(
        _COM_Outptr_ IDWriteFontFallbackBuilder** fontFallbackBuilder
        ) PURE;

    /// <summary>
    /// Translates a glyph run to a sequence of color glyph runs, which can be
    /// rendered to produce a color representation of the original "base" run.
    /// </summary>
    /// <param name="baselineOriginX">Horizontal origin of the base glyph run in
    /// pre-transform coordinates.</param>
    /// <param name="baselineOriginY">Vertical origin of the base glyph run in
    /// pre-transform coordinates.</param>
    /// <param name="glyphRun">Pointer to the original "base" glyph run.</param>
    /// <param name="glyphRunDescription">Optional glyph run description.</param>
    /// <param name="measuringMode">Measuring mode, needed to compute the origins
    /// of each glyph.</param>
    /// <param name="worldToDeviceTransform">Matrix converting from the client's
    /// coordinate space to device coordinates (pixels), i.e., the world transform
    /// multiplied by any DPI scaling.</param>
    /// <param name="colorPaletteIndex">Zero-based index of the color palette to use.
    /// Valid indices are less than the number of palettes in the font, as returned
    /// by IDWriteFontFace2::GetColorPaletteCount.</param>
    /// <param name="colorLayers">If the function succeeds, receives a pointer
    /// to an enumerator object that can be used to obtain the color glyph runs.
    /// If the base run has no color glyphs, then the output pointer is NULL
    /// and the method returns DWRITE_E_NOCOLOR.</param>
    /// <returns>
    /// Returns DWRITE_E_NOCOLOR if the font has no color information, the base
    /// glyph run does not contain any color glyphs, or the specified color palette
    /// index is out of range. In this case, the client should render the base glyph 
    /// run. Otherwise, returns a standard HRESULT error code.
    /// </returns>
    STDMETHOD(TranslateColorGlyphRun)(
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        _In_ DWRITE_GLYPH_RUN const* glyphRun,
        _In_opt_ DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
        DWRITE_MEASURING_MODE measuringMode,
        _In_opt_ DWRITE_MATRIX const* worldToDeviceTransform,
        UINT32 colorPaletteIndex,
        _COM_Outptr_ IDWriteColorGlyphRunEnumerator** colorLayers
        ) PURE;

    /// <summary>
    /// Creates a rendering parameters object with the specified properties.
    /// </summary>
    /// <param name="gamma">The gamma value used for gamma correction, which must be greater than zero and cannot exceed 256.</param>
    /// <param name="enhancedContrast">The amount of contrast enhancement, zero or greater.</param>
    /// <param name="clearTypeLevel">The degree of ClearType level, from 0.0f (no ClearType) to 1.0f (full ClearType).</param>
    /// <param name="pixelGeometry">The geometry of a device pixel.</param>
    /// <param name="renderingMode">Method of rendering glyphs. In most cases, this should be DWRITE_RENDERING_MODE_DEFAULT to automatically use an appropriate mode.</param>
    /// <param name="gridFitMode">How to grid fit glyph outlines. In most cases, this should be DWRITE_GRID_FIT_DEFAULT to automatically choose an appropriate mode.</param>
    /// <param name="renderingParams">Holds the newly created rendering parameters object, or NULL in case of failure.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(CreateCustomRenderingParams)(
        FLOAT gamma,
        FLOAT enhancedContrast,
        FLOAT grayscaleEnhancedContrast,
        FLOAT clearTypeLevel,
        DWRITE_PIXEL_GEOMETRY pixelGeometry,
        DWRITE_RENDERING_MODE renderingMode,
        DWRITE_GRID_FIT_MODE gridFitMode,
        _COM_Outptr_ IDWriteRenderingParams2** renderingParams
        ) PURE;

    using IDWriteFactory::CreateCustomRenderingParams;
    using IDWriteFactory1::CreateCustomRenderingParams;

    /// <summary>
    /// Creates a glyph run analysis object, which encapsulates information
    /// used to render a glyph run.
    /// </summary>
    /// <param name="glyphRun">Structure specifying the properties of the glyph run.</param>
    /// <param name="transform">Optional transform applied to the glyphs and their positions. This transform is applied after the
    /// scaling specified by the emSize and pixelsPerDip.</param>
    /// <param name="renderingMode">Specifies the rendering mode, which must be one of the raster rendering modes (i.e., not default
    /// and not outline).</param>
    /// <param name="measuringMode">Specifies the method to measure glyphs.</param>
    /// <param name="gridFitMode">How to grid-fit glyph outlines. This must be non-default.</param>
    /// <param name="baselineOriginX">Horizontal position of the baseline origin, in DIPs.</param>
    /// <param name="baselineOriginY">Vertical position of the baseline origin, in DIPs.</param>
    /// <param name="glyphRunAnalysis">Receives a pointer to the newly created object.</param>
    /// <returns>
    /// Standard HRESULT error code.
    /// </returns>
    STDMETHOD(CreateGlyphRunAnalysis)(
        _In_ DWRITE_GLYPH_RUN const* glyphRun,
        _In_opt_ DWRITE_MATRIX const* transform,
        DWRITE_RENDERING_MODE renderingMode,
        DWRITE_MEASURING_MODE measuringMode,
        DWRITE_GRID_FIT_MODE gridFitMode,
        DWRITE_TEXT_ANTIALIAS_MODE antialiasMode,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        _COM_Outptr_ IDWriteGlyphRunAnalysis** glyphRunAnalysis
        ) PURE;

    using IDWriteFactory::CreateGlyphRunAnalysis;
};


#endif /* DWRITE_2_H_INCLUDED */
