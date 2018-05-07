// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/templatenameprocessor.h"

#include "script/classtemplate.h"
#include "../template_p.h"

#include "script/compiler/compilererrors.h"
#include "script/compiler/literalprocessor.h"
#include "script/compiler/nameresolver.h"
#include "script/compiler/typeresolver.h"

#include "script/ast/node.h"

#include "script/compiler/scriptcompiler.h"

namespace script
{

namespace compiler
{

TemplateArgument TemplateNameProcessor::argument(const Scope & scp, const std::shared_ptr<ast::Node> & arg)
{
  if (arg->is<ast::Identifier>())
  {
    auto name = std::static_pointer_cast<ast::Identifier>(arg);
    NameLookup lookup = NameLookup::resolve(name, scp);
    if (lookup.resultType() == NameLookup::TypeName)
      return TemplateArgument{ lookup.typeResult() };
    else
      throw InvalidTemplateArgument{ dpos(arg) };
  }
  else if (arg->is<ast::Literal>())
  {
    const ast::Literal & l = arg->as<ast::Literal>();
    if (l.is<ast::BoolLiteral>())
      return TemplateArgument{ l.token == parser::Token::True };
    else if (l.is<ast::IntegerLiteral>())
      return TemplateArgument{ LiteralProcessor::generate(std::static_pointer_cast<ast::IntegerLiteral>(arg)) };
    else
      throw InvalidLiteralTemplateArgument{ dpos(arg) };
  }
  else if (arg->is<ast::TypeNode>())
  {
    auto type = std::static_pointer_cast<ast::TypeNode>(arg);
    TypeResolver<BasicNameResolver> r;
    return TemplateArgument{ r.resolve(type->value, scp) };
  }

  throw InvalidTemplateArgument{ dpos(arg) };
}

std::vector<TemplateArgument> TemplateNameProcessor::arguments(const Scope & scp, const std::vector<std::shared_ptr<ast::Node>> & args)
{
  std::vector<TemplateArgument> result;
  result.reserve(args.size());
  for (const auto & a : args)
    result.push_back(argument(scp, a));
  return result;
}

TemplateNameProcessor::InstantiationPolicy TemplateNameProcessor::policy() const
{
  return InstantiateIfNeeded;
}

Class TemplateNameProcessor::instantiate(ClassTemplate & ct, const std::vector<TemplateArgument> & args)
{
  if (ct.is_native())
  {
    auto instantiate = ct.native_callback();
    Class ret = instantiate(ct, args);
    ct.impl()->instances[args] = ret;
    return ret;
  }
  else
  {
    Engine *e = ct.engine();
    compiler::Compiler c{ e };
    compiler::ScriptCompiler compiler{ &c, c.session() };
    auto class_decl = std::static_pointer_cast<ast::ClassDecl>(ct.impl()->definition.decl_->declaration);
    Class ret = compiler.compileClassTemplate(ct, args, class_decl);
    ct.impl()->instances[args] = ret;
    return ret;
  }
}

void TemplateNameProcessor::postprocess(const Template & t, const Scope &scp, std::vector<TemplateArgument> & args)
{
  if (t.parameters().size() == args.size())
    return;

  for (size_t i(0); i < t.parameters().size(); ++i)
  {
    if (!t.parameters().at(i).hasDefaultValue())
      throw MissingNonDefaultedTemplateParameter{};

    TemplateArgument arg = argument(scp, t.parameters().at(i).defaultValue());
    args.push_back(arg);
  }
}

} // namespace compiler

} // namespace script

