/*
 * Copyright (C) 2007  Ignacio Casal Quinteiro <nacho.resa@gmail.com>
 *               2008  Pablo Sanxiao <psanxiao@gmail.com>
 * 
 *     This program is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 * 
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 * 
 *     You should have received a copy of the GNU General Public License
 *     along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "application.h"
#include "preferences-dialog.h"
#include "prefs-manager.h"
#include "profile.h"
#include "utils.h"
#include "plugin-manager.h"
#include "profile-dialog.h"
#include "po.h"
#include "utils.h"
#include "../translation-memory/translation-memory.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <gtk/gtk.h>


#define GTR_PREFERENCES_DIALOG_GET_PRIVATE(object)	(G_TYPE_INSTANCE_GET_PRIVATE ( \
						 	(object),	\
						 	GTR_TYPE_PREFERENCES_DIALOG,     \
						 	GtranslatorPreferencesDialogPrivate))


G_DEFINE_TYPE(GtranslatorPreferencesDialog, gtranslator_preferences_dialog, GTK_TYPE_DIALOG)

struct _GtranslatorPreferencesDialogPrivate
{
	GtkWidget *notebook;
	
	/* Files->General */
	GtkWidget *warn_if_fuzzy_checkbutton;
	GtkWidget *delete_compiled_checkbutton;
	
	/* Files->Autosave */
	GtkWidget *autosave_checkbutton;
	GtkWidget *autosave_interval_spinbutton;
	GtkWidget *autosave_hbox;
	GtkWidget *append_suffix_checkbutton;
	GtkWidget *autosave_suffix_entry;
	GtkWidget *autosave_suffix_hbox;
	
	/* Editor->Text display */
	GtkWidget *highlight_checkbutton;
	GtkWidget *visible_whitespace_checkbutton;
	GtkWidget *use_custom_font_checkbutton;
	GtkWidget *editor_font_fontbutton;
	GtkWidget *editor_font_hbox;
	
	/* Editor->Contents */
	GtkWidget *unmark_fuzzy_checkbutton;
	GtkWidget *keep_obsolete_checkbutton;
	GtkWidget *spellcheck_checkbutton;

	/*Profiles*/
	GtkWidget *profile_treeview;
	GtkWidget *add_button;
	GtkWidget *edit_button;
	GtkWidget *delete_button;

        /*Translation Memory*/
        GtkWidget *directory_entry;
        GtkWidget *search_button;
        GtkWidget *add_database_button;

        GtkWidget *show_tm_options_checkbutton;
        GtkWidget *missing_words_spinbutton;
        GtkWidget *sentence_length_spinbutton;

	/*PO header->Personal information*/
	GtkWidget *name_entry;
	GtkWidget *email_entry;
	
	/*PO header->Language settings*/
	GtkWidget *language_comboentry;
	GtkWidget *langcode_comboentry;
	GtkWidget *charset_comboentry;
	GtkWidget *encoding_comboentry;
	GtkWidget *team_email_comboentry;
	GtkWidget *plurals_entry;
	GtkWidget *number_plurals_spinbutton;
	
	/*Inteface*/
	GtkWidget *gdl_combobox;
	
	/*Plugins*/
	GtkWidget *plugins_box;
};

static void setup_profile_pages (GtranslatorPreferencesDialog *dlg);

GtkWidget *gtranslator_preferences_dialog_get_treeview (GtranslatorPreferencesDialog *dlg) 
{
  return dlg->priv->profile_treeview;
}

/***************Files pages****************/

static void
warn_if_fuzzy_checkbutton_toggled(GtkToggleButton *button,
				  GtranslatorPreferencesDialog *dlg)
{
	g_return_if_fail(button == GTK_TOGGLE_BUTTON(dlg->priv->warn_if_fuzzy_checkbutton));
	
	gtranslator_prefs_manager_set_warn_if_fuzzy(gtk_toggle_button_get_active(button));
}

static void
delete_compiled_checkbutton_toggled(GtkToggleButton *button,
				       GtranslatorPreferencesDialog *dlg)
{
	g_return_if_fail(button == GTK_TOGGLE_BUTTON(dlg->priv->delete_compiled_checkbutton));
	
	gtranslator_prefs_manager_set_delete_compiled(gtk_toggle_button_get_active(button));
}

static void
setup_files_general_page(GtranslatorPreferencesDialog *dlg)
{	
	/*Set initial value*/
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dlg->priv->warn_if_fuzzy_checkbutton),
				     gtranslator_prefs_manager_get_warn_if_fuzzy());
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dlg->priv->delete_compiled_checkbutton),
				     gtranslator_prefs_manager_get_delete_compiled());
	
	/*Connect signals*/
	g_signal_connect(dlg->priv->warn_if_fuzzy_checkbutton, "toggled",
			 G_CALLBACK(warn_if_fuzzy_checkbutton_toggled),
			 dlg);
	g_signal_connect(dlg->priv->delete_compiled_checkbutton, "toggled",
			 G_CALLBACK(delete_compiled_checkbutton_toggled),
			 dlg);
}

