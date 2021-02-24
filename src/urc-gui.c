/* urc-gui.c
 *
 * Copyright (C) 2009 Daniele Napolitano
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "config.h"

#include <glib.h>
#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>

#include "urc-graph.h"
#include "urc-upnp.h"
#include "urc-gui.h"

#define URC_RESOURCE_BASE "/org/upnp-router-control/"

enum Columns
{
    UPNP_COLUMN_ENABLED,
    UPNP_COLUMN_DESC,
    UPNP_COLUMN_PROTOCOL,
    UPNP_COLUMN_INT_PORT,
    UPNP_COLUMN_EXT_PORT,
    UPNP_COLUMN_LOCAL_IP,
    UPNP_COLUMN_REM_IP
};

typedef struct
{
    GtkWidget *window,
              *add_desc,
              *add_ext_port,
              *add_proto,
              *add_local_ip,
              *add_local_port,
              *button_apply,
              *button_cancel,
              *expander;

} AddPortWindow;

typedef struct
{
    GtkBuilder* builder;

    GtkWidget *main_window,
              *treeview,
              *router_name_label,
              *router_url_label,
              *config_label,
              *wan_status_label,
              *ip_label,
              *down_rate_label,
              *up_rate_label,
              *total_received_label,
              *total_sent_label,
              *button_remove,
              *button_add,
              *router_icon,
              *refresh_button,
              *network_drawing_area;

    GtkMenuButton *menu_button;

    AddPortWindow* add_port_window;

    GMenuModel *headermenu;

} GuiContext;

static GuiContext* gui;

void gui_set_router_icon(gchar *image_path)
{
    static GdkPixbuf *pixbuf = NULL;

    if(pixbuf != NULL)
        g_object_unref(pixbuf);

    if(image_path == NULL || !g_file_test(image_path, G_FILE_TEST_EXISTS))
        gtk_image_set_from_icon_name(GTK_IMAGE(gui->router_icon), "upnp-router-control", 64);

    else {
        // load the image and resize it
        pixbuf = gdk_pixbuf_new_from_file_at_size(image_path, 64, 64, NULL);
        gtk_image_set_from_pixbuf(GTK_IMAGE(gui->router_icon), pixbuf);

    }
    gtk_widget_set_size_request(gui->router_icon, 64, 64);
}

static void
gui_add_port_window_close(GtkWidget *button,
                          gpointer   user_data)
{
    gtk_widget_hide (gui->add_port_window->window);
}

static void
gui_add_port_window_apply(GtkWidget *button,
                          gpointer   user_data)
{
    PortForwardInfo* port_info;
    GError* error = NULL;
    GtkWidget* spinner;
    gchar const *btn_label;

    // Creating the PortForwardInfo structure
    port_info = g_malloc( sizeof(PortForwardInfo) );

    port_info->description = g_strdup( gtk_entry_get_text(GTK_ENTRY(gui->add_port_window->add_desc)) );
    port_info->protocol = g_strdup( gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT (gui->add_port_window->add_proto)) );
    port_info->internal_port = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON (gui->add_port_window->add_local_port) );
    port_info->external_port = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON (gui->add_port_window->add_ext_port) );
    port_info->internal_host = g_strdup( gtk_entry_get_text(GTK_ENTRY(gui->add_port_window->add_local_ip)) );
    port_info->remote_host = "";
    port_info->lease_time = 0;
    port_info->enabled = TRUE;

    // Set spinner on on the apply button and remove temporarily the label
    spinner = gtk_spinner_new();
    btn_label = g_strdup(gtk_button_get_label (GTK_BUTTON(gui->add_port_window->button_apply)));
    gtk_button_set_label (GTK_BUTTON(gui->add_port_window->button_apply), NULL);
    gtk_button_set_image (GTK_BUTTON(gui->add_port_window->button_apply), spinner);
    gtk_button_set_always_show_image (GTK_BUTTON(gui->add_port_window->button_apply), TRUE);
    gtk_spinner_start (GTK_SPINNER(spinner));

    // Try to add the new port mapping.
    if(add_port_mapping(user_data, port_info, &error) != TRUE)
    {
        // We have errors.
        GtkWidget* dialog;
        dialog = gtk_message_dialog_new(GTK_WINDOW(gui->add_port_window->window),
                                        GTK_DIALOG_MODAL,
                                        GTK_MESSAGE_ERROR,
                                        GTK_BUTTONS_OK,
                                        _("Unable to set this port forward"));

        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG(dialog),
	                                            "%d: %s", error->code, error->message);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }

    // Stop the spinner and restore the button label
    gtk_spinner_stop(GTK_SPINNER(spinner));
    gtk_button_set_label(GTK_BUTTON(gui->add_port_window->button_apply), btn_label);
    gtk_button_set_image (GTK_BUTTON(gui->add_port_window->button_apply), NULL);

    g_free(port_info->description);
    g_free(port_info->protocol);
    g_free(port_info->internal_host);
    g_free(port_info);

    if (error == NULL) {
        // No errors, close the dialog.
        gtk_widget_hide (gui->add_port_window->window);
    }
    else {
        g_error_free (error);
    }
}

static void gui_add_port_window_on_port_change (GtkSpinButton *spinbutton,
                                                gpointer       user_data)
{
	guint num = 0;

	// sync values only in "easy" mode
	if(gtk_expander_get_expanded(GTK_EXPANDER(gui->add_port_window->expander)) == FALSE)
	{
		num = (guint) gtk_spin_button_get_value(spinbutton);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON(gui->add_port_window->add_local_port), num);
	}
}

static void gui_run_add_port_window(GtkWidget *button,
                                    gpointer   user_data)
{

	gtk_entry_set_text (GTK_ENTRY(gui->add_port_window->add_desc), "");
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(gui->add_port_window->add_ext_port), 0);
    gtk_entry_set_text (GTK_ENTRY(gui->add_port_window->add_local_ip), get_client_ip());
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(gui->add_port_window->add_local_port), 0);

    /* Disconnect previous signals */
    g_signal_handlers_disconnect_matched(gui->add_port_window->button_apply,
    									 G_SIGNAL_MATCH_FUNC,
    									 0,
    									 0,
    									 NULL,
										 G_CALLBACK(gui_add_port_window_apply),
    									 NULL);

    /* Connect new signal with new data */
    g_signal_connect(gui->add_port_window->button_apply, "clicked",
                         G_CALLBACK(gui_add_port_window_apply), user_data);

    gtk_widget_show_all (gui->add_port_window->window);
}

