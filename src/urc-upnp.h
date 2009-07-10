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

gboolean upnp_init(const char *host_ip, const gboolean debug);

void delete_port_mapped(GUPnPServiceProxy *wan_service, const gchar *protocol, const guint external_port, const gchar *remote_host);

#endif /* __URC_UPNP_H__ */