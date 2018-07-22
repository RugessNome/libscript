// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMMAND_COMPILER_H
#define LIBSCRIPT_COMMAND_COMPILER_H

#include "script/compiler/compilercomponent.h"
#include "script/compiler/expressioncompiler.h"

#include "script/context.h"

namespace script
{

namespace compiler
{

class CommandExpressionCompiler : public ExpressionCompiler
{
public:
  using ExpressionCompiler::ExpressionCompiler;

  Context context_;

private:
  std::shared_ptr<program::Expression> generateOperation(const std::shared_ptr<ast::Expression> & op) override;
};

class CapturelessLambdaProcessor : public LambdaProcessor
{
public:
  CapturelessLambdaProcessor(Compiler *c);
  ~CapturelessLambdaProcessor() = default;

  Compiler* compiler_;

  std::shared_ptr<program::LambdaExpression> generate(ExpressionCompiler & ec, const std::shared_ptr<ast::LambdaExpression> & le) override;
};

class CommandCompiler : public CompilerComponent
{
public:
  CommandCompiler(Compiler *c);
  ~CommandCompiler() = default;

  void setScope(const Scope & scp);

  std::shared_ptr<program::Expression> compile(const std::string & expr, Context context, Scope scp = Scope{});


  std::shared_ptr<program::Expression> compile(const std::shared_ptr<ast::Expression> & expr, const Context & context);

protected:

private:
  CommandExpressionCompiler expr_;
  CapturelessLambdaProcessor lambda_;
};


} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMMAND_COMPILER_H
