/* urc-graph.h
 *
 * Copyright 2021 Daniele Napolitano <dnax88@gmail.com>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef __URC_GRAPH_H__
#define __URC_GRAPH_H__

#include <gtk/gtk.h>

typedef struct
{
    gdouble  speed;
    gboolean valid;
} SpeedValue;

void
urc_init_network_graph(GtkWidget *drawing_area);

void
update_download_graph_data(SpeedValue *speed);

void
update_upload_graph_data(SpeedValue *speed);

void
urc_disable_graph();

void
urc_enable_graph();

void
urc_graph_set_receiving_color(GdkRGBA color);

void
urc_graph_set_sending_color(GdkRGBA color);

#endif /* __URC_GRAPH_H__ */
