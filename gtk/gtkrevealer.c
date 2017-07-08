/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * Copyright 2013, 2015 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Alexander Larsson <alexl@redhat.com>
 *         Carlos Soriano <csoriano@gnome.org>
 */

#include "config.h"

#include "gtkrevealer.h"

#include "gtkintl.h"
#include "gtkprivate.h"
#include "gtkprogresstrackerprivate.h"
#include "gtksettingsprivate.h"
#include "gtksnapshot.h"
#include "gtktypebuiltins.h"
#include "gtkwidgetprivate.h"

#include "fallback-c89.c"

/**
 * SECTION:gtkrevealer
 * @Short_description: Hide and show with animation
 * @Title: GtkRevealer
 * @See_also: #GtkExpander
 *
 * The GtkRevealer widget is a container which animates
 * the transition of its child from invisible to visible.
 *
 * The style of transition can be controlled with
 * gtk_revealer_set_transition_type().
 *
 * These animations respect the #GtkSettings:gtk-enable-animations
 * setting.
 *
 * # CSS nodes
 *
 * GtkRevealer has a single CSS node with name revealer.
 *
 * The GtkRevealer widget was added in GTK+ 3.10.
 */

/**
 * GtkRevealerTransitionType:
 * @GTK_REVEALER_TRANSITION_TYPE_NONE: No transition
 * @GTK_REVEALER_TRANSITION_TYPE_CROSSFADE: Fade in
 * @GTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT: Slide in from the left
 * @GTK_REVEALER_TRANSITION_TYPE_SLIDE_LEFT: Slide in from the right
 * @GTK_REVEALER_TRANSITION_TYPE_SLIDE_UP: Slide in from the bottom
 * @GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN: Slide in from the top
 *
 * These enumeration values describe the possible transitions
 * when the child of a #GtkRevealer widget is shown or hidden.
 */

enum  {
  PROP_0,
  PROP_TRANSITION_TYPE,
  PROP_TRANSITION_DURATION,
  PROP_REVEAL_CHILD,
  PROP_CHILD_REVEALED,
  LAST_PROP
};

typedef struct {
  GtkRevealerTransitionType transition_type;
  guint transition_duration;

  GdkWindow* bin_window;

  gdouble current_pos;
  gdouble source_pos;
  gdouble target_pos;

  guint tick_id;
  GtkProgressTracker tracker;
} GtkRevealerPrivate;

static GParamSpec *props[LAST_PROP] = { NULL, };

static void     gtk_revealer_real_realize                        (GtkWidget     *widget);
static void     gtk_revealer_real_unrealize                      (GtkWidget     *widget);
static void     gtk_revealer_real_add                            (GtkContainer  *widget,
                                                                  GtkWidget     *child);
static void     gtk_revealer_real_size_allocate                  (GtkWidget     *widget,
                                                                  GtkAllocation *allocation);
static void gtk_revealer_measure (GtkWidget      *widget,
                                  GtkOrientation  orientation,
                                  int             for_size,
                                  int            *minimum,
                                  int            *natural,
                                  int            *minimum_baseline,
                                  int            *natural_baseline);
static void     gtk_revealer_snapshot                            (GtkWidget     *widget,
                                                                  GtkSnapshot   *snapshot);

G_DEFINE_TYPE_WITH_PRIVATE (GtkRevealer, gtk_revealer, GTK_TYPE_BIN)

static void
gtk_revealer_init (GtkRevealer *revealer)
{
  GtkRevealerPrivate *priv = gtk_revealer_get_instance_private (revealer);

  priv->transition_type = GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN;
  priv->transition_duration = 250;
  priv->current_pos = 0.0;
  priv->target_pos = 0.0;

  gtk_widget_set_has_window ((GtkWidget*) revealer, FALSE);
  gtk_widget_set_redraw_on_allocate ((GtkWidget*) revealer, FALSE);
}

static void
gtk_revealer_finalize (GObject *obj)
{
  GtkRevealer *revealer = GTK_REVEALER (obj);
  GtkRevealerPrivate *priv = gtk_revealer_get_instance_private (revealer);

  if (priv->tick_id != 0)
    gtk_widget_remove_tick_callback (GTK_WIDGET (revealer), priv->tick_id);
  priv->tick_id = 0;

  G_OBJECT_CLASS (gtk_revealer_parent_class)->finalize (obj);
}