static void gui_create_add_port_window(GtkBuilder* builder)
{
    AddPortWindow* add_port_window;

    add_port_window = g_malloc( sizeof(AddPortWindow) );

    add_port_window->window = GTK_WIDGET (gtk_builder_get_object (builder, "add_port_window"));
    g_assert (add_port_window->window != NULL);

    gtk_window_set_transient_for(GTK_WINDOW(add_port_window->window), GTK_WINDOW(gui->main_window));

    add_port_window->add_desc = GTK_WIDGET (gtk_builder_get_object (builder, "add_desc"));
    add_port_window->add_ext_port = GTK_WIDGET (gtk_builder_get_object (builder, "add_ext_port"));
    add_port_window->add_proto = GTK_WIDGET (gtk_builder_get_object (builder, "add_proto"));
    add_port_window->add_local_ip = GTK_WIDGET (gtk_builder_get_object (builder, "add_local_ip"));
    add_port_window->add_local_port = GTK_WIDGET (gtk_builder_get_object (builder, "add_local_port"));
    add_port_window->button_apply = GTK_WIDGET (gtk_builder_get_object (builder, "button_apply"));
    add_port_window->button_cancel = GTK_WIDGET (gtk_builder_get_object (builder, "button_cancel"));
    add_port_window->expander = GTK_WIDGET (gtk_builder_get_object (builder, "expander1"));

    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(add_port_window->add_proto), "TCP");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(add_port_window->add_proto), "UDP");
    
    gtk_combo_box_set_active(GTK_COMBO_BOX(add_port_window->add_proto), 0);

    g_signal_connect(add_port_window->add_ext_port, "value-changed",
                         G_CALLBACK(gui_add_port_window_on_port_change), NULL);

    g_signal_connect(add_port_window->button_cancel, "clicked",
                         G_CALLBACK(gui_add_port_window_close), NULL);

    g_signal_connect(add_port_window->window, "delete-event",
                         G_CALLBACK(gtk_widget_hide_on_delete), NULL);

    gui->add_port_window = add_port_window;
}

