// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_FUNCTION_TEMPLATE_H
#define LIBSCRIPT_FUNCTION_TEMPLATE_H

#include "script/template.h"

#include <map>

namespace script
{

class FunctionTemplateImpl;

class FunctionBuilder;
class NameLookup;
class UserData;

typedef Function(*NativeFunctionTemplateInstantiationCallback)(FunctionTemplate, Function);
typedef Function(*NativeFunctionTemplateSubstitutionCallback)(FunctionTemplate, const std::vector<TemplateArgument> &);

namespace program
{
class Expression;
} // namespace program

class LIBSCRIPT_API FunctionTemplate : public Template
{
public:
  FunctionTemplate() = default;
  FunctionTemplate(const FunctionTemplate & other) = default;
  ~FunctionTemplate() = default;

  FunctionTemplate(const std::shared_ptr<FunctionTemplateImpl> & impl);

  static void complete(NameLookup & lookup, const std::vector<TemplateArgument> & targs, const std::vector<std::shared_ptr<program::Expression>> & args);
  static void complete(NameLookup & lookup, const std::vector<TemplateArgument> & targs, const std::vector<Type> & args);

  inline bool deduce(std::vector<TemplateArgument> & out, const std::vector<Type> & in) { return Template::deduce(out, in); }
  Function substitute(const std::vector<TemplateArgument> & targs);
  void instantiate(Function & f);

  Function build(const FunctionBuilder & builder, const std::vector<TemplateArgument> & args);
  static void setInstanceData(Function & f, const std::shared_ptr<UserData> & data);

  bool hasInstance(const std::vector<TemplateArgument> & args, Function *value = nullptr) const;
  Function getInstance(const std::vector<TemplateArgument> & args);

  Function addSpecialization(const std::vector<TemplateArgument> & args, const FunctionBuilder & opts);

  const std::map<std::vector<TemplateArgument>, Function, TemplateArgumentComparison> & instances() const;

  FunctionTemplate & operator=(const FunctionTemplate & other) = default;

protected:
  std::shared_ptr<FunctionTemplateImpl> impl() const;
};

} // namespace script

#endif // LIBSCRIPT_FUNCTION_TEMPLATE_H
