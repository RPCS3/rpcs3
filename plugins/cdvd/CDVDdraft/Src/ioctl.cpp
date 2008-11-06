// Ioctl.cpp: implementation of the ioctl methods
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cdvd.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

/***********************************************************/
/*              IOCTL Init/Shutdown related methods        */
/***********************************************************/
int CCdvd::Ioctl_Init()
{
    int retval = CDVD_ERROR_FAIL;

    if(m_bIoctlInitialized)
        return CDVD_ERROR_SUCCESS;


	
    return retval;
}

// shuts down ioctl
int CCdvd::Ioctl_Shutdown()
{
    int retval = CDVD_ERROR_SUCCESS;


    GetNumSectors = Dummy_T1;
    GetToc        = Dummy_T1;
    Stop          = Dummy_T1;
    Play          = Dummy_T3;
    TestReady     = Dummy_T1;
    SetSpeed      = Dummy_T2;
    SetSectorSize = Dummy_T2;
    ReadSector    = Dummy_T4;
    UpdateInterfaceObject();

    ShutdownBuffers();
    return retval;
}

// open a drive for reading
int CCdvd::Ioctl_OpenDrive(int drv_num)
{
    int retval = CDVD_ERROR_SUCCESS;
    CloseDrive();
    m_nCurrentDrive = drv_num;
    
    return retval;
}

// dummy for now
int CCdvd::Ioctl_CloseDrive()
{
    int retval = CDVD_ERROR_SUCCESS;
    m_nCurrentDrive = -1;

    return retval;
}

// get srb status
int CCdvd::Ioctl_GetSrbStatus(int srb_num)
{
    int retval = CDVD_SRB_ERROR;
    switch(m_nCurrentReadMode)
    {
    case CDVD_READ_MMC:    
    case CDVD_READ_SCSI10: 
    case CDVD_READ_D8:           
    case CDVD_READ_D410:   
    case CDVD_READ_D412:   
    default: break;
    }

    if(retval == SS_COMP) retval = CDVD_SRB_COMPLETED;
    else if(retval == SS_PENDING) retval = CDVD_SRB_PENDING;
    else retval = CDVD_SRB_ERROR;

    return retval;
}

// set read mode
int CCdvd::Ioctl_SetReadMode(CDVD_READ_MODE read_mode)
{
    int retval = CDVD_ERROR_SUCCESS;

    switch(read_mode)
    {
    case CDVD_READ_MMC:    
    case CDVD_READ_SCSI10: 
    case CDVD_READ_D8:          
    case CDVD_READ_D410:   
    case CDVD_READ_D412:   
    default: retval = CDVD_ERROR_FAIL; break;
    }

    UpdateInterfaceObject();

    return retval;
}

// search for scsi adapters with c/dvd drives.
int CCdvd::Ioctl_ScsiBusScan()
{
    int retval = CDVD_ERROR_SUCCESS;

    HANDLE handle;
    char adapter_name[16];
	UI08 buffer[2048];
    int adapter_num = 0;

    memset(m_drvDetails, 0, CDVD_MAX_SUPPORTED_DRIVES * sizeof(ADAPTERINFO));
    
    m_nDrives = 0;
    for(;;)
    {
        wsprintf(adapter_name,"\\\\.\\SCSI%d:", adapter_num);

        ++adapter_num;

        if((handle = CreateFile(adapter_name, 
            GENERIC_READ, 
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            0,
            NULL)) == INVALID_HANDLE_VALUE)
        {
            break; // no more scsi adapters
        }

        UI32 nreturned;
        if(DeviceIoControl(handle, 
            IOCTL_SCSI_GET_INQUIRY_DATA, 
            NULL,
            0,
            buffer,
            sizeof(buffer),
            &nreturned,
            FALSE) == 0) 
        {
            continue; // failed
        }

        Ioctl_AddAdapter(adapter_num, buffer);
    }

    return retval;
}

// add an adapter to the adapter list
int CCdvd::Ioctl_AddAdapter(int adptr_num, UI08 *buffer)
{
    int retval = CDVD_ERROR_SUCCESS;

    SCSI_ADAPTER_BUS_INFO *adapter_info = (SCSI_ADAPTER_BUS_INFO *) buffer;
	char string[40];

    memset(string, 0, sizeof(string));
   
    for (int i = 0; i < adapter_info->NumberOfBuses; i++) 
	{
       SCSI_INQUIRY_DATA *inquiry_data = 
           (PSCSI_INQUIRY_DATA) (buffer + adapter_info->BusData[i].InquiryDataOffset);

        //while 
        if(adapter_info->BusData[i].InquiryDataOffset) 
        {
            if(inquiry_data->InquiryData[0] == 0x05) // c/dvd drive
            {
                m_drvDetails[m_nDrives].ha = adptr_num;
                m_drvDetails[m_nDrives].id = inquiry_data->TargetId;
                m_drvDetails[m_nDrives].lun = inquiry_data->Lun;
                m_drvDetails[m_nDrives].hostname[0] = '\0';
                
                memcpy(m_drvDetails[m_nDrives].name, 
                      &inquiry_data->InquiryData[8] + 9, 27);

                m_drvDetails[m_nDrives].name[27] = '\0';
                ++m_nDrives;
            }

            if(inquiry_data->NextInquiryDataOffset == 0) 
            {
               break;
            }

            inquiry_data = (SCSI_INQUIRY_DATA *) (buffer + inquiry_data->NextInquiryDataOffset);
        }
    }

    return retval;
}

/***********************************************************/
/*              IOCTL Execute SRB                          */
/***********************************************************/

/***********************************************************/
/*              ASPI C/DVD Methods                         */
/***********************************************************/