/* Clean the Treeview */
void gui_clear_ports_list_treeview (void)
{
    GtkTreeModel *model;
    GtkTreeIter   iter;
    gboolean      more;

    if(gui->treeview == NULL)
        return;

    model = gtk_tree_view_get_model (GTK_TREE_VIEW (gui->treeview));
    more = gtk_tree_model_get_iter_first (model, &iter);

    while (more)
        more = gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
}

/* Add a port mapped in the treeview list */
void gui_add_mapped_ports(const GSList *port_list)
{
    GtkTreeModel *model;
    GtkTreeIter   iter;
    const GSList *port_iter = port_list;
    PortForwardInfo *port_info;

    if(gui->treeview == NULL)
        return;

    model = gtk_tree_view_get_model (GTK_TREE_VIEW (gui->treeview));

    while(port_iter)
    {
        port_info = (PortForwardInfo*) port_iter->data;

        gtk_list_store_prepend (GTK_LIST_STORE (model), &iter);
        gtk_list_store_set (GTK_LIST_STORE (model),
                            &iter,
                            UPNP_COLUMN_ENABLED, port_info->enabled,
                            UPNP_COLUMN_DESC, port_info->description,
                            UPNP_COLUMN_PROTOCOL, port_info->protocol,
                            UPNP_COLUMN_INT_PORT, port_info->internal_port,
                            UPNP_COLUMN_EXT_PORT, port_info->external_port,
                            UPNP_COLUMN_LOCAL_IP, port_info->internal_host,
                            UPNP_COLUMN_REM_IP, port_info->remote_host,
                            -1);

        port_iter = g_slist_next (port_iter);
    }

}

/* Button remove callback */
static void on_button_remove_clicked (GtkWidget *button,
                                      gpointer   user_data)
{
    GtkTreeModel *model;
    GtkTreeIter   iter;
    GtkTreeSelection *selection;
    GError *error;

    gchar* remote_host;
    guint external_port;
    gchar* protocol;

    model = gtk_tree_view_get_model (GTK_TREE_VIEW (gui->treeview));
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (gui->treeview));

    if(!gtk_tree_selection_get_selected(selection, NULL, &iter))
    {
    	/* no selection */
    	gtk_widget_set_sensitive(button, FALSE);
    	return;
    }

    gtk_tree_model_get(model, &iter,
                       UPNP_COLUMN_PROTOCOL, &protocol,
                       UPNP_COLUMN_EXT_PORT, &external_port,
                       UPNP_COLUMN_REM_IP, &remote_host,
                       -1);

    if(delete_port_mapped (user_data, protocol, external_port, remote_host, &error) != TRUE)
    {
        // We have errors.
        GtkWidget* dialog;

        dialog = gtk_message_dialog_new(GTK_WINDOW(gui->main_window),
                                        GTK_DIALOG_MODAL,
                                        GTK_MESSAGE_ERROR,
                                        GTK_BUTTONS_OK,
                                        _("Unable to remove this port forward"));

        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG(dialog),
	                                            "%d: %s", error->code, error->message);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_error_free (error);
    }
    else {
        // delete row from the local list
	    gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
    }
}


/* Callback for enable checkbox */
/*static void list_enable_toggled_cb (GtkCellRendererToggle *widget,
                             gchar                 *path,
                             GtkWidget             *treeview)
{
    gboolean enabled;

    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(gui->treeview));

    if (!gtk_tree_model_get_iter_from_string(model, &iter, path))
        return;

    gtk_tree_model_get(model, &iter,
                       UPNP_COLUMN_ENABLED, &enabled,
                       -1);

    // set value in store
    gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                        UPNP_COLUMN_ENABLED, !enabled,
                        -1);

}*/

static gboolean gui_on_treeview_selection (GtkTreeSelection *selection,
                       GtkTreeModel     *model,
                       GtkTreePath      *path,
                       gboolean          path_currently_selected,
                       gpointer          userdata)
{

	if (!path_currently_selected)
		gtk_widget_set_sensitive(gui->button_remove, TRUE);

	else
		gtk_widget_set_sensitive(gui->button_remove, FALSE);


	return TRUE; /* allow selection state to change */
}


