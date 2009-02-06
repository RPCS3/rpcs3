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

#include "DebugDlg.h"
using namespace R5900;

void UpdateDebugger() {

	char *str;
	int i;
	std::string output;
	
	DebugAdj->value = (gfloat)dPC/4;
	gtk_list_store_clear(ListDVModel);

	for (i=0; i<23; i++) {
		GtkTreeIter iter;
		u32 *mem;
		u32 pc = dPC + i*4;
		if (DebugMode) {
			mem = (u32*)PSXM(pc);
		} 
		else
			mem =  (u32*)PSM(pc);
		
		if (mem == NULL) { 
			sprintf(nullAddr, "%8.8lX:\tNULL MEMORY", pc);
			str = nullAddr; 
			}
		else 
		{
			std::string output;
			
			disR5900Fasm(output, *mem, pc);
			output.copy( str, 256 );
		}
		gtk_list_store_append(ListDVModel, &iter);
		gtk_list_store_set(ListDVModel, &iter, 0, str, -1);

	}
}

void OnDebug_Close(GtkButton *button, gpointer user_data) {
	ClosePlugins();
	gtk_widget_destroy(DebugWnd);
	gtk_main_quit();
	gtk_widget_set_sensitive(MainWindow, TRUE);
}

void OnDebug_ScrollChange(GtkAdjustment *adj) {
	dPC = (u32)adj->value*4;
	dPC&= ~0x3;
	
	UpdateDebugger();
}

void OnSetPC_Ok(GtkButton *button, gpointer user_data) {
	char *str = (char*)gtk_entry_get_text(GTK_ENTRY(SetPCEntry));

	sscanf(str, "%lx", &dPC);
	dPC&= ~0x3;

	gtk_widget_destroy(SetPCDlg);
	gtk_main_quit();
	gtk_widget_set_sensitive(DebugWnd, TRUE);
	UpdateDebugger();
}

void OnSetPC_Cancel(GtkButton *button, gpointer user_data) {
	gtk_widget_destroy(SetPCDlg);
	gtk_main_quit();
	gtk_widget_set_sensitive(DebugWnd, TRUE);
}

void OnDebug_SetPC(GtkButton *button, gpointer user_data) {
	SetPCDlg = create_SetPCDlg();
	
	SetPCEntry = lookup_widget(SetPCDlg, "GtkEntry_dPC");
	
	gtk_widget_show_all(SetPCDlg);
	gtk_widget_set_sensitive(DebugWnd, FALSE);
	gtk_main();
}

void OnSetBPA_Ok(GtkButton *button, gpointer user_data) {
	char *str = (char*)gtk_entry_get_text(GTK_ENTRY(SetBPAEntry));

	sscanf(str, "%lx", &dBPA);
	dBPA&= ~0x3;

	gtk_widget_destroy(SetBPADlg);
	gtk_main_quit();
	gtk_widget_set_sensitive(DebugWnd, TRUE);
	UpdateDebugger();
}

void OnSetBPA_Cancel(GtkButton *button, gpointer user_data) {
	gtk_widget_destroy(SetBPADlg);
	gtk_main_quit();
	gtk_widget_set_sensitive(DebugWnd, TRUE);
}

void OnDebug_SetBPA(GtkButton *button, gpointer user_data) {
	SetBPADlg = create_SetBPADlg();
	
	SetBPAEntry = lookup_widget(SetBPADlg, "GtkEntry_BPA");

 	gtk_widget_show_all(SetBPADlg);
	gtk_widget_set_sensitive(DebugWnd, FALSE);
	gtk_main();
}

void OnSetBPC_Ok(GtkButton *button, gpointer user_data) {
	char *str = (char*)gtk_entry_get_text(GTK_ENTRY(SetBPCEntry));

	sscanf(str, "%lx", &dBPC);

	gtk_widget_destroy(SetBPCDlg);
	gtk_main_quit();
	gtk_widget_set_sensitive(DebugWnd, TRUE);
	UpdateDebugger();
}

void OnSetBPC_Cancel(GtkButton *button, gpointer user_data) {
	gtk_widget_destroy(SetBPCDlg);
	gtk_main_quit();
	gtk_widget_set_sensitive(DebugWnd, TRUE);
}