static void
autosave_checkbutton_toggled(GtkToggleButton *button,
			     GtranslatorPreferencesDialog *dlg)
{
	gboolean autosave;
	
	g_return_if_fail(button == GTK_TOGGLE_BUTTON(dlg->priv->autosave_checkbutton));
	
	autosave = gtk_toggle_button_get_active(button);
	
	gtk_widget_set_sensitive(dlg->priv->autosave_hbox, autosave);
	gtranslator_prefs_manager_set_autosave(autosave);
}

static void
append_suffix_checkbutton_toggled(GtkToggleButton *button,
					 GtranslatorPreferencesDialog *dlg)
{
	gboolean append_suffix;
	
	g_return_if_fail(button == GTK_TOGGLE_BUTTON(dlg->priv->append_suffix_checkbutton));
	
	append_suffix = gtk_toggle_button_get_active(button);
	
	gtk_widget_set_sensitive(dlg->priv->autosave_suffix_hbox, append_suffix);
	gtranslator_prefs_manager_set_append_suffix(append_suffix);
}

static void
autosave_interval_spinbutton_value_changed(GtkSpinButton *spin_button,
					   GtranslatorPreferencesDialog *dlg)
{
	g_return_if_fail (spin_button == GTK_SPIN_BUTTON (dlg->priv->autosave_interval_spinbutton));
	
	gtranslator_prefs_manager_set_autosave_interval(gtk_spin_button_get_value_as_int(spin_button));
}

static void
autosave_suffix_entry_changed(GObject    *gobject,
			      GParamSpec *arg1,
			      GtranslatorPreferencesDialog *dlg)
{
	const gchar *text;
	
	text = gtk_entry_get_text(GTK_ENTRY(gobject));
	
	gtranslator_prefs_manager_set_autosave_suffix(text);
}

static void
setup_files_autosave_page(GtranslatorPreferencesDialog *dlg)
{
	gboolean autosave, suffix;
	gint autosave_interval;
	const gchar *autosave_suffix;

	/*Set initial value*/
	autosave = gtranslator_prefs_manager_get_autosave();
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dlg->priv->autosave_checkbutton),
				     autosave);
	suffix = gtranslator_prefs_manager_get_append_suffix();
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dlg->priv->append_suffix_checkbutton),
				     suffix);
	
	autosave_interval = gtranslator_prefs_manager_get_autosave_interval();
	
	if(autosave_interval <= 0)
		autosave_interval = GPM_DEFAULT_AUTOSAVE_INTERVAL;
	
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(dlg->priv->autosave_interval_spinbutton),
				  autosave_interval);
	
	autosave_suffix = gtranslator_prefs_manager_get_autosave_suffix();
	if(!autosave_suffix)
		autosave_suffix = GPM_DEFAULT_AUTOSAVE_SUFFIX;
	gtk_entry_set_text(GTK_ENTRY(dlg->priv->autosave_suffix_entry),
			   autosave_suffix);
	
	
	/*Set sensitive*/
	gtk_widget_set_sensitive(dlg->priv->autosave_hbox,
				 autosave);
	gtk_widget_set_sensitive(dlg->priv->autosave_suffix_hbox,
				 suffix);
	
	/*Connect signals*/
	g_signal_connect(dlg->priv->autosave_checkbutton, "toggled",
			 G_CALLBACK(autosave_checkbutton_toggled),
			 dlg);
	g_signal_connect(dlg->priv->append_suffix_checkbutton, "toggled",
			 G_CALLBACK(append_suffix_checkbutton_toggled),
			 dlg);
	g_signal_connect(dlg->priv->autosave_interval_spinbutton, "value-changed",
			 G_CALLBACK(autosave_interval_spinbutton_value_changed),
			 dlg);
	g_signal_connect(dlg->priv->autosave_suffix_entry, "notify::text",
			 G_CALLBACK(autosave_suffix_entry_changed),
			 dlg);
}

static void
setup_files_pages(GtranslatorPreferencesDialog *dlg)
{	
	/*Children*/
	setup_files_general_page(dlg);
	setup_files_autosave_page(dlg);
}


/***************Editor pages****************/
static void
highlight_checkbutton_toggled(GtkToggleButton *button,
			      GtranslatorPreferencesDialog *dlg)
{
	g_return_if_fail(button == GTK_TOGGLE_BUTTON(dlg->priv->highlight_checkbutton));
	
	gtranslator_prefs_manager_set_highlight(gtk_toggle_button_get_active(button));
}

static void
visible_whitespace_checkbutton_toggled(GtkToggleButton *button,
				 GtranslatorPreferencesDialog *dlg)
{
	g_return_if_fail(button == GTK_TOGGLE_BUTTON(dlg->priv->visible_whitespace_checkbutton));
	
	gtranslator_prefs_manager_set_visible_whitespace(gtk_toggle_button_get_active(button));
}

static void
use_custom_font_checkbutton_toggled(GtkToggleButton *button,
				    GtranslatorPreferencesDialog *dlg)
{
	gboolean use_custom_font;
	
	g_return_if_fail(button == GTK_TOGGLE_BUTTON(dlg->priv->use_custom_font_checkbutton));

	use_custom_font = gtk_toggle_button_get_active(button);
	
	gtranslator_prefs_manager_set_use_custom_font(use_custom_font);
	
	gtk_widget_set_sensitive(dlg->priv->editor_font_hbox, use_custom_font);
}

