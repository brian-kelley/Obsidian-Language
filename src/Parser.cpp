#include "Parser.hpp"

namespace Parser
{
  size_t pos;
  vector<Token*>* tokens;
  None none;

  //Furthest the recursive descent reached, and associated error message if parsing failed there
  size_t deepest = 0;
  string deepestErr;

  template<typename NT>
  AP(NT) parse()
  {
    cout << "FATAL ERROR: non-implemented parse called, for type " << typeid(NT).name() << "\n";
    exit(1);
    return AP(NT)(nullptr);
  }

  //Need to forward-declare all parse() specializations
  template<> AP(Module) parse<Module>();
  template<> AP(ScopedDecl) parse<ScopedDecl>();
  template<> AP(TypeNT) parse<TypeNT>();
  template<> AP(Statement) parse<Statement>();
  template<> AP(Typedef) parse<Typedef>();
  template<> AP(Return) parse<Return>();
  template<> AP(SwitchCase) parse<SwitchCase>();
  template<> AP(Switch) parse<Switch>();
  template<> AP(ForC) parse<ForC>();
  template<> AP(ForRange1) parse<ForRange1>();
  template<> AP(ForRange2) parse<ForRange2>();
  template<> AP(ForArray) parse<ForArray>();
  template<> AP(For) parse<For>();
  template<> AP(While) parse<While>();
  template<> AP(If) parse<If>();
  template<> AP(Assertion) parse<Assertion>();
  template<> AP(TestDecl) parse<TestDecl>();
  template<> AP(EnumItem) parse<EnumItem>();
  template<> AP(Enum) parse<Enum>();
  template<> AP(Block) parse<Block>();
  template<> AP(VarDecl) parse<VarDecl>();
  template<> AP(VarAssign) parse<VarAssign>();
  template<> AP(Print) parse<Print>();
  template<> AP(Expression) parse<Expression>();
  template<> AP(Call) parse<Call>();
  template<> AP(Arg) parse<Arg>();
  template<> AP(FuncDecl) parse<FuncDecl>();
  template<> AP(FuncDef) parse<FuncDef>();
  template<> AP(FuncTypeNT) parse<FuncTypeNT>();
  template<> AP(ProcDecl) parse<ProcDecl>();
  template<> AP(ProcDef) parse<ProcDef>();
  template<> AP(ProcTypeNT) parse<ProcTypeNT>();
  template<> AP(StructMem) parse<StructMem>();
  template<> AP(StructDecl) parse<StructDecl>();
  template<> AP(UnionDecl) parse<UnionDecl>();
  template<> AP(TraitDecl) parse<TraitDecl>();
  template<> AP(StructLit) parse<StructLit>();
  template<> AP(Member) parse<Member>();
  template<> AP(TraitType) parse<TraitType>();
  template<> AP(TupleTypeNT) parse<TupleTypeNT>();
  template<> AP(Expr1) parse<Expr1>();
  template<> AP(Expr1RHS) parse<Expr1RHS>();
  template<> AP(Expr2) parse<Expr2>();
  template<> AP(Expr2RHS) parse<Expr2RHS>();
  template<> AP(Expr3) parse<Expr3>();
  template<> AP(Expr3RHS) parse<Expr3RHS>();
  template<> AP(Expr4) parse<Expr4>();
  template<> AP(Expr4RHS) parse<Expr4RHS>();
  template<> AP(Expr5) parse<Expr5>();
  template<> AP(Expr5RHS) parse<Expr5RHS>();
  template<> AP(Expr6) parse<Expr6>();
  template<> AP(Expr6RHS) parse<Expr6RHS>();
  template<> AP(Expr7) parse<Expr7>();
  template<> AP(Expr7RHS) parse<Expr7RHS>();
  template<> AP(Expr8) parse<Expr8>();
  template<> AP(Expr8RHS) parse<Expr8RHS>();
  template<> AP(Expr9) parse<Expr9>();
  template<> AP(Expr9RHS) parse<Expr9RHS>();
  template<> AP(Expr10) parse<Expr10>();
  template<> AP(Expr10RHS) parse<Expr10RHS>();
  template<> AP(Expr11) parse<Expr11>();
  template<> AP(Expr12) parse<Expr12>();

