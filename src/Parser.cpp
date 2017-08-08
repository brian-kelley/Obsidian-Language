#include "Parser.hpp"
#include "AST_PrintNTer.hpp"

struct BlockScope;

namespace Parser
{
  size_t pos;
  vector<Token*>* tokens;

  //Furthest the recursive descent reached, and associated error message if parsing failed there
  size_t deepest = 0;
  string deepestErr;

  template<typename NT>
  NT* parse()
  {
    cout << "FATAL ERROR: non-implemented parse called, for type " << typeid(NT).name() << "\n";
    INTERNAL_ERROR;
    return NULL;
  }

  //Need to forward-declare all parse() specializations
  template<> Module* parse<Module>();
  template<> ScopedDecl* parse<ScopedDecl>();
  template<> TypeNT* parse<TypeNT>();
  template<> StatementNT* parse<StatementNT>();
  template<> Typedef* parse<Typedef>();
  template<> Return* parse<Return>();
  template<> SwitchCase* parse<SwitchCase>();
  template<> Switch* parse<Switch>();
  template<> ForC* parse<ForC>();
  template<> ForRange1* parse<ForRange1>();
  template<> ForRange2* parse<ForRange2>();
  template<> ForArray* parse<ForArray>();
  template<> For* parse<For>();
  template<> While* parse<While>();
  template<> If* parse<If>();
  template<> Assertion* parse<Assertion>();
  template<> TestDecl* parse<TestDecl>();
  template<> EnumItem* parse<EnumItem>();
  template<> Enum* parse<Enum>();
  template<> Block* parse<Block>();
  template<> VarDecl* parse<VarDecl>();
  template<> VarAssign* parse<VarAssign>();
  template<> PrintNT* parse<PrintNT>();
  template<> ExpressionNT* parse<ExpressionNT>();
  template<> CallNT* parse<CallNT>();
  template<> Arg* parse<Arg>();
  template<> FuncDecl* parse<FuncDecl>();
  template<> FuncDef* parse<FuncDef>();
  template<> FuncTypeNT* parse<FuncTypeNT>();
  template<> ProcDecl* parse<ProcDecl>();
  template<> ProcDef* parse<ProcDef>();
  template<> ProcTypeNT* parse<ProcTypeNT>();
  template<> StructMem* parse<StructMem>();
  template<> StructDecl* parse<StructDecl>();
  template<> UnionDecl* parse<UnionDecl>();
  template<> TraitDecl* parse<TraitDecl>();
  template<> StructLit* parse<StructLit>();
  template<> TupleLit* parse<TupleLit>();
  template<> Member* parse<Member>();
  template<> TraitType* parse<TraitType>();
  template<> TupleTypeNT* parse<TupleTypeNT>();
  template<> Expr1* parse<Expr1>();
  template<> Expr1RHS* parse<Expr1RHS>();
  template<> Expr2* parse<Expr2>();
  template<> Expr2RHS* parse<Expr2RHS>();
  template<> Expr3* parse<Expr3>();
  template<> Expr3RHS* parse<Expr3RHS>();
  template<> Expr4* parse<Expr4>();
  template<> Expr4RHS* parse<Expr4RHS>();
  template<> Expr5* parse<Expr5>();
  template<> Expr5RHS* parse<Expr5RHS>();
  template<> Expr6* parse<Expr6>();
  template<> Expr6RHS* parse<Expr6RHS>();
  template<> Expr7* parse<Expr7>();
  template<> Expr7RHS* parse<Expr7RHS>();
  template<> Expr8* parse<Expr8>();
  template<> Expr8RHS* parse<Expr8RHS>();
  template<> Expr9* parse<Expr9>();
  template<> Expr9RHS* parse<Expr9RHS>();
  template<> Expr10* parse<Expr10>();
  template<> Expr10RHS* parse<Expr10RHS>();
  template<> Expr11* parse<Expr11>();
  template<> Expr12* parse<Expr12>();