static void
editor_font_set(GtkFontButton *button,
		 GtranslatorPreferencesDialog *dlg)
{
	const gchar *editor_font;
	
	g_return_if_fail(button == GTK_FONT_BUTTON(dlg->priv->editor_font_fontbutton));
	
	editor_font = gtk_font_button_get_font_name(GTK_FONT_BUTTON(dlg->priv->editor_font_fontbutton));
	
	if(editor_font)
		gtranslator_prefs_manager_set_editor_font(editor_font);
	//FIXME:else set default font
}

static void
setup_editor_text_display_page(GtranslatorPreferencesDialog *dlg)
{
	gboolean use_custom_font;
	const gchar *editor_font;
	
	/*Set initial value*/
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dlg->priv->highlight_checkbutton),
				     gtranslator_prefs_manager_get_highlight());
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dlg->priv->visible_whitespace_checkbutton),
				     gtranslator_prefs_manager_get_visible_whitespace());
	
	use_custom_font = gtranslator_prefs_manager_get_use_custom_font();
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dlg->priv->use_custom_font_checkbutton),
				     use_custom_font);
	
	if(use_custom_font)
	{
		editor_font = gtranslator_prefs_manager_get_editor_font();
		if(editor_font)
			gtk_font_button_set_font_name(GTK_FONT_BUTTON(dlg->priv->editor_font_fontbutton),
						      editor_font);
		//else FIXME: I think i need to set the system font (maybe Sans 12?)
	}
	
	/*Set sensitive*/
	gtk_widget_set_sensitive(dlg->priv->editor_font_hbox, use_custom_font);
	
	/*Connect signals*/
	g_signal_connect(dlg->priv->highlight_checkbutton, "toggled",
			 G_CALLBACK(highlight_checkbutton_toggled),
			 dlg);
	g_signal_connect(dlg->priv->visible_whitespace_checkbutton, "toggled",
			 G_CALLBACK(visible_whitespace_checkbutton_toggled),
			 dlg);
	g_signal_connect(dlg->priv->use_custom_font_checkbutton, "toggled",
			 G_CALLBACK(use_custom_font_checkbutton_toggled),
			 dlg);
	g_signal_connect(dlg->priv->editor_font_fontbutton, "font-set",
			 G_CALLBACK(editor_font_set),
			 dlg);
}

static void
unmark_fuzzy_checkbutton_toggled(GtkToggleButton *button,
				 GtranslatorPreferencesDialog *dlg)
{
	g_return_if_fail(button == GTK_TOGGLE_BUTTON(dlg->priv->unmark_fuzzy_checkbutton));
	
	gtranslator_prefs_manager_set_unmark_fuzzy(gtk_toggle_button_get_active(button));
}

static void
keep_obsolete_checkbutton_toggled(GtkToggleButton *button,
				  GtranslatorPreferencesDialog *dlg)
{
	g_return_if_fail(button == GTK_TOGGLE_BUTTON(dlg->priv->keep_obsolete_checkbutton));

	gtranslator_prefs_manager_set_keep_obsolete(gtk_toggle_button_get_active(button));
}

static void
spellcheck_checkbutton_toggled(GtkToggleButton *button,
					   GtranslatorPreferencesDialog *dlg)
{
	g_return_if_fail(button == GTK_TOGGLE_BUTTON(dlg->priv->spellcheck_checkbutton));
	
	gtranslator_prefs_manager_set_spellcheck(gtk_toggle_button_get_active(button));
}

static void
setup_editor_contents(GtranslatorPreferencesDialog *dlg)
{	
	/*Set initial values*/
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dlg->priv->unmark_fuzzy_checkbutton),
				     gtranslator_prefs_manager_get_unmark_fuzzy());
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dlg->priv->keep_obsolete_checkbutton),
				     gtranslator_prefs_manager_get_keep_obsolete());
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dlg->priv->spellcheck_checkbutton),
				     gtranslator_prefs_manager_get_spellcheck());
	
	/*Connect signals*/
	g_signal_connect(dlg->priv->unmark_fuzzy_checkbutton, "toggled",
			 G_CALLBACK(unmark_fuzzy_checkbutton_toggled),
			 dlg);
	g_signal_connect(dlg->priv->keep_obsolete_checkbutton, "toggled",
			 G_CALLBACK(keep_obsolete_checkbutton_toggled),
			 dlg);
	g_signal_connect(dlg->priv->spellcheck_checkbutton, "toggled",
			 G_CALLBACK(spellcheck_checkbutton_toggled),
			 dlg);
}

static void
setup_editor_pages(GtranslatorPreferencesDialog *dlg)
{	
	/*Children*/
	setup_editor_text_display_page(dlg);
	setup_editor_contents(dlg);
}

