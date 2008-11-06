// Aspi.cpp: implementation of the aspi methods
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cdvd.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define MOVESCSIDWORD(pdwSrc,pdwDst) \
{\
    ((PBYTE)(pdwDst))[0] = ((PBYTE)(pdwSrc))[3];\
    ((PBYTE)(pdwDst))[1] = ((PBYTE)(pdwSrc))[2];\
    ((PBYTE)(pdwDst))[2] = ((PBYTE)(pdwSrc))[1];\
    ((PBYTE)(pdwDst))[3] = ((PBYTE)(pdwSrc))[0];\
}

#define MOVESCSIWORD(pwSrc,pwDst) \
{\
    ((PBYTE)(pwDst))[0] = ((PBYTE)(pwSrc))[1];\
    ((PBYTE)(pwDst))[1] = ((PBYTE)(pwSrc))[0];\
}


/***********************************************************/
/*              ASPI Init/Shutdown related methods         */
/***********************************************************/

// using MFC classes, no aspi in other systems.
int CCdvd::Aspi_CheckValidInstallation()
{
    int retval = CDVD_ERROR_FAIL;

	CFileStatus fstat;
	TCHAR szDir[MAX_PATH];
	
	if(GetSystemDirectory(szDir,MAX_PATH))
	{
		CString directory = szDir;
		WIN32OSTYPE ostype = GetWin32OSType();

		if(ostype != WINNTNEW && ostype != WINNTOLD)
		{
			if(CFile::GetStatus(directory+_T("\\wnaspi32.dll"), fstat) && 
				CFile::GetStatus(directory+_T("\\winaspi.dll"), fstat) &&
				CFile::GetStatus(directory+_T("\\iosubsys\\apix.vxd"), fstat))
            {
                retval = CDVD_ERROR_SUCCESS;
            }
		}
		else
		{
			if(CFile::GetStatus(directory+_T("\\wnaspi32.dll"), fstat) && 
			   CFile::GetStatus(directory+_T("\\drivers\\aspi32.sys"), fstat))
            {
                retval = CDVD_ERROR_SUCCESS;
            }
		}
	}
    else
    {
        // are you crazy? o_O, no fucking windows directory? o_O
        retval = CDVD_ERROR_FAIL;
    }

	return retval;

}

// called once to init Aspi
int CCdvd::Aspi_Init()
{
    int retval = CDVD_ERROR_FAIL;
    if(m_bAspiInitialized) 
        return CDVD_ERROR_SUCCESS;

    if(Aspi_CheckValidInstallation() == CDVD_ERROR_SUCCESS)
    {
        if((m_hAspi = AfxLoadLibrary("WNASPI32")) != NULL)
        {
        	SendASPI32Command    = (CDROMSendASPI32Command)    GetProcAddress(m_hAspi,"SendASPI32Command");
        	GetASPI32SupportInfo = (CDROMGetASPI32SupportInfo) GetProcAddress(m_hAspi,"GetASPI32SupportInfo");
            
            if(SendASPI32Command != NULL &&
               GetASPI32SupportInfo != NULL)
            {
                UI32 support_info = GetASPI32SupportInfo();
                if( HIBYTE(LOWORD(support_info)) != SS_COMP &&
                    HIBYTE(LOWORD(support_info)) != SS_NO_ADAPTERS )
                {
                    Aspi_Shutdown();              
                }
                else
                {
                    if(InitializeBuffers() == CDVD_ERROR_SUCCESS)
                    {
                        if(Aspi_ScsiBusScan() == CDVD_ERROR_SUCCESS)
                        {
                            SetCurrentBuffer(0);
                            m_bAspiInitialized = TRUE;
                            retval = CDVD_ERROR_SUCCESS;
                            GetNumSectors = Aspi_GetNumSectors;
                            GetToc        = Aspi_GetToc;
                        //  GetHeader     = Aspi_GetHeader;
                            SetSpeed      = Aspi_SetSpeed;
                            Stop          = Aspi_Stop;
                            SetSectorSize = Aspi_SetSectorSize;
                            Play          = Aspi_Play;
                            TestReady     = Aspi_TestReady;
                            ReadSector    = Dummy_T4;
                            UpdateInterfaceObject();
                        }
                    }
                }
            }
            else
            {
                Aspi_Shutdown();
            }
        }
    }

    return retval;
}

// shuts down aspi
int CCdvd::Aspi_Shutdown()
{
    int retval = CDVD_ERROR_SUCCESS;

    // gota detect the read_mode first before i do this
    // but just reset to 2048 for now.
    Aspi_SetSectorSize(CDVD_SECTOR_SIZE_DVD);

    // the old ASPI drivers suck shit to hell and damnation,
    // freeing them up immediately after init will crash sometimes
    ::Sleep(200); 

    if(m_hAspi != NULL) 
        AfxFreeLibrary(m_hAspi);
    m_hAspi = NULL;

    SendASPI32Command    = NULL;
    GetASPI32SupportInfo = NULL;               

    GetNumSectors = Dummy_T1;
    GetToc        = Dummy_T1;
    Stop          = Dummy_T1;
    Play          = Dummy_T3;
    TestReady     = Dummy_T1;
//  GetHeader     = Dummy_T1;
    SetSpeed      = Dummy_T2;
    SetSectorSize = Dummy_T2;
    ReadSector    = Dummy_T4;
    UpdateInterfaceObject();

    ShutdownBuffers();

    m_bAspiInitialized = FALSE;

    return retval;
}

