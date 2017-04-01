#ifndef PARSER_H
#define PARSER_H

#include "Misc.hpp"
#include "Token.hpp"
#include "Type.hpp"

#include <stdexcept>
#include <memory>
#include "variadic-variant/variant.h"

using namespace std;

/*
template<typename T>
struct upHelper
{
  //typedef unique_ptr<T, default_delete<T>> upImpl;
  typedef shared_ptr<T> upImpl;
};

#define UP(T) typename upHelper<T>::upImpl
*/

template<typename T>
struct AutoPtr
{
  AutoPtr()
  {
    p = nullptr;
  }
  AutoPtr(T* newPtr)
  {
    p = newPtr;
  }
  AutoPtr(const AutoPtr& rhs)
  {
    p = rhs.p;
    ((AutoPtr&) rhs).p = nullptr;
  }
  T& operator*() const
  {
    return *p;
  }
  T* operator->() const
  {
    return p;
  }
  T* operator=(const AutoPtr& rhs)
  {
    p = rhs.p;
    rhs.p = nullptr;
    return p;
  }
  operator bool() const
  {
    return p != nullptr;
  }
  bool operator!() const
  {
    return p == nullptr;
  }
  ~AutoPtr()
  {
    if(p)
    {
      delete p;
    }
  }
  mutable T* p;
};

#define UP(T) AutoPtr<T>

//Use empty struct as default value in some variants
struct None{};
typedef runtime_error ParseErr;

namespace Parser
{
  struct ModuleDef;
  //Parse a program from token string (only function needed outside namespace)
  UP(ModuleDef) parseProgram(vector<Token*>& toks);

  //Token stream utilities
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

  //the types of nonterminals
  enum struct NodeType
  {
    MODULE,
    MODULE_DEF,
    SCOPED_DECL,
    TYPE,
    STATEMENT,
    EMPTY_STATEMENT,
    TYPEDEF,
    RETURN,
    CONTINUE,
    BREAK,
    SWITCH,
    FOR,
    WHILE,
    IF,
    USING,
    ASSSERTION,
    TEST_DECL,
    ENUM,
    BLOCK,
    VAR_DECL,
    VAR_ASSIGN,
    PRINT,
    EXPRESSION,
    CALL,
    ARG,
    ARGS,
    FUNC_DECL,
    FUNC_DEF,
    FUNC_TYPE,
    PROC_DECL,
    PROC_DEF,
    PROC_TYPE,
    STRUCT_DECL,
    VARIANT_DECL,
    TRAIT_DECL,
    ARRAY_LIT,
    STRUCT_LIT,
    ERROR,
    MEMBER,
    TRAIT_TYPE,
    BOOL_LIT,
    EXPR_1,
    EXPR_1_RHS,
    EXPR_2,
    EXPR_2_RHS,
    EXPR_3,
    EXPR_3_RHS,
    EXPR_4,
    EXPR_4_RHS,
    EXPR_5,
    EXPR_5_RHS,
    EXPR_6,
    EXPR_6_RHS,
    EXPR_7,
    EXPR_7_RHS,
    EXPR_8,
    EXPR_8_RHS,
    EXPR_9,
    EXPR_9_RHS,
    EXPR_10,
    EXPR_10_RHS,
    EXPR_11,
    EXPR_11_RHS,
    EXPR_12
  };
  
  //lots of mutual recursion in nonterminal structs so forward-declare all
  struct Module;
  struct ScopedDecl;
  struct Type;
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
  struct Expr11RHS;
  struct Expr12;

  struct Module
  {
    string name;
    UP(ModuleDef) def;
  };

  struct ModuleDef
  {
    vector<UP(ScopedDecl)> decls;
  };

  struct ScopedDecl
  {
    ScopedDecl();
    variant<
      None,
      UP(Module),
      UP(VarDecl),
      UP(StructDecl),
      UP(VariantDecl),
      UP(TraitDecl),
      UP(Enum),
      UP(Typedef),
      UP(FuncDecl),
      UP(FuncDef),
      UP(ProcDecl),
      UP(ProcDef),
      UP(TestDecl)> decl;
  };

  struct Type
  {
    Type();
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
      UP(Member),
      UP(TupleType)> t;
    int arrayDims;
  };

