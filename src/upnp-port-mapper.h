/* upnp-port-mapper.h
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


#include <gtk/gtk.h>
#include <libgupnp/gupnp-control-point.h>

struct router_struct
{
    GUPnPDeviceInfo *main_device;
    gchar* friendly_name;
    gchar* vendor;
    gchar* model;
    gchar* http_address;
    guint PortMappingNumberOfEntries;
    
    gchar* external_ip;
    
    gboolean connected;
    
    GUPnPServiceProxy *wan_service;
    GUPnPServiceProxy *wan_device;
};

typedef struct router_struct router_info;

enum Protocols
{
    TCP, UDP
};

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

struct PortMap
{
    gchar remote_host;
    guint external_port;
    
    gchar internal_host;    
    guint internal_port;
    
    gchar description;
    
    uint protocol;
    
    uint lease_time;
    
};

struct ui_struct
{
    GtkWidget *main_window,
              *treeview,
              *router_name_label,
              *router_url_eventbox,
              *router_url_label,
              *wan_status_label,
              *ip_label,
              *down_rate_label,
              *up_rate_label;
};

typedef struct ui_struct ui_context;
