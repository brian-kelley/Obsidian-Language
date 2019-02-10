#ifndef IR_H
#define IR_H

#include "Common.hpp"
#include "Subroutine.hpp"

/* Lower-level IR
 *
 * Is much higher level than 3-address code
 * (operands are Onyx expressions, not primitive values)
 *
 * Should be easy to do basic optimizations and natural to emit as C, LLVM or x86
 */

struct Subroutine;
struct Expression;


namespace IR
{
  struct StatementIR;
  struct AssignIR;
  struct EvalIR;
  struct Jump;
  struct CondJump;
  struct Label;
  struct ReturnIR;
  struct PrintIR;
  struct AssertionIR;
  struct BasicBlock;
  struct SubroutineIR;

  //IR value types (expressions)
  //
  //Associative arithmetic operators on > 2 operands
  //are combined into one "operation"
  //
  //All values are given unique IDs
  //
  //Expressions can quicky be tested for symbolic equivalence for CSE
  //
  //Need a multi-pass approach to create minimal set of values in subroutine
  //
  //  1. Create an initial set of values, using them as arguments to IR instructions
  //    1a. Variables before and after any assignment to them are different Values
  //  2. For each BB with multiple incoming CFG edges, add the necessary phi nodes
  //    2a. phi nodes don't need corresponding Assign instructions, just keep a separate table of them
  //    2b. this creates valid (but not optimal, yet) SSA
  //  3. Until no more updates can be done:
  //    3a. Test if all joined values in a phi node are actually equivalent
  //        (can safely delete those nodes, and replace their usage by any of the incoming versions)
  //    3b. Find constant subexpressions (and evaluate them, except when it would
  //        create a constant bigger than an arbitrary cutoff, like 256 bytes)

  extern map<Subroutine*, SubroutineIR*> ir;
  //construct all (un-optimized) IR and CFGs from AST
  void buildIR();
  void optimizeIR();

  struct StatementIR
  {
    //get the variables used (read) by the statement (add to set)
    virtual void getReads(set<Variable*>& vars) {}
    virtual Variable* getWrite() {return nullptr;}
    //integer position in subroutine
    int intLabel;
  };

  struct AssignIR : public StatementIR
  {
    AssignIR(Expression* d, Expression* s) : dst(d), src(s) {}
    Expression* dst;
    Expression* src;
    void getReads(set<Variable*>& vars)
    {
      dst->getReads(vars, true);
      src->getReads(vars, false);
    }
    Variable* getWrite()
    {
      return dst->getWrite();
    }
  };

  struct EvalIR : public StatementIR
  {
    EvalIR(Expression* e) : eval(e) {}
    //currently this is only used by CallStmts
    Expression* eval;
    void getReads(set<Variable*>& vars)
    {
      eval->getReads(vars, false);
    }
  };

  struct Jump : public StatementIR
  {
    Jump(Label* l) : dst(l) {}
    Label* dst;
  };

  struct CondJump : public StatementIR
  {
    CondJump(Expression* c, Label* dst) : cond(c), taken(dst) {}
    //the boolean condition
    //false = jump taken, true = not taken (fall through)
    Expression* cond;
    Label* taken;
    void getReads(set<Variable*>& vars)
    {
      cond->getReads(vars, false);
    }
  };

  struct Label : public StatementIR {};

  struct ReturnIR : public StatementIR
  {
    ReturnIR() : expr(nullptr) {}
    ReturnIR(Expression* val) : expr(val) {}
    Expression* expr;
    void getReads(set<Variable*>& vars)
    {
      if(expr)
        expr->getReads(vars, false);
    }
  };

  struct PrintIR : public StatementIR
  {
    PrintIR(Expression* e) : expr(e) {}
    Expression* expr;
    void getReads(set<Variable*>& vars)
    {
      expr->getReads(vars, false);
    }
  };

  struct AssertionIR : public StatementIR
  {
    AssertionIR(Assertion* a) : asserted(a->asserted) {}
    Expression* asserted;
    void getReads(set<Variable*>& vars)
    {
      asserted->getReads(vars, false);
    }
  };

  struct Nop : public StatementIR {};

  struct BasicBlock
  {
    BasicBlock(int s, int e) : start(s), end(e) {}
    //add an outgoing edge, and add this as an incoming
    void link(BasicBlock* other)
    {
      out.push_back(other);
      other->in.push_back(this);
    }
    vector<BasicBlock*> in;
    vector<BasicBlock*> out;
    //Dominator basic blocks
    vector<bool> dom;
    //statements where BB starts and ends
    int start;
    int end;
    //which BB is this in linear code sequence?
    int index;
  };

  struct SubroutineIR
  {
    SubroutineIR(Subroutine* s);
    void addStatement(Statement* s);
    void addInstruction(StatementIR* s);
    //Create a new variable that is tied to non-constant expression
    //Generate IR instructions to compute it (in proper evaluation order)
    Expression* expandExpression(Expression* e);
    //Remove no-ops and labels, and rebuild the control flow graph
    //(labels are just a convenience for translating AST to IR)
    void buildCFG();
    Subroutine* subr;
    vector<StatementIR*> stmts;
    vector<BasicBlock*> blocks;
    map<int, BasicBlock*> blockStarts;
    //is one BB reachable from another?
    bool reachable(BasicBlock* root, BasicBlock* target);
    string getTempName();
    VarExpr* generateTemp(Expression* e);
    private:
    void addForC(ForC* fc);
    void addForRange(ForRange* fr);
    void addForArray(ForArray* fa);
    void addSwitch(Switch* sw);
    void addMatch(Match* ma);
    void solveDominators();
    int tempCounter;
  };
}

extern IR::Nop* nop;

ostream& operator<<(ostream& os, IR::StatementIR* stmt);

#endif

