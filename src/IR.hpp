#ifndef IR_H
#define IR_H

/* Lower-level IR
 *
 * Use for building control-flow graphs,
 * doing constant propagation and dead
 * code elimination
 *
 * Is much higher level than 3-address code
 * (works with high-level Onyx expressions, not just primitives)
 *
 * Should be fairly natural to emit as C, LLVM or x86
 */

struct Subroutine;
struct Expression;

namespace IR
{
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
  struct Jump
  {
    Jump(Label* l) : label(l) {}
    Label* label;
  };
  struct CondJump
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
    Return() : expr(nullptr) {}
    Return(Expression* val) : expr(val) {}
    Expression* expr;
    vector<Expression*> getInput()
    {
      if(expr)
        return vector<Expression*>(1, cond);
      else
        return vector<Expression*>();
    }
  };
  struct PrintIR : public StatementIR
  {
    vector<Expression*> exprs;
    vector<Expression*> getInput()
    {
      return exprs;
    }
  };
  struct AssertionIR : public StatementIR
  {
    Expression* asserted;
    vector<Expression*> getInput()
    {
      return vector<Expression*>(1, asserted);
    }
  };
  struct SubroutineIR
  {
    SubroutineIR(Subroutine* subr);
    vector<StatementIR*> stmts;
    void addStatement(Statement* s);
    void addForC(ForC* fc);
    void addForRange(ForRange* fr);
    void addForArray(ForArray* fa);
  };
}

#endif

