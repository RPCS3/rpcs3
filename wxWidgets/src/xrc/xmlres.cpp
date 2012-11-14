/////////////////////////////////////////////////////////////////////////////
// Name:        src/xrc/xmlres.cpp
// Purpose:     XRC resources
// Author:      Vaclav Slavik
// Created:     2000/03/05
// RCS-ID:      $Id: xmlres.cpp 63465 2010-02-11 12:36:43Z VS $
// Copyright:   (c) 2000 Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_XRC

#include "wx/xrc/xmlres.h"

#ifndef WX_PRECOMP
    #include "wx/intl.h"
    #include "wx/log.h"
    #include "wx/panel.h"
    #include "wx/frame.h"
    #include "wx/dialog.h"
    #include "wx/settings.h"
    #include "wx/bitmap.h"
    #include "wx/image.h"
    #include "wx/module.h"
#endif

#ifndef __WXWINCE__
    #include <locale.h>
#endif

#include "wx/wfstream.h"
#include "wx/filesys.h"
#include "wx/filename.h"
#include "wx/tokenzr.h"
#include "wx/fontenum.h"
#include "wx/fontmap.h"
#include "wx/artprov.h"

#include "wx/xml/xml.h"

#include "wx/arrimpl.cpp"
WX_DEFINE_OBJARRAY(wxXmlResourceDataRecords)


wxXmlResource *wxXmlResource::ms_instance = NULL;

/*static*/ wxXmlResource *wxXmlResource::Get()
{
    if ( !ms_instance )
        ms_instance = new wxXmlResource;
    return ms_instance;
}

/*static*/ wxXmlResource *wxXmlResource::Set(wxXmlResource *res)
{
    wxXmlResource *old = ms_instance;
    ms_instance = res;
    return old;
}

wxXmlResource::wxXmlResource(int flags, const wxString& domain)
{
    m_flags = flags;
    m_version = -1;
    m_domain = NULL;
    if (! domain.empty() )
        SetDomain(domain);
}

wxXmlResource::wxXmlResource(const wxString& filemask, int flags, const wxString& domain)
{
    m_flags = flags;
    m_version = -1;
    m_domain = NULL;
    if (! domain.empty() )
        SetDomain(domain);
    Load(filemask);
}

wxXmlResource::~wxXmlResource()
{
    if (m_domain)
        free(m_domain);
    ClearHandlers();
}

void wxXmlResource::SetDomain(const wxChar* domain)
{
    if (m_domain)
        free(m_domain);
    m_domain = NULL;
    if (domain && wxStrlen(domain))
        m_domain = wxStrdup(domain);
}


/* static */
wxString wxXmlResource::ConvertFileNameToURL(const wxString& filename)
{
    wxString fnd(filename);

    // NB: as Load() and Unload() accept both filenames and URLs (should
    //     probably be changed to filenames only, but embedded resources
    //     currently rely on its ability to handle URLs - FIXME) we need to
    //     determine whether found name is filename and not URL and this is the
    //     fastest/simplest way to do it
    if (wxFileName::FileExists(fnd))
    {
        // Make the name absolute filename, because the app may
        // change working directory later:
        wxFileName fn(fnd);
        if (fn.IsRelative())
        {
            fn.MakeAbsolute();
            fnd = fn.GetFullPath();
        }
#if wxUSE_FILESYSTEM
        fnd = wxFileSystem::FileNameToURL(fnd);
#endif
    }

    return fnd;
}

#if wxUSE_FILESYSTEM

/* static */
bool wxXmlResource::IsArchive(const wxString& filename)
{
    const wxString fnd = filename.Lower();

    return fnd.Matches(wxT("*.zip")) || fnd.Matches(wxT("*.xrs"));
}

#endif // wxUSE_FILESYSTEM

bool wxXmlResource::Load(const wxString& filemask)
{
    wxString fnd;
    wxXmlResourceDataRecord *drec;
    bool iswild = wxIsWild(filemask);
    bool rt = true;

#if wxUSE_FILESYSTEM
    wxFileSystem fsys;
#   define wxXmlFindFirst  fsys.FindFirst(filemask, wxFILE)
#   define wxXmlFindNext   fsys.FindNext()
#else
#   define wxXmlFindFirst  wxFindFirstFile(filemask, wxFILE)
#   define wxXmlFindNext   wxFindNextFile()
#endif
    if (iswild)
        fnd = wxXmlFindFirst;
    else
        fnd = filemask;
    while (!fnd.empty())
    {
        fnd = ConvertFileNameToURL(fnd);

#if wxUSE_FILESYSTEM
        if ( IsArchive(fnd) )
        {
            rt = rt && Load(fnd + wxT("#zip:*.xrc"));
        }
        else // a single resource URL
#endif // wxUSE_FILESYSTEM
        {
            drec = new wxXmlResourceDataRecord;
            drec->File = fnd;
            m_data.Add(drec);
        }

        if (iswild)
            fnd = wxXmlFindNext;
        else
            fnd = wxEmptyString;
    }
#   undef wxXmlFindFirst
#   undef wxXmlFindNext
    return rt && UpdateResources();
}

bool wxXmlResource::Unload(const wxString& filename)
{
    wxASSERT_MSG( !wxIsWild(filename),
                    _T("wildcards not supported by wxXmlResource::Unload()") );

    wxString fnd = ConvertFileNameToURL(filename);
#if wxUSE_FILESYSTEM
    const bool isArchive = IsArchive(fnd);
    if ( isArchive )
        fnd += _T("#zip:");
#endif // wxUSE_FILESYSTEM

    bool unloaded = false;
    const size_t count = m_data.GetCount();
    for ( size_t i = 0; i < count; i++ )
    {
#if wxUSE_FILESYSTEM
        if ( isArchive )
        {
            if ( m_data[i].File.StartsWith(fnd) )
                unloaded = true;
            // don't break from the loop, we can have other matching files
        }
        else // a single resource URL
#endif // wxUSE_FILESYSTEM
        {
            if ( m_data[i].File == fnd )
            {
                m_data.RemoveAt(i);
                unloaded = true;

                // no sense in continuing, there is only one file with this URL
                break;
            }
        }
    }

    return unloaded;
}


IMPLEMENT_ABSTRACT_CLASS(wxXmlResourceHandler, wxObject)

void wxXmlResource::AddHandler(wxXmlResourceHandler *handler)
{
    m_handlers.Append(handler);
    handler->SetParentResource(this);
}

void wxXmlResource::InsertHandler(wxXmlResourceHandler *handler)
{
    m_handlers.Insert(handler);
    handler->SetParentResource(this);
}



void wxXmlResource::ClearHandlers()
{
    WX_CLEAR_LIST(wxList, m_handlers);
}


wxMenu *wxXmlResource::LoadMenu(const wxString& name)
{
    return (wxMenu*)CreateResFromNode(FindResource(name, wxT("wxMenu")), NULL, NULL);
}



