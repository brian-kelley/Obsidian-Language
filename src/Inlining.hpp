#ifndef INLINING_H
#define INLINING_H

namespace IR
{
  struct SubroutineIR;
  struct AssignIR;
  struct EvalIR;
}

//Inline the call that is the RHS of an assignment
void inlineCall(IR::SubroutineIR* subr, IR::AssignIR* assign);
//Inline the call that 
void inlineCall(IR::SubroutineIR* subr, IR::EvalIR* eval);

#endif