/***************Profile pages****************/
static void
active_toggled_cb (GtkCellRendererToggle *cell_renderer,
		   gchar *path_str,
		   GtranslatorPreferencesDialog *dlg)
{
  GtkTreeIter iter;
  GtkTreePath *path;
  GtkTreeModel *model;
  gboolean active;
  gchar *profile_row;
  GtranslatorProfile *old_profile_active;
  GList *l = NULL, *profiles_list = NULL;
  gchar *filename;
  gchar *config_folder;
  GFile *file;


  config_folder = gtranslator_utils_get_user_config_dir ();
  filename = g_build_filename (config_folder,
			       "profiles.xml",
			       NULL);
  file = g_file_new_for_path (filename);

  path = gtk_tree_path_new_from_string (path_str);
  
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (dlg->priv->profile_treeview));
  g_return_if_fail (model != NULL);
  
  gtk_tree_model_get_iter (model, &iter, path);

  gtk_tree_model_get (model, &iter, TOGGLE_COL, &active, -1);
    
  if (!active) {
    
    old_profile_active = gtranslator_application_get_active_profile (GTR_APP);
    
    gtk_list_store_set (GTK_LIST_STORE (model),
			&iter,
			TRUE,
			TOGGLE_COL,
			-1);

    gtk_tree_model_get (model, &iter, PROFILE_NAME_COL, &profile_row, -1);
    profiles_list = gtranslator_application_get_profiles (GTR_APP);

    for (l = profiles_list; l; l = l->next) {
      GtranslatorProfile *profile;
      profile = (GtranslatorProfile *)l->data;
      if (!strcmp (gtranslator_profile_get_name (profile), profile_row)) {
	gtranslator_application_set_active_profile (GTR_APP, profile);
      }
    }
  }
  gtk_list_store_clear (GTK_LIST_STORE(model));
  gtranslator_preferences_fill_profile_treeview (dlg, model);
  gtk_tree_path_free (path);
}

void gtranslator_preferences_fill_profile_treeview (GtranslatorPreferencesDialog *dlg,
						    GtkTreeModel *model)
{
  GtkTreeIter iter;
  GtranslatorProfile *active_profile;
  GList *l = NULL, *profiles_list = NULL;

  gtk_list_store_clear (GTK_LIST_STORE (model));
  
  profiles_list = gtranslator_application_get_profiles (GTR_APP);
  active_profile = gtranslator_application_get_active_profile (GTR_APP);
  
  for (l = profiles_list; l; l = l->next) {
    
    GtranslatorProfile *profile;
    gchar *profile_name;
    
    profile = (GtranslatorProfile *)l->data;

    profile_name = gtranslator_profile_get_name (profile);
    gtk_list_store_append (GTK_LIST_STORE (model), &iter);
   
    if (!strcmp (gtranslator_profile_get_name (active_profile), profile_name)) {
      gtk_list_store_set (GTK_LIST_STORE (model),
			  &iter,
			  PROFILE_NAME_COL,
			  profile_name,
			  TOGGLE_COL,
			  TRUE,
			  -1);
    } else {
      gtk_list_store_set (GTK_LIST_STORE (model),
			  &iter,
			  PROFILE_NAME_COL,
			  profile_name,
			  -1);
    }
  }
}

static void
setup_profile_pages (GtranslatorPreferencesDialog *dlg)
{
  
  GtkTreeViewColumn *name_column, *toggle_column;
  GtkCellRenderer *text_renderer, *toggle_renderer;
  GtkListStore *model;
  
  model = gtk_list_store_new (N_COLUMNS_PROFILES, G_TYPE_STRING, G_TYPE_BOOLEAN);

  gtk_tree_view_set_model (GTK_TREE_VIEW (dlg->priv->profile_treeview),
			   GTK_TREE_MODEL (model)); 

  g_object_unref (model);
  
  text_renderer = gtk_cell_renderer_text_new ();
  toggle_renderer = gtk_cell_renderer_toggle_new ();

  g_signal_connect (toggle_renderer,
		    "toggled",
		    G_CALLBACK (active_toggled_cb),
		    dlg);

  g_object_set (toggle_renderer,
		"mode",
		GTK_CELL_RENDERER_MODE_ACTIVATABLE,
		NULL);

  name_column = gtk_tree_view_column_new_with_attributes ("Profile",
						     text_renderer,
						     "text",
						     PROFILE_NAME_COL,
						     NULL);
  
  toggle_column = gtk_tree_view_column_new_with_attributes ("Active",
							    toggle_renderer,
							    "active",
							    TOGGLE_COL,
							    NULL);

  gtk_tree_view_column_set_resizable (toggle_column, TRUE);
  gtk_tree_view_column_set_resizable (name_column, TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (dlg->priv->profile_treeview), name_column);
  gtk_tree_view_append_column (GTK_TREE_VIEW (dlg->priv->profile_treeview), toggle_column);

  g_object_set (name_column, 
		"expand", 
		TRUE,
		NULL);

  gtranslator_preferences_fill_profile_treeview (dlg,GTK_TREE_MODEL(model));
}