/* Initialize model for Treeview */
static void gui_init_treeview()
{
    GtkTreeModel *model;
    GtkTreeSelection *selection;
    int i;
    char         *headers[8] = {_("Enabled"),
                                _("Description"),
                                _("Protocol"),
                                _("Int. Port"),
                                _("Ext. Port"),
                                _("Local IP"),
                                /*_("Remote IP"),*/
                                NULL };

    GtkListStore *store;

    store = gtk_list_store_new (7,
                                G_TYPE_BOOLEAN, /* Enabled           */
                                G_TYPE_STRING,  /* Description       */
                                G_TYPE_STRING,  /* Protocol          */
                                G_TYPE_UINT,    /* Internal port     */
                                G_TYPE_UINT,    /* External port     */
                                G_TYPE_STRING,  /* Internal host     */
                                G_TYPE_STRING); /* Remove host Value */

    model = GTK_TREE_MODEL (store);

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (gui->treeview));
    g_assert (selection != NULL);

    for (i = 0; headers[i] != NULL; i++) {
                GtkCellRenderer   *renderer;
                GtkTreeViewColumn *column;

                column = gtk_tree_view_column_new ();
                if(i == 0)
                {
                    renderer = gtk_cell_renderer_toggle_new ();
                    gtk_tree_view_column_pack_end (column, renderer, FALSE);

                    gtk_tree_view_column_add_attribute (column,
                                                    renderer,
                                                    "active", i);

                    /* enable/disable port mapping callback */
                    /*g_signal_connect(G_OBJECT(renderer), "toggled",
                         G_CALLBACK(list_enable_toggled_cb),
                         NULL);*/

                    g_object_set(renderer,
                     "radio", FALSE,
                     "activatable", TRUE,
                      NULL);
                }
                else
                {
                    renderer = gtk_cell_renderer_text_new ();
                    gtk_tree_view_column_pack_end (column, renderer, TRUE);
                    gtk_tree_view_column_set_title (column, headers[i]);

                    gtk_tree_view_column_add_attribute (column,
                                                    renderer,
                                                    "text", i);
                }

                gtk_tree_view_column_set_sort_column_id(column, i);

                gtk_tree_view_column_set_sizing(column,
                                                GTK_TREE_VIEW_COLUMN_FIXED);

                gtk_tree_view_insert_column (GTK_TREE_VIEW (gui->treeview),
                                             column, -1);

                gtk_tree_view_column_set_sizing(column,
                                                GTK_TREE_VIEW_COLUMN_AUTOSIZE);

                gtk_tree_view_column_set_resizable (column, TRUE);
        }

        gtk_tree_view_set_model (GTK_TREE_VIEW (gui->treeview),
                                 model);
        gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

        gtk_tree_selection_set_select_function(selection, gui_on_treeview_selection, NULL, NULL);

}

void gui_disable_total_received()
{
   if(gui->total_received_label == NULL)
    	return;

    gtk_widget_set_sensitive(gui->total_received_label, FALSE);
}

void gui_set_total_received (const unsigned int total_received)
{
    gchar* str;
    gchar* unit;
    float value;

    if(total_received >= 1073741824)
    {
        value = total_received / 1073741824.0;
        unit = g_strdup("GiB");
    }
    else if(total_received >= 1048576)
    {
        value = total_received / 1048576.0;
        unit = g_strdup("MiB");
    }
    else if(total_received >= 1024)
    {
        value = total_received / 1024.0;
        unit = g_strdup("KiB");
    }
    else
    {
        value = total_received;
        unit = g_strdup("B");
    }
    str = g_strdup_printf("<b>%s</b> %0.1f %s", _("Total received:"), value, unit);
    g_free(unit);

    if(gui->total_received_label == NULL)
    	return;

    gtk_label_set_markup (GTK_LABEL(gui->total_received_label), str);
    g_free(str);

    gtk_widget_set_sensitive(gui->total_received_label, TRUE);
}

void gui_disable_total_sent()
{
   if(gui->total_sent_label == NULL)
    	return;

    gtk_widget_set_sensitive(gui->total_sent_label, FALSE);
}

void gui_set_total_sent (const unsigned int total_sent)
{
    gchar* str;
    gchar* unit;
    float value;

    if(total_sent >= 1073741824)
    {
        value = total_sent / 1073741824.0;
        unit = g_strdup("GiB");
    }
    else if(total_sent >= 1048576)
    {
        value = total_sent / 1048576.0;
        unit = g_strdup("MiB");
    }
    else if(total_sent >= 1024)
    {
        value = total_sent / 1024.0;
        unit = g_strdup("KiB");
    }
    else
    {
        value = total_sent;
        unit = g_strdup("B");
    }
    str = g_strdup_printf("<b>%s</b> %0.1f %s", _("Total sent:"), value, unit);
    g_free(unit);

    if(gui->total_sent_label == NULL)
    	return;

    gtk_label_set_markup (GTK_LABEL(gui->total_sent_label), str);
    g_free(str);

    gtk_widget_set_sensitive(gui->total_sent_label, TRUE);
}

