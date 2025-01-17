/*
 * gedit-search-dialog.c
 * This file is part of gtranslator based on gedit
 *
 * Copyright (C) 2005  Paolo Maggi
 *               2007  Ignacio Casal
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

/*
 * Modified by the gedit Team, 2005. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 *
 * $Id: gedit-search-dialog.c 5931 2007-09-25 20:05:40Z pborelli $
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>

#include "gtr-dirs.h"
#include "gtr-search-dialog.h"
#include "gtr-history-entry.h"
#include "gtr-utils.h"
#include "gtr-marshal.h"

/* Signals */
enum
{
  SHOW_REPLACE,
  LAST_SIGNAL
};

static guint dialog_signals[LAST_SIGNAL] = { 0 };

typedef struct
{
  gboolean show_replace;

  GtkWidget *grid;
  GtkWidget *search_label;
  GtkWidget *search_entry;
  GtkWidget *search_text_entry;
  GtkWidget *replace_label;
  GtkWidget *replace_entry;
  GtkWidget *replace_text_entry;
  GtkWidget *original_text_checkbutton;
  GtkWidget *translated_text_checkbutton;
  GtkWidget *match_case_checkbutton;
  GtkWidget *entire_word_checkbutton;
  GtkWidget *backwards_checkbutton;
  GtkWidget *wrap_around_checkbutton;
  GtkWidget *find_button;
  GtkWidget *replace_button;
  GtkWidget *replace_all_button;

  gboolean glade_error;
} GtrSearchDialogPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GtrSearchDialog, gtr_search_dialog, GTK_TYPE_DIALOG)

enum
{
  PROP_0,
  PROP_SHOW_REPLACE
};

static void
gtr_search_dialog_set_property (GObject * object,
                                guint prop_id,
                                const GValue * value,
                                GParamSpec * pspec)
{
  GtrSearchDialog *dlg = GTR_SEARCH_DIALOG (object);

  switch (prop_id)
    {
    case PROP_SHOW_REPLACE:
      gtr_search_dialog_set_show_replace (dlg, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gtr_search_dialog_get_property (GObject * object,
                                guint prop_id,
                                GValue * value, GParamSpec * pspec)
{
  GtrSearchDialog *dlg = GTR_SEARCH_DIALOG (object);
  GtrSearchDialogPrivate *priv = gtr_search_dialog_get_instance_private (dlg);

  switch (prop_id)
    {
    case PROP_SHOW_REPLACE:
      g_value_set_boolean (value, priv->show_replace);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static gboolean
show_replace (GtrSearchDialog * dlg)
{
  gtr_search_dialog_set_show_replace (dlg, TRUE);

  return TRUE;
}

static void
gtr_search_dialog_class_init (GtrSearchDialogClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkBindingSet *binding_set;

  object_class->set_property = gtr_search_dialog_set_property;
  object_class->get_property = gtr_search_dialog_get_property;

  klass->show_replace = show_replace;

  dialog_signals[SHOW_REPLACE] =
    g_signal_new ("show_replace",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (GtrSearchDialogClass,
                                   show_replace), NULL, NULL,
                  gtr_marshal_BOOLEAN__VOID, G_TYPE_BOOLEAN, 0);

  g_object_class_install_property (object_class, PROP_SHOW_REPLACE,
                                   g_param_spec_boolean ("show-replace",
                                                         "Show Replace",
                                                         "Whether the dialog is used for Search&Replace",
                                                         FALSE,
                                                         G_PARAM_READWRITE));

  binding_set = gtk_binding_set_by_class (klass);

  /* Note: we cannot use the keyval/modifier associated with the 
   * GTK_STOCK_FIND_AND_REPLACE stock item since GNOME HIG suggests Ctrl+h
   * for Replace while gtk+ uses Ctrl+r */
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_h, GDK_CONTROL_MASK,
                                "show_replace", 0);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_H, GDK_CONTROL_MASK,
                                "show_replace", 0);
}

static void
insert_text_handler (GtkEditable * editable,
                     const gchar * text,
                     gint length, gint * position, gpointer data)
{
  static gboolean insert_text = FALSE;
  gchar *escaped_text;
  gint new_len;

  /* To avoid recursive behavior */
  if (insert_text)
    return;

  escaped_text = gtr_utils_escape_search_text (text);

  new_len = strlen (escaped_text);

  if (new_len == length)
    {
      g_free (escaped_text);
      return;
    }

  insert_text = TRUE;

  g_signal_stop_emission_by_name (editable, "insert_text");

  gtk_editable_insert_text (editable, escaped_text, new_len, position);

  insert_text = FALSE;

  g_free (escaped_text);
}

static void
search_entry_changed (GtkComboBox * combo, GtrSearchDialog * dialog)
{
  GtkWidget *entry;
  const gchar *search_string;

  entry = gtk_bin_get_child (GTK_BIN (combo));
  search_string = gtk_entry_get_text (GTK_ENTRY (entry));
  g_return_if_fail (search_string != NULL);

  if (*search_string != '\0')
    {
      gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                         GTR_SEARCH_DIALOG_FIND_RESPONSE,
                                         TRUE);
      gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                         GTR_SEARCH_DIALOG_REPLACE_ALL_RESPONSE,
                                         TRUE);
    }
  else
    {
      gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                         GTR_SEARCH_DIALOG_FIND_RESPONSE,
                                         FALSE);
      gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                         GTR_SEARCH_DIALOG_REPLACE_RESPONSE,
                                         FALSE);
      gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                         GTR_SEARCH_DIALOG_REPLACE_ALL_RESPONSE,
                                         FALSE);
    }
}