// open a drive for reading
int CCdvd::Aspi_OpenDrive(int drv_num)
{
    int retval = CDVD_ERROR_SUCCESS;
    CloseDrive();
    m_nCurrentDrive = drv_num;
    
    return retval;
}

// dummy for now
int CCdvd::Aspi_CloseDrive()
{
    int retval = CDVD_ERROR_SUCCESS;
    m_nCurrentDrive = -1;

    return retval;
}

// get srb status
int CCdvd::Aspi_GetSrbStatus(int srb_num)
{
    int retval = CDVD_SRB_ERROR;
    switch(m_nCurrentReadMode)
    {
    case CDVD_READ_MMC:    retval = m_srbReadCd  [srb_num].SRB_Status; break;
    case CDVD_READ_SCSI10: retval = m_srbRead10  [srb_num].SRB_Status; break;
    case CDVD_READ_D8:     retval = m_srbReadMat [srb_num].SRB_Status; break;      
    case CDVD_READ_D410:   retval = m_srbReadSony[srb_num].SRB_Status; break;
    case CDVD_READ_D412:   retval = m_srbReadNec [srb_num].SRB_Status; break;
    default: break;
    }

    if(retval == SS_COMP) retval = CDVD_SRB_COMPLETED;
    else if(retval == SS_PENDING) retval = CDVD_SRB_PENDING;
    else //if(retval == SS_ERR) 
        retval = CDVD_SRB_ERROR;

    return retval;
}

// set read mode
int CCdvd::Aspi_SetReadMode(CDVD_READ_MODE read_mode)
{
    int retval = CDVD_ERROR_SUCCESS;

    switch(read_mode)
    {
    case CDVD_READ_MMC:    ReadSector = Aspi_ReadSector_mmc;    break;
    case CDVD_READ_SCSI10: TRACE("cdvdlib: setting scsi10 readmode.\n");
        ReadSector = Aspi_ReadSector_scsi10; 
        if(((this)->*(SetSectorSize))((m_nMmcDataMode == CDVD_MMC_DATAMODE_RAW) ? 
            CDVD_SECTOR_SIZE_CD : CDVD_SECTOR_SIZE_DVD) != CDVD_ERROR_SUCCESS)
        {
            retval = CDVD_ERROR_FAIL;
        }
        break;
    case CDVD_READ_D8:     ReadSector = Aspi_ReadSector_matsu;  break;      
    case CDVD_READ_D410:   ReadSector = Aspi_ReadSector_sony;   break;
    case CDVD_READ_D412:   ReadSector = Aspi_ReadSector_nec;    break;
    default: retval = CDVD_ERROR_FAIL; break;
    }

    UpdateInterfaceObject();

    return retval;
}

// search for scsi adapters with c/dvd drives.
int CCdvd::Aspi_ScsiBusScan()
{
    int retval = CDVD_ERROR_SUCCESS;

    SRB_HAInquiry srbHAInquiry;
    m_nDrives = 0;
    memset(m_drvDetails, 0, CDVD_MAX_SUPPORTED_DRIVES * sizeof(ADAPTERINFO));
    memset(&srbHAInquiry, 0, sizeof (SRB_HAInquiry) );

    srbHAInquiry.SRB_Cmd = SC_HA_INQUIRY;
    srbHAInquiry.SRB_HaId = 0;

    SendASPI32Command ((LPSRB) &srbHAInquiry);
    if (srbHAInquiry.SRB_Status != SS_COMP)
    {
         return CDVD_ERROR_FAIL;
    }

    UI08 HA_Count = srbHAInquiry.HA_Count;
    srbHAInquiry.HA_ManagerId[16] = 0;

    for (UI08 HA_num = 0; HA_num < HA_Count; HA_num++)
    {
        char szHA_num[10];
        itoa((int) HA_num, szHA_num, 10);
        memset(&srbHAInquiry, 0, sizeof(SRB_HAInquiry));
        srbHAInquiry.SRB_Cmd  = SC_HA_INQUIRY;
        srbHAInquiry.SRB_HaId = HA_num;

        SendASPI32Command ((LPSRB) &srbHAInquiry);
        if (srbHAInquiry.SRB_Status != SS_COMP)
        {
            return CDVD_ERROR_FAIL;
        }
        else
        {
            srbHAInquiry.HA_Identifier[16] = 0;
            for (UI08 SCSI_Id = 0; SCSI_Id < 8; SCSI_Id++)
            {
                char szSCSI_Id[10];
                itoa ((int) SCSI_Id, szSCSI_Id, 10);
                for(UI08 SCSI_Lun = 0; SCSI_Lun < 8; SCSI_Lun++)
                {
                    char szSCSI_Lun[10];
                    itoa ((int) SCSI_Lun, szSCSI_Lun, 10);

                    SRB_GDEVBlock srbGDEVBlock;
                    memset(&srbGDEVBlock, 0, sizeof(srbGDEVBlock));
                    srbGDEVBlock.SRB_Cmd    = SC_GET_DEV_TYPE;
                    srbGDEVBlock.SRB_HaId   = HA_num;
                    srbGDEVBlock.SRB_Target = SCSI_Id;
                    srbGDEVBlock.SRB_Lun    = SCSI_Lun;
                    SendASPI32Command ((LPSRB) &srbGDEVBlock);
                    if (srbGDEVBlock.SRB_Status != SS_COMP || 
                        srbGDEVBlock.SRB_DeviceType != DTYPE_CDROM) continue;

                    char temp[40];
                    if(Aspi_ScsiInquiry(HA_num, SCSI_Id, SCSI_Lun, temp) == CDVD_ERROR_SUCCESS)
                    {
                        if(m_nDrives < CDVD_MAX_SUPPORTED_DRIVES)
                        {
                            TRACE("drive %ld: [%ld:%ld:%ld] - %s %s\n", m_nDrives, HA_num, SCSI_Id, SCSI_Lun, 
                                m_drvDetails[m_nDrives].hostname,
                                temp);
                            m_drvDetails[m_nDrives].ha  = HA_num;
                            m_drvDetails[m_nDrives].id  = SCSI_Id;
                            m_drvDetails[m_nDrives].lun = SCSI_Lun;
                            strcpy(m_drvDetails[m_nDrives].hostname, (const char*) srbHAInquiry.HA_Identifier);
                            strcpy(m_drvDetails[m_nDrives].name, temp);
                            ++m_nDrives;
                        }
                        else
                        {
                            break;
                        }
                    }
                    else
                    {
                        retval = CDVD_ERROR_FAIL;
                        break;
                    }
                }
            }
        }
    }

    return retval;
}

