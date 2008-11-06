/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#define WINVER 0x0500
#include <windows.h>
#include <commdlg.h>

#include "resource.h"
#include "Debugger.h"
#include "Debug.h"
#include "R5900.h"
#include "R3000a.h"
#include "VUmicro.h"

HINSTANCE m_hInst;
HWND m_hWnd;
char text1[256];


/*R3000a registers handle */
static HWND IOPGPR0Handle=NULL;
static HWND IOPGPR1Handle=NULL;
static HWND IOPGPR2Handle=NULL;
static HWND IOPGPR3Handle=NULL;
static HWND IOPGPR4Handle=NULL;
static HWND IOPGPR5Handle=NULL;
static HWND IOPGPR6Handle=NULL;
static HWND IOPGPR7Handle=NULL;
static HWND IOPGPR8Handle=NULL;
static HWND IOPGPR9Handle=NULL;
static HWND IOPGPR10Handle=NULL;
static HWND IOPGPR11Handle=NULL;
static HWND IOPGPR12Handle=NULL;
static HWND IOPGPR13Handle=NULL;
static HWND IOPGPR14Handle=NULL;
static HWND IOPGPR15Handle=NULL;
static HWND IOPGPR16Handle=NULL;
static HWND IOPGPR17Handle=NULL;
static HWND IOPGPR18Handle=NULL;
static HWND IOPGPR19Handle=NULL;
static HWND IOPGPR20Handle=NULL;
static HWND IOPGPR21Handle=NULL;
static HWND IOPGPR22Handle=NULL;
static HWND IOPGPR23Handle=NULL;
static HWND IOPGPR24Handle=NULL;
static HWND IOPGPR25Handle=NULL;
static HWND IOPGPR26Handle=NULL;
static HWND IOPGPR27Handle=NULL;
static HWND IOPGPR28Handle=NULL;
static HWND IOPGPR29Handle=NULL;
static HWND IOPGPR30Handle=NULL;
static HWND IOPGPR31Handle=NULL;
static HWND IOPGPRPCHandle=NULL;
static HWND IOPGPRHIHandle=NULL;
static HWND IOPGPRLOHandle=NULL;

/*R5900 registers handle */
static HWND GPR0Handle=NULL;
static HWND GPR1Handle=NULL;
static HWND GPR2Handle=NULL;
static HWND GPR3Handle=NULL;
static HWND GPR4Handle=NULL;
static HWND GPR5Handle=NULL;
static HWND GPR6Handle=NULL;
static HWND GPR7Handle=NULL;
static HWND GPR8Handle=NULL;
static HWND GPR9Handle=NULL;
static HWND GPR10Handle=NULL;
static HWND GPR11Handle=NULL;
static HWND GPR12Handle=NULL;
static HWND GPR13Handle=NULL;
static HWND GPR14Handle=NULL;
static HWND GPR15Handle=NULL;
static HWND GPR16Handle=NULL;
static HWND GPR17Handle=NULL;
static HWND GPR18Handle=NULL;
static HWND GPR19Handle=NULL;
static HWND GPR20Handle=NULL;
static HWND GPR21Handle=NULL;
static HWND GPR22Handle=NULL;
static HWND GPR23Handle=NULL;
static HWND GPR24Handle=NULL;
static HWND GPR25Handle=NULL;
static HWND GPR26Handle=NULL;
static HWND GPR27Handle=NULL;
static HWND GPR28Handle=NULL;
static HWND GPR29Handle=NULL;
static HWND GPR30Handle=NULL;
static HWND GPR31Handle=NULL;
static HWND GPRPCHandle=NULL;
static HWND GPRHIHandle=NULL;
static HWND GPRLOHandle=NULL;
/*end of r3000a registers handle */
/*cop0 registers here */
static HWND COP00Handle=NULL;
static HWND COP01Handle=NULL;
static HWND COP02Handle=NULL;
static HWND COP03Handle=NULL;
static HWND COP04Handle=NULL;
static HWND COP05Handle=NULL;
static HWND COP06Handle=NULL;
static HWND COP07Handle=NULL;
static HWND COP08Handle=NULL;
static HWND COP09Handle=NULL;
static HWND COP010Handle=NULL;
static HWND COP011Handle=NULL;
static HWND COP012Handle=NULL;
static HWND COP013Handle=NULL;
static HWND COP014Handle=NULL;
static HWND COP015Handle=NULL;
static HWND COP016Handle=NULL;
static HWND COP017Handle=NULL;
static HWND COP018Handle=NULL;
static HWND COP019Handle=NULL;
static HWND COP020Handle=NULL;
static HWND COP021Handle=NULL;
static HWND COP022Handle=NULL;
static HWND COP023Handle=NULL;
static HWND COP024Handle=NULL;
static HWND COP025Handle=NULL;
static HWND COP026Handle=NULL;
static HWND COP027Handle=NULL;
static HWND COP028Handle=NULL;
static HWND COP029Handle=NULL;
static HWND COP030Handle=NULL;
static HWND COP031Handle=NULL;
static HWND COP0PCHandle=NULL;
static HWND COP0HIHandle=NULL;
static HWND COP0LOHandle=NULL;
/*end of cop0 registers */
/*cop1 registers here */
static HWND COP10Handle=NULL;
static HWND COP11Handle=NULL;
static HWND COP12Handle=NULL;
static HWND COP13Handle=NULL;
static HWND COP14Handle=NULL;
static HWND COP15Handle=NULL;
static HWND COP16Handle=NULL;
static HWND COP17Handle=NULL;
static HWND COP18Handle=NULL;
static HWND COP19Handle=NULL;
static HWND COP110Handle=NULL;
static HWND COP111Handle=NULL;
static HWND COP112Handle=NULL;
static HWND COP113Handle=NULL;
static HWND COP114Handle=NULL;
static HWND COP115Handle=NULL;
static HWND COP116Handle=NULL;
static HWND COP117Handle=NULL;
static HWND COP118Handle=NULL;
static HWND COP119Handle=NULL;
static HWND COP120Handle=NULL;
static HWND COP121Handle=NULL;
static HWND COP122Handle=NULL;
static HWND COP123Handle=NULL;
static HWND COP124Handle=NULL;
static HWND COP125Handle=NULL;
static HWND COP126Handle=NULL;
static HWND COP127Handle=NULL;
static HWND COP128Handle=NULL;
static HWND COP129Handle=NULL;
static HWND COP130Handle=NULL;
static HWND COP131Handle=NULL;
static HWND COP1C0Handle=NULL;
static HWND COP1C1Handle=NULL;
static HWND COP1ACCHandle=NULL;
/*end of cop1 registers */
/*cop2 floating registers*/
static HWND VU0F00Handle=NULL;
static HWND VU0F01Handle=NULL;
static HWND VU0F02Handle=NULL;
static HWND VU0F03Handle=NULL;
static HWND VU0F04Handle=NULL;
static HWND VU0F05Handle=NULL;
static HWND VU0F06Handle=NULL;
static HWND VU0F07Handle=NULL;
static HWND VU0F08Handle=NULL;
static HWND VU0F09Handle=NULL;
static HWND VU0F10Handle=NULL;
static HWND VU0F11Handle=NULL;
static HWND VU0F12Handle=NULL;
static HWND VU0F13Handle=NULL;
static HWND VU0F14Handle=NULL;
static HWND VU0F15Handle=NULL;
static HWND VU0F16Handle=NULL;
static HWND VU0F17Handle=NULL;
static HWND VU0F18Handle=NULL;
static HWND VU0F19Handle=NULL;
static HWND VU0F20Handle=NULL;
static HWND VU0F21Handle=NULL;
static HWND VU0F22Handle=NULL;
static HWND VU0F23Handle=NULL;
static HWND VU0F24Handle=NULL;
static HWND VU0F25Handle=NULL;
static HWND VU0F26Handle=NULL;
static HWND VU0F27Handle=NULL;
static HWND VU0F28Handle=NULL;
static HWND VU0F29Handle=NULL;
static HWND VU0F30Handle=NULL;
static HWND VU0F31Handle=NULL;
/*end of cop2 floating registers*/
/*cop2 control registers */
static HWND VU0C00Handle=NULL;
static HWND VU0C01Handle=NULL;
static HWND VU0C02Handle=NULL;
static HWND VU0C03Handle=NULL;
static HWND VU0C04Handle=NULL;
static HWND VU0C05Handle=NULL;
static HWND VU0C06Handle=NULL;
static HWND VU0C07Handle=NULL;
static HWND VU0C08Handle=NULL;
static HWND VU0C09Handle=NULL;
static HWND VU0C10Handle=NULL;
static HWND VU0C11Handle=NULL;
static HWND VU0C12Handle=NULL;
static HWND VU0C13Handle=NULL;
static HWND VU0C14Handle=NULL;
static HWND VU0C15Handle=NULL;
static HWND VU0C16Handle=NULL;
static HWND VU0C17Handle=NULL;
static HWND VU0C18Handle=NULL;
static HWND VU0C19Handle=NULL;
static HWND VU0C20Handle=NULL;
static HWND VU0C21Handle=NULL;
static HWND VU0C22Handle=NULL;
static HWND VU0C23Handle=NULL;
static HWND VU0C24Handle=NULL;
static HWND VU0C25Handle=NULL;
static HWND VU0C26Handle=NULL;
static HWND VU0C27Handle=NULL;
static HWND VU0C28Handle=NULL;
static HWND VU0C29Handle=NULL;
static HWND VU0C30Handle=NULL;
static HWND VU0C31Handle=NULL;
static HWND VU0ACCHandle=NULL;
/*end of cop2 control registers */
/*vu1 floating registers*/
static HWND VU1F00Handle=NULL;
static HWND VU1F01Handle=NULL;
static HWND VU1F02Handle=NULL;
static HWND VU1F03Handle=NULL;
static HWND VU1F04Handle=NULL;
static HWND VU1F05Handle=NULL;
static HWND VU1F06Handle=NULL;
static HWND VU1F07Handle=NULL;
static HWND VU1F08Handle=NULL;
static HWND VU1F09Handle=NULL;
static HWND VU1F10Handle=NULL;
static HWND VU1F11Handle=NULL;
static HWND VU1F12Handle=NULL;
static HWND VU1F13Handle=NULL;
static HWND VU1F14Handle=NULL;
static HWND VU1F15Handle=NULL;
static HWND VU1F16Handle=NULL;
static HWND VU1F17Handle=NULL;
static HWND VU1F18Handle=NULL;
static HWND VU1F19Handle=NULL;
static HWND VU1F20Handle=NULL;
static HWND VU1F21Handle=NULL;
static HWND VU1F22Handle=NULL;
static HWND VU1F23Handle=NULL;
static HWND VU1F24Handle=NULL;
static HWND VU1F25Handle=NULL;
static HWND VU1F26Handle=NULL;
static HWND VU1F27Handle=NULL;
static HWND VU1F28Handle=NULL;
static HWND VU1F29Handle=NULL;
static HWND VU1F30Handle=NULL;
static HWND VU1F31Handle=NULL;
/*end of vu1 floating registers*/
/*vu1 control registers */
static HWND VU1C00Handle=NULL;
static HWND VU1C01Handle=NULL;
static HWND VU1C02Handle=NULL;
static HWND VU1C03Handle=NULL;
static HWND VU1C04Handle=NULL;
static HWND VU1C05Handle=NULL;
static HWND VU1C06Handle=NULL;
static HWND VU1C07Handle=NULL;
static HWND VU1C08Handle=NULL;
static HWND VU1C09Handle=NULL;
static HWND VU1C10Handle=NULL;
static HWND VU1C11Handle=NULL;
static HWND VU1C12Handle=NULL;
static HWND VU1C13Handle=NULL;
static HWND VU1C14Handle=NULL;
static HWND VU1C15Handle=NULL;
static HWND VU1C16Handle=NULL;
static HWND VU1C17Handle=NULL;
static HWND VU1C18Handle=NULL;
static HWND VU1C19Handle=NULL;
static HWND VU1C20Handle=NULL;
static HWND VU1C21Handle=NULL;
static HWND VU1C22Handle=NULL;
static HWND VU1C23Handle=NULL;
static HWND VU1C24Handle=NULL;
static HWND VU1C25Handle=NULL;
static HWND VU1C26Handle=NULL;
static HWND VU1C27Handle=NULL;
static HWND VU1C28Handle=NULL;
static HWND VU1C29Handle=NULL;
static HWND VU1C30Handle=NULL;
static HWND VU1C31Handle=NULL;
static HWND VU1ACCHandle=NULL;
/*end of vu1 control registers */