  AP(Module) parseProgram(vector<Token*>& toks)
  {
    pos = 0;
    tokens = &toks;
    AP(Module) globalModule(new Module);
    globalModule->name = "";
    globalModule->decls = parseSome<ScopedDecl>();
    if(pos != tokens->size())
    {
      //If not all tokens were used, there was a parse error
      //print the deepest error message produced
      errAndQuit(deepestErr);
    }
    return globalModule;
  }

  template<>
  AP(Module) parse<Module>()
  {
    AP(Module) m(new Module);
    expectKeyword(MODULE);
    m->name = ((Ident*) expect(IDENTIFIER))->name;
    expectPunct(LBRACE);
    m->decls = parseSome<ScopedDecl>();
    expectPunct(RBRACE);
    return m;
  }

  template<>
  AP(ScopedDecl) parse<ScopedDecl>()
  {
    AP(ScopedDecl) sd(new ScopedDecl);
    //use short-circuit evaluation to find the pattern that parses successfully
    if(!(sd->decl = parseOptional<Module>()) &&
        !(sd->decl = parseOptional<VarDecl>()) &&
        !(sd->decl = parseOptional<StructDecl>()) &&
        !(sd->decl = parseOptional<UnionDecl>()) &&
        !(sd->decl = parseOptional<TraitDecl>()) &&
        !(sd->decl = parseOptional<Enum>()) &&
        !(sd->decl = parseOptional<Typedef>()) &&
        !(sd->decl = parseOptional<TestDecl>()) &&
        !(sd->decl = parseOptional<FuncDecl>()) &&
        !(sd->decl = parseOptional<FuncDef>()) &&
        !(sd->decl = parseOptional<ProcDecl>()) &&
        !(sd->decl = parseOptional<ProcDef>()))
    {
      err("invalid scoped declaration");
    }
    return sd;
  }

