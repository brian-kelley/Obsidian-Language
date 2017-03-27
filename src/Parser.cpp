#include "Parser.hpp"

namespace Parser
{
  int pos;
  vector<Token*>* tokens;

  ModuleDef* parseProgram(vector<Token*>& toks)
  {
    pos = 0;
    tokens = &toks;
    return parse<ModuleDef>();
  }

  template<>
  Module* parse<Module>(bool canFail)
  {
    Module m;
    expectKeyword(MODULE);
    m.name = (Ident*) accept(IDENTIFIER, canFail);
    expectPunct(LBRACE);
    m.def = parse<ModuleDef>();
    expectPunct(RBRACE);
    return new Module(m);
  }

  template<>
  ModuleDef* parse<ModuleDef>(bool canFail)
  {
    ModuleDef md;
    md.decls = parseMany<ScopedDecl>();
    return new ModuleDef(md);
  }

  template<>
  ScopedDecl* parse<ScopedDecl>()
  {
    ScopedDecl* sd = new ScopedDecl;
    if(sd->nt = parseOptional<Module>())
      return sd;
    if(sd->nt = parseOptional<VarDecl>())
      return sd;
    if(sd->nt = parseOptional<VariantDecl>())
      return sd;
    if(sd->nt = parseOptional<TraitDecl>())
      return sd;
    if(sd->nt = parseOptional<Enum>())
      return sd;
    if(sd->nt = parseOptional<Typedef>())
      return sd;
    if(sd->nt = parseOptional<TestDecl>())
      return sd;
    if(sd->nt = parseOptional<FuncDecl>())
      return sd;
    if(sd->nt = parseOptional<FuncDef>())
      return sd;
    if(sd->nt = parseOptional<ProcDecl>())
      return sd;
    if(sd->nt = parseOptional<ProcDef>())
      return sd;
    delete sd;
    throw ParseErr("invalid scoped declaration");
  }

  template<>
  Type* parse<Type>()
  {
    Type* t = new Type;
    TupleType* tup = NULL;
    Member* mem = NULL;
    if(acceptKeyword(VOID))
    {
      t->type = Type::PRIMITIVE;
      t->t.prim = Type::VOID;
    }
    else if(acceptKeyword(BOOL))
    {
      t->type = Type::PRIMITIVE;
      t->t.prim = Type::BOOL;
    }
    else if(acceptKeyword(CHAR))
    {
      t->type = Type::PRIMITIVE;
      t->t.prim = Type::CHAR;
    }
    else if(acceptKeyword(UCHAR))
    {
      t->type = Type::PRIMITIVE;
      t->t.prim = Type::UCHAR;
    }
    else if(acceptKeyword(SHORT))
    {
      t->type = Type::PRIMITIVE;
      t->t.prim = Type::SHORT;
    }
    else if(acceptKeyword(USHORT))
    {
      t->type = Type::PRIMITIVE;
      t->t.prim = Type::USHORT;
    }
    else if(acceptKeyword(INT))
    {
      t->type = Type::PRIMITIVE;
      t->t.prim = Type::INT;
    }
    else if(acceptKeyword(UINT))
    {
      t->type = Type::PRIMITIVE;
      t->t.prim = Type::UINT;
    }
    else if(acceptKeyword(LONG))
    {
      t->type = Type::PRIMITIVE;
      t->t.prim = Type::LONG;
    }
    else if(acceptKeyword(ULONG))
    {
      t->type = Type::PRIMITIVE;
      t->t.prim = Type::ULONG;
    }
    else if(acceptKeyword(FLOAT))
    {
      t->type = Type::PRIMITIVE;
      t->t.prim = Type::FLOAT;
    }
    else if(acceptKeyword(DOUBLE))
    {
      t->type = Type::PRIMITIVE;
      t->t.prim = Type::DOUBLE;
    }
    else if(acceptKeyword(STRING))
    {
      t->type = Type::PRIMITIVE;
      t->t.prim = Type::STRING;
    }
    else if(tup = parseOptional<TupleType>())
    {
      t->type = Type::TUPLE;
      t->t.tup = *tup;
      delete tup;
    }
    else if(mem = parse<Member>())
    {
      t->type = Type::MEMBER;
      t->t.mem = mem;
    }
    else
    {
      throw ParseErr("invalid type");
    }
    //now that base type has been parsed, there may be array dimension brackets after
    int dims = 0;
    while(1)
    {
      if(!acceptPunct(LBRACKET))
      {
        break;
      }
      expectPunct(RBRACKET);
      dims++;
    }
    if(dims > 0)
    {
      //this type is an array, and its underlying type is the previously parsed type
      ArrayType at;
      at.t = t;
      at.dims = dims;
      Type* arrayType = new Type;
      arrayType->type = Type::ARRAY;
      arrayType->t.arr = at;
      return arrayType;
    }
    else
    {
      //keep the original type
      return t;
    }
  }