void gui_disable_download_speed()
{
    if(gui->down_rate_label == NULL)
    	return;

    gtk_widget_set_sensitive(gui->down_rate_label, FALSE);
    gtk_label_set_text (GTK_LABEL(gui->down_rate_label), "—" );
}

/* Update router speeds, values in KiB/sec */
void gui_set_download_speed(const gdouble down_speed)
{
    gchar* str;
    gchar* unit;
    gfloat value;
    SpeedValue *speed;

    speed = g_malloc (sizeof(SpeedValue));

    speed->speed = down_speed;
    speed->valid = TRUE;

    update_download_graph_data(speed);

    /* Method of divisions is too expensive? */
    /* Down speed */
    if(down_speed >= 1048576)
    {
        value = down_speed / 1048576.0;
        unit = g_strdup("GiB/s");
    }
    else if(down_speed >= 1024)
    {
        value = down_speed / 1024.0;
        unit = g_strdup("MiB/s");
    }
    else
    {
        value = down_speed;
        unit = g_strdup("KiB/s");
    }
    str = g_strdup_printf("%0.1f %s", value, unit);
    g_free(unit);

    if(gui->down_rate_label == NULL)
    	return;

    gtk_label_set_text (GTK_LABEL(gui->down_rate_label), str);
    g_free(str);

    gtk_widget_set_sensitive(gui->down_rate_label, TRUE);

}

void gui_disable_upload_speed()
{
    if(gui->up_rate_label == NULL)
    	return;

    gtk_widget_set_sensitive(gui->up_rate_label, FALSE);
    gtk_label_set_text (GTK_LABEL(gui->up_rate_label), "—" );
}

/* Update router speeds, values in KiB/sec */
void gui_set_upload_speed(const gdouble up_speed)
{
    gchar* str;
    gchar* unit;
    gfloat value;
    SpeedValue *speed;

    speed = g_malloc (sizeof(SpeedValue));

    speed->speed = up_speed;
    speed->valid = TRUE;

    update_upload_graph_data(speed);

    /* Method of divisions is too expensive? */
    /* Up speed */
    if(up_speed >= 1048576)
    {
        value = up_speed / 1048576.0;
        unit = g_strdup("GiB/s");
    }
    else  if(up_speed >= 1024)
    {
        value = up_speed / 1024.0;
        unit = g_strdup("MiB/s");
    }
    else
    {
        value = up_speed;
        unit = g_strdup("KiB/s");
    }
    str = g_strdup_printf("%0.1f %s", value, unit);
    g_free(unit);

    if(gui->up_rate_label == NULL)
    	return;

    gtk_label_set_text (GTK_LABEL(gui->up_rate_label), str);
    g_free(str);

    gtk_widget_set_sensitive(gui->up_rate_label, TRUE);

}

void gui_update_graph()
{
    if(gui->network_drawing_area == NULL)
        return;

    gtk_widget_queue_draw(gui->network_drawing_area);
}

/* Set WAN state label */
void gui_set_conn_status(const gchar *state)
{
    gchar* str = NULL;

    if(gui->wan_status_label == NULL)
    	return;

    if(g_strcmp0("Connected", state) == 0)
            str = g_strdup_printf( "<b>%s</b> <span color=\"#009000\"><b>%s</b></span>", _("WAN status:"), _("Connected") );
    if(g_strcmp0("Disconnected", state) == 0)
            str = g_strdup_printf( "<b>%s</b> <span color=\"#900000\"><b>%s</b></span>", _("WAN status:"), _("Disconnected"));
    if(g_strcmp0("Connecting", state) == 0)
            str = g_strdup_printf( "<b>%s</b> <span color=\"#0000a0\"><b>%s</b></span>", _("WAN status:"), _("Connecting"));
    if(g_strcmp0("Disconnecting", state) == 0)
            str = g_strdup_printf( "<b>%s</b> <span color=\"#c04000\"><b>%s</b></span>", _("WAN status:"), _("Disconnecting"));

    gtk_label_set_markup (GTK_LABEL(gui->wan_status_label), str);

    gtk_widget_set_sensitive(gui->wan_status_label, TRUE);

}

