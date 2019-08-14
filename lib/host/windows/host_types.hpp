#if !defined(CYCFI_ELEMENTS_HOST_TYPES_AUGUST_14_2019)
#define CYCFI_ELEMENTS_HOST_TYPES_AUGUST_14_2019

#include <elements/base_view.hpp>
#include <Windows.h>

namespace cycfi { namespace elements {

inline HWND as_handle(host_view view)
{
   return reinterpret_cast<HWND>(view);
}

inline host_view as_host_view(HWND hwnd)
{
   return reinterpret_cast<host_view>(hwnd);
}

inline HWND as_handle(host_window window)
{
   return reinterpret_cast<HWND>(window);
}

inline host_window as_host_window(HWND hwnd)
{
   return reinterpret_cast<host_window>(hwnd);
}

}}

#endif