  template<>
  Statement* parse<Statement>()
  {
    /*
       ScopedDecl
       VarAssign
       Print
       Expression p";"
       Block
       Return
       Continue
       Break
       Switch
       For
       While
       If
       Using
       Assertion
       EmptyStatement
       */
    Statement* s = new Statement;
    if(s->nt = parseOptional<ScopedDecl>())
      return s;
    if(s->nt = parseOptional<VarAssign>())
      return s;
    if(s->nt = parseOptional<Print>())
      return s;
    if(s->nt = parseOptional<Expression>())
    {
      expectPunct(SEMICOLON);
      return s;
    }
    if(s->nt = parseOptional<Block>())
      return s;
    if(s->nt = parseOptional<Return>())
      return s;
    if(s->nt = parseOptional<Continue>())
      return s;
    if(s->nt = parseOptional<Break>())
      return s;
    if(s->nt = parseOptional<Switch>())
      return s;
    if(s->nt = parseOptional<For>())
      return s;
    if(s->nt = parseOptional<While>())
      return s;
    if(s->nt = parseOptional<If>())
      return s;
    if(s->nt = parseOptional<Using>())
      return s;
    if(s->nt = parseOptional<Assertion>())
      return s;
    if(s->nt = parseOptional<EmptyStatement>())
      return s;
    throw parseErr("invalid statement");
  }

  template<>
  Typedef* parse<Typedef>()
  {
    expectKeyword(TYPEDEF);
  }

  template<>
  Return* parse<Return>(bool canFail)
  {
  }

  template<>
  Switch* parse<Switch>(bool canFail)
  {
  }

  template<>
  For* parse<For>(bool canFail)
  {
  }

  template<>
  While* parse<While>(bool canFail)
  {
  }

  template<>
  If* parse<If>(bool canFail)
  {
  }

  template<>
  Using* parse<Using>(bool canFail)
  {
  }

  template<>
  Assertion* parse<Assertion>(bool canFail)
  {
  }

  template<>
  TestDecl* parse<TestDecl>(bool canFail)
  {
  }

  template<>
  Enum* parse<Enum>(bool canFail)
  {
  }

  template<>
  Block* parse<Block>(bool canFail)
  {
  }

  template<>
  VarDecl* parse<VarDecl>(bool canFail)
  {
  }

  template<>
  VarAssign* parse<VarAssign>(bool canFail)
  {
  }

  template<>
  Print* parse<Print>(bool canFail)
  {
  }

  template<>
  Expression* parse<Expr>(bool canFail)
  {
  }

  template<>
  Call* parse<Call>(bool canFail)
  {
  }

  template<>
  FuncDecl* parse<FuncDecl>(bool canFail)
  {
  }

  template<>
  FuncDef* parse<FuncDef>(bool canFail)
  {
  }

  template<>
  FuncType* parse<FuncType>(bool canFail)
  {
  }

  template<>
  ProcDecl* parse<ProcDecl>(bool canFail)
  {
  }
  
  template<>
  ProcDef* parse<ProcDef>(bool canFail)
  {
  }

  template<>
  ProcType* parse<ProcType>(bool canFail)
  {
  }

  template<>
  StructDecl* parse<StructDecl>(bool canFail)
  {
  }

  template<>
  VariantDecl* parse<VariantDecl>(bool canFail)
  {
  }

