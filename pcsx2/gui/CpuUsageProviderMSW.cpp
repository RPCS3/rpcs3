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
#include "CpuUsageProvider.h"

// undef this for compiler errors, code errors, security denies, and total fail.
#define _COM_SUCKS_SHIT_

#ifndef _COM_SUCKS_SHIT_

#include "MswStuff.h"
#include "System.h"
#include "SysThreads.h"
#include "GS.h"

#include <wx/dynlib.h>
#include <wbemcli.h>
#include <wbemprov.h>
#include <comutil.h>
#include <atlcomcli.h>

using namespace Threading;

#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "comsuppw.lib")

class CpuUsageProviderMSW : public BaseCpuUsageProvider
{
protected:
	bool	m_IsImplemented;

	CComPtr<IWbemServices>				m_WbemServices;

	//CComPtr<IWbemRefresher>				m_Refresher;
	//CComPtr<IWbemHiPerfEnum>			m_Enum;

public:
	CpuUsageProviderMSW();
	virtual ~CpuUsageProviderMSW() throw();

	bool IsImplemented() const;
	void UpdateStats();
	int GetEEcorePct() const;
	int GetGsPct() const;
	int GetVUPct() const;
	int GetGuiPct() const;
};

CpuUsageProviderMSW::CpuUsageProviderMSW()
{
	m_IsImplemented = false;

	{
		// Test if the OS supports cooked thread performance info...

		wxDoNotLogInThisScope please;
		wxDynamicLibrary perfinst( L"WmiPerfInst.dll" );
		if( !perfinst.IsLoaded() )
		{
			wxDynamicLibrary cooker( L"Wmicookr.dll" );
			if( !cooker.IsLoaded() ) return;
		}
	}

	m_IsImplemented = true;

	HRESULT hr;
	CComPtr<IWbemLocator>	m_IWbemLocator;

	// We're pretty well assured COM is Initialized.
	CoInitialize(NULL);

	if( FAILED (hr = CoInitializeSecurity(
			NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_NONE, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, 0) ) )
	{
		switch( hr )
		{
			case RPC_E_TOO_LATE: break;		// harmless failure, expected with wxWidgets.

			case RPC_E_NO_GOOD_SECURITY_PACKAGES:
				throw Exception::RuntimeError().SetDiagMsg( L"(CpuUsageProviderMSW) CoInitializeSecurity failed: No good security packages! .. whatever tht means." );

			case E_OUTOFMEMORY:
				throw Exception::OutOfMemory( L"(CpuUsageProviderMSW) Out of Memory error returned during call to CoInitializeSecurity." );

			default:
				throw Exception::RuntimeError().SetDiagMsg( wxsFormat( L"(CpuUsageProviderMSW) CoInitializeSecurity failed with an unknown error code: %d", hr ) );
		}
	}

	//BSTR bstrNamespace = L"\\\\.\\root\\cimv2";
	BSTR bstrNamespace = L"root\\cimv2";

	if( FAILED (hr = CoCreateInstance (
		CLSID_WbemAdministrativeLocator,
		NULL ,
		CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER ,
		IID_IUnknown ,
		(LPVOID*)&m_IWbemLocator) ) )
	{
		return;
	}

	if( FAILED (hr = m_IWbemLocator->ConnectServer(
		bstrNamespace, NULL, NULL, NULL, 0, NULL, NULL, &m_WbemServices) ) )
	{
		return;
	}

	/*
	HRESULT hr;
	CComPtr<IWbemConfigureRefresher>	m_Config;

	if (FAILED (hr = CoCreateInstance(
		CLSID_WbemRefresher,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IWbemRefresher,
		(void**) &m_Refresher)))
	{
		return;
	}

	if (FAILED (hr = m_Refresher->QueryInterface(
		IID_IWbemConfigureRefresher,
		(void **)&m_Config)))
	{
		return;
	}

	long lID = 0;		// why?

	// Add an enumerator to the refresher.
	if (FAILED (hr = m_Config->AddEnum(
		m_WbemServices,
		L"Win32_PerfRawData_PerfProc_Process",
		0,
		NULL,
		&m_Enum,
		&lID)))
	{
		return;
	}*/
}

