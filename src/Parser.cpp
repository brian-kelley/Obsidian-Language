#include "Parser.hpp"

namespace Parser
{
  size_t pos;
  vector<Token*>* tokens;
  None none;

  template<typename NT>
  UP(NT) parse()
  {
    cout << "FATAL ERROR: non-implemented parse called.\n";
    exit(1);
    return UP(NT)(nullptr);
  }

  //Need to forward-declare all parse() specializations
  template<> UP(Module) parse<Module>();
  template<> UP(ModuleDef) parse<ModuleDef>();
  template<> UP(ScopedDecl) parse<ScopedDecl>();
  template<> UP(Type) parse<Type>();
  template<> UP(Statement) parse<Statement>();
  template<> UP(Typedef) parse<Typedef>();
  template<> UP(Return) parse<Return>();
  template<> UP(SwitchCase) parse<SwitchCase>();
  template<> UP(Switch) parse<Switch>();
  template<> UP(ForC) parse<ForC>();
  template<> UP(ForRange1) parse<ForRange1>();
  template<> UP(ForRange2) parse<ForRange2>();
  template<> UP(ForArray) parse<ForArray>();
  template<> UP(For) parse<For>();
  template<> UP(While) parse<While>();
  template<> UP(If) parse<If>();
  template<> UP(Using) parse<Using>();
  template<> UP(Assertion) parse<Assertion>();
  template<> UP(TestDecl) parse<TestDecl>();
  template<> UP(EnumItem) parse<EnumItem>();
  template<> UP(Enum) parse<Enum>();
  template<> UP(Block) parse<Block>();
  template<> UP(VarDecl) parse<VarDecl>();
  template<> UP(VarAssign) parse<VarAssign>();
  template<> UP(Print) parse<Print>();
  template<> UP(Expression) parse<Expression>();
  template<> UP(Call) parse<Call>();
  template<> UP(Arg) parse<Arg>();
  template<> UP(FuncDecl) parse<FuncDecl>();
  template<> UP(FuncDef) parse<FuncDef>();
  template<> UP(FuncType) parse<FuncType>();
  template<> UP(ProcDecl) parse<ProcDecl>();
  template<> UP(ProcDef) parse<ProcDef>();
  template<> UP(ProcType) parse<ProcType>();
  template<> UP(StructMem) parse<StructMem>();
  template<> UP(StructDecl) parse<StructDecl>();
  template<> UP(VariantDecl) parse<VariantDecl>();
  template<> UP(TraitDecl) parse<TraitDecl>();
  template<> UP(StructLit) parse<StructLit>();
  template<> UP(Member) parse<Member>();
  template<> UP(TraitType) parse<TraitType>();
  template<> UP(TupleType) parse<TupleType>();
  template<> UP(Expr1) parse<Expr1>();
  template<> UP(Expr1RHS) parse<Expr1RHS>();
  template<> UP(Expr2) parse<Expr2>();
  template<> UP(Expr2RHS) parse<Expr2RHS>();
  template<> UP(Expr3) parse<Expr3>();
  template<> UP(Expr3RHS) parse<Expr3RHS>();
  template<> UP(Expr4) parse<Expr4>();
  template<> UP(Expr4RHS) parse<Expr4RHS>();
  template<> UP(Expr5) parse<Expr5>();
  template<> UP(Expr5RHS) parse<Expr5RHS>();
  template<> UP(Expr6) parse<Expr6>();
  template<> UP(Expr6RHS) parse<Expr6RHS>();
  template<> UP(Expr7) parse<Expr7>();
  template<> UP(Expr7RHS) parse<Expr7RHS>();
  template<> UP(Expr8) parse<Expr8>();
  template<> UP(Expr8RHS) parse<Expr8RHS>();
  template<> UP(Expr9) parse<Expr9>();
  template<> UP(Expr9RHS) parse<Expr9RHS>();
  template<> UP(Expr10) parse<Expr10>();
  template<> UP(Expr10RHS) parse<Expr10RHS>();
  template<> UP(Expr11) parse<Expr11>();
  template<> UP(Expr11RHS) parse<Expr11RHS>();
  template<> UP(Expr12) parse<Expr12>();

