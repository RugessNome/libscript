// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_STRING_P_H
#define LIBSCRIPT_STRING_P_H

#if defined(LIBSCRIPT_HAS_CONFIG)
#include "config/libscript/stringimpl.h"
#else

#include <string>

#include "script/string.h"

namespace script
{

class Engine;
class Class;

std::string get_string_typename();
void register_string_type(Class cla);

namespace string
{

inline String convert(const std::string & str) { return str; }

} // namespace string

} // namespace script

#endif // defined(LIBSCRIPT_HAS_CONFIG)

#endif // LIBSCRIPT_STRING_P_H
