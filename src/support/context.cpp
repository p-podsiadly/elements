/*=================================================================================================   Copyright (c) 2016 Joel de Guzman   Licensed under a Creative Commons Attribution-ShareAlike 4.0 International.   http://creativecommons.org/licenses/by-sa/4.0/=================================================================================================*/#include <photon/support/context.hpp>#include <photon/view.hpp>namespace photon{    photon::canvas basic_context::canvas() const    {        return view.canvas();    }        photon::theme& basic_context::theme() const    {        return view.theme;    }        point basic_context::cursor_pos() const    {        return view.cursor_pos();    }}