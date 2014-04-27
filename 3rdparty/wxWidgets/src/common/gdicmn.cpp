/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/gdicmn.cpp
// Purpose:     Common GDI classes
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: gdicmn.cpp 50022 2007-11-17 14:24:18Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include "wx/gdicmn.h"
#include "wx/gdiobj.h"

#ifndef WX_PRECOMP
    #include "wx/log.h"
    #include "wx/pen.h"
    #include "wx/brush.h"
    #include "wx/palette.h"
    #include "wx/icon.h"
    #include "wx/cursor.h"
    #include "wx/settings.h"
    #include "wx/bitmap.h"
    #include "wx/colour.h"
    #include "wx/font.h"
#endif


IMPLEMENT_DYNAMIC_CLASS(wxGDIObject, wxObject)


WXDLLIMPEXP_DATA_CORE(wxBrushList*) wxTheBrushList;
WXDLLIMPEXP_DATA_CORE(wxFontList*)  wxTheFontList;
WXDLLIMPEXP_DATA_CORE(wxPenList*)   wxThePenList;

WXDLLIMPEXP_DATA_CORE(wxColourDatabase*) wxTheColourDatabase;

WXDLLIMPEXP_DATA_CORE(wxBitmap)  wxNullBitmap;
WXDLLIMPEXP_DATA_CORE(wxBrush)   wxNullBrush;
WXDLLIMPEXP_DATA_CORE(wxColour)  wxNullColour;
WXDLLIMPEXP_DATA_CORE(wxCursor)  wxNullCursor;
WXDLLIMPEXP_DATA_CORE(wxFont)    wxNullFont;
WXDLLIMPEXP_DATA_CORE(wxIcon)    wxNullIcon;
WXDLLIMPEXP_DATA_CORE(wxPen)     wxNullPen;
#if wxUSE_PALETTE
WXDLLIMPEXP_DATA_CORE(wxPalette) wxNullPalette;
#endif

const wxSize wxDefaultSize(wxDefaultCoord, wxDefaultCoord);
const wxPoint wxDefaultPosition(wxDefaultCoord, wxDefaultCoord);

#if wxUSE_EXTENDED_RTTI

// wxPoint

template<> void wxStringReadValue(const wxString &s , wxPoint &data )
{
    wxSscanf(s, wxT("%d,%d"), &data.x , &data.y ) ;
}

template<> void wxStringWriteValue(wxString &s , const wxPoint &data )
{
    s = wxString::Format(wxT("%d,%d"), data.x , data.y ) ;
}

wxCUSTOM_TYPE_INFO(wxPoint, wxToStringConverter<wxPoint> , wxFromStringConverter<wxPoint>)

template<> void wxStringReadValue(const wxString &s , wxSize &data )
{
    wxSscanf(s, wxT("%d,%d"), &data.x , &data.y ) ;
}

template<> void wxStringWriteValue(wxString &s , const wxSize &data )
{
    s = wxString::Format(wxT("%d,%d"), data.x , data.y ) ;
}

wxCUSTOM_TYPE_INFO(wxSize, wxToStringConverter<wxSize> , wxFromStringConverter<wxSize>)

#endif

wxRect::wxRect(const wxPoint& point1, const wxPoint& point2)
{
    x = point1.x;
    y = point1.y;
    width = point2.x - point1.x;
    height = point2.y - point1.y;

    if (width < 0)
    {
        width = -width;
        x = point2.x;
    }
    width++;

    if (height < 0)
    {
        height = -height;
        y = point2.y;
    }
    height++;
}

bool wxRect::operator==(const wxRect& rect) const
{
    return ((x == rect.x) &&
            (y == rect.y) &&
            (width == rect.width) &&
            (height == rect.height));
}

wxRect wxRect::operator+(const wxRect& rect) const
{
    int x1 = wxMin(this->x, rect.x);
    int y1 = wxMin(this->y, rect.y);
    int y2 = wxMax(y+height, rect.height+rect.y);
    int x2 = wxMax(x+width, rect.width+rect.x);
    return wxRect(x1, y1, x2-x1, y2-y1);
}

