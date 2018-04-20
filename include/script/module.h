// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_MODULE_H
#define LIBSCRIPT_MODULE_H

#include "libscriptdefs.h"
#include "script/namespace.h"

namespace script
{

class Scope;

class Module;
class ModuleImpl;

typedef void(*ModuleLoadFunction)(Module);
typedef void(*ModuleCleanupFunction)(Module);

class LIBSCRIPT_API Module
{
public:
  Module() = default;
  Module(const Module & other) = default;
  ~Module() = default;

  Module(const std::shared_ptr<ModuleImpl> & impl);

  inline bool isNull() const { return d == nullptr; }
  Engine * engine() const;

  const std::string & name() const;

  Module newSubModule(const std::string & name);
  Module newSubModule(const std::string & name, ModuleLoadFunction load, ModuleCleanupFunction cleanup);
  Module getSubModule(const std::string & name) const;
  const std::vector<Module> & submodules() const;

  bool isLoaded() const;
  void load();

  const Namespace & root() const;
  Scope scope() const;

  inline ModuleImpl* impl() const { return d.get(); }
  inline std::weak_ptr<ModuleImpl> weakref() const { return std::weak_ptr<ModuleImpl>(d); }
  inline const std::shared_ptr<ModuleImpl> & strongref() const { return d; }

private:
  friend class Engine;
  void destroy();

private:
  std::shared_ptr<ModuleImpl> d;
};

} // namespace script


#endif // LIBSCRIPT_CONTEXT_H