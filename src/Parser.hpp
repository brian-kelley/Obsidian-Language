#ifndef PARSER_H
#define PARSER_H

#include "Misc.hpp"
#include "Token.hpp"
#include "Type.hpp"

#include <stdexcept>

using namespace std;

typedef runtime_error ParseErr;

namespace Parser
{
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
  
  struct Nonterm
  {
    virtual int getType() = 0;
  }

  //lots of mutual recursion in nonterminal structs so forward-declare all
  struct Module;
  struct ModuleDef;
  struct ScopedDecl;
  struct Type;
  struct Statement;
  struct Typedef;
  struct Return;
  struct Switch;
  struct For;
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
  struct Args;
  struct FuncDecl;
  struct FuncDef;
  struct FuncType;
  struct ProcDecl;
  struct ProcDef;
  struct ProcType;
  struct StructDecl;
  struct VariantDecl;
  struct TraitDecl;
  struct ArrayLit;
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

  struct Module : public Nonterm
  {
    Ident* name;
    ModuleDef* def;
  };

  struct ModuleDef : public Nonterm
  {
    vector<ScopedDecl*> decls;
  };

  struct ScopedDecl : public Nonterm
  {
    Nonterm* nt;
  };

  struct Type : public Nonterm
  {
    struct ArrayType
    {
      Type* t;
      int dims;
    };
    enum Primitives
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
    enum TypeType
    {
      PRIMITIVE,
      MEMBER,
      ARRAY,
      TUPLE
    };
    int type; //TypeType value
    union
    {
      int prim;
      Member* mem;
      ArrayType arr;
      TupleType tup;
    } t;
  };

  struct Statement : public Nonterm
  {
    Nonterm* nt;
  };

  struct Typedef : public Nonterm
  {
    Type* type;
    Ident* ident;
  };

  struct Return : public Nonterm
  {
    //optional returned expression (NULL if unused)
    Expression* ex;
  };

  struct Switch : public Nonterm
  {
    Expression* sw;
    struct SwitchCase
    {
      Expression* matchVal;
      Statement* s;
    };
    vector<SwitchCase*> cases;
    //optional default: statement, NULL if unused
    Statement* defaultStatement;
  };

  struct For : public Nonterm
  {
    enum ForType
    {
      C_STYLE,
      RANGE_1,
      RANGE_2,
      ARRAY,
    };
    struct ForC
    {
      VarDecl* decl;
      Expression* condition;
      VarAssign* incr;
    };
    struct ForRange1
    {
      Expression* expr;
    };
    struct ForRange2
    {
      Expression* start;
      Expression* end;
    };
    struct ForArray
    {
      Expression* container;
    };
    int type;
    union
    {
      ForC* forC;
      ForRange1* forRange1;
      ForRange2* forRange2;
      ForArray* forArray;
    } loop;
    Statement* body;
  };

  struct While : public Nonterm
  {
    Expression* cond;
    Statement* body;
  };

  struct If : public Nonterm
  {
    Expression* cond;
    Statement* ifBody;
    //elseBody NULL if there is no else clause
    Statement* elseBody;
  };

  struct Using : public Nonterm
  {
    Member* mem;
  };

  struct Assertion : public Nonterm
  {
    Expression* expr;
  };

  struct TestDecl : public Nonterm
  {
    Call* call;
  };

  struct Enum : public Nonterm
  {
    struct EnumItem
    {
      Ident* name;
      //value is optional (assigned automatically if not explicit)
      IntLit* value;
    };
    Ident* name;
    vector<EnumItem> items;
  };

  struct Block : public Nonterm
  {
    vector<Statement*> statements;
  };

  struct VarDecl : public Nonterm
  {
    //NULL if "auto"
    Type* type;
    //NULL if uninitialized
    Expression* val;
  };

  struct VarAssign : public Nonterm
  {
    Member* target;
    Oper* op;
    //optional
    Expression* rhs;
  };

  struct Print : public Nonterm
  {
    Type* retType;
    Member* name;
    vector<Arg> args;
    vector<Expression*> toPrint;
  };

  struct Expression : public Nonterm
  {
    NodeType type;
    union
    {
      IntLit* il;
      FloatLit* fl;
      StrLit* sl;
      CharLit* cl;
      BoolLit* bl;
      ArrayLit* al;
      StructLit* struc;
      Call* call;
      Member* mem;
      Expr1* ex1;
      //Error also allowed
    } e;
  };

  struct Call : public Nonterm
  {
    Member* name;
    vector<Expression*> args;
  };

  struct Arg : public Nonterm
  {
    bool traitType;
    union
    {
      Type* t;
      TraitType* tt;
    } t;
    //optional
    Ident* name;
  };

  struct Args : public Nonterm
  {
    vector<Arg*> args;
  };

  struct FuncDecl : public Nonterm
  {
    Type* retType;
    Ident* name;
    vector<Arg> args;
  };

  struct FuncDef : public Nonterm
  {
    Type* retType;
    Member* name;
    vector<Arg> args;
    Block* body;
  };

  struct FuncType : public Nonterm
  {
    Type* retType;
    Member* name;
    vector<Arg> args;
  };

  struct ProcDecl : public Nonterm
  {
    Type* retType;
    Ident* name;
    vector<Arg> args;
  };

  struct ProcDef : public Nonterm
  {
    Type* retType;
    Member* name;
    vector<Arg> args;
    Block* body;
  };

  struct ProcType : public Nonterm
  {
    Type* retType;
    Member* name;
    vector<Arg> args;
  };

