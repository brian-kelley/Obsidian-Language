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
    map<Variable*, AssignIR*> defs;
    //The blacklist is for variables whose definition
    //can't be summed up by a single AssignIR (like an array
    //whose elements are assigned individually)
    //Variables can always be removed from the blacklist
    //when their values are completely overwritten in an Assign.
    set<Variable*> blacklist;
  };
  //Check if two AssignIRs define the same value
  bool definitionsMatch(AssignIR* d1, AssignIR* d2);
  //Next 2 functions return true if any updates are made
  //Attempt copy propagation (by replacing VarExprs inside e)
  bool copyProp(Expression*& e, DefSet& defs);
  //Attempt CSE: replace computation with a VarExpr
  bool elimComputation(AssignIR* a, DefSet& defs);
  //Transfer function: process effects of the assignment
  void transfer(AssignIR* a, DefSet& defs);
  //Meet function: intersect all incoming definitions
  DefSet meet(SubroutineIR* subr, int bb);
};

