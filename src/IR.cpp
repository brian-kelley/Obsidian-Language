#include "IR.cpp"
#include "Subroutine.hpp"

namespace IR
{
  //addStatement() will save, modify and restore these as needed
  static Label* breakLabel = nullptr;
  static Label* continueLabel = nullptr;

  SubroutineIR::SubroutineIR(Subroutine* subr)
  {
    //create the IR instructions for the whole body
    addStatement(subr->body);
  }

  void SubroutineIR::addStatement(Statement* s)
  {
    //empty statements allowed in some places
    if(!s)
      return;
    if(Block* b = dynamic_cast<Block*>(s))
    {
      for(auto stmt : b->stmts)
      {
        addStatement(stmt);
      }
    }
    else if(Assign* a = dynamic_cast<Assign*>(s))
    {
      stmts.push_back(new AssignIR(a->lvalue, a->rvalue));
    }
    else if(CallStmt* cs = dynamic_cast<CallStmt*>(s))
    {
      stmts.push_back(new CallIR(cs->eval));
    }
    else if(ForC* fc = dynamic_cast<ForC*>(s))
    {
      addForC(fc);
    }
    else if(ForRange* fr = dynamic_cast<ForRange*>(s))
    {
      addForRange(fr);
    }
    else if(ForArray* fa = dynamic_cast<ForArray*>(s))
    {
      addForArray(fa);
    }
    else if(While* w = dynamic_cast<While*>(s))
    {
    }
    else if(If* i = dynamic_cast<If*>(s))
    {
    }
    else if(Return* r = dynamic_cast<Return*>(s))
    {
    }
    else if(Break* brk = dynamic_cast<Break*>(s))
    {
    }
    else if(Continue* c = dynamic_cast<Continue*>(s))
    {
    }
    else if(Print* p = dynamic_cast<Print*>(s))
    {
    }
    else if(Assertion* ass = dynamic_cast<Assertion*>(s))
    {
    }
    else if(Switch* sw = dynamic_cast<Switch*>(s))
    {
    }
    else if(Match* ma = dynamic_cast<Match*>(s))
    {
      INTERNAL_ERROR("Match isn't supported yet.");
    }
    INTERNAL_ERROR("Need to implement a statement type in IR.\n");
  }

  void SubroutineIR::addForC(ForC* fc)
  {
    addStatement(fc->init);
    auto savedBreak = breakLabel;
    auto savedCont = continueLabel;
    Label* top = new Label;     //top: just before condition eval
    Label* mid = new Label;     //mid: just before incr (continue)
    Label* bottom = new Label;  //bottom: just after loop (break)
    breakLabel = bottom;
    continueLabel = mid;
    stmts.push_back(top);
    //here the condition is evaluated
    //false: jump to bottom, otherwise continue
    stmts.push_back(new CondJump(fc->cond, bottom));
    addStatement(fc->inner);
    stmts.push_back(mid);
    addStatement(fc->increment);
    stmts.push_back(new Jump(top));
    stmts.push_back(bottom);
    //restore previous break/continue labels
    breakLabel = savedBreak;
    continueLabel = savedCont;
  }

  void SubroutineIR::addForRange(ForRange* fr)
  {
    //generate initialization assign (don't need an AST assign)
    Expression* counterExpr = new VarExpr(fr->counter);
    counterExpr->finalResolve();
    Expression* counterP1 = new BinaryArith(counterExpr, PLUS, new IntLiteral(1));
    counterP1->finalResolve();
    Expression* cond = new BinaryArith(counterExpr, CMPL, fr->end);
    cond->finalResolve();
    //init
    stmts.push_back(new AssignIR(counterExpr, fr->begin));
    auto savedBreak = breakLabel;
    auto savedCont = continueLabel;
    Label* top = new Label;     //just before condition eval
    Label* mid = new Label;     //before increment (continue)
    Label* bottom = new Label;  //just after loop (break)
    stmts.push_back(top);
    //condition eval and possible break
    stmts.push_back(new CondJump(cond, bottom));
    addStatement(fr->inner);
    stmts.push_back(mid);
    addStatement(new AssignIR(counterExpr, counterP1));
    stmts.push_back(new Jump(top));
    stmts.push_back(bottom);
    //restore previous break/continue labels
    breakLabel = savedBreak;
    continueLabel = savedCont;
  }

  void SubroutineIR::addForArray(ForArray* fa)
  {
    //generate a standard 0->n loop for each dimension
    //break corresponds to jump from outermost loop,
    //but continue just jumps to innermost increment
    vector<Label*> topLabels;
    vector<Label*> midLabels;
    vector<Label*> bottomLabels;
    //how many loops/counters there are
    int n = fa->counters.size();
    for(int i = 0; i < n; i++)
    {
      topLabels.push_back(new Label);
      midLabels.push_back(new Label);
      bottomLabels.push_back(new Label);
    }
    //subarrays: previous subarray indexed with loop counter
    //array[i] is the array traversed in loop of depth i
    vector<Expression*> subArrays(n, nullptr);
    Expression* zeroLong = new Converted(new IntLiteral(0), primitives[Prim::LONG]);
    zeroLong->finalResolve();
    Expression* oneLong = new Converted(new IntLiteral(1), primitives[Prim::LONG]);
    oneLong->finalResolve();
    subArrays[0] = fa->arr;
    for(int i = 1; i < n; i++)
    {
      subArray[i] = new Indexed(subArray[i - 1], new VarExpr(fa->counters[i]));
      subArray[i]->finalResolve();
    }
    vector<Expression*> dims(n, nullptr);
    for(int i = 0; i < n; i++)
    {
      dims[i] = new ArrayLength(subArrays[i]);
      dims[i]->finalResolve();
    }
    for(int i = 0; i < n; i++)
    {
      //initialize loop i counter with 0
      //convert "int 0" to counter type if needed
      stmts.push_back(new AssignIR(new VarExpr(fa->counters[i])), zeroLong);
      stmts.push_back(topLabels[i]);
      //compare against array dim: if false, break from loop i
      Expression* cond = new BinaryArith(fa->counters[i], CMPL, dims[i]);
      cond->finalResolve();
      stmts.push_back(new CondJump(cond, bottomLabels[i]));
    }
    //update iteration variable before executing inner body
    VarExpr* iterVar = new VarExpr(fa->iter);
    iterVar->finalResolve();
    Indexed* iterValue = new Indexed(subArrays.back(), new VarExpr(fa->counters.back()));
    iterValue->finalResolve();
    stmts.push_back(new AssignIR(iterVar, iterValue));
    //add user body
    //user break stmts break from whole ForArray
    //user continue just goes to top of innermost
    Label* savedBreak = breakLabel;
    Label* savedCont = continueLabel;
    breakLabel = bottomLabels.front();
    continueLabel = midLabels.back();
    addStatement(fa->inner);
    breakLabel = savedBreak;
    continueLabel = savedCont;
    //in reverse order, add the loop increment/closing
    for(int i = n - 1; i >= 0; i--)
    {
      stmts.push_back(midLabels[i]);
      Expression* counterIncremented = new BinaryArith(iterVar, PLUS, oneLong);
      counterIncremented->finalResolve();
      stmts.push_back(new AssignIR(new VarExpr(fa->counters[i], counterIncremented)));
      stmts.push_back(new Jump(topLabels[i]));
      stmts.push_back(bottomLabels[i]);
    }
  }
}

