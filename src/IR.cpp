#include "IR.cpp"
#include "Subroutine.hpp"
#include "Scope.hpp"
#include <algorithm>

extern Module* global;

namespace IR
{
  map<Subroutine*, SubroutineIR*> ir;

  //walk the AST and build independent IR for each subroutine
  void buildIR()
  {
    stack<Scope*> searchStack;
    searchStack.push(global);
    while(!searchStack.empty())
    {
      Scope* scope = searchStack.top();
      searchStack.pop();
      for(auto& name : scope->names)
      {
        if(name.kind == Name::SUBROUTINE)
        {
          Subroutine* subr = (Subroutine*) name.item;
          ir[subr] = new SubroutineIR(subr);
        }
      }
      for(auto child : scope->children)
      {
        searchStack.push_back(child);
      }
    }
  }

  //addStatement() will save, modify and restore these as needed
  static Label* breakLabel = nullptr;
  static Label* continueLabel = nullptr;

  SubroutineIR::SubroutineIR(Subroutine* s)
  {
    subr = s;
    //create the IR instructions for the whole body
    addStatement(subr->body);
    //create basic blocks: count non-label statements and detect boundaries
    //BBs start at jump targets (labels/after cond jump), after return, after jump
    //several of these cases overlap naturally
    vector<size_t> boundaries;
    boundaries.push_back(0);
    for(size_t i = 0; i < stmts.size(); i++)
    {
      if(dynamic_cast<Label*>(stmts[i]) ||
         (i != 0 && dynamic_cast<CondJump*>(stmts[i])) ||
         (i != 0 && dynamic_cast<ReturnIR*>(stmts[i])) ||
         (i != 0 && dynamic_cast<Jump*>(stmts[i])) ||
         (i == stmts.size() - 1))
      {
        boundaries.push_back(i);
      }
    }
    boundaries.push_back(stmts.size());
    //remove duplicates from boundaries (don't want 0-stmt blocks)
    std::erase(std::unique(boudaries.begin(), boundaries.end()),
        boundaries.end());
    //construct BBs (no edges yet, but remember first stmt in each)
    map<Label*, BasicBlock*> leaders;
    for(size_t i = 0; i < boundaries.size() - 1; i++)
    {
      blocks.push_back(new BasicBlock(boundaries[i], boundaries[i + 1]));
      leaders[stmts[boundaries[i]]] = blocks.back();
    }
    //Add edges to complete the CFG.
    //Easy since all possible jump targets
    //are leaders, and all flow stmts are ends of BBs.
    for(size_t i = 0; i < blocks.size(); i++)
    {
      auto last = stmts.back();
      if(auto cj = dynamic_cast<CondJump*>(last))
      {
        blocks[i]->link(blocks[i + 1]);
        blocks[i]->link(leaders[cj->taken]);
      }
      else if(auto j = dynamic_cast<Jump*>(last))
      {
        blocks[i]->link(leaders[j->dst]);
      }
      else if(!dynamic_cast<ReturnIR*>(last))
      {
        if(i == blocks.size() - 1)
        {
          errMsgLoc(subr, "no return at end of non-void subroutine");
        }
        //fall-through to next block
        blocks[i]->link(blocks[i + 1]);
      }
    }
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
      Label* top = new Label;
      Label* bottom = new Label;
      Label* savedBreak = breakLabel;
      stmts.push_back(top);
      stmts.push_back(new CondJump(w->condition, bottom));
      addStatement(w->body);
      stmts.push_back(new Jump(top));
      stmts.push_back(bottom);
    }
    else if(If* i = dynamic_cast<If*>(s))
    {
      if(i->elseBody)
      {
        Label* ifEnd = new Label;
        Label* elseEnd = new Label;
        stmts.push_back(new CondJump(i->condition, ifEnd));
        addStatement(i->body);
        stmts.push_back(new Jump(elseEnd));
        stmts.push_back(ifEnd);
        addStatement(i->elseBody);
        stmts.push_back(elseEnd);
      }
      else
      {
        Label* ifEnd = new Label;
        stmts.push_back(new CondJump(i->condition, ifEnd));
        addStatement(i->body);
        stmts.push_back(ifEnd);
      }
    }
    else if(Return* r = dynamic_cast<Return*>(s))
    {
      stmts.push_back(new ReturnIR(r->value));
    }
    else if(Break* brk = dynamic_cast<Break*>(s))
    {
      stmts.push_back(new Jump(breakLabel));
    }
    else if(Continue* c = dynamic_cast<Continue*>(s))
    {
      stmts.push_back(new Jump(continueLabel));
    }
    else if(Print* p = dynamic_cast<Print*>(s))
    {
      stmts.push_back(new PrintIR(p->exprs));
    }
    else if(Assertion* a = dynamic_cast<Assertion*>(s))
    {
      stmts.push_back(new AssertionIR(a->asserted));
    }
    else if(Switch* sw = dynamic_cast<Switch*>(s))
    {
      INTERNAL_ERROR("Switch isn't supported yet.");
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

  void SubroutineIR::print()
  {
    cout << "subroutine " << subr->name << '\n';
    for(auto stmt : stmts)
    {
      if(!dynamic_cast<Label*>(stmt))
        cout << "  " << stmt->intLabel << ": " << stmt->print();
    }
    cout << '\n';
  }
}

