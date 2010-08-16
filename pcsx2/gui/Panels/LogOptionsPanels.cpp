/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include "PrecompiledHeader.h"
#include "LogOptionsPanels.h"

#include "Utilities/IniInterface.h"
#include "DebugTools/Debug.h"

#include <wx/statline.h>


using namespace pxSizerFlags;

Panels::eeLogOptionsPanel::eeLogOptionsPanel( LogOptionsPanel* parent )
	: BaseCpuLogOptionsPanel( parent, L"EE Logs" )
{
	SetMinWidth( 300 );

	m_miscGroup		= new wxStaticBoxSizer( wxVERTICAL, this, L"General" );

	m_disasmPanel	= new CheckedStaticBox( this, wxVERTICAL, L"Disasm" );
	m_hwPanel		= new CheckedStaticBox( this, wxVERTICAL, L"Registers" );
	m_evtPanel		= new CheckedStaticBox( this, wxVERTICAL, L"Events" );

	wxFlexGridSizer& eeTable( *new wxFlexGridSizer( 2, 5 ) );

	eeTable.AddGrowableCol(0);
	eeTable.AddGrowableCol(1);

	eeTable	+= m_miscGroup		| SubGroup();
	eeTable += m_hwPanel		| SubGroup();
	eeTable += m_evtPanel		| SubGroup();
	eeTable += m_disasmPanel	| SubGroup();

	ThisSizer	+= 4;
	ThisSizer	+= eeTable | pxExpand;

	SetValue( true );
}

Panels::iopLogOptionsPanel::iopLogOptionsPanel( LogOptionsPanel* parent )
	: BaseCpuLogOptionsPanel( parent, L"IOP Logs" )
{
	SetMinWidth( 280 );

	m_miscGroup		= new wxStaticBoxSizer( wxVERTICAL, this, L"General" );

	m_disasmPanel	= new CheckedStaticBox( this, wxVERTICAL, L"Disasm" );
	m_hwPanel		= new CheckedStaticBox( this, wxVERTICAL, L"Registers" );
	m_evtPanel		= new CheckedStaticBox( this, wxVERTICAL, L"Events" );

	wxFlexGridSizer& iopTable( *new wxFlexGridSizer( 2, 5 ) );

	iopTable.AddGrowableCol(0);
	iopTable.AddGrowableCol(1);

	iopTable	+= m_miscGroup		| SubGroup();
	iopTable	+= m_hwPanel		| SubGroup();
	iopTable	+= m_evtPanel		| SubGroup();
	iopTable	+= m_disasmPanel	| SubGroup();

	ThisSizer	+= 4;
	ThisSizer	+= iopTable	| pxExpand;

	SetValue( true );
}

CheckedStaticBox* Panels::eeLogOptionsPanel::GetStaticBox( const wxString& subgroup ) const
{
	if (0 == subgroup.CmpNoCase( L"Disasm" ))		return m_disasmPanel;
	if (0 == subgroup.CmpNoCase( L"Registers" ))	return m_hwPanel;
	if (0 == subgroup.CmpNoCase( L"Events" ))		return m_evtPanel;

	return NULL;
}

CheckedStaticBox* Panels::iopLogOptionsPanel::GetStaticBox( const wxString& subgroup ) const
{
	if (0 == subgroup.CmpNoCase( L"Disasm" ))		return m_disasmPanel;
	if (0 == subgroup.CmpNoCase( L"Registers" ))	return m_hwPanel;
	if (0 == subgroup.CmpNoCase( L"Events" ))		return m_evtPanel;

	return NULL;
}


void Panels::eeLogOptionsPanel::OnSettingsChanged()
{
	const TraceLogFilters& conf( g_Conf->EmuOptions.Trace );

	SetValue( conf.EE.m_EnableAll );

	m_disasmPanel	->SetValue( conf.EE.m_EnableDisasm );
	m_evtPanel		->SetValue( conf.EE.m_EnableEvents );
	m_hwPanel		->SetValue( conf.EE.m_EnableRegisters );
}