wxRect& wxRect::Union(const wxRect& rect)
{
    // ignore empty rectangles: union with an empty rectangle shouldn't extend
    // this one to (0, 0)
    if ( !width || !height )
    {
        *this = rect;
    }
    else if ( rect.width && rect.height )
    {
        int x1 = wxMin(x, rect.x);
        int y1 = wxMin(y, rect.y);
        int y2 = wxMax(y + height, rect.height + rect.y);
        int x2 = wxMax(x + width, rect.width + rect.x);

        x = x1;
        y = y1;
        width = x2 - x1;
        height = y2 - y1;
    }
    //else: we're not empty and rect is empty

    return *this;
}

wxRect& wxRect::Inflate(wxCoord dx, wxCoord dy)
{
     if (-2*dx>width)
     {
         // Don't allow deflate to eat more width than we have,
         // a well-defined rectangle cannot have negative width.
         x+=width/2;
         width=0;
     }
     else
     {
         // The inflate is valid.
         x-=dx;
         width+=2*dx;
     }

     if (-2*dy>height)
     {
         // Don't allow deflate to eat more height than we have,
         // a well-defined rectangle cannot have negative height.
         y+=height/2;
         height=0;
     }
     else
     {
         // The inflate is valid.
         y-=dy;
         height+=2*dy;
     }

    return *this;
}

bool wxRect::Contains(int cx, int cy) const
{
    return ( (cx >= x) && (cy >= y)
          && ((cy - y) < height)
          && ((cx - x) < width)
          );
}

bool wxRect::Contains(const wxRect& rect) const
{
    return Contains(rect.GetTopLeft()) && Contains(rect.GetBottomRight());
}

wxRect& wxRect::Intersect(const wxRect& rect)
{
    int x2 = GetRight(),
        y2 = GetBottom();

    if ( x < rect.x )
        x = rect.x;
    if ( y < rect.y )
        y = rect.y;
    if ( x2 > rect.GetRight() )
        x2 = rect.GetRight();
    if ( y2 > rect.GetBottom() )
        y2 = rect.GetBottom();

    width = x2 - x + 1;
    height = y2 - y + 1;

    if ( width <= 0 || height <= 0 )
    {
        width =
        height = 0;
    }

    return *this;
}

bool wxRect::Intersects(const wxRect& rect) const
{
    wxRect r = Intersect(rect);

    // if there is no intersection, both width and height are 0
    return r.width != 0;
}

// ============================================================================
// wxColourDatabase
// ============================================================================

// ----------------------------------------------------------------------------
// wxColourDatabase ctor/dtor
// ----------------------------------------------------------------------------

wxColourDatabase::wxColourDatabase ()
{
    // will be created on demand in Initialize()
    m_map = NULL;
}

wxColourDatabase::~wxColourDatabase ()
{
    if ( m_map )
    {
        WX_CLEAR_HASH_MAP(wxStringToColourHashMap, *m_map);

        delete m_map;
    }

#ifdef __WXPM__
    delete [] m_palTable;
#endif
}