wxMenuBar *wxXmlResource::LoadMenuBar(wxWindow *parent, const wxString& name)
{
    return (wxMenuBar*)CreateResFromNode(FindResource(name, wxT("wxMenuBar")), parent, NULL);
}



#if wxUSE_TOOLBAR
wxToolBar *wxXmlResource::LoadToolBar(wxWindow *parent, const wxString& name)
{
    return (wxToolBar*)CreateResFromNode(FindResource(name, wxT("wxToolBar")), parent, NULL);
}
#endif


wxDialog *wxXmlResource::LoadDialog(wxWindow *parent, const wxString& name)
{
    return (wxDialog*)CreateResFromNode(FindResource(name, wxT("wxDialog")), parent, NULL);
}

bool wxXmlResource::LoadDialog(wxDialog *dlg, wxWindow *parent, const wxString& name)
{
    return CreateResFromNode(FindResource(name, wxT("wxDialog")), parent, dlg) != NULL;
}



wxPanel *wxXmlResource::LoadPanel(wxWindow *parent, const wxString& name)
{
    return (wxPanel*)CreateResFromNode(FindResource(name, wxT("wxPanel")), parent, NULL);
}

bool wxXmlResource::LoadPanel(wxPanel *panel, wxWindow *parent, const wxString& name)
{
    return CreateResFromNode(FindResource(name, wxT("wxPanel")), parent, panel) != NULL;
}

wxFrame *wxXmlResource::LoadFrame(wxWindow* parent, const wxString& name)
{
    return (wxFrame*)CreateResFromNode(FindResource(name, wxT("wxFrame")), parent, NULL);
}

bool wxXmlResource::LoadFrame(wxFrame* frame, wxWindow *parent, const wxString& name)
{
    return CreateResFromNode(FindResource(name, wxT("wxFrame")), parent, frame) != NULL;
}

wxBitmap wxXmlResource::LoadBitmap(const wxString& name)
{
    wxBitmap *bmp = (wxBitmap*)CreateResFromNode(
                               FindResource(name, wxT("wxBitmap")), NULL, NULL);
    wxBitmap rt;

    if (bmp) { rt = *bmp; delete bmp; }
    return rt;
}

wxIcon wxXmlResource::LoadIcon(const wxString& name)
{
    wxIcon *icon = (wxIcon*)CreateResFromNode(
                            FindResource(name, wxT("wxIcon")), NULL, NULL);
    wxIcon rt;

    if (icon) { rt = *icon; delete icon; }
    return rt;
}


wxObject *wxXmlResource::LoadObject(wxWindow *parent, const wxString& name, const wxString& classname)
{
    return CreateResFromNode(FindResource(name, classname), parent, NULL);
}

bool wxXmlResource::LoadObject(wxObject *instance, wxWindow *parent, const wxString& name, const wxString& classname)
{
    return CreateResFromNode(FindResource(name, classname), parent, instance) != NULL;
}


bool wxXmlResource::AttachUnknownControl(const wxString& name,
                                         wxWindow *control, wxWindow *parent)
{
    if (parent == NULL)
        parent = control->GetParent();
    wxWindow *container = parent->FindWindow(name + wxT("_container"));
    if (!container)
    {
        wxLogError(_("Cannot find container for unknown control '%s'."), name.c_str());
        return false;
    }
    return control->Reparent(container);
}


static void ProcessPlatformProperty(wxXmlNode *node)
{
    wxString s;
    bool isok;

    wxXmlNode *c = node->GetChildren();
    while (c)
    {
        isok = false;
        if (!c->GetPropVal(wxT("platform"), &s))
            isok = true;
        else
        {
            wxStringTokenizer tkn(s, wxT(" |"));

            while (tkn.HasMoreTokens())
            {
                s = tkn.GetNextToken();
#ifdef __WINDOWS__
                if (s == wxT("win")) isok = true;
#endif
#if defined(__MAC__) || defined(__APPLE__)
                if (s == wxT("mac")) isok = true;
#elif defined(__UNIX__)
                if (s == wxT("unix")) isok = true;
#endif
#ifdef __OS2__
                if (s == wxT("os2")) isok = true;
#endif

                if (isok)
                    break;
            }
        }

        if (isok)
        {
            ProcessPlatformProperty(c);
            c = c->GetNext();
        }
        else
        {
            wxXmlNode *c2 = c->GetNext();
            node->RemoveChild(c);
            delete c;
            c = c2;
        }
    }
}



bool wxXmlResource::UpdateResources()
{
    bool rt = true;
    bool modif;
#   if wxUSE_FILESYSTEM
    wxFSFile *file = NULL;
    wxUnusedVar(file);
    wxFileSystem fsys;
#   endif

    wxString encoding(wxT("UTF-8"));
#if !wxUSE_UNICODE && wxUSE_INTL
    if ( (GetFlags() & wxXRC_USE_LOCALE) == 0 )
    {
        // In case we are not using wxLocale to translate strings, convert the
        // strings GUI's charset. This must not be done when wxXRC_USE_LOCALE
        // is on, because it could break wxGetTranslation lookup.
        encoding = wxLocale::GetSystemEncodingName();
    }
#endif

    for (size_t i = 0; i < m_data.GetCount(); i++)
    {
        modif = (m_data[i].Doc == NULL);

        if (!modif && !(m_flags & wxXRC_NO_RELOADING))
        {
#           if wxUSE_FILESYSTEM
            file = fsys.OpenFile(m_data[i].File);
#           if wxUSE_DATETIME
            modif = file && file->GetModificationTime() > m_data[i].Time;
#           else // wxUSE_DATETIME
            modif = true;
#           endif // wxUSE_DATETIME
            if (!file)
            {
                wxLogError(_("Cannot open file '%s'."), m_data[i].File.c_str());
                rt = false;
            }
            wxDELETE(file);
            wxUnusedVar(file);
#           else // wxUSE_FILESYSTEM
#           if wxUSE_DATETIME
            modif = wxDateTime(wxFileModificationTime(m_data[i].File)) > m_data[i].Time;
#           else // wxUSE_DATETIME
            modif = true;
#           endif // wxUSE_DATETIME
#           endif // wxUSE_FILESYSTEM
        }

        if (modif)
        {
            wxLogTrace(_T("xrc"),
                       _T("opening file '%s'"), m_data[i].File.c_str());

            wxInputStream *stream = NULL;

#           if wxUSE_FILESYSTEM
            file = fsys.OpenFile(m_data[i].File);
            if (file)
                stream = file->GetStream();
#           else
            stream = new wxFileInputStream(m_data[i].File);
#           endif

            if (stream)
            {
                delete m_data[i].Doc;
                m_data[i].Doc = new wxXmlDocument;
            }
            if (!stream || !m_data[i].Doc->Load(*stream, encoding))
            {
                wxLogError(_("Cannot load resources from file '%s'."),
                           m_data[i].File.c_str());
                wxDELETE(m_data[i].Doc);
                rt = false;
            }
            else if (m_data[i].Doc->GetRoot()->GetName() != wxT("resource"))
            {
                wxLogError(_("Invalid XRC resource '%s': doesn't have root node 'resource'."), m_data[i].File.c_str());
                wxDELETE(m_data[i].Doc);
                rt = false;
            }
            else
            {
                long version;
                int v1, v2, v3, v4;
                wxString verstr = m_data[i].Doc->GetRoot()->GetPropVal(
                                      wxT("version"), wxT("0.0.0.0"));
                if (wxSscanf(verstr.c_str(), wxT("%i.%i.%i.%i"),
                    &v1, &v2, &v3, &v4) == 4)
                    version = v1*256*256*256+v2*256*256+v3*256+v4;
                else
                    version = 0;
                if (m_version == -1)
                    m_version = version;
                if (m_version != version)
                {
                    wxLogError(_("Resource files must have same version number!"));
                    rt = false;
                }

                ProcessPlatformProperty(m_data[i].Doc->GetRoot());
#if wxUSE_DATETIME
#if wxUSE_FILESYSTEM
                m_data[i].Time = file->GetModificationTime();
#else // wxUSE_FILESYSTEM
                m_data[i].Time = wxDateTime(wxFileModificationTime(m_data[i].File));
#endif // wxUSE_FILESYSTEM
#endif // wxUSE_DATETIME
            }

#           if wxUSE_FILESYSTEM
                wxDELETE(file);
                wxUnusedVar(file);
#           else
                wxDELETE(stream);
#           endif
        }
    }

    return rt;
}


