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
  ScopedDecl* parse<ScopedDecl>(bool canFail)
  {
    /*
       Module
       StructDecl
       VariantDecl
       TraitDecl
       Enum
       Typedef
       TestDecl
       FuncDecl
       FuncDef
       ProcDecl
       ProcDef
       VarDecl
       */
    ScopedDecl sd;
    sd.
    else if(sd.decl.
    {
      sd.
    }
    else if(acceptKeyword(VARIANT))
    {
      unget();
      sd->type = NodeType::VARIANT_DECL;
      sd->decl.vd = parseVariantDecl();
    }
    else if(acceptKeyword(TRAIT))
    {
      unget();
      sd->type = NodeType::TRAIT_DECL;
      sd->decl.td = parseTraitDecl();
    }
    else if(acceptKeyword(ENUM))
    {
      unget();
      sd->type = NodeType::ENUM;
      sd->decl.e = parseEnum();
    }
    else if(acceptKeyword(TYPEDEF))
    {
      unget();
      sd->type = NodeType::TYPEDEF;
      sd->decl.t = parseTypedef();
    }
    else if(acceptKeyword(TEST))
    {
      unget();
      sd->type = NodeType::TEST_DECL;
      sd->decl.test = parseTestDecl();
    }
    else if(acceptKeyword(FUNC))
    {
      //either func decl or def
      auto fdef = parseFuncDef();
      if(fdef)
      {
        sd->type = NodeType::FUNC_DEF;
        sd->decl.fdef = fdef;
      }
      else
      {
        sd->type = NodeType::FUNC_DECL;
        sd->decl.fdcl = parseFuncDecl();
      }
    }
    else if(acceptKeyword(PROC))
    {
    }
  }

  template<>
  Type* parse<Type>(bool canFail)
  {
  }

  template<>
  Statement* parse<Statement>(bool canFail)
  {
  }

  template<>
  Typedef* parse<Typedef>(bool canFail)
  {
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
    else
      err(string("expected ") + tokTypeTable[tokType] + " but got " + next->getStr());
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