/* Set WAN state label unknown */
void gui_disable_conn_status()
{
    gchar* str = NULL;

    if(gui->wan_status_label == NULL)
    	return;

    str = g_strdup_printf("<b>%s</b> %s", _("WAN status:"), _("unknown"));

    gtk_label_set_markup (GTK_LABEL(gui->wan_status_label), str);

    g_free(str);

}

/* Set external IP address */
void gui_set_ext_ip(const gchar *ip)
{
    gchar* str;

    if(gui->ip_label == NULL)
    	return;

    if(ip == NULL)
        str = g_strdup_printf( "<b>%s</b> %s", _("IP:"), _("none"));
    else
        str = g_strdup_printf( "<b>%s</b> %s", _("IP:"), ip);

    gtk_label_set_markup (GTK_LABEL(gui->ip_label), str);
    g_free(str);

    gtk_widget_set_sensitive(gui->ip_label, TRUE);

}

/* Set external IP address */
void gui_disable_ext_ip()
{
    gchar* str = NULL;

    if(gui->ip_label == NULL)
    	return;

    str = g_strdup_printf( "<b>%s</b> %s", _("IP:"), _("unknown"));

    gtk_label_set_markup (GTK_LABEL(gui->ip_label), str);

    g_free(str);
}

/* Set router informations */
void gui_set_router_info (const gchar *router_friendly_name,
                          const gchar *router_conf_url,
                          const gchar *router_brand,
                          const gchar *router_brand_website,
                          const gchar *router_model_description,
                          const gchar *router_model_name,
                          const gchar *router_model_number)
{
    gchar* str;

    if(gui == NULL || gui->main_window == NULL)
        return;

    str = g_strdup_printf( "<b>%s</b> %s", _("Router name:"), router_friendly_name);
    gtk_label_set_markup (GTK_LABEL(gui->router_name_label), str);
    g_free(str);

    gtk_widget_set_sensitive(gui->router_name_label, TRUE);

    str = g_strdup_printf( "<b>%s</b> %s\n<b>%s</b> %s\n<b>%s</b> %s\n<b>%s</b> %s\n<b>%s</b> %s",
                           _("Brand"),
                           router_brand,
                           _("Brand website:"),
                           router_brand_website,
                           _("Model Description:"),
                           router_model_description,
                           _("Model Name:"),
                           router_model_name,
                           _("Model Number:"),
                           router_model_number);

    gtk_widget_set_tooltip_markup(gui->router_name_label, str);
    g_free(str);

    if(router_conf_url != NULL) {

        str = g_strdup_printf( "<a href=\"%s\">%s</a>", router_conf_url, router_conf_url);
        gtk_label_set_markup (GTK_LABEL(gui->router_url_label), str);
        g_free(str);

        gtk_widget_set_sensitive(gui->router_url_label, TRUE);
        gtk_widget_set_sensitive(gui->config_label, TRUE);

        gtk_widget_set_tooltip_text(gui->router_url_label, _("Open the router config in the default browser"));

	} else {

	    gtk_label_set_label(GTK_LABEL(gui->router_url_label), _("not available"));
        gtk_widget_set_sensitive(gui->config_label, FALSE);
    }

}

/* updates all values */
static void on_refresh_activate_cb (GtkMenuItem *menuitem,
                                  gpointer     user_data)
{

    get_conn_status ((RouterInfo *) user_data);
    get_external_ip ((RouterInfo *) user_data);
    get_nat_rsip_status ((RouterInfo *) user_data);
    discovery_mapped_ports_list((RouterInfo *) user_data);
}

void gui_set_ports_buttons_callback_data(gpointer data)
{
    g_signal_connect(gui->button_remove, "clicked",
                     G_CALLBACK(on_button_remove_clicked), data);

    g_signal_connect(gui->button_add, "clicked",
                     G_CALLBACK(gui_run_add_port_window), data);

    gtk_widget_set_sensitive(gui->button_add, TRUE);
}

void gui_set_refresh_callback_data(gpointer data)
{
    g_signal_connect(gui->refresh_button, "clicked",
                     G_CALLBACK(on_refresh_activate_cb), data);

    gtk_widget_set_sensitive(gui->refresh_button, TRUE);
}