  UP(ModuleDef) parseProgram(vector<Token*>& toks)
  {
    pos = 0;
    tokens = &toks;
    UP(ModuleDef) prog = parse<ModuleDef>();
    if(pos != tokens->size())
    {
      cout << "Parse error: not all tokens accepted.\n";
      cout << "Remaining tokens:";
      for(size_t i = pos; i < tokens->size(); i++)
      {
        cout << (*tokens)[i]->getStr() << " ";
      }
      cout << '\n';
    }
    return prog;
  }

  template<>
  UP(Module) parse<Module>()
  {
    UP(Module) m(new Module);
    expectKeyword(MODULE);
    m->name = ((Ident*) expect(IDENTIFIER))->name;
    expectPunct(LBRACE);
    m->def = parse<ModuleDef>();
    expectPunct(RBRACE);
    return m;
  }

  template<>
  UP(ModuleDef) parse<ModuleDef>()
  {
    UP(ModuleDef) md(new ModuleDef);
    md->decls = parseSome<ScopedDecl>();
    cout << ">>> Successfully parsed " << md->decls.size() << " scoped decls.\n";
    return md;
  }

  template<>
  UP(ScopedDecl) parse<ScopedDecl>()
  {
    cout << "Trying to parse ScopedDecl at " << pos << '\n';
    UP(ScopedDecl) sd(new ScopedDecl);
    //use short-circuit evaluation to find the pattern that parses successfully
    if(!(sd->decl = parseOptional<Module>()) &&
        !(sd->decl = parseOptional<VarDecl>()) &&
        !(sd->decl = parseOptional<VariantDecl>()) &&
        !(sd->decl = parseOptional<TraitDecl>()) &&
        !(sd->decl = parseOptional<Enum>()) &&
        !(sd->decl = parseOptional<Typedef>()) &&
        !(sd->decl = parseOptional<TestDecl>()) &&
        !(sd->decl = parseOptional<FuncDecl>()) &&
        !(sd->decl = parseOptional<FuncDef>()) &&
        !(sd->decl = parseOptional<ProcDecl>()) &&
        !(sd->decl = parseOptional<ProcDef>()))
    {
      throw ParseErr("invalid scoped declaration");
    }
    return sd;
  }

