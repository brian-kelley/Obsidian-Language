#ifndef PARSER_H
#define PARSER_H

#include "Misc.hpp"
#include "Token.hpp"
#include "Utils.hpp"
#include "AutoPtr.hpp"

#include <stdexcept>
#include <memory>
#include "variant.h"

using namespace std;

//Use empty struct as default value in some variants
struct None{};
typedef runtime_error ParseErr;

namespace Parser
{
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
  struct Module;
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
  struct Expression;
  struct Call;
  struct Arg;
  struct FuncDecl;
  struct FuncDef;
  struct FuncType;
  struct ProcDecl;
  struct ProcDef;
  struct ProcType;
  struct StructDecl;
  struct VariantDecl;
  struct TraitDecl;
  struct StructLit;
  struct Member;
  struct TraitType;
  struct TupleType;
  struct BoolLit;
  struct Expr1;
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
    Scope* enclosing;
    variant<
      None,
      AP(Module),
      AP(VarDecl),
      AP(StructDecl),
      AP(VariantDecl),
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
    Type* entry;        //TypeSystem type table entry for this
    enum struct Prim
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
    variant<
      None,
      Prim,
      AP(Member),
      AP(TupleType),
      AP(FuncType),
      AP(ProcType)> t;
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
      AP(Expression),
      AP(Block),
      AP(Return),
      AP(Continue),
      AP(Break),
      AP(Switch),
      AP(For),
      AP(While),
      AP(If),
      AP(Using),
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
    AP(Expression) ex;
  };

  struct SwitchCase
  {
    AP(Expression) matchVal;
    AP(Statement) s;
  };

  struct Switch
  {
    AP(Expression) sw;
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
    AP(Expression) condition;
    AP(VarAssign) incr;
  };

  struct ForRange1
  {
    AP(Expression) expr;
  };

  struct ForRange2
  {
    AP(Expression) start;
    AP(Expression) end;
  };

  struct ForArray
  {
    AP(Expression) container;
  };

  struct While
  {
    AP(Expression) cond;
    AP(Statement) body;
  };

  struct If
  {
    AP(Expression) cond;
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
    AP(Expression) expr;
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
    AP(BlockScope) scope;
    vector<AP(Statement)> statements;
  };

  struct VarDecl
  {
    //NULL if "auto"
    AP(TypeNT) type;
    string name;
    //NULL if not initialization, but required if auto
    AP(Expression) val;
  };

  struct VarAssign
  {
    AP(Member) target;
    Oper* op;
    //NULL if ++ or --
    AP(Expression) rhs;
  };

  struct Print
  {
    vector<AP(Expression)> exprs;
  };

  struct Expression
  {
    Expression();
    variant<
      None,
      AP(Call),
      AP(Member),
      AP(Expr1)> e;
  };

  struct Call
  {
    //name of func/proc
    AP(Member) callable;
    vector<AP(Expression)> args;
  };

  struct Arg
  {
    Arg();
    variant<
      None,
      AP(TypeNT),
      AP(TraitType)> t;
    bool haveName;
    string name;
  };

  struct FuncDecl
  {
    AP(TypeNT) retType;
    string name;
    vector<AP(Arg)> args;
  };

  struct FuncDef
  {
    AP(TypeNT) retType;
    AP(Member) name;
    vector<AP(Arg)> args;
    AP(Block) body;
  };

  struct FuncType
  {
    AP(TypeNT) retType;
    AP(Member) name;
    vector<AP(Arg)> args;
  };

  struct ProcDecl
  {
    bool nonterm;
    AP(TypeNT) retType;
    string name;
    vector<AP(Arg)> args;
  };

  struct ProcDef
  {
    bool nonterm;
    AP(TypeNT) retType;
    AP(Member) name;
    vector<AP(Arg)> args;
    AP(Block) body;
  };

  struct ProcType
  {
    bool nonterm;
    AP(TypeNT) retType;
    AP(Member) name;
    vector<AP(Arg)> args;
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
    AP(StructScope) scope;
    vector<AP(Member)> traits;
    vector<AP(StructMem)> members;
  };

  struct VariantDecl
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
    vector<AP(Expression)> vals;
  };

  struct Member
  {
    string owner;
    //mem is optional
    AP(Member) mem;
  };

  struct TraitType
  {
    //trait types of the form "<localName> : <traitName>"
    string localName;
    AP(Member) traitName;
  };

  struct TupleType
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
      Expression* index;
    };
    variant<
      None,
      IntLit*,
      CharLit*,
      StrLit*,
      FloatLit*,
      AP(BoolLit),
      AP(Expression),
      AP(Member),
      AP(StructLit),
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
          nts.push_back(nt);
        else
          break;
      }
      else
      {
        if(!acceptPunct(COMMA))
          break;
        nts.push_back(parse<NT>());
      }
    }
    return nts;
  }
}

#endif
