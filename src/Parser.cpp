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
    ScopedDecl sd;
    if(!sd.nt) sd.nt = parseOptional<Module>();
    if(!sd.nt) sd.nt = parseOptional<VarDecl>();
    if(!sd.nt) sd.nt = parseOptional<VariantDecl>();
    if(!sd.nt) sd.nt = parseOptional<TraitDecl>();
    if(!sd.nt) sd.nt = parseOptional<Enum>();
    if(!sd.nt) sd.nt = parseOptional<Typedef>();
    if(!sd.nt) sd.nt = parseOptional<TestDecl>();
    if(!sd.nt) sd.nt = parseOptional<FuncDecl>();
    if(!sd.nt) sd.nt = parseOptional<FuncDef>();
    if(!sd.nt) sd.nt = parseOptional<ProcDecl>();
    if(!sd.nt) sd.nt = parseOptional<ProcDef>();
    throw ParseErr("invalid scoped declaration");
    return new ScopedDecl(sd);
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