  Module* parseProgram(vector<Token*>& toks)
  {
    pos = 0;
    tokens = &toks;
    Module* globalModule = new Module;
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
  Module* parse<Module>()
  {
    Module* m = new Module;
    expectKeyword(MODULE);
    m->name = ((Ident*) expect(IDENTIFIER))->name;
    expectPunct(LBRACE);
    m->decls = parseSome<ScopedDecl>();
    expectPunct(RBRACE);
    return m;
  }

  template<>
  ScopedDecl* parse<ScopedDecl>()
  {
    ScopedDecl* sd =new ScopedDecl;
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
  TypeNT* parse<TypeNT>()
  {
    TypeNT* type = new TypeNT;
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
    if(type->t.is<None>())
    {
      if(acceptKeyword(T_TYPE))
      {
        type->t = TypeNT::Wildcard();
      }
      else if(!(type->t = parseOptional<Member>()) &&
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
  StatementNT* parse<StatementNT>()
  {
    StatementNT* s = new StatementNT;
    if((s->s = parseOptional<ScopedDecl>()) ||
        (s->s = parseOptional<VarAssign>()) ||
        (s->s = parseOptional<PrintNT>()) ||
        (s->s = parseOptional<Call>()) ||
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
    {}
    else if((s->s = parseOptional<Call>()))
    {
      //call doesn't include the semicolon
      expectPunct(SEMICOLON);
    }
    else
    {
      err("invalid statement");
    }
    return s;
  }

  template<>
  Break* parse<Break>()
  {
    expectKeyword(BREAK);
    expectPunct(SEMICOLON);
    return new Break;
  }

  template<>
  Continue* parse<Continue>()
  {
    expectKeyword(CONTINUE);
    expectPunct(SEMICOLON);
    return new Continue;
  }

  template<>
  EmptyStatement* parse<EmptyStatement>()
  {
    expectPunct(SEMICOLON);
    return new EmptyStatement;
  }

  template<>
  Typedef* parse<Typedef>()
  {
    Typedef* td = new Typedef;
    expectKeyword(TYPEDEF);
    td->type = parse<TypeNT>();
    td->ident = ((Ident*) expect(IDENTIFIER))->name;
    expectPunct(SEMICOLON);
    return td;
  }

  template<>
  Return* parse<Return>()
  {
    Return* r = new Return;
    expectKeyword(RETURN);
    r->ex = parseOptional<ExpressionNT>();
    expectPunct(SEMICOLON);
    return r;
  }

  template<>
  SwitchCase* parse<SwitchCase>()
  {
    SwitchCase* sc = new SwitchCase;
    sc->matchVal = parse<ExpressionNT>();
    expectPunct(COLON);
    sc->s = parse<StatementNT>();
    return sc;
  }

  template<>
  Switch* parse<Switch>()
  {
    Switch* sw = new Switch;
    expectKeyword(SWITCH);
    expectPunct(LPAREN);
    sw->sw = parse<ExpressionNT>();
    expectPunct(RPAREN);
    expectPunct(LBRACE);
    sw->cases = parseSome<SwitchCase>();
    if(acceptKeyword(DEFAULT))
    {
      expectPunct(COLON);
      sw->defaultStatement = parse<StatementNT>();
      //otherwise, leave defaultStatment NULL
    }
    expectPunct(RBRACE);
    return sw;
  }

  template<>
  ForC* parse<ForC>()
  {
    //try to parse C style for loop
    ForC* forC = new ForC;
    expectKeyword(FOR);
    expectPunct(LPAREN);
    //all 3 parts of the loop are optional
    forC->decl = parseOptional<StatementNT>();
    if(!forC->decl)
    {
      expectPunct(SEMICOLON);
    }
    forC->condition = parseOptional<ExpressionNT>();
    expectPunct(SEMICOLON);
    forC->incr = parseOptional<StatementNT>();
    expectPunct(RPAREN);
    return forC;
  }

  template<>
  ForRange1* parse<ForRange1>()
  {
    ForRange1* fr1 = new ForRange1;
    expectKeyword(FOR);
    fr1->expr = parse<ExpressionNT>();
    return fr1;
  }

  template<>
  ForRange2* parse<ForRange2>()
  {
    ForRange2* fr2 = new ForRange2;
    expectKeyword(FOR);
    fr2->start = parse<ExpressionNT>();
    expectPunct(COLON);
    fr2->end = parse<ExpressionNT>();
    return fr2;
  }

  template<>
  ForArray* parse<ForArray>()
  {
    ForArray* fa = new ForArray;
    fa->container = parse<ExpressionNT>();
    return fa;
  }

  template<>
  For* parse<For>()
  {
    For* f = new For;
    if((f->f = parseOptional<ForC>()) ||
        (f->f = parseOptional<ForRange1>()) ||
        (f->f = parseOptional<ForRange2>()) ||
        (f->f = parseOptional<ForArray>()))
    {
      f->body = parse<StatementNT>();
    }
    else
    {
      err("invalid for loop");
    }
    return f;
  }

  template<>
  While* parse<While>()
  {
    While* w = new While;
    expectKeyword(WHILE);
    expectPunct(LPAREN);
    w->cond = parse<ExpressionNT>();
    expectPunct(RPAREN);
    w->body = parse<StatementNT>();
    return w;
  }

  template<>
  If* parse<If>()
  {
    If* i = new If;
    expectKeyword(IF);
    expectPunct(LPAREN);
    i->cond = parse<ExpressionNT>();
    expectPunct(RPAREN);
    i->ifBody = parse<StatementNT>();
    if(acceptKeyword(ELSE))
      i->elseBody = parse<StatementNT>();
    return i;
  }

  template<>
  Assertion* parse<Assertion>()
  {
    Assertion* a = new Assertion;
    expectKeyword(ASSERT);
    a->expr = parse<ExpressionNT>();
    expectPunct(SEMICOLON);
    return a;
  }

  template<>
  TestDecl* parse<TestDecl>()
  {
    TestDecl* t = new TestDecl;
    expectKeyword(TEST);
    t->call = parse<CallNT>();
    expectPunct(SEMICOLON);
    return t;
  }

  template<>
  EnumItem* parse<EnumItem>()
  {
    EnumItem* ei = new EnumItem;
    ei->name = ((Ident*) expect(IDENTIFIER))->name;
    if(acceptOper(ASSIGN))
      ei->value = (IntLit*) expect(INT_LITERAL);
    else
      ei->value = NULL;
    return ei;
  }

  template<>
  Enum* parse<Enum>()
  {
    Enum* e = new Enum;
    expectKeyword(ENUM);
    e->name = ((Ident*) expect(IDENTIFIER))->name;
    expectPunct(LBRACE);
    e->items = parseSomeCommaSeparated<EnumItem>();
    expectPunct(RBRACE);
    return e;
  }

  template<>
  Block* parse<Block>()
  {
    Block* b = new Block;
    expectPunct(LBRACE);
    b->statements = parseSome<StatementNT>();
    expectPunct(RBRACE);
    bs = nullptr;
    return b;
  }

  template<>
  VarDecl* parse<VarDecl>()
  {
    VarDecl* vd = new VarDecl;
    vd->isStatic = acceptKeyword(STATIC);
    if(!acceptKeyword(AUTO))
    {
      vd->type = parse<TypeNT>();
    }
    vd->name = ((Ident*) expect(IDENTIFIER))->name;
    if(acceptOper(ASSIGN))
    {
      vd->val = parse<ExpressionNT>();
    }
    expectPunct(SEMICOLON);
    if(!vd->type && !vd->val)
    {
      err("auto declaration requires initialization");
    }
    return vd;
  }

  template<>
  VarAssign* parse<VarAssign>()
  {
    VarAssign* va = new VarAssign;
    //need to determine lvalue and rvalue (target and rhs)
    ExpressionNT* target = parse<ExpressionNT>();
    ExpressionNT* rhs = nullptr;
    int otype = ((Oper*) expect(OPERATOR))->op;
    //unary assign operators don't have rhs
    if(otype != INC && otype != DEC)
    {
      rhs = parse<ExpressionNT>();
    }
    if(
        otype != ASSIGN &&
        otype != INC && otype != DEC &&
        otype != PLUSEQ && otype != SUBEQ && otype != MULEQ &&
        otype != DIVEQ && otype != MODEQ && otype != BOREQ &&
        otype != BANDEQ && otype != BXOREQ)
    {
      err("invalid operator for variable assignment/update: " + operatorTable[otype]);
    }
    switch(otype)
    {
      case ASSIGN:
      {
        va->target = target;
        va->rhs = rhs;
        return va;
      }
      case INC:
      case DEC:
      {
        Expr12* oneLit = new Expr12;
        oneLit->e = new IntLit(1);
        //addition and subtraction encoded in Expr10
        Expr9* sum = new Expr9(new Expr12(target));
        Expr9RHS* oneRHS = new Expr9RHS;
        oneRHS->rhs = new Expr10(oneLit);
        if(otype == INC)
          oneRHS->op = PLUS;
        else
          oneRHS->op = SUB;
        sum->tail.push_back(oneRHS);
        va->target = target;
        va->rhs = new ExpressionNT(sum);
        return va;
      }
      case PLUSEQ:
      case SUBEQ:
      {
        Expr9* ex = new Expr9(new Expr12(target));
        Expr9RHS* r = new Expr9RHS;
        if(otype == PLUSEQ)
          r->op = PLUS;
        else
          r->op = SUB;
        r->rhs = new Expr10(new Expr12(rhs));
        ex->tail.push_back(r);
        va->target = target;
        va->rhs = new ExpressionNT(ex);
        return va;
      }
      case MULEQ:
      case DIVEQ:
      case MODEQ:
      {
        Expr10* ex = new Expr10(new Expr12(target));
        Expr10RHS* r = new Expr10RHS;
        if(otype == MULEQ)
          r->op = MUL;
        else if(otype == DIVEQ)
          r->op = DIV;
        else
          r->op = MOD;
        r->rhs = new Expr11(new Expr12(rhs));
        ex->tail.push_back(r);
        va->target = target;
        va->rhs = new ExpressionNT(ex);
        return va;
      }
      case BOREQ:
      {
        Expr3* ex = new Expr3(new Expr12(target));
        Expr3RHS* r = new Expr3RHS;
        r->rhs = new Expr4(new Expr12(rhs));
        ex->tail.push_back(r);
        va->target = target;
        va->rhs = new ExpressionNT(ex);
        return va;
      }
      case BANDEQ:
      {
        Expr5* ex = new Expr5(new Expr12(target));
        Expr5RHS* r = new Expr5RHS;
        r->rhs = new Expr6(new Expr12(rhs));
        ex->tail.push_back(r);
        va->target = target;
        va->rhs = new ExpressionNT(ex);
        return va;
      }
      case BXOREQ:
      {
        Expr4* ex = new Expr4(new Expr12(target));
        Expr4RHS* r = new Expr4RHS;
        r->rhs = new Expr5(new Expr12(rhs));
        ex->tail.push_back(r);
        va->target = target;
        va->rhs = new ExpressionNT(ex);
        return va;
      }
      case SHLEQ:
      case SHREQ:
      {
        Expr8* ex = new Expr8(new Expr12(target));
        Expr8RHS* r = new Expr8RHS;
        r->rhs = new Expr9(new Expr12(rhs));
        if(otype == SHLEQ)
          r->op = SHL;
        else
          r->op = SHR;
        ex->tail.push_back(r);
        va->target = target;
        va->rhs = new ExpressionNT(ex);
        return va;
      }
      default: INTERNAL_ERROR;
    }
    //like all statements, must be terminated with semicolon
    expectPunct(SEMICOLON);
    return va;
  }

  template<>
  PrintNT* parse<PrintNT>()
  {
    PrintNT* p = new PrintNT;
    expectKeyword(PRINT);
    expectPunct(LPAREN);
    p->exprs = parseSomeCommaSeparated<ExpressionNT>();
    expectPunct(RPAREN);
    expectPunct(SEMICOLON);
    return p;
  }

  template<>
  CallNT* parse<CallNT>()
  {
    CallNT* c = new CallNT;
    c->callable = parse<Member>();
    expectPunct(LPAREN);
    c->args = parseSomeCommaSeparated<ExpressionNT>();
    expectPunct(RPAREN);
    return c;
  }

  template<>
  Arg* parse<Arg>()
  {
    Arg* a = new Arg;
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
  FuncDecl* parse<FuncDecl>()
  {
    FuncDecl* fd = new FuncDecl;
    expectKeyword(FUNC);
    fd->type.retType = parse<TypeNT>();
    fd->name = ((Ident*) expect(IDENTIFIER))->name;
    expectPunct(LPAREN);
    fd->type.args = parseSomeCommaSeparated<Arg>();
    expectPunct(RPAREN);
    expectPunct(SEMICOLON);
    return fd;
  }

  template<>
  FuncDef* parse<FuncDef>()
  {
    FuncDef* fd = new FuncDef;
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
  FuncTypeNT* parse<FuncTypeNT>()
  {
    FuncTypeNT* ft = new FuncTypeNT;
    expectKeyword(FUNCTYPE);
    ft->retType = parse<TypeNT>();
    expectPunct(LPAREN);
    ft->args = parseSomeCommaSeparated<Arg>();
    expectPunct(RPAREN);
    return ft;
  }

  template<>
  ProcDecl* parse<ProcDecl>()
  {
    ProcDecl* pd = new ProcDecl;
    if(acceptKeyword(NONTERM))
      pd->type.nonterm = true;
    expectKeyword(PROC);
    pd->type.retType = parse<TypeNT>();
    pd->name = ((Ident*) expect(IDENTIFIER))->name;
    expectPunct(LPAREN);
    pd->type.args = parseSomeCommaSeparated<Arg>();
    expectPunct(RPAREN);
    expectPunct(SEMICOLON);
    return pd;
  }
  
  template<>
  ProcDef* parse<ProcDef>()
  {
    ProcDef* pd = new ProcDef;
    if(acceptKeyword(NONTERM))
      pd->type.nonterm = true;
    expectKeyword(PROC);
    pd->type.retType = parse<TypeNT>();
    pd->name = parse<Member>();
    expectPunct(LPAREN);
    pd->type.args = parseSomeCommaSeparated<Arg>();
    expectPunct(RPAREN);
    pd->body = parse<Block>();
    return pd;
  }

  template<>
  ProcTypeNT* parse<ProcTypeNT>()
  {
    ProcTypeNT* pt = new ProcTypeNT;
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
  StructMem* parse<StructMem>()
  {
    StructMem* sm = new StructMem;
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
  StructDecl* parse<StructDecl>()
  {
    StructDecl* sd = new StructDecl;
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
  UnionDecl* parse<UnionDecl>()
  {
    UnionDecl* vd = new UnionDecl;
    expectKeyword(UNION);
    vd->name = ((Ident*) expect(IDENTIFIER))->name;
    vd->types = parseSomeCommaSeparated<TypeNT>();
    return vd;
  }

  template<>
  TraitDecl* parse<TraitDecl>()
  {
    TraitDecl* td = new TraitDecl;
    expectKeyword(TRAIT);
    td->name = ((Ident*) expect(IDENTIFIER))->name;
    expectPunct(LBRACE);
    while(true)
    {
      FuncDecl* fd;
      ProcDecl* pd;
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
    return td;
  }

  template<>
  StructLit* parse<StructLit>()
  {
    StructLit* sl = new StructLit;
    expectPunct(LBRACE);
    sl->vals = parseSomeCommaSeparated<ExpressionNT>();
    expectPunct(RBRACE);
    return sl;
  }

  template<>
  TupleLit* parse<TupleLit>()
  {
    TupleLit* sl = new TupleLit;
    expectPunct(LPAREN);
    sl->vals = parseSomeCommaSeparated<ExpressionNT>();
    //this check avoids ambiguity with the "Expr12 := ( Expression )" rule
    if(sl->vals.size() < 2)
    {
      err("Tuple literal must have at least 2 values (a singleton "
          "is exactly equivalent to its value without parentheses)");
    }
    expectPunct(RPAREN);
    return sl;
  }

  template<>
  Member* parse<Member>()
  {
    Member* m = new Member;
    //get a list of all strings separated by dots
    m->scopes.push_back(((Ident*) expect(IDENTIFIER))->name);
    while(acceptPunct(DOT))
    {
      m->scopes.push_back(((Ident*) expect(IDENTIFIER))->name);
    }
    //the last item in scopes is the ident
    //scopes must have size >= 1
    m->ident = m->scopes.back();
    m->scopes.pop_back();
    return m;
  }

  template<>
  TraitType* parse<TraitType>()
  {
    TraitType* tt = new TraitType;
    tt->localName = ((Ident*) expect(IDENTIFIER))->name;
    expectPunct(COLON);
    tt->traits = parseSomeCommaSeparated<Member>();
    return tt;
  }

  template<>
  TupleTypeNT* parse<TupleTypeNT>()
  {
    TupleTypeNT* tt = new TupleTypeNT;
    expectPunct(LPAREN);
    tt->members = parseSomeCommaSeparated<TypeNT>();
    expectPunct(RPAREN);
    return tt;
  }

  template<>
  BoolLit* parse<BoolLit>()
  {
    BoolLit* bl = new BoolLit;
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

  Expr1::Expr1(Expr2* e)
  {
    head = e;
  }

  Expr1::Expr1(Expr3* e)
  {
    head = new Expr2(e);
  }

  Expr1::Expr1(Expr4* e)
  {
    head = new Expr2(new Expr3(e));
  }

  Expr1::Expr1(Expr5* e)
  {
    head = new Expr2(new Expr3(new Expr4(e)));
  }

  Expr1::Expr1(Expr6* e)
  {
    head = new Expr2(new Expr3(new Expr4(new Expr5(e))));
  }

  Expr1::Expr1(Expr7* e)
  {
    head = new Expr2(new Expr3(new Expr4(new Expr5(new Expr6(e)))));
  }

  Expr1::Expr1(Expr8* e)
  {
    head = new Expr2(new Expr3(new Expr4(new Expr5(new Expr6(new Expr7(e))))));
  }

  Expr1::Expr1(Expr9* e)
  {
    head = new Expr2(new Expr3(new Expr4(new Expr5(new Expr6(new Expr7(new Expr8(e)))))));
  }

  Expr1::Expr1(Expr10* e)
  {
    head = new Expr2(new Expr3(new Expr4(new Expr5(new Expr6(new Expr7(new Expr8(new Expr9(e))))))));
  }

  Expr1::Expr1(Expr11* e)
  {
    head = new Expr2(new Expr3(new Expr4(new Expr5(new Expr6(new Expr7( new Expr8(new Expr9(new Expr10(e)))))))));
  }

  Expr1::Expr1(Expr12* e12)
  {
    head = new Expr2(e12);
  }

  Expr2::Expr2(Expr12* e12)
  {
    head = new Expr3(e12);
  }

  Expr3::Expr3(Expr12* e12)
  {
    head = new Expr4(e12);
  }

  Expr4::Expr4(Expr12* e12)
  {
    head = new Expr5(e12);
  }

  Expr5::Expr5(Expr12* e12)
  {
    head = new Expr6(e12);
  }

  Expr6::Expr6(Expr12* e12)
  {
    head = new Expr7(e12);
  }

  Expr7::Expr7(Expr12* e12)
  {
    head = new Expr8(e12);
  }

  Expr8::Expr8(Expr12* e12)
  {
    head = new Expr9(e12);
  }

  Expr9::Expr9(Expr12* e12)
  {
    head = new Expr10(e12);
  }

  Expr10::Expr10(Expr12* e12)
  {
    head = new Expr11(e12);
  }

  Expr11::Expr11(Expr12* e12)
  {
    e = e12;
  }

  template<>
  Expr1* parse<Expr1>()
  {
    Expr1* e1 = new Expr1;
    e1->head = parse<Expr2>();
    e1->tail = parseSome<Expr1RHS>();
    return e1;
  }

  template<>
  Expr1RHS* parse<Expr1RHS>()
  {
    Expr1RHS* e1r =new Expr1RHS;
    expectOper(LOR);
    e1r->rhs = parse<Expr2>();
    return e1r;
  }

  template<>
  Expr2* parse<Expr2>()
  {
    Expr2* e2 = new Expr2;
    e2->head = parse<Expr3>();
    e2->tail = parseSome<Expr2RHS>();
    return e2;
  }

  template<>
  Expr2RHS* parse<Expr2RHS>()
  {
    Expr2RHS* e2r = new Expr2RHS;
    expectOper(LAND);
    e2r->rhs = parse<Expr3>();
    return e2r;
  }

  template<>
  Expr3* parse<Expr3>()
  {
    Expr3* e3 = new Expr3;
    e3->head = parse<Expr4>();
    e3->tail = parseSome<Expr3RHS>();
    return e3;
  }

  template<>
  Expr3RHS* parse<Expr3RHS>()
  {
    Expr3RHS* e3r = new Expr3RHS;
    expectOper(BOR);
    e3r->rhs = parse<Expr4>();
    return e3r;
  }

  template<>
  Expr4* parse<Expr4>()
  {
    Expr4* e4 = new Expr4;
    e4->head = parse<Expr5>();
    e4->tail = parseSome<Expr4RHS>();
    return e4;
  }

  template<>
  Expr4RHS* parse<Expr4RHS>()
  {
    Expr4RHS* e4r = new Expr4RHS;
    expectOper(BXOR);
    e4r->rhs = parse<Expr5>();
    return e4r;
  }

  template<>
  Expr5* parse<Expr5>()
  {
    Expr5* e5 = new Expr5;
    e5->head = parse<Expr6>();
    e5->tail = parseSome<Expr5RHS>();
    return e5;
  }

  template<>
  Expr5RHS* parse<Expr5RHS>()
  {
    Expr5RHS* e5r = new Expr5RHS;
    expectOper(BAND);
    e5r->rhs = parse<Expr6>();
    return e5r;
  }

  template<>
  Expr6* parse<Expr6>()
  {
    Expr6* e6 = new Expr6;
    e6->head = parse<Expr7>();
    e6->tail = parseSome<Expr6RHS>();
    return e6;
  }

  template<>
  Expr6RHS* parse<Expr6RHS>()
  {
    Expr6RHS* e6r = new Expr6RHS;
    e6r->op = ((Oper*) expect(OPERATOR))->op;
    if(e6r->op != CMPEQ && e6r->op != CMPNEQ)
    {
      err("expected == !=");
    }
    e6r->rhs = parse<Expr7>();
    return e6r;
  }

  template<>
  Expr7* parse<Expr7>()
  {
    Expr7* e7 = new Expr7;
    e7->head = parse<Expr8>();
    e7->tail = parseSome<Expr7RHS>();
    return e7;
  }

  template<>
  Expr7RHS* parse<Expr7RHS>()
  {
    Expr7RHS* e7r = new Expr7RHS;
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
  Expr8* parse<Expr8>()
  {
    Expr8* e8 = new Expr8;
    e8->head = parse<Expr9>();
    e8->tail = parseSome<Expr8RHS>();
    return e8;
  }

  template<>
  Expr8RHS* parse<Expr8RHS>()
  {
    Expr8RHS* e8r = new Expr8RHS;
    e8r->op = ((Oper*) expect(OPERATOR))->op;
    if(e8r->op != SHL && e8r->op != SHR)
    {
      err("expected << >>");
    }
    e8r->rhs = parse<Expr9>();
    return e8r;
  }

  template<>
  Expr9* parse<Expr9>()
  {
    Expr9* e9 = new Expr9;
    e9->head = parse<Expr10>();
    e9->tail = parseSome<Expr9RHS>();
    return e9;
  }

  template<>
  Expr9RHS* parse<Expr9RHS>()
  {
    Expr9RHS* e9r = new Expr9RHS;
    e9r->op = ((Oper*) expect(OPERATOR))->op;
    if(e9r->op != PLUS && e9r->op != SUB)
    {
      err("expected + -");
    }
    e9r->rhs = parse<Expr10>();
    return e9r;
  }

  template<>
  Expr10* parse<Expr10>()
  {
    Expr10* e10 = new Expr10;
    e10->head = parse<Expr11>();
    e10->tail = parseSome<Expr10RHS>();
    return e10;
  }

  template<>
  Expr10RHS* parse<Expr10RHS>()
  {
    Expr10RHS* e10r = new Expr10RHS;
    e10r->op = ((Oper*) expect(OPERATOR))->op;
    if(e10r->op != MUL && e10r->op != DIV && e10r->op != MOD)
    {
      err("expected * / % but got " + operatorTable[e10r->op]);
    }
    e10r->rhs = parse<Expr11>();
    return e10r;
  }

  template<>
  Expr11* parse<Expr11>()
  {
    Expr11* e11 = new Expr11;
    Oper* oper = (Oper*) accept(OPERATOR);
    if(oper)
    {
      if(oper->op != SUB && oper->op != LNOT && oper->op != BNOT)
      {
        ostringstream oss;
        oss << '\"' << oper->getStr() << "\" is an invalid unary operator.";
        err(oss.str());
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
  Expr12* parse<Expr12>()
  {
    Expr12* e12 = new Expr12;
    e12->e = parseOptional<TupleLit>();
    if(acceptPunct(LPAREN))
    {
      //any expression inside parentheses
      e12->e = parse<ExpressionNT>();
      expectPunct(RPAREN);
    }
    else if(
        !(e12->e = (IntLit*) accept(INT_LITERAL)) &&
        !(e12->e = (CharLit*) accept(CHAR_LITERAL)) &&
        !(e12->e = (StrLit*) accept(STRING_LITERAL)) &&
        !(e12->e = (FloatLit*) accept(FLOAT_LITERAL)) &&
        !(e12->e = parseOptional<BoolLit>()) &&
        !(e12->e = parseOptional<Member>()) &&
        !(e12->e = parseOptional<StructLit>()) &&
        !(e12->e = parseOptional<CallNT>()))
    {
      err("invalid expression");
    }
    //check for array indexing
    if(acceptPunct(LBRACKET))
    {
      Expr12::ArrayIndex ai;
      //previously parsed expr12 is the array/tuple expression
      ai.arr = e12;
      ai.index = parse<ExpressionNT>();
      Expr12* outer =new Expr12;
      outer->e = ai;
      expectPunct(RBRACKET);
      return outer;
    }
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
}

ostream& operator<<(ostream& os, const Parser::Member& mem)
{
  for(auto s : mem.scopes)
  {
    os << s << '.';
  }
  os << mem.ident;
  return os;
}

