/* urc-gui.h
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

#ifndef __URC_GUI_H__
#define __URC_GUI_H__

#include <glib.h>
#include "urc-upnp.h"

void gui_init();

void gui_enable();

void gui_disable();

void gui_set_ports_buttons_callback_data(gpointer data);

void gui_set_refresh_callback_data(gpointer data);

void gui_set_router_info (const gchar *router_friendly_name,
                          const gchar *router_conf_url,
                          const gchar *router_brand,
                          const gchar *router_brand_website,
                          const gchar *router_model_description,
                          const gchar *router_model_name,
                          const gchar *router_model_number);

void gui_set_ext_ip(const gchar *ip);

void gui_disable_ext_ip();

void gui_set_conn_status(const gchar *state);

void gui_update_graph();

void gui_disable_conn_status();

void gui_disable_total_received (void);

void gui_disable_total_sent (void);

void gui_set_total_received (unsigned int total_received);

void gui_set_total_sent (unsigned int total_sent);

void gui_disable_download_speed(void);

void gui_disable_upload_speed(void);

void gui_set_download_speed(const gdouble down_speed);

void gui_set_upload_speed(const gdouble up_speed);

void gui_add_mapped_port(const PortForwardInfo* port_info);

void gui_clear_ports_list_treeview(void);

void gui_set_router_icon(gchar *image_path);

#endif /* __URC_GUI_H__ */
