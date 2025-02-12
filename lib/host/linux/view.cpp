/*=============================================================================
   Copyright (c) 2016-2019 Joel de Guzman

   Distributed under the MIT License (https://opensource.org/licenses/MIT)
=============================================================================*/
#include "view_impl.hpp"
#include <cairo.h>

namespace cycfi { namespace elements
{
   ///////////////////////////////////////////////////////////////////////////
   // Main view creation callback
   std::function<std::unique_ptr<base_view>(host_view* h)> new_view;

   struct platform_access
   {
      inline static host_view* get_host_view(base_view& main_view)
      {
         return main_view.h;
      }
   };

   host_view::~host_view()
   {
      if (surface)
         cairo_surface_destroy(surface);
      surface = nullptr;
   }

   namespace
   {
      inline base_view& get(gpointer user_data)
      {
         return *reinterpret_cast<base_view*>(user_data);
      }

      void clear_surface(cairo_surface_t* surface)
      {
         cairo_t* cr = cairo_create(surface);
         cairo_set_source_rgb(cr, 0, 0, 0); // $$$ fixme $$$
         cairo_paint(cr);
         cairo_destroy(cr);
      }

      gboolean on_configure(GtkWidget* widget, GdkEventConfigure* event, gpointer user_data)
      {
         auto& main_view = get(user_data);
         auto* host_view = platform_access::get_host_view(main_view);
         auto* window = host_view->window;

         if (host_view->surface)
            cairo_surface_destroy(host_view->surface);

         host_view->surface = gdk_window_create_similar_surface(
            gtk_widget_get_window(window), CAIRO_CONTENT_COLOR,
            gtk_widget_get_allocated_width(window),
            gtk_widget_get_allocated_height(window)
         );

         clear_surface(host_view->surface);
         return true;
      }

      gboolean on_draw(GtkWidget* widget, cairo_t* cr, gpointer user_data)
      {
         auto& main_view = get(user_data);
         auto* host_view = platform_access::get_host_view(main_view);
         cairo_set_source_surface(cr, host_view->surface, 0, 0);
         cairo_paint(cr);

         // Note that cr (cairo_t) is already clipped to only draw the
         // exposed areas of the widget.
         double left, top, right, bottom;
         cairo_clip_extents(cr, &left, &top, &right, &bottom);
         main_view.draw(
            cr,
            rect{ float(left), float(top), float(right), float(bottom) }
         );

         return false;
      }

      template <typename Event>
      bool get_mouse(Event* event, mouse_button& btn, host_view* view)
      {
         btn.modifiers = 0;
         if (event->state & GDK_SHIFT_MASK)
            btn.modifiers |= mod_shift;
         if (event->state & GDK_CONTROL_MASK)
            btn.modifiers |= mod_control;
         if (event->state & GDK_MOD1_MASK)
            btn.modifiers |= mod_alt;
         if (event->state & GDK_SUPER_MASK)
            btn.modifiers |= mod_super;

         btn.num_clicks = view->click_count;
         btn.pos = { float(event->x), float(event->y) };
         return true;
      }

      bool get_button(GdkEventButton* event, mouse_button& btn, host_view* view)
      {
         if (event->button > 4)
            return false;

         static constexpr auto _250ms = 250;
         switch (event->type)
         {
            case GDK_BUTTON_PRESS:
               btn.down = true;
               if ((event->time - view->click_time) < _250ms)
                  ++view->click_count;
               else
                  view->click_count = 1;
               view->click_time = event->time;
               break;
            case GDK_BUTTON_RELEASE:
               btn.down = false;
               view->click_count = 0;
               break;
            default:
               return false;
         }

         if (!get_mouse(event, btn, view))
            return false;
         return true;
      }

      gboolean on_button(GtkWidget* widget, GdkEventButton* event, gpointer user_data)
      {
         auto& main_view = get(user_data);
         mouse_button btn;
         if (get_button(event, btn, platform_access::get_host_view(main_view)))
            main_view.click(btn);
         return TRUE;
      }

      gboolean on_motion(GtkWidget* widget, GdkEventMotion* event, gpointer user_data)
      {
         auto& main_view = get(user_data);
         host_view* view = platform_access::get_host_view(main_view);
         mouse_button btn;
         if (get_mouse(event, btn, view))
         {
            view->cursor_position = btn.pos;

            if (event->state & GDK_BUTTON1_MASK)
            {
               btn.down = true;
               btn.state = mouse_button::left;
            }
            else if (event->state & GDK_BUTTON2_MASK)
            {
               btn.down = true;
               btn.state = mouse_button::middle;
            }
            else if (event->state & GDK_BUTTON3_MASK)
            {
               btn.down = true;
               btn.state = mouse_button::right;
            }
            else
            {
               btn.down = false;
            }

            if (btn.down)
               main_view.drag(btn);
            else
               main_view.cursor(view->cursor_position, cursor_tracking::hovering);
         }
         return TRUE;
      }

