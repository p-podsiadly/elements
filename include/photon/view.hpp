/*=================================================================================================
   Copyright (c) 2016 Joel de Guzman

   Licensed under a Creative Commons Attribution-ShareAlike 4.0 International.
   http://creativecommons.org/licenses/by-sa/4.0/
=================================================================================================*/
#if !defined(PHOTON_GUI_LIB_VIEW_AUGUST_15_2016)
#define PHOTON_GUI_LIB_VIEW_AUGUST_15_2016

#include <photon/support/rect.hpp>
#include <photon/support/canvas.hpp>
#include <photon/support/theme.hpp>
#include <photon/widget/widget.hpp>
#include <photon/widget/layer.hpp>
#include <memory>

namespace photon
{
   class view;
}

namespace client
{
   ////////////////////////////////////////////////////////////////////////////////////////////////
   // Main user initialization. This client supplied function initializes the view at
   // construction time. This is the main customization point provided for clients.
   ////////////////////////////////////////////////////////////////////////////////////////////////
   void  init(photon::view& v);
}

namespace photon
{
   struct view_impl;
   struct view_state;
   class platform_access;

   ////////////////////////////////////////////////////////////////////////////////////////////////
   // The View. There's only one of this per window.
   ////////////////////////////////////////////////////////////////////////////////////////////////
   class view
   {
   public:
                           view();
                           view(view const&) = delete;
      view&                operator=(view const&) = delete;

      void                 draw(rect dirty_);
      photon::canvas       canvas();
      point                cursor_pos() const;
      point                size() const;
      void                 size(point size_);
      void                 limits(rect limits_) const;

      void                 refresh();
      rect                 dirty() const   { return _dirty; }

      layer_composite      content;
      photon::theme        theme;

   private:

      friend class platform_access;
      friend class canvas;
      using view_state_ptr = std::shared_ptr<view_state>;

      view_impl*           _impl;
      view_state_ptr       _state;
      rect                 _dirty;
      rect                 _current_bounds;
   };
}

#endif