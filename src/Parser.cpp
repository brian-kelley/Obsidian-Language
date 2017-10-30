#include "Parser.hpp"
#include "AST_Printer.hpp"

struct BlockScope;

namespace Parser
{
  size_t pos;
  vector<Token*>* tokens;

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
  template<> CallOp* parse<CallOp>();
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
  template<> NewArrayNT* parse<NewArrayNT>();

  Module* parseProgram(vector<Token*>& toks)
  {
    pos = 0;
    tokens = &toks;
    Module* globalModule = new Module;
    globalModule->name = "";
    globalModule->decls = parseStar<ScopedDecl>(PastEOF());
    if(pos != tokens->size())
    {
      //If not all tokens were used, there was a parse error
      //print the deepest error message produced
      ERR_MSG(deepestErr);
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
    m->decls = parseStar<ScopedDecl>(Punct(RBRACE));
    return m;
  }

  template<>
  ScopedDecl* parse<ScopedDecl>()
  {
    ScopedDecl* sd = new ScopedDecl;
    //peek at next token to check for keywords that start decls
    //use short-circuit evaluation to find the pattern that parses successfully
    Keyword* nextKeyword = (Keyword*) accept(KEYWORD);
    if(nextKeyword)
    {
      accept();
      bool found = true;
      switch(nextKeyword->kw)
      {
        case MODULE:
          sd->decl = parse<Module>(); break;
        case STRUCT:
          sd->decl = parse<StructDecl>(); break;
        case UNION:
          sd->decl = parse<UnionDecl>(); break;
        case TRAIT:
          sd->decl = parse<TraitDecl>(); break;
        case ENUM:
          sd->decl = parse<Enum>(); break;
        case TYPEDEF:
          sd->decl = parse<Typedef>(); break;
        case FUNC:
          sd->decl = parse<FuncDef>(); break;
        case PROC:
          sd->decl = parse<ProcDef>(); break;
        default: found = false;
      }
      if(found)
        return sd;
    }
    //only other possibility is VarDecl
    sd->decl = parse<VarDecl>();
    return sd;
  }

  //Parse a ScopedDecl, given that it begins with a Member
  //The only rule that works is VarDecl, where Member is the type
  //This is called by parse<StatementNT>()
  ScopedDecl* parseScopedDeclGivenMember(mem)
  {
    ScopedDecl* sd = new ScopedDecl;
    VarDecl* vd = new VarDecl;
    TypeNT* type = new TypeNT;
    type->t = mem;
    vd->type = type;
    if(acceptOper(ASSIGN))
    {
      //vd has an rhs expression
      vd->val = parse<ExpressionNT>();
    }
    else
    {
      vd->val = nullptr;
    }
    sd->decl = vd;
    expectPunct(SEMICOLON);
    return sd;
  }

  template<>
  TypeNT* parse<TypeNT>()
  {
    TypeNT* type = new TypeNT;
    type->arrayDims = 0;
    //check for keyword
    Keyword* keyword = (Keyword*) accept(KEYWORD);
    bool found = true;
    if(keyword)
    {
      switch(keyword->kw)
      {
        case BOOL:
          type->t = TypeNT::Prim::BOOL; break;
        case CHAR:
          type->t = TypeNT::Prim::CHAR; break;
        case BYTE:
          type->t = TypeNT::Prim::BYTE; break;
        case SHORT:
          type->t = TypeNT::Prim::SHORT; break;
        case USHORT:
          type->t = TypeNT::Prim::USHORT; break;
        case INT:
          type->t = TypeNT::Prim::INT; break;
        case UINT:
          type->t = TypeNT::Prim::UINT; break;
        case LONG:
          type->t = TypeNT::Prim::LONG; break;
        case ULONG:
          type->t = TypeNT::Prim::ULONG; break;
        case FLOAT:
          type->t = TypeNT::Prim::FLOAT; break;
        case DOUBLE:
          type->t = TypeNT::Prim::DOUBLE; break;
        case VOID:
          type->t = TypeNT::Prim::VOID; break;
        case FUNCTYPE:
          unget();
          type->t = (SubroutineTypeNT*) parse<FuncTypeNT>(); break;
        case PROCTYPE:
          unget();
          type->t = (SubroutineTypeNT*) parse<ProcTypeNT>(); break;
        case TTYPE:
          type->t = TypeNT::TTypeNT; break;
        default:
          err("expected type");
      }
    }
    else if(acceptPunct(LPAREN))
    {
      unget();
      type->t = parse<TupleTypeNT>();
    }
    else if(lookAhead()->getType() == IDENTIFIER)
    {
      //must be a member
      type->t = parse<Member>();
    }
    else
    {
      //unexpected token type
      err("expected type");
    }
    //check for square bracket pairs after, indicating array type
    while(acceptPunct(LBRACKET))
    {
      expectPunct(RBRACKET);
      type->arrayDims++;
    }
    return type;
  }

  Block* parseBlockWrappedStatement()
  {
    StatementNT* s = parse<StatementNT>();
    Block* b = new Block;
    b->statements.push_back(s);
    return b;
  }

  template<>
  StatementNT* parse<StatementNT>()
  {
    //Get some possibilities for the next token
    Token* next = lookAhead();
    Keyword* keyword = dynamic_cast<Keyword*>(next);
    Punct* punct = dynamic_cast<Punct*>(next);
    Ident* ident = dynamic_cast<Ident*>(next);
    StatementNT* s = new StatementNT;
    if(keyword)
    {
      switch(keyword->kw)
      {
        case PRINT:
          s->s = parse<PrintNT>(); break;
        case RETURN:
          s->s = parse<Return>(); break;
        case CONTINUE:
          s->s = parse<Continue>(); break;
        case BREAK:
          s->s = parse<Break>(); break;
        case SWITCH:
          s->s = parse<Switch>(); break;
        case FOR:
          s->s = parse<For>(); break;
        case WHILE:
          s->s = parse<While>(); break;
        case IF:
          s->s = parse<If>(); break;
        case ASSERTION:
          s->s = parse<Assertion>(); break;
        default: err("expected statement");
      }
      return s;
    }
    else if(punct && punct->val == LBRACE)
    {
      s->s = parse<Block>();
      return s;
    }
    //at this point, need to distinguish between ScopedDecl, VarAssign and Call (as Expr12)
    //All 3 can begin as Member
    //If next token is Ident, parse a whole Member
    //  If token after that is also Ident, is a ScopedDecl
    //Otherwise, definitely not a ScopedDecl
    //  Parse an Expr12
    //  If next token is '=', is a VarAssign
    //  Otherwise, must be a Call (check tail)
    Expr12* leadingExpr12;
    if(ident)
    {
      //this must succeed when starting with an Ident
      Member* mem = parse<Member>();
      //now peek at next token
      Token* afterMem = lookAhead(0);
      Oper* amOp = dynamic_cast<Oper*>(afterMem);
      Punct* amPunct = dynamic_cast<Punct*>(afterMem);
      if(afterMem->type == IDENTIFIER)
      {
        //definitely have a VarDecl
        //parse it, starting with the member as a type
        s->s = parseScopedDeclGivenMember(mem);
        return s;
      }
      else
      {
        //parse Expr12 using member
        leadingExpr12 = parseExpr12GivenMember(mem);
      }
    }
    else
    {
      //Don't have a member, but parse an Expr12
      leadingExpr12 = parse<Expr12>();
    }
    //if '=' after Expr12, is an assign
    //if Expr12 tail back is CallOp, is a call
    //otherwise is an error
    if(acceptOper(ASSIGN))
    {
      ExpressionNT* rhs = parse<ExpressionNT>();
      VarAssign* va = new VarAssign;
      va->target = leadingExpr12;
      va->rhs = rhs;
      s->s = va;
      return s;
    }
    else if(leadingExpr12->tail.size() && leadingExpr12->tail.back()->e.is<CallOp*>())
    {
      //call
      s->s = leadingExpr12;
      return s;
    }
    //parsing leadingExpr12 must have succeeded, so give
    //error message knowing that there was some valid expression there
    err("expected call or assignment, but got some other expression");
    return nullptr;
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
    //parse cases until either default or rbrace is found
    Keyword defaultKW(DEFAULT);
    Punct rbrace(RBRACE);
    while(*lookAhead() != defaultKW && *lookAhead() != rbrace)
    {
      sw->cases.push_back(parse<SwitchCase>());
    }
    if(acceptKeyword(DEFAULT))
    {
      expectPunct(COLON);
      sw->defaultStatement = parse<StatementNT>();
    }
    else
    {
      sw->defaultStatement = nullptr;
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
  For* parse<For>()
  {
    For* f = new For;
    if((f->f = parseOptional<ForC>()) ||
        (f->f = parseOptional<ForRange1>()) ||
        (f->f = parseOptional<ForRange2>()))
    {
      f->body = parseBlockWrappedStatement();
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
    w->body = parseBlockWrappedStatement();
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
    //test statement is executed, and the test
    //passes if no assertions fail
    t->stmt = parse<StatementNT*>();
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
    e->items = parsePlusComma<EnumItem>();
    expectPunct(RBRACE);
    return e;
  }

  template<>
  Block* parse<Block>()
  {
    Block* b = new Block;
    expectPunct(LBRACE);
    Punct rbrace(RBRACE);
    b->statements = parseStar<StatementNT>(rbrace);
    b->bs = nullptr;
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
    expectPunct(SEMICOLON);
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
    p->exprs = parsePlusComma<ExpressionNT>();
    expectPunct(RPAREN);
    expectPunct(SEMICOLON);
    return p;
  }

  template<>
  Arg* parse<Arg>()
  {
    Arg* a = new Arg;
    a->matched = acceptOper(MATCH);
    a->expr = parse<ExpressionNT>();
    return a;
  }

  template<>
  Parameter* parse<Parameter>()
  {
    Parameter* p = new Parameter;
    p->type = parse<TypeNT>();
    p->name = (Ident*) expect(IDENTIFIER);
    return p;
  }

  template<>
  FuncDecl* parse<FuncDecl>()
  {
    FuncDecl* fd = new FuncDecl;
    expectKeyword(FUNC);
    fd->isStatic = false;
    if(acceptKeyword(STATIC))
      fd->isStatic = true;
    fd->type.retType = parse<TypeNT>();
    fd->name = ((Ident*) expect(IDENTIFIER))->name;
    expectPunct(LPAREN);
    Punct rparen(RPAREN);
    fd->type.args = parseStarComma<Parameter>();
    expectPunct(RPAREN);
    expectPunct(SEMICOLON);
    return fd;
  }

  template<>
  FuncDef* parse<FuncDef>()
  {
    FuncDef* fd = new FuncDef;
    expectKeyword(FUNC);
    fd->type.isStatic = false;
    if(acceptKeyword(STATIC))
      fd->type.isStatic = true;
    fd->type.retType = parse<TypeNT>();
    fd->name = ((Ident*) expect(IDENTIFIER))->name;
    expectPunct(LPAREN);
    Punct rparen(RPAREN);
    fd->type.args = parseStarComma<Parameter>(rparen);
    fd->body = parse<Block>();
    return fd;
  }

  template<>
  FuncTypeNT* parse<FuncTypeNT>()
  {
    FuncTypeNT* ft = new FuncTypeNT;
    expectKeyword(FUNCTYPE);
    ft->isStatic = false;
    if(acceptKeyword(STATIC))
      ft->isStatic = true;
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
    pd->isStatic = false;
    if(acceptKeyword(STATIC))
      pd->isStatic = true;
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
    pd->type.isStatic = false;
    if(acceptKeyword(STATIC))
      pd->type.isStatic = true;
    pd->type.retType = parse<TypeNT>();
    pd->name = ((Ident*) expect(IDENTIFIER))->name;
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
    pt->isStatic = false;
    if(acceptKeyword(STATIC))
      pt->isStatic = true;
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
    expectPunct(LBRACE);
    vd->types = parseSomeCommaSeparated<TypeNT>();
    expectPunct(RBRACE);
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
    expectPunct(LBRACKET);
    sl->vals = parseSomeCommaSeparated<ExpressionNT>();
    expectPunct(RBRACKET);
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
    // <ident> : <trait1, trait2, ... traitN>
    // Given the ':', there must be one or more trait names
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
    if(acceptKeyword(ARRAY))
    {
      e1->e = parse<NewArrayNT*>();
      //no tail for NewArrayNT (not compatible with any operands)
    }
    else
    {
      e1->e.head = parse<Expr2>();
      //check whether parse<Expr1RHS>() should succeed
      Token* next = lookAhead();
      while(next->type == OPERATOR && ((Oper*) next)->op == LOR)
      {
        e1->tail.push_back(parse<Expr1RHS>());
        next = lookAhead();
      }
    }
    return e1;
  }

  template<>
  Expr1RHS* parse<Expr1RHS>()
  {
    Expr1RHS* e1r = new Expr1RHS;
    expectOper(LOR);
    e1r->rhs = parse<Expr2>();
    return e1r;
  }

  template<>
  Expr2* parse<Expr2>()
  {
    //
    Expr2* e2 = new Expr2;
    Token* next = lookAhead();
    while(next->type == OPERATOR && ((Oper*) next)->op == LAND)
    {
      e2->tail.push_back(parse<Expr2RHS>());
      next = lookAhead();
    }
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
    Token* next = lookAhead();
    while(next->type == OPERATOR && ((Oper*) next)->op == BOR)
    {
      e3->tail.push_back(parse<Expr3RHS>());
      next = lookAhead();
    }
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
    Token* next = lookAhead();
    while(next->type == OPERATOR && ((Oper*) next)->op == BXOR)
    {
      e4->tail.push_back(parse<Expr4RHS>());
      next = lookAhead();
    }
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
    Token* next = lookAhead();
    while(next->type == OPERATOR && ((Oper*) next)->op == BAND)
    {
      e5->tail.push_back(parse<Expr5RHS>());
      next = lookAhead();
    }
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
    Token* next = lookAhead();
    while(next->type == OPERATOR && (((Oper*) next)->op == CMPEQ || ((Oper*) next)->op == CMPNEQ))
    {
      e6->tail.push_back(parse<Expr6RHS>());
      next = lookAhead();
    }
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
    Oper* next = dynamic_cast<Oper*>(lookAhead());
    while(next && (next->op == CMPL || next->op == CMPLE || next->op == CMPG || next->op == CMPGE))
    {
      e7->tail.push_back(parse<Expr7RHS>());
      next = dynamic_cast<Oper*>(lookAhead());
    }
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
    Oper* next = dynamic_cast<Oper*>(lookAhead());
    while(next && (next->op == SHL || next->op == SHR))
    {
      e8->tail.push_back(parse<Expr8RHS>());
      next = dynamic_cast<Oper*>(lookAhead());
    }
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
    Oper* next = dynamic_cast<Oper*>(lookAhead());
    while(next && (next->op == PLUS || next->op == SUB))
    {
      e9->tail.push_back(parse<Expr9RHS>());
      next = dynamic_cast<Oper*>(lookAhead());
    }
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
    Oper* next = dynamic_cast<Oper*>(lookAhead());
    while(next && (next->op == MUL || next->op == DIV || next->op == MOD))
    {
      e10->tail.push_back(parse<Expr10RHS>());
      next = dynamic_cast<Oper*>(lookAhead());
    }
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
    Oper* oper = accept(OPERATOR);
    if(oper)
    {
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
    Token* next = lookAhead(0);
    switch(next->type)
    {
      case PUNCTUATION:
      {
        Punct* punct = (Punct*) next;
        if(next->val == LPAREN)
        {
          //expression in parentheses
          accept();
          e12->e = parse<ExpressionNT>();
          expectPunct(RPAREN);
        }
        else if(next->val == LBRACKET)
        {
          //struct lit
          e12->e = parse<StructLit>();
        }
        else
        {
          //nothing else valid
          err("invalid punctuation in expression");
        }
        break;
      }
      case KEYWORD:
      {
        Keyword* kw = (Keyword*) next;
        if(kw->kw == TRUE)
          e12->e = new BoolLit(true);
        else if(kw->kw == FALSE)
          e12->e = new BoolLit(true);
        else
          err("invalid keyword in expression");
        break;
      }
      case INT_LITERAL:
        e12->e = accept(INT_LITERAL);
        break;
      case FLOAT_LITERAL:
        e12->e = accept(FLOAT_LITERAL);
        break;
      case CHAR_LITERAL:
        e12->e = accept(CHAR_LITERAL);
        break;
      case STRING_LITERAL:
        e12->e = accept(STRING_LITERAL);
        break;
      case IDENTIFIER:
        e12->e = parse<Member*>();
        break;
      default: err("unexpected token in expression");
    }
    parseExpr12Tail(e12);
    return e12;
  }

  void parseExpr12Tail(Expr12* head)
  {
    Punct lparen(LPAREN);
    while(true)
    {
      if(acceptPunct(LBRACKET))
      {
        //"[ Expr ]"
        e12->tail.push_back(new Expr12RHS);
        e12->tail.back()->e = parse<ExpressionNT>();
      }
      else if(acceptPunct(DOT))
      {
        //". Ident"
        e12->tail.push_back(new Expr12RHS);
        e12->tail.back()->e = (Ident*) expect(IDENTIFIER);
      }
      else if(lookAhead()->compareTo(&lparen))
      {
        //"( Args )" - aka a CallOp
        e12->tail.push_back(new Expr12RHS);
        e12->tail.back()->e = parse<CallOp>();
      }
    }
  }

  Expr12* parseExpr12GivenMember(Member* mem)
  {
    //first, consume the member by using a chain of "member" operators
    Expr12* e12 = new Expr12;
    if(mem->head.size())
    {
      e12->e = head[0];
      for(size_t i = 1; i < mem->head.size(); i++)
      {
        e12->tail.push_back(new Expr12RHS);
        e12->tail.back()->e = mem->head[i];
      }
    }
    e12->tail.push_back(new Expr12RHS);
    e12->tail.back()->e = mem->tail;
    parseExpr12Tail(e12);
  }

  template<>
  CallOp* parse<CallOp>()
  {
    CallOp* co = new CallOp;
    expectPunct(LPAREN);
    Punct term(RPAREN);
    co->args = parseStarComma<Arg>(term);
    return co;
  }

  template<>
  NewArrayNT* parse<NewArrayNT>()
  {
    expectKeyword(keywordMap["array"]);
    NewArrayNT* na = new NewArrayNT;
    na->elemType = parse<TypeNT>();
    //check that elem type isn't itself an array type
    if(na->elemType->arrayDims > 0)
    {
      err("can't create an array of arrays (all dimensions must be specified)");
    }
    int dims = 0;
    while(acceptPunct(LBRACKET))
    {
      na->dimensions.push_back(parse<ExpressionNT>());
      expectPunct(RBRACKET);
      dims++;
    }
    return na;
  }

  void accept()
  {
    pos++;
  }

  bool accept(Token& t)
  {
    bool res = lookAhead()->compareTo(&t);
    if(res)
      pos++;
    return res;
  }

  Token* accept(int tokType)
  {
    Token* next = lookAhead();
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
    Token* next = lookAhead();
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
    Token* next = lookAhead();
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

  Token* lookAhead()
  {
    if(pos >= tokens->size())
      return &PastEOF::inst;
    else
      return (*tokens)[pos];
  }

  void err(string msg)
  {
    string fullMsg = string("Syntax error at line ") + to_string(lookAhead()->line) + ", column " + to_string(lookAhead()->col);
    if(msg.length())
      fullMsg += string(": ") + msg;
    else
      fullMsg += '.';
    //display error and terminate
    errAndQuit(fullMsg);
  }

  void unget()
  {
    if(pos > 0)
      pos--;
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

