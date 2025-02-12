/*=============================================================================
   Copyright (c) 2016-2019 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_ELEMENTS_GUI_LIB_CANVAS_IMPL_MAY_3_2016)
#define CYCFI_ELEMENTS_GUI_LIB_CANVAS_IMPL_MAY_3_2016

#ifdef __linux__
# include <map>
# include <cairo-ft.h>
# include <ft2build.h>
# include FT_SFNT_NAMES_H
# include FT_FREETYPE_H
# include FT_GLYPH_H
# include FT_OUTLINE_H
# include FT_BBOX_H
# include FT_TYPE1_TABLES_H
#endif

#ifndef M_PI
# define M_PI 3.14159265358979323846
#endif

namespace cycfi { namespace elements
{
   ////////////////////////////////////////////////////////////////////////////
   // Inlines
   ////////////////////////////////////////////////////////////////////////////
   inline canvas::~canvas()
   {
#if defined(__linux__)
      for (auto& p : _custom_fonts)
         cairo_font_face_destroy(p.second);
#endif
   }

   inline cairo_t& canvas::cairo_context() const
   {
      return _context;
   }

   inline void canvas::translate(point p)
   {
      cairo_translate(&_context, p.x, p.y);
   }

   inline void canvas::rotate(float rad)
   {
      cairo_rotate(&_context, rad);
   }

   inline void canvas::scale(point p)
   {
      cairo_scale(&_context, p.x, p.y);
   }

   inline point canvas::device_to_user(point p)
   {
      double x = p.x;
      double y = p.y;
      cairo_device_to_user(&_context, &x, &y);
      return { float(x), float(y) };
   }

   inline point canvas::user_to_device(point p)
   {
      double x = p.x;
      double y = p.y;
      cairo_user_to_device(&_context, &x, &y);
      return { float(x), float(y) };
   }

   inline void canvas::begin_path()
   {
      cairo_new_path(&_context);
   }

   inline void canvas::close_path()
   {
      cairo_close_path(&_context);
   }

   inline void canvas::fill()
   {
      apply_fill_style();
      cairo_fill(&_context);
   }

   inline void canvas::fill_preserve()
   {
      apply_fill_style();
      cairo_fill_preserve(&_context);
   }

   inline void canvas::stroke()
   {
      apply_stroke_style();
      cairo_stroke(&_context);
   }

   inline void canvas::stroke_preserve()
   {
      apply_stroke_style();
      cairo_stroke_preserve(&_context);
   }

   inline void canvas::clip()
   {
      cairo_clip(&_context);
   }

   inline bool canvas::hit_test(point p) const
   {
      return cairo_in_fill(&_context, p.x, p.y);
   }

   inline rect canvas::fill_extent() const
   {
      double x1, y1, x2, y2;
      cairo_fill_extents(&_context, &x1, &y1, &x2, &y2);
      return elements::rect(x1, y1, x2, y2);
   }

   inline void canvas::move_to(point p)
   {
      cairo_move_to(&_context, p.x, p.y);
   }

   inline void canvas::line_to(point p)
   {
      cairo_line_to(&_context, p.x, p.y);
   }

   inline void canvas::arc_to(point p1, point p2, float radius)
   {
      assert(false); // unimplemented
   }

   inline void canvas::arc(
      point p, float radius,
      float start_angle, float end_angle,
      bool ccw
   )
   {
      if (ccw)
         cairo_arc_negative(&_context, p.x, p.y, radius, start_angle, end_angle);
      else
         cairo_arc(&_context, p.x, p.y, radius, start_angle, end_angle);
   }

   inline void canvas::rect(struct rect r)
   {
      cairo_rectangle(&_context, r.left, r.top, r.width(), r.height());
   }

   inline void canvas::round_rect(struct rect bounds, float radius)
   {
      auto x = bounds.left;
      auto y = bounds.top;
      auto r = bounds.right;
      auto b = bounds.bottom;
      auto const a = M_PI/180.0;

      cairo_new_sub_path(&_context);
      cairo_arc(&_context, r-radius, y+radius, radius, -90*a, 0*a);
      cairo_arc(&_context, r-radius, b-radius, radius, 0*a, 90*a);
      cairo_arc(&_context, x+radius, b-radius, radius, 90*a, 180*a);
      cairo_arc(&_context, x+radius, y+radius, radius, 180*a, 270*a);
      cairo_close_path(&_context);
   }

   inline void canvas::circle(struct circle c)
   {
      arc(point{ c.cx, c.cy }, c.radius, 0.0, 2 * M_PI);
   }

   inline void canvas::fill_style(color c)
   {
      _state.fill_style = [this, c]()
      {
         cairo_set_source_rgba(&_context, c.red, c.green, c.blue, c.alpha);
      };
      if (_state.pattern_set == _state.fill_set)
         _state.pattern_set = _state.none_set;
   }

   inline void canvas::stroke_style(color c)
   {
      _state.stroke_style = [this, c]()
      {
         cairo_set_source_rgba(&_context, c.red, c.green, c.blue, c.alpha);
      };
      if (_state.pattern_set == _state.stroke_set)
         _state.pattern_set = _state.none_set;
   }

   inline void canvas::line_width(float w)
   {
      cairo_set_line_width(&_context, w);
   }

   inline void canvas::fill_style(linear_gradient const& gr)
   {
      _state.fill_style = [this, gr]()
      {
         cairo_pattern_t* pat =
            cairo_pattern_create_linear(
               gr.start.x, gr.start.y, gr.end.x, gr.end.y
            );

         for (auto cs : gr.space)
         {
            cairo_pattern_add_color_stop_rgba(
               pat, cs.offset,
               cs.color.red, cs.color.green, cs.color.blue, cs.color.alpha
            );
         }
         cairo_set_source(&_context, pat);
      };
      if (_state.pattern_set == _state.fill_set)
         _state.pattern_set = _state.none_set;
   }

   inline void canvas::fill_style(radial_gradient const& gr)
   {
      _state.fill_style = [this, gr]()
      {
         cairo_pattern_t* pat =
            cairo_pattern_create_radial(
               gr.c1.x, gr.c1.y, gr.c1_radius,
               gr.c2.x, gr.c2.y, gr.c2_radius
            );

         for (auto cs : gr.space)
         {
            cairo_pattern_add_color_stop_rgba(
               pat, cs.offset,
               cs.color.red, cs.color.green, cs.color.blue, cs.color.alpha
            );
         }
         cairo_set_source(&_context, pat);
      };
      if (_state.pattern_set == _state.fill_set)
         _state.pattern_set = _state.none_set;
   }

   inline void canvas::fill_rule(fill_rule_enum rule)
   {
      cairo_set_fill_rule(
         &_context, rule == fill_winding ? CAIRO_FILL_RULE_WINDING : CAIRO_FILL_RULE_EVEN_ODD);
   }

   inline void canvas::linear_gradient::add_color_stop(color_stop cs)
   {
      space.push_back(cs);
   }

   inline void canvas::radial_gradient::add_color_stop(color_stop cs)
   {
      space.push_back(cs);
   }

   inline void canvas::fill_rect(struct rect r)
   {
      rect(r);
      fill();
   }

   inline void canvas::fill_round_rect(struct rect r, float radius)
   {
      round_rect(r, radius);
      fill();
   }

   inline void canvas::stroke_rect(struct rect r)
   {
      rect(r);
      stroke();
   }

   inline void canvas::stroke_round_rect(struct rect r, float radius)
   {
      round_rect(r, radius);
      stroke();
   }

   inline void canvas::font(char const* face, float size, int style)
   {
      cairo_font_slant_t slant = (style & italic) ? CAIRO_FONT_SLANT_ITALIC : CAIRO_FONT_SLANT_NORMAL;
      cairo_font_weight_t weight = (style & bold) ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL;
      cairo_select_font_face(&_context, face, slant, weight);
      cairo_set_font_size(&_context, size);
   }

   inline void canvas::custom_font(char const* font, float size)
   {
#if defined(__APPLE__)

      cairo_select_font_face(&_context, font, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
      cairo_set_font_size(&_context, size);

#elif defined(__linux__)

      FT_Library value;
      FT_Error status = FT_Init_FreeType(&value);
      if (status != 0)
         return;

      cairo_font_face_t* ct = nullptr;
      auto fi = _custom_fonts.find(font);
      if (_custom_fonts.find(font) == _custom_fonts.end())
      {
         FT_Face face;
         std::string file = "./" + std::string{font} + ".ttf";
         status = FT_New_Face(value, file.c_str(), 0, &face);
         if (status != 0)
            return;
         ct = cairo_ft_font_face_create_for_ft_face(face, 0);
         _custom_fonts[font] = ct;
      }
      else
      {
         ct = fi->second;
      }

      cairo_set_font_face(&_context, ct);
      cairo_set_font_size(&_context, size);

#endif
   }

   namespace
   {
      inline point get_text_start(cairo_t& _context, point p, int align, char const* utf8)
      {
         cairo_text_extents_t extents;
         cairo_text_extents(&_context, utf8, &extents);

         cairo_font_extents_t font_extents;
         cairo_scaled_font_extents(cairo_get_scaled_font(&_context), &font_extents);

         switch (align & 0x3)
         {
            case canvas::text_alignment::right:
               p.x -= extents.width;
               break;
            case canvas::text_alignment::center:
               p.x -= extents.width/2;
               break;
            default:
               break;
         }

         switch (align & 0x1C)
         {
            case canvas::text_alignment::top:
               p.y += font_extents.ascent;
               break;

            case canvas::text_alignment::middle:
               p.y += font_extents.ascent/2 - font_extents.descent/2;
               break;

            case canvas::text_alignment::bottom:
               p.y -= font_extents.descent;
               break;

            default:
               break;
         }

         return p;
      }
   }

   inline void canvas::fill_text(point p, char const* utf8)
   {
      apply_fill_style();
      p = get_text_start(_context, p, _state.align, utf8);
      cairo_move_to(&_context, p.x, p.y);
      cairo_show_text(&_context, utf8);
   }

   inline void canvas::stroke_text(point p, char const* utf8)
   {
      apply_stroke_style();
      p = get_text_start(_context, p, _state.align, utf8);
      cairo_move_to(&_context, p.x, p.y);
      cairo_text_path(&_context, utf8);
      stroke();
   }

   inline canvas::text_metrics canvas::measure_text(char const* utf8)
   {
      cairo_text_extents_t extents;
      cairo_text_extents(&_context, utf8, &extents);

      cairo_font_extents_t font_extents;
      cairo_scaled_font_extents(cairo_get_scaled_font(&_context), &font_extents);

      return {
         /*ascent=*/    float(font_extents.ascent),
         /*descent=*/   float(font_extents.descent),
         /*leading=*/   float(font_extents.height-(font_extents.ascent+font_extents.descent)),
         /*size=*/      { float(extents.width), float(extents.height) }
      };
   }

   inline void canvas::text_align(int align)
   {
      _state.align = align;
   }

   inline void canvas::draw(pixmap const& pm, elements::rect src, elements::rect dest)
   {
      auto  state = new_state();
      auto  w = dest.width();
      auto  h = dest.height();
      translate(dest.top_left());
      auto scale_ = point{ w/src.width(), h/src.height() };
      scale(scale_);
      cairo_set_source_surface(&_context, pm._surface, -src.left, -src.top);
      rect({ 0, 0, w/scale_.x, h/scale_.y });
      cairo_fill(&_context);
   }

   inline void canvas::draw(pixmap const& pm, elements::rect dest)
   {
      draw(pm, { 0, 0, pm.size() }, dest);
   }

   inline void canvas::draw(pixmap const& pm, point pos)
   {
      draw(pm, { 0, 0, pm.size() }, { pos, pm.size() });
   }

   inline canvas::state::state(canvas& cnv_)
     : cnv(&cnv_)
   {
      cnv->save();
   }

   inline canvas::state::state(state&& rhs)
    : cnv(rhs.cnv)
   {
      rhs.cnv = 0;
   }

   inline canvas::state::~state()
   {
      if (cnv)
         cnv->restore();
   }

   inline canvas::state& canvas::state::operator=(state&& rhs)
   {
      cnv = rhs.cnv;
      rhs.cnv = 0;
      return *this;
   }

   inline void canvas::save()
   {
      cairo_save(&_context);
      _state_stack.push(_state);
   }

   inline void canvas::restore()
   {
      _state = _state_stack.top();
      _state_stack.pop();
      cairo_restore(&_context);
   }

   inline void canvas::apply_fill_style()
   {
      if (_state.pattern_set != _state.fill_set && _state.fill_style)
      {
         _state.fill_style();
         _state.pattern_set = _state.fill_set;
      }
   }

   inline void canvas::apply_stroke_style()
   {
      if (_state.pattern_set != _state.stroke_set && _state.stroke_style)
      {
         _state.stroke_style();
         _state.pattern_set = _state.stroke_set;
      }
   }
}}

#endif
