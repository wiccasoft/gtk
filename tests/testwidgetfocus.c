#include <gtk/gtk.h>



typedef struct _GtkFocusWidget      GtkFocusWidget;
typedef struct _GtkFocusWidgetClass GtkFocusWidgetClass;

#define GTK_TYPE_FOCUS_WIDGET           (gtk_focus_widget_get_type ())
#define GTK_FOCUS_WIDGET(obj)           (G_TYPE_CHECK_INSTANCE_CAST(obj, GTK_TYPE_FOCUS_WIDGET, GtkFocusWidget))
#define GTK_FOCUS_WIDGET_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST(cls, GTK_TYPE_FOCUS_WIDGET, GtkFocusWidgetClass))
#define GTK_IS_FOCUS_WIDGET(obj)        (G_TYPE_CHECK_INSTANCE_TYPE(obj, GTK_TYPE_FOCUS_WIDGET))
#define GTK_IS_FOCUS_WIDGET_CLASS(cls)   (G_TYPE_CHECK_CLASS_TYPE(cls, GTK_TYPE_FOCUS_WIDGET))
#define GTK_FOCUS_WIDGET_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS(obj, GTK_TYPE_FOCUS_WIDGET, GtkFocusWidgetClass))

const char *css =
"* {"
"  transition: none; "
"}"
"focuswidget {"
"  padding: 30px;"
"  font-size: 70%;"
"}"
"focuswidget button:nth-child(1) {"
"  margin-right: 15px;"
"  margin-bottom: 15px;"
"}"
"focuswidget button:nth-child(2) {"
"  margin-left: 15px;"
"  margin-bottom: 15px;"
"}"
"focuswidget button:nth-child(3) {"
"  margin-right: 15px;"
"  margin-top: 15px;"
"}"
"focuswidget button:nth-child(4) {"
"  margin-left: 15px;"
"  margin-top: 15px;"
"}"
"focuswidget button {"
"  min-width: 80px;"
"  min-height: 80px;"
"  margin: 0px;"
"  border: 5px solid green;"
"  border-radius: 0px;"
"  padding: 10px;"
"  background-image: none;"
"  background-color: white;"
"  box-shadow: none;"
"}"
"focuswidget button:hover {"
"  background-color: black;"
"  color: white;"
"}"
"focuswidget button label:hover {"
"  background-color: green;"
"}"
;

struct _GtkFocusWidget
{
  GtkWidget parent_instance;
  int mouse_x;
  int mouse_y;

  union {
    struct {
      GtkWidget *child1;
      GtkWidget *child2;
      GtkWidget *child3;
      GtkWidget *child4;
    };
    GtkWidget* children[4];
  };
};

struct _GtkFocusWidgetClass
{
  GtkWidgetClass parent_class;
};

GType gtk_focus_widget_get_type (void) G_GNUC_CONST;


G_DEFINE_TYPE(GtkFocusWidget, gtk_focus_widget, GTK_TYPE_WIDGET)

static void
gtk_focus_widget_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
  GtkFocusWidget *self = GTK_FOCUS_WIDGET (widget);
  int child_width  = (allocation->width)  / 2;
  int child_height = (allocation->height) / 2;
  GtkAllocation child_alloc;

  child_alloc.x = 0;
  child_alloc.y = 0;
  child_alloc.width = child_width;
  child_alloc.height = child_height;

  gtk_widget_size_allocate (self->child1, &child_alloc);

  child_alloc.x += child_width;

  gtk_widget_size_allocate (self->child2, &child_alloc);

  child_alloc.y += child_height;

  gtk_widget_size_allocate (self->child4, &child_alloc);

  child_alloc.x -= child_width;

  gtk_widget_size_allocate (self->child3, &child_alloc);
}

static void
gtk_focus_widget_measure (GtkWidget      *widget,
                          GtkOrientation  orientation,
                          int             for_size,
                          int            *minimum,
                          int            *natural,
                          int            *minimum_baseline,
                          int            *natural_baseline)
{
  GtkFocusWidget *self = GTK_FOCUS_WIDGET (widget);
  int min, nat;
  int i;

  *minimum = 0;
  *natural = 0;

  for (i = 0; i < 4; i ++)
    {
      gtk_widget_measure (self->children[i], orientation, for_size,
                          &min, &nat, NULL, NULL);

      *minimum = MAX (*minimum, min);
      *natural = MAX (*natural, nat);
    }

  *minimum *= 2;
  *natural *= 2;
}