// Colour database stuff
void wxColourDatabase::Initialize()
{
    if ( m_map )
    {
        // already initialized
        return;
    }

    m_map = new wxStringToColourHashMap;

    static const struct wxColourDesc
    {
        const wxChar *name;
        unsigned char r,g,b;
    }
    wxColourTable[] =
    {
        {wxT("AQUAMARINE"),112, 219, 147},
        {wxT("BLACK"),0, 0, 0},
        {wxT("BLUE"), 0, 0, 255},
        {wxT("BLUE VIOLET"), 159, 95, 159},
        {wxT("BROWN"), 165, 42, 42},
        {wxT("CADET BLUE"), 95, 159, 159},
        {wxT("CORAL"), 255, 127, 0},
        {wxT("CORNFLOWER BLUE"), 66, 66, 111},
        {wxT("CYAN"), 0, 255, 255},
        {wxT("DARK GREY"), 47, 47, 47},   // ?

        {wxT("DARK GREEN"), 47, 79, 47},
        {wxT("DARK OLIVE GREEN"), 79, 79, 47},
        {wxT("DARK ORCHID"), 153, 50, 204},
        {wxT("DARK SLATE BLUE"), 107, 35, 142},
        {wxT("DARK SLATE GREY"), 47, 79, 79},
        {wxT("DARK TURQUOISE"), 112, 147, 219},
        {wxT("DIM GREY"), 84, 84, 84},
        {wxT("FIREBRICK"), 142, 35, 35},
        {wxT("FOREST GREEN"), 35, 142, 35},
        {wxT("GOLD"), 204, 127, 50},
        {wxT("GOLDENROD"), 219, 219, 112},
        {wxT("GREY"), 128, 128, 128},
        {wxT("GREEN"), 0, 255, 0},
        {wxT("GREEN YELLOW"), 147, 219, 112},
        {wxT("INDIAN RED"), 79, 47, 47},
        {wxT("KHAKI"), 159, 159, 95},
        {wxT("LIGHT BLUE"), 191, 216, 216},
        {wxT("LIGHT GREY"), 192, 192, 192},
        {wxT("LIGHT STEEL BLUE"), 143, 143, 188},
        {wxT("LIME GREEN"), 50, 204, 50},
        {wxT("LIGHT MAGENTA"), 255, 0, 255},
        {wxT("MAGENTA"), 255, 0, 255},
        {wxT("MAROON"), 142, 35, 107},
        {wxT("MEDIUM AQUAMARINE"), 50, 204, 153},
        {wxT("MEDIUM GREY"), 100, 100, 100},
        {wxT("MEDIUM BLUE"), 50, 50, 204},
        {wxT("MEDIUM FOREST GREEN"), 107, 142, 35},
        {wxT("MEDIUM GOLDENROD"), 234, 234, 173},
        {wxT("MEDIUM ORCHID"), 147, 112, 219},
        {wxT("MEDIUM SEA GREEN"), 66, 111, 66},
        {wxT("MEDIUM SLATE BLUE"), 127, 0, 255},
        {wxT("MEDIUM SPRING GREEN"), 127, 255, 0},
        {wxT("MEDIUM TURQUOISE"), 112, 219, 219},
        {wxT("MEDIUM VIOLET RED"), 219, 112, 147},
        {wxT("MIDNIGHT BLUE"), 47, 47, 79},
        {wxT("NAVY"), 35, 35, 142},
        {wxT("ORANGE"), 204, 50, 50},
        {wxT("ORANGE RED"), 255, 0, 127},
        {wxT("ORCHID"), 219, 112, 219},
        {wxT("PALE GREEN"), 143, 188, 143},
        {wxT("PINK"), 255, 192, 203},
        {wxT("PLUM"), 234, 173, 234},
        {wxT("PURPLE"), 176, 0, 255},
        {wxT("RED"), 255, 0, 0},
        {wxT("SALMON"), 111, 66, 66},
        {wxT("SEA GREEN"), 35, 142, 107},
        {wxT("SIENNA"), 142, 107, 35},
        {wxT("SKY BLUE"), 50, 153, 204},
        {wxT("SLATE BLUE"), 0, 127, 255},
        {wxT("SPRING GREEN"), 0, 255, 127},
        {wxT("STEEL BLUE"), 35, 107, 142},
        {wxT("TAN"), 219, 147, 112},
        {wxT("THISTLE"), 216, 191, 216},
        {wxT("TURQUOISE"), 173, 234, 234},
        {wxT("VIOLET"), 79, 47, 79},
        {wxT("VIOLET RED"), 204, 50, 153},
        {wxT("WHEAT"), 216, 216, 191},
        {wxT("WHITE"), 255, 255, 255},
        {wxT("YELLOW"), 255, 255, 0},
        {wxT("YELLOW GREEN"), 153, 204, 50}
    };

    size_t n;

    for ( n = 0; n < WXSIZEOF(wxColourTable); n++ )
    {
        const wxColourDesc& cc = wxColourTable[n];
        (*m_map)[cc.name] = new wxColour(cc.r, cc.g, cc.b);
    }

#ifdef __WXPM__
    m_palTable = new long[n];
    for ( n = 0; n < WXSIZEOF(wxColourTable); n++ )
    {
        const wxColourDesc& cc = wxColourTable[n];
        m_palTable[n] = OS2RGB(cc.r,cc.g,cc.b);
    }
    m_nSize = n;
#endif
}

