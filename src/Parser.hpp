#ifndef PARSER_H
#define PARSER_H

#include "Common.hpp"
#include "PoolAlloc.hpp"
#include "Token.hpp"

#include "variadic-variant/variant.h"

//Use empty struct as default (first) value for variants (always trivially constructible)
struct None{};
struct BlockScope;
typedef runtime_error ParseErr;

namespace Parser
{
  struct Module;
  //Parse a program from token string (only function needed outside namespace)
  Module* parseProgram(vector<Token*>& toks);

  //Token stream & utilities
  extern size_t pos;                //token iterator
  extern vector<Token*>* tokens;    //all tokens from lexer

  void unget();                 //back up one token (no-op if at start of token string)
  void accept();                //accept any token
  bool accept(Token& t);
  Token* accept(int tokType);   //return NULL if tokType doesn't match next
  bool acceptKeyword(int type);
  bool acceptOper(int type);
  bool acceptPunct(int type);
  void expect(Token& t);
  Token* expect(int tokType);
  void expectKeyword(int type);
  void expectOper(int type);
  void expectPunct(int type);
  Token* lookAhead();  //get the next token without advancing pos
  void err(string msg = "");

  //lots of mutual recursion in nonterminal structs so just forward-declare all of them
  struct ScopedDecl;
  struct TypeNT;
  struct StatementNT;
  struct Typedef;
  struct Return;
  struct Switch;
  struct Continue;
  struct Break;
  struct EmptyStatement;
  struct For;
  struct ForC;
  struct ForRange1;
  struct ForRange2;
  struct While;
  struct If;
  struct Using;
  struct Assertion;
  struct TestDecl;
  struct Enum;
  struct Block;
  struct VarDecl;
  struct VarAssign;
  struct PrintNT;
  struct CallOp;
  struct Arg;
  struct SubroutineTypeNT;
  struct FuncDecl;
  struct FuncDef;
  struct FuncTypeNT;
  struct ProcDecl;
  struct ProcDef;
  struct ProcTypeNT;
  struct StructDecl;
  struct UnionDecl;
  struct TraitDecl;
  struct StructLit;
  struct Member;
  struct TraitType;
  struct TupleTypeNT;
  struct BoolLit;
  struct Expr1;
  typedef Expr1 ExpressionNT;
  struct Expr1RHS;
  struct Expr2;
  struct Expr2RHS;
  struct Expr3;
  struct Expr3RHS;
  struct Expr4;
  struct Expr4RHS;
  struct Expr5;
  struct Expr5RHS;
  struct Expr6;
  struct Expr6RHS;
  struct Expr7;
  struct Expr7RHS;
  struct Expr8;
  struct Expr8RHS;
  struct Expr9;
  struct Expr9RHS;
  struct Expr10;
  struct Expr10RHS;
  struct Expr11;
  struct Expr12;
  struct Expr12RHS;
  struct NewArrayNT;

  struct ParseNode : public PoolAllocated
  {
    ParseNode() : line(0), col(0) {}
    //set location, given the first token in the nonterminal
    void setLoc(Token* t)
    {
      line = t->line;
      col = t->col;
    }
    int line;
    int col;
  };

  struct Module : public PoolAllocated
  {
    string name;
    vector<ScopedDecl*> decls;
  };
  
  struct ScopedDecl : public PoolAllocated
  {
    ScopedDecl() : decl(None()) {}
    variant<
      None,
      Module*,
      VarDecl*,
      StructDecl*,
      UnionDecl*,
      TraitDecl*,
      Enum*,
      Typedef*,
      FuncDecl*,
      FuncDef*,
      ProcDecl*,
      ProcDef*,
      TestDecl*> decl;
  };

  ScopedDecl* parseScopedDeclGivenMember(Member* mem);

  struct TypeNT : public PoolAllocated
  {
    TypeNT() : t(None()), arrayDims(0) {}
    enum Prim
    {
      BOOL,
      CHAR,
      BYTE,
      UBYTE,
      SHORT,
      USHORT,
      INT,
      UINT,
      LONG,
      ULONG,
      FLOAT,
      DOUBLE,
      VOID
    };
    struct TTypeNT {};
    variant<
      None,
      Prim,
      Member*,
      TupleTypeNT*,
      SubroutineTypeNT*,
      TraitType*,
      TTypeNT> t;
    int arrayDims;
  };

  struct StatementNT : public PoolAllocated
  {
    StatementNT() : s(None()) {}
    variant<
      None,
      ScopedDecl*,
      VarAssign*,
      PrintNT*,
      Expr12*,
      Block*,
      Return*,
      Continue*,
      Break*,
      Switch*,
      For*,
      While*,
      If*,
      Assertion*,
      EmptyStatement*> s;
    StatementNT* stmt;
  };