CpuUsageProviderMSW::~CpuUsageProviderMSW() throw()
{
	//CoUninitialize();
}

bool CpuUsageProviderMSW::IsImplemented() const
{
	return m_IsImplemented;
}

void CpuUsageProviderMSW::UpdateStats()
{
	HRESULT hRes;
	BSTR strQuery	= L"Select * from Win32_PerfFormattedData_PerfProc_Thread where Name='EEcore'";
	BSTR strQL		= L"WQL";

	CComPtr<IEnumWbemClassObject>		m_EnumObject;
	hRes = m_WbemServices->ExecQuery(strQL, strQuery, WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &m_EnumObject);

	if( FAILED(hRes) )
	{
		//throw Exception::WinApiError("Could not execute Query");
		return;
	}

	hRes = m_EnumObject->Reset();
	if( FAILED(hRes) )
	{
		//MessageBox("Could not Enumerate");
		return;
	}

	ULONG uCount = 1, uReturned;
	IWbemClassObject* pClassObject = NULL;
	hRes = m_EnumObject->Next(WBEM_INFINITE,uCount, &pClassObject, &uReturned);
	if( FAILED(hRes) )
	{
		const wxChar* msg = NULL;

		switch( hRes )
		{
			case WBEM_E_INVALID_PARAMETER:
				msg = L"One or more invalid parameters were specified in the call.";
			break;

			case WBEM_E_UNEXPECTED:
				msg = L"An object in the enumeration has been deleted, destroying the validity of the enumeration.";
			break;

			case WBEM_E_TRANSPORT_FAILURE:
				msg = L"This indicates the failure of the remote procedure call (RPC) link between the current process and Windows Management.";
			break;

			case WBEM_S_FALSE:
				msg = L"The number of objects returned was less than the number requested. WBEM_S_FALSE is also returned when this method is called with a value of 0 for the uCount parameter.";
			break;

			case WBEM_S_TIMEDOUT:
				msg = L"A time-out occurred before you obtained all the objects.";
			break;

			case WBEM_E_ACCESS_DENIED:
				msg = L"Access Denied to WMI Query.";
			break;

			case WBEM_E_OUT_OF_MEMORY:
				throw Exception::OutOfMemory( L"(CpuUsageProviderMSW) Out of Memory Enumerating WMI Object." );

			default:
				throw Exception::RuntimeError().SetDiagMsg( wxsFormat( L"(CpuUsageProviderMSW) WMI Object Enumeration failed with an unknown error code: %d", hRes ) );
		}

		if( msg != NULL )
			throw Exception::RuntimeError().SetDiagMsg( (wxString)L"(CpuUsageProviderMSW) " + msg );

		return;
	}

	VARIANT v;
	BSTR strClassProp = SysAllocString(L"LoadPercentage");
	hRes = pClassObject->Get(strClassProp, 0, &v, 0, 0);
	if( FAILED(hRes) )
	{
		//MessageBox("Could not Get Value");
		return;
	}

	SysFreeString(strClassProp);
	_bstr_t bstrPath = &v; //Just to convert BSTR to ANSI
	char* strPath=(char*)bstrPath;

	/*if (SUCCEEDED(hRes))
		MessageBox(strPath);
	else
		MessageBox("Error in getting object");*/

	VariantClear(&v);
}

int CpuUsageProviderMSW::GetEEcorePct() const
{
	return 0;
}

int CpuUsageProviderMSW::GetGsPct() const
{
	return 0;
}

int CpuUsageProviderMSW::GetVUPct() const
{
	return 0;
}

int CpuUsageProviderMSW::GetGuiPct() const
{
	return 0;
}

#endif

CpuUsageProvider::CpuUsageProvider() :
	m_Implementation( new DefaultCpuUsageProvider() )
{
}

CpuUsageProvider::~CpuUsageProvider() throw()
{
}