  struct Statement
  {
    Statement();
    variant<
      None,
      UP(ScopedDecl),
      UP(VarAssign),
      UP(Print),
      UP(Expression),
      UP(Block),
      UP(Return),
      UP(Continue),
      UP(Break),
      UP(Switch),
      UP(For),
      UP(While),
      UP(If),
      UP(Using),
      UP(Assertion),
      UP(EmptyStatement)> s;
  };

  struct Typedef
  {
    UP(Type) type;
    string ident;
  };

  struct Return
  {
    //optional returned expression (NULL if unused)
    UP(Expression) ex;
  };

  struct SwitchCase
  {
    UP(Expression) matchVal;
    UP(Statement) s;
  };

  struct Switch
  {
    UP(Expression) sw;
    vector<UP(SwitchCase)> cases;
    //optional default: statement, NULL if unused
    UP(Statement) defaultStatement;
  };

  struct Continue {};
  struct Break {};
  struct EmptyStatement {};

  struct For
  {
    For();
    variant<
      None,
      UP(ForC),
      UP(ForRange1),
      UP(ForRange2),
      UP(ForArray)> f;
    UP(Statement) body;
  };

  struct ForC
  {
    UP(VarDecl) decl;
    UP(Expression) condition;
    UP(VarAssign) incr;
  };

  struct ForRange1
  {
    UP(Expression) expr;
  };

  struct ForRange2
  {
    UP(Expression) start;
    UP(Expression) end;
  };

  struct ForArray
  {
    UP(Expression) container;
  };

  struct While
  {
    UP(Expression) cond;
    UP(Statement) body;
  };

  struct If
  {
    UP(Expression) cond;
    UP(Statement) ifBody;
    //elseBody NULL if there is no else clause
    UP(Statement) elseBody;
  };

  struct Using
  {
    UP(Member) mem;
  };

  struct Assertion
  {
    UP(Expression) expr;
  };

  struct TestDecl
  {
    UP(Call) call;
  };

  struct EnumItem
  {
    string name;
    //value is optional (assigned automatically if not explicit)
    IntLit* value;
  };

  struct Enum
  {
    string name;
    vector<UP(EnumItem)> items;
  };

  struct Block
  {
    vector<UP(Statement)> statements;
  };

  struct VarDecl
  {
    //NULL if "auto"
    UP(Type) type;
    string name;
    //NULL if not initialization, but required if auto
    UP(Expression) val;
  };

  struct VarAssign
  {
    UP(Member) target;
    Oper* op;
    //NULL if ++ or --
    UP(Expression) rhs;
  };

  struct Print
  {
    vector<UP(Expression)> exprs;
  };

  struct Expression
  {
    Expression();
    variant<
      None,
      UP(Call),
      UP(Member),
      UP(Expr1)> e;
  };

  struct Call
  {
    //name of func/proc
    UP(Member) callable;
    vector<UP(Expression)> args;
  };

  struct Arg
  {
    Arg();
    variant<
      None,
      UP(Type),
      UP(TraitType)> t;
    bool haveName;
    string name;
  };

  struct FuncDecl
  {
    UP(Type) retType;
    string name;
    vector<UP(Arg)> args;
  };

  struct FuncDef
  {
    UP(Type) retType;
    UP(Member) name;
    vector<UP(Arg)> args;
    UP(Block) body;
  };

  struct FuncType
  {
    UP(Type) retType;
    UP(Member) name;
    vector<UP(Arg)> args;
  };

  struct ProcDecl
  {
    bool nonterm;
    UP(Type) retType;
    string name;
    vector<UP(Arg)> args;
  };

  struct ProcDef
  {
    bool nonterm;
    UP(Type) retType;
    UP(Member) name;
    vector<UP(Arg)> args;
    UP(Block) body;
  };

  struct ProcType
  {
    bool nonterm;
    UP(Type) retType;
    UP(Member) name;
    vector<UP(Arg)> args;
  };

  struct StructMem
  {
    UP(ScopedDecl) sd;
    //composition only used if sd is a VarDecl
    bool compose;
  };

  struct StructDecl
  {
    string name;
    vector<UP(Member)> traits;
    vector<UP(StructMem)> members;
  };

  struct VariantDecl
  {
    string name;
    vector<UP(Type)> types;
  };

  struct TraitDecl
  {
    string name;
    vector<variant<
      None,
      UP(FuncDecl),
      UP(ProcDecl)>> members;
  };