void gui_enable()
{
    if(gui == NULL || gui->main_window == NULL)
        return;

    gtk_widget_set_sensitive(gui->router_name_label, TRUE);
    gtk_widget_set_sensitive(gui->wan_status_label, TRUE);
    gtk_widget_set_sensitive(gui->ip_label, TRUE);
    gtk_widget_set_sensitive(gui->down_rate_label, TRUE);
    gtk_widget_set_sensitive(gui->up_rate_label, TRUE);
    gtk_widget_set_sensitive(gui->total_received_label, TRUE);
    gtk_widget_set_sensitive(gui->total_sent_label, TRUE);
    gtk_widget_set_sensitive(gui->router_url_label, TRUE);
    gtk_widget_set_sensitive(gui->button_add, TRUE);
    gtk_widget_set_sensitive(gui->config_label, TRUE);
    gtk_widget_set_sensitive(gui->refresh_button, TRUE);
}

void gui_disable()
{
    if(gui == NULL || gui->main_window == NULL)
        return;

    gchar *str;

    gui_clear_ports_list_treeview();

    str = g_strdup_printf("<b>%s</b> %s", _("Router name:"), _("not available") );
    gtk_label_set_markup (GTK_LABEL(gui->router_name_label), str);
    gtk_widget_set_sensitive(gui->router_name_label, FALSE);
    g_free(str);

    str = g_strdup_printf("<b>%s</b> %s", _("WAN status:"), _("not available") );
    gtk_label_set_markup (GTK_LABEL(gui->wan_status_label), str);
    gtk_widget_set_sensitive(gui->wan_status_label, FALSE);
    g_free(str);

    str = g_strdup_printf("<b>%s</b> %s", _("IP:"), _("not available") );
    gtk_label_set_markup (GTK_LABEL(gui->ip_label), str);
    gtk_widget_set_sensitive(gui->ip_label, FALSE);
    g_free(str);

    gtk_label_set_markup (GTK_LABEL(gui->down_rate_label), "—");
    gtk_widget_set_sensitive(gui->down_rate_label, FALSE);

    gtk_label_set_markup (GTK_LABEL(gui->up_rate_label), "—");
    gtk_widget_set_sensitive(gui->up_rate_label, FALSE);

    str = g_strdup_printf("<b>%s</b> %s", _("Total received:"), "—" );
    gtk_label_set_markup (GTK_LABEL(gui->total_received_label), str);
    gtk_widget_set_sensitive(gui->total_received_label, FALSE);
    g_free(str);

    str = g_strdup_printf("<b>%s</b> %s", _("Total sent:"), "—" );
    gtk_label_set_markup (GTK_LABEL(gui->total_sent_label), str);
    gtk_widget_set_sensitive(gui->total_sent_label, FALSE);
    g_free(str);

    gtk_label_set_text (GTK_LABEL(gui->router_url_label), _("not available"));
    gtk_widget_set_sensitive(gui->router_url_label, FALSE);

    gtk_widget_set_sensitive(gui->button_add, FALSE);
    gtk_widget_set_sensitive(gui->button_remove, FALSE);

    gtk_widget_set_sensitive(gui->config_label, FALSE);

    gtk_widget_set_sensitive(gui->refresh_button, FALSE);

    disable_graph_data();
    gui_update_graph();

}


/* Menu About activate callback */
static void on_about_activate_cb (GSimpleAction *simple, GVariant *parameter, gpointer user_data)
{
    gchar* authors[] = {
		  "Daniele Napolitano <dnax88@gmail.com>",
		  "Giuseppe Cicalini <cicone@gmail.com> (basic cURL code)",
		  NULL
	  };

    gchar* artists[] = {
	  	"Frédéric Bellaiche http://www.quantum-bits.org",
	  	NULL
	  };

	  /* Feel free to put your names here translators :-) */
	  gchar* translators = _("translator-credits");

	  gtk_show_about_dialog (GTK_WINDOW(gui->main_window),
        "authors", authors,
        "artists", artists,
        "translator-credits", strcmp("translator-credits", translators) ? translators : NULL,
        "comments", _("A simple program to manage UPnP IGD compliant routers"),
        "copyright", "Copyright © 2009-2021 Daniele Napolitano \"DnaX\"",
        "version", VERSION,
        "license-type", GTK_LICENSE_GPL_3_0,
        "website", "https://launchpad.net/upnp-router-control",
        "logo-icon-name", "org.upnproutercontrol.UPnPRouterControl",
        NULL);
}