/***************Interface pages****************/
static void
style_changed_cb (GtkComboBox *combobox,
		  GtranslatorPreferencesDialog *dlg)
{
	g_return_if_fail(combobox == GTK_COMBO_BOX(dlg->priv->gdl_combobox));
	
	gtranslator_prefs_manager_set_gdl_style (gtk_combo_box_get_active (combobox));
}

static void
setup_interface_pages(GtranslatorPreferencesDialog *dlg)
{
	gint gdl_style;
	
	/*Set initial value*/
	gdl_style = gtranslator_prefs_manager_get_gdl_style ();
	if (gdl_style)
		gtk_combo_box_set_active (GTK_COMBO_BOX (dlg->priv->gdl_combobox),
					  gdl_style);
		
	/*Connect signals*/
	g_signal_connect(dlg->priv->gdl_combobox, "changed",
			 G_CALLBACK (style_changed_cb),
			 dlg);
}

/***************Translation Memory pages****************/
static void
response_filechooser_cb (GtkDialog *dialog,
			 gint       response_id,
			 gpointer   user_data)  
{
  GtranslatorPreferencesDialog *dlg;

  dlg = (GtranslatorPreferencesDialog *)user_data;
  

  if (response_id == GTK_RESPONSE_YES) {
    gtk_entry_set_text (GTK_ENTRY (dlg->priv->directory_entry),
			gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog)));
    gtranslator_prefs_manager_set_tm_dir (gtk_file_chooser_get_current_folder_uri (GTK_FILE_CHOOSER (dialog)));
    gtk_widget_destroy (GTK_WIDGET (dialog));
  } else {
    gtk_widget_destroy (GTK_WIDGET (dialog));
  }
}

static void
on_search_button_pulsed (GtkButton *button,
			 gpointer data)
{
  GtkWidget *filechooser;
  GtranslatorPreferencesDialog *dlg;

  dlg = (GtranslatorPreferencesDialog *)data;
  
  filechooser = gtk_file_chooser_dialog_new ("Select PO directory",
					     NULL,
					     GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
					     GTK_STOCK_CANCEL,
					     GTK_RESPONSE_CANCEL,
					     GTK_STOCK_OK,
					     GTK_RESPONSE_YES,
					     NULL);
 
  g_signal_connect (GTK_DIALOG (filechooser), "response",
		    G_CALLBACK (response_filechooser_cb),
		    dlg);

  gtk_dialog_run (GTK_DIALOG (filechooser));
}

static void
gtranslator_project_utils_scan_dir (GFile *dir,
				    GList **list,
				    const gchar *po_name)
{
	GFileInfo *info;
	GError *error;
	GFile *file;
	GFileEnumerator *enumerator;

	error = NULL;
	enumerator = g_file_enumerate_children (dir,
						G_FILE_ATTRIBUTE_STANDARD_NAME,
						G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
						NULL,
						&error);
	if (enumerator) 
	{
		error = NULL;
		
		while ((info = g_file_enumerator_next_file (enumerator, NULL, &error)) != NULL) 
		{
			const gchar *name;
			gchar *filename;
			
			name = g_file_info_get_name (info);
			file = g_file_get_child (dir, name);

			if (po_name != NULL)
			{
				if (g_str_has_suffix (po_name, ".po"))
					filename = g_strdup (po_name);
				else 
					filename = g_strconcat (po_name, ".po", NULL);
			}
			else
				filename = g_strdup (".po");
			
			if (g_str_has_suffix (name, filename))
				*list = g_list_prepend (*list, g_file_get_path (file));
			g_free (filename);

			gtranslator_project_utils_scan_dir (file, list, po_name);
			g_object_unref (file);
			g_object_unref (info);
		}
		g_file_enumerator_close (enumerator, NULL, NULL);
		g_object_unref (enumerator);
		
		if (error)
		{
			g_warning (error->message);
		}
	}
}


static void
on_add_database_button_pulsed (GtkButton *button,
			       gpointer data)
{
  GList *files_list = NULL;
  GList *l = NULL;
  GFile *dir;
  const gchar *dir_name;
  GtranslatorPo *po;
  GtranslatorTranslationMemory *tm;
  GtkWidget *dialog;
  
  tm = (GtranslatorTranslationMemory *)gtranslator_application_get_translation_memory (GTR_APP);

  dir_name = gtranslator_prefs_manager_get_tm_dir ();

  dir = g_file_new_for_uri (dir_name);

  gtranslator_project_utils_scan_dir (dir, &files_list, NULL);

  for (l = files_list; l; l = l->next) {
    GList *msg_list = NULL;
    GList *l2 = NULL;
    gchar *file_uri;

    po = gtranslator_po_new ();
    file_uri = (gchar*)l->data;
    
    gtranslator_po_parse (po, file_uri, NULL);
    msg_list = gtranslator_po_get_messages (po);
       
    for (l2 = msg_list; l2; l2 = l2->next) {
      GtranslatorMsg *msg;
      msg = (GtranslatorMsg *)l2->data;
      if (gtranslator_msg_is_translated (msg))
	gtranslator_translation_memory_store (tm,
					      gtranslator_msg_get_msgid (msg),
					      gtranslator_msg_get_msgstr (msg));
    }
    //g_list_foreach (msg_list, (GFunc)g_object_unref, NULL);
    //g_list_free (msg_list);
    g_free (file_uri);
    g_object_unref (po);
  }
  //g_list_foreach (files_list, (GFunc)g_object_unref, NULL);
  //g_list_free (files_list); 

  dialog = gtk_message_dialog_new (NULL,
				   GTK_DIALOG_MODAL,
				   GTK_MESSAGE_INFO,
				   GTK_BUTTONS_CLOSE,
				   NULL);

  gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG (dialog),
				 _("<span weight=\"bold\" size=\"large\">Strings added to database</span>"));
  
  g_signal_connect_swapped (dialog,
			    "response",
			    G_CALLBACK (gtk_widget_destroy),
			    dialog);
  gtk_dialog_run (GTK_DIALOG (dialog));
}

