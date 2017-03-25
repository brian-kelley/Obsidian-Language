#ifndef PARSER_H
#define PARSER_H

#include "Misc.hpp"
#include "Token.hpp"
#include "Type.hpp"

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
    EXPR_2,
    EXPR_3,
    EXPR_4,
    EXPR_5,
    EXPR_6,
    EXPR_7,
    EXPR_8,
    EXPR_9,
    EXPR_10,
    EXPR_11,
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
  struct BoolLit;
  struct Expr1;
  struct Expr2;
  struct Expr3;
  struct Expr4;
  struct Expr5;
  struct Expr6;
  struct Expr7;
  struct Expr8;
  struct Expr9;
  struct Expr10;
  struct Expr11;
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
    struct TupleType
    {
      vector<Type*> types;
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
    int type;
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

  struct BoolLit : public Nonterm
  {
    bool val;
  };

  struct Expr1 : public Nonterm
  {
    struct OrExpr
    {
      Expr1* lhs;
      Expr2* rhs;
    };
    enum
    {
      EX2,
      OR
    };
    union
    {
      Expr2* ex2;
      OrExpr orEx;
    } e;
    int op;
  };

  struct Expr2 : public Nonterm
  {
    struct AndExpr
    {
      Expr2* lhs;
      Expr3* rhs;
    };
    enum
    {
      EX3,
      AND
    };
    union
    {
      Expr3* ex3;
      AndExpr andEx;
    } e;
    int op;
  };

  struct Expr3 : public Nonterm
  {
    struct BitOrExpr
    {
      Expr3* lhs;
      Expr4* rhs;
    };
    enum
    {
      EX4,
      OR
    };
    union
    {
      Expr4* ex4;
      BitOrExpr bitOrEx;
    } e;
    int op;
  };

  struct Expr4 : public Nonterm
  {
    struct XorExpr
    {
      Expr4* lhs;
      Expr5* rhs;
    };
    enum
    {
      EX5,
      XOR
    };
    union
    {
      Expr5* ex5;
      XorExpr xorEx;
    } e;
    int op;
  };

  struct Expr5 : public Nonterm
  {
    struct BitAndExpr
    {
      Expr5* lhs;
      Expr6* rhs;
    };
    enum
    {
      EX6,
      AND
    };
    union
    {
      Expr6* ex6;
      BitAndExpr bitAndEx;
    } e;
    int op;
  };

  struct Expr6 : public Nonterm
  {
    struct EqualsExpr
    {
      Expr6* lhs;
      Expr7* rhs;
    };
    struct NotEqualsExpr
    {
      Expr6* lhs;
      Expr7* rhs;
    };
    enum
    {
      EX7,
      EQUAL,
      NOT_EQUAL
    };
    union
    {
      Expr7* ex7;
      EqualsExpr eqEx;
      NotEqualsExpr neqEx;
    } e;
    int op;
  };

  struct Expr7 : public Nonterm
  {
    struct LessExpr
    {
      Expr7* lhs;
      Expr8* rhs;
    };
    struct GreaterExpr
    {
      Expr7* lhs;
      Expr8* rhs;
    };
    struct LessEqExpr
    {
      Expr7* lhs;
      Expr8* rhs;
    };
    struct GreaterEqExpr
    {
      Expr7* lhs;
      Expr8* rhs;
    };
    enum
    {
      EX8,
      LESS,
      LESS_EQ,
      GREATER,
      GREATER_EQ
    };
    union
    {
      Expr8* ex8;
      LessExpr lessEx;
      GreaterExpr greaterEx;
      LessEqExpr lessEqEx;
      GreaterEqExpr greaterEqEx;
    } e;
    int op;
  };

  struct Expr8 : public Nonterm
  {
    struct ShlExpr
    {
      Expr8* lhs;
      Expr9* rhs;
    };
    struct ShrExpr
    {
      Expr8* lhs;
      Expr9* rhs;
    };
    enum
    {
      EX9,
      SHL,
      SHR
    };
    union
    {
      Expr9* ex9;
      ShlExpr shlEx;
      ShrExpr shrEx;
    } e;
    int op;
  };

  struct Expr9 : public Nonterm
  {
    struct AddExpr
    {
      Expr9* lhs;
      Expr10* rhs;
    };
    struct SubExpr
    {
      Expr9* lhs;
      Expr10* rhs;
    };
    enum
    {
      EX10,
      ADD,
      SUB
    };
    union
    {
      Expr10* ex10;
      AddExpr addEx;
      SubExpr subEx;
    } e;
    int op;
  };

  struct Expr10 : public Nonterm
  {
    struct MulExpr
    {
      Expr10* lhs;
      Expr11* rhs;
    };
    struct DivExpr
    {
      Expr10* lhs;
      Expr11* rhs;
    };
    struct ModExpr
    {
      Expr10* lhs;
      Expr11* rhs;
    };
    enum
    {
      EX11,
      MUL,
      DIV,
      MOD
    };
    union
    {
      Expr11* ex11;
      MulExpr mulEx;
      DivExpr divEx;
      ModExpr modEx;
    } e;
    int op;
  };

  struct Expr11 : public Nonterm
  {
    struct NotExpr    //!
    {
      Expr11* val;
    };
    struct NegExpr    //-
    {
      Expr11* val;
    };
    struct BitNotExpr //~
    {
      Expr11* val;
    }
    enum
    {
      EXPR12,
      NOT,
      NEG,
      BITNOT
    };
    union
    {
      Expr12* ex12;
      NotExpr notEx;
      NegExpr negEx;
      BitNotExpr bitNotEx;
    } e;
    int op;
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

  //Parse a nonterminal of type NT. 
  //If allowFail, will silently return NULL if the parse fails
  //If !allowFail, will print error message and stop the compiler if parse fails
  template<typename NT>
  NT* parse(bool allowFail = true);

  //Parsing utilities
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

