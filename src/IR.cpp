#include "IR.hpp"
#include "Expression.hpp"
#include "Variable.hpp"
#include "Scope.hpp"
#include "IRDebug.hpp"
#include "ConstantProp.hpp"
#include "DeadCodeElim.hpp"
#include "JumpThreading.hpp"
#include "CSElim.hpp"
#include "Inlining.hpp"
#include "CallGraph.hpp"
#include "Memory.hpp"
#include <algorithm>

extern Module* global;

IR::Nop* nop = new IR::Nop;

namespace IR
{
  /* *************************** */
  /* Full Program Representation */
  /* *************************** */

  SubroutineIR* mainIR;
  vector<SubroutineIR*> ir;
  vector<ExternalSubroutine*> externIR;
  vector<Variable*> allGlobals;

  /* ****************** */
  /* IR Statement Types */
  /* ****************** */

  CallIR::CallIR(CallExpr* c, SubroutineIR* subr)
  {
    origCallable = c->callable;
    callableType = (CallableType*) origCallable->type;
    //if the callable's type contains a "thisObject" type,
    //the call needs a "this" expression
    thisObject = nullptr;
    SubroutineExpr* subExpr = dynamic_cast<SubroutineExpr*>(origCallable);
    if(callableType->ownerStruct)
    {
      if(subExpr)
        thisObject = subr->expandExpression(subExpr->thisObject);
      else
      {
        //indirect method call - callable must be a StructMem
        StructMem* sm = dynamic_cast<StructMem*>(origCallable);
        INTERNAL_ASSERT(sm);
        thisObject = subr->expandExpression(sm->base);
      }
    }
    //now, set up "callable"
    if(subExpr)
    {
      if(subExpr->subr)
        callable = subExpr->subr->subrIR;
      else if(subExpr)
        callable = subExpr->exSubr;
    }
    else
    {
      //some indirect call - use whole callable
      callable = subr->expandExpression(origCallable);
    }
    //expand each argument
    for(auto a : c->args)
    {
      args.push_back(subr->expandExpression(a));
    }
    //finally create a temporary to hold the return value
    //OK if the type is "void", since it's just a special
    //case of SimpleType. Doesn't actually use storage.
    output = subr->generateTemp(callableType->returnType);
  }

  bool CallIR::argBorrowed(int index)
  {
    INTERNAL_ASSERT(index >= 0);
    INTERNAL_ASSERT(index < args.size());
    if(callable.is<SubroutineIR*>())
    {
      SubroutineIR* subr = callable.get<SubroutineIR*>();
      //Memory management subsystem has already computed this
      return Memory::subrParamBorrowable(subr->params[index]);
    }
    else if(callable.is<ExternalSubroutine*>())
    {
      //ExternalSubroutine already knows which params are borrowed
      ExternalSubroutine* es = callable.get<ExternalSubroutine*>();
      return es->paramBorrowed[index];
    }
    else
      return Memory::indirectParamBorrowable(callableType, index);
  }

  ostream& CallIR::print(ostream& os) const
  {
    if(output)
    {
      os << output->var->name << " = ";
    }
    os << origCallable << "(";
    for(size_t i = 0; i < args.size(); i++)
    {
      os << args[i];
      if(i == args.size() - 1)
        os << ", ";
    }
    os << ")";
    return os;
  }

  /* *************** */
  /* IR Construction */
  /* *************** */

  //walk the AST and build independent IR for each subroutine
  void buildIR()
  {
    Scope::walk([&](Scope* scope)
    {
      for(auto& name : scope->names)
      {
        if(name.second.kind == Name::SUBROUTINE)
        {
          Subroutine* subr = (Subroutine*) name.second.item;
          SubroutineIR* subrIR = new SubroutineIR(subr);
          ir.push_back(subrIR);
          subr->subrIR = subrIR;
          if(scope == global->scope && subr->name == "main")
          {
            mainIR = ir.back();
          }
        }
        else if(name.second.kind == Name::EXTERN_SUBR)
        {
          externIR.push_back((ExternalSubroutine*) name.second.item);
        }
        else if(name.second.kind == Name::VARIABLE)
        {
          Variable* var = (Variable*) name.second.item;
          allGlobals.push_back(var);
        }
      }
    });
    INTERNAL_ASSERT(mainIR);
    //sort allGlobals by variable ID, so that they are in exact order of initialization.
    //global initial values may have side effects!
    std::sort(allGlobals.begin(), allGlobals.end(),
      [](const Variable* v1, const Variable* v2)
      {
        return v1->id < v2->id;
      });
    //main is special - first need to add statements that initialize global variables.
    //This is beneficial because constant prop, CSE, etc. will be applied to globals.
    for(auto glob : allGlobals)
    {
      //addInstruction() expands the RHS as necessary.
      mainIR->addInstruction(new AssignIR(new VarExpr(glob), glob->initial));
    }
    //now translate statements from the AST to IR, and build all CFGs
    for(auto& subrIR : ir)
    {
      subrIR->build();
    }
  }

