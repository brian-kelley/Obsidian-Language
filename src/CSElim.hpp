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
    void insert(Variable* v, AssignIR* a);
    //Remove definition for v if it differs from a->src
    void intersect(Variable* v, AssignIR* a);
    //Remove definition for v if it exists
    void invalidate(Variable* v);
    bool defined(Variable* v);
    //Clear all definitions
    void clear();
    //Returns null if no def for v
    Expression* getDef(Variable* v);
    //Returns null if no variable has this value
    Variable* varForExpr(Expression* e);
    //set of known definitions
    unordered_map<Variable*, AssignIR*> d;
    //set of expressions that are the definition of some variable
    unordered_map<Expression*, Variable*, ExprHash, ExprEqual> avail;
  };
  static bool operator==(const DefSet& d1, const DefSet& d2);
  //All definitions (one per basic block)
  vector<DefSet> definitions;
  //Attempt to replace e (or a subexpression) with a
  //VarExpr whose current definition is the same.
  //Return true if any changes are made.
  bool replaceExpr(Expression*& e, DefSet& defs);
  //Transfer function: process effects of the assignment
  bool transfer(AssignIR* a, DefSet& defs);
  //Kill all definitions that read a global 
  bool transferSideEffects(DefSet& defs);
  //Update incoming def set for b (uses "definitions")
  void meet(SubroutineIR* subr, BasicBlock* b);
};