// ----------------------------------------------------------------------------
// wxColourDatabase operations
// ----------------------------------------------------------------------------

void wxColourDatabase::AddColour(const wxString& name, const wxColour& colour)
{
    Initialize();

    // canonicalize the colour names before using them as keys: they should be
    // in upper case
    wxString colName = name;
    colName.MakeUpper();

    // ... and we also allow both grey/gray
    wxString colNameAlt = colName;
    if ( !colNameAlt.Replace(_T("GRAY"), _T("GREY")) )
    {
        // but in this case it is not necessary so avoid extra search below
        colNameAlt.clear();
    }

    wxStringToColourHashMap::iterator it = m_map->find(colName);
    if ( it == m_map->end() && !colNameAlt.empty() )
        it = m_map->find(colNameAlt);
    if ( it != m_map->end() )
    {
        *(it->second) = colour;
    }
    else // new colour
    {
        (*m_map)[colName] = new wxColour(colour);
    }
}

wxColour wxColourDatabase::Find(const wxString& colour) const
{
    wxColourDatabase * const self = wxConstCast(this, wxColourDatabase);
    self->Initialize();

    // make the comparaison case insensitive and also match both grey and gray
    wxString colName = colour;
    colName.MakeUpper();
    wxString colNameAlt = colName;
    if ( !colNameAlt.Replace(_T("GRAY"), _T("GREY")) )
        colNameAlt.clear();

    wxStringToColourHashMap::iterator it = m_map->find(colName);
    if ( it == m_map->end() && !colNameAlt.empty() )
        it = m_map->find(colNameAlt);
    if ( it != m_map->end() )
        return *(it->second);

    // we did not find any result in existing colours:
    // we won't use wxString -> wxColour conversion because the
    // wxColour::Set(const wxString &) function which does that conversion
    // internally uses this function (wxColourDatabase::Find) and we want
    // to avoid infinite recursion !
    return wxNullColour;
}

wxString wxColourDatabase::FindName(const wxColour& colour) const
{
    wxColourDatabase * const self = wxConstCast(this, wxColourDatabase);
    self->Initialize();

    typedef wxStringToColourHashMap::iterator iterator;

    for ( iterator it = m_map->begin(), en = m_map->end(); it != en; ++it )
    {
        if ( *(it->second) == colour )
            return it->first;
    }

    return wxEmptyString;
}

// ----------------------------------------------------------------------------
// deprecated wxColourDatabase methods
// ----------------------------------------------------------------------------

#if WXWIN_COMPATIBILITY_2_6
wxColour *wxColourDatabase::FindColour(const wxString& name)
{
    // This function is deprecated, use Find() instead.
    // Formerly this function sometimes would return a deletable pointer and
    // sometimes a non-deletable one (when returning a colour from the database).
    // Trying to delete the latter anyway results in problems, so probably
    // nobody ever freed the pointers. Currently it always returns a new
    // instance, which means there will be memory leaks.
    wxLogDebug(wxT("wxColourDataBase::FindColour():")
        wxT(" Please use wxColourDataBase::Find() instead"));

    // using a static variable here is not the most elegant solution but unless
    // we want to make wxStringToColourHashMap public (i.e. move it to the
    // header) so that we could have a member function returning
    // wxStringToColourHashMap::iterator, there is really no good way to do it
    // otherwise
    //
    // and knowing that this function is going to disappear in the next release
    // anyhow I don't want to waste time on this

    static wxColour s_col;

    s_col = Find(name);
    if ( !s_col.Ok() )
        return NULL;

    return new wxColour(s_col);
}
#endif // WXWIN_COMPATIBILITY_2_6

