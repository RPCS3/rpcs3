#include <gtk/gtk.h>


void
OnAbout_Ok                             (GtkDialog       *dialog,
                                        gint             response_id,
                                        gpointer         user_data);

void
OnConf_Pad1                            (GtkButton       *button,
                                        gpointer         user_data);

void
OnConf_Pad2                            (GtkButton       *button,
                                        gpointer         user_data);

void
OnConf_Pad3                            (GtkButton       *button,
                                        gpointer         user_data);

void
OnConf_Pad4                            (GtkButton       *button,
                                        gpointer         user_data);

void
on_joydevicescombo_changed             (GtkComboBox     *combobox,
                                        gpointer         user_data);

void
OnConf_Key                             (GtkButton       *button,
                                        gpointer         user_data);

void
on_checkbutton_reversery_toggled       (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_checkbutton_reverserx_toggled       (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_forcefeedback_toggled               (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_checkbutton_reverselx_toggled       (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_checkbutton_reversely_toggled       (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
OnConf_Ok                              (GtkButton       *button,
                                        gpointer         user_data);

void
OnConf_Cancel                          (GtkButton       *button,
                                        gpointer         user_data);