  template<>
  UP(Type) parse<Type>()
  {
    UP(Type) type(new Type);
    #define TRY_PRIMITIVE(p) { \
      if(acceptKeyword(p)) { \
        type->t = Type::Prim::p; \
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
    if(!(type->t = parseOptional<Member>()))
    {
      if(!(type->t = parseOptional<TupleType>()))
      {
        throw ParseErr("Invalid type");
      }
    }
    //check for square bracket pairs after, indicating array type
    type->arrayDims = 0;
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
  UP(Statement) parse<Statement>()
  {
    UP(Statement) s(new Statement);
    if(s->s = parseOptional<Expression>())
    {
      expectPunct(SEMICOLON);
      return s;
    }
    if((s->s = parseOptional<ScopedDecl>()) ||
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
        (s->s = parseOptional<Using>()) ||
        (s->s = parseOptional<Assertion>()) ||
        (s->s = parseOptional<EmptyStatement>()))
    {
      return s;
    }
    else
    {
      throw ParseErr("invalid statement");
      return s;
    }
  }

  template<>
  UP(Break) parse<Break>()
  {
    expectKeyword(BREAK);
    expectPunct(SEMICOLON);
    return UP(Break)(new Break);
  }

  template<>
  UP(Continue) parse<Continue>()
  {
    expectKeyword(CONTINUE);
    expectPunct(SEMICOLON);
    return UP(Continue)(new Continue);
  }

  template<>
  UP(Typedef) parse<Typedef>()
  {
    cout << "\n******************\n";
    cout << "Parsing typedef...";
    cout << "\n******************\n";
    UP(Typedef) td(new Typedef);
    expectKeyword(TYPEDEF);
    td->type = parse<Type>();
    cout << "Got type.\n";
    td->ident = ((Ident*) expect(IDENTIFIER))->name;
    cout << "Got name: " << td->ident << '\n';
    expectPunct(SEMICOLON);
    return td;
  }

  template<>
  UP(Return) parse<Return>()
  {
    UP(Return) r(new Return);
    expectKeyword(RETURN);
    r->ex = parseOptional<Expression>();
    expectPunct(SEMICOLON);
    return r;
  }

  template<>
  UP(SwitchCase) parse<SwitchCase>()
  {
    UP(SwitchCase) sc(new SwitchCase);
    sc->matchVal = parse<Expression>();
    expectPunct(COLON);
    sc->s = parse<Statement>();
    return sc;
  }

  template<>
  UP(Switch) parse<Switch>()
  {
    UP(Switch) sw(new Switch);
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
  UP(ForC) parse<ForC>()
  {
    //try to parse C style for loop
    UP(ForC) forC(new ForC);
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
  UP(ForRange1) parse<ForRange1>()
  {
    UP(ForRange1) fr1(new ForRange1);
    expectKeyword(FOR);
    fr1->expr = parse<Expression>();
    return fr1;
  }

  template<>
  UP(ForRange2) parse<ForRange2>()
  {
    UP(ForRange2) fr2(new ForRange2);
    expectKeyword(FOR);
    fr2->start = parse<Expression>();
    expectPunct(COLON);
    fr2->end = parse<Expression>();
    return fr2;
  }

  template<>
  UP(ForArray) parse<ForArray>()
  {
    UP(ForArray) fa(new ForArray);
    fa->container = parse<Expression>();
    return fa;
  }

  template<>
  UP(For) parse<For>()
  {
    UP(For) f(new For);
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
      throw ParseErr("invalid for loop");
    }
  }

  template<>
  UP(While) parse<While>()
  {
    UP(While) w(new While);
    expectKeyword(WHILE);
    expectPunct(LPAREN);
    w->cond = parse<Expression>();
    expectPunct(RPAREN);
    w->body = parse<Statement>();
    return w;
  }

  template<>
  UP(If) parse<If>()
  {
    UP(If) i(new If);
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
  UP(Using) parse<Using>()
  {
    UP(Using) u(new Using);
    expectKeyword(USING);
    u->mem = parse<Member>();
    expectPunct(SEMICOLON);
    return u;
  }

  template<>
  UP(Assertion) parse<Assertion>()
  {
    UP(Assertion) a(new Assertion);
    expectKeyword(ASSERT);
    a->expr = parse<Expression>();
    expectPunct(SEMICOLON);
    return a;
  }

  template<>
  UP(TestDecl) parse<TestDecl>()
  {
    UP(TestDecl) t(new TestDecl);
    expectKeyword(TEST);
    t->call = parse<Call>();
    expectPunct(SEMICOLON);
    return t;
  }

  template<>
  UP(EnumItem) parse<EnumItem>()
  {
    UP(EnumItem) ei(new EnumItem);
    ei->name = ((Ident*) expect(IDENTIFIER))->name;
    if(acceptOper(ASSIGN))
    {
      ei->value = (IntLit*) expect(INT_LITERAL);
    }
    return ei;
  }

  template<>
  UP(Enum) parse<Enum>()
  {
    cout << "Parsing enum\n";
    UP(Enum) e(new Enum);
    expectKeyword(ENUM);
    e->name = ((Ident*) expect(IDENTIFIER))->name;
    cout << "Name: " << e->name << '\n';
    expectPunct(LBRACE);
    e->items = parseSomeCommaSeparated<EnumItem>();
    cout << "Got " << e->items.size() << " items.\n";
    expectPunct(RBRACE);
    return e;
  }

  template<>
  UP(Block) parse<Block>()
  {
    UP(Block) b(new Block);
    expectPunct(LBRACE);
    b->statements = parseSome<Statement>();
    expectPunct(RBRACE);
    return b;
  }

  template<>
  UP(VarDecl) parse<VarDecl>()
  {
    UP(VarDecl) vd(new VarDecl);
    if(!acceptKeyword(AUTO))
    {
      vd->type = parse<Type>();
    }
    vd->name = ((Ident*) expect(IDENTIFIER))->name;
    vd->val = parse<Expression>();
    expectPunct(SEMICOLON);
    if(!vd->type && !vd->val)
    {
      throw ParseErr("auto declaration requires initialization");
    }
    return vd;
  }

  template<>
  UP(VarAssign) parse<VarAssign>()
  {
    UP(VarAssign) va(new VarAssign);
    va->target = parse<Member>();
    va->op = (Oper*) expect(OPERATOR);
    //unary assign operators don't have rhs
    if(va->op->getType() != INC && va->op->getType() != DEC)
    {
      va->rhs = parse<Expression>();
    }
    expectPunct(SEMICOLON);
    return va;
  }

  template<>
  UP(Print) parse<Print>()
  {
    UP(Print) p(new Print);
    expectPunct(LPAREN);
    p->exprs = parseSomeCommaSeparated<Expression>();
    expectPunct(RPAREN);
    expectPunct(SEMICOLON);
    return p;
  }

  template<>
  UP(Expression) parse<Expression>()
  {
    UP(Expression) e(new Expression);
    if((e->e = parse<Call>()) ||
        (e->e = parse<Member>()) ||
        (e->e = parse<Expr1>()))
    {
      return e;
    }
    throw ParseErr("invalid expression");
    return e;
  }

  template<>
  UP(Call) parse<Call>()
  {
    UP(Call) c(new Call);
    c->callable = parse<Member>();
    expectPunct(LPAREN);
    c->args = parseSomeCommaSeparated<Expression>();
    expectPunct(RPAREN);
    return c;
  }

  template<>
  UP(Arg) parse<Arg>()
  {
    UP(Arg) a(new Arg);
    if((a->t = parseOptional<Type>()) ||
        (a->t = parseOptional<TraitType>()))
    {
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
    throw ParseErr("invalid argument");
    return a;
  }

  template<>
  UP(FuncDecl) parse<FuncDecl>()
  {
    UP(FuncDecl) fd(new FuncDecl);
    expectKeyword(FUNC);
    fd->retType = parse<Type>();
    fd->name = ((Ident*) expect(IDENTIFIER))->name;
    expectPunct(LPAREN);
    fd->args = parseSomeCommaSeparated<Arg>();
    expectPunct(RPAREN);
    expectPunct(SEMICOLON);
    return fd;
  }

  template<>
  UP(FuncDef) parse<FuncDef>()
  {
    UP(FuncDef) fd(new FuncDef);
    expectKeyword(FUNC);
    fd->retType = parse<Type>();
    fd->name = parse<Member>();
    expectPunct(LPAREN);
    fd->args = parseSomeCommaSeparated<Arg>();
    expectPunct(RPAREN);
    fd->body = parse<Block>();
    return fd;
  }

  template<>
  UP(FuncType) parse<FuncType>()
  {
    UP(FuncType) ft(new FuncType);
    expectKeyword(FUNCTYPE);
    ft->retType = parse<Type>();
    ft->name = parse<Member>();
    expectPunct(LPAREN);
    ft->args = parseSomeCommaSeparated<Arg>();
    expectPunct(RPAREN);
    return ft;
  }

  template<>
  UP(ProcDecl) parse<ProcDecl>()
  {
    UP(ProcDecl) pd(new ProcDecl);
    if(acceptKeyword(NONTERM))
      pd->nonterm = true;
    expectKeyword(PROC);
    pd->retType = parse<Type>();
    pd->name = ((Ident*) expect(IDENTIFIER))->name;
    expectPunct(LPAREN);
    pd->args = parseSomeCommaSeparated<Arg>();
    expectPunct(RPAREN);
    expectPunct(SEMICOLON);
    return pd;
  }
  
  template<>
  UP(ProcDef) parse<ProcDef>()
  {
    UP(ProcDef) pd(new ProcDef);
    if(acceptKeyword(NONTERM))
      pd->nonterm = true;
    expectKeyword(PROC);
    pd->retType = parse<Type>();
    pd->name = parse<Member>();
    expectPunct(LPAREN);
    pd->args = parseSomeCommaSeparated<Arg>();
    expectPunct(RPAREN);
    pd->body = parse<Block>();
    return pd;
  }

  template<>
  UP(ProcType) parse<ProcType>()
  {
    UP(ProcType) pt(new ProcType);
    if(acceptKeyword(NONTERM))
      pt->nonterm = true;
    expectKeyword(PROCTYPE);
    pt->retType = parse<Type>();
    pt->name = parse<Member>();
    expectPunct(LPAREN);
    pt->args = parseSomeCommaSeparated<Arg>();
    expectPunct(RPAREN);
    return pt;
  }

  template<>
  UP(StructMem) parse<StructMem>()
  {
    UP(StructMem) sm(new StructMem);
    if(acceptOper(BXOR))
    {
      sm->compose = true;
    }
    sm->sd = parse<ScopedDecl>();
    return sm;
  }

  template<>
  UP(StructDecl) parse<StructDecl>()
  {
    UP(StructDecl) sd(new StructDecl);
    expectKeyword(STRUCT);
    sd->name = ((Ident*) expect(IDENTIFIER))->name;
    if(acceptPunct(COLON))
    {
      sd->traits = parseSomeCommaSeparated<Member>();
    }
    expectPunct(LBRACE);
    sd->members = parseSomeCommaSeparated<StructMem>();
    expectPunct(RBRACE);
    return sd;
  }

  template<>
  UP(VariantDecl) parse<VariantDecl>()
  {
    UP(VariantDecl) vd(new VariantDecl);
    expectKeyword(VARIANT);
    vd->name = ((Ident*) expect(IDENTIFIER))->name;
    vd->types = parseSomeCommaSeparated<Type>();
    return vd;
  }

  template<>
  UP(TraitDecl) parse<TraitDecl>()
  {
    UP(TraitDecl) td(new TraitDecl);
    expectKeyword(TRAIT);
    td->name = ((Ident*) expect(IDENTIFIER))->name;
    while(true)
    {
      UP(FuncDecl) fd;
      UP(ProcDecl) pd;
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
    return td;
  }

  template<>
  UP(StructLit) parse<StructLit>()
  {
    UP(StructLit) sl(new StructLit);
    expectPunct(LBRACE);
    sl->vals = parseSomeCommaSeparated<Expression>();
    expectPunct(RBRACE);
    return sl;
  }

  template<>
  UP(Member) parse<Member>()
  {
    UP(Member) m(new Member);
    m->owner = ((Ident*) expect(IDENTIFIER))->name;
    if(acceptPunct(DOT))
    {
      m->mem = parse<Member>();
    }
    return m;
  }

  template<>
  UP(TraitType) parse<TraitType>()
  {
    UP(TraitType) tt(new TraitType);
    tt->localName = ((Ident*) expect(IDENTIFIER))->name;
    expectPunct(COLON);
    tt->traitName = parse<Member>();
    return tt;
  }

  template<>
  UP(TupleType) parse<TupleType>()
  {
    UP(TupleType) tt(new TupleType);
    expectPunct(LPAREN);
    tt->members = parseSomeCommaSeparated<Type>();
    expectPunct(RPAREN);
    return tt;
  }

  template<>
  UP(BoolLit) parse<BoolLit>()
  {
    UP(BoolLit) bl(new BoolLit);
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
      throw ParseErr("invalid bool literal");
    }
    return bl;
  }

  template<>
  UP(Expr1) parse<Expr1>()
  {
    UP(Expr1) e1(new Expr1);
    e1->head = parse<Expr2>();
    e1->tail = parseSome<Expr1RHS>();
    return e1;
  }

  template<>
  UP(Expr1RHS) parse<Expr1RHS>()
  {
    UP(Expr1RHS) e1r(new Expr1RHS);
    expectOper(LOR);
    e1r->rhs = parse<Expr2>();
    return e1r;
  }

  template<>
  UP(Expr2) parse<Expr2>()
  {
    UP(Expr2) e2(new Expr2);
    e2->head = parse<Expr3>();
    e2->tail = parseSome<Expr2RHS>();
    return e2;
  }

  template<>
  UP(Expr2RHS) parse<Expr2RHS>()
  {
    UP(Expr2RHS) e2r(new Expr2RHS);
    expectOper(LAND);
    e2r->rhs = parse<Expr3>();
    return e2r;
  }

  template<>
  UP(Expr3) parse<Expr3>()
  {
    UP(Expr3) e3(new Expr3);
    e3->head = parse<Expr4>();
    e3->tail = parseSome<Expr3RHS>();
    return e3;
  }

  template<>
  UP(Expr3RHS) parse<Expr3RHS>()
  {
    UP(Expr3RHS) e3r(new Expr3RHS);
    expectOper(BOR);
    e3r->rhs = parse<Expr4>();
    return e3r;
  }

  template<>
  UP(Expr4) parse<Expr4>()
  {
    UP(Expr4) e4(new Expr4);
    e4->head = parse<Expr5>();
    e4->tail = parseSome<Expr4RHS>();
    return e4;
  }

  template<>
  UP(Expr4RHS) parse<Expr4RHS>()
  {
    UP(Expr4RHS) e4r(new Expr4RHS);
    expectOper(BXOR);
    e4r->rhs = parse<Expr5>();
    return e4r;
  }

  template<>
  UP(Expr5) parse<Expr5>()
  {
    UP(Expr5) e5(new Expr5);
    e5->head = parse<Expr6>();
    e5->tail = parseSome<Expr5RHS>();
    return e5;
  }

  template<>
  UP(Expr5RHS) parse<Expr5RHS>()
  {
    UP(Expr5RHS) e5r(new Expr5RHS);
    expectOper(BAND);
    e5r->rhs = parse<Expr6>();
    return e5r;
  }

  template<>
  UP(Expr6) parse<Expr6>()
  {
    UP(Expr6) e6(new Expr6);
    e6->head = parse<Expr7>();
    e6->tail = parseSome<Expr6RHS>();
    return e6;
  }

  template<>
  UP(Expr6RHS) parse<Expr6RHS>()
  {
    UP(Expr6RHS) e6r(new Expr6RHS);
    e6r->op = ((Oper*) expect(OPERATOR))->op;
    if(e6r->op != CMPEQ && e6r->op != CMPNEQ)
    {
      throw ParseErr("expected == !=");
    }
    e6r->rhs = parse<Expr7>();
    return e6r;
  }

  template<>
  UP(Expr7) parse<Expr7>()
  {
    UP(Expr7) e7(new Expr7);
    e7->head = parse<Expr8>();
    e7->tail = parseSome<Expr7RHS>();
    return e7;
  }

  template<>
  UP(Expr7RHS) parse<Expr7RHS>()
  {
    UP(Expr7RHS) e7r(new Expr7RHS);
    e7r->op = ((Oper*) expect(OPERATOR))->op;
    if(e7r->op != CMPL && e7r->op != CMPLE &&
        e7r->op != CMPG && e7r->op != CMPGE)
    {
      throw ParseErr("expected < > <= >=");
    }
    e7r->rhs = parse<Expr8>();
    return e7r;
  }

  template<>
  UP(Expr8) parse<Expr8>()
  {
    UP(Expr8) e8(new Expr8);
    e8->head = parse<Expr9>();
    e8->tail = parseSome<Expr8RHS>();
    return e8;
  }

  template<>
  UP(Expr8RHS) parse<Expr8RHS>()
  {
    UP(Expr8RHS) e8r(new Expr8RHS);
    e8r->op = ((Oper*) expect(OPERATOR))->op;
    if(e8r->op != SHL && e8r->op != SHR)
    {
      throw ParseErr("expected << >>");
    }
    e8r->rhs = parse<Expr9>();
    return e8r;
  }

  template<>
  UP(Expr9) parse<Expr9>()
  {
    UP(Expr9) e9(new Expr9);
    e9->head = parse<Expr10>();
    e9->tail = parseSome<Expr9RHS>();
    return e9;
  }

  template<>
  UP(Expr9RHS) parse<Expr9RHS>()
  {
    UP(Expr9RHS) e9r(new Expr9RHS);
    e9r->op = ((Oper*) expect(OPERATOR))->op;
    if(e9r->op != PLUS && e9r->op != SUB)
    {
      throw ParseErr("expected + -");
    }
    e9r->rhs = parse<Expr10>();
    return e9r;
  }

  template<>
  UP(Expr10) parse<Expr10>()
  {
    UP(Expr10) e10(new Expr10);
    e10->head = parse<Expr11>();
    e10->tail = parseSome<Expr10RHS>();
    return e10;
  }

  template<>
  UP(Expr10RHS) parse<Expr10RHS>()
  {
    UP(Expr10RHS) e10r(new Expr10RHS);
    e10r->op = ((Oper*) expect(OPERATOR))->op;
    if(e10r->op != MUL && e10r->op != DIV && e10r->op != MOD)
    {
      throw ParseErr("expected * / %");
    }
    e10r->rhs = parse<Expr11>();
    return e10r;
  }

  template<>
  UP(Expr11) parse<Expr11>()
  {
    UP(Expr11) e11(new Expr11);
    if((e11->e = parseOptional<Expr11RHS>()))
    {
      return e11;
    }
    else if((e11->e = parseOptional<Expr12>()))
    {
      return e11;
    }
    else
    {
      throw ParseErr("invalid unary expression");
    }
    return e11;
  }

  template<>
  UP(Expr11RHS) parse<Expr11RHS>()
  {
    UP(Expr11RHS) e11r(new Expr11RHS);
    e11r->op = ((Oper*) expect(OPERATOR))->op;
    // must be SUB, BNOT, or LNOT
    if(e11r->op != SUB && e11r->op != BNOT && e11r->op != LNOT)
      throw ParseErr("expected - ! ~");
    e11r->base = parse<Expr11>();
    return e11r;
  }

  template<>
  UP(Expr12) parse<Expr12>()
  {
    UP(Expr12) e12(new Expr12);
    if((e12->e = (IntLit*) accept(INT_LITERAL)))
      return e12;
    else if((e12->e = (CharLit*) accept(CHAR_LITERAL)))
      return e12;
    else if((e12->e = (StrLit*) accept(STRING_LITERAL)))
      return e12;
    else if((e12->e = (FloatLit*) accept(FLOAT_LITERAL)))
      return e12;
    else if((e12->e = parseOptional<BoolLit>()))
      return e12;
    else if((e12->e = parseOptional<Member>()))
      return e12;
    else if((e12->e = parseOptional<StructLit>()))
      return e12;
    //only other option is (Expr)
    expectPunct(LPAREN);
    e12->e = parse<Expression>();
    expectPunct(RPAREN);
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
    Punct p((PUNC) type);
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
    throw ParseErr(string("expected ") + t.getStr() + " but got " + next->getStr());
  }

  Token* expect(int tokType)
  {
    Token* next = getNext();
    bool res = next->getType() == tokType;
    if(res)
      pos++;
    else
      throw ParseErr(string("expected ") + tokTypeTable[tokType] + " but got " + next->getStr());
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
    Punct p((PUNC) type);
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
    if(msg.length())
      printf("Parse error on line %i, col %i: %s\n", 0, 0, msg.c_str());  //TODO!
    else
      printf("Parse error on line %i, col %i\n", 0, 0);  //TODO!
    exit(1);
  }

  ScopedDecl::ScopedDecl() : decl(none) {}
  Type::Type() : t(none) {}
  Statement::Statement() : s(none) {}
  For::For() : f(none) {}
  Expression::Expression() : e(none) {}
  Arg::Arg() : t(none) {}
  Expr11::Expr11() : e(none) {}
  Expr12::Expr12() : e(none) {}
}

