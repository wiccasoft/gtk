/* GTK - The GIMP Toolkit
 * Copyright (C) 2017 Benjamin Otte <otte@gnome.org>
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

#ifndef __GTK_FISHBOWL_H__
#define __GTK_FISHBOWL_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTK_TYPE_FISHBOWL                  (gtk_fishbowl_get_type ())
#define GTK_FISHBOWL(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_FISHBOWL, GtkFishbowl))
#define GTK_FISHBOWL_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_FISHBOWL, GtkFishbowlClass))
#define GTK_IS_FISHBOWL(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_FISHBOWL))
#define GTK_IS_FISHBOWL_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_FISHBOWL))
#define GTK_FISHBOWL_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_FISHBOWL, GtkFishbowlClass))

typedef struct _GtkFishbowl              GtkFishbowl;
typedef struct _GtkFishbowlClass         GtkFishbowlClass;

struct _GtkFishbowl
{
  GtkContainer container;
};

struct _GtkFishbowlClass
{
  GtkContainerClass parent_class;
};

GType      gtk_fishbowl_get_type          (void) G_GNUC_CONST;

GtkWidget* gtk_fishbowl_new               (void);

void       gtk_fishbowl_set_use_icons     (GtkFishbowl       *fishbowl,
                                           gboolean           use_icons);

guint      gtk_fishbowl_get_count         (GtkFishbowl       *fishbowl);
void       gtk_fishbowl_set_count         (GtkFishbowl       *fishbowl,
                                           guint              count);
gboolean   gtk_fishbowl_get_animating     (GtkFishbowl       *fishbowl);
void       gtk_fishbowl_set_animating     (GtkFishbowl       *fishbowl,
                                           gboolean           animating);

G_END_DECLS

#endif /* __GTK_FISHBOWL_H__ */
