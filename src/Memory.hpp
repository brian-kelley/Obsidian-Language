#ifndef MEMORY_H
#define MEMORY_H

#include "IR.hpp"

//Onyx automatic memory management rules:
//  -For each subr, must determine set of directly modified parameters (easy)
//
//  -Never ever leak anything
//    (although all globals have indefinite lifetime so are never freed)
//  -Variables which are recursive unions, arrays or maps own heap memory.
//    -Memory resources can be nested (e.g. multidimensional array)
//    -For now, don't worry about eliding inner-dimensional deep copies.
//  -Memory ownership can be transferred to another variable,
//    as long as the original owner's current def is dead from then on
//    -Common pattern: a = f(a);
//      -f borrows a at first
//      -f computes some new expr and returns it (transferring to caller)
//      -caller transfers return value to a (shallow)
//      -while (shallow) copying to a, caller frees original a
//      -Note: in whole process, there is never a deep copy!
//        (f could modify a in place or even resize it)
//  -Memory can be borrowed, but only during a duration
//    where neither owner nor borrower are modified
//    -Very important for call performance
//     (callee borrows but doesn't modify, and caller continues owning after)
//
//  -Optimization strategy:
//    -Initially, replace every copy (not access!) of a complex data
//     type with a DeepCopy wrapper expr
//    -Temporaries are never deep copied, since their lifetime ends immediately
//      -Examples: array+ arithmetic, function return values
//    -If complex expr is assigned to index of higher-dim object
//     (like int[] into int[][]) then it's still copy in the codegen, but
//     the outermost array still owns the copy, so unwrap the DeepCopy
//    -Until stabilization:
//      -Replace a copy "b = DeepCopy(a);" with just a,
//        if over this definition's reach a is not modified again (a, b share the resource there)

using IR::SubroutineIR;
using IR::StatementIR;

namespace Memory
{
  //Is the parameter modified inside its subroutine body?
  bool parameterModified(Variable* p);

  /*
struct Management
{

  //"Optimal" memory sharing is computed using forward dataflow.
  //Store the memory resource ID for each var.
  //Memory resources here are just identified by an integer, since
  //the details of managing them will not be done until codegen.
  //
  //Initialize by giving each variable a unique ID.
  //Go through 

  struct ReachingDefs
  {
    ReachingDefs(SubroutineIR* subr);
    void transfer(ReachingSet& r, StatementIR* stmt);
    //Table of all assignments and corresponding indices in reaching[k]
    unordered_map<AssignIR*, int> assignTable;
    //Reaching-def set for each block
    vector<AssignIR*> allAssigns;
    vector<ReachingSet> reaching;
  };
};
*/
}

#endif

