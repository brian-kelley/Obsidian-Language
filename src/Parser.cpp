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
    #define TRY_PARSE(type) { \
      UP<type> type##Val = parseOptional<type>(); \
      if(type##Val) { \
        sd.decl = type##Val; \
        return sd; \
      } \
    }
    TRY_PARSE(Module);
    TRY_PARSE(VarDecl);
    TRY_PARSE(VariantDecl);
    TRY_PARSE(TraitDecl);
    TRY_PARSE(Enum);
    TRY_PARSE(Typedef);
    TRY_PARSE(TestDecl);
    TRY_PARSE(FuncDecl);
    TRY_PARSE(FuncDef);
    TRY_PARSE(ProcDecl);
    TRY_PARSE(ProcDef);
    #undef TRY_PARSE
    throw ParseErr("invalid scoped declaration");
    return UP<ScopedDecl>;
  }

  template<>
  UP<Type> parse<Type>()
  {
  /*
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
    */

    UP<Type> type = UP(new Type);
    #define TRY_PRIMITIVE(p) { \
      if(acceptKeyword(p)) { \
        type.t = Prim::p; \
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
    UP<Type> arrayBase = UP(new Type(*type));

  }

  template<>
  Statement* parse<Statement>()
  {
    Statement s;
    if(!s.nt) s.nt = parseOptional<ScopedDecl>();
    if(!s.nt) s.nt = parseOptional<VarAssign>();
    if(!s.nt) s.nt = parseOptional<Print>();
    //expression must be followed by semicolon
    if(!s.nt && (s.nt = parseOptional<Expression>()))
      expectPunct(SEMICOLON);
    if(!s.nt) s.nt = parseOptional<Block>();
    if(!s.nt) s.nt = parseOptional<Return>();
    if(!s.nt) s.nt = parseOptional<Continue>();
    if(!s.nt) s.nt = parseOptional<Break>();
    if(!s.nt) s.nt = parseOptional<Switch>();
    if(!s.nt) s.nt = parseOptional<For>();
    if(!s.nt) s.nt = parseOptional<While>();
    if(!s.nt) s.nt = parseOptional<If>();
    if(!s.nt) s.nt = parseOptional<Using>();
    if(!s.nt) s.nt = parseOptional<Assertion>();
    if(!s.nt) s.nt = parseOptional<EmptyStatement>();
    if(!s.nt)
      throw parseErr("invalid statement");
    return new Statement(s);
  }

  template<>
  Typedef* parse<Typedef>()
  {
    Typedef t;
    expectKeyword(TYPEDEF);
    t.type = parse<Type>();
    t.ident = (Ident*) expect(IDENT);
    expectPunct(SEMICOLON);
    return new Typedef(t);
  }

  template<>
  Return* parse<Return>()
  {
    expectKeyword(RETURN);
    Return* r = new Return;
    r->ex = parseOptional<Expression>();
    expectPunct(SEMICOLON);
    return r;
  }

  template<>
  Switch* parse<Switch>()
  {
    Switch sw;
    sw.defaultStatement = NULL;
    expectKeyword(SWITCH);
    expectPunct(LPAREN);
    sw.sw = parse<Expression>();
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
        sw.cases.push_back(sc);
      }
      else
        break;
    }
    if(acceptKeyword(DEFAULT))
    {
      sw.defaultStatement = parse<Statement>();
    }
    expectPunct(RBRACE);
  }

  template<>
  ForC* parse<ForC>()
  {
    //try to parse C style for loop
    ForC forC;
    expectKeyword(FOR);
    expectPunct(LPAREN);
    //all 3 parts of the loop are optional
    forC.decl = parseOptional<VarDecl>();
    if(!forC.decl)
    {
      expectPunct(SEMICOLON);
    }
    forC.condition = parseOptional<Expression>();
    expectPunct(SEMICOLON);
    forC.incr = parseOptional<VarDecl>();
    expectParen(RPAREN);
    //parse succeeded, use forC
    return new ForC(forC);
  }

  template<>
  ForRange1* parse<ForRange1>()
  {
    ForRange1 fr1;
    expectKeyword(FOR);
    fr1.expr = parse<Expression>();
    return new ForRange1(fr1);
  }

  template<>
  ForRange2* parse<ForRange2>()
  {
    ForRange1 fr1;
    expectKeyword(FOR);
    fr1.start = parse<Expression>();
    expectPunct(COLON);
    fr1.end = parse<Expression>();
    return new ForRange1(fr1);
  }

  template<>
  ForArray* parse<ForArray>()
  {
    Expression* arr = parse<Expression>();
    ForArray fa;
    fa.container = arr;
    return new ForArray(fa);
  }

  template<>
  For* parse<For>()
  {
    if(f.loop.forC = parseOptional<ForC>())
    {
      f.type = For::C_STYLE;
    }
    else if(f.loop.forRange1 = parseOptional<ForRange1>())
    {
      f.type = For::RANGE_1;
    }
    else if(f.loop.forRange2 = parseOptional<ForRange2>())
    {
      f.type = For::RANGE_2;
    }
    else if(f.loop.forArray = parseOptional<ForArray>())
    {
      f.type = For::ARRAY;
    }
    else
    {
      throw ParseErr("invalid for loop");
    }
    f.body = parse<Expression>();
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