// retrieve ha info
int CCdvd::Aspi_ScsiInquiry(UI08 ha, UI08 id, UI08 lun, char *destination)
{
    int retval = CDVD_ERROR_SUCCESS;

	UI08 buffer[36];
  	SRB_ExecSCSICmd srb;

	memset(buffer, 0x20, sizeof(buffer));
	memset(&srb, 0, sizeof(SRB_ExecSCSICmd));

	srb.SRB_Cmd			= SC_EXEC_SCSI_CMD;
	srb.SRB_HaId		= ha;
	srb.SRB_Flags		= SRB_DIR_IN;
	srb.SRB_Target		= id;
	srb.SRB_Lun			= lun;
	srb.SRB_BufLen		= 36;
	srb.SRB_BufPointer	= buffer;
	srb.SRB_SenseLen	= SENSE_LEN;
	srb.SRB_CDBLen		= 6;
	srb.CDBByte[0]	    = SCSI_INQUIRY;
	srb.CDBByte[4]	    = 36;  // allocation length per buffer

	if (Aspi_ExecuteSrb(srb, NULL, 0) == CDVD_ERROR_SUCCESS)
	{
        memcpy(destination, buffer + 8, 27);
        destination[27] = '\0';
	}
	else
	{
		destination[0] = '\0';
	}

    return retval;
}

/***********************************************************/
/*              ASPI Execute SRB                           */
/***********************************************************/
// if HANDLE = NULL, SRB is synchronous, else its asynchronous
int CCdvd::Aspi_ExecuteSrb(SRB_ExecSCSICmd &srbExec, HANDLE *handle, int nretry)
{
    int retval = CDVD_ERROR_SUCCESS;

    ++nretry;    
    // lets do sync with handle == NULL
    if(handle == NULL)
    {
        TRACE("executing synchronous SRB.\n");
        while(nretry)
	    {
	        HANDLE heventExec = CreateEvent( NULL, TRUE, FALSE, NULL );
            if(heventExec) // do event based notification
            {
                srbExec.SRB_Flags |= SRB_EVENT_NOTIFY;
                srbExec.SRB_PostProc = (LPVOID)heventExec;
        
                DWORD dwASPIStatus = SendASPI32Command((LPSRB) &srbExec);
			    if(dwASPIStatus == SS_ERR) 
			    {
				    CloseHandle(heventExec);
				    return CDVD_ERROR_FAIL;
			    }

                if(dwASPIStatus == SS_PENDING)
                {
				    WaitForSingleObject(heventExec, INFINITE);
                }
                CloseHandle(heventExec);
            }
            else // do shitty polling
            {
                SendASPI32Command((LPSRB)&srbExec);
                while(srbExec.SRB_Status == SS_PENDING);
            }
            
            TRACE("SRB_Status: %ld\n", srbExec.SRB_Status);
	        if( srbExec.SRB_Status != SS_COMP )
            {
                if(nretry-- && 
                    (srbExec.SRB_TargStat != STATUS_CHKCOND ||
                    (srbExec.SenseArea[2] & 0x0F) != KEY_UNITATT) )
                {
			        UI08 sense = srbExec.SenseArea[2] & 0x0F;
			        UI08 asc   = srbExec.SenseArea[12];
			        UI08 ascq  = srbExec.SenseArea[13];
                    TRACE("sensekey: %02Xh, %02Xh, %02Xh\n", sense, asc, ascq); 
                    continue;
                }
            }
            else
            {
                // we're done.
                break;
            }
        }
    }
    else
    {
        TRACE("executing asynchronous SRB.\n");

        ResetEvent(*handle);
        srbExec.SRB_Flags   |= SRB_EVENT_NOTIFY;
        srbExec.SRB_PostProc = *handle;
    
        DWORD dwASPIStatus = SendASPI32Command((LPSRB) &srbExec);

	    if(dwASPIStatus == SS_ERR) 
	    {
		    SetEvent(*handle);
		    TRACE("SRB unrecoverable error.\n");
            return CDVD_ERROR_FAIL;
	    }
    
	     if(dwASPIStatus == SS_PENDING) 
         {
             TRACE("SRB Pending completion.\n");
		     return CDVD_ERROR_PENDING;
         }
        
         TRACE("SRB completed immediately.\n");       
    }

    if(nretry == 0) retval = CDVD_ERROR_FAIL;
	return retval;
}

