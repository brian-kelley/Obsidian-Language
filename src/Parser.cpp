#include "Parser.hpp"

namespace Parser
{
  int pos;
  vector<Token*>* tokens;

  UP<ModuleDef> parseProgram(vector<Token*>& toks)
  {
    pos = 0;
    tokens = &toks;
    return parse<ModuleDef>();
  }

  template<>
  UP<Module> parse<Module>()
  {
    UP<Module> m = UP(new Module);
    expectKeyword(MODULE);
    m->name = ((Ident*) expect(IDENTIFIER))->name;
    expectPunct(LBRACE);
    m->def = parse<ModuleDef>();
    expectPunct(RBRACE);
    return m
  }

  template<>
  UP<ModuleDef> parse<ModuleDef>()
  {
    UP<ModuleDef> md = UP(new ModuleDef);
    md->decls = parseMany<ScopedDecl>();
    return md;
  }

  template<>
  UP<ScopedDecl> parse<ScopedDecl>()
  {
    UP<ScopedDecl> sd = UP(new ScopedDecl);
    //use short-circuit evaluation to find the pattern that parses successfully
    if(sd->decl = parseOptional<Module>() ||
        sd->decl = parseOptional<VarDecl() ||
        sd->decl = parseOptional<VariantDecl>() ||
        sd->decl = parseOptional<TraitDecl>() ||
        sd->decl = parseOptional<Enum>() ||
        sd->decl = parseOptional<Typedef>() ||
        sd->decl = parseOptional<TestDecl>() ||
        sd->decl = parseOptional<FuncDecl>() ||
        sd->decl = parseOptional<FuncDef>() ||
        sd->decl = parseOptional<ProcDecl>() ||
        sd->decl = parseOptional<ProcDef>())
    {
      return sd;
    }
    else
    {
      throw ParseErr("invalid scoped declaration");
      return UP<ScopedDecl>;
    }
  }

