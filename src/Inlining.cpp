#include "Inlining.hpp"
#include "IR.hpp"
#include "Variable.hpp"
#include "Expression.hpp"

using namespace IR;

//Counts the number of inlines done, so that
//unique placeholder var names can be generated
static int inlineCounter = 0;

struct Inliner
{
  //Inline call into into_ at stmt index pos.
  //Return value (if any) is put into ret_ (if ret_ is not null)
  Inliner(CallExpr* call, SubroutineIR* into_, int pos, Expression* ret_)
    : into(into_), endLabel(new Label), ret(ret_)
  {
    SubroutineExpr* callable = dynamic_cast<SubroutineExpr*>(call->callable);
    INTERNAL_ASSERT(callable);
    auto subr = callable->subr;
    //can't inline external (C) subroutines
    INTERNAL_ASSERT(subr);
    auto subrIR = IR::ir[subr];
    thisValue = callable->thisObject;
    if(thisValue)
    {
      INTERNAL_ASSERT(!thisValue->hasSideEffects());
    }
    cout << "*** Inlining call to " << subr->name <<
      " at " << call->printLocation() << '\n';
    //Make a mirror of each local variable in the inlined subroutine
    //These are added to the IR's variable table but not any AST scope
    for(auto v : subrIR->vars)
    {
      Variable* mirror = new Variable(
          v->name + "_inlined" + to_string(inlineCounter) + "__", v->type);
      mirror->resolve();
      cout << "Created mirror variable " << mirror->name << " for " << v->name << '\n';
      varTranslation[v] = mirror;
      into->vars.insert(mirror);
    }
    vector<StatementIR*> newStmts;
    //assign argument values to parameters mirrors
    for(size_t i = 0; i < call->args.size(); i++)
    {
      VarExpr* origParam = new VarExpr(subr->args[i]);
      origParam->resolve();
      newStmts.push_back(new AssignIR(translateExpr(origParam), call->args[i]));
    }
    for(auto stmt : subrIR->stmts)
    {
      translateStmt(newStmts, stmt);
    }
    newStmts.push_back(endLabel);
    into->stmts.insert(into->stmts.begin() + pos, newStmts.begin(), newStmts.end());
    inlineCounter++;
    into->buildCFG();
  }

  //Deep copy an IR stmt, with variables translated, and append to newStmts
  void translateStmt(vector<StatementIR*>& newStmts, StatementIR* stmt)
  {
    if(auto a = dynamic_cast<AssignIR*>(stmt))
    {
      newStmts.push_back(new AssignIR(
            translateExpr(a->dst), translateExpr(a->src)));
    }
    else if(auto e = dynamic_cast<EvalIR*>(stmt))
    {
      newStmts.push_back(new EvalIR(translateExpr(e->eval)));
    }
    else if(auto l = dynamic_cast<Label*>(stmt))
    {
      newStmts.push_back(translateLabel(l));
    }
    else if(auto j = dynamic_cast<Jump*>(stmt))
    {
      newStmts.push_back(new Jump(translateLabel(j->dst)));
    }
    else if(auto cj = dynamic_cast<CondJump*>(stmt))
    {
      newStmts.push_back(new CondJump(
            translateExpr(cj->cond), translateLabel(cj->taken)));
    }
    else if(auto r = dynamic_cast<ReturnIR*>(stmt))
    {
      if(r->expr)
      {
        if(ret)
        {
          //need to assign returned value
          newStmts.push_back(new AssignIR(ret, translateExpr(r->expr)));
        }
        else if(r->expr->hasSideEffects())
        {
          //not keeping return value, but still have to evaluate it
          newStmts.push_back(new EvalIR(translateExpr(r->expr)));
        }
      }
      newStmts.push_back(new Jump(endLabel));
    }
    else if(auto p = dynamic_cast<PrintIR*>(stmt))
    {
      newStmts.push_back(new PrintIR(translateExpr(p->expr)));
    }
    else if(auto assertion = dynamic_cast<AssertionIR*>(stmt))
    {
      newStmts.push_back(new AssertionIR(translateExpr(assertion->asserted)));
    }
  }