  void optimizeIR()
  {
    IRDebug::dumpIR("IR/0-unoptimized.dot");
    findGlobalConstants();
    for(auto& s : ir)
      constantFold(s);
    IRDebug::dumpIR("IR/1-folded.dot");
    for(auto& s : ir)
      constantPropagation(s);
    IRDebug::dumpIR("IR/2-propagation.dot");
    for(auto& s : ir)
      deadCodeElim(s);
    unusedSubrElim();
    callGraph.dump("IR/call-graph.dot");
    unusedGlobalElim();
    IRDebug::dumpIR("IR/3-dce.dot");
    for(auto& s : ir)
      simplifyCFG(s);
    IRDebug::dumpIR("IR/4-simplified.dot");
    for(auto& s : ir)
      cse(s);
    IRDebug::dumpIR("IR/5-cse.dot");
    for(auto& s : ir)
      deadStoreElim(s);
    IRDebug::dumpIR("IR/6-deadstore.dot");
    IRDebug::dumpIR("IR/7-inlined.dot");
  }

  //addStatement() will save, modify and restore these as needed
  //(using the call stack to respect nested control structures)
  static Label* breakLabel = nullptr;
  static Label* continueLabel = nullptr;
  static Block* currentBlock = nullptr;

  SubroutineIR::SubroutineIR(Subroutine* s)
  {
    Node::setLocation(s);
    subr = s;
  }

  void SubroutineIR::build()
  {
    tempCounter = 0;
    //create the IR instructions for the whole body
    addStatement(subr->body);
    for(size_t i = 0; i < stmts.size(); i++)
      stmts[i]->intLabel = i;
    //DFS through scopes to list all variables
    //(only want to include variables in modules/blocks, so
    //can't use Scope::walk here)
    for(auto param : subr->params)
    {
      params.push_back(param);
      vars.push_back(param);
    }
    stack<Scope*> search;
    search.push(subr->body->scope);
    while(search.size())
    {
      Scope* process = search.top();
      search.pop();
      for(auto& n : process->names)
      {
        if(n.second.kind == Name::VARIABLE)
        {
          Variable* v = (Variable*) n.second.item;
          vars.push_back(v);
        }
      }
      for(auto child : process->children)
      {
        if(child->node.is<Block*>() || child->node.is<Module*>())
          search.push(child);
      }
    }
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
    solveDominators();
  }

  void SubroutineIR::solveDominators()
  {
    auto n = blocks.size();
    if(n == 0)
      return;
    queue<BasicBlock*> processQ;
    //process each block at least once
    for(size_t i = 0; i < n; i++)
      processQ.push(blocks[i]);
    vector<bool> queued(n, true);
    //make sure dominator sets are allocated in each block
    for(auto& b : blocks)
    {
      b->dom.resize(n);
    }
    while(processQ.size())
    {
      BasicBlock* process = processQ.front();
      processQ.pop();
      auto procInd = process->index;
      queued[procInd] = false;
      //need to save dominator set to test if any changes happen
      //it's a dense bitset so this is cheap
      auto savedDom = process->dom;
      if(procInd == 0)
      {
        //nothing dominates first block except itself
        for(size_t i = 0; i < n; i++)
          process->dom[procInd] = false;
        process->dom[procInd] = true;
      }
      else
      {
        //intersect dominators of all incoming blocks
        for(size_t i = 0; i < n; i++)
          process->dom[i] = true;
        for(auto pred : process->in)
        {
          for(size_t i = 0; i < n; i++)
            process->dom[i] = process->dom[i] && pred->dom[i];
        }
        process->dom[procInd] = true;
        //if any updates happened...
        if(process->dom != savedDom)
        {
          //Need to process all successors.
          //Don't add blocks to the queue more than once.
          //Don't need to re-add process even if it precedes itself.
          for(auto succ : process->out)
          {
            auto succInd = succ->index;
            if(succInd != procInd && !queued[succInd])
            {
              queued[succInd] = true;
              processQ.push(succ);
            }
          }
        }
      }
    }
  }