  struct Typedef : public PoolAllocated
  {
    Typedef() : type(nullptr) {}
    TypeNT* type;
    string ident;
  };

  struct Return : public PoolAllocated
  {
    Return() : ex(nullptr) {}
    //optional returned expression (NULL if unused)
    ExpressionNT* ex;
  };

  struct SwitchCase : public PoolAllocated
  {
    SwitchCase() : matchVal(nullptr), s(nullptr) {}
    ExpressionNT* matchVal;
    StatementNT* s;
  };

  struct Switch : public PoolAllocated
  {
    Switch() : sw(nullptr), defaultStatement(nullptr) {}
    ExpressionNT* sw;
    vector<SwitchCase*> cases;
    //optional default: statement, NULL if unused
    StatementNT* defaultStatement;
  };

  struct Continue {};
  struct Break {};
  struct EmptyStatement {};

  struct For : public PoolAllocated
  {
    For() : f(None()), body(nullptr) {}
    variant<
      None,
      ForC*,
      ForRange1*,
      ForRange2*> f;
    Block* body;
  };

  struct ForC : public PoolAllocated
  {
    ForC() : decl(nullptr), condition(nullptr), incr(nullptr) {}
    //for(decl; condition; incr) <body>
    //allow arbitrary statements for dcel and incr, not just VarDecl and VarAssign
    StatementNT* decl;
    ExpressionNT* condition;
    StatementNT* incr;
  };

  struct ForRange1 : public PoolAllocated
  {
    ForRange1() : expr(nullptr) {}
    ExpressionNT* expr;
  };

  struct ForRange2 : public PoolAllocated
  {
    ForRange2() : start(nullptr), end(nullptr) {}
    ExpressionNT* start;
    ExpressionNT* end;
  };

  struct While : public PoolAllocated
  {
    While() : cond(nullptr), body(nullptr) {}
    ExpressionNT* cond;
    Block* body;
  };

  struct If : public PoolAllocated
  {
    If() : cond(nullptr), ifBody(nullptr), elseBody(nullptr) {}
    ExpressionNT* cond;
    StatementNT* ifBody;
    //elseBody NULL when there is no else clause
    StatementNT* elseBody;
    /* note: if(cond1) if(cond2) a else b else c will parse depth-first:
     * if(cond1)
     * {
     *   if(cond2)
     *     a
     *   else
     *     b
     * }
     * else
     *   c
     */
  };

  struct Using : public PoolAllocated
  {
    Using() : mem(nullptr) {}
    Member* mem;
  };

  struct Assertion : public PoolAllocated
  {
    Assertion() : expr(nullptr) {}
    ExpressionNT* expr;
  };

  struct TestDecl : public PoolAllocated
  {
    TestDecl() : stmt(nullptr) {}
    StatementNT* stmt;
  };

  struct EnumItem : public PoolAllocated
  {
    EnumItem() : value(nullptr) {}
    string name;
    //value is optional
    //(NULL if not explicit, then set automatically)
    IntLit* value;
  };

  struct Enum : public PoolAllocated
  {
    string name;
    vector<EnumItem*> items;
  };

  struct Block : public PoolAllocated
  {
    vector<StatementNT*> statements;
    BlockScope* bs;
  };

  struct VarDecl : public PoolAllocated
  {
    VarDecl() : type(nullptr), val(nullptr) {}
    //NULL if "auto"
    TypeNT* type;
    //var decl's name can only be a single identifier (but variables can be referenced via compound idents)
    string name;
    //initializing expression (optional)
    ExpressionNT* val;
    //true if static, false otherwise
    //if true, only has valid semantics if inside a struct decl
    bool isStatic;
  };

  struct VarAssign : public PoolAllocated
  {
    VarAssign() : target(nullptr), rhs(nullptr) {}
    //note: target must be an lvalue (checked in middle end)
    //but all lvalues are Expr12 so that is used here
    Expr12* target;
    ExpressionNT* rhs;
  };

  struct PrintNT : public PoolAllocated
  {
    vector<ExpressionNT*> exprs;
  };

  //Parameter - used by Func/Proc Def (not decl)
  struct Parameter : public PoolAllocated
  {
    Parameter() : type(nullptr), name(nullptr) {}
    TypeNT* type;
    Ident* name;
  };

  struct SubroutineTypeNT : public PoolAllocated
  {
    SubroutineTypeNT() : retType(nullptr) {}
    TypeNT* retType;
    vector<Parameter*> params;
    bool isStatic;
  };

  struct FuncTypeNT : public SubroutineTypeNT
  {};

  struct FuncDecl : public PoolAllocated
  {
    string name;
    FuncTypeNT type;
  };

  struct FuncDef : public PoolAllocated
  {
    FuncDef() : body(nullptr) {}
    string name;
    FuncTypeNT type;
    Block* body;
  };