      gboolean on_scroll(GtkWidget* widget, GdkEventScroll* event, gpointer user_data)
      {
         auto& main_view = get(user_data);
         auto* host_view = platform_access::get_host_view(main_view);
         auto elapsed = std::max<float>(10.0f, event->time - host_view->scroll_time);
         static constexpr float _1s = 100;
         host_view->scroll_time = event->time;

         float dx = 0;
         float dy = 0;
         float step = _1s / elapsed;

         switch (event->direction)
         {
            case GDK_SCROLL_UP:
               dy = step;
               break;
            case GDK_SCROLL_DOWN:
               dy = -step;
               break;
            case GDK_SCROLL_LEFT:
               dx = step;
               break;
            case GDK_SCROLL_RIGHT:
               dx = -step;
               break;
            case GDK_SCROLL_SMOOTH:
               dx = event->delta_x;
               dy = event->delta_y;
               break;
            default:
               break;
         }

         main_view.scroll(
            { dx, dy },
            { float(event->x), float(event->y) }
         );
         return TRUE;
      }
   }

   gboolean on_event_crossing(GtkWidget* widget, GdkEventCrossing* event, gpointer user_data)
   {
      auto& main_view = get(user_data);
      auto* host_view = platform_access::get_host_view(main_view);
      host_view->cursor_position = point{ float(event->x), float(event->y) };
      main_view.cursor(
         host_view->cursor_position,
         (event->type == GDK_ENTER_NOTIFY) ?
            cursor_tracking::entering :
            cursor_tracking::leaving
      );
   }

   void make_main_window(base_view& main_view, GtkWidget* window)
   {
      auto* drawing_area = gtk_drawing_area_new();

      gtk_container_add(GTK_CONTAINER(window), drawing_area);

      g_signal_connect(G_OBJECT(drawing_area), "configure-event",
         G_CALLBACK(on_configure), &main_view);

      g_signal_connect(G_OBJECT(drawing_area), "draw",
         G_CALLBACK(on_draw), &main_view);

      //gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
      gtk_window_set_default_size(GTK_WINDOW(window), 400, 300);

      g_signal_connect(drawing_area, "button-press-event",
         G_CALLBACK(on_button), &main_view);
      g_signal_connect (drawing_area, "button-release-event",
         G_CALLBACK(on_button), &main_view);
      g_signal_connect(drawing_area, "motion-notify-event",
         G_CALLBACK(on_motion), &main_view);
      g_signal_connect(drawing_area, "scroll-event",
         G_CALLBACK(on_scroll), &main_view);
      g_signal_connect(drawing_area, "enter-notify-event",
         G_CALLBACK(on_event_crossing), &main_view);
      g_signal_connect(drawing_area, "leave-notify-event",
         G_CALLBACK(on_event_crossing), &main_view);

      // Ask to receive events the drawing area doesn't normally
      // subscribe to. In particular, we need to ask for the
      // button press and motion notify events that want to handle.
      gtk_widget_set_events(drawing_area,
         gtk_widget_get_events(drawing_area)
         | GDK_BUTTON_PRESS_MASK
         | GDK_BUTTON_RELEASE_MASK
         | GDK_POINTER_MOTION_MASK
         | GDK_SCROLL_MASK
         | GDK_ENTER_NOTIFY_MASK
         | GDK_LEAVE_NOTIFY_MASK
                            // GDK_SMOOTH_SCROLL_MASK
      );
   }

   point base_view::cursor_pos() const
   {
      return h->cursor_position;
   }

   point base_view::size() const
   {
      auto x = gtk_widget_get_allocated_width(h->window);
      auto y = gtk_widget_get_allocated_height(h->window);
      return { float(x), float(y) };
   }

   void base_view::size(point p)
   {
       gtk_window_resize(GTK_WINDOW(h->window), p.x, p.y);
   }

   void base_view::refresh()
   {
      auto x = gtk_widget_get_allocated_width(h->window);
      auto y = gtk_widget_get_allocated_height(h->window);
      refresh({ 0, 0, float(x), float(y) });
   }

   void base_view::refresh(rect area)
   {
      gtk_widget_queue_draw_area(h->window,
         area.left, area.top, area.width(), area.height());
   }

   void base_view::limits(view_limits limits_)
   {
      GdkGeometry hints;
      hints.min_width = limits_.min.x;
      hints.min_height = limits_.min.y;
      hints.max_width = limits_.max.x;
      hints.max_height = limits_.max.y;

      gtk_window_set_geometry_hints(GTK_WINDOW(h->window), nullptr,
         &hints, GdkWindowHints(GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE));
   }

   std::string clipboard()
   {
      return "";
   }

   void clipboard(std::string const& text)
   {
   }

   void set_cursor(cursor_type type)
   {
      switch (type)
      {
         case cursor_type::arrow:
            break;
         case cursor_type::ibeam:
            break;
         case cursor_type::cross_hair:
            break;
         case cursor_type::hand:
            break;
         case cursor_type::h_resize:
            break;
         case cursor_type::v_resize:
            break;
      }
   }
}}