  template<>
  UP<Type> parse<Type>()
  {
    UP<Type> type = UP(new Type);
    #define TRY_PRIMITIVE(p) { \
      if(acceptKeyword(p)) { \
        type.t = Type::Prim::p; \
        return type; \
      } \
    }
    TRY_PRIMITIVE(BOOL);
    TRY_PRIMITIVE(CHAR);
    TRY_PRIMITIVE(UCHAR);
    TRY_PRIMITIVE(SHORT);
    TRY_PRIMITIVE(USHORT);
    TRY_PRIMITIVE(INT);
    TRY_PRIMITIVE(UINT);
    TRY_PRIMITIVE(LONG);
    TRY_PRIMITIVE(ULONG);
    TRY_PRIMITIVE(FLOAT);
    TRY_PRIMITIVE(DOUBLE);
    TRY_PRIMITIVE(STRING);
    #undef TRY_PRIMITIVE
    if(!(type.t = parseOptional<Member>()))
    {
      if(!type.t = parseOptional<TupleType>())
      {
        throw ParseErr("Invalid type");
      }
    }
    //check for square bracket pairs after, indicating array type
    int dims = 0;
    while(true)
    {
      if(!acceptPunct(LBRACKET))
        break;
      expectPunct(RBRACKET);
      dims++;
    }
    if(dims > 0)
    {
      ArrayType at;
      //ArrayType takes ownership of type
      at.t = type;
      at.dims = dims;
      UP arrType = UP(new Type);
      arrType->t = at;
      return arrType;
    }
    else
    {
      //not an array
      return type;
    }
  }

  template<>
  UP<Statement> parse<Statement>()
  {
    UP<Statement> s = UP(new Statement);
    if(s->s = parseOptional<Expression>())
    {
      expectPunct(SEMICOLON);
      return s;
    }
    if((s->s = parseOptional<ScopedDecl>()) ||
        (s->s = parseOption<VarAssign>()) ||
        (s->s = parseOptional<Print>()) ||
        (s->s = parseOptional<Block>()) ||
        (s->s = parseOptional<Return>()) ||
        (s->s = parseOptional<Continue>()) ||
        (s->s = parseOptional<Break>()) ||
        (s->s = parseOptional<Switch>()) ||
        (s->s = parseOptional<For>()) ||
        (s->s = parseOptional<While>()) ||
        (s->s = parseOptional<If>()) ||
        (s->s = parseOptional<Using>()) ||
        (s->s = parseOptional<Assertion>()) ||
        (s->s = parseOptional<EmptyStatement>()))
    {
      return s;
    }
    else
    {
      throw parseErr("invalid statement");
      return UP<Statement>;
    }
  }

  template<>
  UP<Typedef> parse<Typedef>()
  {
    UP<Typedef> td = UP(new Typedef);
    expectKeyword(TYPEDEF);
    td->type = parse<Type>();
    td->ident = ((Ident*) expect(IDENT))->name;
    expectPunct(SEMICOLON);
    return td;
  }

  template<>
  UP<Return> parse<Return>()
  {
    UP<Return> r = UP(new Return);
    expectKeyword(RETURN);
    r->ex = parseOptional<Expression>();
    expectPunct(SEMICOLON);
    return r;
  }

  template<>
  UP<Switch> parse<Switch>()
  {
    UP<Switch> sw = UP(new Switch);
    expectKeyword(SWITCH);
    expectPunct(LPAREN);
    sw->sw = parse<Expression>();
    expectPunct(RPAREN);
    expectPunct(LBRACE);
    while(true)
    {
      if(acceptKeyword(CASE))
      {
        SwitchCase sc;
        sc.matchVal = parse<Expression>();
        expectPunct(COLON);
        sc.s = parse<Statement>();
        sw->cases.push_back(sc);
      }
      else
        break;
    }
    if(acceptKeyword(DEFAULT))
    {
      expectPunct(COLON);
      sw->defaultStatement = parse<Statement>();
      //otherwise, leave defaultStatment NULL
    }
    expectPunct(RBRACE);
    return sw;
  }

  template<>
  UP<ForC> parse<ForC>()
  {
    //try to parse C style for loop
    UP<ForC> forC = UP(new ForC);
    expectKeyword(FOR);
    expectPunct(LPAREN);
    //all 3 parts of the loop are optional
    forC->decl = parseOptional<VarDecl>();
    if(!forC.decl)
    {
      expectPunct(SEMICOLON);
    }
    forC->condition = parseOptional<Expression>();
    expectPunct(SEMICOLON);
    forC->incr = parseOptional<VarDecl>();
    expectParen(RPAREN);
    //parse succeeded, use forC
    return forC;
  }

  template<>
  UP<ForRange1> parse<ForRange1>()
  {
    UP<ForRange1> fr1 = UP(new ForRange1);
    expectKeyword(FOR);
    fr1->expr = parse<Expression>();
    return fr1;
  }

  template<>
  UP<ForRange2> parse<ForRange2>()
  {
    UP<ForRange2> fr2;
    expectKeyword(FOR);
    fr2->start = parse<Expression>();
    expectPunct(COLON);
    fr2->end = parse<Expression>();
    return fr2;
  }

  template<>
  UP<ForArray> parse<ForArray>()
  {
    UP<ForArray> fa = UP(new ForArray);
    fa->container = parse<Expression>();
    return fa;
  }

  template<>
  UP<For> parse<For>()
  {
    UP<For> f = UP(new For);
    if((f.f = parseOptional<ForC>()) ||
        (f.f = parseOptional<ForRange1>()) ||
        (f.f = parseOptional<ForRange2>()) ||
        (f.f = parseOptional<ForArray>()))
    {
      f.body = parse<Expression>();
      return f;
    }
    else
    {
      throw ParseErr("invalid for loop");
    }
  }

  template<>
  While* parse<While>()
  {
    While w;
    expectKeyword(WHILE);
    expectPunct(LPAREN);
    w.cond = parse<Expression>();
    expectPunct(RPAREN);
    w.body = parse<Statement>();
    return new While(w);
  }

  template<>
  If* parse<If>()
  {
    If i;
    expectKeyword(IF);
    expectPunct(LPAREN);
    i.cond = parse<Expression>();
    expectPunct(RPAREN);
    i.ifBody = parse<Statement>();
    if(acceptKeyword(ELSE))
      i.elseBody = parse<Statement>();
    else
      i.elseBody = NULL;
    return new If(i);
  }

  template<>
  Using* parse<Using>()
  {
    Using u;
    expectKeyword(USING);
    u.mem = parse<Member>();
    expectPunct(SEMICOLON);
    return new Using(u);
  }

  template<>
  Assertion* parse<Assertion>()
  {
    Assertion a;
    expectKeyword(ASSERT);
    a.expr = parse<Expression>();
    expectPunct(SEMICOLON);
    return new Assertion(a);
  }

  template<>
  TestDecl* parse<TestDecl>()
  {
    Test t;
    expectKeyword(TEST);
    t.call = parse<Call>();
    expectPunct(SEMICOLON);
    return new Test(t);
  }

  template<>
  Enum* parse<Enum>()
  {
    Enum e;
    expectKeyword(ENUM);
    e.name = (Ident*) expect(IDENTIFIER);
    expectPunct(LBRACE);
    while(true)
    {
      if(e.items.size() != 0)
        expectPunct(COMMA);
      EnumItem item;
      item.name = (Ident*) accept(IDENTIFIER);
      if(!item.name)
        break;
      if(acceptOper(ASSIGN))
      {
        item.value = (IntLit*) expect(INT_LITERAL);
      }
      e.items.push_back(item);
    }
    expectPunct(RBRACE);
    return new Enum(e);
  }

  template<>
  Block* parse<Block>()
  {
    Block b;
    expectPunct(LBRACE);
    while(true)
    {
      Statement* s = parseOptional<Statement>();
      if(!s)
        break;
      b.statements.push_back(s);
    }
    expectPunct(RBRACE);
    return new Block(b);
  }

  template<>
  VarDecl* parse<VarDecl>()
  {
  }

  template<>
  VarAssign* parse<VarAssign>()
  {
  }

  template<>
  Print* parse<Print>()
  {
  }

  template<>
  Expression* parse<Expression>()
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
  
  //nonterm destructors

  Module::Module(const Module& rhs)
  {
    name = rhs.name;
    def = new ModuleDef(rhs.def);
  }
  Module::~Module()
  {
    if(def)
      delete def;
  }

  ModuleDef::~ModuleDef()
  {
    for(auto decl : decls)
      delete decl;
  }

  ScopedDecl::~ScopedDecl()
  {
    if(nt)
      delete nt;
  }

  Type::~Type()
  {
    switch(type)
    {
      case MEMBER:
        delete t.member;
        break;
      case ARRAY:
        delete t.arr;
        break;
      case TUPLE:
        delete t.tup;
        break;
      default:;
    }
  }

  Statement::~Statement()
  {
    if(nt)
      delete nt;
  }

  Typedef::~Typedef()
  {
    if(type)
      delete type;
  }

  Return::~Return()
  {
    if(ex)
      delete ex;
  }

  Switch::~Switch()
  {
  }

  For::~For()
  {
  }
  
  While::~While()
  {
  }

  If::~If()
  {
  }

  Using::~Using()
  {
  }

  Assertion::~Assertion()
  {
  }

  TestDecl::~TestDecl()
  {
  }

  Enum::~Enum()
  {
  }

  Block::~Block()
  {
  }

  VarDecl::~VarDecl()
  {
  }

  VarAssign::~VarAssign()
  {
  }

  Print::~Print()
  {
  }

  Expression::~Expression()
  {
  }

  Call::~Call()
  {
  }

  Arg::~Arg()
  {
  }

  Args::~Args()
  {
  }

  FuncDecl::~FuncDecl()
  {
  }

  FuncDef::~FuncDef()
  {
  }

  FuncType::~FuncType()
  {
  }

  ProcDecl::~ProcDecl()
  {
  }

  ProcDef::~ProcDef()
  {
  }

  ProcType::~ProcType()
  {
  }

  StructDecl::~StructDecl()
  {
  }

  VariantDecl::~VariantDecl()
  {
  }

  TraitDecl::~TraitDecl()
  {
  }

  ArrayLit::~ArrayLit()
  {
  }

  StructLit::~StructLit()
  {
  }

  Member::~Member()
  {
  }

  TraitType::~TraitType()
  {
  }

  TupleType::~TupleType()
  {
  }

  BoolLit::~BoolLit()
  {
  }

  Expr1::~Expr1()
  {
  }

  Expr1RHS::~Expr1RHS()
  {
  }

  Expr2::~Expr2()
  {
  }

  Expr2RHS::~Expr2RHS()
  {
  }

  Expr3::~Expr3()
  {
  }

  Expr3RHS::~Expr3RHS()
  {
  }

  Expr4::~Expr4()
  {
  }

  Expr4RHS::~Expr4RHS()
  {
  }

  Expr5::~Expr5()
  {
  }

  Expr5RHS::~Expr5RHS()
  {
  }

  Expr6::~Expr6()
  {
  }

  Expr6RHS::~Expr6RHS()
  {
  }

  Expr7::~Expr7()
  {
  }

  Expr7RHS::~Expr7RHS()
  {
  }

  Expr8::~Expr8()
  {
  }

  Expr8RHS::~Expr8RHS()
  {
  }

  Expr9::~Expr9()
  {
  }

  Expr9RHS::~Expr9RHS()
  {
  }

  Expr10::~Expr10()
  {
  }

  Expr10RHS::~Expr10RHS()
  {
  }

  Expr11::~Expr11()
  {
  }

  Expr11RHS::~Expr11RHS()
  {
  }

  Expr12::~Expr12()
  {
  }
}