/***********************************************************/
/*              ASPI C/DVD Methods                         */
/***********************************************************/

// read using mmc 0xbe command
int CCdvd::Aspi_ReadSector_mmc(int sect, HANDLE *handle)
{
	_ASSERT(m_bAspiInitialized == TRUE);

	m_srbReadCd[m_nCurrentBuffer].SRB_Cmd		 = SC_EXEC_SCSI_CMD;
	m_srbReadCd[m_nCurrentBuffer].SRB_HaId	     = m_drvDetails[m_nCurrentDrive].ha;
	m_srbReadCd[m_nCurrentBuffer].SRB_Flags	     = SRB_DIR_IN | SRB_ENABLE_RESIDUAL_COUNT;
	m_srbReadCd[m_nCurrentBuffer].SRB_Target	 = m_drvDetails[m_nCurrentDrive].id;
	m_srbReadCd[m_nCurrentBuffer].SRB_BufPointer = m_ReadBuffer[m_nCurrentBuffer].AB_BufPointer;
	m_srbReadCd[m_nCurrentBuffer].SRB_BufLen	 = m_ReadBuffer[m_nCurrentBuffer].AB_BufLen;
	m_srbReadCd[m_nCurrentBuffer].SRB_Lun		 = m_drvDetails[m_nCurrentDrive].lun;	
	m_srbReadCd[m_nCurrentBuffer].SRB_SenseLen   = SENSE_LEN;

	m_srbReadCd[m_nCurrentBuffer].SRB_CDBLen  = 12;
	m_srbReadCd[m_nCurrentBuffer].CDBByte[0]  = 0xBE;
	m_srbReadCd[m_nCurrentBuffer].CDBByte[1]  = 0x00;
	m_srbReadCd[m_nCurrentBuffer].CDBByte[2]  = HIBYTE(HIWORD(sect));
	m_srbReadCd[m_nCurrentBuffer].CDBByte[3]  = LOBYTE(HIWORD(sect));
	m_srbReadCd[m_nCurrentBuffer].CDBByte[4]  = HIBYTE(LOWORD(sect));
	m_srbReadCd[m_nCurrentBuffer].CDBByte[5]  = LOBYTE(LOWORD(sect));
	m_srbReadCd[m_nCurrentBuffer].CDBByte[6]  = 0x00;
	m_srbReadCd[m_nCurrentBuffer].CDBByte[7]  = 0x00;
	m_srbReadCd[m_nCurrentBuffer].CDBByte[8]  = CDVD_NUM_SECTORS_PER_BUFF;
	m_srbReadCd[m_nCurrentBuffer].CDBByte[9]  = (UI08) m_nMmcDataMode; 
	m_srbReadCd[m_nCurrentBuffer].CDBByte[10] = 0x00;
	m_srbReadCd[m_nCurrentBuffer].CDBByte[11] = 0x00;

    return Aspi_ExecuteSrb(m_srbReadCd[m_nCurrentBuffer], handle, CDVD_MAX_RETRY);
}

// read using scsi-10 (0x28) command, size is dependent on SetSectorSize
int CCdvd::Aspi_ReadSector_scsi10(int sect, HANDLE *handle)
{
	_ASSERT(m_bAspiInitialized == TRUE);

	m_srbRead10[m_nCurrentBuffer].SRB_Cmd		 = SC_EXEC_SCSI_CMD;
	m_srbRead10[m_nCurrentBuffer].SRB_HaId	     = m_drvDetails[m_nCurrentDrive].ha;
	m_srbRead10[m_nCurrentBuffer].SRB_Flags	     = SRB_DIR_IN;// | SRB_ENABLE_RESIDUAL_COUNT;
	m_srbRead10[m_nCurrentBuffer].SRB_Target	 = m_drvDetails[m_nCurrentDrive].id;
	m_srbRead10[m_nCurrentBuffer].SRB_BufPointer = m_ReadBuffer[m_nCurrentBuffer].AB_BufPointer;
	m_srbRead10[m_nCurrentBuffer].SRB_BufLen	 = m_ReadBuffer[m_nCurrentBuffer].AB_BufLen;
	m_srbRead10[m_nCurrentBuffer].SRB_Lun		 = m_drvDetails[m_nCurrentDrive].lun;	
	m_srbRead10[m_nCurrentBuffer].SRB_SenseLen   = SENSE_LEN;

	m_srbRead10[m_nCurrentBuffer].SRB_CDBLen  = 10;
	m_srbRead10[m_nCurrentBuffer].CDBByte[0]  = 0x28;
	m_srbRead10[m_nCurrentBuffer].CDBByte[1]  = 0x00;
	m_srbRead10[m_nCurrentBuffer].CDBByte[2]  = HIBYTE(HIWORD(sect));
	m_srbRead10[m_nCurrentBuffer].CDBByte[3]  = LOBYTE(HIWORD(sect));
	m_srbRead10[m_nCurrentBuffer].CDBByte[4]  = HIBYTE(LOWORD(sect));
	m_srbRead10[m_nCurrentBuffer].CDBByte[5]  = LOBYTE(LOWORD(sect));
	m_srbRead10[m_nCurrentBuffer].CDBByte[6]  = 0x00;
	m_srbRead10[m_nCurrentBuffer].CDBByte[7]  = 0x00;
	m_srbRead10[m_nCurrentBuffer].CDBByte[8]  = CDVD_NUM_SECTORS_PER_BUFF;
	m_srbRead10[m_nCurrentBuffer].CDBByte[9]  = 0x00; 

    return Aspi_ExecuteSrb(m_srbRead10[m_nCurrentBuffer], handle, CDVD_MAX_RETRY);
}