// ============================================================================
// stock objects
// ============================================================================

static wxStockGDI gs_wxStockGDI_instance;
wxStockGDI* wxStockGDI::ms_instance = &gs_wxStockGDI_instance;
wxObject* wxStockGDI::ms_stockObject[ITEMCOUNT];

wxStockGDI::wxStockGDI()
{
}

wxStockGDI::~wxStockGDI()
{
}

void wxStockGDI::DeleteAll()
{
    for (unsigned i = 0; i < ITEMCOUNT; i++)
    {
        delete ms_stockObject[i];
        ms_stockObject[i] = NULL;
    }
}

const wxBrush* wxStockGDI::GetBrush(Item item)
{
    wxBrush* brush = wx_static_cast(wxBrush*, ms_stockObject[item]);
    if (brush == NULL)
    {
        switch (item)
        {
        case BRUSH_BLACK:
            brush = new wxBrush(*GetColour(COLOUR_BLACK), wxSOLID);
            break;
        case BRUSH_BLUE:
            brush = new wxBrush(*GetColour(COLOUR_BLUE), wxSOLID);
            break;
        case BRUSH_CYAN:
            brush = new wxBrush(*GetColour(COLOUR_CYAN), wxSOLID);
            break;
        case BRUSH_GREEN:
            brush = new wxBrush(*GetColour(COLOUR_GREEN), wxSOLID);
            break;
        case BRUSH_GREY:
            brush = new wxBrush(wxColour(wxT("GREY")), wxSOLID);
            break;
        case BRUSH_LIGHTGREY:
            brush = new wxBrush(*GetColour(COLOUR_LIGHTGREY), wxSOLID);
            break;
        case BRUSH_MEDIUMGREY:
            brush = new wxBrush(wxColour(wxT("MEDIUM GREY")), wxSOLID);
            break;
        case BRUSH_RED:
            brush = new wxBrush(*GetColour(COLOUR_RED), wxSOLID);
            break;
        case BRUSH_TRANSPARENT:
            brush = new wxBrush(*GetColour(COLOUR_BLACK), wxTRANSPARENT);
            break;
        case BRUSH_WHITE:
            brush = new wxBrush(*GetColour(COLOUR_WHITE), wxSOLID);
            break;
        default:
            wxFAIL;
        }
        ms_stockObject[item] = brush;
    }
    return brush;
}

const wxColour* wxStockGDI::GetColour(Item item)
{
    wxColour* colour = wx_static_cast(wxColour*, ms_stockObject[item]);
    if (colour == NULL)
    {
        switch (item)
        {
        case COLOUR_BLACK:
            colour = new wxColour(0, 0, 0);
            break;
        case COLOUR_BLUE:
            colour = new wxColour(0, 0, 255);
            break;
        case COLOUR_CYAN:
            colour = new wxColour(wxT("CYAN"));
            break;
        case COLOUR_GREEN:
            colour = new wxColour(0, 255, 0);
            break;
        case COLOUR_LIGHTGREY:
            colour = new wxColour(wxT("LIGHT GREY"));
            break;
        case COLOUR_RED:
            colour = new wxColour(255, 0, 0);
            break;
        case COLOUR_WHITE:
            colour = new wxColour(255, 255, 255);
            break;
        default:
            wxFAIL;
        }
        ms_stockObject[item] = colour;
    }
    return colour;
}

const wxCursor* wxStockGDI::GetCursor(Item item)
{
    wxCursor* cursor = wx_static_cast(wxCursor*, ms_stockObject[item]);
    if (cursor == NULL)
    {
        switch (item)
        {
        case CURSOR_CROSS:
            cursor = new wxCursor(wxCURSOR_CROSS);
            break;
        case CURSOR_HOURGLASS:
            cursor = new wxCursor(wxCURSOR_WAIT);
            break;
        case CURSOR_STANDARD:
            cursor = new wxCursor(wxCURSOR_ARROW);
            break;
        default:
            wxFAIL;
        }
        ms_stockObject[item] = cursor;
    }
    return cursor;
}