LRESULT CALLBACK R3000reg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

//comctl32 lib must add to project..
int CreatePropertySheet(HWND hwndOwner)
{
	
	PROPSHEETPAGE psp[7];
	PROPSHEETHEADER psh;

	psp[0].dwSize = sizeof(PROPSHEETPAGE);
	psp[0].dwFlags = PSP_USETITLE;
	psp[0].hInstance = m_hInst;
	psp[0].pszTemplate = MAKEINTRESOURCE( IDD_GPREGS);
	psp[0].pszIcon = NULL;
	psp[0].pfnDlgProc =(DLGPROC)R5900reg;
	psp[0].pszTitle = "R5900";
	psp[0].lParam = 0;
   
	psp[1].dwSize = sizeof(PROPSHEETPAGE);
	psp[1].dwFlags = PSP_USETITLE;
	psp[1].hInstance =  m_hInst;
	psp[1].pszTemplate = MAKEINTRESOURCE( IDD_CP0REGS );
	psp[1].pszIcon = NULL;
	psp[1].pfnDlgProc =(DLGPROC)COP0reg;
	psp[1].pszTitle = "COP0";
	psp[1].lParam = 0;

	psp[2].dwSize = sizeof(PROPSHEETPAGE);
	psp[2].dwFlags = PSP_USETITLE;
	psp[2].hInstance =  m_hInst;
	psp[2].pszTemplate = MAKEINTRESOURCE( IDD_CP1REGS );
	psp[2].pszIcon = NULL;
	psp[2].pfnDlgProc =(DLGPROC)COP1reg;
	psp[2].pszTitle = "COP1";
	psp[2].lParam = 0;

	psp[3].dwSize = sizeof(PROPSHEETPAGE);
	psp[3].dwFlags = PSP_USETITLE;
	psp[3].hInstance =  m_hInst;
	psp[3].pszTemplate = MAKEINTRESOURCE( IDD_VU0REGS );
	psp[3].pszIcon = NULL;
	psp[3].pfnDlgProc =(DLGPROC)COP2Freg;
	psp[3].pszTitle = "COP2F";
	psp[3].lParam = 0;

	psp[4].dwSize = sizeof(PROPSHEETPAGE);
	psp[4].dwFlags = PSP_USETITLE;
	psp[4].hInstance =  m_hInst;
	psp[4].pszTemplate = MAKEINTRESOURCE( IDD_VU0INTEGER );
	psp[4].pszIcon = NULL;
	psp[4].pfnDlgProc =(DLGPROC)COP2Creg;
	psp[4].pszTitle = "COP2C";
	psp[4].lParam = 0;

	psp[5].dwSize = sizeof(PROPSHEETPAGE);
	psp[5].dwFlags = PSP_USETITLE;
	psp[5].hInstance =  m_hInst;
	psp[5].pszTemplate = MAKEINTRESOURCE( IDD_VU1REGS );
	psp[5].pszIcon = NULL;
	psp[5].pfnDlgProc =(DLGPROC)VU1Freg;
	psp[5].pszTitle = "VU1F";
	psp[5].lParam = 0;

	psp[6].dwSize = sizeof(PROPSHEETPAGE);
	psp[6].dwFlags = PSP_USETITLE;
	psp[6].hInstance =  m_hInst;
	psp[6].pszTemplate = MAKEINTRESOURCE( IDD_VU1INTEGER );
	psp[6].pszIcon = NULL;
	psp[6].pfnDlgProc =(DLGPROC)VU1Creg;
	psp[6].pszTitle = "VU1C";
	psp[6].lParam = 0;

	psp[6].dwSize = sizeof(PROPSHEETPAGE);
	psp[6].dwFlags = PSP_USETITLE;
	psp[6].hInstance =  m_hInst;
	psp[6].pszTemplate = MAKEINTRESOURCE( IDD_IOPREGS );
	psp[6].pszIcon = NULL;
	psp[6].pfnDlgProc =(DLGPROC)R3000reg;
	psp[6].pszTitle = "R3000";
	psp[6].lParam = 0;

	psh.dwSize = sizeof(PROPSHEETHEADER);
	psh.dwFlags = PSH_PROPSHEETPAGE | PSH_MODELESS;
	psh.hwndParent =hwndOwner;
	psh.hInstance = m_hInst;
	psh.pszIcon = NULL;
	psh.pszCaption = (LPSTR) "Debugger";
	psh.nStartPage = 0;
	psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
	psh.ppsp = (LPCPROPSHEETPAGE) &psp;
   
      return (PropertySheet(&psh)); 

}
LRESULT CALLBACK R3000reg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
        IOPGPR0Handle=GetDlgItem(hDlg,IDC_IOPGPR0);
        IOPGPR1Handle=GetDlgItem(hDlg,IDC_IOPGPR1);
        IOPGPR2Handle=GetDlgItem(hDlg,IDC_IOPGPR2);
        IOPGPR3Handle=GetDlgItem(hDlg,IDC_IOPGPR3);
        IOPGPR4Handle=GetDlgItem(hDlg,IDC_IOPGPR4);
        IOPGPR5Handle=GetDlgItem(hDlg,IDC_IOPGPR5);
        IOPGPR6Handle=GetDlgItem(hDlg,IDC_IOPGPR6);
        IOPGPR7Handle=GetDlgItem(hDlg,IDC_IOPGPR7);
        IOPGPR8Handle=GetDlgItem(hDlg,IDC_IOPGPR8);
        IOPGPR9Handle=GetDlgItem(hDlg,IDC_IOPGPR9);
        IOPGPR10Handle=GetDlgItem(hDlg,IDC_IOPGPR10);
        IOPGPR11Handle=GetDlgItem(hDlg,IDC_IOPGPR11);
        IOPGPR12Handle=GetDlgItem(hDlg,IDC_IOPGPR12);
        IOPGPR13Handle=GetDlgItem(hDlg,IDC_IOPGPR13);
        IOPGPR14Handle=GetDlgItem(hDlg,IDC_IOPGPR14);
        IOPGPR15Handle=GetDlgItem(hDlg,IDC_IOPGPR15);
        IOPGPR16Handle=GetDlgItem(hDlg,IDC_IOPGPR16);
        IOPGPR17Handle=GetDlgItem(hDlg,IDC_IOPGPR17);
        IOPGPR18Handle=GetDlgItem(hDlg,IDC_IOPGPR18);
        IOPGPR19Handle=GetDlgItem(hDlg,IDC_IOPGPR19);
        IOPGPR20Handle=GetDlgItem(hDlg,IDC_IOPGPR20);
        IOPGPR21Handle=GetDlgItem(hDlg,IDC_IOPGPR21);
        IOPGPR22Handle=GetDlgItem(hDlg,IDC_IOPGPR22);
        IOPGPR23Handle=GetDlgItem(hDlg,IDC_IOPGPR23);
        IOPGPR24Handle=GetDlgItem(hDlg,IDC_IOPGPR24);
        IOPGPR25Handle=GetDlgItem(hDlg,IDC_IOPGPR25);
        IOPGPR26Handle=GetDlgItem(hDlg,IDC_IOPGPR26);
        IOPGPR27Handle=GetDlgItem(hDlg,IDC_IOPGPR27);
        IOPGPR28Handle=GetDlgItem(hDlg,IDC_IOPGPR28);
        IOPGPR29Handle=GetDlgItem(hDlg,IDC_IOPGPR29);
        IOPGPR30Handle=GetDlgItem(hDlg,IDC_IOPGPR30);
        IOPGPR31Handle=GetDlgItem(hDlg,IDC_IOPGPR31);
        IOPGPRPCHandle=GetDlgItem(hDlg,IDC_IOPGPR_PC);
        IOPGPRHIHandle=GetDlgItem(hDlg,IDC_IOPGPR_HI);
        IOPGPRLOHandle=GetDlgItem(hDlg,IDC_IOPGPR_LO);
		UpdateRegs();
	    return (TRUE);
		break;
	case WM_COMMAND:
 
			switch(LOWORD(wParam))
		{
		case (IDOK || IDCANCEL):
			EndDialog(hDlg,TRUE);
			return(TRUE);
			break;
	
		}
		break;
	}

	return(FALSE);
}
LRESULT CALLBACK R5900reg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
        GPR0Handle=GetDlgItem(hDlg,IDC_GPR0);
        GPR1Handle=GetDlgItem(hDlg,IDC_GPR1);
        GPR2Handle=GetDlgItem(hDlg,IDC_GPR2);
        GPR3Handle=GetDlgItem(hDlg,IDC_GPR3);
        GPR4Handle=GetDlgItem(hDlg,IDC_GPR4);
        GPR5Handle=GetDlgItem(hDlg,IDC_GPR5);
        GPR6Handle=GetDlgItem(hDlg,IDC_GPR6);
        GPR7Handle=GetDlgItem(hDlg,IDC_GPR7);
        GPR8Handle=GetDlgItem(hDlg,IDC_GPR8);
        GPR9Handle=GetDlgItem(hDlg,IDC_GPR9);
        GPR10Handle=GetDlgItem(hDlg,IDC_GPR10);
        GPR11Handle=GetDlgItem(hDlg,IDC_GPR11);
        GPR12Handle=GetDlgItem(hDlg,IDC_GPR12);
        GPR13Handle=GetDlgItem(hDlg,IDC_GPR13);
        GPR14Handle=GetDlgItem(hDlg,IDC_GPR14);
        GPR15Handle=GetDlgItem(hDlg,IDC_GPR15);
        GPR16Handle=GetDlgItem(hDlg,IDC_GPR16);
        GPR17Handle=GetDlgItem(hDlg,IDC_GPR17);
        GPR18Handle=GetDlgItem(hDlg,IDC_GPR18);
        GPR19Handle=GetDlgItem(hDlg,IDC_GPR19);
        GPR20Handle=GetDlgItem(hDlg,IDC_GPR20);
        GPR21Handle=GetDlgItem(hDlg,IDC_GPR21);
        GPR22Handle=GetDlgItem(hDlg,IDC_GPR22);
        GPR23Handle=GetDlgItem(hDlg,IDC_GPR23);
        GPR24Handle=GetDlgItem(hDlg,IDC_GPR24);
        GPR25Handle=GetDlgItem(hDlg,IDC_GPR25);
        GPR26Handle=GetDlgItem(hDlg,IDC_GPR26);
        GPR27Handle=GetDlgItem(hDlg,IDC_GPR27);
        GPR28Handle=GetDlgItem(hDlg,IDC_GPR28);
        GPR29Handle=GetDlgItem(hDlg,IDC_GPR29);
        GPR30Handle=GetDlgItem(hDlg,IDC_GPR30);
        GPR31Handle=GetDlgItem(hDlg,IDC_GPR31);
        GPRPCHandle=GetDlgItem(hDlg,IDC_GPR_PC);
        GPRHIHandle=GetDlgItem(hDlg,IDC_GPR_HI);
        GPRLOHandle=GetDlgItem(hDlg,IDC_GPR_LO);
		UpdateRegs();
	    return (TRUE);
		break;
	case WM_COMMAND:
 
			switch(LOWORD(wParam))
		{
		case (IDOK || IDCANCEL):
			EndDialog(hDlg,TRUE);
			return(TRUE);
			break;
	
		}
		break;
	}

	return(FALSE);
}
LRESULT CALLBACK COP0reg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
 	switch(message)
	{
	case WM_INITDIALOG:
		   COP00Handle=GetDlgItem(hDlg,IDC_CP00);
           COP01Handle=GetDlgItem(hDlg,IDC_CP01);
           COP02Handle=GetDlgItem(hDlg,IDC_CP02);
           COP03Handle=GetDlgItem(hDlg,IDC_CP03);
           COP04Handle=GetDlgItem(hDlg,IDC_CP04);
           COP05Handle=GetDlgItem(hDlg,IDC_CP05);
           COP06Handle=GetDlgItem(hDlg,IDC_CP06);
           COP07Handle=GetDlgItem(hDlg,IDC_CP07);
           COP08Handle=GetDlgItem(hDlg,IDC_CP08);
           COP09Handle=GetDlgItem(hDlg,IDC_CP09);
           COP010Handle=GetDlgItem(hDlg,IDC_CP010);
           COP011Handle=GetDlgItem(hDlg,IDC_CP011);
           COP012Handle=GetDlgItem(hDlg,IDC_CP012);
           COP013Handle=GetDlgItem(hDlg,IDC_CP013);
           COP014Handle=GetDlgItem(hDlg,IDC_CP014);
           COP015Handle=GetDlgItem(hDlg,IDC_CP015);
           COP016Handle=GetDlgItem(hDlg,IDC_CP016);
           COP017Handle=GetDlgItem(hDlg,IDC_CP017);
           COP018Handle=GetDlgItem(hDlg,IDC_CP018);
           COP019Handle=GetDlgItem(hDlg,IDC_CP019);
           COP020Handle=GetDlgItem(hDlg,IDC_CP020);
           COP021Handle=GetDlgItem(hDlg,IDC_CP021);
           COP022Handle=GetDlgItem(hDlg,IDC_CP022);
           COP023Handle=GetDlgItem(hDlg,IDC_CP023);
           COP024Handle=GetDlgItem(hDlg,IDC_CP024);
           COP025Handle=GetDlgItem(hDlg,IDC_CP025);
           COP026Handle=GetDlgItem(hDlg,IDC_CP026);
           COP027Handle=GetDlgItem(hDlg,IDC_CP027);
           COP028Handle=GetDlgItem(hDlg,IDC_CP028);
           COP029Handle=GetDlgItem(hDlg,IDC_CP029);
           COP030Handle=GetDlgItem(hDlg,IDC_CP030);
           COP031Handle=GetDlgItem(hDlg,IDC_CP031);
           UpdateRegs();
		return (TRUE);
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case (IDOK || IDCANCEL):
			EndDialog(hDlg,TRUE);
			return(TRUE);
			break;
	
		}
		break;
	}

	return(FALSE);
}
LRESULT CALLBACK COP1reg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
 	switch(message)
	{
	case WM_INITDIALOG:
		   COP10Handle=GetDlgItem(hDlg,IDC_FP0);
           COP11Handle=GetDlgItem(hDlg,IDC_FP1);
           COP12Handle=GetDlgItem(hDlg,IDC_FP2);
           COP13Handle=GetDlgItem(hDlg,IDC_FP3);
           COP14Handle=GetDlgItem(hDlg,IDC_FP4);
           COP15Handle=GetDlgItem(hDlg,IDC_FP5);
           COP16Handle=GetDlgItem(hDlg,IDC_FP6);
           COP17Handle=GetDlgItem(hDlg,IDC_FP7);
           COP18Handle=GetDlgItem(hDlg,IDC_FP8);
           COP19Handle=GetDlgItem(hDlg,IDC_FP9);
           COP110Handle=GetDlgItem(hDlg,IDC_FP10);
           COP111Handle=GetDlgItem(hDlg,IDC_FP11);
           COP112Handle=GetDlgItem(hDlg,IDC_FP12);
           COP113Handle=GetDlgItem(hDlg,IDC_FP13);
           COP114Handle=GetDlgItem(hDlg,IDC_FP14);
           COP115Handle=GetDlgItem(hDlg,IDC_FP15);
           COP116Handle=GetDlgItem(hDlg,IDC_FP16);
           COP117Handle=GetDlgItem(hDlg,IDC_FP17);
           COP118Handle=GetDlgItem(hDlg,IDC_FP18);
           COP119Handle=GetDlgItem(hDlg,IDC_FP19);
           COP120Handle=GetDlgItem(hDlg,IDC_FP20);
           COP121Handle=GetDlgItem(hDlg,IDC_FP21);
           COP122Handle=GetDlgItem(hDlg,IDC_FP22);
           COP123Handle=GetDlgItem(hDlg,IDC_FP23);
           COP124Handle=GetDlgItem(hDlg,IDC_FP24);
           COP125Handle=GetDlgItem(hDlg,IDC_FP25);
           COP126Handle=GetDlgItem(hDlg,IDC_FP26);
           COP127Handle=GetDlgItem(hDlg,IDC_FP27);
           COP128Handle=GetDlgItem(hDlg,IDC_FP28);
           COP129Handle=GetDlgItem(hDlg,IDC_FP29);
           COP130Handle=GetDlgItem(hDlg,IDC_FP30);
           COP131Handle=GetDlgItem(hDlg,IDC_FP31);
		   COP1C0Handle=GetDlgItem(hDlg,IDC_FCR0);
           COP1C1Handle=GetDlgItem(hDlg,IDC_FCR31);
           COP1ACCHandle=GetDlgItem(hDlg,IDC_FPU_ACC);
           UpdateRegs();
		return (TRUE);
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case (IDOK || IDCANCEL):
			EndDialog(hDlg,TRUE);
			return(TRUE);
			break;
	
		}
		break;
	}

	return(FALSE);
}

