#include "IR.hpp"
#include "Expression.hpp"
#include "Scope.hpp"
#include <algorithm>

extern Module* global;

IR::Nop* nop = new IR::Nop;

namespace IR
{
  map<Subroutine*, SubroutineIR*> ir;

  //walk the AST and build independent IR for each subroutine
  void buildIR()
  {
    stack<Scope*> searchStack;
    searchStack.push(global->scope);
    while(!searchStack.empty())
    {
      Scope* scope = searchStack.top();
      searchStack.pop();
      for(auto& name : scope->names)
      {
        if(name.second.kind == Name::SUBROUTINE)
        {
          Subroutine* subr = (Subroutine*) name.second.item;
          ir[subr] = new SubroutineIR(subr);
        }
      }
      for(auto child : scope->children)
      {
        searchStack.push(child);
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
    for(size_t i = 0; i < stmts.size(); i++)
      stmts[i]->intLabel = i;
    buildCFG();
  }

  void SubroutineIR::buildCFG()
  {
    blocks.clear();
    //remove no-ops
    auto newEnd = std::remove_if(
        stmts.begin(), stmts.end(),
        [](StatementIR* s) {return dynamic_cast<Nop*>(s);});
    stmts.erase(newEnd, stmts.end());
    //renumber statements
    for(size_t i = 0; i < stmts.size(); i++)
    {
      stmts[i]->intLabel = i;
    }
    //create basic blocks: count non-label statements and detect boundaries
    //BBs start at jump targets (labels/after cond jump), after return, after jump
    //several of these cases overlap naturally
    vector<size_t> boundaries;
    boundaries.push_back(0);
    for(size_t i = 0; i < stmts.size(); i++)
    {
      if(dynamic_cast<Label*>(stmts[i]) ||
         (i > 0 && dynamic_cast<CondJump*>(stmts[i - 1])) ||
         (i > 0 && dynamic_cast<ReturnIR*>(stmts[i - 1])) ||
         (i > 0 && dynamic_cast<Jump*>(stmts[i - 1])))
      {
        boundaries.push_back(i);
      }
    }
    boundaries.push_back(stmts.size());
    //remove duplicates from boundaries (don't want 0-stmt blocks)
    boundaries.erase(std::unique(boundaries.begin(), boundaries.end()), boundaries.end());
    //construct BBs (no edges yet, but remember first stmt in each)
    map<StatementIR*, BasicBlock*> leaders;
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
      auto last = stmts[blocks[i]->end - 1];
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
        //all others fall-through to next block
        blocks[i]->link(blocks[i + 1]);
      }
    }
  }

  void SubroutineIR::addStatement(Statement* s)
  {
    //empty statements allowed in some places (they produce no IR)
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
      stmts.push_back(new CallIR(cs));
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
      auto savedBreak = breakLabel;
      breakLabel = bottom;
      stmts.push_back(top);
      stmts.push_back(new CondJump(w->condition, bottom));
      addStatement(w->body);
      stmts.push_back(new Jump(top));
      stmts.push_back(bottom);
      breakLabel = savedBreak;
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
    else if(dynamic_cast<Break*>(s))
    {
      stmts.push_back(new Jump(breakLabel));
    }
    else if(dynamic_cast<Continue*>(s))
    {
      stmts.push_back(new Jump(continueLabel));
    }
    else if(Print* p = dynamic_cast<Print*>(s))
    {
      stmts.push_back(new PrintIR(p));
    }
    else if(Assertion* assertion = dynamic_cast<Assertion*>(s))
    {
      stmts.push_back(new AssertionIR(assertion));
    }
    else if(/*Switch* sw =*/ dynamic_cast<Switch*>(s))
    {
      cout << "Switch isn't supported yet.\n";
      INTERNAL_ERROR;
    }
    else if(/*Match* ma =*/ dynamic_cast<Match*>(s))
    {
      cout << "Match isn't supported yet.\n";
      INTERNAL_ERROR;
    }
    else
    {
      cout << "Need to implement a statement type in IR: " << typeid(s).name() << '\n';
      INTERNAL_ERROR;
    }
  }