wxXmlNode *wxXmlResource::DoFindResource(wxXmlNode *parent,
                                         const wxString& name,
                                         const wxString& classname,
                                         bool recursive)
{
    wxString dummy;
    wxXmlNode *node;

    // first search for match at the top-level nodes (as this is
    // where the resource is most commonly looked for):
    for (node = parent->GetChildren(); node; node = node->GetNext())
    {
        if ( node->GetType() == wxXML_ELEMENT_NODE &&
                 (node->GetName() == wxT("object") ||
                  node->GetName() == wxT("object_ref")) &&
             node->GetPropVal(wxT("name"), &dummy) && dummy == name )
        {
            wxString cls(node->GetPropVal(wxT("class"), wxEmptyString));
            if (!classname || cls == classname)
                return node;
            // object_ref may not have 'class' property:
            if (cls.empty() && node->GetName() == wxT("object_ref"))
            {
                wxString refName = node->GetPropVal(wxT("ref"), wxEmptyString);
                if (refName.empty())
                    continue;
                wxXmlNode* refNode = FindResource(refName, wxEmptyString, true);
                if (refNode &&
                    refNode->GetPropVal(wxT("class"), wxEmptyString) == classname)
                {
                    return node;
                }
            }
        }
    }

    if ( recursive )
        for (node = parent->GetChildren(); node; node = node->GetNext())
        {
            if ( node->GetType() == wxXML_ELEMENT_NODE &&
                 (node->GetName() == wxT("object") ||
                  node->GetName() == wxT("object_ref")) )
            {
                wxXmlNode* found = DoFindResource(node, name, classname, true);
                if ( found )
                    return found;
            }
        }

   return NULL;
}

wxXmlNode *wxXmlResource::FindResource(const wxString& name,
                                       const wxString& classname,
                                       bool recursive)
{
    UpdateResources(); //ensure everything is up-to-date

    wxString dummy;
    for (size_t f = 0; f < m_data.GetCount(); f++)
    {
        if ( m_data[f].Doc == NULL || m_data[f].Doc->GetRoot() == NULL )
            continue;

        wxXmlNode* found = DoFindResource(m_data[f].Doc->GetRoot(),
                                          name, classname, recursive);
        if ( found )
        {
#if wxUSE_FILESYSTEM
            m_curFileSystem.ChangePathTo(m_data[f].File);
#endif
            return found;
        }
    }

    wxLogError(_("XRC resource '%s' (class '%s') not found!"),
               name.c_str(), classname.c_str());
    return NULL;
}

static void MergeNodes(wxXmlNode& dest, wxXmlNode& with)
{
    // Merge properties:
    for (wxXmlProperty *prop = with.GetProperties(); prop; prop = prop->GetNext())
    {
        wxXmlProperty *dprop;
        for (dprop = dest.GetProperties(); dprop; dprop = dprop->GetNext())
        {

            if ( dprop->GetName() == prop->GetName() )
            {
                dprop->SetValue(prop->GetValue());
                break;
            }
        }

        if ( !dprop )
            dest.AddProperty(prop->GetName(), prop->GetValue());
   }

    // Merge child nodes:
    for (wxXmlNode* node = with.GetChildren(); node; node = node->GetNext())
    {
        wxString name = node->GetPropVal(wxT("name"), wxEmptyString);
        wxXmlNode *dnode;

        for (dnode = dest.GetChildren(); dnode; dnode = dnode->GetNext() )
        {
            if ( dnode->GetName() == node->GetName() &&
                 dnode->GetPropVal(wxT("name"), wxEmptyString) == name &&
                 dnode->GetType() == node->GetType() )
            {
                MergeNodes(*dnode, *node);
                break;
            }
        }

        if ( !dnode )
        {
            static const wxChar *AT_END = wxT("end");
            wxString insert_pos = node->GetPropVal(wxT("insert_at"), AT_END);
            if ( insert_pos == AT_END )
            {
                dest.AddChild(new wxXmlNode(*node));
            }
            else if ( insert_pos == wxT("begin") )
            {
                dest.InsertChild(new wxXmlNode(*node), dest.GetChildren());
            }
        }
    }

    if ( dest.GetType() == wxXML_TEXT_NODE && with.GetContent().length() )
         dest.SetContent(with.GetContent());
}

wxObject *wxXmlResource::CreateResFromNode(wxXmlNode *node, wxObject *parent,
                                           wxObject *instance,
                                           wxXmlResourceHandler *handlerToUse)
{
    if (node == NULL) return NULL;

    // handling of referenced resource
    if ( node->GetName() == wxT("object_ref") )
    {
        wxString refName = node->GetPropVal(wxT("ref"), wxEmptyString);
        wxXmlNode* refNode = FindResource(refName, wxEmptyString, true);

        if ( !refNode )
        {
            wxLogError(_("Referenced object node with ref=\"%s\" not found!"),
                       refName.c_str());
            return NULL;
        }

        wxXmlNode copy(*refNode);
        MergeNodes(copy, *node);

        return CreateResFromNode(&copy, parent, instance);
    }

    wxXmlResourceHandler *handler;

    if (handlerToUse)
    {
        if (handlerToUse->CanHandle(node))
        {
            return handlerToUse->CreateResource(node, parent, instance);
        }
    }
    else if (node->GetName() == wxT("object"))
    {
        wxList::compatibility_iterator ND = m_handlers.GetFirst();
        while (ND)
        {
            handler = (wxXmlResourceHandler*)ND->GetData();
            if (handler->CanHandle(node))
            {
                return handler->CreateResource(node, parent, instance);
            }
            ND = ND->GetNext();
        }
    }

    wxLogError(_("No handler found for XML node '%s', class '%s'!"),
               node->GetName().c_str(),
               node->GetPropVal(wxT("class"), wxEmptyString).c_str());
    return NULL;
}