void OnDebug_SetBPC(GtkButton *button, gpointer user_data) {
	SetBPCDlg = create_SetBPCDlg();
	
	SetBPCEntry = lookup_widget(SetBPCDlg, "GtkEntry_BPC");

 	gtk_widget_show_all(SetBPCDlg);
	gtk_widget_set_sensitive(DebugWnd, FALSE);
	gtk_main();
}

void OnDebug_ClearBPs(GtkButton *button, gpointer user_data) {
	dBPA = -1;
	dBPC = -1;
}

void OnDumpC_Ok(GtkButton *button, gpointer user_data) {
	FILE *f;
	char *str = (char*)gtk_entry_get_text(GTK_ENTRY(DumpCFEntry));
	u32 addrf, addrt;

	sscanf(str, "%lx", &addrf); addrf&=~0x3;
	str = (char*)gtk_entry_get_text(GTK_ENTRY(DumpCTEntry));
	sscanf(str, "%lx", &addrt); addrt&=~0x3;

	f = fopen("dump.txt", "w");
	if (f == NULL) return;

	while (addrf != addrt) {
		u32 *mem;

		if (DebugMode) {
			mem = (u32*)PSXM(addrf);
		} 
		else {
			mem = (u32*)PSM(addrf);
		}
		
		if (mem == NULL) { 
			sprintf(nullAddr, "%8.8lX:\tNULL MEMORY", addrf); 
			str = nullAddr; 
		}
		else 
		{
			std::string output;
			
			disR5900Fasm(output, *mem, addrf);
			output.copy( str, 256 );
		}

		fprintf(f, "%s\n", str);
		addrf+= 4;
	}

	fclose(f);
	gtk_widget_destroy(DumpCDlg);
	gtk_main_quit();
	gtk_widget_set_sensitive(DebugWnd, TRUE);
}

void OnDumpC_Cancel(GtkButton *button, gpointer user_data) {
gtk_widget_destroy(DumpCDlg);
	gtk_main_quit();
	gtk_widget_set_sensitive(DebugWnd, TRUE);
}

void OnDebug_DumpCode(GtkButton *button, gpointer user_data) {
	DumpCDlg = create_DumpCDlg();
	
	DumpCFEntry = lookup_widget(DumpCDlg, "GtkEntry_DumpCF");
	DumpCTEntry = lookup_widget(DumpCDlg, "GtkEntry_DumpCT");

 	gtk_widget_show_all(DumpCDlg);
	gtk_widget_set_sensitive(DebugWnd, FALSE);
	gtk_main();
}

void OnDumpR_Ok(GtkButton *button, gpointer user_data) {
	FILE *f;
	char *str = (char*)gtk_entry_get_text(GTK_ENTRY(DumpRFEntry));
	u32 addrf, addrt;

	sscanf(str, "%lx", &addrf); addrf&=~0x3;
	str = (char*)gtk_entry_get_text(GTK_ENTRY(DumpRTEntry));
	sscanf(str, "%lx", &addrt); addrt&=~0x3;

	f = fopen("dump.txt", "w");
	if (f == NULL) return;

	while (addrf != addrt) {
		u32 *mem;
		u32 out;

		if (DebugMode) {
			mem = (u32*)PSXM(addrf);
		} else {
			mem = (u32*)PSM(addrf);
		}
		if (mem == NULL) out = 0;
		else out = *mem;

		fwrite(&out, 4, 1, f);
		addrf+= 4;
	}

	fclose(f);
	gtk_widget_destroy(DumpRDlg);
	gtk_main_quit();
	gtk_widget_set_sensitive(DebugWnd, TRUE);
}

void OnDumpR_Cancel(GtkButton *button, gpointer user_data) {
	gtk_widget_destroy(DumpRDlg);
	gtk_main_quit();
	gtk_widget_set_sensitive(DebugWnd, TRUE);
}

void OnDebug_RawDump(GtkButton *button, gpointer user_data) {
	DumpRDlg = create_DumpRDlg();
	
	DumpRFEntry = lookup_widget(DumpRDlg, "GtkEntry_DumpRF");
	DumpRTEntry = lookup_widget(DumpRDlg, "GtkEntry_DumpRT");

 	gtk_widget_show_all(DumpRDlg);
	gtk_widget_set_sensitive(DebugWnd, FALSE);
	gtk_main();
}

void OnDebug_Step(GtkButton *button, gpointer user_data) {
	Cpu->Step();
	dPC = cpuRegs.pc;
	UpdateDebugger();
}