  struct StructLit
  {
    vector<UP(Expression)> vals;
  };

  struct Member
  {
    string owner;
    //mem is optional
    UP(Member) mem;
  };

  struct TraitType
  {
    //trait types of the form "<localName> : <traitName>"
    string localName;
    UP(Member) traitName;
  };

  struct TupleType
  {
    //cannot be empty
    vector<UP(Type)> members;
  };

  struct BoolLit
  {
    bool val;
  };

  struct Expr1
  {
    UP(Expr2) head;
    vector<UP(Expr1RHS)> tail;
  };

  struct Expr1RHS
  {
    // || is only op
    UP(Expr2) rhs;
  };

  struct Expr2
  {
    UP(Expr3) head;
    vector<UP(Expr2RHS)> tail;
  };

  struct Expr2RHS
  {
    // && is only op
    UP(Expr3) rhs;
  };

  struct Expr3
  {
    UP(Expr4) head;
    vector<UP(Expr3RHS)> tail;
  };

  struct Expr3RHS
  {
    // | is only op
    UP(Expr4) rhs;
  };

  struct Expr4
  {
    UP(Expr5) head;
    vector<UP(Expr4RHS)> tail;
  };

  struct Expr4RHS
  {
    // ^ is only op
    UP(Expr5) rhs;
  };

  struct Expr5
  {
    UP(Expr6) head; 
    vector<UP(Expr5RHS)> tail;
  };

  struct Expr5RHS
  {
    // & is only op
    UP(Expr6) rhs;
  };

  struct Expr6
  {
    UP(Expr7) head;
    vector<UP(Expr6RHS)> tail;
  };

  struct Expr6RHS
  {
    int op; //CMPEQ or CMPNEQ
    UP(Expr7) rhs;
  };

  struct Expr7
  {
    UP(Expr8) head;
    vector<UP(Expr7RHS)> tail;
  };

  struct Expr7RHS
  {
    int op;  //CMPL, CMPLE, CMPG, CMPGE
    UP(Expr8) rhs;
  };

  struct Expr8
  {
    UP(Expr9) head;
    vector<UP(Expr8RHS)> tail;
  };

  struct Expr8RHS
  {
    int op; //SHL, SHR
    UP(Expr9) rhs;
  };

  struct Expr9
  {
    UP(Expr10) head;
    vector<UP(Expr9RHS)> tail;
  };

  struct Expr9RHS
  {
    int op; //PLUS, SUB
    UP(Expr10) rhs;
  };

  struct Expr10
  {
    UP(Expr11) head;
    vector<UP(Expr10RHS)> tail;
  };

  struct Expr10RHS
  {
    int op; //MUL, DIV, MOD
    UP(Expr11) rhs;
  };

  //Expr11 can be Expr12 or <op>Expr11
  struct Expr11
  {
    Expr11();
    variant<None,
      UP(Expr12),
      UP(Expr11RHS)> e;
  };

  struct Expr11RHS
  {
    int op; //SUB, LNOT, BNOT
    UP(Expr11) base;
  };

  struct Expr12
  {
    Expr12();
    variant<
      None,
      IntLit*,
      CharLit*,
      StrLit*,
      FloatLit*,
      UP(BoolLit),
      UP(Expression),
      UP(Member),
      UP(StructLit)> e;
  }; 

  //Parse a nonterminal of type NT
  template<typename NT>
  UP(NT) parse();

  template<typename NT>
  UP(NT) parseOptional()
  {
    int prevPos = pos;
    UP(NT) nt;
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
  vector<UP(NT)> parseSome()
  {
    vector<UP(NT)> nts;
    while(true)
    {
      UP(NT) nt = parseOptional<NT>();
      if(!nt)
        break;
      else
        nts.push_back(nt);
    };
    return nts;
  }

  //Parse some comma-separated NTs
  template<typename NT>
  vector<UP(NT)> parseSomeCommaSeparated()
  {
    vector<UP(NT)> nts;
    while(true)
    {
      if(nts.size() != 0)
      {
        if(!acceptPunct(COMMA))
          break;
      }
      UP(NT) nt = parseOptional<NT>();
      if(!nt)
        break;
      else
        nts.push_back(nt);
    };
    return nts;
  }
}

#endif