static void
gtk_focus_widget_snapshot (GtkWidget *widget, GtkSnapshot *snapshot)
{
  GtkFocusWidget *self = GTK_FOCUS_WIDGET (widget);

  gtk_widget_snapshot_child (widget, self->child1, snapshot);
  gtk_widget_snapshot_child (widget, self->child2, snapshot);
  gtk_widget_snapshot_child (widget, self->child3, snapshot);
  gtk_widget_snapshot_child (widget, self->child4, snapshot);

  if (self->mouse_x != G_MININT && self->mouse_y != G_MININT)
    {
      PangoLayout *layout;
      char *text;
      GtkAllocation alloc;
      graphene_rect_t bounds;
      GdkRGBA black = {0, 0, 0, 1};

      gtk_widget_get_allocation (widget, &alloc);

      /* Since event coordinates and drawing is supposed to happen in the
       * same coodinates space, this should all work out just fine. */
      bounds.origin.x = self->mouse_x;
      bounds.origin.y = -30;
      bounds.size.width = 1;
      bounds.size.height = alloc.height;
      gtk_snapshot_append_color (snapshot,
                                 &black,
                                 &bounds,
                                 "Crosshair 1");

      bounds.origin.x = -30;
      bounds.origin.y = self->mouse_y;
      bounds.size.width = alloc.width;
      bounds.size.height = 1;
      gtk_snapshot_append_color (snapshot,
                                 &black,
                                 &bounds,
                                 "Crosshair 2");

      layout = gtk_widget_create_pango_layout (widget, NULL);
      text = g_strdup_printf ("%d×%d", self->mouse_x, self->mouse_y);
      pango_layout_set_text (layout, text, -1);

      gtk_snapshot_render_layout (snapshot,
                                  gtk_widget_get_style_context (widget),
                                  self->mouse_x + 2,
                                  self->mouse_y - 15, /* *shrug* */
                                  layout);

      g_free (text);
      g_object_unref (layout);
    }
}

static gboolean
gtk_focus_widget_motion_notify_event (GtkWidget *widget,
                                      GdkEventMotion *event)
{
  GtkFocusWidget *self = GTK_FOCUS_WIDGET (widget);

  self->mouse_x = event->x;
  self->mouse_y = event->y;

  gtk_widget_queue_draw (widget);

  return GDK_EVENT_PROPAGATE;
}

static void
gtk_focus_widget_finalize (GObject *object)
{
  GtkFocusWidget *self = GTK_FOCUS_WIDGET (object);

  gtk_widget_unparent (self->child1);
  gtk_widget_unparent (self->child2);
  gtk_widget_unparent (self->child3);
  gtk_widget_unparent (self->child4);

  G_OBJECT_CLASS (gtk_focus_widget_parent_class)->finalize (object);
}

static void
gtk_focus_widget_init (GtkFocusWidget *self)
{
  gtk_widget_set_has_window (GTK_WIDGET (self), FALSE);

  self->child1 = gtk_button_new_with_label ("1");
  gtk_widget_set_parent (self->child1, GTK_WIDGET (self));
  self->child2 = gtk_button_new_with_label ("2");
  gtk_widget_set_parent (self->child2, GTK_WIDGET (self));
  self->child3 = gtk_button_new_with_label ("3");
  gtk_widget_set_parent (self->child3, GTK_WIDGET (self));
  self->child4 = gtk_button_new_with_label ("4");
  gtk_widget_set_parent (self->child4, GTK_WIDGET (self));

  self->mouse_x = G_MININT;
  self->mouse_y = G_MININT;
}

static void
gtk_focus_widget_class_init (GtkFocusWidgetClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = gtk_focus_widget_finalize;

  widget_class->snapshot = gtk_focus_widget_snapshot;
  widget_class->measure = gtk_focus_widget_measure;
  widget_class->size_allocate = gtk_focus_widget_size_allocate;
  widget_class->motion_notify_event = gtk_focus_widget_motion_notify_event;

  gtk_widget_class_set_css_name (widget_class, "focuswidget");
}

int
main()
{
  GtkWidget *window;
  GtkWidget *widget;
  GtkCssProvider *provider;

  gtk_init ();

  provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_data (provider, css, -1);
  gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
                                             GTK_STYLE_PROVIDER (provider),
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  widget = g_object_new (GTK_TYPE_FOCUS_WIDGET, NULL);

  gtk_window_set_decorated (GTK_WINDOW (window), FALSE);

  gtk_container_add (GTK_CONTAINER (window), widget);
  g_signal_connect (window, "delete-event", G_CALLBACK (gtk_main_quit), NULL);

  gtk_widget_show (window);

  gtk_main ();
}
