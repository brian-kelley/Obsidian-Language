#ifndef PARSER_H
#define PARSER_H

#include "Misc.hpp"
#include "Token.hpp"
#include "Utils.hpp"

#include "variadic-variant/variant.h"
#include <stdexcept>

using namespace std;

//Use empty struct as default value in some variants
struct None{};
typedef runtime_error ParseErr;

namespace Parser
{
  struct Module;
  //Parse a program from token string (only function needed outside namespace)
  Module* parseProgram(vector<Token*>& toks);

  //Token stream & utilities
  extern size_t pos;                //token iterator
  extern vector<Token*>* tokens;    //tokens from lexer

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
  Token* getNext();
  void unget();
  Token* lookAhead(int ahead);  //get token ahead elements ahead iter (0 means next token)
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
  struct ForArray;
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
  struct CallNT;
  struct Arg;
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
  struct TupleLit;
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

  struct Module
  {
    string name;
    vector<ScopedDecl*> decls;
  };

  struct ScopedDecl
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

  struct TypeNT
  {
    TypeNT() : t(None()), arrayDims(0) {}
    enum Prim
    {
      BOOL,
      CHAR,
      UCHAR,
      SHORT,
      USHORT,
      INT,
      UINT,
      LONG,
      ULONG,
      FLOAT,
      DOUBLE,
      STRING
    };
    struct Wildcard {};
    variant<
      None,
      Prim,
      Member*,
      TupleTypeNT*,
      FuncTypeNT*,
      ProcTypeNT*,
      TraitType*,
      Wildcard> t;
    int arrayDims;
  };

  struct StatementNT
  {
    StatementNT() : s(None()) {}
    variant<
      None,
      ScopedDecl*,
      VarAssign*,
      PrintNT*,
      Call*,
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
    Statement* stmt;
  };

  struct Typedef
  {
    Typedef() : type(nullptr) {}
    TypeNT* type;
    string ident;
  };

  struct Return
  {
    Return() : ex(nullptr) {}
    //optional returned expression (NULL if unused)
    ExpressionNT* ex;
  };

  struct SwitchCase
  {
    SwitchCase() : matchVal(nullptr), s(nullptr) {}
    ExpressionNT* matchVal;
    StatementNT* s;
  };

  struct Switch
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

  struct For
  {
    For() : f(None()), body(nullptr), scope(nullptr) {}
    variant<
      None,
      ForC*,
      ForRange1*,
      ForRange2*,
      ForArray*> f;
    BlockScope* scope;
    StatementNT* body;
  };

  struct ForC
  {
    ForC() : decl(nullptr), condition(nullptr), incr(nullptr) {}
    //for(decl; condition; incr) <body>
    //allow arbitrary statements for dcel and incr, not just VarDecl and VarAssign
    StatementNT* decl;
    ExpressionNT* condition;
    StatementNT* incr;
  };

  struct ForRange1
  {
    ForRange1() : expr(nullptr) {}
    ExpressionNT* expr;
  };

  struct ForRange2
  {
    ForRange2() : start(nullptr), end(nullptr) {}
    ExpressionNT* start;
    ExpressionNT* end;
  };

  struct ForArray
  {
    ForArray() : container(nullptr) {}
    ExpressionNT* container;
  };

  struct While
  {
    While() : cond(nullptr), body(nullptr), scope(nullptr) {}
    ExpressionNT* cond;
    StatementNT* body;
    BlockScope* scope;
  };

  struct If
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

  struct Using
  {
    Using() : mem(nullptr) {}
    Member* mem;
  };

  struct Assertion
  {
    Assertion() : expr(nullptr) {}
    ExpressionNT* expr;
  };

  struct TestDecl
  {
    TestDecl() : call(nullptr) {}
    CallNT* call;
  };

  struct EnumItem
  {
    EnumItem() : value(nullptr) {}
    string name;
    //value is optional
    //(NULL if not explicit, then set automatically)
    IntLit* value;
  };

  struct Enum
  {
    string name;
    vector<EnumItem*> items;
  };

  struct Block
  {
    vector<StatementNT*> statements;
    BlockScope* bs;
  };

  struct VarDecl
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