/*
 * Functions to make sure either translated or original checkbutton
 * is selected at any time. Anything else does not make any sense.
 */
static void
translated_checkbutton_toggled (GtkToggleButton * button,
                                         GtrSearchDialog * dialog)
{
  gboolean original_text;
  gboolean translated_text;
  GtrSearchDialogPrivate *priv = gtr_search_dialog_get_instance_private (dialog);


  original_text =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
                                  (priv->original_text_checkbutton));
  translated_text =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
                                  (
                                   priv->translated_text_checkbutton));

  if (!original_text && !translated_text)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
                                    (priv->original_text_checkbutton),
                                     TRUE);
    }
}

static void
original_checkbutton_toggled (GtkToggleButton * button,
                                         GtrSearchDialog * dialog)
{
  gboolean original_text;
  gboolean translated_text;
  GtrSearchDialogPrivate *priv = gtr_search_dialog_get_instance_private (dialog);

  original_text =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
                                  (priv->original_text_checkbutton));
  translated_text =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
                                  (
                                   priv->translated_text_checkbutton));

  if (!original_text && !translated_text)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
                                    (priv->translated_text_checkbutton),
                                     TRUE);
    }
}

static void
response_handler (GtrSearchDialog * dialog, gint response_id, gpointer data)
{
  const gchar *str;
  GtrSearchDialogPrivate *priv = gtr_search_dialog_get_instance_private (dialog);

  switch (response_id)
    {
    case GTR_SEARCH_DIALOG_REPLACE_RESPONSE:
    case GTR_SEARCH_DIALOG_REPLACE_ALL_RESPONSE:
      str = gtk_entry_get_text (GTK_ENTRY (priv->replace_text_entry));
      if (*str != '\0')
        {
          gchar *text;

          text = gtr_utils_unescape_search_text (str);
          gtr_history_entry_prepend_text
            (GTR_HISTORY_ENTRY (priv->replace_entry), text);

          g_free (text);
        }
      /* fall through, so that we also save the find entry */
    case GTR_SEARCH_DIALOG_FIND_RESPONSE:
      str = gtk_entry_get_text (GTK_ENTRY (priv->search_text_entry));
      if (*str != '\0')
        {
          gchar *text;

          text = gtr_utils_unescape_search_text (str);
          gtr_history_entry_prepend_text
            (GTR_HISTORY_ENTRY (priv->search_entry), text);

          g_free (text);
        }
    }
}

