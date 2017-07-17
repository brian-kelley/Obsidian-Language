#ifndef PARSER_H
#define PARSER_H

#include "Misc.hpp"
#include "Token.hpp"
#include "Utils.hpp"
#include "AutoPtr.hpp"

#include <stdexcept>
#include <memory>
#include "variadic-variant/variant.h"

using namespace std;

//Use empty struct as default value in some variants
struct None{};
typedef runtime_error ParseErr;

namespace Parser
{
  struct Module;
  //Parse a program from token string (only function needed outside namespace)
  AP(Module) parseProgram(vector<Token*>& toks);

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
  struct Statement;
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
  struct Print;
  struct Call;
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
    vector<AP(ScopedDecl)> decls;
  };

  struct ScopedDecl
  {
    ScopedDecl();
    variant<
      None,
      AP(Module),
      AP(VarDecl),
      AP(StructDecl),
      AP(UnionDecl),
      AP(TraitDecl),
      AP(Enum),
      AP(Typedef),
      AP(FuncDecl),
      AP(FuncDef),
      AP(ProcDecl),
      AP(ProcDef),
      AP(TestDecl)> decl;
  };

  struct TypeNT
  {
    TypeNT();
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
      AP(Member),
      AP(TupleTypeNT),
      AP(FuncTypeNT),
      AP(ProcTypeNT),
      AP(TraitType),
      Wildcard> t;
    int arrayDims;
  };

  struct Statement
  {
    Statement();
    variant<
      None,
      AP(ScopedDecl),
      AP(VarAssign),
      AP(Print),
      AP(ExpressionNT),
      AP(Block),
      AP(Return),
      AP(Continue),
      AP(Break),
      AP(Switch),
      AP(For),
      AP(While),
      AP(If),
      AP(Assertion),
      AP(EmptyStatement),
      AP(VarDecl)> s;
  };

  struct Typedef
  {
    AP(TypeNT) type;
    string ident;
  };

  struct Return
  {
    //optional returned expression (NULL if unused)
    AP(ExpressionNT) ex;
  };

  struct SwitchCase
  {
    AP(ExpressionNT) matchVal;
    AP(Statement) s;
  };

  struct Switch
  {
    AP(ExpressionNT) sw;
    vector<AP(SwitchCase)> cases;
    //optional default: statement, NULL if unused
    AP(Statement) defaultStatement;
  };

  struct Continue {};
  struct Break {};
  struct EmptyStatement {};

  struct For
  {
    For();
    variant<
      None,
      AP(ForC),
      AP(ForRange1),
      AP(ForRange2),
      AP(ForArray)> f;
    AP(Statement) body;
  };

  struct ForC
  {
    AP(VarDecl) decl;
    AP(ExpressionNT) condition;
    AP(VarAssign) incr;
  };

  struct ForRange1
  {
    AP(ExpressionNT) expr;
  };

  struct ForRange2
  {
    AP(ExpressionNT) start;
    AP(ExpressionNT) end;
  };

  struct ForArray
  {
    AP(ExpressionNT) container;
  };

  struct While
  {
    AP(ExpressionNT) cond;
    AP(Statement) body;
  };

  struct If
  {
    AP(ExpressionNT) cond;
    AP(Statement) ifBody;
    //elseBody NULL if there is no else clause
    AP(Statement) elseBody;
  };

  struct Using
  {
    AP(Member) mem;
  };

  struct Assertion
  {
    AP(ExpressionNT) expr;
  };

  struct TestDecl
  {
    AP(Call) call;
  };

  struct EnumItem
  {
    string name;
    //value is optional
    //(NULL if not explicit, then set automatically)
    IntLit* value;
  };

  struct Enum
  {
    string name;
    vector<AP(EnumItem)> items;
  };

  struct Block
  {
    vector<AP(Statement)> statements;
  };

  struct VarDecl
  {
    //NULL if "auto"
    AP(TypeNT) type;
    //var decl's name can only be a single identifier (but variables can be referenced via compound idents)
    string name;
    //NULL if not initialization, but required if auto
    AP(ExpressionNT) val;
    //true if static, false otherwise
    //if true, only has valid semantics if inside a struct decl
    bool isStatic;
  };

  struct VarAssign
  {
    //note: target must be an lvalue (this is checked in middle end)
    AP(ExpressionNT) target;
    Oper* op;
    //NULL if ++ or --
    AP(ExpressionNT) rhs;
  };

  struct Print
  {
    vector<AP(ExpressionNT)> exprs;
  };

  struct Call
  {
    //name of func/proc
    AP(Member) callable;
    vector<AP(ExpressionNT)> args;
  };

  struct Arg
  {
    AP(TypeNT) type;
    //arg name is optional in some contexts
    bool haveName;
    string name;
  };

  struct FuncTypeNT
  {
    AP(TypeNT) retType;
    vector<AP(Arg)> args;
  };

  struct FuncDecl
  {
    string name;
    FuncTypeNT type;
  };

  struct FuncDef
  {
    AP(Member) name;
    FuncTypeNT type;
    AP(Block) body;
  };

  struct ProcTypeNT
  {
    bool nonterm;
    AP(TypeNT) retType;
    vector<AP(Arg)> args;
  };

  struct ProcDecl
  {
    string name;
    ProcTypeNT type;
  };

  struct ProcDef
  {
    AP(Member) name;
    ProcTypeNT type;
    AP(Block) body;
  };

  struct StructMem
  {
    AP(ScopedDecl) sd;
    //composition only used if sd is a VarDecl
    bool compose;
  };

  struct StructDecl
  {
    string name;
    vector<AP(Member)> traits;
    vector<AP(StructMem)> members;
  };

  struct UnionDecl
  {
    string name;
    vector<AP(TypeNT)> types;
  };

  struct TraitDecl
  {
    string name;
    vector<variant<
      None,
      AP(FuncDecl),
      AP(ProcDecl)>> members;
  };

  struct StructLit
  {
    vector<AP(ExpressionNT)> vals;
  };

  struct TupleLit
  {
    vector<AP(ExpressionNT)> vals;
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
    vector<AP(Member)> traits;
  };

  struct TupleTypeNT
  {
    //cannot be empty
    vector<AP(TypeNT)> members;
  };

  struct BoolLit
  {
    bool val;
  };

  struct Expr1
  {
    AP(Expr2) head;
    vector<AP(Expr1RHS)> tail;
  };

  struct Expr1RHS
  {
    // || is only op
    AP(Expr2) rhs;
  };

  struct Expr2
  {
    AP(Expr3) head;
    vector<AP(Expr2RHS)> tail;
  };

  struct Expr2RHS
  {
    // && is only op
    AP(Expr3) rhs;
  };

  struct Expr3
  {
    AP(Expr4) head;
    vector<AP(Expr3RHS)> tail;
  };

  struct Expr3RHS
  {
    // | is only op
    AP(Expr4) rhs;
  };

  struct Expr4
  {
    AP(Expr5) head;
    vector<AP(Expr4RHS)> tail;
  };

  struct Expr4RHS
  {
    // ^ is only op
    AP(Expr5) rhs;
  };

  struct Expr5
  {
    AP(Expr6) head; 
    vector<AP(Expr5RHS)> tail;
  };

  struct Expr5RHS
  {
    // & is only op
    AP(Expr6) rhs;
  };

  struct Expr6
  {
    AP(Expr7) head;
    vector<AP(Expr6RHS)> tail;
  };

  struct Expr6RHS
  {
    int op; //CMPEQ or CMPNEQ
    AP(Expr7) rhs;
  };

  struct Expr7
  {
    AP(Expr8) head;
    vector<AP(Expr7RHS)> tail;
  };

  struct Expr7RHS
  {
    int op;  //CMPL, CMPLE, CMPG, CMPGE
    AP(Expr8) rhs;
  };

  struct Expr8
  {
    AP(Expr9) head;
    vector<AP(Expr8RHS)> tail;
  };

  struct Expr8RHS
  {
    int op; //SHL, SHR
    AP(Expr9) rhs;
  };

  struct Expr9
  {
    AP(Expr10) head;
    vector<AP(Expr9RHS)> tail;
  };

  struct Expr9RHS
  {
    int op; //PLUS, SUB
    AP(Expr10) rhs;
  };

  struct Expr10
  {
    AP(Expr11) head;
    vector<AP(Expr10RHS)> tail;
  };

  struct Expr10RHS
  {
    int op; //MUL, DIV, MOD
    AP(Expr11) rhs;
  };

  //Expr11 can be Expr12 or <op>Expr11
  struct Expr11
  {
    Expr11();
    struct UnaryExpr
    {
      int op; //SUB, LNOT, BNOT
      AP(Expr11) rhs;
    };
    variant<None,
      AP(Expr12),
      UnaryExpr> e;
  };

  struct Expr12
  {
    Expr12();
    struct ArrayIndex
    {
      //arr[index]
      AP(Expr12) arr;
      AP(ExpressionNT) index;
    };
    variant<
      None,
      IntLit*,
      CharLit*,
      StrLit*,
      FloatLit*,
      AP(BoolLit),
      AP(ExpressionNT),
      AP(Member),
      AP(StructLit),
      AP(TupleLit),
      AP(Call),
      ArrayIndex> e;
  }; 

  //Parse a nonterminal of type NT
  template<typename NT>
  AP(NT) parse();

  template<typename NT>
  AP(NT) parseOptional()
  {
    int prevPos = pos;
    AP(NT) nt;
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
  vector<AP(NT)> parseSome()
  {
    vector<AP(NT)> nts;
    while(true)
    {
      AP(NT) nt = parseOptional<NT>();
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
  vector<AP(NT)> parseSomeCommaSeparated()
  {
    vector<AP(NT)> nts;
    while(true)
    {
      if(nts.size() == 0)
      {
        AP(NT) nt = parseOptional<NT>();
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

#endif