// read using proprietary matsushita command, heck if it works
int CCdvd::Aspi_ReadSector_matsu(int sect, HANDLE *handle)
{
	_ASSERT(m_bAspiInitialized == TRUE);

	m_srbReadMat[m_nCurrentBuffer].SRB_Cmd	      = SC_EXEC_SCSI_CMD;
	m_srbReadMat[m_nCurrentBuffer].SRB_HaId	      = m_drvDetails[m_nCurrentDrive].ha;
	m_srbReadMat[m_nCurrentBuffer].SRB_Flags	  = SRB_DIR_IN | SRB_ENABLE_RESIDUAL_COUNT;
	m_srbReadMat[m_nCurrentBuffer].SRB_Target	  = m_drvDetails[m_nCurrentDrive].id;
	m_srbReadMat[m_nCurrentBuffer].SRB_BufPointer = m_ReadBuffer[m_nCurrentBuffer].AB_BufPointer;
	m_srbReadMat[m_nCurrentBuffer].SRB_BufLen	  = m_ReadBuffer[m_nCurrentBuffer].AB_BufLen;
	m_srbReadMat[m_nCurrentBuffer].SRB_Lun		  = m_drvDetails[m_nCurrentDrive].lun;	
	m_srbReadMat[m_nCurrentBuffer].SRB_SenseLen   = SENSE_LEN;

	m_srbReadMat[m_nCurrentBuffer].SRB_CDBLen  = 12;
	m_srbReadMat[m_nCurrentBuffer].CDBByte[0]  = 0xd4;
	m_srbReadMat[m_nCurrentBuffer].CDBByte[1]  = 0x00;
	m_srbReadMat[m_nCurrentBuffer].CDBByte[2]  = HIBYTE(HIWORD(sect));
	m_srbReadMat[m_nCurrentBuffer].CDBByte[3]  = LOBYTE(HIWORD(sect));
	m_srbReadMat[m_nCurrentBuffer].CDBByte[4]  = HIBYTE(LOWORD(sect));
	m_srbReadMat[m_nCurrentBuffer].CDBByte[5]  = LOBYTE(LOWORD(sect));
	m_srbReadMat[m_nCurrentBuffer].CDBByte[6]  = 0x00;
	m_srbReadMat[m_nCurrentBuffer].CDBByte[7]  = 0x00;
	m_srbReadMat[m_nCurrentBuffer].CDBByte[8]  = CDVD_NUM_SECTORS_PER_BUFF;
	m_srbReadMat[m_nCurrentBuffer].CDBByte[9]  = 0x00; 
	m_srbReadMat[m_nCurrentBuffer].CDBByte[10] = 0x00; 
	m_srbReadMat[m_nCurrentBuffer].CDBByte[11] = 0x00; 

    return Aspi_ExecuteSrb(m_srbReadMat[m_nCurrentBuffer], handle, CDVD_MAX_RETRY);
}