static void
show_replace_widgets (GtrSearchDialog * dlg, gboolean show_replace)
{
  GtrSearchDialogPrivate *priv = gtr_search_dialog_get_instance_private (dlg);
  if (show_replace)
    {
      gtk_widget_hide (priv->original_text_checkbutton);
      gtk_widget_hide (priv->translated_text_checkbutton);

      /*
       * Set the default value of checkbutton
       */
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
                                    (priv->translated_text_checkbutton),
                                    TRUE);

      gtk_widget_show (priv->replace_label);
      gtk_widget_show (priv->replace_entry);
      gtk_widget_show (priv->replace_all_button);
      gtk_widget_show (priv->replace_button);

      gtk_window_set_title (GTK_WINDOW (dlg), _("Replace"));
    }
  else
    {
      gtk_widget_show (priv->original_text_checkbutton);
      gtk_widget_show (priv->translated_text_checkbutton);

      /*
       * Set the default value of checkbuttons
       */
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
                                    (priv->original_text_checkbutton),
                                    TRUE);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
                                    (priv->translated_text_checkbutton),
                                    TRUE);

      gtk_widget_hide (priv->replace_label);
      gtk_widget_hide (priv->replace_entry);
      gtk_widget_hide (priv->replace_all_button);
      gtk_widget_hide (priv->replace_button);

      gtk_window_set_title (GTK_WINDOW (dlg), _("Find"));
    }

  gtk_widget_show (priv->find_button);
}