static void
on_show_tm_options_checkbutton_changed (GtkToggleButton *button,
					gpointer user_data)
{ 
  gtranslator_prefs_manager_set_show_tm_options (gtk_toggle_button_get_active(button));
}

static void
on_missing_words_spinbutton_changed (GtkSpinButton *spinbutton,
				      gpointer data)
{
  gint value;

  value = gtk_spin_button_get_value_as_int (spinbutton);

  gtranslator_prefs_manager_set_missing_words (value);
}

static void
on_sentence_length_spinbutton_changed (GtkSpinButton *spinbutton,
				      gpointer data)
{
  gint value;

  value = gtk_spin_button_get_value_as_int (spinbutton);

  gtranslator_prefs_manager_set_sentence_length (value);
}

static void
directory_entry_changed(GObject    *gobject,
			GParamSpec *arg1,
			GtranslatorPreferencesDialog *dlg)
{
	const gchar *text;
	
	g_return_if_fail(GTK_ENTRY(gobject) == GTK_ENTRY(dlg->priv->directory_entry));

	text = gtk_entry_get_text(GTK_ENTRY(gobject));
	
	if(text)
	  gtranslator_prefs_manager_set_tm_dir (text);
}


static void
setup_tm_pages(GtranslatorPreferencesDialog *dlg)
{	
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->priv->show_tm_options_checkbutton),
				gtranslator_prefs_manager_get_show_tm_options ());
  
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (dlg->priv->missing_words_spinbutton),
			     (gdouble) gtranslator_prefs_manager_get_missing_words ());
  
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (dlg->priv->sentence_length_spinbutton),
			     (gdouble) gtranslator_prefs_manager_get_sentence_length ());
  
  g_signal_connect (GTK_BUTTON (dlg->priv->search_button), "clicked",
		    G_CALLBACK (on_search_button_pulsed),
		    dlg);

  g_signal_connect(dlg->priv->directory_entry, "notify::text",
			 G_CALLBACK(directory_entry_changed),
			 dlg);
  
  g_signal_connect (GTK_BUTTON (dlg->priv->add_database_button), "clicked",
		    G_CALLBACK (on_add_database_button_pulsed),
		    dlg);

  g_signal_connect (GTK_TOGGLE_BUTTON (dlg->priv->show_tm_options_checkbutton), "toggled",
		    G_CALLBACK (on_show_tm_options_checkbutton_changed),
		    dlg);

  g_signal_connect (GTK_SPIN_BUTTON (dlg->priv->missing_words_spinbutton), "value-changed",
		    G_CALLBACK (on_missing_words_spinbutton_changed),
		    dlg);
  
  g_signal_connect (GTK_SPIN_BUTTON (dlg->priv->sentence_length_spinbutton), "value-changed",
		    G_CALLBACK (on_sentence_length_spinbutton_changed),
		    dlg);
}

/***************Plugins pages****************/
static void
setup_plugin_pages(GtranslatorPreferencesDialog *dlg)
{
	GtkWidget *page_content;

	page_content = gtranslator_plugin_manager_new ();
	g_return_if_fail (page_content != NULL);

	gtk_box_pack_start (GTK_BOX (dlg->priv->plugins_box),
			    page_content,
			    TRUE,
			    TRUE,
			    0);

	gtk_widget_show_all (page_content);
}


static void
dialog_response_handler (GtkDialog *dlg, 
			 gint       res_id)
{
	switch (res_id)
	{
		default:
			gtk_widget_destroy (GTK_WIDGET(dlg));
	}
}

static void
add_button_pulsed (GtkWidget *button,
		   GtranslatorPreferencesDialog *dlg)
{
  GtranslatorProfile *profile;
  profile = gtranslator_profile_new ();
  gtranslator_show_profile_dialog(dlg, profile, NEW_PROFILE);
}

