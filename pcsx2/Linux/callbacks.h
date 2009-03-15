#include <gtk/gtk.h>


void
on_Advanced_Defaults                   (GtkButton       *button,
                                        gpointer         user_data);

void
on_Advanced_OK                         (GtkButton       *button,
                                        gpointer         user_data);

void
On_Dialog_Cancelled                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_Speed_Hack_OK                       (GtkButton       *button,
                                        gpointer         user_data);

void
on_Game_Fix_OK                         (GtkButton       *button,
                                        gpointer         user_data);

void
OnMemWrite32_Ok                        (GtkButton       *button,
                                        gpointer         user_data);

void
OnArguments_Ok                         (GtkButton       *button,
                                        gpointer         user_data);

void
OnDumpR_Ok                             (GtkButton       *button,
                                        gpointer         user_data);

void
OnDumpC_Ok                             (GtkButton       *button,
                                        gpointer         user_data);

void
OnSetBPC_Ok                            (GtkButton       *button,
                                        gpointer         user_data);

void
OnSetBPA_Ok                            (GtkButton       *button,
                                        gpointer         user_data);

void
OnSetBPA_Cancel                        (GtkButton       *button,
                                        gpointer         user_data);

void
OnSetPC_Ok                             (GtkButton       *button,
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
OnConfButton                           (GtkButton       *button,
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
OnHelpAbout_Ok                         (GtkButton       *button,
                                        gpointer         user_data);

void
OnDestroy                              (GtkObject       *object,
                                        gpointer         user_data);

gboolean
OnDelete                               (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
OnFile_RunCD                           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
OnFile_LoadElf                         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
OnStates_Load                          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
OnStates_LoadOther                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
OnStates_Save                          (GtkMenuItem     *menuitem,
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
OnConf_Conf                            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
OnConf_Menu                            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
OnConf_Memcards                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
OnConf_Cpu                             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_Game_Fixes                          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_Speed_Hacks                         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_Advanced                            (GtkMenuItem     *menuitem,
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
OnPrintCdvdInfo                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
OnDebug_Debugger                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
OnDebug_Logging                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
OnEmu_Arguments                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
OnHelp_About                           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
OnCpu_Ok                               (GtkButton       *button,
                                        gpointer         user_data);

void
OnLogging_Ok                           (GtkButton       *button,
                                        gpointer         user_data);

void
OnMemcards_Ok                          (GtkButton       *button,
                                        gpointer         user_data);