  struct StructDecl : public Nonterm
  {
    struct StructMem
    {
      ScopedDecl* sd;
      //composition only used if sd is a VarDecl
      bool compose;
    };
    vector<StructMem> traits;
  };

  struct VariantDecl : public Nonterm
  {
    Ident* name;
    vector<Type*> types;
  };

  struct TraitDecl : public Nonterm
  {
    struct TraitMember
    {
      union
      {
        FuncDecl* fd;
        ProcDecl* pd;
      } f;
      bool proc;
    };
    Ident* name;
    vector<TraitMember> members;
  };

  struct ArrayLit : public Nonterm
  {
    vector<Expression*> vals;
  };

  struct StructLit : public Nonterm
  {
    vector<Expression*> members;
  };

  struct Member : public Nonterm
  {
    //owner is optional
    Member* owner;
    Ident* mem;
  };

  struct TraitType : public Nonterm
  {
    //trait types of the form "<localName> : <traitName>"
    Ident* localName;
    Ident* traitName;
  };

  struct TupleType : public Nonterm
  {
    //cannot be empty
    vector<Type*> members;
  };

  struct BoolLit : public Nonterm
  {
    bool val;
  };

  struct Expr1 : public Nonterm
  {
    Expr2* head;
    vector<Expr1RHS*> tail;
  };

  struct Expr1RHS: public Nonterm
  {
    // || is only op
    Expr2* rhs;
  };

  struct Expr2 : public Nonterm
  {
    Expr3* head;
    vector<Expr2RHS*> tail;
  };

  struct Expr2RHS : public Nonterm
  {
    Expr3* rhs;
  };

  struct Expr3 : public Nonterm
  {
    Expr4* head;
    vector<Expr3HRS*> tail;
  };

  struct Expr3RHS : public Nonterm
  {
    Expr4* rhs;
  };

  struct Expr4 : public Nonterm
  {
    Expr5* head;
    vector<Expr4RHS*> tail;
  };

  struct Expr4RHS : public Nonterm
  {
    Expr5* rhs;
  };

  struct Expr5 : public Nonterm
  {
    Expr6* head; 
    vector<Expr5RHS*> tail;
  };

  struct Expr5RHS : public Nonterm
  {
    Expr6* rhs;
  };

  struct Expr6 : public Nonterm
  {
    Expr7* head;
    vector<Expr6RHS*> tail;
  };

  struct Expr6RHS : public Nonterm
  {
    int op; //CMPEQ or CMPNEQ
    Expr7* rhs;
  };

  struct Expr7 : public Nonterm
  {
    Expr8* head;
    vector<Expr7RHS*> tail;
  };

  struct Expr7RHS : public Nonterm
  {
    int op;  //CMPL, CMPLE, CMPG, CMPGE
    Expr8* rhs;
  };

  struct Expr8 : public Nonterm
  {
    Expr9* head;
    vector<Expr8RHS*> tail;
  };

  struct Expr8RHS : public Nonterm
  {
    int op; //SHL, SHR
    Expr9* rhs;
  };

  struct Expr9 : public Nonterm
  {
    Expr10* head;
    vector<Expr9RHS*> tail;
  };

  struct Expr9RHS : public Nonterm
  {
    int op; //PLUS, SUB
    Expr10* rhs;
  };

  struct Expr10 : public Nonterm
  {
    Expr11* head;
    vector<Expr10RHS*> tail;
  };

  struct Expr10RHS : public Nonterm
  {
    int op; //MUL, DIV, MOD
    Expr11* rhs;
  };

  struct Expr11 : public Nonterm
  {
    Expr12* head;
    vector<Expr11RHS*> tail;
  };

  struct Expr11RHS : public Nonterm
  {
    int op; //SUB, LNOT, BNOT
    Expr12* rhs;
  };

  struct Expr12 : public Nonterm
  {
    enum
    {
      PAREN_EX,
      INT,
      BOOL,
      CHAR,
      STR,
      FLOAT,
      ARRAY,
      STRUCT
    }
    union
    {
      Expression* paren;
      IntLit* i;
      BoolLit* b;
      CharLit* c;
      StrLit* s;
      FloatLit* f;
      ArrayLit* a;
      StructLit* struc;
    } e;
    int type;
  }; 

  //Parse from a linear token stream into a program (default module)
  ModuleDef* parseProgram(vector<Token*>& toks);

  //Parse a nonterminal of type NT
  template<typename NT>
  NT* parse();

  template<typename NT>
  NT* parseOptional()
  {
    int prevPos = pos;
    try
    {
      NT* nt = parse<NT>();
      return nt;
    }
    catch(...)
    {
      //backtrack
      pos = prevPos;
      return NULL;
    }
  }

  template<typename NT>
  vector<NT*> parseSome()
  {
    vector<NT*> nts;
    while(true)
    {
      NT* nt = parseOptional<NT>();
      if(!nt)
        break;
      else
        nts.push_back(nt);
    };
    return nts;
  }

  //Token stream utilities
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
  extern int pos;
  extern vector<Token*>* tokens;
}

#endif

//parse plan
//parse<type> parses nonterm type (failure is an error)
//parseOptional<type> parses one nonterm, but failure just returns NULL
//parseSome<type> parses 0 or more nonterms (repeatedly parseOptional)
//use exceptions to propagate parse failures up the chain
//parseOptional just catches the error, and backtracks token iterator
//the top-level parse function can print the error
// error messages: i.e. "Parse error: expected <nonterm type/keyword/etc> but got "<token text>"
 