void Panels::iopLogOptionsPanel::OnSettingsChanged()
{
	const TraceLogFilters& conf( g_Conf->EmuOptions.Trace );

	SetValue( conf.IOP.m_EnableAll );

	m_disasmPanel	->SetValue( conf.IOP.m_EnableDisasm );
	m_evtPanel		->SetValue( conf.IOP.m_EnableEvents );
	m_hwPanel		->SetValue( conf.IOP.m_EnableRegisters );
}

static SysTraceLog * const traceLogList[] =
{
	&SysTrace.SIF,

	&SysTrace.EE.Bios,
	&SysTrace.EE.Memory,

	&SysTrace.EE.R5900,
	&SysTrace.EE.COP0,
	&SysTrace.EE.COP1,
	&SysTrace.EE.COP2,
	&SysTrace.EE.Cache,

	&SysTrace.EE.KnownHw,
	&SysTrace.EE.UnknownHw,
	&SysTrace.EE.DMAhw,
	&SysTrace.EE.IPU,
	&SysTrace.EE.GIFtag,
	&SysTrace.EE.VIFcode,

	&SysTrace.EE.DMAC,
	&SysTrace.EE.Counters,
	&SysTrace.EE.SPR,
	&SysTrace.EE.VIF,
	&SysTrace.EE.GIF,
	
	
	// IOP Section
	
	&SysTrace.IOP.Bios,
	&SysTrace.IOP.Memcards,
	&SysTrace.IOP.PAD,

	&SysTrace.IOP.R3000A,
	&SysTrace.IOP.COP2,
	&SysTrace.IOP.Memory,

	&SysTrace.IOP.KnownHw,
	&SysTrace.IOP.UnknownHw,
	&SysTrace.IOP.DMAhw,
	&SysTrace.IOP.DMAC,
	&SysTrace.IOP.Counters,
	&SysTrace.IOP.CDVD,
};

static const int traceLogCount = ArraySize(traceLogList);

void SysTraceLog_LoadSaveSettings( IniInterface& ini )
{
	ScopedIniGroup path(ini, L"TraceLogSources");

	for (uint i=0; i<traceLogCount; ++i)
	{
		if (SysTraceLog* log = traceLogList[i])
		{
			pxAssertMsg(log->GetName(), "Trace log without a name!" );
			ini.Entry( log->GetCategory() + L"." + log->GetShortName(), log->Enabled, false );
		}
	}
}

static bool traceLogEnabled( const wxString& ident )
{
	// Brute force search for now.  not enough different source logs to
	// justify using a hash table, and switching over to a "complex" class
	// type from the current simple array initializer requires effort to
	// avoid C++ global initializer dependency hell.

	for( uint i=0; i<traceLogCount; ++i )
	{
		if( 0 == ident.CmpNoCase(traceLogList[i]->GetCategory()) )
			return traceLogList[i]->Enabled;
	}

	pxFailDev( wxsFormat(L"Invalid or unknown TraceLog identifier: %s", ident.c_str()) );
	return false;
}

