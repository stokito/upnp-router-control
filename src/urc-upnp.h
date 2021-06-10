/* urc-upnp.h
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

#ifndef __URC_UPNP_H__
#define __URC_UPNP_H__


#include <glib.h>
#include <libgupnp/gupnp-control-point.h>

typedef struct
{
    gboolean enabled;
    gchar*   description;
    gchar*   protocol;
    guint    internal_port;
    guint    external_port;
    gchar*   internal_host;
    gchar*   remote_host;
    guint    lease_time;

} PortForwardInfo;

typedef struct
{
    GUPnPDeviceInfo *main_device;
    gchar* friendly_name;
    gchar* brand;
    gchar* brand_website;
    gchar* model_description;
    gchar* model_name;
    gchar* model_number;
    gchar* http_address;
    gchar* upc;
    const gchar* udn;

    gchar* external_ip;

    gboolean rsip_available;
    gboolean nat_enabled;
    gboolean connected;

    /* no-event refresh timer */
    guint refresh_timeout;

    guint data_rate_timer;

    GUPnPServiceProxy *wan_conn_service;
    GUPnPServiceProxy *wan_common_ifc;

} RouterInfo;

/* Functions */
const gchar*
get_client_ip();

gboolean
upnp_init();

gboolean
delete_port_mapped(GUPnPServiceProxy *wan_service, const gchar *protocol, const guint external_port, const gchar *remote_host, GError **error);

gboolean
add_port_mapping(GUPnPServiceProxy *wan_service, PortForwardInfo *port_info, GError **error);

void
urc_upnp_refresh_data (RouterInfo *router);

#endif /* __URC_UPNP_H__ */