static void
gtr_search_dialog_init (GtrSearchDialog * dlg)
{
  GtkWidget *content;
  GtkBuilder *builder;
  GtkBox *content_area;
  gchar *root_objects[] = {
    "search_dialog_content",
    NULL
  };
  GtrSearchDialogPrivate *priv = gtr_search_dialog_get_instance_private (dlg);

  gtk_window_set_resizable (GTK_WINDOW (dlg), FALSE);
  gtk_window_set_destroy_with_parent (GTK_WINDOW (dlg), TRUE);

  gtk_dialog_add_buttons (GTK_DIALOG (dlg),
                          _("_Close"), GTK_RESPONSE_CANCEL, NULL);

  content_area = GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dlg)));

  /* HIG defaults */
  gtk_container_set_border_width (GTK_CONTAINER (dlg), 5);
  gtk_box_set_spacing (content_area, 2);    /* 2 * 5 + 2 = 12 */

  builder = gtk_builder_new ();
  gtk_builder_add_objects_from_resource (builder, "/org/gnome/translator/gtr-search-dialog.ui",
                                         root_objects, NULL);
  content = GTK_WIDGET (gtk_builder_get_object (builder, "search_dialog_content"));
  g_object_ref (content);
  priv->grid = GTK_WIDGET (gtk_builder_get_object (builder, "grid"));
  priv->search_label = GTK_WIDGET (gtk_builder_get_object (builder, "search_label"));
  priv->replace_label = GTK_WIDGET (gtk_builder_get_object (builder, "replace_with_label"));
  priv->original_text_checkbutton = GTK_WIDGET (gtk_builder_get_object (builder, "original_text_checkbutton"));
  priv->translated_text_checkbutton = GTK_WIDGET (gtk_builder_get_object (builder, "translated_text_checkbutton"));
  priv->match_case_checkbutton = GTK_WIDGET (gtk_builder_get_object (builder, "match_case_checkbutton"));
  priv->entire_word_checkbutton = GTK_WIDGET (gtk_builder_get_object (builder, "entire_word_checkbutton"));
  priv->backwards_checkbutton = GTK_WIDGET (gtk_builder_get_object (builder, "search_backwards_checkbutton"));
  priv->wrap_around_checkbutton = GTK_WIDGET (gtk_builder_get_object (builder, "wrap_around_checkbutton"));
  g_object_unref (builder);

  priv->search_entry = gtr_history_entry_new ("search-for-entry", TRUE);
  gtk_widget_set_size_request (priv->search_entry, 300, -1);
  gtr_history_entry_set_escape_func (GTR_HISTORY_ENTRY (priv->search_entry),
                                     (GtrHistoryEntryEscapeFunc) gtr_utils_escape_search_text);
  gtk_widget_set_hexpand (GTK_WIDGET (priv->search_entry), TRUE);

  priv->search_text_entry = gtr_history_entry_get_entry (GTR_HISTORY_ENTRY (priv->search_entry));
  gtk_entry_set_activates_default (GTK_ENTRY (priv->search_text_entry), TRUE);
  gtk_widget_show (priv->search_entry);
  gtk_grid_attach_next_to (GTK_GRID (priv->grid),
                           priv->search_entry,
                           priv->search_label,
                           GTK_POS_RIGHT, 1, 1);

  priv->replace_entry = gtr_history_entry_new ("replace-with-entry", TRUE);
  gtr_history_entry_set_escape_func (GTR_HISTORY_ENTRY
                                     (priv->replace_entry),
                                     (GtrHistoryEntryEscapeFunc)
                                     gtr_utils_escape_search_text);
  gtk_widget_set_hexpand (GTK_WIDGET (priv->replace_entry), TRUE);

  priv->replace_text_entry = gtr_history_entry_get_entry (GTR_HISTORY_ENTRY (priv->replace_entry));
  gtk_entry_set_activates_default (GTK_ENTRY (priv->replace_text_entry), TRUE);
  gtk_widget_show (priv->replace_entry);
  gtk_grid_attach_next_to (GTK_GRID (priv->grid),
                           priv->replace_entry,
                           priv->replace_label,
                           GTK_POS_RIGHT, 1, 1);

  gtk_label_set_mnemonic_widget (GTK_LABEL (priv->search_label),
                                 priv->search_entry);
  gtk_label_set_mnemonic_widget (GTK_LABEL (priv->replace_label),
                                 priv->replace_entry);

  priv->find_button = gtk_button_new_with_label (_("Find"));
  priv->replace_all_button =
    gtk_button_new_with_mnemonic (_("Replace _All"));
  priv->replace_button =
    gtk_button_new_with_label (_("Find and replace"));
  gtk_button_set_label (GTK_BUTTON (priv->replace_button),
                        _("_Replace"));

  gtk_dialog_add_action_widget (GTK_DIALOG (dlg),
                                priv->replace_all_button,
                                GTR_SEARCH_DIALOG_REPLACE_ALL_RESPONSE);
  gtk_dialog_add_action_widget (GTK_DIALOG (dlg),
                                priv->replace_button,
                                GTR_SEARCH_DIALOG_REPLACE_RESPONSE);
  gtk_dialog_add_action_widget (GTK_DIALOG (dlg),
                                priv->find_button,
                                GTR_SEARCH_DIALOG_FIND_RESPONSE);
  g_object_set (G_OBJECT (priv->find_button), "can-default", TRUE, NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (dlg),
                                   GTR_SEARCH_DIALOG_FIND_RESPONSE);

  /* insensitive by default */
  gtk_dialog_set_response_sensitive (GTK_DIALOG (dlg),
                                     GTR_SEARCH_DIALOG_FIND_RESPONSE, FALSE);
  gtk_dialog_set_response_sensitive (GTK_DIALOG (dlg),
                                     GTR_SEARCH_DIALOG_REPLACE_RESPONSE,
                                     FALSE);
  gtk_dialog_set_response_sensitive (GTK_DIALOG (dlg),
                                     GTR_SEARCH_DIALOG_REPLACE_ALL_RESPONSE,
                                     FALSE);

  gtk_box_pack_start (content_area, content, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (content), 5);

  g_signal_connect (priv->search_text_entry,
                    "insert_text", G_CALLBACK (insert_text_handler), NULL);
  g_signal_connect (priv->replace_text_entry,
                    "insert_text", G_CALLBACK (insert_text_handler), NULL);
  g_signal_connect (priv->search_entry,
                    "changed", G_CALLBACK (search_entry_changed), dlg);

  g_signal_connect (priv->original_text_checkbutton,
                    "toggled",
                    G_CALLBACK (original_checkbutton_toggled),
                    dlg);

  g_signal_connect (priv->translated_text_checkbutton,
                    "toggled",
                    G_CALLBACK (translated_checkbutton_toggled),
                    dlg);

  g_signal_connect (dlg, "response", G_CALLBACK (response_handler), NULL);
}

