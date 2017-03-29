#ifndef PARSER_H
#define PARSER_H

#include "Misc.hpp"
#include "Token.hpp"
#include "Type.hpp"

#include <stdexcept>
#include <memory>
#include <variant>

using namespace std;

#define UP unique_ptr
typedef monostate None;
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
    string name;
    UP<ModuleDef> def;
  };

  struct ModuleDef : public Nonterm
  {
    vector<UP<ScopedDecl>> decls;
  };

  struct ScopedDecl : public Nonterm
  {
    variant<
      None,
      UP<Module>,
      UP<VarDecl>,
      UP<StructDecl>,
      UP<VariantDecl>,
      UP<TraitDecl>,
      UP<Enum>,
      UP<Typedef>,
      UP<FuncDecl>,
      UP<FuncDef>,
      UP<ProcDecl>,
      UP<ProcDef>,
      UP<TestDecl>> decl;
  };

  struct Type : public Nonterm
  {
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
      UP<Member>,
      UP<ArrayType>,
      UP<TupleType>> t;
  };

  struct Statement : public Nonterm
  {
    variant<
      None,
      UP<ScopedDecl>,
      UP<VarAssign>,
      UP<Print>,
      UP<Expression>,
      UP<Block>,
      UP<Return>,
      UP<Continue>,
      UP<Break>,
      UP<Switch>,
      UP<For>,
      UP<While>,
      UP<If>,
      UP<Using>,
      UP<Assertion>,
      UP<EmptyStatement>> s;
  };

  struct Typedef : public Nonterm
  {
    UP<Type> type;
    string ident;
  };

  struct Return : public Nonterm
  {
    //optional returned expression (NULL if unused)
    UP<Expression> ex;
  };

  struct Switch : public Nonterm
  {
    UP<Expression> sw;
    struct SwitchCase
    {
      UP<Expression> matchVal;
      UP<Statement> s;
    };
    vector<SwitchCase> cases;
    //optional default: statement, NULL if unused
    UP<Statement> defaultStatement;
  };

  struct For : public Nonterm
  {
    variant<
      None,
      UP<ForC>,
      UP<ForRange1>,
      UP<ForRange2>,
      UP<ForArray>> f;
    UP<Statement> body;
  };

  struct ForC : public Nonterm
  {
    UP<VarDecl> decl;
    UP<Expression> condition;
    UP<VarAssign> incr;
  };

  struct ForRange1 : public Nonterm
  {
    UP<Expression> expr;
  };

  struct ForRange2 : public Nonterm
  {
    UP<Expression> start;
    UP<Expression> end;
  };

  struct ForArray : public Nonterm
  {
    UP<Expression> container;
  };

  struct While : public Nonterm
  {
    UP<Expression> cond;
    UP<Statement> body;
  };

  struct If : public Nonterm
  {
    UP<Expression> cond;
    UP<Statement> ifBody;
    //elseBody NULL if there is no else clause
    UP<Statement> elseBody;
  };

  struct Using : public Nonterm
  {
    UP<Member> mem;
  };

  struct Assertion : public Nonterm
  {
    UP<Expression> expr;
  };

  struct TestDecl : public Nonterm
  {
    UP<Call> call;
  };

  struct Enum : public Nonterm
  {
    struct EnumItem
    {
      string name;
      //value is optional (assigned automatically if not explicit)
      UP<IntLit> value;
    };
    string name;
    vector<EnumItem> items;
  };

  struct Block : public Nonterm
  {
    vector<UP<Statement>> statements;
  };

  struct VarDecl : public Nonterm
  {
    //NULL if "auto"
    UP<Type> type;
    //NULL if uninitialized
    UP<Expression> val;
  };

  struct VarAssign : public Nonterm
  {
    UP<Member> target;
    UP<Oper> op;
    //optional
    UP<Expression> rhs;
  };

  struct Print : public Nonterm
  {
    UP<Type> retType;
    UP<Member> name;
    vector<UP<Arg>> args;
    vector<UP<Expression>> toPrint;
  };

  struct Expression : public Nonterm
  {
    variant<
      None,
      UP<Call>,
      UP<Member>,
      UP<Expr1>> e;
  };

  struct Call : public Nonterm
  {
    UP<Member> name;
    vector<UP<Expression>> args;
  };

  struct Arg : public Nonterm
  {
    bool traitType;
    variant<
      None,
      UP<Type>,
      UP<TraitType>> t;
    string name;
  };

  struct Args : public Nonterm
  {
    vector<UP<Arg>> args;
  };

  struct FuncDecl : public Nonterm
  {
    UP<Type> retType;
    string name;
    vector<UP<Arg>> args;
  };

  struct FuncDef : public Nonterm
  {
    UP<Type> retType;
    UP<Member> name;
    vector<UP<Arg>> args;
    UP<Block> body;
  };

  struct FuncType : public Nonterm
  {
    UP<Type> retType;
    UP<Member> name;
    vector<UP<Arg>> args;
  };

  struct ProcDecl : public Nonterm
  {
    UP<Type> retType;
    string name;
    vector<UP<Arg>> args;
  };

  struct ProcDef : public Nonterm
  {
    UP<Type> retType;
    UP<Member> name;
    vector<UP<Arg>> args;
    UP<Block> body;
  };

  struct ProcType : public Nonterm
  {
    UP<Type> retType;
    UP<Member> name;
    vector<UP<Arg>> args;
  };

  struct StructDecl : public Nonterm
  {
    struct StructMem
    {
      UP<ScopedDecl> sd;
      //composition only used if sd is a VarDecl
      bool compose;
    };
    vector<UP<StructMem>> traits;
  };

  struct VariantDecl : public Nonterm
  {
    string name;
    vector<UP<Type>> types;
  };

  struct TraitDecl : public Nonterm
  {
    struct TraitMember
    {
      variant<
        None,
        UP<FuncDecl>,
        UP<ProcDecl>> tm;
      bool proc;
    };
    string name;
    vector<UP<TraitMember>> members;
  };

  struct ArrayLit : public Nonterm
  {
    vector<UP<Expression>> vals;
  };

  struct StructLit : public Nonterm
  {
    vector<UP<Expression>> members;
  };

  struct Member : public Nonterm
  {
    string owner;
    //mem is optional
    UP<Member> mem;
  };

  struct TraitType : public Nonterm
  {
    //trait types of the form "<localName> : <traitName>"
    string localName;
    UP<Member> traitName;
  };

  struct ArrayType
  {
    UP<Type> t;
    int dims;
  };

  struct TupleType : public Nonterm
  {
    //cannot be empty
    vector<UP<Type>> members;
  };

  struct BoolLit : public Nonterm
  {
    bool val;
  };

  struct Expr1 : public Nonterm
  {
    UP<Expr2> head;
    vector<UP<Expr1RHS>> tail;
  };

  struct Expr1RHS: public Nonterm
  {
    // || is only op
    UP<Expr2> rhs;
  };

  struct Expr2 : public Nonterm
  {
    UP<Expr3> head;
    vector<UP<Expr2RHS>> tail;
  };

  struct Expr2RHS : public Nonterm
  {
    UP<Expr3> rhs;
  };

  struct Expr3 : public Nonterm
  {
    UP<Expr4> head;
    vector<UP<Expr3HRS>> tail;
  };

  struct Expr3RHS : public Nonterm
  {
    UP<Expr4> rhs;
  };

  struct Expr4 : public Nonterm
  {
    UP<Expr5> head;
    vector<UP<Expr4RHS>> tail;
  };

  struct Expr4RHS : public Nonterm
  {
    UP<Expr5> rhs;
  };

  struct Expr5 : public Nonterm
  {
    UP<Expr6> head; 
    vector<UP<Expr5RHS>> tail;
  };

  struct Expr5RHS : public Nonterm
  {
    UP<Expr6> rhs;
  };

  struct Expr6 : public Nonterm
  {
    UP<Expr7> head;
    vector<UP<Expr6RHS>> tail;
  };

  struct Expr6RHS : public Nonterm
  {
    int op; //CMPEQ or CMPNEQ
    UP<Expr7> rhs;
  };

  struct Expr7 : public Nonterm
  {
    UP<Expr8> head;
    vector<UP<Expr7RHS>> tail;
  };

  struct Expr7RHS : public Nonterm
  {
    int op;  //CMPL, CMPLE, CMPG, CMPGE
    UP<Expr8> rhs;
  };

  struct Expr8 : public Nonterm
  {
    UP<Expr9> head;
    vector<UP<Expr8RHS>> tail;
  };

  struct Expr8RHS : public Nonterm
  {
    int op; //SHL, SHR
    UP<Expr9> rhs;
  };

  struct Expr9 : public Nonterm
  {
    UP<Expr10> head;
    vector<UP<Expr9RHS>> tail;
  };

  struct Expr9RHS : public Nonterm
  {
    int op; //PLUS, SUB
    UP<Expr10> rhs;
  };

  struct Expr10 : public Nonterm
  {
    UP<Expr11> head;
    vector<UP<Expr10RHS>> tail;
  };

  struct Expr10RHS : public Nonterm
  {
    int op; //MUL, DIV, MOD
    UP<Expr11> rhs;
  };

  struct Expr11 : public Nonterm
  {
    UP<Expr12> head;
    vector<UP<Expr11RHS>> tail;
  };

  struct Expr11RHS : public Nonterm
  {
    int op; //SUB, LNOT, BNOT
    UP<Expr12> rhs;
  };

  struct Expr12 : public Nonterm
  {
    variant<
      IntLit*,
      BoolLit*,
      CharLit*,
      StrLit*,
      FloatLit*,
      UP<Expression>,
      UP<Member>,
      UP<ArrayLit>,
      UP<StructLit>> e;
  }; 

  //Parse from a linear token stream into a program (default module)
  UP<ModuleDef> parseProgram(vector<Token*>& toks);

  //Parse a nonterminal of type NT
  template<typename NT>
  UP<NT> parse();

  template<typename NT>
  UP<NT> parseOptional()
  {
    int prevPos = pos;
    try
    {
      UP<NT> nt = parse<NT>();
      return nt;
    }
    catch(...)
    {
      //backtrack
      pos = prevPos;
      return UP<NT>;
    }
  }

  template<typename NT>
  vector<UP<NT>> parseSome()
  {
    vector<UP<NT>> nts;
    while(true)
    {
      UP<NT> nt = parseOptional<NT>();
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
 