// read using proprietary sony command, heck if it works
int CCdvd::Aspi_ReadSector_sony(int sect, HANDLE *handle)
{
	_ASSERT(m_bAspiInitialized == TRUE);

	m_srbReadSony[m_nCurrentBuffer].SRB_Cmd	       = SC_EXEC_SCSI_CMD;
	m_srbReadSony[m_nCurrentBuffer].SRB_HaId	   = m_drvDetails[m_nCurrentDrive].ha;
	m_srbReadSony[m_nCurrentBuffer].SRB_Flags	   = SRB_DIR_IN | SRB_ENABLE_RESIDUAL_COUNT;
	m_srbReadSony[m_nCurrentBuffer].SRB_Target	   = m_drvDetails[m_nCurrentDrive].id;
	m_srbReadSony[m_nCurrentBuffer].SRB_BufPointer = m_ReadBuffer[m_nCurrentBuffer].AB_BufPointer;
	m_srbReadSony[m_nCurrentBuffer].SRB_BufLen	   = m_ReadBuffer[m_nCurrentBuffer].AB_BufLen;
	m_srbReadSony[m_nCurrentBuffer].SRB_Lun		   = m_drvDetails[m_nCurrentDrive].lun;	
	m_srbReadSony[m_nCurrentBuffer].SRB_SenseLen   = SENSE_LEN;

	m_srbReadSony[m_nCurrentBuffer].SRB_CDBLen  = 12;
	m_srbReadSony[m_nCurrentBuffer].CDBByte[0]  = 0xd8;
	m_srbReadSony[m_nCurrentBuffer].CDBByte[1]  = 0x00;
	m_srbReadSony[m_nCurrentBuffer].CDBByte[2]  = HIBYTE(HIWORD(sect));
	m_srbReadSony[m_nCurrentBuffer].CDBByte[3]  = LOBYTE(HIWORD(sect));
	m_srbReadSony[m_nCurrentBuffer].CDBByte[4]  = HIBYTE(LOWORD(sect));
	m_srbReadSony[m_nCurrentBuffer].CDBByte[5]  = LOBYTE(LOWORD(sect));
	m_srbReadSony[m_nCurrentBuffer].CDBByte[6]  = 0x00;
	m_srbReadSony[m_nCurrentBuffer].CDBByte[7]  = 0x00;
	m_srbReadSony[m_nCurrentBuffer].CDBByte[8]  = CDVD_NUM_SECTORS_PER_BUFF;
	m_srbReadSony[m_nCurrentBuffer].CDBByte[9]  = 0x00; 
	m_srbReadSony[m_nCurrentBuffer].CDBByte[10] = 0x00; 
	m_srbReadSony[m_nCurrentBuffer].CDBByte[11] = 0x00; 

    return Aspi_ExecuteSrb(m_srbReadSony[m_nCurrentBuffer], handle, CDVD_MAX_RETRY);
}

// read using proprietary nec command, heck if it works
int CCdvd::Aspi_ReadSector_nec(int sect, HANDLE *handle)
{
	_ASSERT(m_bAspiInitialized == TRUE);

	m_srbReadNec[m_nCurrentBuffer].SRB_Cmd	      = SC_EXEC_SCSI_CMD;
	m_srbReadNec[m_nCurrentBuffer].SRB_HaId	      = m_drvDetails[m_nCurrentDrive].ha;
	m_srbReadNec[m_nCurrentBuffer].SRB_Flags	  = SRB_DIR_IN | SRB_ENABLE_RESIDUAL_COUNT;
	m_srbReadNec[m_nCurrentBuffer].SRB_Target	  = m_drvDetails[m_nCurrentDrive].id;
	m_srbReadNec[m_nCurrentBuffer].SRB_BufPointer = m_ReadBuffer[m_nCurrentBuffer].AB_BufPointer;
	m_srbReadNec[m_nCurrentBuffer].SRB_BufLen	  = m_ReadBuffer[m_nCurrentBuffer].AB_BufLen;
	m_srbReadNec[m_nCurrentBuffer].SRB_Lun		  = m_drvDetails[m_nCurrentDrive].lun;	
	m_srbReadNec[m_nCurrentBuffer].SRB_SenseLen   = SENSE_LEN;

	m_srbReadNec[m_nCurrentBuffer].SRB_CDBLen  = 10;
	m_srbReadNec[m_nCurrentBuffer].CDBByte[0]  = 0xd4;
	m_srbReadNec[m_nCurrentBuffer].CDBByte[1]  = 0x00;
	m_srbReadNec[m_nCurrentBuffer].CDBByte[2]  = HIBYTE(HIWORD(sect));
	m_srbReadNec[m_nCurrentBuffer].CDBByte[3]  = LOBYTE(HIWORD(sect));
	m_srbReadNec[m_nCurrentBuffer].CDBByte[4]  = HIBYTE(LOWORD(sect));
	m_srbReadNec[m_nCurrentBuffer].CDBByte[5]  = LOBYTE(LOWORD(sect));
	m_srbReadNec[m_nCurrentBuffer].CDBByte[6]  = 0x00;
	m_srbReadNec[m_nCurrentBuffer].CDBByte[7]  = 0x00;
	m_srbReadNec[m_nCurrentBuffer].CDBByte[8]  = CDVD_NUM_SECTORS_PER_BUFF;
	m_srbReadNec[m_nCurrentBuffer].CDBByte[9]  = 0x00; 

    return Aspi_ExecuteSrb(m_srbReadNec[m_nCurrentBuffer], handle, CDVD_MAX_RETRY);
}

// retrieve number of sectors present in c/dvd media 
int CCdvd::Aspi_GetNumSectors()
{
	_ASSERT(m_bAspiInitialized == TRUE);
    
    SRB_ExecSCSICmd srb;
    UI08 capacity[8];
    UI32 max_sector;

    memset(&srb, 0, sizeof(SRB_ExecSCSICmd));

    srb.SRB_Cmd			= SC_EXEC_SCSI_CMD;
    srb.SRB_HaId		= m_drvDetails[m_nCurrentDrive].ha;
    srb.SRB_Flags		= SRB_DIR_IN;
    srb.SRB_Target		= m_drvDetails[m_nCurrentDrive].id;
	srb.SRB_Lun			= m_drvDetails[m_nCurrentDrive].lun;
    srb.SRB_BufLen		= 8;
    srb.SRB_BufPointer	= capacity;
    srb.SRB_SenseLen	= SENSE_LEN;
    srb.SRB_CDBLen		= 10;

    srb.CDBByte[0]      = SCSI_RD_CAPAC;     


    if(Aspi_ExecuteSrb(srb, NULL, CDVD_MAX_RETRY) 
        != CDVD_ERROR_SUCCESS)
    	return 0; // no sectors found
 
    MOVESCSIDWORD(&capacity[0], &max_sector);

	return max_sector+1;
}

