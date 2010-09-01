/* urc-graph.h
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

typedef struct
{
    gdouble  speed;
    gboolean valid;
} SpeedValue;

void
init_graph();

void
update_download_graph_data(SpeedValue *speed);

void
update_upload_graph_data(SpeedValue *speed);

gboolean
on_drawing_area_configure_event (GtkWidget         *widget,
		                         GdkEventConfigure *event,
		                         gpointer           data_ptr);

gboolean
on_drawing_area_expose_event (GtkWidget      *widget,
                              GdkEventExpose *event,
                              gpointer        user_data);