  set<Variable*> SubroutineIR::getReads(BasicBlock* bb)
  {
    set<Variable*> reads;
    for(int i = bb->start; i < bb->end; i++)
    {
      auto inputs = stmts[i]->getInput();
      for(auto input : inputs)
      {
        auto exprReads = input->getReads();
        reads.insert(exprReads.begin(), exprReads.end());
      }
    }
    return reads;
  }

  set<Variable*> SubroutineIR::getWrites(BasicBlock* bb)
  {
    set<Variable*> writes;
    for(int i = bb->start; i < bb->end; i++)
    {
      auto outputs = stmts[i]->getOutput();
      for(auto output : outputs)
      {
        auto exprWrites = output->getWrites();
        writes.insert(exprWrites.begin(), exprWrites.end());
      }
    }
    return writes;
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
    stmts.push_back(new CondJump(fc->condition, bottom));
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
    stmts.push_back(new AssignIR(counterExpr, counterP1));
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
    Expression* zeroLong = new Converted(new IntLiteral(0ull), primitives[Prim::LONG]);
    zeroLong->finalResolve();
    Expression* oneLong = new Converted(new IntLiteral(1ull), primitives[Prim::LONG]);
    oneLong->finalResolve();
    subArrays[0] = fa->arr;
    for(int i = 1; i < n; i++)
    {
      subArrays[i] = new Indexed(subArrays[i - 1], new VarExpr(fa->counters[i]));
      subArrays[i]->finalResolve();
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
      stmts.push_back(new AssignIR(new VarExpr(fa->counters[i]), zeroLong));
      stmts.push_back(topLabels[i]);
      //compare against array dim: if false, break from loop i
      Expression* cond = new BinaryArith(new VarExpr(fa->counters[i]), CMPL, dims[i]);
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
      stmts.push_back(new AssignIR(new VarExpr(fa->counters[i]), counterIncremented));
      stmts.push_back(new Jump(topLabels[i]));
      stmts.push_back(bottomLabels[i]);
    }
  }
}

ostream& operator<<(ostream& os, IR::StatementIR* stmt)
{
  using namespace IR;
  if(auto ai = dynamic_cast<AssignIR*>(stmt))
  {
    os << ai->dst << " = " << ai->src;
  }
  else if(auto call = dynamic_cast<CallIR*>(stmt))
  {
    os << call->eval->callable << '(';
    for(size_t i = 0; i < call->eval->args.size(); i++)
    {
      os << call->eval->args[i];
      if(i != call->eval->args.size() - 1)
        os << ", ";
    }
    os << ')';
  }
  else if(auto j = dynamic_cast<Jump*>(stmt))
  {
    os << "Goto " << j->dst->intLabel;
  }
  else if(auto cj = dynamic_cast<CondJump*>(stmt))
  {
    os << "If " << cj->cond << " goto " << cj->taken->intLabel << " else " << stmt->intLabel + 1;
  }
  else if(auto label = dynamic_cast<Label*>(stmt))
  {
    os << "Label " << label->intLabel;
  }
  else if(auto ret = dynamic_cast<ReturnIR*>(stmt))
  {
    os << "Return";
    if(ret->expr)
      os << ' ' << ret->expr;
  }
  else if(auto p = dynamic_cast<PrintIR*>(stmt))
  {
    os << "Print(";
    for(size_t i = 0; i < p->exprs.size(); i++)
    {
      os << p->exprs[i];
      if(i != p->exprs.size() - 1)
        os << ", ";
    }
    os << ')';
  }
  else if(auto assertion = dynamic_cast<AssertionIR*>(stmt))
  {
    os << "Assert " << assertion->asserted;
  }
  else if(dynamic_cast<Nop*>(stmt))
  {
    os << "No-op";
  }
  else
  {
    INTERNAL_ERROR;
  }
  return os;
}