// retrieve TOC data
int CCdvd::Aspi_GetToc()
{   
	_ASSERT(m_bAspiInitialized == TRUE);

    SRB_ExecSCSICmd srb;
    UI08 buff[804];
    memset(buff, 0, sizeof(buff));
    memset (&srb, 0, sizeof(SRB_ExecSCSICmd));

    srb.SRB_Cmd		   = SC_EXEC_SCSI_CMD;
    srb.SRB_Flags      = SRB_DIR_IN;
    srb.SRB_HaId	   = m_drvDetails[m_nCurrentDrive].ha;
    srb.SRB_Target	   = m_drvDetails[m_nCurrentDrive].id;
    srb.SRB_Lun		   = m_drvDetails[m_nCurrentDrive].lun;
    srb.SRB_SenseLen   = SENSE_LEN;
    srb.SRB_BufPointer = buff;
    srb.SRB_BufLen     = sizeof(buff);
    srb.SRB_CDBLen	   = 10;
    srb.CDBByte[0]	   = SCSI_READ_TOC;
    srb.CDBByte[6]     = 0;
    srb.CDBByte[7]     = 0x03;
    srb.CDBByte[8]     = 0x24;

    if(Aspi_ExecuteSrb(srb, NULL, CDVD_MAX_RETRY) 
        != CDVD_ERROR_SUCCESS)
        return CDVD_ERROR_FAIL;
    
    if(ExtractTocData(buff) != CDVD_ERROR_SUCCESS)
        return CDVD_ERROR_FAIL;
   
    return CDVD_ERROR_SUCCESS;
}

// set cdrom speed
int CCdvd::Aspi_SetSpeed(int speed)
{
	_ASSERT(m_bAspiInitialized == TRUE);
	
    SRB_ExecSCSICmd srb;     
	UI16 kbSpeed = 0xFFFF;
	switch(speed)
	{
	case 0: kbSpeed = 0xFFFF; break;
	case 1: kbSpeed = 176; break;
	case 2: kbSpeed = 353; break;  
	case 3: kbSpeed = 528; break;
	case 4: kbSpeed = 706; break;
	case 5: kbSpeed = 1400; break;
	case 6: kbSpeed = 2800; break;
	default: kbSpeed = 0xFFFF; break; // max
	}

    memset(&srb,0,sizeof(srb));

	srb.SRB_Cmd		 = SC_EXEC_SCSI_CMD;
    srb.SRB_HaId	 = m_drvDetails[m_nCurrentDrive].ha;
    srb.SRB_Target	 = m_drvDetails[m_nCurrentDrive].id;
    srb.SRB_Lun		 = m_drvDetails[m_nCurrentDrive].lun;
	srb.SRB_Flags	 = SRB_DIR_OUT; //|SRB_ENABLE_RESIDUAL_COUNT;
	srb.SRB_SenseLen = SENSE_LEN;

	srb.SRB_CDBLen   = 12;
	srb.CDBByte[0]   = 0xBB;
	srb.CDBByte[2]   = HIBYTE(kbSpeed);
	srb.CDBByte[3]   = LOBYTE(kbSpeed);

	return Aspi_ExecuteSrb(srb, NULL, CDVD_MAX_RETRY);    
}

// used for scsi only, sets sector size (i.e, 2352, 2048)
int CCdvd::Aspi_SetSectorSize(int sect_size)
{
	_ASSERT(m_bAspiInitialized == TRUE);

    TRACE("sectorsize: %ld\n", sect_size);
    MODESELHEADER mh;
    SRB_ExecSCSICmd srb;
 
    memset (&srb, 0, sizeof(SRB_ExecSCSICmd));
    memset (&mh,  0, sizeof(MODESELHEADER));

    mh.block_desc_length	= 0x08;
    mh.block_length_med		= HIBYTE(LOWORD(sect_size));
    mh.block_length_lo		= LOBYTE(LOWORD(sect_size));

    srb.SRB_Cmd		   = SC_EXEC_SCSI_CMD;
    srb.SRB_HaId	   = m_drvDetails[m_nCurrentDrive].ha;
    srb.SRB_Target	   = m_drvDetails[m_nCurrentDrive].id;
    srb.SRB_Lun		   = m_drvDetails[m_nCurrentDrive].lun;
    srb.SRB_SenseLen   = SENSE_LEN;
    srb.SRB_Flags	   = SRB_DIR_OUT;
    srb.SRB_BufLen	   = sizeof(mh);
    srb.SRB_BufPointer = (UI08 *) &mh;
    srb.SRB_CDBLen	   = 6;
    srb.CDBByte[0]	   = 0x15;
    srb.CDBByte[1]     = 0x10;
    srb.CDBByte[4]     = 0x0C;

    return Aspi_ExecuteSrb(srb, NULL, CDVD_MAX_RETRY);
}