void OnDebug_Skip(GtkButton *button, gpointer user_data) {
	cpuRegs.pc+= 4;
	dPC = cpuRegs.pc;
	UpdateDebugger();
}

int HasBreakPoint(u32 pc) {
	if (pc == dBPA) return 1;
	if (DebugMode == 0) {
		if ((cpuRegs.cycle - 10) <= dBPC &&
			(cpuRegs.cycle + 10) >= dBPC) return 1;
	} else {
		if ((psxRegs.cycle - 100) <= dBPC &&
			(psxRegs.cycle + 100) >= dBPC) return 1;
	}
	return 0;
}

void OnDebug_Go(GtkButton *button, gpointer user_data) {
	for (;;) {
		if (HasBreakPoint(cpuRegs.pc)) break;
		Cpu->Step();
	}
	dPC = cpuRegs.pc;
	UpdateDebugger();
}

void OnDebug_Log(GtkButton *button, gpointer user_data) {
#ifdef PCSX2_DEVBUILD
	//Log = 1 - Log;
#endif
}

void OnDebug_EEMode(GtkToggleButton *togglebutton, gpointer user_data) {
	DebugMode = 0;
	dPC = cpuRegs.pc;
	UpdateDebugger();
}

void OnDebug_IOPMode(GtkToggleButton *togglebutton, gpointer user_data) {
	DebugMode = 1;
	dPC = psxRegs.pc;
	UpdateDebugger();
}

void OnMemWrite32_Ok(GtkButton *button, gpointer user_data) {
	char *mem  = (char*)gtk_entry_get_text(GTK_ENTRY(MemEntry));
	char *data = (char*)gtk_entry_get_text(GTK_ENTRY(DataEntry));

	printf("memWrite32: %s, %s\n", mem, data);
	memWrite32(strtol(mem, (char**)NULL, 0), strtol(data, (char**)NULL, 0));
	gtk_widget_destroy(MemWriteDlg);
	gtk_main_quit();
	gtk_widget_set_sensitive(DebugWnd, TRUE);
}

void OnMemWrite32_Cancel(GtkButton *button, gpointer user_data) {
	gtk_widget_destroy(MemWriteDlg);
	gtk_main_quit();
	gtk_widget_set_sensitive(DebugWnd, TRUE);
}

void OnDebug_memWrite32(GtkButton *button, gpointer user_data) {
	MemWriteDlg = create_MemWrite32();

	MemEntry = lookup_widget(MemWriteDlg, "GtkEntry_Mem");
	DataEntry = lookup_widget(MemWriteDlg, "GtkEntry_Data");

 	gtk_widget_show_all(MemWriteDlg);
	gtk_widget_set_sensitive(DebugWnd, FALSE);
	gtk_main();

	UpdateDebugger();
}

void OnDebug_Debugger(GtkMenuItem *menuitem, gpointer user_data) {
	GtkWidget *scroll;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	if (OpenPlugins(NULL) == -1) return;

	/*if (!efile)
		efile=GetPS2ElfName(elfname);
	if (efile)
		loadElfFile(elfname);
	efile=0;*/

	dPC = cpuRegs.pc;
	
	DebugWnd = create_DebugWnd();
	
 	ListDVModel = gtk_list_store_new (1, G_TYPE_STRING);
	ListDV = lookup_widget(DebugWnd, "GtkList_DisView");
	gtk_tree_view_set_model(GTK_TREE_VIEW(ListDV), GTK_TREE_MODEL(ListDVModel));
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("heading", renderer,
	                                                   "text", 0,
	                                                   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (ListDV), column);
	scroll = lookup_widget(DebugWnd, "GtkVScrollbar_VList");

	DebugAdj = GTK_RANGE(scroll)->adjustment;
	DebugAdj->lower = (gfloat)0x00000000/4;
	DebugAdj->upper = (gfloat)0xffffffff/4;
	DebugAdj->step_increment = (gfloat)1;
	DebugAdj->page_increment = (gfloat)20;
	DebugAdj->page_size = (gfloat)23;

	gtk_signal_connect(GTK_OBJECT(DebugAdj),
	                   "value_changed", GTK_SIGNAL_FUNC(OnDebug_ScrollChange),
					   NULL);

	UpdateDebugger();

	gtk_widget_show_all(DebugWnd);
	gtk_widget_set_sensitive(MainWindow, FALSE);
	gtk_main();
}