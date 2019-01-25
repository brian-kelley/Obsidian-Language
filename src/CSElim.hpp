#include "IR.hpp"

/* Common Subexpression Elimination (CSE)
 * and Copy Propagation (done at the same time)
 *
 * Uses reaching definitions */

using IR::SubroutineIR;
using IR::StatementIR;
using IR::AssignIR;

void cse(SubroutineIR* subr);

struct CSElim
{
  CSElim(SubroutineIR* subr);
  struct DefSet
  {
    //Update functions return true if changes are made.
    //Add or overwrite a definition
    bool insert(Variable* v, AssignIR* a);
    bool intersect(Variable* v, AssignIR* a);
    bool invalidate(Variable* v);
    bool defined(Variable* v);
    void clear();
    //precondition: defined(v)
    Expression* getDef(Variable* v);
    //No precondition - returns NULL if no variable has this value
    Variable* varForExpr(Expression* e);
    //set of known definitions
    map<Variable*, AssignIR*> d;
    //set of expressions that are the definition of some variable
    unordered_map<Expression*, Variable*, ExprHash, ExprEqual> avail;
  };
  static bool operator==(const DefSet& d1, const DefSet& d2);
  //All definitions (one per basic block)
  vector<DefSet> definitions;
  //Check if two AssignIRs define the same value
  bool definitionsMatch(AssignIR* d1, AssignIR* d2);
  //Attempt to replace e (or a subexpression) with a
  //VarExpr whose current definition is the same.
  //Return true if any changes are made.
  bool replaceExpr(Expression*& e, DefSet& defs);
  //Transfer function: process effects of the assignment
  void transfer(AssignIR* a, DefSet& defs);
  //Kill all definitions that read a global 
  void transferSideEffects(DefSet& defs);
  //Update incoming def set for b
  void meet(SubroutineIR* subr, BasicBlock* b);
};

