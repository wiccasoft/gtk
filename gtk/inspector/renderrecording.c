/*
 * Copyright (c) 2016 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include "renderrecording.h"

G_DEFINE_TYPE (GtkInspectorRenderRecording, gtk_inspector_render_recording, GTK_TYPE_INSPECTOR_RECORDING)

static void
gtk_inspector_render_recording_finalize (GObject *object)
{
  GtkInspectorRenderRecording *recording = GTK_INSPECTOR_RENDER_RECORDING (object);

  g_clear_pointer (&recording->clip, cairo_region_destroy);
  g_clear_pointer (&recording->node, gsk_render_node_unref);

  G_OBJECT_CLASS (gtk_inspector_render_recording_parent_class)->finalize (object);
}

static void
gtk_inspector_render_recording_class_init (GtkInspectorRenderRecordingClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gtk_inspector_render_recording_finalize;
}

static void
gtk_inspector_render_recording_init (GtkInspectorRenderRecording *vis)
{
}

GtkInspectorRecording *
gtk_inspector_render_recording_new (gint64                timestamp,
                                    const GdkRectangle   *area,
                                    const cairo_region_t *clip,
                                    GskRenderNode        *node)
{
  GtkInspectorRenderRecording *recording;

  recording = g_object_new (GTK_TYPE_INSPECTOR_RENDER_RECORDING,
                            "timestamp", timestamp,
                            NULL);

  recording->area = *area;
  recording->clip = cairo_region_copy (clip);
  recording->node = gsk_render_node_ref (node);

  return GTK_INSPECTOR_RECORDING (recording);
}

// vim: set et sw=2 ts=2: