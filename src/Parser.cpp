#include "Parser.hpp"

namespace Parser
{
  int pos;
  vector<Token*>* tokens;

  ModuleDef* parseProgram(vector<Token*>& toks)
  {
    pos = 0;
    tokens = &toks;
    return parseModuleDef(false);
  }

  template<>
  Module* parse<Module>(bool canFail)
  {
    Module m;
    if(canFail)
    {
      expectKeyword(
    }
    expectKeyword(MODULE);
    m.name = (Ident*) expect(IDENTIFIER);
    expectPunct(LBRACE);
    m.def = parseModuleDef();
    expectPunct(RBRACE);
  }

  template<>
  ModuleDef* parse<ModuleDef>(bool canFail)
  {
    ModuleDef* md = new ModuleDef;
    while(true)
    {
      ScopedDecl* sd = parse<ScopedDecl>();
      if(sd)
        md->decls.push_back(sd);
      else
        break;
    }
    return md;
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
    ScopedDecl* sd = new ScopedDecl;
    if(acceptKeyword(MODULE))
    {
      unget();
      sd->type = NodeType::MODULE;
      sd->decl.m = parseModule();
    }
    else if(acceptKeyword(STRUCT))
    {
      unget();
      sd->type = NodeType::STRUCT_DECL;
      sd->decl.structDecl = parseStructDecl();
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
      pos++;
    return res ? next : NULL;
  }
  bool acceptKeyword(int type)
  {
    Keyword k(type);
    return accept(k);
  }
  bool acceptOper(int type)
  {
    Oper o(type);
    return accept(o);
  }
  bool acceptPunct(int type)
  {
    Punct p(type);
    return accept(p);
  }

  void expect(Token& t)
  {
    if(*getNext() != t)
      err(string("expected token: ") + t.getStr());
    if(res)
      pos++;
    return res;
  }
  Token* expect(int tokType)
  {
    if(getNext()->getType() == tokType)
      return *tokens[pos++];
    err(string("expected ") + tokTypeTable[tokType]);
    return NULL;
  }
  void expectKeyword(int type)
  {
    Keyword k(type);
    expect(k);
  }
  void expectOper(int type)
  {
    Oper o(type);
    expect(o);
  }
  void expectPunct(int type)
  {
    Punct p(type);
    expect(p);
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

