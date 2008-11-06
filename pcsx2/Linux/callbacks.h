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
#include <gtk/gtk.h>


void
OnDestroy                              (GtkObject       *object,
                                        gpointer         user_data);

void
OnFile_RunCD                           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
OnFile_LoadElf                         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
OnStates_Load1                         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
OnStates_Load2                         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
OnStates_Load3                         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
OnStates_Load4                         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
OnStates_Load5                         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
OnStates_LoadOther                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
OnStates_Save1                         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
OnStates_Save2                         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
OnStates_Save3                         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
OnStates_Save4                         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
OnStates_Save5                         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
OnStates_SaveOther                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
OnFile_Exit                            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
OnEmu_Run                              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
OnEmu_Reset                            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
OnEmu_Arguments                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
OnConf_Conf                            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
OnConf_Gs                              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
OnConf_Pads                            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
OnConf_Spu2                            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
OnConf_Cdvd                            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
OnConf_Dev9                            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
OnConf_Usb                             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
OnConf_Fw                              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
OnConf_Cpu                             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_patch_browser1_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_patch_finder2_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_enable_console1_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_enable_patches1_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
OnDebug_Debugger                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
OnDebug_Logging                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
OnHelp_About                           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
OnHelpAbout_Ok                         (GtkButton       *button,
                                        gpointer         user_data);

void
OnConfConf_Pad2Conf                    (GtkButton       *button,
                                        gpointer         user_data);

void
OnConfConf_Pad2Test                    (GtkButton       *button,
                                        gpointer         user_data);

void
OnConfConf_Pad2About                   (GtkButton       *button,
                                        gpointer         user_data);

void
OnConfConf_Pad1Conf                    (GtkButton       *button,
                                        gpointer         user_data);

void
OnConfConf_Pad1Test                    (GtkButton       *button,
                                        gpointer         user_data);

void
OnConfConf_Pad1About                   (GtkButton       *button,
                                        gpointer         user_data);

void
OnConfConf_GsConf                      (GtkButton       *button,
                                        gpointer         user_data);

void
OnConfConf_GsTest                      (GtkButton       *button,
                                        gpointer         user_data);

void
OnConfConf_GsAbout                     (GtkButton       *button,
                                        gpointer         user_data);

void
OnConfConf_Spu2Conf                    (GtkButton       *button,
                                        gpointer         user_data);

void
OnConfConf_Spu2Test                    (GtkButton       *button,
                                        gpointer         user_data);

void
OnConfConf_Spu2About                   (GtkButton       *button,
                                        gpointer         user_data);

void
OnConfConf_Dev9Conf                    (GtkButton       *button,
                                        gpointer         user_data);

void
OnConfConf_Dev9Test                    (GtkButton       *button,
                                        gpointer         user_data);

void
OnConfConf_Dev9About                   (GtkButton       *button,
                                        gpointer         user_data);

void
OnConfConf_CdvdConf                    (GtkButton       *button,
                                        gpointer         user_data);

void
OnConfConf_CdvdTest                    (GtkButton       *button,
                                        gpointer         user_data);

void
OnConfConf_CdvdAbout                   (GtkButton       *button,
                                        gpointer         user_data);

void
OnConfConf_UsbConf                     (GtkButton       *button,
                                        gpointer         user_data);

void
OnConfConf_UsbTest                     (GtkButton       *button,
                                        gpointer         user_data);

void
OnConfConf_UsbAbout                    (GtkButton       *button,
                                        gpointer         user_data);

void
OnConfConf_FWConf                      (GtkButton       *button,
                                        gpointer         user_data);

void
OnConfConf_FWTest                      (GtkButton       *button,
                                        gpointer         user_data);

void
OnConfConf_FWAbout                     (GtkButton       *button,
                                        gpointer         user_data);

void
OnConfConf_PluginsPath                 (GtkButton       *button,
                                        gpointer         user_data);

void
OnConfConf_BiosPath                    (GtkButton       *button,
                                        gpointer         user_data);

void
OnConfConf_Ok                          (GtkButton       *button,
                                        gpointer         user_data);

void
OnConfConf_Cancel                      (GtkButton       *button,
                                        gpointer         user_data);

void
OnCpu_Ok                               (GtkButton       *button,
                                        gpointer         user_data);

void
OnCpu_Cancel                           (GtkButton       *button,
                                        gpointer         user_data);

void
OnDebug_EEMode                         (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
OnDebug_IOPMode                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
OnDebug_Step                           (GtkButton       *button,
                                        gpointer         user_data);

void
OnDebug_Skip                           (GtkButton       *button,
                                        gpointer         user_data);

void
OnDebug_Go                             (GtkButton       *button,
                                        gpointer         user_data);

void
OnDebug_Log                            (GtkButton       *button,
                                        gpointer         user_data);

void
OnDebug_SetPC                          (GtkButton       *button,
                                        gpointer         user_data);

void
OnDebug_SetBPA                         (GtkButton       *button,
                                        gpointer         user_data);

void
OnDebug_SetBPC                         (GtkButton       *button,
                                        gpointer         user_data);

void
OnDebug_ClearBPs                       (GtkButton       *button,
                                        gpointer         user_data);

void
OnDebug_DumpCode                       (GtkButton       *button,
                                        gpointer         user_data);

void
OnDebug_RawDump                        (GtkButton       *button,
                                        gpointer         user_data);

void
OnDebug_Close                          (GtkButton       *button,
                                        gpointer         user_data);

void
OnDebug_memWrite32                     (GtkButton       *button,
                                        gpointer         user_data);

void
OnSetPC_Ok                             (GtkButton       *button,
                                        gpointer         user_data);

void
OnSetPC_Cancel                         (GtkButton       *button,
                                        gpointer         user_data);

void
OnSetBPA_Ok                            (GtkButton       *button,
                                        gpointer         user_data);

void
OnSetBPA_Cancel                        (GtkButton       *button,
                                        gpointer         user_data);

void
OnSetBPC_Ok                            (GtkButton       *button,
                                        gpointer         user_data);

void
OnSetBPC_Cancel                        (GtkButton       *button,
                                        gpointer         user_data);

void
OnDumpC_Ok                             (GtkButton       *button,
                                        gpointer         user_data);

void
OnDumpC_Cancel                         (GtkButton       *button,
                                        gpointer         user_data);

void
OnDumpR_Ok                             (GtkButton       *button,
                                        gpointer         user_data);

void
OnDumpR_Cancel                         (GtkButton       *button,
                                        gpointer         user_data);

void
OnLogging_Ok                           (GtkButton       *button,
                                        gpointer         user_data);

void
OnLogging_Cancel                       (GtkButton       *button,
                                        gpointer         user_data);

void
OnArguments_Ok                         (GtkButton       *button,
                                        gpointer         user_data);

void
OnArguments_Cancel                     (GtkButton       *button,
                                        gpointer         user_data);

void
OnMemWrite32_Ok                        (GtkButton       *button,
                                        gpointer         user_data);

void
OnMemWrite32_Cancel                    (GtkButton       *button,
                                        gpointer         user_data);
