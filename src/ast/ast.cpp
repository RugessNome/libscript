// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/ast.h"
#include "script/ast/ast_p.h"

#include "script/script.h"

namespace script
{

namespace ast
{

AST::AST()
{

}

AST::AST(const Script & s)
  : source(s.source())
  , script(s.impl())
{

}

AST::AST(const SourceFile& src)
  : source(src)
{

}

void AST::add(const std::shared_ptr<Statement> & statement)
{
  auto *scriptnode = static_cast<ScriptRootNode*>(root.get());

  scriptnode->statements.push_back(statement);
  if (statement->is<Declaration>())
    scriptnode->declarations.push_back(std::static_pointer_cast<Declaration>(statement));
}

std::string AST::text(const parser::Token & tok)
{
  return std::string(source.data() + tok.pos, tok.length);
}

SourceFile::Position AST::position(const parser::Token& tok) const
{
  SourceFile::Position pos;
  pos.pos = tok.pos;
  pos.line = tok.line;
  pos.col = tok.column;
  return pos;
}

} // namespace ast


/*!
 * \class Ast
 * \brief Represents an abstract syntax tree.
 *
 */

/*!
 * \fn Ast()
 * \brief Null-constructs an ast
 */

/*!
 * \fn Ast(const Ast &)
 * \brief Constructs a new reference to another ast
 */

/*!
 * \fn ~Ast()
 * \brief Destructor
 */

/*!
 * \fn bool isNull() const
 * \brief Returns whether the Ast is null.
 */

/*!
 * \fn SourceFile source() const
 * \brief Returns the ast source file.
 */
SourceFile Ast::source() const
{
  return d->source;
}

/*!
 * \fn const std::shared_ptr<ast::Node> & root() const
 * \brief Returns the ast root node.
 */
const std::shared_ptr<ast::Node> & Ast::root() const
{
  return d->root;
}

/*!
 * \fn bool isScript() const
 * \brief Returns whether this is the ast of a Script
 */
bool Ast::isScript() const
{
  return d->root->is<ast::ScriptRootNode>();
}

/*!
 * \fn Script script() const
 * \brief Returns the script associated with this ast.
 *
 * Note that you can call this function even if \m isScript returns false;
 * in such case, a null script is returned.
 */
Script Ast::script() const
{
  return Script{ d->script.lock() };
}

/*!
 * \fn const std::vector<std::shared_ptr<ast::Statement>> & statements() const
 * \brief Returns the top-level statements of the script
 *
 * Calling this function when \m isScript returns false is undefined behavior.
 */
const std::vector<std::shared_ptr<ast::Statement>> & Ast::statements() const
{
  return d->root->as<ast::ScriptRootNode>().statements;
}

/*!
 * \fn const std::vector<std::shared_ptr<ast::Declaration>> & declarations() const
 * \brief Returns the top-level declarations of the script
 *
 * This returns the subset of \m statements for which \c{Statement::isDeclaration()} returns true.
 *
 * Calling this function when \m isScript returns false is undefined behavior.
 */
const std::vector<std::shared_ptr<ast::Declaration>> & Ast::declarations() const
{
  return d->root->as<ast::ScriptRootNode>().declarations;
}

/*!
 * \fn bool isExpression() const
 * \brief Returns whether this is the ast of an expression
 *
 */
bool Ast::isExpression() const
{
  return d->root->is<ast::Expression>();
}

/*!
 * \fn std::shared_ptr<ast::Expression> expression() const
 * \brief Returns the expression associated with this ast
 *
 * It is safe to call this function even if \m isExpression returns false; 
 * in such case a null pointer is returned.
 */
std::shared_ptr<ast::Expression> Ast::expression() const
{
  return std::dynamic_pointer_cast<ast::Expression>(d->root);
}

} // namespace script