  template<>
  TraitDecl* parse<TraitDecl>(bool canFail)
  {
  }

  template<>
  ArrayLit* parse<ArrayLit>(bool canFail)
  {
  }

  template<>
  StructLit* parse<StructLit>(bool canFail)
  {
  }

  template<>
  Member* parse<Member>(bool canFail)
  {
  }

  template<>
  TraitType* parse<TraitType>(bool canFail)
  {
  }

  template<>
  BoolLit* parse<BoolLit>(bool canFail)
  {
  }

  template<>
  Expr1* parse<Expr1>(bool canFail)
  {
  }

  template<>
  Expr2* parse<Expr2>(bool canFail)
  {
  }

  template<>
  Expr3* parse<Expr3>(bool canFail)
  {
  }

  template<>
  Expr4* parse<Expr4>(bool canFail)
  {
  }

  template<>
  Expr5* parse<Expr5>(bool canFail)
  {
  }

  template<>
  Expr6* parse<Expr6>(bool canFail)
  {
  }

  template<>
  Expr7* parse<Expr7>(bool canFail)
  {
  }

  template<>
  Expr8* parse<Expr8>(bool canFail)
  {
  }

  template<>
  Expr9* parse<Expr9>(bool canFail)
  {
  }

  template<>
  Expr10* parse<Expr10>(bool canFail)
  {
  }

  template<>
  Expr11* parse<Expr11>(bool canFail)
  {
  }

  template<>
  Expr12* parse<Expr12>(bool canFail)
  {
  }

  bool accept(Token& t)
  {
    bool res = *getNext() == t;
    if(res)
      pos++;
    return res;
  }

  Token* accept(int tokType)
  {
    Token* next = getNext();
    bool res = next->getType() == tokType;
    if(res)
    {
      pos++;
      return next;
    }
    else
      return NULL;
  }

  bool acceptKeyword(int type)
  {
    Keyword kw(type);
    return accept(kw);
  }

  bool acceptOper(int type)
  {
    Oper op(type);
    return accept(op);
  }

  bool acceptPunct(int type)
  {
    Punct p(type);
    return accept(p);
  }

  void expect(Token& t)
  {
    bool res = *getNext() == t;
    if(res)
    {
      pos++;
      return;
    }
    throw ParseErr(string("expected ") + t.getStr() + " but got " + next->getStr());
  }

  Token* expect(int tokType)
  {
    Token* next = getNext();
    bool res = next->getType() == tokType;
    if(res)
      pos++;
    else
      throw parseErr(string("expected ") + tokTypeTable[tokType] + " but got " + next->getStr());
    return next;
  }

  void expectKeyword(int type)
  {
    Keyword kw(type);
    expect(kw);
  }

  void expectOper(int type)
  {
    Oper op(type);
    expect(op);
  }

  void expectPunct(int type)
  {
    Punct p(type);
    expect(p);
  }

  Token* accept(int tokType)
  {
    Token* next = getNext();
    bool res = next->getType() == tokType;
    if(res)
      pos++;
    return res ? next : NULL;
  }

  bool acceptKeyword(int type, bool canFail)
  {
    Keyword k(type);
    return accept(k, canFail);
  }

  bool acceptOper(int type, bool canFail)
  {
    Oper o(type);
    return accept(o, canFail1);
  }

  bool acceptPunct(int type, bool canFail)
  {
    Punct p(type);
    return accept(p, canFail);
  }

  Token* getNext()
  {
    if(pos < tokens->size())
      return &PastEOF::inst;
    else
      return *tokens[pos];
  }

  Token* lookAhead(int ahead)
  {
    if(pos + ahead < tokens->size())
      return &PastEOF::inst;
    else
      return *tokens[pos + ahead];
  }

  void unget()
  {
    pos--;
  }

  void err(string msg)
  {
    if(msg.length())
      printf("Parse error on line %i, col %i: %s\n", 0, 0, msg.c_str());  //TODO!
    else
      printf("Parse error on line %i, col %i\n", 0, 0);  //TODO!
    exit(1);
  }
}

