#include "IR.hpp"
#include "Expression.hpp"
#include "Variable.hpp"
#include "Scope.hpp"
#include "IRDebug.hpp"
#include "ConstantProp.hpp"
#include "DeadCodeElim.hpp"
#include "JumpThreading.hpp"
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

  void optimizeIR()
  {
    IRDebug::dumpIR("unoptimized.dot");
    findGlobalConstants();
    for(auto& s : ir)
    {
      constantFold(s.second);
      int sweeps = 0;
      bool update = true;
      while(update)
      {
        update = false;
        //fold all constants 
        //this does constant propagation and folding of VarExprs
        update = constantPropagation(s.first) || update;
        //update = jumpThreading(s.second) || update;
        update = deadCodeElim(s.second) || update;
        update = simplifyCFG(s.second) || update;
        sweeps++;
      }
      cout << "Subroutine " << s.first->name << " optimized in " << sweeps << " passes.\n";
    }
    IRDebug::dumpIR("optimized.dot");
  }

  //addStatement() will save, modify and restore these as needed
  //(using the call stack to respect nested control structures)
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
    blockStarts.clear();
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
    //(several of these cases overlap naturally)
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
    for(size_t i = 0; i < blocks.size(); i++)
    {
      auto bb = blocks[i];
      bb->index = i;
      blockStarts[bb->start] = bb;
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
    else if(Switch* sw = dynamic_cast<Switch*>(s))
    {
      addSwitch(sw);
    }
    else if(Match* ma = dynamic_cast<Match*>(s))
    {
      addMatch(ma);
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
    counterExpr->resolve();
    Expression* counterP1 = new BinaryArith(counterExpr, PLUS, new IntConstant((int64_t) 1));
    counterP1->resolve();
    Expression* cond = new BinaryArith(counterExpr, CMPL, fr->end);
    cond->resolve();
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
    Expression* zeroLong = new IntConstant((int64_t) 0);
    zeroLong->resolve();
    Expression* oneLong = new IntConstant((int64_t) 1);
    oneLong->resolve();
    subArrays[0] = fa->arr;
    for(int i = 1; i < n; i++)
    {
      subArrays[i] = new Indexed(subArrays[i - 1], new VarExpr(fa->counters[i]));
      subArrays[i]->resolve();
    }
    vector<Expression*> dims(n, nullptr);
    for(int i = 0; i < n; i++)
    {
      dims[i] = new ArrayLength(subArrays[i]);
      dims[i]->resolve();
    }
    for(int i = 0; i < n; i++)
    {
      //initialize loop i counter with 0
      //convert "int 0" to counter type if needed
      stmts.push_back(new AssignIR(new VarExpr(fa->counters[i]), zeroLong));
      stmts.push_back(topLabels[i]);
      //compare against array dim: if false, break from loop i
      Expression* cond = new BinaryArith(new VarExpr(fa->counters[i]), CMPL, dims[i]);
      cond->resolve();
      stmts.push_back(new CondJump(cond, bottomLabels[i]));
    }
    //update iteration variable before executing inner body
    VarExpr* iterVar = new VarExpr(fa->iter);
    iterVar->resolve();
    Indexed* iterValue = new Indexed(subArrays.back(), new VarExpr(fa->counters.back()));
    iterValue->resolve();
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
      counterIncremented->resolve();
      stmts.push_back(new AssignIR(new VarExpr(fa->counters[i]), counterIncremented));
      stmts.push_back(new Jump(topLabels[i]));
      stmts.push_back(bottomLabels[i]);
    }
  }

  void SubroutineIR::addSwitch(Switch* sw)
  {
    auto savedBreak = breakLabel;
    breakLabel = new Label;
    size_t numStmts = sw->block->stmts.size();
    size_t caseIndex = 0;
    vector<Label*> caseLabels;
    for(size_t i = 0; i < sw->caseLabels.size(); i++)
      caseLabels.push_back(new Label);
    Label* defaultLabel = new Label;
    cout << "Switch case values:\n";
    for(size_t i = 0; i < numStmts; i++)
    {
      while(caseIndex < sw->caseLabels.size() &&
          sw->caseLabels[caseIndex] == i)
      {
        //add the label
        stmts.push_back(caseLabels[caseIndex]);
        Expression* compareExpr = new BinaryArith(
            sw->switched, CMPEQ, sw->caseValues[caseIndex]);
        compareExpr->resolve();
        //this case branches to the next case, or the default
        Label* branch = nullptr;
        if(caseIndex == sw->caseLabels.size() - 1)
          branch = defaultLabel;
        else
          branch = caseLabels[caseIndex + 1];
        stmts.push_back(new CondJump(compareExpr, branch));
        caseIndex++;
      }
      //default is just a label (needs no comparison)
      if(sw->defaultPosition == i)
        stmts.push_back(defaultLabel);
      addStatement(sw->block->stmts[i]);
    }
    stmts.push_back(breakLabel);
    breakLabel = savedBreak;
  }

  void SubroutineIR::addMatch(Match* ma)
  {
    //match is like a switch, except add jump to break after each case
    Label* endLabel = new Label;
    size_t numCases = ma->cases.size();
    vector<Label*> caseLabels;
    for(size_t i = 0; i < numCases; i++)
      caseLabels.push_back(new Label);
    for(size_t i = 0; i < numCases; i++)
    {
      stmts.push_back(caseLabels[i]);
      Expression* compareExpr = new IsExpr(ma->matched, ma->types[i]);
      compareExpr->resolve();
      Label* branch = nullptr;
      if(i == numCases - 1)
        branch = endLabel;
      else
        branch = caseLabels[i + 1];
      stmts.push_back(new CondJump(compareExpr, branch));
      //assign the "as" expr to the case variable
      VarExpr* caseVar = new VarExpr(ma->caseVars[i]);
      caseVar->resolve();
      AsExpr* caseVal = new AsExpr(ma->matched, ma->types[i]);
      caseVal->resolve();
      stmts.push_back(new AssignIR(caseVar, caseVal));
      addStatement(ma->cases[i]);
      //jump to the match end (if not the last case)
      if(i != numCases - 1)
        stmts.push_back(new Jump(endLabel));
    }
    stmts.push_back(endLabel);
  }

  //is target reachable from root?
  bool SubroutineIR::reachable(BasicBlock* root, BasicBlock* target)
  {
    if(root == target)
      return true;
    //a block is "reached" as soon as it is added to the visit queue
    vector<bool> reached(blocks.size(), false);
    stack<BasicBlock*> toVisit;
    toVisit.push(root);
    reached[root->index] = true;
    while(!toVisit.empty())
    {
      BasicBlock* process = toVisit.top();
      toVisit.pop();
      for(auto out : process->out)
      {
        if(!reached[out->index])
        {
          if(out == target)
            return true;
          reached[out->index] = true;
          toVisit.push(out);
        }
      }
    }
    return false;
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
    //false condition means branch taken,
    //true means fall through!
    os << "If " << cj->cond << " goto " << stmt->intLabel + 1 << " else " << cj->taken->intLabel;
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