  void SubroutineIR::addStatement(Statement* s)
  {
    //empty (null) statements allowed in some places (they produce no IR)
    if(!s)
      return;
    currentBlock = s->block;
    //set the current block (for use in expandExpression)
    if(Block* b = dynamic_cast<Block*>(s))
    {
      for(auto stmt : b->stmts)
      {
        addStatement(stmt);
      }
    }
    else if(Assign* a = dynamic_cast<Assign*>(s))
    {
      //Compound assignments are split up so that every
      //AssignIR modifies exactly one variable.
      //
      //Also, the entire RHS is evaluated fully before assignment
      Block* block = a->block;
      if(auto compoundLHS = dynamic_cast<CompoundLiteral*>(a->lvalue))
      {
        if(auto compoundRHS = dynamic_cast<CompoundLiteral*>(a->rvalue))
        {
          //the RHS is also compound so generate multiple assigns
          for(size_t i = 0; i < compoundLHS->members.size(); i++)
          {
            //these subAssigns might expand again, so use addStatement
            Assign* subAssign = new Assign(block,
                compoundLHS->members[i], compoundRHS->members[i]);
            subAssign->resolve();
            addStatement(subAssign);
          }
        }
        else
        {
          //put rhs in a temp, then assign individual elements
          Variable* temp = new Variable(getTempName(), a->rvalue->type, block);
          temp->resolve();
          block->scope->addName(temp);
          auto tempSave = new Assign(block, new VarExpr(temp), a->rvalue);
          tempSave->resolve();
          addStatement(tempSave);
          VarExpr* tempVar = new VarExpr(temp);
          //now, assign individual elements
          for(size_t i = 0; i < compoundLHS->members.size(); i++)
          {
            Assign* subAssign = new Assign(block, compoundLHS->members[i],
                new Indexed(tempVar, new IntConstant((uint64_t) i)));
            subAssign->resolve();
            addStatement(subAssign);
          }
        }
      }
      else
      {
        //Simple assignment to single variable/member/indexed
        addInstruction(new AssignIR(a->lvalue, a->rvalue));
      }
    }
    else if(CallStmt* cs = dynamic_cast<CallStmt*>(s))
    {
      //Only calls with side effects need to be evaluated
      if(cs->eval->hasSideEffects())
      {
        //just evaluate the call
        expandExpression(cs->eval);
      }
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
      addInstruction(top);
      addInstruction(new CondJump(w->condition, bottom));
      addStatement(w->body);
      addInstruction(new Jump(top));
      addInstruction(bottom);
      breakLabel = savedBreak;
    }
    else if(If* i = dynamic_cast<If*>(s))
    {
      if(i->elseBody)
      {
        Label* ifEnd = new Label;
        Label* elseEnd = new Label;
        addInstruction(new CondJump(i->condition, ifEnd));
        addStatement(i->body);
        addInstruction(new Jump(elseEnd));
        addInstruction(ifEnd);
        addStatement(i->elseBody);
        addInstruction(elseEnd);
      }
      else
      {
        Label* ifEnd = new Label;
        addInstruction(new CondJump(i->condition, ifEnd));
        addStatement(i->body);
        addInstruction(ifEnd);
      }
    }
    else if(Return* r = dynamic_cast<Return*>(s))
    {
      addInstruction(new ReturnIR(r->value));
    }
    else if(dynamic_cast<Break*>(s))
    {
      addInstruction(new Jump(breakLabel));
    }
    else if(dynamic_cast<Continue*>(s))
    {
      addInstruction(new Jump(continueLabel));
    }
    else if(Print* p = dynamic_cast<Print*>(s))
    {
      for(auto e : p->exprs)
      {
        addInstruction(new PrintIR(e));
      }
    }
    else if(Assertion* assertion = dynamic_cast<Assertion*>(s))
    {
      addInstruction(new AssertionIR(assertion));
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

  void SubroutineIR::addInstruction(StatementIR* s)
  {
    //here, expand all expressions into pseudo-SSA form
    if(auto a = dynamic_cast<AssignIR*>(s))
    {
      //evaluate RHS, then LHS
      a->src = expandExpression(a->src);
      a->dst = expandExpression(a->dst);
    }
    else if(auto cj = dynamic_cast<CondJump*>(s))
    {
      cj->cond = expandExpression(cj->cond);
    }
    else if(auto r = dynamic_cast<ReturnIR*>(s))
    {
      if(r->expr)
        r->expr = expandExpression(r->expr);
    }
    else if(auto p = dynamic_cast<PrintIR*>(s))
    {
      p->expr = expandExpression(p->expr);
    }
    else if(auto as = dynamic_cast<AssertionIR*>(s))
    {
      as->asserted = expandExpression(as->asserted);
    }
    stmts.push_back(s);
  }

  VarExpr* SubroutineIR::generateTemp(Type* t)
  {
    auto v = new Variable(getTempName(), t, currentBlock);
    v->resolve();
    vars.push_back(v);
    auto ve = new VarExpr(v);
    ve->resolve();
    return ve;
  }

  VarExpr* SubroutineIR::generateTemp(Expression* e)
  {
    VarExpr* ve = generateTemp(e->type);
    //assign the original expression to the variable
    stmts.push_back(new AssignIR(ve, e));
    return ve;
  }

  Expression* SubroutineIR::expandExpression(Expression* e)
  {
    //Expression expansion creates temporary variables (in currentBlock)
    //to hold results of intermediate expressions.
    //
    //Then those intermediates are replaced by VarExprs in the IR
    //
    //This provides straightforward common subexpression elimination
    //and simplifies code generation by having storage for every expr
    if(auto ua = dynamic_cast<UnaryArith*>(e))
    {
      ua->expr = expandExpression(ua->expr);
      e = generateTemp(ua);
    }
    else if(auto ba = dynamic_cast<BinaryArith*>(e))
    {
      //short-circuit evaluation is implemented here
      //LHS is always evaluated first
      ba->lhs = expandExpression(ba->lhs);
      if(ba->op == LOR)
      {
        //make the temp now, with value lhs
        VarExpr* tmp = generateTemp(ba->lhs);
        Label* skip = new Label;
        Expression* skipCond = new UnaryArith(LNOT, tmp);
        skipCond->resolve();
        stmts.push_back(new CondJump(skipCond, skip));
        //if tmp is true, jump to skip (never evaluating rhs)
        //lhs false, so (lhs || rhs) == rhs
        Expression* rhs = expandExpression(ba->rhs);
        stmts.push_back(new AssignIR(tmp, rhs));
        stmts.push_back(skip);
        e = tmp;
      }
      else if(ba->op == LAND)
      {
        VarExpr* tmp = generateTemp(ba->lhs);
        Label* skip = new Label;
        stmts.push_back(new CondJump(tmp, skip));
        //lhs true, so (lhs && rhs) == rhs
        Expression* rhs = expandExpression(ba->rhs);
        stmts.push_back(new AssignIR(tmp, rhs));
        stmts.push_back(skip);
        e = tmp;
      }
      else
      {
        ba->rhs = expandExpression(ba->rhs);
        e = generateTemp(ba);
      }
    }
    else if(auto ind = dynamic_cast<Indexed*>(e))
    {
      ind->group = expandExpression(ind->group);
      ind->index = expandExpression(ind->index);
      e = generateTemp(ind);
    }
    else if(auto al = dynamic_cast<ArrayLength*>(e))
    {
      al->array = expandExpression(al->array);
      e = generateTemp(al);
    }
    else if(auto ae = dynamic_cast<AsExpr*>(e))
    {
      ae->base = expandExpression(ae->base);
      e = generateTemp(ae);
    }
    else if(auto ie = dynamic_cast<IsExpr*>(e))
    {
      ie->base = expandExpression(ie->base);
      e = generateTemp(ie);
    }
    else if(auto ce = dynamic_cast<CallExpr*>(e))
    {
      //CallIR's constructor handles all necessary expansion,
      //and creates the temporary for return value.
      CallIR* ci = new CallIR(ce, this);
      e = ci->output;
    }
    else if(auto conv = dynamic_cast<Converted*>(e))
    {
      //Many primitive constants need to be converted to another
      //type - save a temporary by attempting this now
      foldExpression(e);
      //If e is still a Converted, must create a temporary
      conv = dynamic_cast<Converted*>(e);
      if(conv)
      {
        conv->value = expandExpression(conv->value);
        e = generateTemp(conv);
      }
    }
    else if(auto uc = dynamic_cast<UnionConstant*>(e))
    {
      //expand uc's value (it may or may not be a constant)
      uc->value = expandExpression(uc->value);
      e = generateTemp(uc);
    }
    else if(auto na = dynamic_cast<NewArray*>(e))
    {
      //expand the new array dimensions, but don't store the
      //NewArray itself in a temporary
      for(size_t i = 0; i < na->dims.size(); i++)
      {
        na->dims[i] = expandExpression(na->dims[i]);
      }
    }
    else if(auto sm = dynamic_cast<StructMem*>(e))
    {
      sm->base = expandExpression(sm->base);
    }
    //All other Expression types don't need to be decomposed
    //e.g. variables, constants.
    return e;
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
    addInstruction(top);
    //here the condition is evaluated
    //false: jump to bottom, otherwise continue
    addInstruction(new CondJump(fc->condition, bottom));
    addStatement(fc->inner);
    addInstruction(mid);
    addStatement(fc->increment);
    addInstruction(new Jump(top));
    addInstruction(bottom);
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
    addInstruction(new AssignIR(counterExpr, fr->begin));
    auto savedBreak = breakLabel;
    auto savedCont = continueLabel;
    Label* top = new Label;     //just before condition eval
    Label* mid = new Label;     //before increment (continue)
    Label* bottom = new Label;  //just after loop (break)
    addInstruction(top);
    //condition eval and possible break
    addInstruction(new CondJump(cond, bottom));
    addStatement(fr->inner);
    addInstruction(mid);
    addInstruction(new AssignIR(counterExpr, counterP1));
    addInstruction(new Jump(top));
    addInstruction(bottom);
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
      addInstruction(new AssignIR(new VarExpr(fa->counters[i]), zeroLong));
      addInstruction(topLabels[i]);
      //compare against array dim: if false, break from loop i
      Expression* cond = new BinaryArith(new VarExpr(fa->counters[i]), CMPL, dims[i]);
      cond->resolve();
      addInstruction(new CondJump(cond, bottomLabels[i]));
    }
    //update iteration variable before executing inner body
    VarExpr* iterVar = new VarExpr(fa->iter);
    iterVar->resolve();
    Indexed* iterValue = new Indexed(subArrays.back(), new VarExpr(fa->counters.back()));
    iterValue->resolve();
    addInstruction(new AssignIR(iterVar, iterValue));
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
      addInstruction(midLabels[i]);
      Expression* counterIncremented = new BinaryArith(iterVar, PLUS, oneLong);
      counterIncremented->resolve();
      addInstruction(new AssignIR(new VarExpr(fa->counters[i]), counterIncremented));
      addInstruction(new Jump(topLabels[i]));
      addInstruction(bottomLabels[i]);
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
    for(size_t i = 0; i < numStmts; i++)
    {
      while(caseIndex < sw->caseLabels.size() &&
          sw->caseLabels[caseIndex] == i)
      {
        //add the label
        addInstruction(caseLabels[caseIndex]);
        Expression* compareExpr = new BinaryArith(
            sw->switched, CMPEQ, sw->caseValues[caseIndex]);
        compareExpr->resolve();
        //this case branches to the next case, or the default
        Label* branch = nullptr;
        if(caseIndex == sw->caseLabels.size() - 1)
          branch = defaultLabel;
        else
          branch = caseLabels[caseIndex + 1];
        addInstruction(new CondJump(compareExpr, branch));
        caseIndex++;
      }
      //default is just a label (needs no comparison)
      if(sw->defaultPosition == i)
        addInstruction(defaultLabel);
      addStatement(sw->block->stmts[i]);
    }
    addInstruction(breakLabel);
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
      addInstruction(caseLabels[i]);
      Expression* compareExpr = new IsExpr(ma->matched, ma->types[i]);
      compareExpr->resolve();
      Label* branch = nullptr;
      if(i == numCases - 1)
        branch = endLabel;
      else
        branch = caseLabels[i + 1];
      addInstruction(new CondJump(compareExpr, branch));
      //assign the "as" expr to the case variable
      VarExpr* caseVar = new VarExpr(ma->caseVars[i]);
      caseVar->resolve();
      AsExpr* caseVal = new AsExpr(ma->matched, ma->types[i]);
      caseVal->resolve();
      addInstruction(new AssignIR(caseVar, caseVal));
      addStatement(ma->cases[i]);
      //jump to the match end (if not the last case)
      if(i != numCases - 1)
        addInstruction(new Jump(endLabel));
    }
    addInstruction(endLabel);
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

  string SubroutineIR::getTempName()
  {
    //identifiers can't end with "__", so this can't conflict
    return "t" + to_string(tempCounter++) + "__";
  }
}

ostream& operator<<(ostream& os, const IR::StatementIR* stmt)
{
  return stmt->print(os);
}