LRESULT CALLBACK COP2Freg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
        VU0F00Handle=GetDlgItem(hDlg,IDC_VU0_VF00);
        VU0F01Handle=GetDlgItem(hDlg,IDC_VU0_VF01);
        VU0F02Handle=GetDlgItem(hDlg,IDC_VU0_VF02);
        VU0F03Handle=GetDlgItem(hDlg,IDC_VU0_VF03);
        VU0F04Handle=GetDlgItem(hDlg,IDC_VU0_VF04);
        VU0F05Handle=GetDlgItem(hDlg,IDC_VU0_VF05);
        VU0F06Handle=GetDlgItem(hDlg,IDC_VU0_VF06);
        VU0F07Handle=GetDlgItem(hDlg,IDC_VU0_VF07);
        VU0F08Handle=GetDlgItem(hDlg,IDC_VU0_VF08);
        VU0F09Handle=GetDlgItem(hDlg,IDC_VU0_VF09);
        VU0F10Handle=GetDlgItem(hDlg,IDC_VU0_VF10);
        VU0F11Handle=GetDlgItem(hDlg,IDC_VU0_VF11);
        VU0F12Handle=GetDlgItem(hDlg,IDC_VU0_VF12);
        VU0F13Handle=GetDlgItem(hDlg,IDC_VU0_VF13);
        VU0F14Handle=GetDlgItem(hDlg,IDC_VU0_VF14);
        VU0F15Handle=GetDlgItem(hDlg,IDC_VU0_VF15);
        VU0F16Handle=GetDlgItem(hDlg,IDC_VU0_VF16);
        VU0F17Handle=GetDlgItem(hDlg,IDC_VU0_VF17);
        VU0F18Handle=GetDlgItem(hDlg,IDC_VU0_VF18);
        VU0F19Handle=GetDlgItem(hDlg,IDC_VU0_VF19);
        VU0F20Handle=GetDlgItem(hDlg,IDC_VU0_VF20);
        VU0F21Handle=GetDlgItem(hDlg,IDC_VU0_VF21);
        VU0F22Handle=GetDlgItem(hDlg,IDC_VU0_VF22);
        VU0F23Handle=GetDlgItem(hDlg,IDC_VU0_VF23);
        VU0F24Handle=GetDlgItem(hDlg,IDC_VU0_VF24);
        VU0F25Handle=GetDlgItem(hDlg,IDC_VU0_VF25);
        VU0F26Handle=GetDlgItem(hDlg,IDC_VU0_VF26);
        VU0F27Handle=GetDlgItem(hDlg,IDC_VU0_VF27);
        VU0F28Handle=GetDlgItem(hDlg,IDC_VU0_VF28);
        VU0F29Handle=GetDlgItem(hDlg,IDC_VU0_VF29);
        VU0F30Handle=GetDlgItem(hDlg,IDC_VU0_VF30);
        VU0F31Handle=GetDlgItem(hDlg,IDC_VU0_VF31);
		UpdateRegs();
	    return (TRUE);
		break;
	case WM_COMMAND:
 
			switch(LOWORD(wParam))
		{
		case (IDOK || IDCANCEL):
			EndDialog(hDlg,TRUE);
			return(TRUE);
			break;
	
		}
		break;
	}

	return(FALSE);
}
LRESULT CALLBACK COP2Creg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
        VU0C00Handle=GetDlgItem(hDlg,IDC_VU0_VI00);
        VU0C01Handle=GetDlgItem(hDlg,IDC_VU0_VI01);
        VU0C02Handle=GetDlgItem(hDlg,IDC_VU0_VI02);
        VU0C03Handle=GetDlgItem(hDlg,IDC_VU0_VI03);
        VU0C04Handle=GetDlgItem(hDlg,IDC_VU0_VI04);
        VU0C05Handle=GetDlgItem(hDlg,IDC_VU0_VI05);
        VU0C06Handle=GetDlgItem(hDlg,IDC_VU0_VI06);
        VU0C07Handle=GetDlgItem(hDlg,IDC_VU0_VI07);
        VU0C08Handle=GetDlgItem(hDlg,IDC_VU0_VI08);
        VU0C09Handle=GetDlgItem(hDlg,IDC_VU0_VI09);
        VU0C10Handle=GetDlgItem(hDlg,IDC_VU0_VI10);
        VU0C11Handle=GetDlgItem(hDlg,IDC_VU0_VI11);
        VU0C12Handle=GetDlgItem(hDlg,IDC_VU0_VI12);
        VU0C13Handle=GetDlgItem(hDlg,IDC_VU0_VI13);
        VU0C14Handle=GetDlgItem(hDlg,IDC_VU0_VI14);
        VU0C15Handle=GetDlgItem(hDlg,IDC_VU0_VI15);
        VU0C16Handle=GetDlgItem(hDlg,IDC_VU0_VI16);
        VU0C17Handle=GetDlgItem(hDlg,IDC_VU0_VI17);
        VU0C18Handle=GetDlgItem(hDlg,IDC_VU0_VI18);
        VU0C19Handle=GetDlgItem(hDlg,IDC_VU0_VI19);
        VU0C20Handle=GetDlgItem(hDlg,IDC_VU0_VI20);
        VU0C21Handle=GetDlgItem(hDlg,IDC_VU0_VI21);
        VU0C22Handle=GetDlgItem(hDlg,IDC_VU0_VI22);
        VU0C23Handle=GetDlgItem(hDlg,IDC_VU0_VI23);
        VU0C24Handle=GetDlgItem(hDlg,IDC_VU0_VI24);
        VU0C25Handle=GetDlgItem(hDlg,IDC_VU0_VI25);
        VU0C26Handle=GetDlgItem(hDlg,IDC_VU0_VI26);
        VU0C27Handle=GetDlgItem(hDlg,IDC_VU0_VI27);
        VU0C28Handle=GetDlgItem(hDlg,IDC_VU0_VI28);
        VU0C29Handle=GetDlgItem(hDlg,IDC_VU0_VI29);
        VU0C30Handle=GetDlgItem(hDlg,IDC_VU0_VI30);
        VU0C31Handle=GetDlgItem(hDlg,IDC_VU0_VI31);
        VU0ACCHandle=GetDlgItem(hDlg,IDC_VU0_ACC);
		UpdateRegs();
	    return (TRUE);
		break;
	case WM_COMMAND:
 
			switch(LOWORD(wParam))
		{
		case (IDOK || IDCANCEL):
			EndDialog(hDlg,TRUE);
			return(TRUE);
			break;
	
		}
		break;
	}

	return(FALSE);
}


