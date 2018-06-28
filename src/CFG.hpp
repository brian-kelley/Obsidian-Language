#ifndef CFG_H
#define CFG_H

struct Subroutine;
struct Statement;

struct Label {};
struct Jump {};
struct CondJump {};

//CFG needs to operate on a 
typedef variant<Statement*, Label, Jump, CondJump> PseudoStatement;

//Per-subroutine control flow graph
//Edges are statements, 
struct CFG
{
  CFG(Subroutine* s);
  //Basic blocks start at subr entry or branch targets,
  //and end just after jumps or returns
  struct BasicBlock
  {
    void addStatement(Statement* s);
    void link(BasicBlock* next);
    vector<Statement*> stmts;
    vector<BasicBlock*> in;
    vector<BasicBlock*> out;
  };
  vector<BasicBlock> verts;
  //create an edge
  BasicBlock entry;
  Subroutine* subr;
  void buildBasicBlocks(Statement* s);
  void linkBasicBlocks(Statement* s);
};

#endif

