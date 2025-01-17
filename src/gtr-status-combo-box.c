/*
 * gtr-status-combo-box.c
 * This file is part of gtr
 *
 * Copyright (C) 2008  Jesse van den Kieboom
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

#include "gtr-status-combo-box.h"

#include <gdk/gdkkeysyms.h>

#define COMBO_BOX_TEXT_DATA "GtrStatusComboBoxTextData"

typedef struct
{
	GtkWidget *frame;
	GtkWidget *button;
	GtkWidget *hbox;
	GtkWidget *label;
	GtkWidget *item;
	GtkWidget *arrow;
	
	GtkWidget *menu;
	GtkWidget *current_item;
} GtrStatusComboBoxPrivate;

struct _GtrStatusComboBoxClassPrivate
{
	GtkCssProvider *css;
};

/* Signals */
enum
{
	CHANGED,
	NUM_SIGNALS
};

/* Properties */
enum 
{
	PROP_0,
	
	PROP_LABEL
};

static guint signals[NUM_SIGNALS] = { 0 };

G_DEFINE_TYPE_WITH_CODE (GtrStatusComboBox, gtr_status_combo_box, GTK_TYPE_EVENT_BOX,
			 G_ADD_PRIVATE (GtrStatusComboBox)
                         g_type_add_class_private (g_define_type_id, sizeof (GtrStatusComboBoxClassPrivate)))

static void
gtr_status_combo_box_finalize (GObject *object)
{
	G_OBJECT_CLASS (gtr_status_combo_box_parent_class)->finalize (object);
}