// start cddda playback, dunno if it works right really.. :p
int CCdvd::Aspi_Play(int sect_start, int sect_stop)
{
	_ASSERT(m_bAspiInitialized == TRUE);

    SRB_ExecSCSICmd srb;
	
    memset( &srb, 0, sizeof(SRB_ExecSCSICmd) );

    srb.SRB_Cmd		   = SC_EXEC_SCSI_CMD;
    srb.SRB_HaId	   = m_drvDetails[m_nCurrentDrive].ha;
    srb.SRB_Target	   = m_drvDetails[m_nCurrentDrive].id;
    srb.SRB_Lun		   = m_drvDetails[m_nCurrentDrive].lun;
    srb.SRB_Flags	   = SRB_DIR_IN;
    srb.SRB_SenseLen   = SENSE_LEN;
    srb.SRB_CDBLen	   = 12;

	srb.CDBByte[0]  = 0xa5;
	srb.CDBByte[2]  = HIBYTE(HIWORD(sect_start));
	srb.CDBByte[3]  = LOBYTE(HIWORD(sect_start));
	srb.CDBByte[4]  = HIBYTE(LOWORD(sect_start));
	srb.CDBByte[5]  = LOBYTE(LOWORD(sect_start));
	srb.CDBByte[6]  = HIBYTE(HIWORD(sect_stop));
	srb.CDBByte[7]  = LOBYTE(HIWORD(sect_stop));
	srb.CDBByte[8]  = HIBYTE(LOWORD(sect_stop));
	srb.CDBByte[9]  = LOBYTE(LOWORD(sect_stop));
//	srb.CDBByte[10] = 0x8F;

    return Aspi_ExecuteSrb(srb, NULL, CDVD_MAX_RETRY);
}

// stop cdda playback, dunno if it works right really.. :p
int CCdvd::Aspi_Stop()
{
	_ASSERT(m_bAspiInitialized == TRUE);

    SRB_ExecSCSICmd srb;
	
    memset(&srb, 0, sizeof(SRB_ExecSCSICmd));

    srb.SRB_Cmd		   = SC_EXEC_SCSI_CMD;
    srb.SRB_HaId	   = m_drvDetails[m_nCurrentDrive].ha;
    srb.SRB_Target	   = m_drvDetails[m_nCurrentDrive].id;
    srb.SRB_Lun		   = m_drvDetails[m_nCurrentDrive].lun;
    srb.SRB_Flags	   = SRB_DIR_IN;
    srb.SRB_SenseLen   = SENSE_LEN;
    srb.SRB_CDBLen	   = 10;
	srb.CDBByte[0]	   = 0x4E;

    return Aspi_ExecuteSrb(srb, NULL, CDVD_MAX_RETRY);
}

// detect if media/LUN is ready, legacy command, 
// can prolly be substituted with GetMediaType(), with some modifs
int CCdvd::Aspi_TestReady()
{
    SRB_ExecSCSICmd srb;

    memset (&srb, 0, sizeof(SRB_ExecSCSICmd));
    srb.SRB_Cmd		 = SC_EXEC_SCSI_CMD;
    srb.SRB_HaId	 = m_drvDetails[m_nCurrentDrive].ha;
    srb.SRB_Target	 = m_drvDetails[m_nCurrentDrive].id;
    srb.SRB_Lun		 = m_drvDetails[m_nCurrentDrive].lun;
    srb.SRB_SenseLen = SENSE_LEN;
    srb.SRB_CDBLen	 = 6;
    srb.CDBByte[0]	 = SCSI_TST_U_RDY;
    
   return Aspi_ExecuteSrb(srb, NULL, CDVD_MAX_RETRY);
}

// retrieve feature details
int CCdvd::Aspi_GetConfiguration(UI16 feature, UI08 *dest, UI32 dest_size)
{
    _ASSERT(m_bAspiInitialized == TRUE);

    int retval = CDVD_ERROR_SUCCESS;

    SRB_ExecSCSICmd srb;
    memset(&srb,   0, sizeof(SRB_ExecSCSICmd));
    memset(dest,   0, dest_size);

    srb.SRB_Cmd		   = SC_EXEC_SCSI_CMD;
    srb.SRB_HaId	   = m_drvDetails[m_nCurrentDrive].ha;
    srb.SRB_Target	   = m_drvDetails[m_nCurrentDrive].id;
    srb.SRB_Lun		   = m_drvDetails[m_nCurrentDrive].lun;
    srb.SRB_Flags	   = SRB_DIR_IN;
    srb.SRB_SenseLen   = SENSE_LEN;
    srb.SRB_BufPointer = dest;
    srb.SRB_BufLen     = dest_size;

	srb.SRB_CDBLen  = 12;
	srb.CDBByte[0]  = 0x46;
	srb.CDBByte[1]  = 0x02; // return a single feature set ONLY.
	srb.CDBByte[2]  = HIBYTE(feature);
	srb.CDBByte[3]  = LOBYTE(feature);
	srb.CDBByte[7]  = HIBYTE(dest_size);
	srb.CDBByte[8]  = LOBYTE(dest_size);

    return Aspi_ExecuteSrb(srb, NULL, CDVD_MAX_RETRY);
}