#include "wx/listimpl.cpp"
WX_DECLARE_LIST(wxXmlSubclassFactory, wxXmlSubclassFactoriesList);
WX_DEFINE_LIST(wxXmlSubclassFactoriesList)

wxXmlSubclassFactoriesList *wxXmlResource::ms_subclassFactories = NULL;

/*static*/ void wxXmlResource::AddSubclassFactory(wxXmlSubclassFactory *factory)
{
    if (!ms_subclassFactories)
    {
        ms_subclassFactories = new wxXmlSubclassFactoriesList;
    }
    ms_subclassFactories->Append(factory);
}

class wxXmlSubclassFactoryCXX : public wxXmlSubclassFactory
{
public:
    ~wxXmlSubclassFactoryCXX() {}

    wxObject *Create(const wxString& className)
    {
        wxClassInfo* classInfo = wxClassInfo::FindClass(className);

        if (classInfo)
            return classInfo->CreateObject();
        else
            return NULL;
    }
};




wxXmlResourceHandler::wxXmlResourceHandler()
        : m_node(NULL), m_parent(NULL), m_instance(NULL),
          m_parentAsWindow(NULL)
{}



wxObject *wxXmlResourceHandler::CreateResource(wxXmlNode *node, wxObject *parent, wxObject *instance)
{
    wxXmlNode *myNode = m_node;
    wxString myClass = m_class;
    wxObject *myParent = m_parent, *myInstance = m_instance;
    wxWindow *myParentAW = m_parentAsWindow;

    m_instance = instance;
    if (!m_instance && node->HasProp(wxT("subclass")) &&
        !(m_resource->GetFlags() & wxXRC_NO_SUBCLASSING))
    {
        wxString subclass = node->GetPropVal(wxT("subclass"), wxEmptyString);
        if (!subclass.empty())
        {
            for (wxXmlSubclassFactoriesList::compatibility_iterator i = wxXmlResource::ms_subclassFactories->GetFirst();
                 i; i = i->GetNext())
            {
                m_instance = i->GetData()->Create(subclass);
                if (m_instance)
                    break;
            }

            if (!m_instance)
            {
                wxString name = node->GetPropVal(wxT("name"), wxEmptyString);
                wxLogError(_("Subclass '%s' not found for resource '%s', not subclassing!"),
                           subclass.c_str(), name.c_str());
            }
        }
    }

    m_node = node;
    m_class = node->GetPropVal(wxT("class"), wxEmptyString);
    m_parent = parent;
    m_parentAsWindow = wxDynamicCast(m_parent, wxWindow);

    wxObject *returned = DoCreateResource();

    m_node = myNode;
    m_class = myClass;
    m_parent = myParent; m_parentAsWindow = myParentAW;
    m_instance = myInstance;

    return returned;
}


void wxXmlResourceHandler::AddStyle(const wxString& name, int value)
{
    m_styleNames.Add(name);
    m_styleValues.Add(value);
}



void wxXmlResourceHandler::AddWindowStyles()
{
    XRC_ADD_STYLE(wxCLIP_CHILDREN);

    // the border styles all have the old and new names, recognize both for now
    XRC_ADD_STYLE(wxSIMPLE_BORDER); XRC_ADD_STYLE(wxBORDER_SIMPLE);
    XRC_ADD_STYLE(wxSUNKEN_BORDER); XRC_ADD_STYLE(wxBORDER_SUNKEN);
    XRC_ADD_STYLE(wxDOUBLE_BORDER); XRC_ADD_STYLE(wxBORDER_DOUBLE); // deprecated
    XRC_ADD_STYLE(wxBORDER_THEME);
    XRC_ADD_STYLE(wxRAISED_BORDER); XRC_ADD_STYLE(wxBORDER_RAISED);
    XRC_ADD_STYLE(wxSTATIC_BORDER); XRC_ADD_STYLE(wxBORDER_STATIC);
    XRC_ADD_STYLE(wxNO_BORDER);     XRC_ADD_STYLE(wxBORDER_NONE);

    XRC_ADD_STYLE(wxTRANSPARENT_WINDOW);
    XRC_ADD_STYLE(wxWANTS_CHARS);
    XRC_ADD_STYLE(wxTAB_TRAVERSAL);
    XRC_ADD_STYLE(wxNO_FULL_REPAINT_ON_RESIZE);
    XRC_ADD_STYLE(wxFULL_REPAINT_ON_RESIZE);
    XRC_ADD_STYLE(wxALWAYS_SHOW_SB);
    XRC_ADD_STYLE(wxWS_EX_BLOCK_EVENTS);
    XRC_ADD_STYLE(wxWS_EX_VALIDATE_RECURSIVELY);
}



bool wxXmlResourceHandler::HasParam(const wxString& param)
{
    return (GetParamNode(param) != NULL);
}


int wxXmlResourceHandler::GetStyle(const wxString& param, int defaults)
{
    wxString s = GetParamValue(param);

    if (!s) return defaults;

    wxStringTokenizer tkn(s, wxT("| \t\n"), wxTOKEN_STRTOK);
    int style = 0;
    int index;
    wxString fl;
    while (tkn.HasMoreTokens())
    {
        fl = tkn.GetNextToken();
        index = m_styleNames.Index(fl);
        if (index != wxNOT_FOUND)
            style |= m_styleValues[index];
        else
            wxLogError(_("Unknown style flag ") + fl);
    }
    return style;
}



wxString wxXmlResourceHandler::GetText(const wxString& param, bool translate)
{
    wxXmlNode *parNode = GetParamNode(param);
    wxString str1(GetNodeContent(parNode));
    wxString str2;
    const wxChar *dt;
    wxChar amp_char;

    // VS: First version of XRC resources used $ instead of & (which is
    //     illegal in XML), but later I realized that '_' fits this purpose
    //     much better (because &File means "File with F underlined").
    if (m_resource->CompareVersion(2,3,0,1) < 0)
        amp_char = wxT('$');
    else
        amp_char = wxT('_');

    for (dt = str1.c_str(); *dt; dt++)
    {
        // Remap amp_char to &, map double amp_char to amp_char (for things
        // like "&File..." -- this is illegal in XML, so we use "_File..."):
        if (*dt == amp_char)
        {
            if ( *(++dt) == amp_char )
                str2 << amp_char;
            else
                str2 << wxT('&') << *dt;
        }
        // Remap \n to CR, \r to LF, \t to TAB, \\ to \:
        else if (*dt == wxT('\\'))
            switch (*(++dt))
            {
                case wxT('n'):
                    str2 << wxT('\n');
                    break;

                case wxT('t'):
                    str2 << wxT('\t');
                    break;

                case wxT('r'):
                    str2 << wxT('\r');
                    break;

                case wxT('\\') :
                    // "\\" wasn't translated to "\" prior to 2.5.3.0:
                    if (m_resource->CompareVersion(2,5,3,0) >= 0)
                    {
                        str2 << wxT('\\');
                        break;
                    }
                    // else fall-through to default: branch below

                default:
                    str2 << wxT('\\') << *dt;
                    break;
            }
        else str2 << *dt;
    }

    if (m_resource->GetFlags() & wxXRC_USE_LOCALE)
    {
        if (translate && parNode &&
            parNode->GetPropVal(wxT("translate"), wxEmptyString) != wxT("0"))
        {
            return wxGetTranslation(str2, m_resource->GetDomain());
        }
        else
        {
#if wxUSE_UNICODE
            return str2;
#else
            // The string is internally stored as UTF-8, we have to convert
            // it into system's default encoding so that it can be displayed:
            return wxString(str2.wc_str(wxConvUTF8), wxConvLocal);
#endif
        }
    }

    // If wxXRC_USE_LOCALE is not set, then the string is already in
    // system's default encoding in ANSI build, so we don't have to
    // do anything special here.
    return str2;
}



