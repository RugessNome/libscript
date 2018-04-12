// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/constructorcompiler.h"
#include "script/compiler/compilererrors.h"

#include "script/ast/node.h"

#include "script/program/statements.h"

#include "script/namelookup.h"
#include "script/overloadresolution.h"

namespace script
{

namespace compiler
{

ConstructorCompiler::ConstructorCompiler(FunctionCompiler *c)
  : compiler(c)
{
  assert(c != nullptr);
}

std::shared_ptr<program::CompoundStatement> ConstructorCompiler::generateHeader()
{
  /// TODO : refactor 

  auto ctor_decl = std::dynamic_pointer_cast<ast::ConstructorDecl>(compiler->declaration());

  auto this_object = compiler->generateThisAccess();

  std::vector<ast::MemberInitialization> initializers = ctor_decl->memberInitializationList;

  std::shared_ptr<program::Statement> parent_ctor_call;
  for (size_t i(0); i < initializers.size(); ++i)
  {
    const auto & minit = initializers.at(i);
    NameLookup lookup = compiler->resolve(minit.name);
    if (lookup.typeResult() == compiler->classScope().id()) // delegating constructor
    {
      if (initializers.size() != 1)
        throw InvalidUseOfDelegatedConstructor{ compiler->dpos(minit.name) };

      if (minit.init->is<ast::ConstructorInitialization>())
      {
        std::vector<std::shared_ptr<program::Expression>> args = compiler->generateExpressions(minit.init->as<ast::ConstructorInitialization>().args);
        return program::CompoundStatement::New({ generateDelegateConstructorCall(std::dynamic_pointer_cast<ast::ConstructorInitialization>(minit.init), args) });
      }
      else
      {
        std::vector<std::shared_ptr<program::Expression>> args = compiler->generateExpressions(minit.init->as<ast::BraceInitialization>().args);
        return program::CompoundStatement::New({ generateDelegateConstructorCall(std::dynamic_pointer_cast<ast::BraceInitialization>(minit.init), args) });
      }
    }
    else if (!compiler->classScope().parent().isNull() && lookup.typeResult() == compiler->classScope().parent().id()) // parent constructor call
    {
      if (minit.init->is<ast::ConstructorInitialization>())
      {
        std::vector<std::shared_ptr<program::Expression>> args = compiler->generateExpressions(minit.init->as<ast::ConstructorInitialization>().args);
        parent_ctor_call = generateParentConstructorCall(std::dynamic_pointer_cast<ast::ConstructorInitialization>(minit.init), args);
      }
      else
      {
        std::vector<std::shared_ptr<program::Expression>> args = compiler->generateExpressions(minit.init->as<ast::BraceInitialization>().args);
        parent_ctor_call = generateParentConstructorCall(std::dynamic_pointer_cast<ast::BraceInitialization>(minit.init), args);
      }

      // removes m-initializer from list
      std::swap(initializers.back(), initializers.at(i));
      initializers.pop_back();
      break;
    }
  }

  if (parent_ctor_call == nullptr && !compiler->classScope().parent().isNull())
  {
    std::vector<std::shared_ptr<program::Expression>> args;
    parent_ctor_call = generateParentConstructorCall(std::shared_ptr<ast::ConstructorInitialization>(), args);
  }

  // Initializating data members
  const auto & data_members = compiler->classScope().dataMembers();
  const int data_members_offset = compiler->classScope().attributesOffset();
  std::vector<std::shared_ptr<program::Statement>> members_initialization{ data_members.size(), nullptr };
  for (const auto & minit : initializers)
  {
    NameLookup lookup = compiler->resolve(minit.name);
    if (lookup.resultType() != NameLookup::DataMemberName)
      throw NotDataMember{ compiler->dpos(minit.name), compiler->dstr(minit.name) };

    if (lookup.dataMemberIndex() - data_members_offset < 0)
      throw InheritedDataMember{ compiler->dpos(minit.name), compiler->dstr(minit.name) };

    assert(lookup.dataMemberIndex() - data_members_offset < static_cast<int>(members_initialization.size()));

    const int index = lookup.dataMemberIndex() - data_members_offset;
    if (members_initialization.at(index) != nullptr)
      throw DataMemberAlreadyHasInitializer{ compiler->dpos(minit.name), compiler->dstr(minit.name) };

    const auto & dm = data_members.at(index);

    std::shared_ptr<program::Expression> member_value;
    if (minit.init->is<ast::ConstructorInitialization>())
      member_value = compiler->constructValue(dm.type, std::dynamic_pointer_cast<ast::ConstructorInitialization>(minit.init));
    else
      member_value = compiler->constructValue(dm.type, std::dynamic_pointer_cast<ast::BraceInitialization>(minit.init));

    members_initialization[index] = program::PushDataMember::New(member_value);
  }

  for (size_t i(0); i < members_initialization.size(); ++i)
  {
    if (members_initialization[i] != nullptr)
      continue;

    const auto & dm = data_members.at(i);

    //std::shared_ptr<program::Expression> default_constructed_value = defaultConstructMember(dm.type, dm.name, dpos(ctor_decl));
    std::shared_ptr<program::Expression> default_constructed_value = compiler->constructValue(dm.type, nullptr, compiler->dpos(ctor_decl));
    members_initialization[i] = program::PushDataMember::New(default_constructed_value);
  }

  std::vector<std::shared_ptr<program::Statement>> statements;
  auto init_object = program::InitObjectStatement::New(compiler->classScope().id());
  statements.push_back(init_object);
  if (parent_ctor_call)
    statements.push_back(parent_ctor_call);
  statements.insert(statements.end(), members_initialization.begin(), members_initialization.end());
  return program::CompoundStatement::New(std::move(statements));
}

std::shared_ptr<program::CompoundStatement> ConstructorCompiler::generateDefaultConstructor()
{
  return generateHeader();
}

std::shared_ptr<program::CompoundStatement> ConstructorCompiler::generateCopyConstructor()
{
  auto this_object = compiler->generateThisAccess();
  auto other_object = program::StackValue::New(1, compiler->mStack[1].type);

  std::shared_ptr<program::Statement> parent_ctor_call;
  if (!compiler->classScope().parent().isNull())
  {
    Function parent_copy_ctor = compiler->classScope().parent().copyConstructor();
    if (parent_copy_ctor.isNull())
      throw ParentHasNoCopyConstructor{};
    else if (parent_copy_ctor.isDeleted())
      throw ParentHasDeletedCopyConstructor{};

    parent_ctor_call = program::PlacementStatement::New(this_object, parent_copy_ctor, { other_object });
  }

  // Initializating data members
  const auto & data_members = compiler->classScope().dataMembers();
  const int data_members_offset = compiler->classScope().attributesOffset();
  std::vector<std::shared_ptr<program::Statement>> members_initialization{ data_members.size(), nullptr };
  for (size_t i(0); i < data_members.size(); ++i)
  {
    const auto & dm = data_members.at(i);

    const std::shared_ptr<program::Expression> member_access = program::MemberAccess::New(dm.type, other_object, i + data_members_offset);

    const ConversionSequence conv = ConversionSequence::compute(member_access, dm.type, compiler->engine());
    if (conv == ConversionSequence::NotConvertible())
      throw DataMemberIsNotCopyable{};

    members_initialization[i] = program::PushDataMember::New(compiler->prepareFunctionArgument(member_access, dm.type, conv));
  }

  std::vector<std::shared_ptr<program::Statement>> statements;
  if (parent_ctor_call)
    statements.push_back(parent_ctor_call);
  else
    statements.push_back(program::InitObjectStatement::New(compiler->classScope().id()));
  statements.insert(statements.end(), members_initialization.begin(), members_initialization.end());
  return program::CompoundStatement::New(std::move(statements));
}

std::shared_ptr<program::CompoundStatement> ConstructorCompiler::generateMoveConstructor()
{
  const Class obj_type = compiler->classScope();

  auto this_object = compiler->generateThisAccess();
  auto other_object = program::StackValue::New(1, compiler->mStack[1].type);

  std::shared_ptr<program::Statement> parent_ctor_call;
  if (!obj_type.parent().isNull())
  {
    Function parent_move_ctor = obj_type.parent().moveConstructor();
    if (!parent_move_ctor.isNull())
    {
      if (parent_move_ctor.isDeleted())
        throw ParentHasDeletedMoveConstructor{};
      parent_ctor_call = program::PlacementStatement::New(this_object, parent_move_ctor, { other_object });
    }
    else
    {
      Function parent_copy_ctor = obj_type.parent().copyConstructor();
      if (parent_copy_ctor.isNull())
        throw ParentHasNoCopyConstructor{};
      else if (parent_copy_ctor.isDeleted())
        throw ParentHasDeletedCopyConstructor{};

      parent_ctor_call = program::PlacementStatement::New(this_object, parent_copy_ctor, { other_object });
    }
  }

  // Initializating data members
  const auto & data_members = obj_type.dataMembers();
  const int data_members_offset = obj_type.attributesOffset();
  std::vector<std::shared_ptr<program::Statement>> members_initialization{ data_members.size(), nullptr };
  for (size_t i(0); i < data_members.size(); ++i)
  {
    const auto & dm = data_members.at(i);

    const std::shared_ptr<program::Expression> member_access = program::MemberAccess::New(dm.type, other_object, i + data_members_offset);
    std::shared_ptr<program::Expression> member_value = nullptr;
    if (dm.type.isReference())
      member_value = member_access;
    else
    {
      if (dm.type.isObjectType())
      {
        Function dm_move_ctor = compiler->engine()->getClass(dm.type).moveConstructor();
        if (!dm_move_ctor.isNull())
        {
          if (dm_move_ctor.isDeleted())
            throw DataMemberIsNotMovable{};
          member_value = program::ConstructorCall::New(dm_move_ctor, { member_access });
        }
        else
        {
          Function dm_copy_ctor = compiler->engine()->getClass(dm.type).copyConstructor();
          if (dm_copy_ctor.isNull() || dm_copy_ctor.isDeleted())
            throw DataMemberIsNotMovable{};
          member_value = program::ConstructorCall::New(dm_copy_ctor, { member_access });
        }
      }
      else
      {
        const ConversionSequence conv = ConversionSequence::compute(member_access, dm.type, compiler->engine());
        if (conv == ConversionSequence::NotConvertible())
          throw DataMemberIsNotCopyable{};

        member_value = compiler->prepareFunctionArgument(member_access, dm.type, conv);
      }
    }

    members_initialization[i] = program::PushDataMember::New(member_value);
  }

  std::vector<std::shared_ptr<program::Statement>> statements;
  if (parent_ctor_call)
    statements.push_back(parent_ctor_call);
  else
    statements.push_back(program::InitObjectStatement::New(obj_type.id()));
  statements.insert(statements.end(), members_initialization.begin(), members_initialization.end());
  return program::CompoundStatement::New(std::move(statements));
}

void ConstructorCompiler::checkNarrowingConversions(const std::vector<ConversionSequence> & convs, const std::vector<std::shared_ptr<program::Expression>> & args, const Prototype & proto)
{
  for (size_t i(0); i < convs.size(); ++i)
  {
    const auto & c = convs.at(i);
    if (c.isNarrowing())
      throw NarrowingConversionInBraceInitialization{compiler->dstr(args.at(i)->type()), compiler->dstr(proto.argv(i))};
  }
}

OverloadResolution ConstructorCompiler::getDelegateConstructor(std::vector<std::shared_ptr<program::Expression>> & args)
{
  const std::vector<Function> & ctors = compiler->classScope().constructors();
  OverloadResolution resol = OverloadResolution::New(compiler->engine());
  if (!resol.process(ctors, args))
    throw NoDelegatingConstructorFound{};
  return resol;
}

std::shared_ptr<program::Statement> ConstructorCompiler::makeDelegateConstructorCall(const OverloadResolution & resol, std::vector<std::shared_ptr<program::Expression>> & args)
{
  auto object = program::StackValue::New(0, Type::ref(compiler->classScope().id()));
  Function ctor = resol.selectedOverload();
  const auto & convs = resol.conversionSequence();
  compiler->prepareFunctionArguments(args, ctor.prototype(), convs);
  return program::PlacementStatement::New(object, ctor, std::move(args));
}

std::shared_ptr<program::Statement> ConstructorCompiler::generateDelegateConstructorCall(const std::shared_ptr<ast::ConstructorInitialization> & init, std::vector<std::shared_ptr<program::Expression>> & args)
{
  return makeDelegateConstructorCall(getDelegateConstructor(args), args);
}

std::shared_ptr<program::Statement> ConstructorCompiler::generateDelegateConstructorCall(const std::shared_ptr<ast::BraceInitialization> & init, std::vector<std::shared_ptr<program::Expression>> & args)
{
  auto resol = getDelegateConstructor(args);
  checkNarrowingConversions(resol.conversionSequence(), args, resol.selectedOverload().prototype());
  return makeDelegateConstructorCall(resol, args);
}


OverloadResolution ConstructorCompiler::getParentConstructor(std::vector<std::shared_ptr<program::Expression>> & args)
{
  const std::vector<Function> & ctors = compiler->classScope().parent().constructors();
  OverloadResolution resol = OverloadResolution::New(compiler->engine());
  if (!resol.process(ctors, args))
    throw CouldNotFindValidBaseConstructor{};
  return resol;
}

std::shared_ptr<program::Statement> ConstructorCompiler::makeParentConstructorCall(const OverloadResolution & resol, std::vector<std::shared_ptr<program::Expression>> & args)
{
  auto object = program::StackValue::New(0, Type::ref(compiler->classScope().id()));
  Function ctor = resol.selectedOverload();
  const auto & convs = resol.conversionSequence();
  compiler->prepareFunctionArguments(args, ctor.prototype(), convs);
  return program::PlacementStatement::New(object, ctor, std::move(args));
}

std::shared_ptr<program::Statement> ConstructorCompiler::generateParentConstructorCall(const std::shared_ptr<ast::ConstructorInitialization> & init, std::vector<std::shared_ptr<program::Expression>> & args)
{
  return makeParentConstructorCall(getParentConstructor(args), args);
}

std::shared_ptr<program::Statement> ConstructorCompiler::generateParentConstructorCall(const std::shared_ptr<ast::BraceInitialization> & init, std::vector<std::shared_ptr<program::Expression>> & args)
{
  auto resol = getParentConstructor(args);
  checkNarrowingConversions(resol.conversionSequence(), args, resol.selectedOverload().prototype());
  return makeParentConstructorCall(resol, args);
}

} // namespace compiler

} // namespace script