const wxFont* wxStockGDI::GetFont(Item item)
{
    wxFont* font = wx_static_cast(wxFont*, ms_stockObject[item]);
    if (font == NULL)
    {
        switch (item)
        {
        case FONT_ITALIC:
            font = new wxFont(GetFont(FONT_NORMAL)->GetPointSize(), wxROMAN, wxITALIC, wxNORMAL);
            break;
        case FONT_NORMAL:
            font = new wxFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
            break;
        case FONT_SMALL:
            font = new wxFont(GetFont(FONT_NORMAL)->GetPointSize() - 2, wxSWISS, wxNORMAL, wxNORMAL);
            break;
        case FONT_SWISS:
            font = new wxFont(GetFont(FONT_NORMAL)->GetPointSize(), wxSWISS, wxNORMAL, wxNORMAL);
            break;
        default:
            wxFAIL;
        }
        ms_stockObject[item] = font;
    }
    return font;
}

const wxPen* wxStockGDI::GetPen(Item item)
{
    wxPen* pen = wx_static_cast(wxPen*, ms_stockObject[item]);
    if (pen == NULL)
    {
        switch (item)
        {
        case PEN_BLACK:
            pen = new wxPen(*GetColour(COLOUR_BLACK), 1, wxSOLID);
            break;
        case PEN_BLACKDASHED:
            pen = new wxPen(*GetColour(COLOUR_BLACK), 1, wxSHORT_DASH);
            break;
        case PEN_CYAN:
            pen = new wxPen(*GetColour(COLOUR_CYAN), 1, wxSOLID);
            break;
        case PEN_GREEN:
            pen = new wxPen(*GetColour(COLOUR_GREEN), 1, wxSOLID);
            break;
        case PEN_GREY:
            pen = new wxPen(wxColour(wxT("GREY")), 1, wxSOLID);
            break;
        case PEN_LIGHTGREY:
            pen = new wxPen(*GetColour(COLOUR_LIGHTGREY), 1, wxSOLID);
            break;
        case PEN_MEDIUMGREY:
            pen = new wxPen(wxColour(wxT("MEDIUM GREY")), 1, wxSOLID);
            break;
        case PEN_RED:
            pen = new wxPen(*GetColour(COLOUR_RED), 1, wxSOLID);
            break;
        case PEN_TRANSPARENT:
            pen = new wxPen(*GetColour(COLOUR_BLACK), 1, wxTRANSPARENT);
            break;
        case PEN_WHITE:
            pen = new wxPen(*GetColour(COLOUR_WHITE), 1, wxSOLID);
            break;
        default:
            wxFAIL;
        }
        ms_stockObject[item] = pen;
    }
    return pen;
}

void wxInitializeStockLists()
{
    wxTheColourDatabase = new wxColourDatabase;

    wxTheBrushList = new wxBrushList;
    wxThePenList = new wxPenList;
    wxTheFontList = new wxFontList;
}

void wxDeleteStockLists()
{
    wxDELETE(wxTheBrushList);
    wxDELETE(wxThePenList);
    wxDELETE(wxTheFontList);
}

// ============================================================================
// wxTheXXXList stuff (semi-obsolete)
// ============================================================================

wxGDIObjListBase::wxGDIObjListBase()
{
}

wxGDIObjListBase::~wxGDIObjListBase()
{
    for (wxList::compatibility_iterator node = list.GetFirst(); node; node = node->GetNext())
    {
        delete wx_static_cast(wxObject*, node->GetData());
    }
}

wxPen *wxPenList::FindOrCreatePen (const wxColour& colour, int width, int style)
{
    for ( wxList::compatibility_iterator node = list.GetFirst();
          node;
          node = node->GetNext() )
    {
        wxPen * const pen = (wxPen *) node->GetData();
        if ( pen->GetWidth () == width &&
                pen->GetStyle () == style &&
                    pen->GetColour() == colour )
            return pen;
    }

    wxPen* pen = NULL;
    wxPen penTmp(colour, width, style);
    if (penTmp.Ok())
    {
        pen = new wxPen(penTmp);
        list.Append(pen);
    }

    return pen;
}

