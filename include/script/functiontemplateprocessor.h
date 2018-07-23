// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_FUNCTION_TEMPLATE_PROCESSOR_H
#define LIBSCRIPT_FUNCTION_TEMPLATE_PROCESSOR_H

#include "script/functiontemplate.h"

#include "script/diagnosticmessage.h"
#include "script/templatenameprocessor.h"

namespace script
{

class LIBSCRIPT_API FunctionTemplateProcessor
{
private:
  TemplateNameProcessor *name_;
  TemplateNameProcessor default_name_;

public:
  FunctionTemplateProcessor();
  FunctionTemplateProcessor(const FunctionTemplateProcessor & ) = default;
  virtual ~FunctionTemplateProcessor() = default;

  inline TemplateNameProcessor & name_processor() { return *name_; }
  inline void set_name_processor(TemplateNameProcessor & np) { name_ = &np; }

  static void remove_duplicates(std::vector<FunctionTemplate> & list);

  void complete(std::vector<Function> & functions, const std::vector<FunctionTemplate> & fts, const std::vector<TemplateArgument> & args, const std::vector<Type> & types);
  
  Function deduce_substitute(const FunctionTemplate & ft, const std::vector<TemplateArgument> & args, const std::vector<Type> & types);
  virtual void instantiate(Function & f);
 
  diagnostic::Message emitDiagnostic() const;

  FunctionTemplateProcessor & operator=(const FunctionTemplateProcessor & ) = default;
};

} // namespace script

#endif // LIBSCRIPT_FUNCTION_TEMPLATE_H