  struct ProcTypeNT : public SubroutineTypeNT
  {
    bool nonterm;
  };

  struct ProcDecl : public PoolAllocated
  {
    string name;
    ProcTypeNT type;
  };

  struct ProcDef : public PoolAllocated
  {
    ProcDef() : body(nullptr) {}
    string name;
    ProcTypeNT type;
    Block* body;
  };

  struct StructMem : public PoolAllocated
  {
    StructMem() : sd(nullptr) {}
    ScopedDecl* sd;
    //composition can only apply to VarDecls
    bool compose;
  };

  struct StructDecl : public PoolAllocated
  {
    string name;
    vector<Member*> traits;
    vector<StructMem*> members;
  };

  struct UnionDecl : public PoolAllocated
  {
    string name;
    vector<TypeNT*> types;
  };

  struct TraitDecl : public PoolAllocated
  {
    string name;
    vector<variant<
      None,
      FuncDecl*,
      ProcDecl*>> members;
  };

  struct StructLit : public PoolAllocated
  {
    vector<ExpressionNT*> vals;
  };

  struct Member : public PoolAllocated
  {
    vector<Ident*> head;
    Ident* tail;
  };

  struct TraitType : public PoolAllocated
  {
    //trait types of the form "<localName>: <traitNames> <argName>"
    //i.e.: int f(T: Num, Drawable value)
    string localName;
    vector<Member*> traits;
  };

  struct TupleTypeNT : public PoolAllocated
  {
    //cannot be empty
    vector<TypeNT*> members;
  };

  struct BoolLit : public PoolAllocated
  {
    BoolLit() : val(false) {}
    BoolLit(bool v) : val(v) {}
    bool val;
  };

  struct Expr1 : public PoolAllocated
  {
    Expr1() : head(nullptr) {}
    Expr1(Expr2* e);
    Expr1(Expr3* e);
    Expr1(Expr4* e);
    Expr1(Expr5* e);
    Expr1(Expr6* e);
    Expr1(Expr7* e);
    Expr1(Expr8* e);
    Expr1(Expr9* e);
    Expr1(Expr10* e);
    Expr1(Expr11* e);
    Expr1(Expr12* e12);
    variant<Expr2*, NewArrayNT*> e;
    Expr2* head;
    //note: if e.is<NewArrayNT*>, tail is empty and unused
    vector<Expr1RHS*> tail;
  };

  struct Expr1RHS : public PoolAllocated
  {
    Expr1RHS() : rhs(nullptr) {}
    // || is only op
    Expr2* rhs;
  };

  struct Expr2 : public PoolAllocated
  {
    Expr2() : head(nullptr) {}
    Expr2(Expr3* e) : head(e) {}
    Expr2(Expr12* e12);
    Expr3* head;
    vector<Expr2RHS*> tail;
  };

  struct Expr2RHS : public PoolAllocated
  {
    Expr2RHS() : rhs(nullptr) {}
    // && is only op
    Expr3* rhs;
  };

  struct Expr3 : public PoolAllocated
  {
    Expr3() : head(nullptr) {}
    Expr3(Expr4* e) : head(e) {}
    Expr3(Expr12* e12);
    Expr4* head;
    vector<Expr3RHS*> tail;
  };

  struct Expr3RHS : public PoolAllocated
  {
    Expr3RHS() : rhs(nullptr) {}
    // | is only op
    Expr4* rhs;
  };

  struct Expr4 : public PoolAllocated
  {
    Expr4() : head(nullptr) {}
    Expr4(Expr5* e) : head(e) {}
    Expr4(Expr12* e12);
    Expr5* head;
    vector<Expr4RHS*> tail;
  };

  struct Expr4RHS : public PoolAllocated
  {
    Expr4RHS() : rhs(nullptr) {}
    // ^ is only op
    Expr5* rhs;
  };

  struct Expr5 : public PoolAllocated
  {
    Expr5() : head(nullptr) {}
    Expr5(Expr6* e) : head(e) {}
    Expr5(Expr12* e12);
    Expr6* head; 
    vector<Expr5RHS*> tail;
  };

  struct Expr5RHS : public PoolAllocated
  {
    Expr5RHS() : rhs(nullptr) {}
    // & is only op
    Expr6* rhs;
  };

  struct Expr6 : public PoolAllocated
  {
    Expr6() : head(nullptr) {}
    Expr6(Expr7* e) : head(e) {}
    Expr6(Expr12* e12);
    Expr7* head;
    vector<Expr6RHS*> tail;
  };

  struct Expr6RHS : public PoolAllocated
  {
    Expr6RHS() : rhs(nullptr) {}
    int op; //CMPEQ or CMPNEQ
    Expr7* rhs;
  };

  struct Expr7 : public PoolAllocated
  {
    Expr7() : head(nullptr) {}
    Expr7(Expr8* e) : head(e) {}
    Expr7(Expr12* e12);
    Expr8* head;
    vector<Expr7RHS*> tail;
  };

