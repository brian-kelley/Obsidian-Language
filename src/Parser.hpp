#ifndef PARSER_H
#define PARSER_H

#include "Common.hpp"
#include "Token.hpp"

#include "variadic-variant/variant.h"

//Use empty struct as default (first) value for variants (always trivially constructible)
struct None{};

enum struct Prim
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
  VOID,
  ERROR
};

struct Parser
{
  //Parser initialization
  Parser(vector<Token*>& toks);
  //Token stream management
  vector<vector<Token*>> tokenQueue;

  string emitBuffer;
  //execute the meta-statement starting at offset start in code stream
  void metaStatement(size_t start);
  void emit(string source);
  void emit(ParseNode* nonterm);
  void emit(Token* tok);

  struct Module;
  //Parse a program from token string (only parsing function called from main)
  Module* parseProgram();

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
  struct Match;
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
  struct SubroutineNT;
  struct SubroutineTypeNT;
  struct StructDecl;    // [value1, value2]
  struct StructLit;
  struct Member;
  //Syntactic types
  struct TupleTypeNT;   // (type1, type2, type3)
  struct UnionTypeNT;   // Haskell-style union: type1 | type2 | type3
  struct MapTypeNT;     // map/dictionary: (key : value)
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

  struct Module : public Node
  {
    vector<Token*> unparse();
    string name;
    vector<ScopedDecl*> decls;
  };
  
  struct ScopedDecl : public ParseNode
  {
    ScopedDecl() : decl(None()) {}
    vector<Token*> unparse();
    variant<
      None,
      Module*,
      VarDecl*,
      StructDecl*,
      Enum*,
      Typedef*,
      SubroutineNT*,
      ExternSubroutineNT*,
      TestDecl*> decl;
  };

  ScopedDecl* parseScopedDeclGivenMember(Member* mem);

  struct TypeNT : public ParseNode
  {
    TypeNT() : t(None()), arrayDims(0) {}
    vector<Token*> unparse();
    variant<
      None,
      Prim,
      Member*,
      TupleTypeNT*,
      UnionTypeNT*,
      MapTypeNT*,
      SubroutineTypeNT*> t;
    int arrayDims;
  };

  struct EmitNT : public ParseNode
  {
    EmitNT() : emitted("") {}
    vector<Token*> unparse();
    variant<string, Token*, ParseNode*> emitted;
  };

  struct StatementNT : public ParseNode
  {
    StatementNT() : s(None()) {}
    variant<
      None,
      ScopedDecl*,
      VarAssign*,
      PrintNT*,
      Expr12*,  //call only: can't discard any expression like in C
      Block*,
      Return*,
      Continue*,
      Break*,
      Switch*,
      Match*,
      For*,
      While*,
      If*,
      EmitNT*,
      Assertion*,
      EmptyStatement*> s;
  };

  struct Typedef : public ParseNode
  {
    Typedef() : type(nullptr) {}
    Typedef(string n, TypeNT* t) : ident(n), type(t) {}
    string ident;
    TypeNT* type;
  };

  struct Return : public ParseNode
  {
    Return() : ex(nullptr) {}
    //optional returned expression (NULL if unused)
    ExpressionNT* ex;
  };

  struct Switch : public ParseNode
  {
    ExpressionNT* value;
    struct Label
    {
      //label comes right before stmts[position]
      Label() : position(-1), value(NULL) {}
      Label(int p, ExpressionNT* v) : position(p), value(v) {}
      int position;
      ExpressionNT* value;
    };
    vector<Label> labels;
    //a pseudo-block
    //can't parse it directly because there will be case/default labels mixed in with the statements
    //also can't have any ScopedDecls
    Block* block;
    //default can go anywhere, but if not explicit then it set to stmts.size()
    int defaultPosition;
  };

  struct Match : public ParseNode
  {
    Match() : value(nullptr) {}
    //varName is implicitly created in each case with the case's type
    string varName;
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

  struct Continue : public ParseNode {};
  struct Break : public ParseNode {};
  struct EmptyStatement : public ParseNode {};

  struct For : public ParseNode
  {
    For() : f(None()), body(nullptr) {}
    variant<
      None,
      ForC*,
      ForOverArray*,
      ForRange*> f;
    Block* body;
  };

  struct ForC : public ParseNode
  {
    ForC() : decl(nullptr), condition(nullptr), incr(nullptr) {}
    //for(decl; condition; incr) <body>
    //allow arbitrary statements for dcel and incr, not just VarDecl and VarAssign
    StatementNT* decl;
    ExpressionNT* condition;
    StatementNT* incr;
  };

  struct ForOverArray : public ParseNode
  {
    ForOverArray() : expr(nullptr) {}
    vector<string> tup;
    ExpressionNT* expr;
  };

  struct ForRange : public ParseNode
  {
    ForRange() : start(nullptr), end(nullptr) {}
    string name;
    ExpressionNT* start;
    ExpressionNT* end;
  };

  struct While : public ParseNode
  {
    While() : cond(nullptr), body(nullptr) {}
    ExpressionNT* cond;
    Block* body;
  };

  struct If : public ParseNode
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

  struct Assertion : public ParseNode
  {
    Assertion() : expr(nullptr) {}
    ExpressionNT* expr;
  };

  struct TestDecl : public ParseNode
  {
    TestDecl() : block(nullptr) {}
    Block* block;
  };

  struct EnumItem : public ParseNode
  {
    EnumItem() : value(nullptr) {}
    string name;
    //value is optional
    //(NULL if not explicit, then set automatically)
    IntLit* value;
  };

  struct Enum : public ParseNode
  {
    string name;
    vector<EnumItem*> items;
  };

  struct Block : public ParseNode
  {
    Block() {}
    Block(vector<StatementNT*>& s) : statements(s) {}
    vector<StatementNT*> statements;
  };

  struct VarDecl : public ParseNode
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
    bool composed;
  };

  struct MetaVar : public ParseNode
  {
    MetaVar() : type(nullptr), val(nullptr) {}
    TypeNT* type;
    string name;
    ExpressionNT* val;
  };

  VarAssign* parseAssignGivenExpr12(Expr12* e12);

  struct VarAssign : public ParseNode
  {
    VarAssign() : target(nullptr), rhs(nullptr) {}
    //note: target must be an lvalue (checked in middle end)
    //but all lvalues are Expr12 so that is used here
    Expr12* target;
    ExpressionNT* rhs;
  };

  struct PrintNT : public ParseNode
  {
    vector<ExpressionNT*> exprs;
  };

  //Parameter - used by SubroutineNT
  struct Parameter : public ParseNode
  {
    TypeNT* type;
    //name is optional (if not present, is "")
    string name;
  };

  struct SubroutineNT : public ParseNode
  {
    SubroutineNT() : retType(nullptr), body(nullptr), meta(false) {}
    TypeNT* retType;
    vector<Parameter*> params;
    //body is optional in syntax
    //(correct semantics requires body == NULL if and only if inside a trait decl)
    Block* body;
    string name;
    bool isPure;
    bool isStatic;
    bool nonterm;
    bool meta;
  };

  struct ExternSubroutineNT : public ParseNode
  {
    ExternSubroutineNT() : type(nullptr) {}
    SubroutineTypeNT* type; //the type of this subroutine
    string c;               //the C code to insert for this call
  };

  struct SubroutineTypeNT : public ParseNode
  {
    SubroutineTypeNT() : retType(nullptr) {}
    TypeNT* retType;
    vector<Parameter*> params;
    //body is optional in syntax
    //(correct semantics requires body == NULL if and only if inside a trait decl)
    bool isPure;
    bool isStatic;
    bool nonterm;
  };

  struct StructDecl : public ParseNode
  {
    string name;
    vector<Member*> traits;
    vector<ScopedDecl*> members;
  };

  struct StructLit : public ParseNode
  {
    vector<ExpressionNT*> vals;
  };

  struct Member : public ParseNode
  {
    vector<string> names;
  };

  struct BoundedTypeNT : public ParseNode
  {
    //trait types of the form "<localName>: <traitNames> <argName>"
    //i.e.: int f(T: Num, Drawable value)
    string localName;
    vector<Member*> traits;
  };

  struct TupleTypeNT : public ParseNode
  {
    TupleTypeNT() {}
    TupleTypeNT(vector<TypeNT*> m) : members(m) {}
    vector<TypeNT*> members;
  };
  
  struct UnionTypeNT : public ParseNode
  {
    UnionTypeNT() {}
    UnionTypeNT(vector<TypeNT*> t) : types(t) {}
    vector<TypeNT*> types;
  };

  struct MapTypeNT : public ParseNode
  {
    MapTypeNT() {}
    MapTypeNT(TypeNT* k, TypeNT* v) : keyType(k), valueType(v) {}
    TypeNT* keyType;
    TypeNT* valueType;
  };

  struct BoolLit : public ParseNode
  {
    BoolLit() : val(false) {}
    BoolLit(bool v) : val(v) {}
    bool val;
  };

  struct Expr1 : public ParseNode
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

  struct Expr1RHS : public ParseNode
  {
    Expr1RHS() : rhs(nullptr) {}
    // || is only op
    Expr2* rhs;
  };

  struct Expr2 : public ParseNode
  {
    Expr2() : head(nullptr) {}
    Expr2(Expr3* e) : head(e) {}
    Expr2(Expr12* e12);
    Expr3* head;
    vector<Expr2RHS*> tail;
  };

  struct Expr2RHS : public ParseNode
  {
    Expr2RHS() : rhs(nullptr) {}
    // && is only op
    Expr3* rhs;
  };

  struct Expr3 : public ParseNode
  {
    Expr3() : head(nullptr) {}
    Expr3(Expr4* e) : head(e) {}
    Expr3(Expr12* e12);
    Expr4* head;
    vector<Expr3RHS*> tail;
  };

  struct Expr3RHS : public ParseNode
  {
    Expr3RHS() : rhs(nullptr) {}
    // | is only op
    Expr4* rhs;
  };

  struct Expr4 : public ParseNode
  {
    Expr4() : head(nullptr) {}
    Expr4(Expr5* e) : head(e) {}
    Expr4(Expr12* e12);
    Expr5* head;
    vector<Expr4RHS*> tail;
  };

  struct Expr4RHS : public ParseNode
  {
    Expr4RHS() : rhs(nullptr) {}
    // ^ is only op
    Expr5* rhs;
  };

  struct Expr5 : public ParseNode
  {
    Expr5() : head(nullptr) {}
    Expr5(Expr6* e) : head(e) {}
    Expr5(Expr12* e12);
    Expr6* head; 
    vector<Expr5RHS*> tail;
  };

  struct Expr5RHS : public ParseNode
  {
    Expr5RHS() : rhs(nullptr) {}
    // & is only op
    Expr6* rhs;
  };

  struct Expr6 : public ParseNode
  {
    Expr6() : head(nullptr) {}
    Expr6(Expr7* e) : head(e) {}
    Expr6(Expr12* e12);
    Expr7* head;
    vector<Expr6RHS*> tail;
  };

  struct Expr6RHS : public ParseNode
  {
    Expr6RHS() : rhs(nullptr) {}
    int op; //CMPEQ or CMPNEQ
    Expr7* rhs;
  };

  struct Expr7 : public ParseNode
  {
    Expr7() : head(nullptr) {}
    Expr7(Expr8* e) : head(e) {}
    Expr7(Expr12* e12);
    Expr8* head;
    vector<Expr7RHS*> tail;
  };

  struct Expr7RHS : public ParseNode
  {
    Expr7RHS() : rhs(nullptr) {}
    int op;  //CMPL, CMPLE, CMPG, CMPGE
    Expr8* rhs;
  };

  struct Expr8 : public ParseNode
  {
    Expr8() : head(nullptr) {}
    Expr8(Expr9* e) : head(e) {}
    Expr8(Expr12* e12);
    Expr9* head;
    vector<Expr8RHS*> tail;
  };

  struct Expr8RHS : public ParseNode
  {
    Expr8RHS() : rhs(nullptr) {}
    int op; //SHL, SHR
    Expr9* rhs;
  };

  struct Expr9 : public ParseNode
  {
    Expr9() : head(nullptr) {}
    Expr9(Expr10* e) : head(e) {}
    Expr9(Expr12* e12);
    Expr10* head;
    vector<Expr9RHS*> tail;
  };

  struct Expr9RHS : public ParseNode
  {
    Expr9RHS() : rhs(nullptr) {}
    int op; //PLUS, SUB
    Expr10* rhs;
  };

  struct Expr10 : public ParseNode
  {
    Expr10() : head(nullptr) {}
    Expr10(Expr11* e) : head(e) {}
    Expr10(Expr12* e12);
    Expr11* head;
    vector<Expr10RHS*> tail;
  };

  struct Expr10RHS : public ParseNode
  {
    Expr10RHS() : rhs(nullptr) {}
    int op; //MUL, DIV, MOD
    Expr11* rhs;
  };

  //Expr11 can be Expr12 or <op>Expr11
  struct Expr11 : public ParseNode
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

  struct NewArrayNT : public ParseNode
  {
    TypeNT* elemType;
    vector<ExpressionNT*> dimensions;
  };

  struct Expr12 : public ParseNode
  {
    Expr12() : e(None()) {}
    Expr12(ExpressionNT* expr) : e(expr) {}
    struct Error {};
    struct This {};
    variant<
      None,
      IntLit*,
      CharLit*,
      StrLit*,
      FloatLit*,
      BoolLit*,
      ExpressionNT*,      //for expression inside parentheses
      StructLit*,
      Member*,
      Error,
      This> e;
    vector<Expr12RHS*> tail;  //tail is a chain of 0 or more member acesses, array indices and member calls
  };

  void parseExpr12Tail(Expr12* head);
  Expr12* parseExpr12GivenMember(Member* mem);

  struct CallOp : public ParseNode
  {
    //the arguments inside parens (each may have the match operator)
    vector<ExpressionNT*> args;
  };

  struct Expr12RHS : public ParseNode
  {
    //to parse, get Member first, then if (...) seen is call, otherwise is member
    Expr12RHS() : e(None()) {}
    variant<
      None,
      string,       //struct member: ". Identifier"
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
ostream& operator<<(ostream& os, const Parser::ParseNode& pn);

#endif