GtkWidget *
gtr_search_dialog_new (GtkWindow * parent, gboolean show_replace)
{
  GtrSearchDialog *dlg;

  dlg = g_object_new (GTR_TYPE_SEARCH_DIALOG,
                      "show-replace", show_replace, NULL);

  if (parent != NULL)
    gtk_window_set_transient_for (GTK_WINDOW (dlg), parent);

  return GTK_WIDGET (dlg);
}

void
gtr_search_dialog_present_with_time (GtrSearchDialog * dialog,
                                     guint32 timestamp)
{
  GtrSearchDialogPrivate *priv = gtr_search_dialog_get_instance_private (dialog);
  g_return_if_fail (GTR_IS_SEARCH_DIALOG (dialog));

  gtk_window_present_with_time (GTK_WINDOW (dialog), timestamp);

  gtk_widget_grab_focus (priv->search_text_entry);
}

gboolean
gtr_search_dialog_get_show_replace (GtrSearchDialog * dialog)
{
  GtrSearchDialogPrivate *priv = gtr_search_dialog_get_instance_private (dialog);
  g_return_val_if_fail (GTR_IS_SEARCH_DIALOG (dialog), FALSE);

  return priv->show_replace;
}

void
gtr_search_dialog_set_show_replace (GtrSearchDialog * dialog,
                                    gboolean show_replace)
{
  GtrSearchDialogPrivate *priv = gtr_search_dialog_get_instance_private (dialog);
  g_return_if_fail (GTR_IS_SEARCH_DIALOG (dialog));

  if (priv->glade_error)
    return;

  priv->show_replace = show_replace != FALSE;
  show_replace_widgets (dialog, priv->show_replace);

  g_object_notify (G_OBJECT (dialog), "show-replace");
}

void
gtr_search_dialog_set_search_text (GtrSearchDialog * dialog,
                                   const gchar * text)
{
  GtrSearchDialogPrivate *priv = gtr_search_dialog_get_instance_private (dialog);
  g_return_if_fail (GTR_IS_SEARCH_DIALOG (dialog));
  g_return_if_fail (text != NULL);

  gtk_entry_set_text (GTK_ENTRY (priv->search_text_entry), text);

  gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                     GTR_SEARCH_DIALOG_FIND_RESPONSE,
                                     (*text != '\0'));

  gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                     GTR_SEARCH_DIALOG_REPLACE_ALL_RESPONSE,
                                     (*text != '\0'));
}

/*
 * The text must be unescaped before searching.
 */
const gchar *
gtr_search_dialog_get_search_text (GtrSearchDialog * dialog)
{
  GtrSearchDialogPrivate *priv = gtr_search_dialog_get_instance_private (dialog);
  g_return_val_if_fail (GTR_IS_SEARCH_DIALOG (dialog), NULL);

  return gtk_entry_get_text (GTK_ENTRY (priv->search_text_entry));
}

void
gtr_search_dialog_set_replace_text (GtrSearchDialog * dialog,
                                    const gchar * text)
{
  GtrSearchDialogPrivate *priv = gtr_search_dialog_get_instance_private (dialog);
  g_return_if_fail (GTR_IS_SEARCH_DIALOG (dialog));
  g_return_if_fail (text != NULL);

  gtk_entry_set_text (GTK_ENTRY (priv->replace_text_entry), text);
}

const gchar *
gtr_search_dialog_get_replace_text (GtrSearchDialog * dialog)
{
  GtrSearchDialogPrivate *priv = gtr_search_dialog_get_instance_private (dialog);
  g_return_val_if_fail (GTR_IS_SEARCH_DIALOG (dialog), NULL);

  return gtk_entry_get_text (GTK_ENTRY (priv->replace_text_entry));
}

void
gtr_search_dialog_set_original_text (GtrSearchDialog * dialog,
                                     gboolean match_case)
{
  GtrSearchDialogPrivate *priv = gtr_search_dialog_get_instance_private (dialog);
  g_return_if_fail (GTR_IS_SEARCH_DIALOG (dialog));

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
                                (priv->original_text_checkbutton),
                                match_case);
}