static void
gtr_status_combo_box_get_property (GObject    *object,
			             guint       prop_id,
			             GValue     *value,
			             GParamSpec *pspec)
{
	GtrStatusComboBox *obj = GTR_STATUS_COMBO_BOX (object);

	switch (prop_id)
	{
		case PROP_LABEL:
			g_value_set_string (value, gtr_status_combo_box_get_label (obj));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gtr_status_combo_box_set_property (GObject      *object,
			             guint         prop_id,
			             const GValue *value,
			             GParamSpec   *pspec)
{
	GtrStatusComboBox *obj = GTR_STATUS_COMBO_BOX (object);

	switch (prop_id)
	{
		case PROP_LABEL:
			gtr_status_combo_box_set_label (obj, g_value_get_string (value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gtr_status_combo_box_changed (GtrStatusComboBox *combo,
				GtkMenuItem         *item)
{
	const gchar *text;
	GtrStatusComboBoxPrivate *priv = gtr_status_combo_box_get_instance_private (combo);
	
	text = g_object_get_data (G_OBJECT (item), COMBO_BOX_TEXT_DATA);

	if (text != NULL)
	{
		gtk_label_set_markup (GTK_LABEL (priv->item), text);
		priv->current_item = GTK_WIDGET (item);
	}
}

static void
gtr_status_combo_box_class_init (GtrStatusComboBoxClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	static const gchar style[] =
		"* {\n"
		  "padding: 0;\n"
		"}";
	
	object_class->finalize = gtr_status_combo_box_finalize;
	object_class->get_property = gtr_status_combo_box_get_property;
	object_class->set_property = gtr_status_combo_box_set_property;
	
	klass->changed = gtr_status_combo_box_changed;

	signals[CHANGED] =
	    g_signal_new ("changed",
			  G_OBJECT_CLASS_TYPE (object_class),
			  G_SIGNAL_RUN_LAST,
			  G_STRUCT_OFFSET (GtrStatusComboBoxClass,
					   changed), NULL, NULL,
			  g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1,
			  GTK_TYPE_MENU_ITEM);
			  
	g_object_class_install_property (object_class, PROP_LABEL,
					 g_param_spec_string ("label",
					 		      "LABEL",
					 		      "The label",
					 		      NULL,
					 		      G_PARAM_READWRITE));

	klass->priv = G_TYPE_CLASS_GET_PRIVATE (klass, GTR_TYPE_STATUS_COMBO_BOX, GtrStatusComboBoxClassPrivate);

	klass->priv->css = gtk_css_provider_new ();
	gtk_css_provider_load_from_data (klass->priv->css, style, -1, NULL);
}

static void
menu_deactivate (GtkMenu             *menu,
		 GtrStatusComboBox *combo)
{
	GtrStatusComboBoxPrivate *priv = gtr_status_combo_box_get_instance_private (combo);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->button), FALSE);
}

static void
show_menu (GtrStatusComboBox *combo,
	   guint                button,
	   guint32              time)
{
	GtkRequisition request;
	gint max_height;
	GtkAllocation allocation;
	GtrStatusComboBoxPrivate *priv = gtr_status_combo_box_get_instance_private (combo);

	gtk_widget_get_preferred_size (priv->menu,
	                               &request, NULL);

	/* do something relative to our own height here, maybe we can do better */
	gtk_widget_get_allocation (GTK_WIDGET (combo), &allocation);
	max_height = allocation.height * 20;
	
	if (request.height > max_height)
	{
		gtk_widget_set_size_request (priv->menu, -1, max_height);
		gtk_widget_set_size_request (gtk_widget_get_toplevel (priv->menu), -1, max_height);
	}
	
	GdkEvent *event = gtk_get_current_event ();
	gtk_menu_popup_at_pointer (GTK_MENU (priv->menu), event);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->button), TRUE);

	if (priv->current_item)
	{
		gtk_menu_shell_select_item (GTK_MENU_SHELL (priv->menu),
					    priv->current_item);
	}
}

static gboolean
button_press_event (GtkWidget           *widget,
		    GdkEventButton      *event,
		    GtrStatusComboBox *combo)
{
	if (event->type == GDK_BUTTON_PRESS && event->button == 1)
	{
		show_menu (combo, event->button,event->time);
		return TRUE;
	}

	return FALSE;
}

static gboolean
key_press_event (GtkWidget           *widget,
		 GdkEventKey         *event,
		 GtrStatusComboBox *combo)
{
	if (event->keyval == GDK_KEY_Return || event->keyval == GDK_KEY_ISO_Enter ||
	    event->keyval == GDK_KEY_KP_Enter || event->keyval == GDK_KEY_space ||
	    event->keyval == GDK_KEY_KP_Space)
	{
		show_menu (combo, 0, event->time);
		return TRUE;
	}

	return FALSE;
}

static void
set_shadow_type (GtrStatusComboBox *combo)
{
	GtkStyleContext *context;
	GtkShadowType shadow_type;
	GtkWidget *statusbar;
	GtrStatusComboBoxPrivate *priv = gtr_status_combo_box_get_instance_private (combo);

	/* This is a hack needed to use the shadow type of a statusbar */
	statusbar = gtk_statusbar_new ();
	context = gtk_widget_get_style_context (statusbar);

	gtk_style_context_get_style (context, "shadow-type", &shadow_type, NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (priv->frame), shadow_type);

	gtk_widget_destroy (statusbar);
}

static void
gtr_status_combo_box_init (GtrStatusComboBox *self)
{
	GtkStyleContext *context;
	GtrStatusComboBox *combo = GTR_STATUS_COMBO_BOX (self);
	GtrStatusComboBoxPrivate *priv = gtr_status_combo_box_get_instance_private (combo);

	gtk_event_box_set_visible_window (GTK_EVENT_BOX (self), TRUE);

	priv->frame = gtk_frame_new (NULL);
	gtk_widget_show (priv->frame);
	
	priv->button = gtk_toggle_button_new ();
	gtk_button_set_relief (GTK_BUTTON (priv->button), GTK_RELIEF_NONE);
	gtk_widget_show (priv->button);

	set_shadow_type (self);

	priv->hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 3);
	gtk_widget_show (priv->hbox);

	gtk_container_add (GTK_CONTAINER (self), priv->frame);
	gtk_container_add (GTK_CONTAINER (priv->frame), priv->button);
	gtk_container_add (GTK_CONTAINER (priv->button), priv->hbox);
	
	priv->label = gtk_label_new ("");
	gtk_widget_show (priv->label);
	
	gtk_label_set_single_line_mode (GTK_LABEL (priv->label), TRUE);
	gtk_label_set_xalign (GTK_LABEL (priv->label), 0.0);
	gtk_label_set_yalign (GTK_LABEL (priv->label), 0.5);
	
	gtk_box_pack_start (GTK_BOX (priv->hbox), priv->label, FALSE, TRUE, 0);
	
	priv->item = gtk_label_new ("");
	gtk_widget_show (priv->item);
	
	gtk_label_set_single_line_mode (GTK_LABEL (priv->item), TRUE);
	gtk_label_set_xalign (GTK_LABEL (priv->item), 0.0);
	gtk_label_set_yalign (GTK_LABEL (priv->item), 0.5);
	
	gtk_box_pack_start (GTK_BOX (priv->hbox), priv->item, TRUE, TRUE, 0);
	
	priv->arrow = gtk_image_new_from_icon_name ("go-down-symbolic", GTK_ICON_SIZE_MENU);
	gtk_widget_show (priv->arrow);
	gtk_widget_set_halign (priv->arrow, 0.5);
	gtk_widget_set_valign (priv->arrow, 0.5);
	
	gtk_box_pack_start (GTK_BOX (priv->hbox), priv->arrow, FALSE, TRUE, 0);
	
	priv->menu = gtk_menu_new ();
	g_object_ref_sink (priv->menu);

	g_signal_connect (priv->button,
			  "button-press-event",
			  G_CALLBACK (button_press_event),
			  self);
	g_signal_connect (priv->button,
			  "key-press-event",
			  G_CALLBACK (key_press_event),
			  self);
	g_signal_connect (priv->menu,
			  "deactivate",
			  G_CALLBACK (menu_deactivate),
			  self);

	/* make it as small as possible */
	context = gtk_widget_get_style_context (GTK_WIDGET (priv->button));
	gtk_style_context_add_provider (context,
	                                GTK_STYLE_PROVIDER (GTR_STATUS_COMBO_BOX_GET_CLASS (self)->priv->css),
	                                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	context = gtk_widget_get_style_context (GTK_WIDGET (priv->frame));
	gtk_style_context_add_provider (context,
	                                GTK_STYLE_PROVIDER (GTR_STATUS_COMBO_BOX_GET_CLASS (self)->priv->css),
	                                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

/* public functions */

/**
 * gtr_status_combo_box_new:
 * @label: (allow-none):
 */
GtkWidget *
gtr_status_combo_box_new (const gchar *label)
{
	return g_object_new (GTR_TYPE_STATUS_COMBO_BOX, "label", label, NULL);
}

/**
 * gtr_status_combo_box_set_label:
 * @combo:
 * @label: (allow-none):
 */
void
gtr_status_combo_box_set_label (GtrStatusComboBox *combo, 
				  const gchar         *label)
{
	gchar *text;
	GtrStatusComboBoxPrivate *priv = gtr_status_combo_box_get_instance_private (combo);

	g_return_if_fail (GTR_IS_STATUS_COMBO_BOX (combo));
	
	text = g_strconcat ("  ", label, ": ", NULL);
	gtk_label_set_markup (GTK_LABEL (priv->label), text);
	g_free (text);
}

const gchar *
gtr_status_combo_box_get_label (GtrStatusComboBox *combo)
{
	GtrStatusComboBoxPrivate *priv = gtr_status_combo_box_get_instance_private (combo);
	g_return_val_if_fail (GTR_IS_STATUS_COMBO_BOX (combo), NULL);

	return gtk_label_get_label (GTK_LABEL (priv->label));
}

static void
item_activated (GtkMenuItem         *item,
		GtrStatusComboBox *combo)
{
	gtr_status_combo_box_set_item (combo, item);
}

/**
 * gtr_status_combo_box_add_item:
 * @combo:
 * @item:
 * @text: (allow-none):
 */
void
gtr_status_combo_box_add_item (GtrStatusComboBox *combo,
				 GtkMenuItem         *item,
				 const gchar         *text)
{
	GtrStatusComboBoxPrivate *priv = gtr_status_combo_box_get_instance_private (combo);
	g_return_if_fail (GTR_IS_STATUS_COMBO_BOX (combo));
	g_return_if_fail (GTK_IS_MENU_ITEM (item));

	gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), GTK_WIDGET (item));
	
	gtr_status_combo_box_set_item_text (combo, item, text);
	g_signal_connect (item, "activate", G_CALLBACK (item_activated), combo);
}

void
gtr_status_combo_box_remove_item (GtrStatusComboBox *combo,
				    GtkMenuItem         *item)
{
	GtrStatusComboBoxPrivate *priv = gtr_status_combo_box_get_instance_private (combo);
	g_return_if_fail (GTR_IS_STATUS_COMBO_BOX (combo));
	g_return_if_fail (GTK_IS_MENU_ITEM (item));

	gtk_container_remove (GTK_CONTAINER (priv->menu),
			      GTK_WIDGET (item));
}

/**
 * gtr_status_combo_box_get_items:
 * @combo:
 *
 * Returns: (element-type Gtk.Widget) (transfer container):
 */
GList *
gtr_status_combo_box_get_items (GtrStatusComboBox *combo)
{
	GtrStatusComboBoxPrivate *priv = gtr_status_combo_box_get_instance_private (combo);
	g_return_val_if_fail (GTR_IS_STATUS_COMBO_BOX (combo), NULL);

	return gtk_container_get_children (GTK_CONTAINER (priv->menu));
}

const gchar *
gtr_status_combo_box_get_item_text (GtrStatusComboBox *combo,
				      GtkMenuItem	  *item)
{
	const gchar *ret = NULL;
	
	g_return_val_if_fail (GTR_IS_STATUS_COMBO_BOX (combo), NULL);
	g_return_val_if_fail (GTK_IS_MENU_ITEM (item), NULL);
	
	ret = g_object_get_data (G_OBJECT (item), COMBO_BOX_TEXT_DATA);
	
	return ret;
}

/**
 * gtr_status_combo_box_set_item_text:
 * @combo:
 * @item:
 * @text: (allow-none):
 */
void 
gtr_status_combo_box_set_item_text (GtrStatusComboBox *combo,
				      GtkMenuItem	  *item,
				      const gchar         *text)
{
	g_return_if_fail (GTR_IS_STATUS_COMBO_BOX (combo));
	g_return_if_fail (GTK_IS_MENU_ITEM (item));

	g_object_set_data_full (G_OBJECT (item), 
				COMBO_BOX_TEXT_DATA,
				g_strdup (text),
				(GDestroyNotify)g_free);
}

void
gtr_status_combo_box_set_item (GtrStatusComboBox *combo,
				 GtkMenuItem         *item)
{
	g_return_if_fail (GTR_IS_STATUS_COMBO_BOX (combo));
	g_return_if_fail (GTK_IS_MENU_ITEM (item));

	g_signal_emit (combo, signals[CHANGED], 0, item, NULL);
}

GtkLabel *
gtr_status_combo_box_get_item_label (GtrStatusComboBox *combo)
{
	GtrStatusComboBoxPrivate *priv = gtr_status_combo_box_get_instance_private (combo);
	g_return_val_if_fail (GTR_IS_STATUS_COMBO_BOX (combo), NULL);
	
	return GTK_LABEL (priv->item);
}

/* ex:set ts=8 noet: */
