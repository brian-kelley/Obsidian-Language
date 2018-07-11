#ifndef IR_H
#define IR_H

#include "Common.hpp"
#include "Subroutine.hpp"

/* Lower-level IR
 *
 * Use for building control-flow graphs,
 * doing dead code elimination, and constructing SSA
 *
 * Is much higher level than 3-address code
 * (operands are high-level Onyx expressions, not just primitives)
 *
 * Should be fairly natural to emit as C, LLVM or x86
 */

struct Subroutine;
struct Expression;

namespace IR
{
  struct StatementIR;
  struct AssignIR;
  struct CallIR;
  struct Jump;
  struct CondJump;
  struct Label;
  struct ReturnIR;
  struct PrintIR;
  struct AssertionIR;
  struct BasicBlock;
  struct SubroutineIR;

  extern map<Subroutine*, SubroutineIR*> ir;
  //construct all (un-optimized) IR and CFGs from AST
  void buildIR();

  struct StatementIR
  {
    //data input/output (for data dependency analysis)
    virtual vector<Expression*> getInput()
    {
      return vector<Expression*>();
    }
    virtual vector<Expression*> getOutput()
    {
      return vector<Expression*>();
    }
    virtual ~StatementIR(){}
    //integer position in subroutine
    int intLabel;
  };

  struct AssignIR : public StatementIR
  {
    AssignIR(Expression* d, Expression* s) : dst(d), src(s) {}
    Expression* dst;
    Expression* src;
    vector<Expression*> getInput()
    {
      return vector<Expression*>(1, src);
    }
    vector<Expression*> getOutput()
    {
      return vector<Expression*>(1, dst);
    }
  };

  struct CallIR : public StatementIR
  {
    CallIR(CallStmt* cs) : eval(cs->eval) {}
    //currently this is only used by CallStmts
    CallExpr* eval;
    vector<Expression*> getInput()
    {
      return vector<Expression*>(1, eval);
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
    vector<Expression*> getInput()
    {
      return vector<Expression*>(1, cond);
    }
  };

  struct Label : public StatementIR {};

  struct ReturnIR : public StatementIR
  {
    ReturnIR() : expr(nullptr) {}
    ReturnIR(Expression* val) : expr(val) {}
    Expression* expr;
    vector<Expression*> getInput()
    {
      if(expr)
        return vector<Expression*>(1, expr);
      else
        return vector<Expression*>();
    }
  };

  struct PrintIR : public StatementIR
  {
    PrintIR(Print* p) : exprs(p->exprs) {}
    vector<Expression*> exprs;
    vector<Expression*> getInput()
    {
      return exprs;
    }
  };

  struct AssertionIR : public StatementIR
  {
    AssertionIR(Assertion* a) : asserted(a->asserted) {}
    Expression* asserted;
    vector<Expression*> getInput()
    {
      return vector<Expression*>(1, asserted);
    }
  };

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
    //statements where BB starts and ends
    int start;
    int end;
  };

  struct SubroutineIR
  {
    SubroutineIR(Subroutine* s);
    void addStatement(Statement* s);
    set<Variable*> getReads(BasicBlock* bb);
    set<Variable*> getWrites(BasicBlock* bb);
    Subroutine* subr;
    vector<StatementIR*> stmts;
    vector<BasicBlock*> blocks;
    private:
    void addForC(ForC* fc);
    void addForRange(ForRange* fr);
    void addForArray(ForArray* fa);
  };
}

ostream& operator<<(ostream& os, IR::StatementIR* stmt);

#endif

