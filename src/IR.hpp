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
  struct CallIR;
  struct Jump;
  struct CondJump;
  struct Label;
  struct ReturnIR;
  struct PrintIR;
  struct AssertionIR;
  struct BasicBlock;
  struct SubroutineIR;

  extern SubroutineIR* mainIR;
  extern vector<SubroutineIR*> ir;
  extern vector<ExternalSubroutine*> externIR;
  extern vector<Variable*> allGlobals;
  //construct all (un-optimized) IR and CFGs from AST
  void buildIR();
  void optimizeIR();

  struct StatementIR
  {
    //get the variables used (read) by the statement (add to set)
    virtual void getReads(set<Variable*>& vars) {}
    virtual Variable* getWrite() {return nullptr;}
    virtual ostream& print(ostream& os) const = 0;
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
    //Does this assignment completely overwrite the existing value?
    bool isTotal()
    {
      return dynamic_cast<VarExpr*>(dst);
    }
    ostream& print(ostream& os) const
    {
      os << dst << " = " << src;
      return os;
    }
  };

  struct CallIR : public StatementIR
  {
    CallIR(CallExpr* c, SubroutineIR* subr);
    Expression* origCallable;
    CallableType* callableType;
    //Optional: "this" expression
    Expression* thisObject;
    variant<SubroutineIR*, ExternalSubroutine*, Expression*> callable;
    bool isDirect()
    {
      return callable.is<Expression*>();
    }
    bool isMethod()
    {
      return thisObject;
    }
    vector<Expression*> args;
    bool argBorrowed(int index);
    //If call returns a value, ctor creates a temp.
    //Otherwise, this is left null.
    //If the return value is assigned to any lvalue,
    //the next instruction is just an assign from this temp
    //to the lvalue.
    VarExpr* output;
    void getReads(set<Variable*>& vars)
    {
      if(callable.is<Expression*>())
      {
        callable.get<Expression*>()->getReads(vars, false);
      }
      for(auto arg : args)
      {
        arg->getReads(vars, false);
      }
      //output is always a VarExpr so it has no reads
    }
    Variable* getWrite()
    {
      //Output is the only direct write.
      //It may be null (void), but that's fine
      return output->var;
    }
    ostream& print(ostream& os) const;
  };

  struct Label : public StatementIR
  {
    ostream& print(ostream& os) const
    {
      os << intLabel << ":";
      return os;
    }
  };

  struct Jump : public StatementIR
  {
    Jump(Label* l) : dst(l) {}
    Label* dst;
    ostream& print(ostream& os) const
    {
      os << "Goto " << dst->intLabel;
      return os;
    }
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
    ostream& print(ostream& os) const
    {
      os << "If " << cond << " goto ";
      os << (intLabel + 1) << " else " << taken->intLabel;
      return os;
    }
  };

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
    ostream& print(ostream& os) const
    {
      os << "Return";
      if(expr)
        os << ' ' << expr;
      return os;
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
    ostream& print(ostream& os) const
    {
      os << "Print " << expr;
      return os;
    }
  };

  struct AssertionIR : public StatementIR
  {
    AssertionIR(Assertion* a) : asserted(a->asserted) {}
    AssertionIR(Expression* a) : asserted(a) {}
    Expression* asserted;
    void getReads(set<Variable*>& vars)
    {
      asserted->getReads(vars, false);
    }
    ostream& print(ostream& os) const
    {
      os << "Assert " << asserted;
      return os;
    }
  };

  struct Nop : public StatementIR
  {
    ostream& print(ostream& os) const
    {
      os << "<no-op>";
      return os;
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
    //Dominator basic blocks
    vector<bool> dom;
    //statements where BB starts and ends
    int start;
    int end;
    //which BB is this in linear code sequence?
    int index;
  };

  struct SubroutineIR : public Node
  {
    //Set subr, but don't add instructions yet
    SubroutineIR(Subroutine* s);
    //Actually add the instructions and build CFG
    void build();
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
    //All local variables (including parameters).
    //Dead store elimination can delete unused variables from this,
    //but parameters are never eliminated (even if unused).
    //
    vector<Variable*> vars;
    //params is a subset of vars - but order of params matters
    vector<Variable*> params;
    map<int, BasicBlock*> blockStarts;
    //is one BB reachable from another?
    bool reachable(BasicBlock* root, BasicBlock* target);
    string getTempName();
    //This version just creates a temporary variable.
    //Name is auto generated, but otherwise just a regular var.
    VarExpr* generateTemp(Type* t);
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

ostream& operator<<(ostream& os, const IR::StatementIR* stmt);

#endif