static void gui_destroy()
{
	g_print("* Destroying GUI...\n");

	gtk_widget_destroy(gui->add_port_window->window);
	g_free(gui->add_port_window);

	gui_disable();

	gtk_widget_destroy(gui->main_window);

	gui->down_rate_label = NULL;
	gui->up_rate_label = NULL;
	gui->total_received_label = NULL;
	gui->total_sent_label = NULL;
	gui->wan_status_label = NULL;
	gui->ip_label = NULL;
	gui->treeview = NULL;
	gui->network_drawing_area = NULL;

	gui->main_window = NULL;

	g_free(gui);

	gtk_main_quit();
}

void urc_gui_init(GApplication *app)
{
    GtkBuilder* builder;
    GError* error = NULL;

    g_print("* Initializing GUI...\n");

    gui = g_malloc( sizeof(GuiContext) );

    builder = gtk_builder_new ();
    if (!gtk_builder_add_from_resource (builder, URC_RESOURCE_BASE "ui/main.ui", &error))
    {
        g_error ("Couldn't load builder file: %s", error->message);
        g_error_free (error);
    }

    gui->main_window = GTK_WIDGET (gtk_builder_get_object (builder, "main_window"));
    g_assert (gui->main_window != NULL);

    gui->treeview = GTK_WIDGET (gtk_builder_get_object (builder, "treeview1"));

    gui->router_name_label = GTK_WIDGET (gtk_builder_get_object (builder, "router_name_label"));
    gui->router_url_label = GTK_WIDGET (gtk_builder_get_object (builder, "router_url_label"));
    gui->config_label = GTK_WIDGET (gtk_builder_get_object (builder, "label_config"));
    gui->wan_status_label = GTK_WIDGET (gtk_builder_get_object (builder, "wan_status_label"));
    gui->ip_label = GTK_WIDGET (gtk_builder_get_object (builder, "ip_label"));
    gui->down_rate_label = GTK_WIDGET (gtk_builder_get_object (builder, "down_rate_label"));
    gui->up_rate_label = GTK_WIDGET (gtk_builder_get_object (builder, "up_rate_label"));
    gui->router_icon = GTK_WIDGET (gtk_builder_get_object (builder, "image1"));
    gui->network_drawing_area = GTK_WIDGET (gtk_builder_get_object (builder, "network_graph"));
    gui->total_received_label = GTK_WIDGET (gtk_builder_get_object (builder, "total_received_label"));
    gui->total_sent_label = GTK_WIDGET (gtk_builder_get_object (builder, "total_sent_label"));

    gui->button_add = GTK_WIDGET (gtk_builder_get_object (builder, "button_add"));
    gui->button_remove = GTK_WIDGET (gtk_builder_get_object (builder, "button_remove"));

    gui->refresh_button = GTK_WIDGET (gtk_builder_get_object (builder, "refresh_button"));

    gui->menu_button = GTK_MENU_BUTTON (gtk_builder_get_object (builder, "menu_button"));

    g_signal_connect(G_OBJECT(gui->main_window), "delete-event",
                         G_CALLBACK(gui_destroy), NULL);

    gui_create_add_port_window(builder);
    g_object_unref (G_OBJECT (builder));

    gtk_icon_theme_add_resource_path (gtk_icon_theme_get_default (),
                                    URC_RESOURCE_BASE"/icons");

    builder = gtk_builder_new ();
    if (!gtk_builder_add_from_resource (builder, URC_RESOURCE_BASE "ui/headerbar-menu.ui", &error))
    {
        g_error ("Couldn't load builder file: %s", error->message);
        g_error_free (error);
    }

    gui->headermenu =  G_MENU_MODEL(gtk_builder_get_object (builder, "headermenu"));

    // Menu actions.
    const GActionEntry entries[] = {
        { "about", on_about_activate_cb }
    };

    GActionGroup *actions = G_ACTION_GROUP( g_simple_action_group_new () );
    gtk_widget_insert_action_group (gui->main_window, "app", actions);
    g_action_map_add_action_entries (G_ACTION_MAP (actions), entries, G_N_ELEMENTS (entries), gui->main_window);
    gtk_menu_button_set_menu_model(gui->menu_button, gui->headermenu);

    g_object_unref (G_OBJECT (builder));

    gui_init_treeview();

    gui_disable();

    urc_init_network_graph(gui->network_drawing_area);

    g_print("* Showing GUI...\n");

    gtk_application_add_window(GTK_APPLICATION(app), GTK_WINDOW(gui->main_window));
    gtk_widget_show_all(gui->main_window);
}