static void
edit_button_pulsed (GtkWidget *button,
		    GtranslatorPreferencesDialog *dlg)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkTreeSelection *selection;
  gchar *profile_row, *old_profile_name;
  GtranslatorProfile *edited_profile;
  GtranslatorProfile *active_profile;
  GList *profiles_list = NULL, *l = NULL;
  
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (dlg->priv->profile_treeview));
  g_return_if_fail (model != NULL);
  
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dlg->priv->profile_treeview));
  
  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
    
    gtk_tree_model_get (model, &iter, PROFILE_NAME_COL, &profile_row, -1);
    
    profiles_list = gtranslator_application_get_profiles (GTR_APP);
    active_profile = gtranslator_application_get_active_profile (GTR_APP);
    
    for (l = profiles_list; l; l = l->next) {
      GtranslatorProfile *profile;
      profile = (GtranslatorProfile *)l->data;
      if (!strcmp (gtranslator_profile_get_name (profile), profile_row)) {
	old_profile_name = gtranslator_profile_get_name (profile);
	edited_profile = profile;
      }
    }
    gtranslator_show_profile_dialog (dlg, edited_profile, EDIT_PROFILE);
  }
}

static void
delete_confirm_dialog_cb (GtkWidget *dialog,
			  gint response_id,
			  GtranslatorPreferencesDialog *dlg)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkTreeSelection *selection;
  gchar *profile_row;
  GList *profiles_list = NULL, *l = NULL;
  GList *new_list = NULL;

  if (response_id == GTK_RESPONSE_YES) {
    model = gtk_tree_view_get_model (GTK_TREE_VIEW (dlg->priv->profile_treeview));
    g_return_if_fail (model != NULL);
    
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dlg->priv->profile_treeview));
    
    if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
      
      gtk_tree_model_get (model, &iter, PROFILE_NAME_COL, &profile_row, -1);
      
      profiles_list = gtranslator_application_get_profiles (GTR_APP);
      
      for (l = profiles_list; l; l = l->next) {
	GtranslatorProfile *profile;
	profile = (GtranslatorProfile *)l->data;
	if (!strcmp (gtranslator_profile_get_name (profile), profile_row)) {
	  new_list = g_list_remove (profiles_list, profile);
	  gtranslator_application_set_profiles (GTR_APP, new_list);
	}
      }
      gtranslator_preferences_fill_profile_treeview (dlg, model);
    }
    gtk_widget_destroy (dialog);
  } else {
    gtk_widget_destroy (dialog);
  }
}

static void
delete_button_pulsed (GtkWidget *button,
		      GtranslatorPreferencesDialog *dlg)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkTreeSelection *selection;
  gchar *profile_row;
  GtranslatorProfile *active_profile;
  GtkWidget *dialog;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (dlg->priv->profile_treeview));
  g_return_if_fail (model != NULL);
    
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dlg->priv->profile_treeview));
  
  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
    
    gtk_tree_model_get (model, &iter, PROFILE_NAME_COL, &profile_row, -1);
    
    active_profile = gtranslator_application_get_active_profile (GTR_APP);

    if (!strcmp (gtranslator_profile_get_name (active_profile), profile_row)) {
    
      dialog = gtk_message_dialog_new (GTK_WINDOW (dlg),
					     GTK_DIALOG_MODAL,
					     GTK_MESSAGE_ERROR,
					     GTK_BUTTONS_CLOSE,
					     NULL);

      gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG (dialog),
				 _("<span weight=\"bold\" size=\"large\">Impossible to remove the active profile</span>"));
      
      gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
						_("Another profile should be selected as active before"));
      
      gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);
    } else {
      dialog = gtk_message_dialog_new (GTK_WINDOW (dlg),
				       GTK_DIALOG_MODAL,
				       GTK_MESSAGE_QUESTION,
				       GTK_BUTTONS_NONE,
				       NULL);
      
      gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG (dialog),
				     _("<span weight=\"bold\" size=\"large\">Are you sure you want to delete this profile?</span>"));
      
      gtk_dialog_add_button (GTK_DIALOG (dialog),
			     GTK_STOCK_CANCEL,
			     GTK_RESPONSE_CANCEL);
      
      gtk_dialog_add_button (GTK_DIALOG (dialog),
			     GTK_STOCK_DELETE,
			     GTK_RESPONSE_YES);
      
      gtk_dialog_run (GTK_DIALOG (dialog));
      
      g_signal_connect (GTK_DIALOG (dialog), "response",
			G_CALLBACK (delete_confirm_dialog_cb),
			dlg);
    }
  } 
}

