/* SPU2-X, A plugin for Emulating the Sound Processing Unit of the Playstation 2
 * Developed and maintained by the Pcsx2 Development Team.
 *
 * Original portions from SPU2ghz are (c) 2008 by David Quintana [gigaherz]
 *
 * SPU2-X is free software: you can redistribute it and/or modify it under the terms
 * of the GNU Lesser General Public License as published by the Free Software Found-
 * ation, either version 3 of the License, or (at your option) any later version.
 *
 * SPU2-X is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with SPU2-X.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Global.h"
#include "Dialogs.h"
#include "Config.h"
#include "Utilities/Path.h"

bool DebugEnabled=false;
bool _MsgToConsole=false;
bool _MsgKeyOnOff=false;
bool _MsgVoiceOff=false;
bool _MsgDMA=false;
bool _MsgAutoDMA=false;
bool _MsgOverruns=false;
bool _MsgCache=false;

bool _AccessLog=false;
bool _DMALog=false;
bool _WaveLog=false;

bool _CoresDump=false;
bool _MemDump=false;
bool _RegDump=false;

// this is set true if PCSX2 invokes the SetLogDir callback, which tells SPU2-X to use that over
// the configured crap in the ini file.
static bool LogLocationSetByPcsx2 = false;

static wxDirName LogsFolder;
static wxDirName DumpsFolder;

wxString AccessLogFileName;
wxString WaveLogFileName;
wxString DMA4LogFileName;
wxString DMA7LogFileName;

wxString CoresDumpFileName;
wxString MemDumpFileName;
wxString RegDumpFileName;

void CfgSetLogDir( const char* dir )
{
	LogsFolder	= (dir==NULL) ? wxString(L"logs") : fromUTF8(dir);
	DumpsFolder	= (dir==NULL) ? wxString(L"logs") : fromUTF8(dir);
	LogLocationSetByPcsx2 = (dir!=NULL);
}

FILE* OpenBinaryLog( const wxString& logfile )
{
	return wxFopen( Path::Combine(LogsFolder, logfile), L"wb" );
}

FILE* OpenLog( const wxString& logfile )
{
	return wxFopen( Path::Combine(LogsFolder, logfile), L"w" );
}

FILE* OpenDump( const wxString& logfile )
{
	return wxFopen( Path::Combine(DumpsFolder, logfile), L"w" );
}

namespace DebugConfig {
static const wchar_t* Section = L"DEBUG";

static void set_default_filenames()
{
	AccessLogFileName = L"SPU2Log.txt";
	WaveLogFileName = L"SPU2log.wav";
	DMA4LogFileName = L"SPU2dma4.dat";
	DMA7LogFileName = L"SPU2dma7.dat";

	CoresDumpFileName = L"SPU2Cores.txt";
	MemDumpFileName = L"SPU2mem.dat";
	RegDumpFileName = L"SPU2regs.dat";
}

void ReadSettings()
{
	DebugEnabled = CfgReadBool(Section, L"Global_Enable",0);
	_MsgToConsole= CfgReadBool(Section, L"Show_Messages",0);
	_MsgKeyOnOff = CfgReadBool(Section, L"Show_Messages_Key_On_Off",0);
	_MsgVoiceOff = CfgReadBool(Section, L"Show_Messages_Voice_Off",0);
	_MsgDMA      = CfgReadBool(Section, L"Show_Messages_DMA_Transfer",0);
	_MsgAutoDMA  = CfgReadBool(Section, L"Show_Messages_AutoDMA",0);
	_MsgOverruns = CfgReadBool(Section, L"Show_Messages_Overruns",0);
	_MsgCache    = CfgReadBool(Section, L"Show_Messages_CacheStats",0);

	_AccessLog   = CfgReadBool(Section, L"Log_Register_Access",0);
	_DMALog      = CfgReadBool(Section, L"Log_DMA_Transfers",0);
	_WaveLog     = CfgReadBool(Section, L"Log_WAVE_Output",0);

	_CoresDump   = CfgReadBool(Section, L"Dump_Info",0);
	_MemDump     = CfgReadBool(Section, L"Dump_Memory",0);
	_RegDump     = CfgReadBool(Section, L"Dump_Regs",0);

	set_default_filenames();

	CfgReadStr(Section,L"Access_Log_Filename",AccessLogFileName,	L"logs/SPU2Log.txt");
	CfgReadStr(Section,L"WaveLog_Filename",   WaveLogFileName,		L"logs/SPU2log.wav");
	CfgReadStr(Section,L"DMA4Log_Filename",   DMA4LogFileName,		L"logs/SPU2dma4.dat");
	CfgReadStr(Section,L"DMA7Log_Filename",   DMA7LogFileName,		L"logs/SPU2dma7.dat");

	CfgReadStr(Section,L"Info_Dump_Filename",CoresDumpFileName,		L"logs/SPU2Cores.txt");
	CfgReadStr(Section,L"Mem_Dump_Filename", MemDumpFileName,		L"logs/SPU2mem.dat");
	CfgReadStr(Section,L"Reg_Dump_Filename", RegDumpFileName,		L"logs/SPU2regs.dat");
}


void WriteSettings()
{
	CfgWriteBool(Section,L"Global_Enable",DebugEnabled);

	CfgWriteBool(Section,L"Show_Messages",             _MsgToConsole);
	CfgWriteBool(Section,L"Show_Messages_Key_On_Off",  _MsgKeyOnOff);
	CfgWriteBool(Section,L"Show_Messages_Voice_Off",   _MsgVoiceOff);
	CfgWriteBool(Section,L"Show_Messages_DMA_Transfer",_MsgDMA);
	CfgWriteBool(Section,L"Show_Messages_AutoDMA",     _MsgAutoDMA);
	CfgWriteBool(Section,L"Show_Messages_Overruns",    _MsgOverruns);
	CfgWriteBool(Section,L"Show_Messages_CacheStats",  _MsgCache);

	CfgWriteBool(Section,L"Log_Register_Access",_AccessLog);
	CfgWriteBool(Section,L"Log_DMA_Transfers",  _DMALog);
	CfgWriteBool(Section,L"Log_WAVE_Output",    _WaveLog);

	CfgWriteBool(Section,L"Dump_Info",  _CoresDump);
	CfgWriteBool(Section,L"Dump_Memory",_MemDump);
	CfgWriteBool(Section,L"Dump_Regs",  _RegDump);

	set_default_filenames();
	CfgWriteStr(Section,L"Access_Log_Filename",AccessLogFileName);
	CfgWriteStr(Section,L"WaveLog_Filename",   WaveLogFileName);
	CfgWriteStr(Section,L"DMA4Log_Filename",   DMA4LogFileName);
	CfgWriteStr(Section,L"DMA7Log_Filename",   DMA7LogFileName);

	CfgWriteStr(Section,L"Info_Dump_Filename",CoresDumpFileName);
	CfgWriteStr(Section,L"Mem_Dump_Filename", MemDumpFileName);
	CfgWriteStr(Section,L"Reg_Dump_Filename", RegDumpFileName);

}

void DisplayDialog()
{
    GtkWidget *dialog;
    int return_value;

    GtkWidget *msg_box, *log_box, *dump_box, *main_box;
    GtkWidget *msg_frame, *log_frame, *dump_frame, *main_frame;

    GtkWidget *msg_console_check, *msg_key_check, *msg_voice_check, *msg_dma_check;
    GtkWidget *msg_autodma_check, *msg_overrun_check, *msg_cache_check;

    GtkWidget *log_access_check, *log_dma_check, *log_wave_check;
    GtkWidget *dump_core_check, *dump_mem_check, *dump_reg_check;

	ReadSettings();

    // Create the widgets
    dialog = gtk_dialog_new_with_buttons (
		"Spu2-X Config",
		NULL, // parent window
		(GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
		GTK_STOCK_OK,
			GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL,
			GTK_RESPONSE_REJECT,
		NULL);

    main_box = gtk_hbox_new(false, 5);
    main_frame = gtk_frame_new ("Spu2-X Config");
    gtk_container_add (GTK_CONTAINER(main_frame), main_box);

    // Message Section

    msg_box =  gtk_vbox_new(false, 5);

	msg_console_check = gtk_check_button_new_with_label("Show In Console");
    msg_key_check = gtk_check_button_new_with_label("KeyOn/Off Events");
    msg_voice_check = gtk_check_button_new_with_label("Voice Stop Events");
    msg_dma_check = gtk_check_button_new_with_label("DMA Operations");
    msg_autodma_check = gtk_check_button_new_with_label("AutoDMA Operations");
    msg_overrun_check = gtk_check_button_new_with_label("Buffer Over/Underruns");
    msg_cache_check = gtk_check_button_new_with_label("ADPCM Cache Statistics");

	gtk_container_add(GTK_CONTAINER(msg_box), msg_console_check);
	gtk_container_add(GTK_CONTAINER(msg_box), msg_key_check);
	gtk_container_add(GTK_CONTAINER(msg_box), msg_voice_check);
	gtk_container_add(GTK_CONTAINER(msg_box), msg_dma_check);
	gtk_container_add(GTK_CONTAINER(msg_box), msg_autodma_check);
	gtk_container_add(GTK_CONTAINER(msg_box), msg_overrun_check);
	gtk_container_add(GTK_CONTAINER(msg_box), msg_cache_check);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(msg_console_check), _MsgToConsole);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(msg_key_check), _MsgKeyOnOff);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(msg_voice_check), _MsgVoiceOff);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(msg_dma_check), _MsgDMA);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(msg_autodma_check), _MsgAutoDMA);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(msg_overrun_check), _MsgOverruns);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(msg_cache_check), _MsgCache);

    msg_frame = gtk_frame_new ("Message/Log Options");
    gtk_container_add (GTK_CONTAINER(msg_frame), msg_box);

    // Log Section
    log_box =  gtk_vbox_new(false, 5);

    log_access_check = gtk_check_button_new_with_label("Log Register/DMA Actions");
    log_dma_check = gtk_check_button_new_with_label("Log DMA Writes");
    log_wave_check = gtk_check_button_new_with_label("Log Audio Output");

	gtk_container_add(GTK_CONTAINER(log_box), log_access_check);
	gtk_container_add(GTK_CONTAINER(log_box), log_dma_check);
	gtk_container_add(GTK_CONTAINER(log_box), log_wave_check);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(log_access_check), _AccessLog);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(log_dma_check), _DMALog);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(log_wave_check), _WaveLog);

    log_frame = gtk_frame_new ("Log Options");
    gtk_container_add (GTK_CONTAINER(log_frame), log_box);

    // Dump Section
    dump_box = gtk_vbox_new(false, 5);

    dump_core_check = gtk_check_button_new_with_label("Dump Core and Voice State");
    dump_mem_check = gtk_check_button_new_with_label("Dump Memory Contents");
    dump_reg_check = gtk_check_button_new_with_label("Dump Register Data");

	gtk_container_add(GTK_CONTAINER(dump_box), dump_core_check);
	gtk_container_add(GTK_CONTAINER(dump_box), dump_mem_check);
	gtk_container_add(GTK_CONTAINER(dump_box), dump_reg_check);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dump_core_check), _CoresDump);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dump_mem_check), _MemDump);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dump_reg_check), _RegDump);

    dump_frame = gtk_frame_new ("Dumps (on close)");
    gtk_container_add (GTK_CONTAINER(dump_frame), dump_box);

    // Add everything

    gtk_container_add (GTK_CONTAINER(main_box), msg_frame);
    gtk_container_add (GTK_CONTAINER(main_box), log_frame);
    gtk_container_add (GTK_CONTAINER(main_box), dump_frame);

    // Add all our widgets, and show everything we've added to the dialog.
    gtk_container_add (GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), main_frame);
    gtk_widget_show_all (dialog);

    return_value = gtk_dialog_run (GTK_DIALOG (dialog));

    if (return_value == GTK_RESPONSE_ACCEPT)
    {
		_MsgToConsole = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(msg_console_check));
		_MsgKeyOnOff = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(msg_key_check));
		_MsgVoiceOff = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(msg_voice_check));
		_MsgDMA = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(msg_dma_check));
		_MsgAutoDMA = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(msg_autodma_check));
		_MsgOverruns = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(msg_overrun_check));
		_MsgCache = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(msg_cache_check));

		_AccessLog = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(log_access_check));
		_DMALog = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(log_dma_check));
		_WaveLog = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(log_wave_check));

		_CoresDump = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dump_core_check));
		_MemDump = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dump_mem_check));
		_RegDump = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dump_reg_check));
    }

    gtk_widget_destroy (dialog);

    WriteSettings();
}

}