static void
gtk_revealer_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  GtkRevealer *revealer = GTK_REVEALER (object);

  switch (property_id)
   {
    case PROP_TRANSITION_TYPE:
      g_value_set_enum (value, gtk_revealer_get_transition_type (revealer));
      break;
    case PROP_TRANSITION_DURATION:
      g_value_set_uint (value, gtk_revealer_get_transition_duration (revealer));
      break;
    case PROP_REVEAL_CHILD:
      g_value_set_boolean (value, gtk_revealer_get_reveal_child (revealer));
      break;
    case PROP_CHILD_REVEALED:
      g_value_set_boolean (value, gtk_revealer_get_child_revealed (revealer));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gtk_revealer_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  GtkRevealer *revealer = GTK_REVEALER (object);

  switch (property_id)
    {
    case PROP_TRANSITION_TYPE:
      gtk_revealer_set_transition_type (revealer, g_value_get_enum (value));
      break;
    case PROP_TRANSITION_DURATION:
      gtk_revealer_set_transition_duration (revealer, g_value_get_uint (value));
      break;
    case PROP_REVEAL_CHILD:
      gtk_revealer_set_reveal_child (revealer, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gtk_revealer_unmap (GtkWidget *widget)
{
  GtkRevealer *revealer = GTK_REVEALER (widget);
  GtkRevealerPrivate *priv = gtk_revealer_get_instance_private (revealer);

  GTK_WIDGET_CLASS (gtk_revealer_parent_class)->unmap (widget);

  /* Finish & stop the animation */
  if (priv->current_pos != priv->target_pos)
    {
      priv->current_pos = priv->target_pos;
      g_object_notify_by_pspec (G_OBJECT (revealer), props[PROP_CHILD_REVEALED]);
    }
  if (priv->tick_id != 0)
    {
      gtk_widget_remove_tick_callback (GTK_WIDGET (revealer), priv->tick_id);
      priv->tick_id = 0;
    }
}

static void
gtk_revealer_class_init (GtkRevealerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

  object_class->get_property = gtk_revealer_get_property;
  object_class->set_property = gtk_revealer_set_property;
  object_class->finalize = gtk_revealer_finalize;

  widget_class->realize = gtk_revealer_real_realize;
  widget_class->unrealize = gtk_revealer_real_unrealize;
  widget_class->unmap = gtk_revealer_unmap;
  widget_class->size_allocate = gtk_revealer_real_size_allocate;
  widget_class->measure = gtk_revealer_measure;
  widget_class->snapshot = gtk_revealer_snapshot;

  container_class->add = gtk_revealer_real_add;

  props[PROP_TRANSITION_TYPE] =
    g_param_spec_enum ("transition-type",
                       P_("Transition type"),
                       P_("The type of animation used to transition"),
                       GTK_TYPE_REVEALER_TRANSITION_TYPE,
                       GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN,
                       GTK_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_TRANSITION_DURATION] =
    g_param_spec_uint ("transition-duration",
                       P_("Transition duration"),
                       P_("The animation duration, in milliseconds"),
                       0, G_MAXUINT, 250,
                       GTK_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_REVEAL_CHILD] =
    g_param_spec_boolean ("reveal-child",
                          P_("Reveal Child"),
                          P_("Whether the container should reveal the child"),
                          FALSE,
                          GTK_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_CHILD_REVEALED] =
    g_param_spec_boolean ("child-revealed",
                          P_("Child Revealed"),
                          P_("Whether the child is revealed and the animation target reached"),
                          FALSE,
                          G_PARAM_READABLE);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  gtk_widget_class_set_css_name (widget_class, "revealer");
}

/**
 * gtk_revealer_new:
 *
 * Creates a new #GtkRevealer.
 *
 * Returns: a newly created #GtkRevealer
 *
 * Since: 3.10
 */
GtkWidget *
gtk_revealer_new (void)
{
  return g_object_new (GTK_TYPE_REVEALER, NULL);
}

static GtkRevealerTransitionType
effective_transition (GtkRevealer *revealer)
{
  GtkRevealerPrivate *priv = gtk_revealer_get_instance_private (revealer);

  if (gtk_widget_get_direction (GTK_WIDGET (revealer)) == GTK_TEXT_DIR_RTL)
    {
      if (priv->transition_type == GTK_REVEALER_TRANSITION_TYPE_SLIDE_LEFT)
        return GTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT;
      else if (priv->transition_type == GTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT)
        return GTK_REVEALER_TRANSITION_TYPE_SLIDE_LEFT;
    }

  return priv->transition_type;
}

static void
gtk_revealer_get_child_allocation (GtkRevealer   *revealer,
                                   GtkAllocation *allocation,
                                   GtkAllocation *child_allocation)
{
  GtkWidget *child;
  GtkRevealerTransitionType transition;

  g_return_if_fail (revealer != NULL);
  g_return_if_fail (allocation != NULL);

  child_allocation->x = 0;
  child_allocation->y = 0;
  child_allocation->width = 0;
  child_allocation->height = 0;

  child = gtk_bin_get_child (GTK_BIN (revealer));
  if (child != NULL && gtk_widget_get_visible (child))
    {
      transition = effective_transition (revealer);
      if (transition == GTK_REVEALER_TRANSITION_TYPE_SLIDE_LEFT ||
          transition == GTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT)
        gtk_widget_measure (child, GTK_ORIENTATION_HORIZONTAL,
                            MAX (0, allocation->height),
                            NULL, &child_allocation->width, NULL, NULL);
      else
        gtk_widget_measure (child, GTK_ORIENTATION_VERTICAL,
                            MAX (0, allocation->width),
                            NULL, &child_allocation->height, NULL, NULL);
    }

  child_allocation->width = MAX (child_allocation->width, allocation->width);
  child_allocation->height = MAX (child_allocation->height, allocation->height);
}

static void
gtk_revealer_real_realize (GtkWidget *widget)
{
  GtkRevealer *revealer = GTK_REVEALER (widget);
  GtkRevealerPrivate *priv = gtk_revealer_get_instance_private (revealer);
  GtkAllocation allocation;
  GtkAllocation child_allocation;
  GtkWidget *child;
  GtkRevealerTransitionType transition;

  GTK_WIDGET_CLASS (gtk_revealer_parent_class)->realize (widget);

  gtk_widget_get_allocation (widget, &allocation);

  gtk_revealer_get_child_allocation (revealer, &allocation, &child_allocation);

  /* See explanation on gtk_revealer_real_size_allocate */
  transition = effective_transition (revealer);
  if (transition == GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN)
    {
      child_allocation.y = allocation.height - child_allocation.height;
      child_allocation.x = 0;
    }
  else if (transition == GTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT)
    {
      child_allocation.y = 0;
      child_allocation.x = allocation.width - child_allocation.width;
    }
 else
   {
     child_allocation.y = 0;
     child_allocation.x = 0;
   }

  priv->bin_window =
    gdk_window_new_child (gtk_widget_get_window (widget),
                          GDK_ALL_EVENTS_MASK,
                          &child_allocation);
  gtk_widget_register_window (widget, priv->bin_window);

  child = gtk_bin_get_child (GTK_BIN (revealer));
  if (child != NULL)
    gtk_widget_set_parent_window (child, priv->bin_window);

  gdk_window_show (priv->bin_window);
}

static void
gtk_revealer_real_unrealize (GtkWidget *widget)
{
  GtkRevealer *revealer = GTK_REVEALER (widget);
  GtkRevealerPrivate *priv = gtk_revealer_get_instance_private (revealer);

  gtk_widget_unregister_window (widget, priv->bin_window);
  gdk_window_destroy (priv->bin_window);
  priv->bin_window = NULL;

  GTK_WIDGET_CLASS (gtk_revealer_parent_class)->unrealize (widget);
}

static void
gtk_revealer_real_add (GtkContainer *container,
                       GtkWidget    *child)
{
  GtkRevealer *revealer = GTK_REVEALER (container);
  GtkRevealerPrivate *priv = gtk_revealer_get_instance_private (revealer);

  g_return_if_fail (child != NULL);

  gtk_widget_set_parent_window (child, priv->bin_window);
  gtk_widget_set_child_visible (child, priv->current_pos != 0.0);

  GTK_CONTAINER_CLASS (gtk_revealer_parent_class)->add (container, child);
}

static void
gtk_revealer_real_size_allocate (GtkWidget     *widget,
                                 GtkAllocation *allocation)
{
  GtkRevealer *revealer = GTK_REVEALER (widget);
  GtkRevealerPrivate *priv = gtk_revealer_get_instance_private (revealer);
  GtkAllocation child_allocation;
  GtkWidget *child;
  int bin_x, bin_y;
  GtkRevealerTransitionType transition;
  GtkAllocation clip = *allocation;

  g_return_if_fail (allocation != NULL);

  gtk_revealer_get_child_allocation (revealer, allocation, &child_allocation);

  child = gtk_bin_get_child (GTK_BIN (revealer));
  if (child != NULL && gtk_widget_get_visible (child))
    {
      gtk_widget_size_allocate (child, &child_allocation);
      gtk_widget_get_clip (child, &clip);
    }

  if (gtk_widget_get_realized (widget))
    {
      bin_x = 0;
      bin_y = 0;

      transition = effective_transition (revealer);
      /* The child allocation is fixed (it is not modified by the animation),
       * and it's origin is relative to the bin_window.
       * The bin_window has the same allocation as the child, and then the bin_window
       * deals with the relative positioning with respect to the revealer taking
       * into account the paddings of the revealer.
       *
       * For most of transitions, the bin_window moves along with the revealer,
       * as its allocation changes.
       * However for GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN
       * we need to first move the bin_window upwards and then slide it down in
       * the revealer.
       * Otherwise the child would appear as static and the revealer will allocate
       * following the animation, clipping the child.
       * To calculate the correct y position for this case:
       * allocation->height - child_allocation.height is the relative position
       * towards the revealer taking into account the animation progress with
       * both vertical paddings added, therefore we need to substract the part
       * that we don't want to take into account for the y position, which
       * in this case is the bottom padding.
       *
       * The same special treatment is needed for GTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT.
       */
      if (transition == GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN)
        {
          bin_y = allocation->height - child_allocation.height;
          bin_x = 0;
        }
      else if (transition == GTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT)
        {
          bin_y = 0;
          bin_x = allocation->width - child_allocation.width;
        }

      gdk_window_move_resize (priv->bin_window,
                              bin_x + allocation->x,
                              bin_y + allocation->y,
                              child_allocation.width, child_allocation.height);
    }

  gtk_widget_set_clip (widget, &clip);
}

static void
gtk_revealer_set_position (GtkRevealer *revealer,
                           gdouble      pos)
{
  GtkRevealerPrivate *priv = gtk_revealer_get_instance_private (revealer);
  gboolean new_visible;
  GtkWidget *child;
  GtkRevealerTransitionType transition;

  priv->current_pos = pos;

  /* We check target_pos here too, because we want to ensure we set
   * child_visible immediately when starting a reveal operation
   * otherwise the child widgets will not be properly realized
   * after the reveal returns.
   */
  new_visible = priv->current_pos != 0.0 || priv->target_pos != 0.0;

  child = gtk_bin_get_child (GTK_BIN (revealer));
  if (child != NULL &&
      new_visible != gtk_widget_get_child_visible (child))
    gtk_widget_set_child_visible (child, new_visible);

  transition = effective_transition (revealer);
  if (transition == GTK_REVEALER_TRANSITION_TYPE_CROSSFADE)
    {
      gtk_widget_set_opacity (GTK_WIDGET (revealer), priv->current_pos);
      gtk_widget_queue_draw (GTK_WIDGET (revealer));
    }
  else
    {
      gtk_widget_queue_resize (GTK_WIDGET (revealer));
    }

  if (priv->current_pos == priv->target_pos)
    g_object_notify_by_pspec (G_OBJECT (revealer), props[PROP_CHILD_REVEALED]);
}

static gboolean
gtk_revealer_animate_cb (GtkWidget     *widget,
                         GdkFrameClock *frame_clock,
                         gpointer       user_data)
{
  GtkRevealer *revealer = GTK_REVEALER (widget);
  GtkRevealerPrivate *priv = gtk_revealer_get_instance_private (revealer);
  gdouble ease;

  gtk_progress_tracker_advance_frame (&priv->tracker,
                                      gdk_frame_clock_get_frame_time (frame_clock));
  ease = gtk_progress_tracker_get_ease_out_cubic (&priv->tracker, FALSE);
  gtk_revealer_set_position (revealer,
                             priv->source_pos + (ease * (priv->target_pos - priv->source_pos)));

  if (gtk_progress_tracker_get_state (&priv->tracker) == GTK_PROGRESS_STATE_AFTER)
    {
      priv->tick_id = 0;
      return FALSE;
    }

  return TRUE;
}

static void
gtk_revealer_start_animation (GtkRevealer *revealer,
                              gdouble      target)
{
  GtkRevealerPrivate *priv = gtk_revealer_get_instance_private (revealer);
  GtkWidget *widget = GTK_WIDGET (revealer);
  GtkRevealerTransitionType transition;

  if (priv->target_pos == target)
    return;

  priv->target_pos = target;
  g_object_notify_by_pspec (G_OBJECT (revealer), props[PROP_REVEAL_CHILD]);

  transition = effective_transition (revealer);
  if (gtk_widget_get_mapped (widget) &&
      priv->transition_duration != 0 &&
      transition != GTK_REVEALER_TRANSITION_TYPE_NONE &&
      gtk_settings_get_enable_animations (gtk_widget_get_settings (widget)))
    {
      priv->source_pos = priv->current_pos;
      if (priv->tick_id == 0)
        priv->tick_id =
          gtk_widget_add_tick_callback (widget, gtk_revealer_animate_cb, revealer, NULL);
      gtk_progress_tracker_start (&priv->tracker,
                                  priv->transition_duration * 1000,
                                  0,
                                  1.0);
    }
  else
    {
      gtk_revealer_set_position (revealer, target);
    }
}

/**
 * gtk_revealer_set_reveal_child:
 * @revealer: a #GtkRevealer
 * @reveal_child: %TRUE to reveal the child
 *
 * Tells the #GtkRevealer to reveal or conceal its child.
 *
 * The transition will be animated with the current
 * transition type of @revealer.
 *
 * Since: 3.10
 */
void
gtk_revealer_set_reveal_child (GtkRevealer *revealer,
                               gboolean     reveal_child)
{
  g_return_if_fail (GTK_IS_REVEALER (revealer));

  if (reveal_child)
    gtk_revealer_start_animation (revealer, 1.0);
  else
    gtk_revealer_start_animation (revealer, 0.0);
}

/**
 * gtk_revealer_get_reveal_child:
 * @revealer: a #GtkRevealer
 *
 * Returns whether the child is currently
 * revealed. See gtk_revealer_set_reveal_child().
 *
 * This function returns %TRUE as soon as the transition
 * is to the revealed state is started. To learn whether
 * the child is fully revealed (ie the transition is completed),
 * use gtk_revealer_get_child_revealed().
 *
 * Returns: %TRUE if the child is revealed.
 *
 * Since: 3.10
 */
gboolean
gtk_revealer_get_reveal_child (GtkRevealer *revealer)
{
  GtkRevealerPrivate *priv = gtk_revealer_get_instance_private (revealer);

  g_return_val_if_fail (GTK_IS_REVEALER (revealer), FALSE);

  return priv->target_pos != 0.0;
}

/**
 * gtk_revealer_get_child_revealed:
 * @revealer: a #GtkRevealer
 *
 * Returns whether the child is fully revealed, in other words whether
 * the transition to the revealed state is completed.
 *
 * Returns: %TRUE if the child is fully revealed
 *
 * Since: 3.10
 */
gboolean
gtk_revealer_get_child_revealed (GtkRevealer *revealer)
{
  GtkRevealerPrivate *priv = gtk_revealer_get_instance_private (revealer);
  gboolean animation_finished = (priv->target_pos == priv->current_pos);
  gboolean reveal_child = gtk_revealer_get_reveal_child (revealer);

  if (animation_finished)
    return reveal_child;
  else
    return !reveal_child;
}

/* These all report only the natural size, ignoring the minimal size,
 * because its not really possible to allocate the right size during
 * animation if the child size can change (without the child
 * re-arranging itself during the animation).
 */

static void
set_height (GtkRevealer *revealer,
            gint        *minimum_height,
            gint        *natural_height)
{
  GtkRevealerPrivate *priv = gtk_revealer_get_instance_private (revealer);
  GtkRevealerTransitionType transition;

  transition = effective_transition (revealer);
  if (transition == GTK_REVEALER_TRANSITION_TYPE_NONE ||
      transition == GTK_REVEALER_TRANSITION_TYPE_SLIDE_UP ||
      transition == GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN)
    {
      *minimum_height = round (*minimum_height * priv->current_pos);
      *natural_height = round (*natural_height * priv->current_pos);
    }
}

static void
set_width (GtkRevealer *revealer,
           gint        *minimum_width,
           gint        *natural_width)
{
  GtkRevealerPrivate *priv = gtk_revealer_get_instance_private (revealer);
  GtkRevealerTransitionType transition;

  transition = effective_transition (revealer);
  if (transition == GTK_REVEALER_TRANSITION_TYPE_NONE ||
      transition == GTK_REVEALER_TRANSITION_TYPE_SLIDE_LEFT ||
      transition == GTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT)
    {
      *minimum_width = round (*minimum_width * priv->current_pos);
      *natural_width = round (*natural_width * priv->current_pos);
    }
}

static void
gtk_revealer_measure (GtkWidget      *widget,
                      GtkOrientation  orientation,
                      int             for_size,
                      int            *minimum,
                      int            *natural,
                      int            *minimum_baseline,
                      int            *natural_baseline)
{
  GTK_WIDGET_CLASS (gtk_revealer_parent_class)->measure (widget,
                                                         orientation,
                                                         for_size,
                                                         minimum, natural,
                                                         NULL, NULL);
  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    set_width (GTK_REVEALER (widget), minimum, natural);
  else
    set_height (GTK_REVEALER (widget), minimum, natural);
}

static void
gtk_revealer_snapshot (GtkWidget   *widget,
                       GtkSnapshot *snapshot)
{
  GtkRevealer *revealer = GTK_REVEALER (widget);
  GtkRevealerTransitionType transition;
  GtkWidget *child;

  child = gtk_bin_get_child (GTK_BIN (revealer));
  if (child == NULL || !gtk_widget_get_mapped (child))
    return;

  transition = effective_transition (revealer);
  if (transition == GTK_REVEALER_TRANSITION_TYPE_NONE ||
      transition == GTK_REVEALER_TRANSITION_TYPE_CROSSFADE)
    {
      gtk_widget_snapshot_child (widget, child, snapshot);
    }
  else
    {
      gtk_snapshot_push_clip (snapshot,
                              &GRAPHENE_RECT_INIT(
                                  0, 0,
                                  gtk_widget_get_allocated_width (widget),
                                  gtk_widget_get_allocated_height (widget)
                              ),
                              "RevealerClip");
      gtk_widget_snapshot_child (widget, child, snapshot);
      gtk_snapshot_pop (snapshot);
    }
}

/**
 * gtk_revealer_get_transition_duration:
 * @revealer: a #GtkRevealer
 *
 * Returns the amount of time (in milliseconds) that
 * transitions will take.
 *
 * Returns: the transition duration
 *
 * Since: 3.10
 */
guint
gtk_revealer_get_transition_duration (GtkRevealer *revealer)
{
  GtkRevealerPrivate *priv = gtk_revealer_get_instance_private (revealer);

  g_return_val_if_fail (GTK_IS_REVEALER (revealer), 0);

  return priv->transition_duration;
}

/**
 * gtk_revealer_set_transition_duration:
 * @revealer: a #GtkRevealer
 * @duration: the new duration, in milliseconds
 *
 * Sets the duration that transitions will take.
 *
 * Since: 3.10
 */
void
gtk_revealer_set_transition_duration (GtkRevealer *revealer,
                                      guint        value)
{
  GtkRevealerPrivate *priv = gtk_revealer_get_instance_private (revealer);

  g_return_if_fail (GTK_IS_REVEALER (revealer));

  if (priv->transition_duration == value)
    return;

  priv->transition_duration = value;
  g_object_notify_by_pspec (G_OBJECT (revealer), props[PROP_TRANSITION_DURATION]);
}

/**
 * gtk_revealer_get_transition_type:
 * @revealer: a #GtkRevealer
 *
 * Gets the type of animation that will be used
 * for transitions in @revealer.
 *
 * Returns: the current transition type of @revealer
 *
 * Since: 3.10
 */
GtkRevealerTransitionType
gtk_revealer_get_transition_type (GtkRevealer *revealer)
{
  GtkRevealerPrivate *priv = gtk_revealer_get_instance_private (revealer);

  g_return_val_if_fail (GTK_IS_REVEALER (revealer), GTK_REVEALER_TRANSITION_TYPE_NONE);

  return priv->transition_type;
}

/**
 * gtk_revealer_set_transition_type:
 * @revealer: a #GtkRevealer
 * @transition: the new transition type
 *
 * Sets the type of animation that will be used for
 * transitions in @revealer. Available types include
 * various kinds of fades and slides.
 *
 * Since: 3.10
 */
void
gtk_revealer_set_transition_type (GtkRevealer               *revealer,
                                  GtkRevealerTransitionType  transition)
{
  GtkRevealerPrivate *priv = gtk_revealer_get_instance_private (revealer);

  g_return_if_fail (GTK_IS_REVEALER (revealer));

  if (priv->transition_type == transition)
    return;

  priv->transition_type = transition;
  gtk_widget_queue_resize (GTK_WIDGET (revealer));
  g_object_notify_by_pspec (G_OBJECT (revealer), props[PROP_TRANSITION_TYPE]);
}