  template<>
  AP(TypeNT) parse<TypeNT>()
  {
    AP(TypeNT) type(new TypeNT);
    type->arrayDims = 0;
    #define TRY_PRIMITIVE(p) { \
      if(type->t.is<None>() && acceptKeyword(p)) { \
        type->t = TypeNT::Prim::p; \
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
    if(type->t.is<TypeNT::Prim>() != 1)
    {
      if(!(type->t = parseOptional<Member>()) &&
          !(type->t = parseOptional<TupleTypeNT>()) &&
          !(type->t = parseOptional<FuncTypeNT>()) &&
          !(type->t = parseOptional<ProcTypeNT>()) &&
          !(type->t = parseOptional<TraitType>()))
      {
        err("Invalid type");
      }
    }
    //check for square bracket pairs after, indicating array type
    while(true)
    {
      if(!acceptPunct(LBRACKET))
        break;
      expectPunct(RBRACKET);
      type->arrayDims++;
    }
    return type;
  }

  template<>
  AP(Statement) parse<Statement>()
  {
    AP(Statement) s(new Statement);
    if((s->s = parseOptional<ScopedDecl>()) ||
        (s->s = parseOptional<VarDecl>()) ||
        (s->s = parseOptional<VarAssign>()) ||
        (s->s = parseOptional<Print>()) ||
        (s->s = parseOptional<Block>()) ||
        (s->s = parseOptional<Return>()) ||
        (s->s = parseOptional<Continue>()) ||
        (s->s = parseOptional<Break>()) ||
        (s->s = parseOptional<Switch>()) ||
        (s->s = parseOptional<For>()) ||
        (s->s = parseOptional<While>()) ||
        (s->s = parseOptional<If>()) ||
        (s->s = parseOptional<Assertion>()) ||
        (s->s = parseOptional<EmptyStatement>()))
    {
      return s;
    }
    else
    {
      err("invalid statement");
      return s;
    }
  }

  template<>
  AP(Break) parse<Break>()
  {
    expectKeyword(BREAK);
    expectPunct(SEMICOLON);
    return AP(Break)(new Break);
  }

  template<>
  AP(Continue) parse<Continue>()
  {
    expectKeyword(CONTINUE);
    expectPunct(SEMICOLON);
    return AP(Continue)(new Continue);
  }

  template<>
  AP(EmptyStatement) parse<EmptyStatement>()
  {
    expectPunct(SEMICOLON);
    return AP(EmptyStatement)(new EmptyStatement);
  }

  template<>
  AP(Typedef) parse<Typedef>()
  {
    AP(Typedef) td(new Typedef);
    expectKeyword(TYPEDEF);
    td->type = parse<TypeNT>();
    td->ident = ((Ident*) expect(IDENTIFIER))->name;
    expectPunct(SEMICOLON);
    return td;
  }

  template<>
  AP(Return) parse<Return>()
  {
    AP(Return) r(new Return);
    expectKeyword(RETURN);
    r->ex = parseOptional<Expression>();
    expectPunct(SEMICOLON);
    return r;
  }

  template<>
  AP(SwitchCase) parse<SwitchCase>()
  {
    AP(SwitchCase) sc(new SwitchCase);
    sc->matchVal = parse<Expression>();
    expectPunct(COLON);
    sc->s = parse<Statement>();
    return sc;
  }

  template<>
  AP(Switch) parse<Switch>()
  {
    AP(Switch) sw(new Switch);
    expectKeyword(SWITCH);
    expectPunct(LPAREN);
    sw->sw = parse<Expression>();
    expectPunct(RPAREN);
    expectPunct(LBRACE);
    sw->cases = parseSome<SwitchCase>();
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
  AP(ForC) parse<ForC>()
  {
    //try to parse C style for loop
    AP(ForC) forC(new ForC);
    expectKeyword(FOR);
    expectPunct(LPAREN);
    //all 3 parts of the loop are optional
    forC->decl = parseOptional<VarDecl>();
    if(!forC->decl)
    {
      expectPunct(SEMICOLON);
    }
    forC->condition = parseOptional<Expression>();
    expectPunct(SEMICOLON);
    forC->incr = parseOptional<VarAssign>();
    expectPunct(RPAREN);
    //parse succeeded, use forC
    return forC;
  }

  template<>
  AP(ForRange1) parse<ForRange1>()
  {
    AP(ForRange1) fr1(new ForRange1);
    expectKeyword(FOR);
    fr1->expr = parse<Expression>();
    return fr1;
  }

  template<>
  AP(ForRange2) parse<ForRange2>()
  {
    AP(ForRange2) fr2(new ForRange2);
    expectKeyword(FOR);
    fr2->start = parse<Expression>();
    expectPunct(COLON);
    fr2->end = parse<Expression>();
    return fr2;
  }

  template<>
  AP(ForArray) parse<ForArray>()
  {
    AP(ForArray) fa(new ForArray);
    fa->container = parse<Expression>();
    return fa;
  }

  template<>
  AP(For) parse<For>()
  {
    AP(For) f(new For);
    if((f->f = parseOptional<ForC>()) ||
        (f->f = parseOptional<ForRange1>()) ||
        (f->f = parseOptional<ForRange2>()) ||
        (f->f = parseOptional<ForArray>()))
    {
      f->body = parse<Statement>();
      return f;
    }
    else
    {
      err("invalid for loop");
    }
    return f;
  }

  template<>
  AP(While) parse<While>()
  {
    AP(While) w(new While);
    expectKeyword(WHILE);
    expectPunct(LPAREN);
    w->cond = parse<Expression>();
    expectPunct(RPAREN);
    w->body = parse<Statement>();
    return w;
  }

  template<>
  AP(If) parse<If>()
  {
    AP(If) i(new If);
    expectKeyword(IF);
    expectPunct(LPAREN);
    i->cond = parse<Expression>();
    expectPunct(RPAREN);
    i->ifBody = parse<Statement>();
    if(acceptKeyword(ELSE))
      i->elseBody = parse<Statement>();
    return i;
  }

  template<>
  AP(Assertion) parse<Assertion>()
  {
    AP(Assertion) a(new Assertion);
    expectKeyword(ASSERT);
    a->expr = parse<Expression>();
    expectPunct(SEMICOLON);
    return a;
  }

  template<>
  AP(TestDecl) parse<TestDecl>()
  {
    AP(TestDecl) t(new TestDecl);
    expectKeyword(TEST);
    t->call = parse<Call>();
    expectPunct(SEMICOLON);
    return t;
  }

  template<>
  AP(EnumItem) parse<EnumItem>()
  {
    AP(EnumItem) ei(new EnumItem);
    ei->name = ((Ident*) expect(IDENTIFIER))->name;
    if(acceptOper(ASSIGN))
      ei->value = (IntLit*) expect(INT_LITERAL);
    else
      ei->value = nullptr;
    return ei;
  }

  template<>
  AP(Enum) parse<Enum>()
  {
    AP(Enum) e(new Enum);
    expectKeyword(ENUM);
    e->name = ((Ident*) expect(IDENTIFIER))->name;
    expectPunct(LBRACE);
    e->items = parseSomeCommaSeparated<EnumItem>();
    expectPunct(RBRACE);
    return e;
  }

  template<>
  AP(Block) parse<Block>()
  {
    AP(Block) b(new Block);
    expectPunct(LBRACE);
    b->statements = parseSome<Statement>();
    expectPunct(RBRACE);
    return b;
  }

  template<>
  AP(VarDecl) parse<VarDecl>()
  {
    AP(VarDecl) vd(new VarDecl);
    if(!acceptKeyword(AUTO))
    {
      vd->type = parse<TypeNT>();
    }
    vd->name = ((Ident*) expect(IDENTIFIER))->name;
    if(acceptOper(ASSIGN))
    {
      vd->val = parse<Expression>();
    }
    expectPunct(SEMICOLON);
    if(!vd->type && !vd->val)
    {
      err("auto declaration requires initialization");
    }
    return vd;
  }

  template<>
  AP(VarAssign) parse<VarAssign>()
  {
    AP(VarAssign) va(new VarAssign);
    va->target = parse<Member>();
    va->op = (Oper*) expect(OPERATOR);
    //unary assign operators don't have rhs
    int otype = va->op->op;
    if(otype != INC && otype != DEC)
    {
      va->rhs = parse<Expression>();
    }
    expectPunct(SEMICOLON);
    if(otype != INC && otype != DEC &&
        otype != PLUSEQ && otype != SUBEQ && otype != MULEQ &&
        otype != DIVEQ && otype != MODEQ && otype != BOREQ &&
        otype != BANDEQ && otype != BXOREQ)
    {
      err("invalid operator for variable assignment/update");
    }
    return va;
  }

  template<>
  AP(Print) parse<Print>()
  {
    AP(Print) p(new Print);
    expectKeyword(PRINT);
    expectPunct(LPAREN);
    p->exprs = parseSomeCommaSeparated<Expression>();
    expectPunct(RPAREN);
    expectPunct(SEMICOLON);
    return p;
  }

  template<>
  AP(Expression) parse<Expression>()
  {
    AP(Expression) e(new Expression);
    if((e->e = parseOptional<Call>()) ||
        (e->e = parseOptional<Member>()) ||
        (e->e = parseOptional<Expr1>()))
    {
      return e;
    }
    err("invalid expression");
    return e;
  }

  template<>
  AP(Call) parse<Call>()
  {
    AP(Call) c(new Call);
    c->callable = parse<Member>();
    expectPunct(LPAREN);
    c->args = parseSomeCommaSeparated<Expression>();
    expectPunct(RPAREN);
    return c;
  }

  template<>
  AP(Arg) parse<Arg>()
  {
    AP(Arg) a(new Arg);
    a->type = parse<TypeNT>();
    Ident* name = (Ident*) accept(IDENTIFIER);
    if(name)
    {
      a->haveName = true;
      a->name = name->name;
    }
    else
    {
      a->haveName = false;
    }
    return a;
  }

  template<>
  AP(FuncDecl) parse<FuncDecl>()
  {
    AP(FuncDecl) fd(new FuncDecl);
    expectKeyword(FUNC);
    cout << "Parsing funcdecl\n";
    fd->type.retType = parse<TypeNT>();
    fd->name = ((Ident*) expect(IDENTIFIER))->name;
    cout << "Parsing func decl " << fd->name << '\n';
    expectPunct(LPAREN);
    fd->type.args = parseSomeCommaSeparated<Arg>();
    expectPunct(RPAREN);
    expectPunct(SEMICOLON);
    return fd;
  }

  template<>
  AP(FuncDef) parse<FuncDef>()
  {
    AP(FuncDef) fd(new FuncDef);
    expectKeyword(FUNC);
    fd->type.retType = parse<TypeNT>();
    fd->name = parse<Member>();
    expectPunct(LPAREN);
    fd->type.args = parseSomeCommaSeparated<Arg>();
    expectPunct(RPAREN);
    fd->body = parse<Block>();
    return fd;
  }

  template<>
  AP(FuncTypeNT) parse<FuncTypeNT>()
  {
    AP(FuncTypeNT) ft(new FuncTypeNT);
    expectKeyword(FUNCTYPE);
    ft->retType = parse<TypeNT>();
    expectPunct(LPAREN);
    ft->args = parseSomeCommaSeparated<Arg>();
    expectPunct(RPAREN);
    return ft;
  }

  template<>
  AP(ProcDecl) parse<ProcDecl>()
  {
    AP(ProcDecl) pd(new ProcDecl);
    if(acceptKeyword(NONTERM))
      pd->type.nonterm = true;
    expectKeyword(PROC);
    pd->type.retType = parse<TypeNT>();
    pd->name = ((Ident*) expect(IDENTIFIER))->name;
    cout << "Parsing proc decl: " << pd->name << '\n';
    expectPunct(LPAREN);
    pd->type.args = parseSomeCommaSeparated<Arg>();
    expectPunct(RPAREN);
    expectPunct(SEMICOLON);
    return pd;
  }
  
  template<>
  AP(ProcDef) parse<ProcDef>()
  {
    AP(ProcDef) pd(new ProcDef);
    if(acceptKeyword(NONTERM))
      pd->type.nonterm = true;
    expectKeyword(PROC);
    pd->type.retType = parse<TypeNT>();
    pd->name = parse<Member>();
    cout << "Parsing proc def: " << pd->name << '\n';
    expectPunct(LPAREN);
    pd->type.args = parseSomeCommaSeparated<Arg>();
    expectPunct(RPAREN);
    pd->body = parse<Block>();
    return pd;
  }

  template<>
  AP(ProcTypeNT) parse<ProcTypeNT>()
  {
    AP(ProcTypeNT) pt(new ProcTypeNT);
    if(acceptKeyword(NONTERM))
      pt->nonterm = true;
    expectKeyword(PROCTYPE);
    pt->retType = parse<TypeNT>();
    expectPunct(LPAREN);
    pt->args = parseSomeCommaSeparated<Arg>();
    expectPunct(RPAREN);
    return pt;
  }

  template<>
  AP(StructMem) parse<StructMem>()
  {
    AP(StructMem) sm(new StructMem);
    if(acceptOper(BXOR))
    {
      sm->compose = true;
    }
    else
    {
      sm->compose = false;
    }
    sm->sd = parse<ScopedDecl>();
    return sm;
  }

  template<>
  AP(StructDecl) parse<StructDecl>()
  {
    AP(StructDecl) sd(new StructDecl);
    expectKeyword(STRUCT);
    sd->name = ((Ident*) expect(IDENTIFIER))->name;
    if(acceptPunct(COLON))
    {
      sd->traits = parseSomeCommaSeparated<Member>();
    }
    expectPunct(LBRACE);
    sd->members = parseSome<StructMem>();
    expectPunct(RBRACE);
    return sd;
  }

  template<>
  AP(UnionDecl) parse<UnionDecl>()
  {
    AP(UnionDecl) vd(new UnionDecl);
    expectKeyword(UNION);
    vd->name = ((Ident*) expect(IDENTIFIER))->name;
    vd->types = parseSomeCommaSeparated<TypeNT>();
    return vd;
  }

  template<>
  AP(TraitDecl) parse<TraitDecl>()
  {
    AP(TraitDecl) td(new TraitDecl);
    expectKeyword(TRAIT);
    td->name = ((Ident*) expect(IDENTIFIER))->name;
    cout << "Parsing trait " << td->name << '\n';
    expectPunct(LBRACE);
    while(true)
    {
      AP(FuncDecl) fd;
      AP(ProcDecl) pd;
      if((fd = parseOptional<FuncDecl>()) ||
          (pd = parseOptional<ProcDecl>()))
      {
        if(fd)
        {
          td->members.emplace_back(fd);
        }
        else
        {
          td->members.emplace_back(pd);
        }
      }
      else
      {
        break;
      }
    }
    expectPunct(RBRACE);
    std::cout << "Done parsing trait.\n";
    return td;
  }

  template<>
  AP(StructLit) parse<StructLit>()
  {
    AP(StructLit) sl(new StructLit);
    expectPunct(LBRACE);
    sl->vals = parseSomeCommaSeparated<Expression>();
    expectPunct(RBRACE);
    return sl;
  }

  template<>
  AP(Member) parse<Member>()
  {
    AP(Member) m(new Member);
    m->owner = ((Ident*) expect(IDENTIFIER))->name;
    if(acceptPunct(DOT))
    {
      m->mem = parse<Member>();
    }
    return m;
  }

  template<>
  AP(TraitType) parse<TraitType>()
  {
    AP(TraitType) tt(new TraitType);
    tt->localName = ((Ident*) expect(IDENTIFIER))->name;
    expectPunct(COLON);
    tt->traits = parseSomeCommaSeparated<Member>();
    return tt;
  }

  template<>
  AP(TupleTypeNT) parse<TupleTypeNT>()
  {
    AP(TupleTypeNT) tt(new TupleTypeNT);
    expectPunct(LPAREN);
    tt->members = parseSomeCommaSeparated<TypeNT>();
    expectPunct(RPAREN);
    return tt;
  }

  template<>
  AP(BoolLit) parse<BoolLit>()
  {
    AP(BoolLit) bl(new BoolLit);
    if(acceptKeyword(TRUE))
    {
      bl->val = true;
    }
    else if(acceptKeyword(FALSE))
    {
      bl->val = false;
    }
    else
    {
      err("invalid bool literal");
    }
    return bl;
  }

  template<>
  AP(Expr1) parse<Expr1>()
  {
    AP(Expr1) e1(new Expr1);
    e1->head = parse<Expr2>();
    e1->tail = parseSome<Expr1RHS>();
    return e1;
  }

  template<>
  AP(Expr1RHS) parse<Expr1RHS>()
  {
    AP(Expr1RHS) e1r(new Expr1RHS);
    expectOper(LOR);
    e1r->rhs = parse<Expr2>();
    return e1r;
  }

  template<>
  AP(Expr2) parse<Expr2>()
  {
    AP(Expr2) e2(new Expr2);
    e2->head = parse<Expr3>();
    e2->tail = parseSome<Expr2RHS>();
    return e2;
  }

  template<>
  AP(Expr2RHS) parse<Expr2RHS>()
  {
    AP(Expr2RHS) e2r(new Expr2RHS);
    expectOper(LAND);
    e2r->rhs = parse<Expr3>();
    return e2r;
  }

  template<>
  AP(Expr3) parse<Expr3>()
  {
    AP(Expr3) e3(new Expr3);
    e3->head = parse<Expr4>();
    e3->tail = parseSome<Expr3RHS>();
    return e3;
  }

  template<>
  AP(Expr3RHS) parse<Expr3RHS>()
  {
    AP(Expr3RHS) e3r(new Expr3RHS);
    expectOper(BOR);
    e3r->rhs = parse<Expr4>();
    return e3r;
  }

  template<>
  AP(Expr4) parse<Expr4>()
  {
    AP(Expr4) e4(new Expr4);
    e4->head = parse<Expr5>();
    e4->tail = parseSome<Expr4RHS>();
    return e4;
  }

  template<>
  AP(Expr4RHS) parse<Expr4RHS>()
  {
    AP(Expr4RHS) e4r(new Expr4RHS);
    expectOper(BXOR);
    e4r->rhs = parse<Expr5>();
    return e4r;
  }

  template<>
  AP(Expr5) parse<Expr5>()
  {
    AP(Expr5) e5(new Expr5);
    e5->head = parse<Expr6>();
    e5->tail = parseSome<Expr5RHS>();
    return e5;
  }

  template<>
  AP(Expr5RHS) parse<Expr5RHS>()
  {
    AP(Expr5RHS) e5r(new Expr5RHS);
    expectOper(BAND);
    e5r->rhs = parse<Expr6>();
    return e5r;
  }

  template<>
  AP(Expr6) parse<Expr6>()
  {
    AP(Expr6) e6(new Expr6);
    e6->head = parse<Expr7>();
    e6->tail = parseSome<Expr6RHS>();
    return e6;
  }

  template<>
  AP(Expr6RHS) parse<Expr6RHS>()
  {
    AP(Expr6RHS) e6r(new Expr6RHS);
    e6r->op = ((Oper*) expect(OPERATOR))->op;
    if(e6r->op != CMPEQ && e6r->op != CMPNEQ)
    {
      err("expected == !=");
    }
    e6r->rhs = parse<Expr7>();
    return e6r;
  }

  template<>
  AP(Expr7) parse<Expr7>()
  {
    AP(Expr7) e7(new Expr7);
    e7->head = parse<Expr8>();
    e7->tail = parseSome<Expr7RHS>();
    return e7;
  }

  template<>
  AP(Expr7RHS) parse<Expr7RHS>()
  {
    AP(Expr7RHS) e7r(new Expr7RHS);
    e7r->op = ((Oper*) expect(OPERATOR))->op;
    if(e7r->op != CMPL && e7r->op != CMPLE &&
        e7r->op != CMPG && e7r->op != CMPGE)
    {
      err("expected < > <= >=");
    }
    e7r->rhs = parse<Expr8>();
    return e7r;
  }

  template<>
  AP(Expr8) parse<Expr8>()
  {
    AP(Expr8) e8(new Expr8);
    e8->head = parse<Expr9>();
    e8->tail = parseSome<Expr8RHS>();
    return e8;
  }

  template<>
  AP(Expr8RHS) parse<Expr8RHS>()
  {
    AP(Expr8RHS) e8r(new Expr8RHS);
    e8r->op = ((Oper*) expect(OPERATOR))->op;
    if(e8r->op != SHL && e8r->op != SHR)
    {
      err("expected << >>");
    }
    e8r->rhs = parse<Expr9>();
    return e8r;
  }

  template<>
  AP(Expr9) parse<Expr9>()
  {
    AP(Expr9) e9(new Expr9);
    e9->head = parse<Expr10>();
    e9->tail = parseSome<Expr9RHS>();
    return e9;
  }

  template<>
  AP(Expr9RHS) parse<Expr9RHS>()
  {
    AP(Expr9RHS) e9r(new Expr9RHS);
    e9r->op = ((Oper*) expect(OPERATOR))->op;
    if(e9r->op != PLUS && e9r->op != SUB)
    {
      err("expected + -");
    }
    e9r->rhs = parse<Expr10>();
    return e9r;
  }

  template<>
  AP(Expr10) parse<Expr10>()
  {
    AP(Expr10) e10(new Expr10);
    e10->head = parse<Expr11>();
    e10->tail = parseSome<Expr10RHS>();
    return e10;
  }

  template<>
  AP(Expr10RHS) parse<Expr10RHS>()
  {
    AP(Expr10RHS) e10r(new Expr10RHS);
    e10r->op = ((Oper*) expect(OPERATOR))->op;
    if(e10r->op != MUL && e10r->op != DIV && e10r->op != MOD)
    {
      err("expected * / %");
    }
    e10r->rhs = parse<Expr11>();
    return e10r;
  }

  template<>
  AP(Expr11) parse<Expr11>()
  {
    AP(Expr11) e11(new Expr11);
    Oper* oper = (Oper*) accept(OPERATOR);
    if(oper)
    {
      if(oper->op != SUB && oper->op != LNOT && oper->op != BNOT)
      {
        err("invalid unary operator");
      }
      Expr11::UnaryExpr ue;
      ue.op = oper->op;
      ue.rhs = parse<Expr11>();
      e11->e = ue;
    }
    else
    {
      e11->e = parse<Expr12>();
    }
    return e11;
  }

  template<>
  AP(Expr12) parse<Expr12>()
  {
    AP(Expr12) e12(new Expr12);
    if(acceptPunct(LPAREN))
    {
      e12->e = parse<Expression>();
      expectPunct(RPAREN);
    }
    if(!e12->e.is<None>() ||
        (e12->e = (IntLit*) accept(INT_LITERAL)) ||
        (e12->e = (CharLit*) accept(CHAR_LITERAL)) ||
        (e12->e = (StrLit*) accept(STRING_LITERAL)) ||
        (e12->e = (FloatLit*) accept(FLOAT_LITERAL)) ||
        (e12->e = parseOptional<BoolLit>()) ||
        (e12->e = parseOptional<Member>()) ||
        (e12->e = parseOptional<StructLit>()))
    {
      //check for array indexing
      if(acceptPunct(LBRACKET))
      {
        Expr12::ArrayIndex ai;
        ai.arr = e12;
        ai.index = parse<Expression>();
        expectPunct(RBRACKET);
      }
      return e12;
    }
    err("invalid expression.");
    return e12;
  }

  bool accept(Token& t)
  {
    bool res = getNext()->compareTo(&t);
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
    Token* next = getNext();
    bool res = t.compareTo(next);
    if(res)
    {
      pos++;
      return;
    }
    err(string("expected ") + t.getStr() + " but got " + next->getStr());
  }

  Token* expect(int tokType)
  {
    Token* next = getNext();
    bool res = next->getType() == tokType;
    if(res)
      pos++;
    else
      err(string("expected a ") + tokTypeTable[tokType] + " but got " + next->getStr());
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

  Token* getNext()
  {
    if(pos >= tokens->size())
      return &PastEOF::inst;
    else
      return (*tokens)[pos];
  }

  Token* lookAhead(int ahead)
  {
    if(pos + ahead >= tokens->size())
      return &PastEOF::inst;
    else
      return (*tokens)[pos + ahead];
  }

  void unget()
  {
    pos--;
  }

  void err(string msg)
  {
    string fullMsg = string("Parse error on line ") + to_string(getNext()->line) + ", col " + to_string(getNext()->col);
    if(msg.length())
      fullMsg += string(": ") + msg;
    else
      fullMsg += '.';
    if(pos > deepest)
    {
      deepest = pos;
      deepestErr = fullMsg;
    }
    throw ParseErr(deepestErr);
  }

  ScopedDecl::ScopedDecl() : decl(none) {}
  TypeNT::TypeNT() : t(none) {}
  Statement::Statement() : s(none) {}
  For::For() : f(none) {}
  Expression::Expression() : e(none) {}
  Expr11::Expr11() : e(none) {}
  Expr12::Expr12() : e(none) {}
}

