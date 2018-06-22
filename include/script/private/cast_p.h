// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_CAST_P_H
#define LIBSCRIPT_CAST_P_H

#include "script/private/function_p.h"

namespace script
{

class CastImpl : public FunctionImpl
{
public:
  CastImpl(const Prototype &p, Engine *e, FunctionImpl::flag_type f = 0);
  ~CastImpl() = default;

  Name get_name() const override;
};

} // namespace script


#endif // LIBSCRIPT_CAST_P_H
