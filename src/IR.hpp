#ifndef IR_H
#define IR_H

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
    virtual string print() = 0;
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
    string print()
    {
      Oss oss;
      oss << dst << " <= " << src;
      return oss.str();
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
    string print()
    {
      Oss oss;
      oss << "call " << eval->callable << " (";
      for(size_t i = 0; i < eval->args.size(); i++)
      {
        if(i > 0)
          oss << ", ";
        oss << eval->args[i];
      }
      oss << ')';
      return oss.str();
    }
  };

  struct Jump
  {
    Jump(Label* l) : dst(l) {}
    Label* dst;
    int intDst;
    string print()
    {
      Oss oss;
      oss << "jump " << intDst;
      return oss.str();
    }
  };

  struct CondJump
  {
    CondJump(Expression* c, Label* dst) : cond(c), taken(dst) {}
    //the boolean condition
    //false = jump taken, true = not taken (fall through)
    Expression* cond;
    Label* taken;
    int intTaken;
    vector<Expression*> getInput()
    {
      return vector<Expression*>(1, cond);
    }
    string print()
    {
      Oss oss;
      oss << "jump " << intTaken << " if not " << cond;
      return oss.str();
    }
  };

  struct Label : public StatementIR
  {
    string print()
    {
      Oss oss;
      oss << intLabel << ':';
      return oss.str();
    }
  };

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
    string print()
    {
      Oss oss;
      oss << "return " << expr;
      return oss.str();
    }
  };

  struct PrintIR : public StatementIR
  {
    vector<Expression*> exprs;
    vector<Expression*> getInput()
    {
      return exprs;
    }
    string print()
    {
      Oss oss;
      oss << "print(";
      for(size_t i = 0; i < exprs.size(); i++)
      {
        if(i > 0)
          oss << ", ";
        oss << exprs[i];
      }
      oss << ')';
      return oss.str();
    }
  };

  struct AssertionIR : public StatementIR
  {
    Expression* asserted;
    vector<Expression*> getInput()
    {
      return vector<Expression*>(1, asserted);
    }
    string print()
    {
      Oss oss;
      oss << "assert(" << asserted << ')';
      return oss.str();
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
    //print out the name and IR of this subroutine
    //TODO: print expressions in a human-readable way
    void print();
    Subroutine* subr;
    vector<StatementIR*> stmts;
    vector<BasicBlock*> blocks;
    private:
    void addForC(ForC* fc);
    void addForRange(ForRange* fr);
    void addForArray(ForArray* fa);
  };
}

#endif