long wxXmlResourceHandler::GetLong(const wxString& param, long defaultv)
{
    long value;
    wxString str1 = GetParamValue(param);

    if (!str1.ToLong(&value))
        value = defaultv;

    return value;
}

float wxXmlResourceHandler::GetFloat(const wxString& param, float defaultv)
{
    wxString str = GetParamValue(param);

#if wxUSE_INTL
    // strings in XRC always use C locale but wxString::ToDouble() uses the
    // current one, so transform the string to it supposing that the only
    // difference between them is the decimal separator
    str.Replace(wxT("."), wxLocale::GetInfo(wxLOCALE_DECIMAL_POINT,
                                            wxLOCALE_CAT_NUMBER));
#endif // wxUSE_INTL

    double value;
    if (!str.ToDouble(&value))
        value = defaultv;

    return wx_truncate_cast(float, value);
}


int wxXmlResourceHandler::GetID()
{
    return wxXmlResource::GetXRCID(GetName());
}



wxString wxXmlResourceHandler::GetName()
{
    return m_node->GetPropVal(wxT("name"), wxT("-1"));
}



bool wxXmlResourceHandler::GetBool(const wxString& param, bool defaultv)
{
    wxString v = GetParamValue(param);
    v.MakeLower();
    if (!v) return defaultv;

    return (v == wxT("1"));
}