  struct Expr7RHS : public PoolAllocated
  {
    Expr7RHS() : rhs(nullptr) {}
    int op;  //CMPL, CMPLE, CMPG, CMPGE
    Expr8* rhs;
  };

  struct Expr8 : public PoolAllocated
  {
    Expr8() : head(nullptr) {}
    Expr8(Expr9* e) : head(e) {}
    Expr8(Expr12* e12);
    Expr9* head;
    vector<Expr8RHS*> tail;
  };

  struct Expr8RHS : public PoolAllocated
  {
    Expr8RHS() : rhs(nullptr) {}
    int op; //SHL, SHR
    Expr9* rhs;
  };

  struct Expr9 : public PoolAllocated
  {
    Expr9() : head(nullptr) {}
    Expr9(Expr10* e) : head(e) {}
    Expr9(Expr12* e12);
    Expr10* head;
    vector<Expr9RHS*> tail;
  };

  struct Expr9RHS : public PoolAllocated
  {
    Expr9RHS() : rhs(nullptr) {}
    int op; //PLUS, SUB
    Expr10* rhs;
  };

  struct Expr10 : public PoolAllocated
  {
    Expr10() : head(nullptr) {}
    Expr10(Expr11* e) : head(e) {}
    Expr10(Expr12* e12);
    Expr11* head;
    vector<Expr10RHS*> tail;
  };

  struct Expr10RHS : public PoolAllocated
  {
    Expr10RHS() : rhs(nullptr) {}
    int op; //MUL, DIV, MOD
    Expr11* rhs;
  };

  //Expr11 can be Expr12 or <op>Expr11
  struct Expr11 : public PoolAllocated
  {
    Expr11() : e(None()) {}
    Expr11(Expr12* e12);
    struct UnaryExpr
    {
      int op; //SUB, LNOT, BNOT
      Expr11* rhs;
    };
    variant<None,
      Expr12*,
      UnaryExpr> e;
  };

  struct NewArrayNT : public PoolAllocated
  {
    TypeNT* elemType;
    vector<ExpressionNT*> dimensions;
  };

  struct Expr12 : public PoolAllocated
  {
    Expr12() : e(None()) {}
    Expr12(ExpressionNT* expr) : e(expr) {}
    variant<
      None,
      IntLit*,
      CharLit*,
      StrLit*,
      FloatLit*,
      BoolLit*,
      ExpressionNT*,      //for expression inside parentheses
      StructLit*,
      Ident*> e;
    vector<Expr12RHS*> tail;  //tail is a chain of 0 or more member acesses, array indices and member calls
  };

  void parseExpr12Tail(Expr12* head);
  Expr12* parseExpr12GivenMember(Member* mem);

  struct CallOp
  {
    //the arguments inside parens (each may have the match operator)
    vector<ExpressionNT*> args;
  };

  struct Expr12RHS : public PoolAllocated
  {
    //to parse, get Member first, then if (...) seen is call, otherwise is member
    Expr12RHS() : e(None()) {}
    variant<
      None,
      Ident*,       //struct member: ". Identifier"
      CallOp*,      //call operator: "( Args )"
      ExpressionNT* //index operator: "[ Expr ]"
        > e;
  };

  //Parse a nonterminal of type NT
  template<typename NT>
  NT* parse();

  //Parse zero or more nonterminals (NT*)
  //Accept end token to stop
  //  So all calls to parse<NT> must succeed
  template<typename NT>
  vector<NT*> parseStar(Token& end)
  {
    vector<NT*> nts;
    while(!accept(end))
    {
      nts.push_back(parse<NT>());
    }
    return nts;
  }

  //Parse 0 or more comma-separated nonterminals: "(NT(,NT)*)?"
  //until end token is accepted
  template<typename NT>
  vector<NT*> parseStarComma(Token& end)
  {
    vector<NT*> nts;
    if(!accept(end))
    {
      NT* nt = parse<NT>();
      if(nt)
      {
        nts.push_back(nt);
        while(acceptPunct(COMMA))
        {
          nts.push_back(parse<NT>());
        }
      }
      expect(end);
    }
    return nts;
  }

  //Parse 1 or more comma-separated nonterminals "NT(,NT)*"
  //  First nonterminal must succeed
  //  As long as there is a comma, all subsequent nonterminals must also succeed
  //    So no end token is necessary
  template<typename NT>
  vector<NT*> parsePlusComma()
  {
    vector<NT*> nts;
    nts.push_back(parse<NT>());
    while(acceptPunct(COMMA))
    {
      nts.push_back(parse<NT>());
    }
    return nts;
  }
}

//Utils
ostream& operator<<(ostream& os, const Parser::Member& mem);

#endif

