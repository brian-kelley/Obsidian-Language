#ifndef PARSER_H
#define PARSER_H

#include "Common.hpp"
#include "PoolAlloc.hpp"
#include "Token.hpp"

#include "variadic-variant/variant.h"

//Use empty struct as default (first) value for variants (always trivially constructible)
struct None{};
struct BlockScope;

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
  Token* lookAhead(int n = 0);  //get the next token without advancing pos
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
  struct ForOverArray;
  struct ForRange;
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
  struct Parameter;
  struct ParamType;
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
  struct BoundedTypeNT;
  struct TupleTypeNT;
  struct UnionTypeNT; //Haskell-style union: type1 | type2 | type3
  struct MapTypeNT;   //Python-style map/dictionary: (key : value)
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

  struct ParseNode
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

  ScopedDecl* parseScopedDeclGivenMember(Member* mem);

  struct TypeNT
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
      UnionTypeNT*,
      MapTypeNT*,
      SubroutineTypeNT*,
      TTypeNT> t;
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

  struct Switch
  {
    Switch() : value(nullptr) {}
    //sw's type should be a union (checked in middle end)
    ExpressionNT* value;
    //switch is a list of cases (no fall-through)
    struct Case
    {
      Case() : type(NULL), block(NULL) {}
      Case(TypeNT* t, Block* b) : type(t), block(b) {}
      TypeNT* type;
      Block* block;
    };
    vector<Case> cases;
  };

  struct Match
  {
    ExpressionNT* value;
    struct Label
    {
      //label comes before stmts[position]
      Label() : position(-1), value(NULL) {}
      Label(int p, ExpressionNT* v) : position(p), value(v) {}
      int position;
      ExpressionNT* value;
    };
    vector<Label> labels;
    vector<StatementNT*> stmts;
    //default can go anywhere, but if not explicit then it set to stmts.size()
    int defaultPosition;
  };

  struct Continue {};
  struct Break {};
  struct EmptyStatement {};

  struct For
  {
    For() : f(None()), body(nullptr) {}
    variant<
      None,
      ForC*,
      ForOverArray*,
      ForRange*> f;
    Block* body;
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

  struct ForOverArray
  {
    ForOverArray() : expr(nullptr) {}
    StructLit* tup;
    ExpressionNT* expr;
  };

  struct ForRange
  {
    ForRange() : start(nullptr), end(nullptr) {}
    Ident* name;
    ExpressionNT* start;
    ExpressionNT* end;
  };

  struct While
  {
    While() : cond(nullptr), body(nullptr) {}
    ExpressionNT* cond;
    Block* body;
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

  struct Assertion
  {
    Assertion() : expr(nullptr) {}
    ExpressionNT* expr;
  };

  struct TestDecl
  {
    TestDecl() : stmt(nullptr) {}
    StatementNT* stmt;
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
    Block() : bs(NULL) {}
    Block(vector<StatementNT*>& s) : statements(s), bs(NULL) {}
    vector<StatementNT*> statements;
    BlockScope* bs;
  };

  struct VarDecl
  {
    VarDecl() : type(nullptr), val(nullptr) {}
    //NULL if "auto"
    TypeNT* type;
    string name;
    //initializing expression (optional)
    ExpressionNT* val;
    //true if static, false otherwise
    //if true, only has valid semantics if inside a struct decl
    bool isStatic;
  };

  VarAssign* parseAssignGivenExpr12(Expr12* e12);

  struct VarAssign
  {
    VarAssign() : target(nullptr), rhs(nullptr) {}
    //note: target must be an lvalue (checked in middle end)
    //but all lvalues are Expr12 so that is used here
    Expr12* target;
    ExpressionNT* rhs;
  };

  struct PrintNT
  {
    vector<ExpressionNT*> exprs;
  };

  //Parameter - used by Func/Proc Def (not decl)
  struct Parameter
  {
    Parameter() : name(nullptr) {}
    variant<None, TypeNT*, BoundedTypeNT*> type;
    //name is optional
    Ident* name;
  };

  struct SubroutineTypeNT
  {
    SubroutineTypeNT() : retType(nullptr) {}
    virtual ~SubroutineTypeNT() {}
    TypeNT* retType;
    vector<Parameter*> params;
    bool isStatic;
  };

  struct FuncTypeNT : public SubroutineTypeNT
  {};

  struct FuncDecl
  {
    string name;
    FuncTypeNT type;
  };

  struct FuncDef
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

  struct ProcDecl
  {
    string name;
    ProcTypeNT type;
  };

  struct ProcDef
  {
    ProcDef() : body(nullptr) {}
    string name;
    ProcTypeNT type;
    Block* body;
  };

  struct StructMem
  {
    StructMem() : sd(nullptr) {}
    ScopedDecl* sd;
    //composition can only apply to VarDecls
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
    UnionTypeNT* type;
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

  struct Member
  {
    vector<Ident*> head;
    Ident* tail;
  };

  struct BoundedTypeNT
  {
    //trait types of the form "<localName>: <traitNames> <argName>"
    //i.e.: int f(T: Num, Drawable value)
    string localName;
    vector<Member*> traits;
  };

  struct TupleTypeNT
  {
    vector<TypeNT*> members;
  };
  
  struct UnionTypeNT
  {
    vector<TypeNT*> types;
  };

  struct MapTypeNT
  {
    Type* keyType;
    Type* valueType;
  };

  struct BoolLit
  {
    BoolLit() : val(false) {}
    BoolLit(bool v) : val(v) {}
    bool val;
  };

  struct Expr1
  {
    Expr1() {}
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
    variant<None, Expr2*, NewArrayNT*> e;
    //note: if e.is<NewArrayNT*>, tail is empty and unused
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

  struct NewArrayNT
  {
    TypeNT* elemType;
    vector<ExpressionNT*> dimensions;
  };

  struct Expr12
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
      Member*> e;
    vector<Expr12RHS*> tail;  //tail is a chain of 0 or more member acesses, array indices and member calls
  };

  void parseExpr12Tail(Expr12* head);
  Expr12* parseExpr12GivenMember(Member* mem);

  struct CallOp
  {
    //the arguments inside parens (each may have the match operator)
    vector<ExpressionNT*> args;
  };

  struct Expr12RHS
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