  struct VarAssign
  {
    VarAssign() : target(nullptr), rhs(nullptr) {}
    //note: target must be an lvalue (this will be checked in middle end)
    ExpressionNT* target;
    ExpressionNT* rhs;
  };

  struct PrintNT
  {
    vector<ExpressionNT*> exprs;
  };

  struct CallNT
  {
    CallNT() : callable(nullptr) {}
    //name of func/proc
    Member* callable;
    vector<ExpressionNT*> args;
  };

  struct Arg
  {
    Arg() : type(nullptr) {}
    TypeNT* type;
    //arg name is optional in some contexts
    bool haveName;
    string name;
  };

  struct FuncTypeNT
  {
    FuncTypeNT() : retType(nullptr) {}
    TypeNT* retType;
    vector<Arg*> args;
  };

  struct FuncDecl
  {
    string name;
    FuncTypeNT type;
  };

  struct FuncDef
  {
    FuncDef() : name(nullptr), body(nullptr) {}
    Member* name;
    FuncTypeNT type;
    Block* body;
  };

  struct ProcTypeNT
  {
    ProcTypeNT() : retType(nullptr) {}
    bool nonterm;
    TypeNT* retType;
    vector<Arg*> args;
  };

  struct ProcDecl
  {
    string name;
    ProcTypeNT type;
  };

  struct ProcDef
  {
    ProcDef() : name(nullptr), body(nullptr) {}
    Member* name;
    ProcTypeNT type;
    Block* body;
  };

  struct StructMem
  {
    StructMem() : sd(nullptr) {}
    ScopedDecl* sd;
    //composition only used if sd is a VarDecl
    bool compose;
  };

  struct StructDecl
  {
    string name;
    vector<Member*> traits;
    vector<StructMem*> members;
  };

  struct UnionDecl
  {
    string name;
    vector<TypeNT*> types;
  };

  struct TraitDecl
  {
    string name;
    vector<variant<
      None,
      FuncDecl*,
      ProcDecl*>> members;
  };

  struct StructLit
  {
    vector<ExpressionNT*> vals;
  };

  struct TupleLit
  {
    vector<ExpressionNT*> vals;
  };

  struct Member
  {
    vector<string> scopes;
    string ident;
  };

  struct TraitType
  {
    //trait types of the form "<localName>: <traitNames> <argName>"
    //i.e.: int f(T: Num, IO.Printable value)
    string localName;
    vector<Member*> traits;
  };

  struct TupleTypeNT
  {
    //cannot be empty
    vector<TypeNT*> members;
  };

  struct BoolLit
  {
    bool val;
  };

