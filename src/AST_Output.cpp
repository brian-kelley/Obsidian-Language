#include "AST_Output.hpp"

#include "Subroutine.hpp"
#include "Expression.hpp"
#include "TypeSystem.hpp"
#include "Variable.hpp"
#include "Scope.hpp"
#include "Dotfile.hpp"

//The stream for writing dotfile (GraphViz) output
static Dotfile out("AST");

void outputAST(Module* tree, string filename)
{
  out.clear();
  AstOut::emitModule(tree);
  out.write(filename);
}

namespace AstOut
{

int emitModule(Module* m)
{
  int id = 0;
  if(m->name == "")
  {
    id = out.createNode("Program root");
  }
  else
  {
    id = out.createNode("Module: " + m->name);
  }
  for(auto decl : m->scope->names)
  {
    out.createEdge(id, emitName(&decl.second));
  }
  return id;
}

int emitName(Name* n)
{
  switch(n->kind)
  {
    case Name::MODULE:
      return emitModule((Module*) n->item);
    case Name::STRUCT:
      return emitStruct((StructType*) n->item);
    case Name::TYPEDEF:
      return emitAlias((AliasType*) n->item);
    case Name::SUBROUTINE:
      return emitSubroutineDecl((SubroutineDecl*) n->item);
    case Name::VARIABLE:
      return emitVariable((Variable*) n->item);
    case Name::ENUM:
      return emitEnum((EnumType*) n->item);
    case Name::SIMPLE_TYPE:
      return emitSimpleType((SimpleType*) n->item);
    default:
      INTERNAL_ERROR;
  }
  return 0;
}

int emitType(Type* t)
{
  return out.createNode(t->getName());
}

int emitStatement(Statement* s)
{
  int root = 0;
  if(Block* b = dynamic_cast<Block*>(s))
  {
    root = out.createNode("Block");
    if(b->scope->names.size())
    {
      int decls = out.createNode("Decls");
      for(auto& n : b->scope->names)
      {
        out.createEdge(decls, emitName(&n.second));
      }
      out.createEdge(root, decls);
    }
    if(b->stmts.size())
    {
      int stmts = out.createNode("Statements");
      for(auto stmt : b->stmts)
      {
        out.createEdge(stmts, emitStatement(stmt));
      }
      out.createEdge(root, stmts);
    }
  }
  else if(Assign* a = dynamic_cast<Assign*>(s))
  {
    root = out.createNode("Assign");
    out.createEdge(root, emitExpression(a->lvalue));
    out.createEdge(root, emitExpression(a->rvalue));
  }
  else if(CallStmt* cs = dynamic_cast<CallStmt*>(s))
  {
    root = emitExpression(cs->eval);
  }
  else if(ForC* fc = dynamic_cast<ForC*>(s))
  {
    root = out.createNode("For loop (C-style)");
    int outerBlock = out.createNode("Outer block");
    out.createEdge(outerBlock, emitStatement(fc->outer));
    if(fc->init)
    {
      out.createEdge(root, emitStatement(fc->init));
    }
    else
    {
      out.createEdge(root, out.createNode("(no init)"));
    }
    out.createEdge(root, emitStatement(fc->inner));
  }
  else if(ForRange* fr = dynamic_cast<ForRange*>(s))
  {
    root = out.createNode("For loop (range)");
    out.createEdge(root, emitStatement(fr->outer));
    out.createEdge(root, emitExpression(fr->begin));
    out.createEdge(root, emitExpression(fr->end));
    out.createEdge(root, emitStatement(fr->inner));
  }
  else if(ForArray* fa = dynamic_cast<ForArray*>(s))
  {
    root = out.createNode("For loop (array)");
    int outerBlock = out.createNode("Outer block");
    out.createEdge(root, outerBlock);
    out.createEdge(outerBlock, emitStatement(fa->outer));
    out.createEdge(root, emitExpression(fa->arr));
    out.createEdge(root, emitStatement(fa->inner));
  }
  else if(While* w = dynamic_cast<While*>(s))
  {
    root = out.createNode("While loop");
    out.createEdge(root, emitExpression(w->condition));
    out.createEdge(root, emitStatement(w->body));
  }
  else if(If* ifs = dynamic_cast<If*>(s))
  {
    root = out.createNode("If statement");
    out.createEdge(root, emitExpression(ifs->condition));
    out.createEdge(root, emitStatement(ifs->body));
    if(ifs->elseBody)
      out.createEdge(root, emitStatement(ifs->elseBody));
    else
      out.createEdge(root, out.createNode("(no else body)"));
  }
  else if(Return* ret = dynamic_cast<Return*>(s))
  {
    root = out.createNode("Return");
    if(ret->value)
      out.createEdge(root, emitExpression(ret->value));
  }
  else if(dynamic_cast<Break*>(s))
  {
    root = out.createNode("Break statement");
  }
  else if(dynamic_cast<Continue*>(s))
  {
    root = out.createNode("Continue statement");
  }
  else if(Print* p = dynamic_cast<Print*>(s))
  {
    root = out.createNode("Print statement");
    for(auto e : p->exprs)
    {
      out.createEdge(root, emitExpression(e));
    }
  }
  else if(Assertion* as = dynamic_cast<Assertion*>(s))
  {
    root = out.createNode("Assertion");
    out.createEdge(root, emitExpression(as->asserted));
  }
  else if(Switch* sw = dynamic_cast<Switch*>(s))
  {
    root = out.createNode("Switch");
    out.createEdge(root, emitExpression(sw->switched));
    //write the block first
    out.createEdge(root, emitStatement(sw->block));
    //describe the case labels in a single node
    Oss desc;
    for(size_t i = 0; i < sw->caseValues.size(); i++)
    {
      desc << sw->caseLabels[i] << ": " << sw->caseValues[i] << '\n';
    }
    desc << sw->defaultPosition << ": default\n";
    out.createEdge(root, out.createNode(desc.str()));
  }
  else if(Match* mat = dynamic_cast<Match*>(s))
  {
    root = out.createNode("Match");
    out.createEdge(root, emitExpression(mat->matched));
    for(size_t i = 0; i < mat->types.size(); i++)
    {
      int typeNode = emitType(mat->types[i]);
      out.createEdge(root, typeNode);
      out.createEdge(typeNode, emitStatement(mat->cases[i]));
    }
  }
  else
  {
    cout << "Haven't implemented output for a statement type at " << s->printLocation()  << "\n";
    INTERNAL_ERROR;
  }
  return root;
}

int emitExpression(Expression* e)
{
  int root = 0;
  if(UnaryArith* ua = dynamic_cast<UnaryArith*>(e))
  {
    root = out.createNode(operatorTable[ua->op]);
    out.createEdge(root, emitExpression(ua->expr));
  }
  else if(BinaryArith* ba = dynamic_cast<BinaryArith*>(e))
  {
    root = out.createNode(operatorTable[ba->op]);
    out.createEdge(root, emitExpression(ba->lhs));
    out.createEdge(root, emitExpression(ba->rhs));
  }
  else if(IntConstant* ic = dynamic_cast<IntConstant*>(e))
  {
    if(ic->type == getCharType())
      root = out.createNode("'" + generateCharDotfile((char) ic->uval) + "'");
    else if(ic->isSigned())
      root = out.createNode(to_string(ic->sval));
    else
      root = out.createNode(to_string(ic->uval));
  }
  else if(FloatConstant* fc = dynamic_cast<FloatConstant*>(e))
  {
    char buf[32];
    sprintf(buf, "%#f", fc->dp);
    root = out.createNode(buf);
  }
  else if(BoolConstant* bc = dynamic_cast<BoolConstant*>(e))
  {
    if(bc->value)
      root = out.createNode("true");
    else
      root = out.createNode("false");
  }
  else if(CompoundLiteral* compLit = dynamic_cast<CompoundLiteral*>(e))
  {
    if(compLit->type == getStringType())
    {
      //print string with all characters fully escaped
      Oss oss;
      oss << "\\\"";
      for(auto m : compLit->members)
      {
        IntConstant* charElem = dynamic_cast<IntConstant*>(m);
        INTERNAL_ASSERT(charElem);
        oss << generateCharDotfile((char) charElem->uval);
      }
      oss << "\\\"";
      root = out.createNode(oss.str());
    }
    else
    {
      root = out.createNode("compound literal");
      for(auto expr : compLit->members)
      {
        out.createEdge(root, emitExpression(expr));
      }
    }
  }
  else if(Indexed* in = dynamic_cast<Indexed*>(e))
  {
    root = out.createNode("Index");
    out.createEdge(root, emitExpression(in->group));
    out.createEdge(root, emitExpression(in->index));
  }
  else if(CallExpr* call = dynamic_cast<CallExpr*>(e))
  {
    root = out.createNode("Call");
    out.createEdge(root, emitExpression(call->callable));
    int args;
    if(call->args.size())
    {
      args = out.createNode("Args");
      for(auto arg : call->args)
      {
        out.createEdge(args, emitExpression(arg));
      }
    }
    else
    {
      args = out.createNode("(no args)");
    }
    out.createEdge(root, args);
  }
  else if(VarExpr* ve = dynamic_cast<VarExpr*>(e))
  {
    root = out.createNode("Variable " + ve->var->name);
  }
  else if(IsExpr* ie = dynamic_cast<IsExpr*>(e))
  {
    root = out.createNode("Is");
    out.createEdge(root, emitExpression(ie->base));
    out.createEdge(root, emitType(ie->option));
  }
  else if(AsExpr* ae = dynamic_cast<AsExpr*>(e))
  {
    root = out.createNode("As");
    out.createEdge(root, emitExpression(ae->base));
    out.createEdge(root, emitType(ae->type));
  }
  else if(NewArray* na = dynamic_cast<NewArray*>(e))
  {
    root = out.createNode("Array allocation");
    out.createEdge(root, emitType(na->elem));
    for(auto dim : na->dims)
    {
      out.createEdge(root, emitExpression(dim));
    }
  }
  else if(Converted* c = dynamic_cast<Converted*>(e))
  {
    root = out.createNode("Conversion");
    out.createEdge(root, emitExpression(c->value));
    out.createEdge(root, emitType(c->type));
  }
  else if(ArrayLength* al = dynamic_cast<ArrayLength*>(e))
  {
    root = out.createNode("Array length");
    out.createEdge(root, emitExpression(al->array));
  }
  else if(dynamic_cast<ThisExpr*>(e))
  {
    root = out.createNode("this");
  }
  else if(auto sic = dynamic_cast<SimpleConstant*>(e))
  {
    root = out.createNode(sic->st->name);
  }
  else if(auto uc = dynamic_cast<UnionConstant*>(e))
  {
    root = out.createNode("Union constant of " + e->type->getName());
    out.createEdge(root, emitExpression(uc->value));
  }
  else if(auto sm = dynamic_cast<StructMem*>(e))
  {
    root = out.createNode("Struct member");
    out.createEdge(root, emitExpression(sm->base));
    if(sm->member.is<Variable*>())
      out.createEdge(root, emitVariable(sm->member.get<Variable*>()));
    else
      out.createEdge(root, out.createNode(
            "Subroutine " + sm->member.get<Subroutine*>()->decl->name));
  }
  else if(auto se = dynamic_cast<SubroutineExpr*>(e))
  {
    auto subr = dynamic_cast<Subroutine*>(se->subr);
    auto exSubr = dynamic_cast<ExternalSubroutine*>(se->subr);
    if(subr)
      root = out.createNode("Subroutine " + subr->decl->name);
    else
      root = out.createNode("External subroutine " + exSubr->decl->name);
  }
  else if(auto ee = dynamic_cast<EnumExpr*>(e))
  {
    root = out.createNode("Enum value " + ee->value->name);
  }
  else
  {
    cout << "Didn't implement emitExpression for type " << typeid(*e).name() << '\n';
    //resolved AST can't contain any UnresolvedExprs
    INTERNAL_ERROR;
  }
  return root;
}

int emitStruct(StructType* s)
{
  int root = out.createNode("Struct " + s->name);
  //A struct is just a collection of decls, like a module
  for(auto decl : s->scope->names)
  {
    if(auto varMember = dynamic_cast<Variable*>(decl.second.item))
    {
      //find the index of the member
      size_t i = 0;
      for(; i < s->members.size(); i++)
      {
        if(s->members[i] == varMember)
          break;
      }
      if(s->composed[i])
      {
        int varRoot = out.createNode("Composed variable " + varMember->name);
        out.createEdge(root, varRoot);
        out.createEdge(varRoot, emitExpression(varMember->initial));
        continue;
      }
    }
    out.createEdge(root, emitName(&decl.second));
  }
  return root;
}

int emitAlias(AliasType* a)
{
  return out.createNode("Alias " + a->name + " = " + a->actual->getName());
}

int emitSubroutineDecl(SubroutineDecl* s)
{
  int root = out.createNode("Subroutine " + s->name);
  for(auto o : s->overloads)
  {
    auto subr = dynamic_cast<Subroutine*>(o);
    auto exSubr = dynamic_cast<ExternalSubroutine*>(o);
    if(subr)
      out.createEdge(root, emitSubroutine(subr));
    else
      out.createEdge(root, emitExternSubroutine(exSubr));
  }
  return root;
}

int emitSubroutine(Subroutine* s)
{
  int root = out.createNode("Subroutine " + s->name());
  int params = out.createNode("Parameters");
  out.createEdge(root, params);
  for(auto p : s->params)
  {
    out.createEdge(root, emitVariable(p));
  }
  out.createEdge(root, emitStatement(s->body));
  return root;
}

int emitExternSubroutine(ExternalSubroutine* s)
{
  int root = out.createNode("External subroutine " + s->name());
  int args = out.createNode("Args");
  out.createEdge(root, args);
  for(size_t i = 0; i < s->type->paramTypes.size(); i++)
  {
    out.createEdge(args, out.createNode(s->type->paramTypes[i]->getName() + ' ' + s->paramNames[i]));
  }
  out.createEdge(root, out.createNode(s->c));
  return root;
}

int emitVariable(Variable* v)
{
  int root = out.createNode("Variable " + v->name);
  if(v->initial)
    out.createEdge(root, emitExpression(v->initial));
  else
    out.createEdge(root, out.createNode("(default initialized)"));
  return root;
}

int emitEnum(EnumType* e)
{
  int root = out.createNode("Enum " + e->name);
  for(auto ec : e->values)
  {
    string printedValue = ec->isSigned ?
      to_string((int64_t) ec->value) : to_string(ec->value);
    out.createEdge(root, out.createNode(ec->name + " = " + printedValue));
  }
  return root;
}

int emitSimpleType(SimpleType* s)
{
  return out.createNode("Type " + s->name);
}

} //namespace AstOut