LRESULT CALLBACK VU1Freg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
        VU1F00Handle=GetDlgItem(hDlg,IDC_VU1_VF00);
        VU1F01Handle=GetDlgItem(hDlg,IDC_VU1_VF01);
        VU1F02Handle=GetDlgItem(hDlg,IDC_VU1_VF02);
        VU1F03Handle=GetDlgItem(hDlg,IDC_VU1_VF03);
        VU1F04Handle=GetDlgItem(hDlg,IDC_VU1_VF04);
        VU1F05Handle=GetDlgItem(hDlg,IDC_VU1_VF05);
        VU1F06Handle=GetDlgItem(hDlg,IDC_VU1_VF06);
        VU1F07Handle=GetDlgItem(hDlg,IDC_VU1_VF07);
        VU1F08Handle=GetDlgItem(hDlg,IDC_VU1_VF08);
        VU1F09Handle=GetDlgItem(hDlg,IDC_VU1_VF09);
        VU1F10Handle=GetDlgItem(hDlg,IDC_VU1_VF10);
        VU1F11Handle=GetDlgItem(hDlg,IDC_VU1_VF11);
        VU1F12Handle=GetDlgItem(hDlg,IDC_VU1_VF12);
        VU1F13Handle=GetDlgItem(hDlg,IDC_VU1_VF13);
        VU1F14Handle=GetDlgItem(hDlg,IDC_VU1_VF14);
        VU1F15Handle=GetDlgItem(hDlg,IDC_VU1_VF15);
        VU1F16Handle=GetDlgItem(hDlg,IDC_VU1_VF16);
        VU1F17Handle=GetDlgItem(hDlg,IDC_VU1_VF17);
        VU1F18Handle=GetDlgItem(hDlg,IDC_VU1_VF18);
        VU1F19Handle=GetDlgItem(hDlg,IDC_VU1_VF19);
        VU1F20Handle=GetDlgItem(hDlg,IDC_VU1_VF20);
        VU1F21Handle=GetDlgItem(hDlg,IDC_VU1_VF21);
        VU1F22Handle=GetDlgItem(hDlg,IDC_VU1_VF22);
        VU1F23Handle=GetDlgItem(hDlg,IDC_VU1_VF23);
        VU1F24Handle=GetDlgItem(hDlg,IDC_VU1_VF24);
        VU1F25Handle=GetDlgItem(hDlg,IDC_VU1_VF25);
        VU1F26Handle=GetDlgItem(hDlg,IDC_VU1_VF26);
        VU1F27Handle=GetDlgItem(hDlg,IDC_VU1_VF27);
        VU1F28Handle=GetDlgItem(hDlg,IDC_VU1_VF28);
        VU1F29Handle=GetDlgItem(hDlg,IDC_VU1_VF29);
        VU1F30Handle=GetDlgItem(hDlg,IDC_VU1_VF30);
        VU1F31Handle=GetDlgItem(hDlg,IDC_VU1_VF31);
		UpdateRegs();
	    return (TRUE);
		break;
	case WM_COMMAND:
 
			switch(LOWORD(wParam))
		{
		case (IDOK || IDCANCEL):
			EndDialog(hDlg,TRUE);
			return(TRUE);
			break;
	
		}
		break;
	}

	return(FALSE);
}
LRESULT CALLBACK VU1Creg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
        VU1C00Handle=GetDlgItem(hDlg,IDC_VU1_VI00);
        VU1C01Handle=GetDlgItem(hDlg,IDC_VU1_VI01);
        VU1C02Handle=GetDlgItem(hDlg,IDC_VU1_VI02);
        VU1C03Handle=GetDlgItem(hDlg,IDC_VU1_VI03);
        VU1C04Handle=GetDlgItem(hDlg,IDC_VU1_VI04);
        VU1C05Handle=GetDlgItem(hDlg,IDC_VU1_VI05);
        VU1C06Handle=GetDlgItem(hDlg,IDC_VU1_VI06);
        VU1C07Handle=GetDlgItem(hDlg,IDC_VU1_VI07);
        VU1C08Handle=GetDlgItem(hDlg,IDC_VU1_VI08);
        VU1C09Handle=GetDlgItem(hDlg,IDC_VU1_VI09);
        VU1C10Handle=GetDlgItem(hDlg,IDC_VU1_VI10);
        VU1C11Handle=GetDlgItem(hDlg,IDC_VU1_VI11);
        VU1C12Handle=GetDlgItem(hDlg,IDC_VU1_VI12);
        VU1C13Handle=GetDlgItem(hDlg,IDC_VU1_VI13);
        VU1C14Handle=GetDlgItem(hDlg,IDC_VU1_VI14);
        VU1C15Handle=GetDlgItem(hDlg,IDC_VU1_VI15);
        VU1C16Handle=GetDlgItem(hDlg,IDC_VU1_VI16);
        VU1C17Handle=GetDlgItem(hDlg,IDC_VU1_VI17);
        VU1C18Handle=GetDlgItem(hDlg,IDC_VU1_VI18);
        VU1C19Handle=GetDlgItem(hDlg,IDC_VU1_VI19);
        VU1C20Handle=GetDlgItem(hDlg,IDC_VU1_VI20);
        VU1C21Handle=GetDlgItem(hDlg,IDC_VU1_VI21);
        VU1C22Handle=GetDlgItem(hDlg,IDC_VU1_VI22);
        VU1C23Handle=GetDlgItem(hDlg,IDC_VU1_VI23);
        VU1C24Handle=GetDlgItem(hDlg,IDC_VU1_VI24);
        VU1C25Handle=GetDlgItem(hDlg,IDC_VU1_VI25);
        VU1C26Handle=GetDlgItem(hDlg,IDC_VU1_VI26);
        VU1C27Handle=GetDlgItem(hDlg,IDC_VU1_VI27);
        VU1C28Handle=GetDlgItem(hDlg,IDC_VU1_VI28);
        VU1C29Handle=GetDlgItem(hDlg,IDC_VU1_VI29);
        VU1C30Handle=GetDlgItem(hDlg,IDC_VU1_VI30);
        VU1C31Handle=GetDlgItem(hDlg,IDC_VU1_VI31);
        VU1ACCHandle=GetDlgItem(hDlg,IDC_VU1_ACC);
		UpdateRegs();
	    return (TRUE);
		break;
	case WM_COMMAND:
 
			switch(LOWORD(wParam))
		{
		case (IDOK || IDCANCEL):
			EndDialog(hDlg,TRUE);
			return(TRUE);
			break;
	
		}
		break;
	}

	return(FALSE);
}