  //Deep copy expr, with VarExprs translated
  Expression* translateExpr(Expression* expr)
  {
    Expression* e = nullptr;
    if(auto ua = dynamic_cast<UnaryArith*>(expr))
    {
      e = new UnaryArith(ua->op, translateExpr(ua->expr));
    }
    else if(auto ba = dynamic_cast<BinaryArith*>(expr))
    {
      e = new BinaryArith(translateExpr(ba->lhs), ba->op, translateExpr(ba->rhs));
    }
    else if(auto cl = dynamic_cast<CompoundLiteral*>(expr))
    {
      vector<Expression*> mems;
      for(auto m : cl->members)
        mems.push_back(translateExpr(m));
      e = new CompoundLiteral(mems);
    }
    else if(auto ind = dynamic_cast<Indexed*>(expr))
    {
      e = new Indexed(translateExpr(ind->group), translateExpr(ind->index));
    }
    else if(auto al = dynamic_cast<ArrayLength*>(expr))
    {
      e = new ArrayLength(translateExpr(al->array));
    }
    else if(auto as = dynamic_cast<AsExpr*>(expr))
    {
      e = new AsExpr(translateExpr(as->base), as->type);
    }
    else if(auto is = dynamic_cast<IsExpr*>(expr))
    {
      e = new IsExpr(translateExpr(is->base), is->type);
    }
    else if(auto ce = dynamic_cast<CallExpr*>(expr))
    {
      vector<Expression*> args;
      for(auto a : ce->args)
      {
        args.push_back(translateExpr(a));
      }
      e = new CallExpr(translateExpr(ce->callable), args);
    }
    else if(auto ve = dynamic_cast<VarExpr*>(expr))
    {
      auto it = varTranslation.find(ve->var);
      if(it != varTranslation.end())
      {
        e = new VarExpr(it->second);
        cout << "Replaced " << it->first->name << " with " << it->second->name << '\n';
      }
    }
    else if(auto conv = dynamic_cast<Converted*>(expr))
    {
      e = new Converted(translateExpr(conv->value), conv->type);
    }
    else if(auto te = dynamic_cast<ThisExpr*>(expr))
    {
      e = thisValue;
    }
    //no others can depend on variables
    if(!e)
      e = expr->copy();
    else
      e->resolve();
    return e;
  }

  //Copy labels from inlined to be used in into
  Label* translateLabel(Label* orig)
  {
    auto it = labelTable.find(orig);
    if(it == labelTable.end())
    {
      Label* l = new Label;
      labelTable[orig] = l;
      return l;
    }
    return it->second;
  }

  SubroutineIR* into;
  //"return" is replaced by "jump endLabel"
  Label* endLabel;
  //Table of local variables in original vs. inlined
  map<Variable*, Variable*> varTranslation;
  map<Label*, Label*> labelTable;
  //Value representing "this", if the inlined call is a method
  Expression* thisValue;
  //L-value where return value will be stored
  Expression* ret;
};

void inlineCall(SubroutineIR* subr, AssignIR* assign)
{
  Expression* returnSink = assign->dst;
  auto call = dynamic_cast<CallExpr*>(assign->src);
  INTERNAL_ASSERT(call);
  int insertPoint = assign->intLabel;
  //delete the original assignment
  subr->stmts[insertPoint] = nop;
  Inliner inliner(call, subr, insertPoint, returnSink);
}
 
void inlineCall(SubroutineIR* subr, EvalIR* eval)
{
  auto call = dynamic_cast<CallExpr*>(eval->eval);
  INTERNAL_ASSERT(call);
  int insertPoint = eval->intLabel;
  subr->stmts[insertPoint] = nop;
  Inliner inliner(call, subr, insertPoint, nullptr);
}
 