  struct Expr1
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
    Expr2* head;
    vector<Expr1RHS*> tail;
  };

  struct Expr1RHS
  {
    Expr1RHS() : rhs(nullptr) {}
    // || is only op
    Expr2* rhs;
  };

  struct Expr2
  {
    Expr2() : head(nullptr) {}
    Expr2(Expr3* e) : head(e) {}
    Expr2(Expr12* e12);
    Expr3* head;
    vector<Expr2RHS*> tail;
  };

  struct Expr2RHS
  {
    Expr2RHS() : rhs(nullptr) {}
    // && is only op
    Expr3* rhs;
  };

  struct Expr3
  {
    Expr3() : head(nullptr) {}
    Expr3(Expr4* e) : head(e) {}
    Expr3(Expr12* e12);
    Expr4* head;
    vector<Expr3RHS*> tail;
  };

  struct Expr3RHS
  {
    Expr3RHS() : rhs(nullptr) {}
    // | is only op
    Expr4* rhs;
  };

  struct Expr4
  {
    Expr4() : head(nullptr) {}
    Expr4(Expr5* e) : head(e) {}
    Expr4(Expr12* e12);
    Expr5* head;
    vector<Expr4RHS*> tail;
  };

  struct Expr4RHS
  {
    Expr4RHS() : rhs(nullptr) {}
    // ^ is only op
    Expr5* rhs;
  };

  struct Expr5
  {
    Expr5() : head(nullptr) {}
    Expr5(Expr6* e) : head(e) {}
    Expr5(Expr12* e12);
    Expr6* head; 
    vector<Expr5RHS*> tail;
  };

  struct Expr5RHS
  {
    Expr5RHS() : rhs(nullptr) {}
    // & is only op
    Expr6* rhs;
  };

  struct Expr6
  {
    Expr6() : head(nullptr) {}
    Expr6(Expr7* e) : head(e) {}
    Expr6(Expr12* e12);
    Expr7* head;
    vector<Expr6RHS*> tail;
  };

  struct Expr6RHS
  {
    Expr6RHS() : rhs(nullptr) {}
    int op; //CMPEQ or CMPNEQ
    Expr7* rhs;
  };

  struct Expr7
  {
    Expr7() : head(nullptr) {}
    Expr7(Expr8* e) : head(e) {}
    Expr7(Expr12* e12);
    Expr8* head;
    vector<Expr7RHS*> tail;
  };

  struct Expr7RHS
  {
    Expr7RHS() : rhs(nullptr) {}
    int op;  //CMPL, CMPLE, CMPG, CMPGE
    Expr8* rhs;
  };

  struct Expr8
  {
    Expr8() : head(nullptr) {}
    Expr8(Expr9* e) : head(e) {}
    Expr8(Expr12* e12);
    Expr9* head;
    vector<Expr8RHS*> tail;
  };

  struct Expr8RHS
  {
    Expr8RHS() : rhs(nullptr) {}
    int op; //SHL, SHR
    Expr9* rhs;
  };

  struct Expr9
  {
    Expr9() : head(nullptr) {}
    Expr9(Expr10* e) : head(e) {}
    Expr9(Expr12* e12);
    Expr10* head;
    vector<Expr9RHS*> tail;
  };

  struct Expr9RHS
  {
    Expr9RHS() : rhs(nullptr) {}
    int op; //PLUS, SUB
    Expr10* rhs;
  };

  struct Expr10
  {
    Expr10() : head(nullptr) {}
    Expr10(Expr11* e) : head(e) {}
    Expr10(Expr12* e12);
    Expr11* head;
    vector<Expr10RHS*> tail;
  };

  struct Expr10RHS
  {
    Expr10RHS() : rhs(nullptr) {}
    int op; //MUL, DIV, MOD
    Expr11* rhs;
  };

  //Expr11 can be Expr12 or <op>Expr11
  struct Expr11
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

  struct Expr12
  {
    Expr12() : e(None()) {}
    Expr12(ExpressionNT* expr) : e(expr) {}
    struct ArrayIndex
    {
      //arr[index]
      Expr12* arr;
      ExpressionNT* index;
    };
    variant<
      None,
      IntLit*,
      CharLit*,
      StrLit*,
      FloatLit*,
      BoolLit*,
      ExpressionNT*,
      Member*,
      StructLit*,
      TupleLit*,
      CallNT*,
      ArrayIndex> e;
  }; 

  //Parse a nonterminal of type NT
  template<typename NT>
  NT* parse();

  template<typename NT>
  NT* parseOptional()
  {
    int prevPos = pos;
    NT* nt = nullptr;
    try
    {
      nt = parse<NT>();
      return nt;
    }
    catch(...)
    {
      //backtrack
      pos = prevPos;
      return nt;
    }
  }

  //Parse some NTs
  template<typename NT>
  vector<NT*> parseSome()
  {
    vector<NT*> nts;
    while(true)
    {
      NT* nt = parseOptional<NT>();
      if(!nt)
      {
        break;
      }
      else
      {
        nts.push_back(nt);
      }
    }
    return nts;
  }

  //Parse some comma-separated NTs
  template<typename NT>
  vector<NT*> parseSomeCommaSeparated()
  {
    vector<NT*> nts;
    while(true)
    {
      if(nts.size() == 0)
      {
        NT* nt = parseOptional<NT>();
        if(nt)
        {
          nts.push_back(nt);
        }
        else
        {
          break;
        }
      }
      else
      {
        if(!acceptPunct(COMMA))
        {
          break;
        }
        nts.push_back(parse<NT>());
      }
    }
    return nts;
  }
}

//Utils
ostream& operator<<(ostream& os, const Parser::Member& mem);

#endif