static wxColour GetSystemColour(const wxString& name)
{
    if (!name.empty())
    {
        #define SYSCLR(clr) \
            if (name == _T(#clr)) return wxSystemSettings::GetColour(clr);
        SYSCLR(wxSYS_COLOUR_SCROLLBAR)
        SYSCLR(wxSYS_COLOUR_BACKGROUND)
        SYSCLR(wxSYS_COLOUR_DESKTOP)
        SYSCLR(wxSYS_COLOUR_ACTIVECAPTION)
        SYSCLR(wxSYS_COLOUR_INACTIVECAPTION)
        SYSCLR(wxSYS_COLOUR_MENU)
        SYSCLR(wxSYS_COLOUR_WINDOW)
        SYSCLR(wxSYS_COLOUR_WINDOWFRAME)
        SYSCLR(wxSYS_COLOUR_MENUTEXT)
        SYSCLR(wxSYS_COLOUR_WINDOWTEXT)
        SYSCLR(wxSYS_COLOUR_CAPTIONTEXT)
        SYSCLR(wxSYS_COLOUR_ACTIVEBORDER)
        SYSCLR(wxSYS_COLOUR_INACTIVEBORDER)
        SYSCLR(wxSYS_COLOUR_APPWORKSPACE)
        SYSCLR(wxSYS_COLOUR_HIGHLIGHT)
        SYSCLR(wxSYS_COLOUR_HIGHLIGHTTEXT)
        SYSCLR(wxSYS_COLOUR_BTNFACE)
        SYSCLR(wxSYS_COLOUR_3DFACE)
        SYSCLR(wxSYS_COLOUR_BTNSHADOW)
        SYSCLR(wxSYS_COLOUR_3DSHADOW)
        SYSCLR(wxSYS_COLOUR_GRAYTEXT)
        SYSCLR(wxSYS_COLOUR_BTNTEXT)
        SYSCLR(wxSYS_COLOUR_INACTIVECAPTIONTEXT)
        SYSCLR(wxSYS_COLOUR_BTNHIGHLIGHT)
        SYSCLR(wxSYS_COLOUR_BTNHILIGHT)
        SYSCLR(wxSYS_COLOUR_3DHIGHLIGHT)
        SYSCLR(wxSYS_COLOUR_3DHILIGHT)
        SYSCLR(wxSYS_COLOUR_3DDKSHADOW)
        SYSCLR(wxSYS_COLOUR_3DLIGHT)
        SYSCLR(wxSYS_COLOUR_INFOTEXT)
        SYSCLR(wxSYS_COLOUR_INFOBK)
        SYSCLR(wxSYS_COLOUR_LISTBOX)
        SYSCLR(wxSYS_COLOUR_HOTLIGHT)
        SYSCLR(wxSYS_COLOUR_GRADIENTACTIVECAPTION)
        SYSCLR(wxSYS_COLOUR_GRADIENTINACTIVECAPTION)
        SYSCLR(wxSYS_COLOUR_MENUHILIGHT)
        SYSCLR(wxSYS_COLOUR_MENUBAR)
        #undef SYSCLR
    }

    return wxNullColour;
}

wxColour wxXmlResourceHandler::GetColour(const wxString& param, const wxColour& defaultv)
{
    wxString v = GetParamValue(param);

    if ( v.empty() )
        return defaultv;

    wxColour clr;

    // wxString -> wxColour conversion
    if (!clr.Set(v))
    {
        // the colour doesn't use #RRGGBB format, check if it is symbolic
        // colour name:
        clr = GetSystemColour(v);
        if (clr.Ok())
            return clr;

        wxLogError(_("XRC resource: Incorrect colour specification '%s' for property '%s'."),
                   v.c_str(), param.c_str());
        return wxNullColour;
    }

    return clr;
}



wxBitmap wxXmlResourceHandler::GetBitmap(const wxString& param,
                                         const wxArtClient& defaultArtClient,
                                         wxSize size)
{
    /* If the bitmap is specified as stock item, query wxArtProvider for it: */
    wxXmlNode *bmpNode = GetParamNode(param);
    if ( bmpNode )
    {
        wxString sid = bmpNode->GetPropVal(wxT("stock_id"), wxEmptyString);
        if ( !sid.empty() )
        {
            wxString scl = bmpNode->GetPropVal(wxT("stock_client"), wxEmptyString);
            if (scl.empty())
                scl = defaultArtClient;
            else
                scl = wxART_MAKE_CLIENT_ID_FROM_STR(scl);

            wxBitmap stockArt =
                wxArtProvider::GetBitmap(wxART_MAKE_ART_ID_FROM_STR(sid),
                                         scl, size);
            if ( stockArt.Ok() )
                return stockArt;
        }
    }

    /* ...or load the bitmap from file: */
    wxString name = GetParamValue(param);
    if (name.empty()) return wxNullBitmap;
#if wxUSE_FILESYSTEM
    wxFSFile *fsfile = GetCurFileSystem().OpenFile(name, wxFS_READ | wxFS_SEEKABLE);
    if (fsfile == NULL)
    {
        wxLogError(_("XRC resource: Cannot create bitmap from '%s'."),
                   name.c_str());
        return wxNullBitmap;
    }
    wxImage img(*(fsfile->GetStream()));
    delete fsfile;
#else
    wxImage img(name);
#endif

    if (!img.Ok())
    {
        wxLogError(_("XRC resource: Cannot create bitmap from '%s'."),
                   name.c_str());
        return wxNullBitmap;
    }
    if (!(size == wxDefaultSize)) img.Rescale(size.x, size.y);
    return wxBitmap(img);
}

#if wxUSE_ANIMATIONCTRL
wxAnimation wxXmlResourceHandler::GetAnimation(const wxString& param)
{
    wxAnimation ani;

    /* load the animation from file: */
    wxString name = GetParamValue(param);
    if (name.empty()) return wxNullAnimation;
#if wxUSE_FILESYSTEM
    wxFSFile *fsfile = GetCurFileSystem().OpenFile(name, wxFS_READ | wxFS_SEEKABLE);
    if (fsfile == NULL)
    {
        wxLogError(_("XRC resource: Cannot create animation from '%s'."),
                   name.c_str());
        return wxNullAnimation;
    }
    ani.Load(*(fsfile->GetStream()));
    delete fsfile;
#else
    ani.LoadFile(name);
#endif

    if (!ani.IsOk())
    {
        wxLogError(_("XRC resource: Cannot create animation from '%s'."),
                   name.c_str());
        return wxNullAnimation;
    }

    return ani;
}
#endif      // wxUSE_ANIMATIONCTRL



wxIcon wxXmlResourceHandler::GetIcon(const wxString& param,
                                     const wxArtClient& defaultArtClient,
                                     wxSize size)
{
    wxIcon icon;
    icon.CopyFromBitmap(GetBitmap(param, defaultArtClient, size));
    return icon;
}



wxXmlNode *wxXmlResourceHandler::GetParamNode(const wxString& param)
{
    wxCHECK_MSG(m_node, NULL, wxT("You can't access handler data before it was initialized!"));

    wxXmlNode *n = m_node->GetChildren();

    while (n)
    {
        if (n->GetType() == wxXML_ELEMENT_NODE && n->GetName() == param)
            return n;
        n = n->GetNext();
    }
    return NULL;
}



bool wxXmlResourceHandler::IsOfClass(wxXmlNode *node, const wxString& classname)
{
    return node->GetPropVal(wxT("class"), wxEmptyString) == classname;
}



wxString wxXmlResourceHandler::GetNodeContent(wxXmlNode *node)
{
    wxXmlNode *n = node;
    if (n == NULL) return wxEmptyString;
    n = n->GetChildren();

    while (n)
    {
        if (n->GetType() == wxXML_TEXT_NODE ||
            n->GetType() == wxXML_CDATA_SECTION_NODE)
            return n->GetContent();
        n = n->GetNext();
    }
    return wxEmptyString;
}



wxString wxXmlResourceHandler::GetParamValue(const wxString& param)
{
    if (param.empty())
        return GetNodeContent(m_node);
    else
        return GetNodeContent(GetParamNode(param));
}



wxSize wxXmlResourceHandler::GetSize(const wxString& param,
                                     wxWindow *windowToUse)
{
    wxString s = GetParamValue(param);
    if (s.empty()) s = wxT("-1,-1");
    bool is_dlg;
    long sx, sy = 0;

    is_dlg = s[s.length()-1] == wxT('d');
    if (is_dlg) s.RemoveLast();

    if (!s.BeforeFirst(wxT(',')).ToLong(&sx) ||
        !s.AfterLast(wxT(',')).ToLong(&sy))
    {
        wxLogError(_("Cannot parse coordinates from '%s'."), s.c_str());
        return wxDefaultSize;
    }

    if (is_dlg)
    {
        if (windowToUse)
        {
            return wxDLG_UNIT(windowToUse, wxSize(sx, sy));
        }
        else if (m_parentAsWindow)
        {
            return wxDLG_UNIT(m_parentAsWindow, wxSize(sx, sy));
        }
        else
        {
            wxLogError(_("Cannot convert dialog units: dialog unknown."));
            return wxDefaultSize;
        }
    }

    return wxSize(sx, sy);
}



wxPoint wxXmlResourceHandler::GetPosition(const wxString& param)
{
    wxSize sz = GetSize(param);
    return wxPoint(sz.x, sz.y);
}



wxCoord wxXmlResourceHandler::GetDimension(const wxString& param,
                                           wxCoord defaultv,
                                           wxWindow *windowToUse)
{
    wxString s = GetParamValue(param);
    if (s.empty()) return defaultv;
    bool is_dlg;
    long sx;

    is_dlg = s[s.length()-1] == wxT('d');
    if (is_dlg) s.RemoveLast();

    if (!s.ToLong(&sx))
    {
        wxLogError(_("Cannot parse dimension from '%s'."), s.c_str());
        return defaultv;
    }

    if (is_dlg)
    {
        if (windowToUse)
        {
            return wxDLG_UNIT(windowToUse, wxSize(sx, 0)).x;
        }
        else if (m_parentAsWindow)
        {
            return wxDLG_UNIT(m_parentAsWindow, wxSize(sx, 0)).x;
        }
        else
        {
            wxLogError(_("Cannot convert dialog units: dialog unknown."));
            return defaultv;
        }
    }

    return sx;
}


// Get system font index using indexname
static wxFont GetSystemFont(const wxString& name)
{
    if (!name.empty())
    {
        #define SYSFNT(fnt) \
            if (name == _T(#fnt)) return wxSystemSettings::GetFont(fnt);
        SYSFNT(wxSYS_OEM_FIXED_FONT)
        SYSFNT(wxSYS_ANSI_FIXED_FONT)
        SYSFNT(wxSYS_ANSI_VAR_FONT)
        SYSFNT(wxSYS_SYSTEM_FONT)
        SYSFNT(wxSYS_DEVICE_DEFAULT_FONT)
        SYSFNT(wxSYS_DEFAULT_PALETTE)
        SYSFNT(wxSYS_SYSTEM_FIXED_FONT)
        SYSFNT(wxSYS_DEFAULT_GUI_FONT)
        #undef SYSFNT
    }

    return wxNullFont;
}

wxFont wxXmlResourceHandler::GetFont(const wxString& param)
{
    wxXmlNode *font_node = GetParamNode(param);
    if (font_node == NULL)
    {
        wxLogError(_("Cannot find font node '%s'."), param.c_str());
        return wxNullFont;
    }

    wxXmlNode *oldnode = m_node;
    m_node = font_node;

    // font attributes:

    // size
    int isize = -1;
    bool hasSize = HasParam(wxT("size"));
    if (hasSize)
        isize = GetLong(wxT("size"), -1);

    // style
    int istyle = wxNORMAL;
    bool hasStyle = HasParam(wxT("style"));
    if (hasStyle)
    {
        wxString style = GetParamValue(wxT("style"));
        if (style == wxT("italic"))
            istyle = wxITALIC;
        else if (style == wxT("slant"))
            istyle = wxSLANT;
    }

    // weight
    int iweight = wxNORMAL;
    bool hasWeight = HasParam(wxT("weight"));
    if (hasWeight)
    {
        wxString weight = GetParamValue(wxT("weight"));
        if (weight == wxT("bold"))
            iweight = wxBOLD;
        else if (weight == wxT("light"))
            iweight = wxLIGHT;
    }

    // underline
    bool hasUnderlined = HasParam(wxT("underlined"));
    bool underlined = hasUnderlined ? GetBool(wxT("underlined"), false) : false;

    // family and facename
    int ifamily = wxDEFAULT;
    bool hasFamily = HasParam(wxT("family"));
    if (hasFamily)
    {
        wxString family = GetParamValue(wxT("family"));
             if (family == wxT("decorative")) ifamily = wxDECORATIVE;
        else if (family == wxT("roman")) ifamily = wxROMAN;
        else if (family == wxT("script")) ifamily = wxSCRIPT;
        else if (family == wxT("swiss")) ifamily = wxSWISS;
        else if (family == wxT("modern")) ifamily = wxMODERN;
        else if (family == wxT("teletype")) ifamily = wxTELETYPE;
    }


    wxString facename;
    bool hasFacename = HasParam(wxT("face"));
    if (hasFacename)
    {
        wxString faces = GetParamValue(wxT("face"));
        wxArrayString facenames(wxFontEnumerator::GetFacenames());
        wxStringTokenizer tk(faces, wxT(","));
        while (tk.HasMoreTokens())
        {
            int index = facenames.Index(tk.GetNextToken(), false);
            if (index != wxNOT_FOUND)
            {
                facename = facenames[index];
                break;
            }
        }
    }

    // encoding
    wxFontEncoding enc = wxFONTENCODING_DEFAULT;
    bool hasEncoding = HasParam(wxT("encoding"));
    if (hasEncoding)
    {
        wxString encoding = GetParamValue(wxT("encoding"));
        wxFontMapper mapper;
        if (!encoding.empty())
            enc = mapper.CharsetToEncoding(encoding);
        if (enc == wxFONTENCODING_SYSTEM)
            enc = wxFONTENCODING_DEFAULT;
    }

    // is this font based on a system font?
    wxFont font = GetSystemFont(GetParamValue(wxT("sysfont")));

    if (font.Ok())
    {
        if (hasSize && isize != -1)
            font.SetPointSize(isize);
        else if (HasParam(wxT("relativesize")))
            font.SetPointSize(int(font.GetPointSize() *
                                     GetFloat(wxT("relativesize"))));

        if (hasStyle)
            font.SetStyle(istyle);
        if (hasWeight)
            font.SetWeight(iweight);
        if (hasUnderlined)
            font.SetUnderlined(underlined);
        if (hasFamily)
            font.SetFamily(ifamily);
        if (hasFacename)
            font.SetFaceName(facename);
        if (hasEncoding)
            font.SetDefaultEncoding(enc);
    }
    else // not based on system font
    {
        font = wxFont(isize == -1 ? wxNORMAL_FONT->GetPointSize() : isize,
                      ifamily, istyle, iweight,
                      underlined, facename, enc);
    }

    m_node = oldnode;
    return font;
}


void wxXmlResourceHandler::SetupWindow(wxWindow *wnd)
{
    //FIXME : add cursor

    if (HasParam(wxT("exstyle")))
        // Have to OR it with existing style, since
        // some implementations (e.g. wxGTK) use the extra style
        // during creation
        wnd->SetExtraStyle(wnd->GetExtraStyle() | GetStyle(wxT("exstyle")));
    if (HasParam(wxT("bg")))
        wnd->SetBackgroundColour(GetColour(wxT("bg")));
    if (HasParam(wxT("fg")))
        wnd->SetForegroundColour(GetColour(wxT("fg")));
    if (GetBool(wxT("enabled"), 1) == 0)
        wnd->Enable(false);
    if (GetBool(wxT("focused"), 0) == 1)
        wnd->SetFocus();
    if (GetBool(wxT("hidden"), 0) == 1)
        wnd->Show(false);
#if wxUSE_TOOLTIPS
    if (HasParam(wxT("tooltip")))
        wnd->SetToolTip(GetText(wxT("tooltip")));
#endif
    if (HasParam(wxT("font")))
        wnd->SetFont(GetFont());
    if (HasParam(wxT("help")))
        wnd->SetHelpText(GetText(wxT("help")));
}


void wxXmlResourceHandler::CreateChildren(wxObject *parent, bool this_hnd_only)
{
    wxXmlNode *n = m_node->GetChildren();

    while (n)
    {
        if (n->GetType() == wxXML_ELEMENT_NODE &&
           (n->GetName() == wxT("object") || n->GetName() == wxT("object_ref")))
        {
            m_resource->CreateResFromNode(n, parent, NULL,
                                          this_hnd_only ? this : NULL);
        }
        n = n->GetNext();
    }
}


void wxXmlResourceHandler::CreateChildrenPrivately(wxObject *parent, wxXmlNode *rootnode)
{
    wxXmlNode *root;
    if (rootnode == NULL) root = m_node; else root = rootnode;
    wxXmlNode *n = root->GetChildren();

    while (n)
    {
        if (n->GetType() == wxXML_ELEMENT_NODE && CanHandle(n))
        {
            CreateResource(n, parent, NULL);
        }
        n = n->GetNext();
    }
}







// --------------- XRCID implementation -----------------------------

#define XRCID_TABLE_SIZE     1024


struct XRCID_record
{
    int id;
    wxChar *key;
    XRCID_record *next;
};

static XRCID_record *XRCID_Records[XRCID_TABLE_SIZE] = {NULL};

static int XRCID_Lookup(const wxChar *str_id, int value_if_not_found = wxID_NONE)
{
    unsigned int index = 0;

    for (const wxChar *c = str_id; *c != wxT('\0'); c++) index += (unsigned int)*c;
    index %= XRCID_TABLE_SIZE;

    XRCID_record *oldrec = NULL;
    for (XRCID_record *rec = XRCID_Records[index]; rec; rec = rec->next)
    {
        if (wxStrcmp(rec->key, str_id) == 0)
        {
            return rec->id;
        }
        oldrec = rec;
    }

    XRCID_record **rec_var = (oldrec == NULL) ?
                              &XRCID_Records[index] : &oldrec->next;
    *rec_var = new XRCID_record;
    (*rec_var)->key = wxStrdup(str_id);
    (*rec_var)->next = NULL;

    wxChar *end;
    if (value_if_not_found != wxID_NONE)
        (*rec_var)->id = value_if_not_found;
    else
    {
        int asint = wxStrtol(str_id, &end, 10);
        if (*str_id && *end == 0)
        {
            // if str_id was integer, keep it verbosely:
            (*rec_var)->id = asint;
        }
        else
        {
            (*rec_var)->id = wxNewId();
        }
    }

    return (*rec_var)->id;
}

static void AddStdXRCID_Records();

/*static*/
int wxXmlResource::GetXRCID(const wxChar *str_id, int value_if_not_found)
{
    static bool s_stdIDsAdded = false;

    if ( !s_stdIDsAdded )
    {
        s_stdIDsAdded = true;
        AddStdXRCID_Records();
    }

    return XRCID_Lookup(str_id, value_if_not_found);
}


static void CleanXRCID_Record(XRCID_record *rec)
{
    if (rec)
    {
        CleanXRCID_Record(rec->next);
        free(rec->key);
        delete rec;
    }
}

static void CleanXRCID_Records()
{
    for (int i = 0; i < XRCID_TABLE_SIZE; i++)
    {
        CleanXRCID_Record(XRCID_Records[i]);
        XRCID_Records[i] = NULL;
    }
}

static void AddStdXRCID_Records()
{
#define stdID(id) XRCID_Lookup(wxT(#id), id)
    stdID(-1);

    stdID(wxID_ANY);
    stdID(wxID_SEPARATOR);

    stdID(wxID_OPEN);
    stdID(wxID_CLOSE);
    stdID(wxID_NEW);
    stdID(wxID_SAVE);
    stdID(wxID_SAVEAS);
    stdID(wxID_REVERT);
    stdID(wxID_EXIT);
    stdID(wxID_UNDO);
    stdID(wxID_REDO);
    stdID(wxID_HELP);
    stdID(wxID_PRINT);
    stdID(wxID_PRINT_SETUP);
    stdID(wxID_PAGE_SETUP);
    stdID(wxID_PREVIEW);
    stdID(wxID_ABOUT);
    stdID(wxID_HELP_CONTENTS);
    stdID(wxID_HELP_COMMANDS);
    stdID(wxID_HELP_PROCEDURES);
    stdID(wxID_HELP_CONTEXT);
    stdID(wxID_CLOSE_ALL);
    stdID(wxID_PREFERENCES);
    stdID(wxID_EDIT);
    stdID(wxID_CUT);
    stdID(wxID_COPY);
    stdID(wxID_PASTE);
    stdID(wxID_CLEAR);
    stdID(wxID_FIND);
    stdID(wxID_DUPLICATE);
    stdID(wxID_SELECTALL);
    stdID(wxID_DELETE);
    stdID(wxID_REPLACE);
    stdID(wxID_REPLACE_ALL);
    stdID(wxID_PROPERTIES);
    stdID(wxID_VIEW_DETAILS);
    stdID(wxID_VIEW_LARGEICONS);
    stdID(wxID_VIEW_SMALLICONS);
    stdID(wxID_VIEW_LIST);
    stdID(wxID_VIEW_SORTDATE);
    stdID(wxID_VIEW_SORTNAME);
    stdID(wxID_VIEW_SORTSIZE);
    stdID(wxID_VIEW_SORTTYPE);
    stdID(wxID_FILE1);
    stdID(wxID_FILE2);
    stdID(wxID_FILE3);
    stdID(wxID_FILE4);
    stdID(wxID_FILE5);
    stdID(wxID_FILE6);
    stdID(wxID_FILE7);
    stdID(wxID_FILE8);
    stdID(wxID_FILE9);
    stdID(wxID_OK);
    stdID(wxID_CANCEL);
    stdID(wxID_APPLY);
    stdID(wxID_YES);
    stdID(wxID_NO);
    stdID(wxID_STATIC);
    stdID(wxID_FORWARD);
    stdID(wxID_BACKWARD);
    stdID(wxID_DEFAULT);
    stdID(wxID_MORE);
    stdID(wxID_SETUP);
    stdID(wxID_RESET);
    stdID(wxID_CONTEXT_HELP);
    stdID(wxID_YESTOALL);
    stdID(wxID_NOTOALL);
    stdID(wxID_ABORT);
    stdID(wxID_RETRY);
    stdID(wxID_IGNORE);
    stdID(wxID_ADD);
    stdID(wxID_REMOVE);
    stdID(wxID_UP);
    stdID(wxID_DOWN);
    stdID(wxID_HOME);
    stdID(wxID_REFRESH);
    stdID(wxID_STOP);
    stdID(wxID_INDEX);
    stdID(wxID_BOLD);
    stdID(wxID_ITALIC);
    stdID(wxID_JUSTIFY_CENTER);
    stdID(wxID_JUSTIFY_FILL);
    stdID(wxID_JUSTIFY_RIGHT);
    stdID(wxID_JUSTIFY_LEFT);
    stdID(wxID_UNDERLINE);
    stdID(wxID_INDENT);
    stdID(wxID_UNINDENT);
    stdID(wxID_ZOOM_100);
    stdID(wxID_ZOOM_FIT);
    stdID(wxID_ZOOM_IN);
    stdID(wxID_ZOOM_OUT);
    stdID(wxID_UNDELETE);
    stdID(wxID_REVERT_TO_SAVED);
    stdID(wxID_SYSTEM_MENU);
    stdID(wxID_CLOSE_FRAME);
    stdID(wxID_MOVE_FRAME);
    stdID(wxID_RESIZE_FRAME);
    stdID(wxID_MAXIMIZE_FRAME);
    stdID(wxID_ICONIZE_FRAME);
    stdID(wxID_RESTORE_FRAME);

#undef stdID
}





// --------------- module and globals -----------------------------

class wxXmlResourceModule: public wxModule
{
DECLARE_DYNAMIC_CLASS(wxXmlResourceModule)
public:
    wxXmlResourceModule() {}
    bool OnInit()
    {
        wxXmlResource::AddSubclassFactory(new wxXmlSubclassFactoryCXX);
        return true;
    }
    void OnExit()
    {
        delete wxXmlResource::Set(NULL);
        if(wxXmlResource::ms_subclassFactories)
            WX_CLEAR_LIST(wxXmlSubclassFactoriesList, *wxXmlResource::ms_subclassFactories);
        wxDELETE(wxXmlResource::ms_subclassFactories);
        CleanXRCID_Records();
    }
};

IMPLEMENT_DYNAMIC_CLASS(wxXmlResourceModule, wxModule)


// When wxXml is loaded dynamically after the application is already running
// then the built-in module system won't pick this one up.  Add it manually.
void wxXmlInitResourceModule()
{
    wxModule* module = new wxXmlResourceModule;
    module->Init();
    wxModule::RegisterModule(module);
}

#endif // wxUSE_XRC