gboolean
gtr_search_dialog_get_original_text (GtrSearchDialog * dialog)
{
  GtrSearchDialogPrivate *priv = gtr_search_dialog_get_instance_private (dialog);
  g_return_val_if_fail (GTR_IS_SEARCH_DIALOG (dialog), FALSE);

  return
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
                                  (priv->original_text_checkbutton));
}

void
gtr_search_dialog_set_translated_text (GtrSearchDialog *
                                       dialog, gboolean match_case)
{
  GtrSearchDialogPrivate *priv = gtr_search_dialog_get_instance_private (dialog);
  g_return_if_fail (GTR_IS_SEARCH_DIALOG (dialog));

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
                                (priv->translated_text_checkbutton),
                                match_case);
}

gboolean
gtr_search_dialog_get_translated_text (GtrSearchDialog * dialog)
{
  GtrSearchDialogPrivate *priv = gtr_search_dialog_get_instance_private (dialog);
  g_return_val_if_fail (GTR_IS_SEARCH_DIALOG (dialog), FALSE);

  return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->translated_text_checkbutton));
}

void
gtr_search_dialog_set_match_case (GtrSearchDialog * dialog,
                                  gboolean match_case)
{
  GtrSearchDialogPrivate *priv = gtr_search_dialog_get_instance_private (dialog);
  g_return_if_fail (GTR_IS_SEARCH_DIALOG (dialog));

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->match_case_checkbutton),
                                match_case);
}

gboolean
gtr_search_dialog_get_match_case (GtrSearchDialog * dialog)
{
  GtrSearchDialogPrivate *priv = gtr_search_dialog_get_instance_private (dialog);
  g_return_val_if_fail (GTR_IS_SEARCH_DIALOG (dialog), FALSE);

  return
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
                                  (priv->match_case_checkbutton));
}

void
gtr_search_dialog_set_entire_word (GtrSearchDialog * dialog,
                                   gboolean entire_word)
{
  GtrSearchDialogPrivate *priv = gtr_search_dialog_get_instance_private (dialog);
  g_return_if_fail (GTR_IS_SEARCH_DIALOG (dialog));

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
                                (priv->entire_word_checkbutton),
                                entire_word);
}

gboolean
gtr_search_dialog_get_entire_word (GtrSearchDialog * dialog)
{
  GtrSearchDialogPrivate *priv = gtr_search_dialog_get_instance_private (dialog);
  g_return_val_if_fail (GTR_IS_SEARCH_DIALOG (dialog), FALSE);

  return
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
                                  (priv->entire_word_checkbutton));
}

void
gtr_search_dialog_set_backwards (GtrSearchDialog * dialog, gboolean backwards)
{
  GtrSearchDialogPrivate *priv = gtr_search_dialog_get_instance_private (dialog);
  g_return_if_fail (GTR_IS_SEARCH_DIALOG (dialog));

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
                                (priv->backwards_checkbutton),
                                backwards);
}

gboolean
gtr_search_dialog_get_backwards (GtrSearchDialog * dialog)
{
  GtrSearchDialogPrivate *priv = gtr_search_dialog_get_instance_private (dialog);
  g_return_val_if_fail (GTR_IS_SEARCH_DIALOG (dialog), FALSE);

  return
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
                                  (priv->backwards_checkbutton));
}

void
gtr_search_dialog_set_wrap_around (GtrSearchDialog * dialog,
                                   gboolean wrap_around)
{
  GtrSearchDialogPrivate *priv = gtr_search_dialog_get_instance_private (dialog);
  g_return_if_fail (GTR_IS_SEARCH_DIALOG (dialog));

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
                                (priv->wrap_around_checkbutton),
                                wrap_around);
}

gboolean
gtr_search_dialog_get_wrap_around (GtrSearchDialog * dialog)
{
  GtrSearchDialogPrivate *priv = gtr_search_dialog_get_instance_private (dialog);
  g_return_val_if_fail (GTR_IS_SEARCH_DIALOG (dialog), FALSE);

  return
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
                                  (priv->wrap_around_checkbutton));
}