static void
gtranslator_preferences_dialog_init (GtranslatorPreferencesDialog *dlg)
{
	gboolean ret;
	GtkWidget *error_widget;
	
	dlg->priv = GTR_PREFERENCES_DIALOG_GET_PRIVATE (dlg);
	
	gtk_dialog_add_buttons (GTK_DIALOG (dlg),
				GTK_STOCK_CLOSE,
				GTK_RESPONSE_CLOSE,
				GTK_STOCK_HELP,
				GTK_RESPONSE_HELP,
				NULL);
	
	gtk_window_set_title (GTK_WINDOW (dlg), _("Gtranslator Preferences"));
	gtk_window_set_resizable (GTK_WINDOW (dlg), FALSE);
	gtk_dialog_set_has_separator (GTK_DIALOG (dlg), FALSE);
	gtk_window_set_destroy_with_parent (GTK_WINDOW (dlg), TRUE);
	
	/* HIG defaults */
	gtk_container_set_border_width (GTK_CONTAINER (dlg), 5);
	gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dlg)->vbox), 2); /* 2 * 5 + 2 = 12 */
	gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (dlg)->action_area), 5);
	gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dlg)->action_area), 4);
	
	g_signal_connect (dlg,
			  "response",
			  G_CALLBACK (dialog_response_handler),
			  NULL);
	
	/*Glade*/
	
	ret = gtranslator_utils_get_glade_widgets(DATADIR "/preferences-dialog.glade",
		"notebook",
		&error_widget,
		
		"notebook", &dlg->priv->notebook,

		"warn_if_fuzzy_checkbutton", &dlg->priv->warn_if_fuzzy_checkbutton,
		"delete_compiled_checkbutton", &dlg->priv->delete_compiled_checkbutton,

		"autosave_checkbutton", &dlg->priv->autosave_checkbutton,
		"autosave_interval_spinbutton", &dlg->priv->autosave_interval_spinbutton,
		"autosave_hbox", &dlg->priv->autosave_hbox,
		"append_suffix_checkbutton", &dlg->priv->append_suffix_checkbutton,
		"autosave_suffix_entry", &dlg->priv->autosave_suffix_entry,
		"autosave_suffix_hbox", &dlg->priv->autosave_suffix_hbox,

		"highlight_checkbutton", &dlg->priv->highlight_checkbutton,
		"visible_whitespace_checkbutton", &dlg->priv->visible_whitespace_checkbutton,
		"use_custom_font_checkbutton", &dlg->priv->use_custom_font_checkbutton,
		"editor_font_fontbutton", &dlg->priv->editor_font_fontbutton,
		"editor_font_hbox", &dlg->priv->editor_font_hbox,

		"unmark_fuzzy_checkbutton", &dlg->priv->unmark_fuzzy_checkbutton,
		"keep_obsolete_checkbutton", &dlg->priv->keep_obsolete_checkbutton,
		"spellcheck_checkbutton", &dlg->priv->spellcheck_checkbutton,

		"profile_treeview", &dlg->priv->profile_treeview,
		"add_button", &dlg->priv->add_button,
		"edit_button", &dlg->priv->edit_button,
		"delete_button", &dlg->priv->delete_button,

		"directory_entry", &dlg->priv->directory_entry,
 		"search_button", &dlg->priv->search_button,
 		"add_database_button", &dlg->priv->add_database_button,
 						  
 		"show_tm_options_checkbutton", &dlg->priv->show_tm_options_checkbutton,				  
 		"missing_words_spinbutton", &dlg->priv->missing_words_spinbutton,
 		"sentence_length_spinbutton", &dlg->priv->sentence_length_spinbutton,
 		
		"gdl_combobox", &dlg->priv->gdl_combobox,
		
		"plugins_box", &dlg->priv->plugins_box,
		NULL);
	
	if(!ret)
	{
		gtk_widget_show(error_widget);
		gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (dlg)->vbox),
					     error_widget);
		
		return;
	}
	
	
	
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox),
			    dlg->priv->notebook, FALSE, FALSE, 0);
	
	gtk_container_set_border_width (GTK_CONTAINER (dlg->priv->notebook), 5);
	
	g_signal_connect (dlg->priv->add_button,
			  "clicked",
			  G_CALLBACK (add_button_pulsed),
			  dlg);

	g_signal_connect (dlg->priv->delete_button,
			  "clicked",
			  G_CALLBACK (delete_button_pulsed),
			  dlg);

	g_signal_connect (dlg->priv->edit_button,
			  "clicked",
			  G_CALLBACK (edit_button_pulsed),
			  dlg);

	setup_files_pages(dlg);
	setup_editor_pages(dlg);
	setup_profile_pages(dlg);
	setup_interface_pages(dlg);
	setup_tm_pages (dlg);
	setup_plugin_pages(dlg);
}

static void
gtranslator_preferences_dialog_finalize (GObject *object)
{
	G_OBJECT_CLASS (gtranslator_preferences_dialog_parent_class)->finalize (object);
}

static void
gtranslator_preferences_dialog_class_init (GtranslatorPreferencesDialogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GtranslatorPreferencesDialogPrivate));

	object_class->finalize = gtranslator_preferences_dialog_finalize;
}

void
gtranslator_show_preferences_dialog (GtranslatorWindow *window)
{
	static GtkWidget *dlg = NULL;
	
	g_return_if_fail(GTR_IS_WINDOW(window));
	
	if(dlg == NULL)
	{
		dlg = GTK_WIDGET (g_object_new (GTR_TYPE_PREFERENCES_DIALOG, NULL));
		g_signal_connect (dlg,
				  "destroy",
				  G_CALLBACK (gtk_widget_destroyed),
				  &dlg);
		gtk_widget_show_all(dlg);
	}
	
	if (GTK_WINDOW (window) != gtk_window_get_transient_for (GTK_WINDOW (dlg)))
	{
		gtk_window_set_transient_for (GTK_WINDOW (dlg),
					      GTK_WINDOW (window));
	}

	gtranslator_application_set_preferences_dialog (GTR_APP, GTR_PREFERENCES_DIALOG(dlg));

	gtk_window_present (GTK_WINDOW (dlg));
}