// --------------------------------------------------------------------------------------
//  LogOptionsPanel Implementations
// --------------------------------------------------------------------------------------
Panels::LogOptionsPanel::LogOptionsPanel(wxWindow* parent )
	: BaseApplicableConfigPanel( parent )
	, m_checks( traceLogCount )
{
	wxStaticBoxSizer&	s_misc		= *new wxStaticBoxSizer( wxHORIZONTAL, this, L"Misc" );

	m_eeSection		= new eeLogOptionsPanel( this );
	m_iopSection	= new iopLogOptionsPanel( this );

	for( uint i = 0; i<traceLogCount; ++i )
	{
		const SysTraceLog& item = *traceLogList[i];

		pxAssertMsg(item.GetName(), "Trace log without a name!" );

		wxStringTokenizer token( item.GetCategory(), L"." );
		wxSizer* addsizer = NULL;
		wxWindow* addparent = NULL;

		const wxString cpu(token.GetNextToken());
		if( BaseCpuLogOptionsPanel* cpupanel = GetCpuPanel(cpu))
		{
			const wxString group(token.GetNextToken());
			if( CheckedStaticBox* cpugroup = cpupanel->GetStaticBox(group))
			{
				addsizer = &cpugroup->ThisSizer;
				addparent = cpugroup;
			}
			else
			{
				addsizer = cpupanel->GetMiscGroup();
				addparent = cpupanel;
			}
		}
		else
		{
			addsizer = &s_misc;
			addparent = this;
		}

		*addsizer += m_checks[i] = new pxCheckBox( addparent, item.GetName() );
 		if( m_checks[i] && item.HasDescription() )
			m_checks[i]->SetToolTip(item.GetDescription());
	}

	m_masterEnabler = new pxCheckBox( this, _("Enable Trace Logging"),
		_("Trace logs are all written to emulog.txt.  Toggle trace logging at any time using F10.") );
	m_masterEnabler->SetToolTip( _("Warning: Enabling trace logs is typically very slow, and is a leading cause of 'What happened to my FPS?' problems. :)") );

	wxFlexGridSizer& topSizer = *new wxFlexGridSizer( 2 );

	topSizer.AddGrowableCol(0);
	topSizer.AddGrowableCol(1);

	topSizer	+= m_eeSection		| StdExpand();
	topSizer	+= m_iopSection		| StdExpand();

	*this		+= m_masterEnabler				| StdExpand();
	*this		+= new wxStaticLine( this )		| StdExpand().Border(wxLEFT | wxRIGHT, 20);
	*this		+= 5;
	*this		+= topSizer						| StdExpand();
	*this		+= s_misc						| StdSpace().Centre();

	Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(LogOptionsPanel::OnCheckBoxClicked) );
}

Panels::BaseCpuLogOptionsPanel* Panels::LogOptionsPanel::GetCpuPanel( const wxString& token ) const
{
	if( token == L"EE" )	return m_eeSection;
	if( token == L"IOP" )	return m_iopSection;
	
	return NULL;
}

void Panels::LogOptionsPanel::AppStatusEvent_OnSettingsApplied()
{
	TraceLogFilters& conf( g_Conf->EmuOptions.Trace );

	m_masterEnabler->SetValue( conf.Enabled );

	m_eeSection->OnSettingsChanged();
	m_iopSection->OnSettingsChanged();

	for (uint i=0; i<traceLogCount; ++i)
	{
		if (!traceLogList[i] || !m_checks[i]) continue;
		m_checks[i]->SetValue(traceLogList[i]->Enabled);
	}
	OnUpdateEnableAll();
}

void Panels::LogOptionsPanel::OnUpdateEnableAll()
{
	bool enabled( m_masterEnabler->GetValue() );

	m_eeSection->Enable( enabled );
	m_iopSection->Enable( enabled );
}

void Panels::LogOptionsPanel::OnCheckBoxClicked(wxCommandEvent &evt)
{
	m_IsDirty = true;
	if( evt.GetId() == m_masterEnabler->GetWxPtr()->GetId() )
		OnUpdateEnableAll();
}

void Panels::LogOptionsPanel::Apply()
{
	if( !m_IsDirty ) return;

	g_Conf->EmuOptions.Trace.Enabled	= m_masterEnabler->GetValue();

	m_eeSection->Apply();
	m_iopSection->Apply();

	m_IsDirty = false;
	
	for( uint i = 0; i<traceLogCount; ++i )
	{
		if (!traceLogList[i] || !m_checks[i]) continue;
		traceLogList[i]->Enabled = m_checks[i]->IsChecked();
	}
}

#define GetSet( cpu, name )		SysTrace.cpu.name.Enabled	= m_##name->GetValue()

void Panels::eeLogOptionsPanel::Apply()
{
	TraceFiltersEE& conf( g_Conf->EmuOptions.Trace.EE );

	conf.m_EnableAll		= GetValue();
	conf.m_EnableDisasm		= m_disasmPanel->GetValue();
	conf.m_EnableRegisters	= m_hwPanel->GetValue();
	conf.m_EnableEvents		= m_evtPanel->GetValue();
}

void Panels::iopLogOptionsPanel::Apply()
{
	TraceFiltersIOP& conf( g_Conf->EmuOptions.Trace.IOP );

	conf.m_EnableAll		= GetValue();
	conf.m_EnableDisasm		= m_disasmPanel->GetValue();
	conf.m_EnableRegisters	= m_hwPanel->GetValue();
	conf.m_EnableEvents		= m_evtPanel->GetValue();
}

