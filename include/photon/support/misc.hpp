/*=============================================================================
   Copyright (c) 2016-2017 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(PHOTON_GUI_LIB_MISC_APRIL_10_2016)
#define PHOTON_GUI_LIB_MISC_APRIL_10_2016

#include <photon/support/rect.hpp>
#include <photon/support/point.hpp>

#include <cfloat>
#include <cstdint>
#include <cassert>
#include <chrono>

namespace photon
{
   ////////////////////////////////////////////////////////////////////////////
   // Time
   ////////////////////////////////////////////////////////////////////////////
   using duration       = std::chrono::duration<double>;
   using microseconds   = std::chrono::duration<double, std::micro>;
   using milliseconds   = std::chrono::duration<double, std::milli>;
   using seconds        = std::chrono::duration<double>;
   using minutes        = std::chrono::duration<double, std::ratio<60>>;
   using hours          = std::chrono::duration<double, std::ratio<60*60>>;
   using time_point     = std::chrono::time_point<std::chrono::steady_clock, duration>;

   inline time_point now()
   {
      return std::chrono::high_resolution_clock::now();
   }

   ////////////////////////////////////////////////////////////////////////////
   // Utilities
   ////////////////////////////////////////////////////////////////////////////
   template <typename T, typename U>
   inline void clamp_max(T& val, U const& max)
   {
      if (val > max)
         val = max;
   }

   template <typename T, typename U>
   inline void clamp_min(T& val, U const& min)
   {
      if (val < min)
         val = min;
   }

   template <typename T, typename U, typename V>
   inline void clamp(T& val, U const& min, V const& max)
   {
      assert(min <= max);
      clamp_min(val, min);
      clamp_max(val, max);
   }

   template <typename T, typename U, typename V>
   inline bool within(T const& val, U const& min, V const& max)
   {
      return (val >= min) && (val <= max);
   }

   template <typename T>
   constexpr T abs(T i)
   {
      return (i >= 0)? i : -i;
   }

   ////////////////////////////////////////////////////////////////////////////
   // Generic contextual setter
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   struct setter
   {
      setter(T& ref, T const& new_val)
       : _ref(ref)
       , _save(ref)
      {
         _ref = new_val;
      }

      ~setter()
      {
         _ref = _save;
      }

      T& _ref;
      T  _save;
   };

   template <typename T>
   inline setter<T> set(T& ref, T const& new_val)
   {
      return { ref, new_val };
   };
}
#endif
