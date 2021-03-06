// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/initialization.h"

#include "script/class.h"
#include "script/engine.h"
#include "script/templateargument.h"
#include "script/typesystem.h"

#include "script/program/expression.h"

#include <algorithm>
#include <stdexcept>

namespace script
{

struct InitializationData
{
  InitializationData(Type t)
    : type(t) { }

  InitializationData(Function ctor)
    : constructor(ctor) 
  { 
    type = ctor.memberOf().id();
  }

  Type type;
  Function constructor;
  std::vector<Initialization> initializations;
};

Initialization::Initialization()
  : mCategory(DefaultInitialization)
{

}

Initialization::Initialization(Category cat)
  : mCategory(cat)
{

}

Initialization::Initialization(Category cat, Type t)
  : mCategory(cat)
{
  mData = std::make_shared<InitializationData>(t);
}

Initialization::Initialization(Category cat, Function ctor)
  : mCategory(cat)
{
  mData = std::make_shared<InitializationData>(ctor);
}

Initialization::Initialization(Category cat, const Conversion & conv)
  : mCategory(cat)
  , mConversion(conv)
{
  if (conv.isInvalid())
    mCategory = InvalidInitialization;
}

bool Initialization::isValid() const
{
  return mData != nullptr || mConversion != Conversion::NotConvertible();
}

bool Initialization::createsTemporary() const
{
  if (!isReferenceInitialization())
    return false;

  return !mConversion.firstStandardConversion().isReferenceConversion()
    || !mConversion.firstStandardConversion().isReferenceConversion();
}

ConversionRank Initialization::rank() const
{
  if (mData == nullptr)
    return mConversion.rank();

  ConversionRank r = mData->initializations.front().rank();
  for (size_t i(1); i < mData->initializations.size(); ++i)
    r = std::max(r, mData->initializations.at(i).rank());
  return r;
}

bool Initialization::isNarrowing() const
{
  if (mData == nullptr)
    return mConversion.isNarrowing();

  for (const auto & init : mData->initializations)
  {
    if (init.isNarrowing())
      return true;
  }

  return false;
}

bool Initialization::hasInitializations() const
{
  return mData != nullptr;
}

std::vector<Initialization> & Initialization::initializations()
{
  return mData->initializations;
}

const std::vector<Initialization> & Initialization::initializations() const
{
  return mData->initializations;
}

Type Initialization::destType() const
{
  switch (kind())
  {
  case DirectInitialization:
  case ListInitialization:
  case AggregateInitialization:
    return mData->type;
  case DefaultInitialization:
  case ReferenceInitialization:
  case CopyInitialization:
    return Type::Auto;
  case InvalidInitialization:
    return Type::Null;
  }

  return Type::Null;
}

const Function & Initialization::constructor() const
{
  return mData->constructor;
}


int Initialization::comp(const Initialization & a, const Initialization & b)
{
  if ((a.mData == nullptr) != (b.mData == nullptr))
    throw std::runtime_error{ "Initialization::comp() : the two sequences are not comparable" };

  if(a.mData == nullptr)
    return Conversion::comp(a.conversion(), b.conversion());

  if (a.kind() == ListInitialization && b.kind() != ListInitialization)
    return 1;
  else if (a.kind() != ListInitialization && b.kind() == ListInitialization)
    return -1;

  if (a.constructor().isNull() && !b.constructor().isNull())
  {
    // a initializes an initializer_list
    return -1;
  }
  else if (!a.constructor().isNull() && b.constructor().isNull())
  {
    return 1;
  }

  return 0;
}


static Initialization compute_initializer_list_conv(const Type & vartype, const std::shared_ptr<program::Expression> & expr, Engine *engine)
{
  if (!expr->is<program::InitializerList>())
    return Initialization::InvalidInitialization;

  const program::InitializerList & init_list = dynamic_cast<const program::InitializerList &>(*expr);

  Class initializer_list_type = engine->typeSystem()->getClass(vartype);
  Type T = initializer_list_type.arguments().front().type;

  std::vector<Initialization> inits;
  for (const auto & e : init_list.elements)
  {
    Initialization i = Initialization::compute(T, e, engine);
    if (!i.isValid())
      return i;

    inits.push_back(i);
  }

  Initialization result{ Initialization::ListInitialization, initializer_list_type.id() };
  result.initializations() = std::move(inits);
  return result;
}

static Initialization compute_initializer_list_conv(const std::shared_ptr<program::Expression> & expr, const Function & ctor, Engine *engine)
{
  assert(expr->is<program::InitializerList>());
  assert(engine->typeSystem()->isInitializerList(ctor.parameter(1)));

  const program::InitializerList & init_list = dynamic_cast<const program::InitializerList &>(*expr);

  Class initializer_list_type = engine->typeSystem()->getClass(ctor.parameter(1));
  Type T = initializer_list_type.arguments().front().type;

  std::vector<Initialization> inits;
  for (const auto & e : init_list.elements)
  {
    Initialization i = Initialization::compute(T, e, engine);
    if (!i.isValid())
      return i;

    inits.push_back(i);
  }

  Initialization result{ Initialization::ListInitialization, ctor };
  result.initializations() = std::move(inits);
  return result;
}

Initialization Initialization::compute(const Type & vartype, Engine *engine)
{
  if (vartype.isEnumType() || vartype.isClosureType() || vartype.isFunctionType())
    return InvalidInitialization;
  else if (vartype.isFundamentalType())
    return DefaultInitialization;
  
  assert(vartype.isObjectType());
  Class cla = engine->typeSystem()->getClass(vartype);
  if (!cla.isDefaultConstructible())
    return InvalidInitialization;
  return DefaultInitialization;
}

Initialization Initialization::compute(const Type & vartype, const Type & arg, Engine *engine, Initialization::Category cat)
{
  if(cat != DirectInitialization && cat != CopyInitialization)
    throw std::runtime_error{ "Invalid Initialization::Category" };

  auto opts = cat == DirectInitialization ? Conversion::AllowExplicitConversions : Conversion::NoExplicitConversions;
  Conversion conv = Conversion::compute(arg, vartype, engine, opts);
  if(!conv.isInvalid() || !vartype.isConstRef())
    return Initialization{ cat, conv };

  assert(vartype.isConstRef() && conv.isInvalid());

  conv = Conversion::compute(arg, vartype.withoutRef(), engine, opts);
  return Initialization{ ReferenceInitialization, conv };
}

Initialization Initialization::compute(const Type & vartype, const std::shared_ptr<program::Expression> & expr, Engine *engine)
{
  if (expr->type() != Type::InitializerList)
    return compute(vartype, expr->type(), engine, CopyInitialization);

  if (engine->typeSystem()->isInitializerList(vartype))
    return compute_initializer_list_conv(vartype, expr, engine);

  if (vartype.isReference() && !vartype.isConst())
    return InvalidInitialization;

  const program::InitializerList & init_list = dynamic_cast<const program::InitializerList &>(*expr);

  if (init_list.elements.empty())
    return Initialization::compute(vartype, engine);

  if (!vartype.isObjectType())
    return InvalidInitialization;

  const Class dest_class = engine->typeSystem()->getClass(vartype);

  std::vector<Initialization> inits;
  inits.resize(init_list.elements.size());

  /// TODO: check all initializer_list ctors first
  // then perform overload resolution on the other ctos if needed
  for (const auto & ctor : dest_class.constructors())
  {
    if (ctor.prototype().count() == 2 && engine->typeSystem()->isInitializerList(ctor.parameter(1)))
      return compute_initializer_list_conv(expr, ctor, engine);

    if (ctor.prototype().count() != init_list.elements.size() + 1)
      continue;

    inits.clear();
    for (size_t i(0); i < init_list.elements.size(); ++i)
    {
      Initialization init = Initialization::compute(ctor.prototype().at(i+1), init_list.elements.at(i), engine);
      inits.push_back(init);
      if (!init.isValid())
        break;
    }

    if (inits.back().isValid())
    {
      Initialization result{ ListInitialization, ctor };
      result.initializations() = std::move(inits);
      return result;
    }
  }

  // TODO : implement aggregate and initializer_list initialization

  return InvalidInitialization;
}

} // namespace script
