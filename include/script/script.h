// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_SCRIPT_H
#define LIBSCRIPT_SCRIPT_H

#include <string>

#include "script/diagnosticmessage.h"
#include "script/sourcefile.h"
#include "script/namespace.h"

namespace script
{

class ScriptImpl;

class Engine;

class LIBSCRIPT_API Script
{
public:
  Script() = default;
  Script(const Script &) = default;
  ~Script() = default;

  Script(const std::shared_ptr<ScriptImpl> & impl);

  inline bool isNull() const { return d == nullptr; }

  int id() const;
  const std::string & name() const;
  void setName(const std::string & name);

  bool compile();
  bool isReady() const;
  inline bool isCompiled() const { return isReady(); }
  void run();

  void clear();

  const std::string & Script::path() const;
  const SourceFile & source() const;
  const std::map<std::string, int> & globalNames() const;
  const std::vector<Value> & globals() const;

  Namespace rootNamespace() const;

  inline const std::map<std::string, Value> & vars() const { return rootNamespace().vars(); }
  inline const std::vector<Enum> & enums() const { return rootNamespace().enums(); }
  inline const std::vector<Function> & functions() const { return rootNamespace().functions(); }
  inline const std::vector<Operator> & operators() const { return rootNamespace().operators(); }
  inline const std::vector<LiteralOperator> & literalOperators() const { return rootNamespace().literalOperators(); }
  inline const std::vector<Class> & classes() const { return rootNamespace().classes(); }
  inline const std::vector<Namespace> & namespaces() const { return rootNamespace().namespaces(); }
  inline const std::vector<Template> & templates() const { return rootNamespace().templates(); }
  inline const std::vector<Typedef> & typedefs() const { return rootNamespace().typedefs(); }

  const std::vector<diagnostic::Message> & messages() const;

  Engine * engine() const;
  inline const std::shared_ptr<ScriptImpl> & impl() const { return d; }

  Script & operator=(const Script &) = default;

  bool operator==(const Script & other) const;
  bool operator!=(const Script & other) const;

private:
  std::shared_ptr<ScriptImpl> d;
};

} // namespace script

#endif // LIBSCRIPT_SCRIPT_H