wxBrush *wxBrushList::FindOrCreateBrush (const wxColour& colour, int style)
{
    for ( wxList::compatibility_iterator node = list.GetFirst();
          node;
          node = node->GetNext() )
    {
        wxBrush * const brush = (wxBrush *) node->GetData ();
        if ( brush->GetStyle () == style && brush->GetColour() == colour )
            return brush;
    }

    wxBrush* brush = NULL;
    wxBrush brushTmp(colour, style);
    if (brushTmp.Ok())
    {
        brush = new wxBrush(brushTmp);
        list.Append(brush);
    }

    return brush;
}

wxFont *wxFontList::FindOrCreateFont(int pointSize,
                                     int family,
                                     int style,
                                     int weight,
                                     bool underline,
                                     const wxString& facename,
                                     wxFontEncoding encoding)
{
    wxFont *font;
    wxList::compatibility_iterator node;
    for (node = list.GetFirst(); node; node = node->GetNext())
    {
        font = (wxFont *)node->GetData();
        if (
             font->GetPointSize () == pointSize &&
             font->GetStyle () == style &&
             font->GetWeight () == weight &&
             font->GetUnderlined () == underline )
        {
            int fontFamily = font->GetFamily();

#if defined(__WXGTK__)
            // under GTK the default family is wxSWISS, so looking for a font
            // with wxDEFAULT family should return a wxSWISS one instead of
            // creating a new one
            bool same = (fontFamily == family) ||
                        (fontFamily == wxSWISS && family == wxDEFAULT);
#else // !GTK
            // VZ: but why elsewhere do we require an exact match? mystery...
            bool same = fontFamily == family;
#endif // GTK/!GTK

            // empty facename matches anything at all: this is bad because
            // depending on which fonts are already created, we might get back
            // a different font if we create it with empty facename, but it is
            // still better than never matching anything in the cache at all
            // in this case
            if ( same && !facename.empty() )
            {
                const wxString& fontFace = font->GetFaceName();

                // empty facename matches everything
                same = !fontFace || fontFace == facename;
            }

            if ( same && (encoding != wxFONTENCODING_DEFAULT) )
            {
                // have to match the encoding too
                same = font->GetEncoding() == encoding;
            }

            if ( same )
            {
                return font;
            }
        }
    }

    // font not found, create the new one
    font = NULL;
    wxFont fontTmp(pointSize, family, style, weight, underline, facename, encoding);
    if (fontTmp.Ok())
    {
        font = new wxFont(fontTmp);
        list.Append(font);
    }

    return font;
}

#if WXWIN_COMPATIBILITY_2_6
void wxBrushList::AddBrush(wxBrush*) { }
void wxBrushList::RemoveBrush(wxBrush*) { }
void wxFontList::AddFont(wxFont*) { }
void wxFontList::RemoveFont(wxFont*) { }
void wxPenList::AddPen(wxPen*) { }
void wxPenList::RemovePen(wxPen*) { }
#endif

wxSize wxGetDisplaySize()
{
    int x, y;
    wxDisplaySize(& x, & y);
    return wxSize(x, y);
}

wxRect wxGetClientDisplayRect()
{
    int x, y, width, height;
    wxClientDisplayRect(&x, &y, &width, &height);  // call plat-specific version
    return wxRect(x, y, width, height);
}

wxSize wxGetDisplaySizeMM()
{
    int x, y;
    wxDisplaySizeMM(& x, & y);
    return wxSize(x, y);
}

wxResourceCache::~wxResourceCache ()
{
    wxList::compatibility_iterator node = GetFirst ();
    while (node) {
        wxObject *item = (wxObject *)node->GetData();
        delete item;

        node = node->GetNext ();
    }
}