void UpdateRegs(void)
{

    wsprintf(text1,"0x%08X\0",psxRegs.GPR.r[0]);
	SendMessage(IOPGPR0Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X\0",psxRegs.GPR.r[1]);
	SendMessage(IOPGPR1Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X\0",psxRegs.GPR.r[2]);
	SendMessage(IOPGPR2Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X\0",psxRegs.GPR.r[3]);
	SendMessage(IOPGPR3Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X\0",psxRegs.GPR.r[4]);
	SendMessage(IOPGPR4Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X\0",psxRegs.GPR.r[5]);
	SendMessage(IOPGPR5Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X\0",psxRegs.GPR.r[6]);
	SendMessage(IOPGPR6Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X\0",psxRegs.GPR.r[7]);
	SendMessage(IOPGPR7Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X\0",psxRegs.GPR.r[8]);
	SendMessage(IOPGPR8Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X\0",psxRegs.GPR.r[9]);
	SendMessage(IOPGPR9Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X\0",psxRegs.GPR.r[10]);
	SendMessage(IOPGPR10Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X\0",psxRegs.GPR.r[11]);
	SendMessage(IOPGPR11Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X\0",psxRegs.GPR.r[12]);
	SendMessage(IOPGPR12Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X\0",psxRegs.GPR.r[13]);
	SendMessage(IOPGPR13Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X\0",psxRegs.GPR.r[14]);
	SendMessage(IOPGPR14Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X\0",psxRegs.GPR.r[15]);
	SendMessage(IOPGPR15Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X\0",psxRegs.GPR.r[16]);
	SendMessage(IOPGPR16Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X\0",psxRegs.GPR.r[17]);
	SendMessage(IOPGPR17Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X\0",psxRegs.GPR.r[18]);
	SendMessage(IOPGPR18Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X\0",psxRegs.GPR.r[19]);
	SendMessage(IOPGPR19Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X\0",psxRegs.GPR.r[20]);
	SendMessage(IOPGPR20Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X\0",psxRegs.GPR.r[21]);
	SendMessage(IOPGPR21Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X\0",psxRegs.GPR.r[22]);
	SendMessage(IOPGPR22Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X\0",psxRegs.GPR.r[23]);
	SendMessage(IOPGPR23Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X\0",psxRegs.GPR.r[24]);
	SendMessage(IOPGPR24Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X\0",psxRegs.GPR.r[25]);
	SendMessage(IOPGPR25Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X\0",psxRegs.GPR.r[26]);
	SendMessage(IOPGPR26Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X\0",psxRegs.GPR.r[27]);
	SendMessage(IOPGPR27Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X\0",psxRegs.GPR.r[28]);
	SendMessage(IOPGPR28Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X\0",psxRegs.GPR.r[29]);
	SendMessage(IOPGPR29Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X\0",psxRegs.GPR.r[30]);
	SendMessage(IOPGPR30Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X\0",psxRegs.GPR.r[31]);
	SendMessage(IOPGPR31Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"%x",psxRegs.pc  );
	SendMessage(IOPGPRPCHandle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X\0",psxRegs.GPR.r[32]);
	SendMessage(IOPGPRHIHandle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X\0",psxRegs.GPR.r[33]);
	SendMessage(IOPGPRLOHandle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);   

    wsprintf(text1,"0x%08X_%08X_%08X_%08X\0",cpuRegs.GPR.r[0].UL[3],cpuRegs.GPR.r[0].UL[2],cpuRegs.GPR.r[0].UL[1],cpuRegs.GPR.r[0].UL[0] );
	SendMessage(GPR0Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X_%08X_%08X_%08X\0",cpuRegs.GPR.r[1].UL[3], cpuRegs.GPR.r[1].UL[2],cpuRegs.GPR.r[1].UL[1],cpuRegs.GPR.r[1].UL[0] );
	SendMessage(GPR1Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X_%08X_%08X_%08X\0",cpuRegs.GPR.r[2].UL[3],cpuRegs.GPR.r[2].UL[2], cpuRegs.GPR.r[2].UL[1],cpuRegs.GPR.r[2].UL[0]);
	SendMessage(GPR2Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X_%08X_%08X_%08X\0",cpuRegs.GPR.r[3].UL[3],cpuRegs.GPR.r[3].UL[2], cpuRegs.GPR.r[3].UL[1],cpuRegs.GPR.r[3].UL[0] );
	SendMessage(GPR3Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X_%08X_%08X_%08X\0",cpuRegs.GPR.r[4].UL[3],cpuRegs.GPR.r[4].UL[2], cpuRegs.GPR.r[4].UL[1],cpuRegs.GPR.r[4].UL[0] );
	SendMessage(GPR4Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X_%08X_%08X_%08X\0",cpuRegs.GPR.r[5].UL[3],cpuRegs.GPR.r[5].UL[2],cpuRegs.GPR.r[5].UL[1], cpuRegs.GPR.r[5].UL[0] );
	SendMessage(GPR5Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X_%08X_%08X_%08X\0",cpuRegs.GPR.r[6].UL[3],cpuRegs.GPR.r[6].UL[2], cpuRegs.GPR.r[6].UL[1], cpuRegs.GPR.r[6].UL[0]);
	SendMessage(GPR6Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X_%08X_%08X_%08X\0",cpuRegs.GPR.r[7].UL[3], cpuRegs.GPR.r[7].UL[2],cpuRegs.GPR.r[7].UL[1],cpuRegs.GPR.r[7].UL[0] );
	SendMessage(GPR7Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X_%08X_%08X_%08X\0",cpuRegs.GPR.r[8].UL[3],cpuRegs.GPR.r[8].UL[2],cpuRegs.GPR.r[8].UL[1],cpuRegs.GPR.r[8].UL[0]  );
	SendMessage(GPR8Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X_%08X_%08X_%08X\0",cpuRegs.GPR.r[9].UL[3],cpuRegs.GPR.r[9].UL[2],cpuRegs.GPR.r[9].UL[1], cpuRegs.GPR.r[9].UL[0]  );
	SendMessage(GPR9Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X_%08X_%08X_%08X\0",cpuRegs.GPR.r[10].UL[3],cpuRegs.GPR.r[10].UL[2],cpuRegs.GPR.r[10].UL[1],cpuRegs.GPR.r[10].UL[0]  );
	SendMessage(GPR10Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X_%08X_%08X_%08X\0",cpuRegs.GPR.r[11].UL[3],cpuRegs.GPR.r[11].UL[2],cpuRegs.GPR.r[11].UL[1],cpuRegs.GPR.r[11].UL[0]  );
	SendMessage(GPR11Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X_%08X_%08X_%08X\0",cpuRegs.GPR.r[12].UL[3],cpuRegs.GPR.r[12].UL[2],cpuRegs.GPR.r[12].UL[1],cpuRegs.GPR.r[12].UL[0]  );
	SendMessage(GPR12Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X_%08X_%08X_%08X\0",cpuRegs.GPR.r[13].UL[3],cpuRegs.GPR.r[13].UL[2],cpuRegs.GPR.r[13].UL[1],cpuRegs.GPR.r[13].UL[0]  );
	SendMessage(GPR13Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X_%08X_%08X_%08X\0",cpuRegs.GPR.r[14].UL[3],cpuRegs.GPR.r[14].UL[2],cpuRegs.GPR.r[14].UL[1],cpuRegs.GPR.r[14].UL[0]  );
	SendMessage(GPR14Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X_%08X_%08X_%08X\0",cpuRegs.GPR.r[15].UL[3],cpuRegs.GPR.r[15].UL[2],cpuRegs.GPR.r[15].UL[1],cpuRegs.GPR.r[15].UL[0]  );
	SendMessage(GPR15Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X_%08X_%08X_%08X\0",cpuRegs.GPR.r[16].UL[3],cpuRegs.GPR.r[16].UL[2],cpuRegs.GPR.r[16].UL[1],cpuRegs.GPR.r[16].UL[0]  );
	SendMessage(GPR16Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X_%08X_%08X_%08X\0",cpuRegs.GPR.r[17].UL[3],cpuRegs.GPR.r[17].UL[2],cpuRegs.GPR.r[17].UL[1],cpuRegs.GPR.r[17].UL[0]  );
	SendMessage(GPR17Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X_%08X_%08X_%08X\0",cpuRegs.GPR.r[18].UL[3],cpuRegs.GPR.r[18].UL[2],cpuRegs.GPR.r[18].UL[1],cpuRegs.GPR.r[18].UL[0]  );
	SendMessage(GPR18Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X_%08X_%08X_%08X\0",cpuRegs.GPR.r[19].UL[3],cpuRegs.GPR.r[19].UL[2],cpuRegs.GPR.r[19].UL[1],cpuRegs.GPR.r[19].UL[0]  );
	SendMessage(GPR19Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X_%08X_%08X_%08X\0",cpuRegs.GPR.r[20].UL[3],cpuRegs.GPR.r[20].UL[2],cpuRegs.GPR.r[20].UL[1],cpuRegs.GPR.r[20].UL[0]  );
	SendMessage(GPR20Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X_%08X_%08X_%08X\0",cpuRegs.GPR.r[21].UL[3],cpuRegs.GPR.r[21].UL[2],cpuRegs.GPR.r[21].UL[1],cpuRegs.GPR.r[21].UL[0]  );
	SendMessage(GPR21Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X_%08X_%08X_%08X\0",cpuRegs.GPR.r[22].UL[3],cpuRegs.GPR.r[22].UL[2],cpuRegs.GPR.r[22].UL[1],cpuRegs.GPR.r[22].UL[0]  );
	SendMessage(GPR22Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X_%08X_%08X_%08X\0",cpuRegs.GPR.r[23].UL[3],cpuRegs.GPR.r[23].UL[2],cpuRegs.GPR.r[23].UL[1],cpuRegs.GPR.r[23].UL[0]  );
	SendMessage(GPR23Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X_%08X_%08X_%08X\0",cpuRegs.GPR.r[24].UL[3],cpuRegs.GPR.r[24].UL[2],cpuRegs.GPR.r[24].UL[1],cpuRegs.GPR.r[24].UL[0]  );
	SendMessage(GPR24Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X_%08X_%08X_%08X\0",cpuRegs.GPR.r[25].UL[3],cpuRegs.GPR.r[25].UL[2],cpuRegs.GPR.r[25].UL[1],cpuRegs.GPR.r[25].UL[0]  );
	SendMessage(GPR25Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X_%08X_%08X_%08X\0",cpuRegs.GPR.r[26].UL[3],cpuRegs.GPR.r[26].UL[2],cpuRegs.GPR.r[26].UL[1],cpuRegs.GPR.r[26].UL[0]  );
	SendMessage(GPR26Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X_%08X_%08X_%08X\0",cpuRegs.GPR.r[27].UL[3],cpuRegs.GPR.r[27].UL[2],cpuRegs.GPR.r[27].UL[1],cpuRegs.GPR.r[27].UL[0]  );
	SendMessage(GPR27Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X_%08X_%08X_%08X\0",cpuRegs.GPR.r[28].UL[3],cpuRegs.GPR.r[28].UL[2],cpuRegs.GPR.r[28].UL[1],cpuRegs.GPR.r[28].UL[0]  );
	SendMessage(GPR28Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X_%08X_%08X_%08X\0",cpuRegs.GPR.r[29].UL[3],cpuRegs.GPR.r[29].UL[2],cpuRegs.GPR.r[29].UL[1],cpuRegs.GPR.r[29].UL[0]  );
	SendMessage(GPR29Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X_%08X_%08X_%08X\0",cpuRegs.GPR.r[30].UL[3],cpuRegs.GPR.r[30].UL[2],cpuRegs.GPR.r[30].UL[1],cpuRegs.GPR.r[30].UL[0]  );
	SendMessage(GPR30Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X_%08X_%08X_%08X\0",cpuRegs.GPR.r[31].UL[3],cpuRegs.GPR.r[31].UL[2],cpuRegs.GPR.r[31].UL[1],cpuRegs.GPR.r[31].UL[0]  );
	SendMessage(GPR31Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"%x",cpuRegs.pc  );
	SendMessage(GPRPCHandle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X_%08X_%08X_%08X\0",cpuRegs.HI.UL[3],cpuRegs.HI.UL[2] ,cpuRegs.HI.UL[1] ,cpuRegs.HI.UL[0]   );
	SendMessage(GPRHIHandle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"0x%08X_%08X_%08X_%08X\0\0",cpuRegs.LO.UL[3],cpuRegs.LO.UL[2],cpuRegs.LO.UL[1],cpuRegs.LO.UL[0]  );
	SendMessage(GPRLOHandle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	wsprintf(text1,"%x",cpuRegs.CP0.r[0] );
	SendMessage(COP00Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",cpuRegs.CP0.r[1]);
	SendMessage(COP01Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",cpuRegs.CP0.r[2]);
	SendMessage(COP02Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",cpuRegs.CP0.r[3]);
	SendMessage(COP03Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",cpuRegs.CP0.r[4]);
	SendMessage(COP04Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",cpuRegs.CP0.r[5]);
	SendMessage(COP05Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",cpuRegs.CP0.r[6]);
	SendMessage(COP06Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",cpuRegs.CP0.r[7]);
	SendMessage(COP07Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",cpuRegs.CP0.r[8]);
	SendMessage(COP08Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",cpuRegs.CP0.r[9]);
	SendMessage(COP09Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",cpuRegs.CP0.r[10]);
	SendMessage(COP010Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",cpuRegs.CP0.r[11]);
	SendMessage(COP011Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",cpuRegs.CP0.r[12]);
	SendMessage(COP012Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",cpuRegs.CP0.r[13]);
	SendMessage(COP013Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",cpuRegs.CP0.r[14]);
	SendMessage(COP014Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",cpuRegs.CP0.r[15]);
	SendMessage(COP015Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",cpuRegs.CP0.r[16]);
	SendMessage(COP016Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",cpuRegs.CP0.r[17]);
	SendMessage(COP017Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",cpuRegs.CP0.r[18]);
	SendMessage(COP018Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",cpuRegs.CP0.r[19]);
	SendMessage(COP019Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",cpuRegs.CP0.r[20]);
	SendMessage(COP020Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",cpuRegs.CP0.r[21]);
	SendMessage(COP021Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",cpuRegs.CP0.r[22]);
	SendMessage(COP022Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",cpuRegs.CP0.r[23]);
	SendMessage(COP023Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",cpuRegs.CP0.r[24]);
	SendMessage(COP024Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",cpuRegs.CP0.r[25]);
	SendMessage(COP025Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",cpuRegs.CP0.r[26]);
	SendMessage(COP026Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",cpuRegs.CP0.r[27]);
	SendMessage(COP027Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",cpuRegs.CP0.r[28]);
	SendMessage(COP028Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",cpuRegs.CP0.r[29]);
	SendMessage(COP029Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",cpuRegs.CP0.r[30]);
	SendMessage(COP030Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",cpuRegs.CP0.r[31]);
	SendMessage(COP031Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
   
	sprintf(text1,"%f",fpuRegs.fpr[0].f );
	SendMessage(COP10Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    sprintf(text1,"%f",fpuRegs.fpr[1].f);
	SendMessage(COP11Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    sprintf(text1,"%f",fpuRegs.fpr[2].f);
	SendMessage(COP12Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    sprintf(text1,"%f",fpuRegs.fpr[3].f);
	SendMessage(COP13Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    sprintf(text1,"%f",fpuRegs.fpr[4].f);
	SendMessage(COP14Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    sprintf(text1,"%f",fpuRegs.fpr[5].f);
	SendMessage(COP15Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    sprintf(text1,"%f",fpuRegs.fpr[6].f);
	SendMessage(COP16Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    sprintf(text1,"%f",fpuRegs.fpr[7].f);
	SendMessage(COP17Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    sprintf(text1,"%f",fpuRegs.fpr[8].f);
	SendMessage(COP18Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    sprintf(text1,"%f",fpuRegs.fpr[9].f);
	SendMessage(COP19Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    sprintf(text1,"%f",fpuRegs.fpr[10].f);
	SendMessage(COP110Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    sprintf(text1,"%f",fpuRegs.fpr[11].f);
	SendMessage(COP111Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    sprintf(text1,"%f",fpuRegs.fpr[12].f);
	SendMessage(COP112Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    sprintf(text1,"%f",fpuRegs.fpr[13].f);
	SendMessage(COP113Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    sprintf(text1,"%f",fpuRegs.fpr[14].f);
	SendMessage(COP114Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    sprintf(text1,"%f",fpuRegs.fpr[15].f);
	SendMessage(COP115Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    sprintf(text1,"%f",fpuRegs.fpr[16].f);
	SendMessage(COP116Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    sprintf(text1,"%f",fpuRegs.fpr[17].f);
	SendMessage(COP117Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    sprintf(text1,"%f",fpuRegs.fpr[18].f);
	SendMessage(COP118Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    sprintf(text1,"%f",fpuRegs.fpr[19].f);
	SendMessage(COP119Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    sprintf(text1,"%f",fpuRegs.fpr[20].f);
	SendMessage(COP120Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    sprintf(text1,"%f",fpuRegs.fpr[21].f);
	SendMessage(COP121Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    sprintf(text1,"%f",fpuRegs.fpr[22].f);
	SendMessage(COP122Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    sprintf(text1,"%f",fpuRegs.fpr[23].f);
	SendMessage(COP123Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    sprintf(text1,"%f",fpuRegs.fpr[24].f);
	SendMessage(COP124Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    sprintf(text1,"%f",fpuRegs.fpr[25].f);
	SendMessage(COP125Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    sprintf(text1,"%f",fpuRegs.fpr[26].f);
	SendMessage(COP126Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    sprintf(text1,"%f",fpuRegs.fpr[27].f);
	SendMessage(COP127Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    sprintf(text1,"%f",fpuRegs.fpr[28].f);
	SendMessage(COP128Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    sprintf(text1,"%f",fpuRegs.fpr[29].f);
	SendMessage(COP129Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    sprintf(text1,"%f",fpuRegs.fpr[30].f);
	SendMessage(COP130Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    sprintf(text1,"%f",fpuRegs.fpr[31].f);
	SendMessage(COP131Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",fpuRegs.fprc[0]);
	SendMessage(COP1C0Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",fpuRegs.fprc[31]);
	SendMessage(COP1C1Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    sprintf(text1,"%f",fpuRegs.ACC.f);
	SendMessage(COP1ACCHandle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);


    sprintf(text1,"%f_%f_%f_%f\0",VU0.VF[0].f.w,VU0.VF[0].f.z,VU0.VF[0].f.y,VU0.VF[0].f.x );
	SendMessage(VU0F00Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU0.VF[1].f.w,VU0.VF[1].f.z,VU0.VF[1].f.y,VU0.VF[1].f.x  );
	SendMessage(VU0F01Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU0.VF[2].f.w,VU0.VF[2].f.z,VU0.VF[2].f.y,VU0.VF[2].f.x );
	SendMessage(VU0F02Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU0.VF[3].f.w,VU0.VF[3].f.z,VU0.VF[3].f.y,VU0.VF[3].f.x  );
	SendMessage(VU0F03Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU0.VF[4].f.w,VU0.VF[4].f.z,VU0.VF[4].f.y,VU0.VF[4].f.x );
	SendMessage(VU0F04Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU0.VF[5].f.w,VU0.VF[5].f.z,VU0.VF[5].f.y,VU0.VF[5].f.x); 
	SendMessage(VU0F05Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU0.VF[6].f.w,VU0.VF[6].f.z,VU0.VF[6].f.y,VU0.VF[6].f.x );
	SendMessage(VU0F06Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU0.VF[7].f.w,VU0.VF[7].f.z,VU0.VF[7].f.y,VU0.VF[7].f.x );
	SendMessage(VU0F07Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU0.VF[8].f.w,VU0.VF[8].f.z,VU0.VF[8].f.y,VU0.VF[8].f.x  );
	SendMessage(VU0F08Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU0.VF[9].f.w,VU0.VF[9].f.z,VU0.VF[9].f.y,VU0.VF[9].f.x  );
	SendMessage(VU0F09Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU0.VF[10].f.w,VU0.VF[10].f.z,VU0.VF[10].f.y,VU0.VF[10].f.x  );
	SendMessage(VU0F10Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU0.VF[11].f.w,VU0.VF[11].f.z,VU0.VF[11].f.y,VU0.VF[11].f.x   );
	SendMessage(VU0F11Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU0.VF[12].f.w,VU0.VF[12].f.z,VU0.VF[12].f.y,VU0.VF[12].f.x   );
	SendMessage(VU0F12Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU0.VF[13].f.w,VU0.VF[13].f.z,VU0.VF[13].f.y,VU0.VF[13].f.x   );
	SendMessage(VU0F13Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU0.VF[14].f.w,VU0.VF[14].f.z,VU0.VF[14].f.y,VU0.VF[14].f.x   );
	SendMessage(VU0F14Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU0.VF[15].f.w,VU0.VF[15].f.z,VU0.VF[15].f.y,VU0.VF[15].f.x   );
	SendMessage(VU0F15Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU0.VF[16].f.w,VU0.VF[16].f.z,VU0.VF[16].f.y,VU0.VF[16].f.x   );
	SendMessage(VU0F16Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU0.VF[17].f.w,VU0.VF[17].f.z,VU0.VF[17].f.y,VU0.VF[17].f.x   );
	SendMessage(VU0F17Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU0.VF[18].f.w,VU0.VF[18].f.z,VU0.VF[18].f.y,VU0.VF[18].f.x   );
	SendMessage(VU0F18Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU0.VF[19].f.w,VU0.VF[19].f.z,VU0.VF[19].f.y,VU0.VF[19].f.x   );
	SendMessage(VU0F19Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU0.VF[20].f.w,VU0.VF[20].f.z,VU0.VF[20].f.y,VU0.VF[20].f.x   );
	SendMessage(VU0F20Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU0.VF[21].f.w,VU0.VF[21].f.z,VU0.VF[21].f.y,VU0.VF[21].f.x   );
	SendMessage(VU0F21Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU0.VF[22].f.w,VU0.VF[22].f.z,VU0.VF[22].f.y,VU0.VF[22].f.x  );
	SendMessage(VU0F22Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU0.VF[23].f.w,VU0.VF[23].f.z,VU0.VF[23].f.y,VU0.VF[23].f.x  );
	SendMessage(VU0F23Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU0.VF[24].f.w,VU0.VF[24].f.z,VU0.VF[24].f.y,VU0.VF[24].f.x   );
	SendMessage(VU0F24Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU0.VF[25].f.w,VU0.VF[25].f.z,VU0.VF[25].f.y,VU0.VF[25].f.x   );
	SendMessage(VU0F25Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU0.VF[26].f.w,VU0.VF[26].f.z,VU0.VF[26].f.y,VU0.VF[26].f.x   );
	SendMessage(VU0F26Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU0.VF[27].f.w,VU0.VF[27].f.z,VU0.VF[27].f.y,VU0.VF[27].f.x   );
	SendMessage(VU0F27Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU0.VF[28].f.w,VU0.VF[28].f.z,VU0.VF[28].f.y,VU0.VF[28].f.x  );
	SendMessage(VU0F28Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU0.VF[29].f.w,VU0.VF[29].f.z,VU0.VF[29].f.y,VU0.VF[29].f.x   );
	SendMessage(VU0F29Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU0.VF[30].f.w,VU0.VF[30].f.z,VU0.VF[30].f.y,VU0.VF[30].f.x   );
	SendMessage(VU0F30Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU0.VF[31].f.w,VU0.VF[31].f.z,VU0.VF[31].f.y,VU0.VF[31].f.x   );
	SendMessage(VU0F31Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);


    wsprintf(text1,"%x",VU0.VI[0] );
	SendMessage(VU0C00Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU0.VI[1]);
	SendMessage(VU0C01Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU0.VI[2]);
	SendMessage(VU0C02Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU0.VI[3]);
	SendMessage(VU0C03Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU0.VI[4]);
	SendMessage(VU0C04Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU0.VI[5]);
	SendMessage(VU0C05Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU0.VI[6]);
	SendMessage(VU0C06Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU0.VI[7]);
	SendMessage(VU0C07Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU0.VI[8]);
	SendMessage(VU0C08Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU0.VI[9]);
	SendMessage(VU0C09Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU0.VI[10]);
	SendMessage(VU0C10Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU0.VI[11]);
	SendMessage(VU0C11Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU0.VI[12]);
	SendMessage(VU0C12Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU0.VI[13]);
	SendMessage(VU0C13Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU0.VI[14]);
	SendMessage(VU0C14Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU0.VI[15]);
	SendMessage(VU0C15Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU0.VI[16]);
	SendMessage(VU0C16Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU0.VI[17]);
	SendMessage(VU0C17Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU0.VI[18]);
	SendMessage(VU0C18Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU0.VI[19]);
	SendMessage(VU0C19Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU0.VI[20]);
	SendMessage(VU0C20Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU0.VI[21]);
	SendMessage(VU0C21Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU0.VI[22]);
	SendMessage(VU0C22Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU0.VI[23]);
	SendMessage(VU0C23Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU0.VI[24]);
	SendMessage(VU0C24Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU0.VI[25]);
	SendMessage(VU0C25Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU0.VI[26]);
	SendMessage(VU0C26Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU0.VI[27]);
	SendMessage(VU0C27Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU0.VI[28]);
	SendMessage(VU0C28Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU0.VI[29]);
	SendMessage(VU0C29Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU0.VI[30]);
	SendMessage(VU0C30Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU0.VI[31]);
	SendMessage(VU0C31Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    
    sprintf(text1,"%f_%f_%f_%f\0",VU0.ACC.f.w,VU0.ACC.f.z,VU0.ACC.f.y,VU0.ACC.f.x );
	SendMessage(VU0ACCHandle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);



    sprintf(text1,"%f_%f_%f_%f\0",VU1.VF[0].f.w,VU1.VF[0].f.z,VU1.VF[0].f.y,VU1.VF[0].f.x );
	SendMessage(VU1F00Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU1.VF[1].f.w,VU1.VF[1].f.z,VU1.VF[1].f.y,VU1.VF[1].f.x  );
	SendMessage(VU1F01Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU1.VF[2].f.w,VU1.VF[2].f.z,VU1.VF[2].f.y,VU1.VF[2].f.x );
	SendMessage(VU1F02Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU1.VF[3].f.w,VU1.VF[3].f.z,VU1.VF[3].f.y,VU1.VF[3].f.x  );
	SendMessage(VU1F03Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU1.VF[4].f.w,VU1.VF[4].f.z,VU1.VF[4].f.y,VU1.VF[4].f.x );
	SendMessage(VU1F04Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU1.VF[5].f.w,VU1.VF[5].f.z,VU1.VF[5].f.y,VU1.VF[5].f.x); 
	SendMessage(VU1F05Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU1.VF[6].f.w,VU1.VF[6].f.z,VU1.VF[6].f.y,VU1.VF[6].f.x );
	SendMessage(VU1F06Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU1.VF[7].f.w,VU1.VF[7].f.z,VU1.VF[7].f.y,VU1.VF[7].f.x );
	SendMessage(VU1F07Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU1.VF[8].f.w,VU1.VF[8].f.z,VU1.VF[8].f.y,VU1.VF[8].f.x  );
	SendMessage(VU1F08Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU1.VF[9].f.w,VU1.VF[9].f.z,VU1.VF[9].f.y,VU1.VF[9].f.x  );
	SendMessage(VU1F09Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU1.VF[10].f.w,VU1.VF[10].f.z,VU1.VF[10].f.y,VU1.VF[10].f.x  );
	SendMessage(VU1F10Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU1.VF[11].f.w,VU1.VF[11].f.z,VU1.VF[11].f.y,VU1.VF[11].f.x   );
	SendMessage(VU1F11Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU1.VF[12].f.w,VU1.VF[12].f.z,VU1.VF[12].f.y,VU1.VF[12].f.x   );
	SendMessage(VU1F12Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU1.VF[13].f.w,VU1.VF[13].f.z,VU1.VF[13].f.y,VU1.VF[13].f.x   );
	SendMessage(VU1F13Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU1.VF[14].f.w,VU1.VF[14].f.z,VU1.VF[14].f.y,VU1.VF[14].f.x   );
	SendMessage(VU1F14Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU1.VF[15].f.w,VU1.VF[15].f.z,VU1.VF[15].f.y,VU1.VF[15].f.x   );
	SendMessage(VU1F15Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU1.VF[16].f.w,VU1.VF[16].f.z,VU1.VF[16].f.y,VU1.VF[16].f.x   );
	SendMessage(VU1F16Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU1.VF[17].f.w,VU1.VF[17].f.z,VU1.VF[17].f.y,VU1.VF[17].f.x   );
	SendMessage(VU1F17Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU1.VF[18].f.w,VU1.VF[18].f.z,VU1.VF[18].f.y,VU1.VF[18].f.x   );
	SendMessage(VU1F18Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU1.VF[19].f.w,VU1.VF[19].f.z,VU1.VF[19].f.y,VU1.VF[19].f.x   );
	SendMessage(VU1F19Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU1.VF[20].f.w,VU1.VF[20].f.z,VU1.VF[20].f.y,VU1.VF[20].f.x   );
	SendMessage(VU1F20Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU1.VF[21].f.w,VU1.VF[21].f.z,VU1.VF[21].f.y,VU1.VF[21].f.x   );
	SendMessage(VU1F21Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU1.VF[22].f.w,VU1.VF[22].f.z,VU1.VF[22].f.y,VU1.VF[22].f.x  );
	SendMessage(VU1F22Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU1.VF[23].f.w,VU1.VF[23].f.z,VU1.VF[23].f.y,VU1.VF[23].f.x  );
	SendMessage(VU1F23Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU1.VF[24].f.w,VU1.VF[24].f.z,VU1.VF[24].f.y,VU1.VF[24].f.x   );
	SendMessage(VU1F24Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU1.VF[25].f.w,VU1.VF[25].f.z,VU1.VF[25].f.y,VU1.VF[25].f.x   );
	SendMessage(VU1F25Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU1.VF[26].f.w,VU1.VF[26].f.z,VU1.VF[26].f.y,VU1.VF[26].f.x   );
	SendMessage(VU1F26Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU1.VF[27].f.w,VU1.VF[27].f.z,VU1.VF[27].f.y,VU1.VF[27].f.x   );
	SendMessage(VU1F27Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU1.VF[28].f.w,VU1.VF[28].f.z,VU1.VF[28].f.y,VU1.VF[28].f.x  );
	SendMessage(VU1F28Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU1.VF[29].f.w,VU1.VF[29].f.z,VU1.VF[29].f.y,VU1.VF[29].f.x   );
	SendMessage(VU1F29Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU1.VF[30].f.w,VU1.VF[30].f.z,VU1.VF[30].f.y,VU1.VF[30].f.x   );
	SendMessage(VU1F30Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
	sprintf(text1,"%f_%f_%f_%f\0",VU1.VF[31].f.w,VU1.VF[31].f.z,VU1.VF[31].f.y,VU1.VF[31].f.x   );
	SendMessage(VU1F31Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);


    wsprintf(text1,"%x",VU1.VI[0] );
	SendMessage(VU1C00Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU1.VI[1]);
	SendMessage(VU1C01Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU1.VI[2]);
	SendMessage(VU1C02Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU1.VI[3]);
	SendMessage(VU1C03Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU1.VI[4]);
	SendMessage(VU1C04Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU1.VI[5]);
	SendMessage(VU1C05Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU1.VI[6]);
	SendMessage(VU1C06Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU1.VI[7]);
	SendMessage(VU1C07Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU1.VI[8]);
	SendMessage(VU1C08Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU1.VI[9]);
	SendMessage(VU1C09Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU1.VI[10]);
	SendMessage(VU1C10Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU1.VI[11]);
	SendMessage(VU1C11Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU1.VI[12]);
	SendMessage(VU1C12Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU1.VI[13]);
	SendMessage(VU1C13Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU1.VI[14]);
	SendMessage(VU1C14Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU1.VI[15]);
	SendMessage(VU1C15Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU1.VI[16]);
	SendMessage(VU1C16Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU1.VI[17]);
	SendMessage(VU1C17Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU1.VI[18]);
	SendMessage(VU1C18Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU1.VI[19]);
	SendMessage(VU1C19Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU1.VI[20]);
	SendMessage(VU1C20Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU1.VI[21]);
	SendMessage(VU1C21Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU1.VI[22]);
	SendMessage(VU1C22Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU1.VI[23]);
	SendMessage(VU1C23Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU1.VI[24]);
	SendMessage(VU1C24Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU1.VI[25]);
	SendMessage(VU1C25Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU1.VI[26]);
	SendMessage(VU1C26Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU1.VI[27]);
	SendMessage(VU1C27Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU1.VI[28]);
	SendMessage(VU1C28Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU1.VI[29]);
	SendMessage(VU1C29Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU1.VI[30]);
	SendMessage(VU1C30Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    wsprintf(text1,"%x",VU1.VI[31]);
	SendMessage(VU1C31Handle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);
    
    sprintf(text1,"%f_%f_%f_%f\0",VU1.ACC.f.w,VU1.ACC.f.z,VU1.ACC.f.y,VU1.ACC.f.x );
	SendMessage(VU1ACCHandle,WM_SETTEXT,0,(LPARAM)(LPCTSTR)text1);

}


void EEDumpRegs(FILE * fp)
{
	char text2[256];
	int i;
	for(i = 0; i < 32; i++)
	{
		sprintf(text1,"%x_%x_%x_%x",cpuRegs.GPR.r[i].UL[3],cpuRegs.GPR.r[i].UL[2],cpuRegs.GPR.r[i].UL[1],cpuRegs.GPR.r[i].UL[0]);
		sprintf(text2,"GPR Register %d: ",i+1);
		fprintf(fp,text2);
		fprintf(fp,text1);
		fprintf(fp,"\n");
	}
	sprintf(text1,"0x%x",cpuRegs.pc);
	fprintf(fp,"PC Register : ");
	fprintf(fp,text1);
	fprintf(fp,"\n");
	sprintf(text1,"%x_%x_%x_%x",cpuRegs.HI.UL[3],cpuRegs.HI.UL[2],cpuRegs.HI.UL[1],cpuRegs.HI.UL[0]);
	fprintf(fp,"GPR Register HI: ");
	fprintf(fp,text1);
	fprintf(fp,"\n");
	sprintf(text1,"%x_%x_%x_%x",cpuRegs.LO.UL[3],cpuRegs.LO.UL[2],cpuRegs.LO.UL[1],cpuRegs.LO.UL[0]);
	fprintf(fp,"GPR Register LO: ");
	fprintf(fp,text1);
	fprintf(fp,"\n");


	for(i = 0; i < 32; i++)
	{
		sprintf(text1,"0x%x",cpuRegs.CP0.r[i]);
		sprintf(text2,"COP0 Register %d: ",i+1);
		fprintf(fp,text2);
		fprintf(fp,text1);
		fprintf(fp,"\n");
	}
}

void IOPDumpRegs(FILE * fp)
{
	char text2[256];
	int i;
	for(i = 0; i < 32; i++)
	{
		sprintf(text1,"%x",psxRegs.GPR.r[i]);
		sprintf(text2,"GPR Register %d: ",i+1);
		fprintf(fp,text2);
		fprintf(fp,text1);
		fprintf(fp,"\n");
	}
	sprintf(text1,"0x%x",psxRegs.pc);
	fprintf(fp,"PC Register : ");
	fprintf(fp,text1);
	fprintf(fp,"\n");
	sprintf(text1,"%x",psxRegs.GPR.r[32]);
	fprintf(fp,"GPR Register HI: ");
	fprintf(fp,text1);
	fprintf(fp,"\n");
	sprintf(text1,"%x",psxRegs.GPR.r[33]);
	fprintf(fp,"GPR Register LO: ");
	fprintf(fp,text1);
	fprintf(fp,"\n");
}
