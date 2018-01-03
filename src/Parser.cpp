#include "Parser.hpp"

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
  template<> Break* parse<Break>();
  template<> Continue* parse<Continue>();
  template<> Switch* parse<Switch>();
  template<> Match* parse<Match>();
  template<> ForC* parse<ForC>();
  template<> ForOverArray* parse<ForOverArray>();
  template<> ForRange* parse<ForRange>();
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
  template<> SubroutineNT* parse<SubroutineNT>();
  template<> SubroutineTypeNT* parse<SubroutineTypeNT>();
  template<> Parameter* parse<Parameter>();
  template<> StructDecl* parse<StructDecl>();
  template<> TraitDecl* parse<TraitDecl>();
  template<> StructLit* parse<StructLit>();
  template<> BoolLit* parse<BoolLit>();
  template<> Member* parse<Member>();
  template<> BoundedTypeNT* parse<BoundedTypeNT>();
  template<> TupleTypeNT* parse<TupleTypeNT>();
  template<> UnionTypeNT* parse<UnionTypeNT>();
  template<> MapTypeNT* parse<MapTypeNT>();
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
    while(!accept(PastEOF::inst))
    {
      globalModule->decls.push_back(parse<ScopedDecl>());
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
    while(!acceptPunct(RBRACE))
    {
      m->decls.push_back(parse<ScopedDecl>());
    }
    return m;
  }

  static ScopedDecl* parseScopedDeclGeneral(bool semicolon)
  {
    ScopedDecl* sd = new ScopedDecl;
    //peek at next token to check for keywords that start decls
    //use short-circuit evaluation to find the pattern that parses successfully
    Keyword* nextKeyword = dynamic_cast<Keyword*>(lookAhead());
    if(nextKeyword)
    {
      bool found = true;
      switch(nextKeyword->kw)
      {
        case MODULE:
          sd->decl = parse<Module>(); break;
        case STRUCT:
          sd->decl = parse<StructDecl>(); break;
        case UNION:
        {
          //parse union <name> { type1, type2, ... }
          accept();
          string name = ((Ident*) expect(IDENTIFIER))->name;
          expectPunct(LBRACE);
          UnionTypeNT* ut = new UnionTypeNT(parsePlusComma<TypeNT>());
          TypeNT* t = new TypeNT;
          t->t = ut;
          expectPunct(RBRACE);
          //alias the union type
          sd->decl = new Typedef(name, t);
          break;
        }
        case TRAIT:
          sd->decl = parse<TraitDecl>(); break;
        case ENUM:
          sd->decl = parse<Enum>(); break;
        case TYPEDEF:
          sd->decl = parse<Typedef>();
          if(semicolon)
            expectPunct(SEMICOLON);
          break;
        case FUNC:
        case PROC:
          sd->decl = parse<SubroutineNT>();
          break;
        default: found = false;
      }
      if(found)
        return sd;
    }
    //only other possibility is VarDecl
    sd->decl = parse<VarDecl>();
    if(semicolon)
      expectPunct(SEMICOLON);
    return sd;
  }

  template<>
  ScopedDecl* parse<ScopedDecl>()
  {
    return parseScopedDeclGeneral(true);
  }

  //if prec ("high precedence"), stop before ? and |
  //anything in parens is automatically high precedence
  static TypeNT* parseTypeGeneral(bool prec)
  {
    TypeNT* type = new TypeNT;
    type->arrayDims = 0;
    //check for keyword
    Keyword* keyword = (Keyword*) accept(KEYWORD);
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
        case ERROR_TYPE:
          type->t = TypeNT::Prim::ERROR; break;
        case FUNCTYPE:
        case PROCTYPE:
          unget();
          type->t = parse<SubroutineTypeNT>();
          break;
        default:
          err("expected type");
      }
    }
    else if(acceptPunct(LPAREN))
    {
      //parens always give the overall type high-precedence
      prec = true;
      TypeNT* first = parseTypeGeneral(false);
      Punct comma(COMMA);
      if(lookAhead()->compareTo(&comma))
      {
        //tuple
        vector<TypeNT*> tupleMembers;
        tupleMembers.push_back(first);
        while(acceptPunct(COMMA))
        {
          tupleMembers.push_back(parseTypeGeneral(false));
        }
        expectPunct(RPAREN);
        if(tupleMembers.size() == 1)
        {
          //not really a tuple (just a single type in parens)
          //this could be a union, so no need to cover union type here
          type = first;
        }
        else
        {
          type->t = new TupleTypeNT(tupleMembers);
        }
      }
      else if(acceptPunct(COLON))
      {
        TypeNT* valueType = parseTypeGeneral(false);
        type->t = new MapTypeNT(first, valueType);
        expectPunct(RPAREN);
      }
      else
      {
        expectPunct(RPAREN);
        return first;
      }
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
    Punct lbrack(LBRACKET);
    Punct rbrack(RBRACKET);
    //check for square bracket pairs after, indicating array type
    while(lookAhead(0)->compareTo(&lbrack) && lookAhead(1)->compareTo(&rbrack))
    {
      accept();
      accept();
      type->arrayDims++;
    }
    if(acceptPunct(QUESTION))
    {
      //Form a union type with type and Error as its options
      auto ut = new UnionTypeNT;
      TypeNT* errType = new TypeNT;
      errType->t = TypeNT::ERROR;
      ut->types.push_back(type);
      ut->types.push_back(errType);
      type->t = ut;
    }
    //Union chain is low precedence, so stop if high prec is required
    if(prec)
      return type;
    //have parsed a type: first check for "?" indicating (T | Error)
    //(this has higher "precedence" than | for normal union)
    //now check for "|" for union
    if(acceptOper(BOR))
    {
      vector<TypeNT*> unionTypes;
      unionTypes.push_back(type);
      do
      {
        unionTypes.push_back(parseTypeGeneral(true));
      }
      while(acceptOper(BOR));
      TypeNT* wrapper = new TypeNT;
      wrapper->t = new UnionTypeNT(unionTypes);
      return wrapper;
    }
    return type;
  }

  template<>
  TypeNT* parse<TypeNT>()
  {
    return parseTypeGeneral(false);
  }

  Block* parseBlockWrappedStatement()
  {
    Block* b = new Block;
    StatementNT* s = parse<StatementNT>();
    if(s->s.is<Block*>())
      return s->s.get<Block*>();
    b->statements.push_back(s);
    return b;
  }

  StatementNT* parseVarDeclGivenMember(Member* mem)
  {
    auto stmt = new StatementNT;
    auto sd = new ScopedDecl;
    auto vd = new VarDecl;
    auto type = new TypeNT;
    type->t = mem;
    //can have any number of high-precedence types unioned together
    //parse one full one first to see if this is actually a union
    while(acceptPunct(LBRACKET))
    {
      type->arrayDims++;
      expectPunct(RBRACKET);
    }
    Oper bor(BOR);
    if(lookAhead()->compareTo(&bor))
    {
      UnionTypeNT* ut = new UnionTypeNT;
      ut->types.push_back(type);
      //now use "type" to store the overall union
      type->t = ut;
      while(acceptOper(BOR))
      {
        ut->types.push_back(parseTypeGeneral(true));
      }
    }
    //type is done, get the var name (required) and then
    //init expr (optional)
    vd->type = type;
    vd->name = ((Ident*) expect(IDENTIFIER))->name;
    if(acceptOper(ASSIGN))
    {
      vd->val = parse<ExpressionNT>();
    }
    vd->isStatic = false;
    vd->composed = false;
    sd->decl = vd;
    stmt->s = sd;
    return stmt;
  }

  StatementNT* parseStatementGeneral(bool semicolon)
  {
    //Get some possibilities for the next token
    Token* next = lookAhead();
    Keyword* keyword = dynamic_cast<Keyword*>(next);
    Punct* punct = dynamic_cast<Punct*>(next);
    Ident* ident = dynamic_cast<Ident*>(next);
    StatementNT* s = new StatementNT;
    Expr12* leadingExpr12;
    if(keyword)
    {
      switch(keyword->kw)
      {
        case PRINT:
          s->s = parse<PrintNT>();
          if(semicolon)
            expectPunct(SEMICOLON);
          return s;
        case RETURN:
          s->s = parse<Return>();
          if(semicolon)
            expectPunct(SEMICOLON);
          return s;
        case CONTINUE:
          s->s = parse<Continue>();
          if(semicolon)
            expectPunct(SEMICOLON);
          return s;
        case BREAK:
          s->s = parse<Break>();
          if(semicolon)
            expectPunct(SEMICOLON);
          return s;
        case SWITCH:
          s->s = parse<Switch>();
          return s;
        case MATCH:
          s->s = parse<Match>();
          return s;
        case FOR:
          s->s = parse<For>();
          return s;
        case WHILE:
          s->s = parse<While>();
          return s;
        case IF:
          s->s = parse<If>();
          return s;
        case ASSERT:
          s->s = parse<Assertion>();
          if(semicolon)
            expectPunct(SEMICOLON);
          return s;
        case THIS:
        case ERROR_VALUE:
          break;
        //handle various kinds of ScopedDecl
        case MODULE:
        case STRUCT:
        case UNION:
        case TRAIT:
        case ENUM:
        case FUNC:
        case PROC:
        case TYPEDEF:
        case TEST:
        //handle type keywords (the type for VarDecl)
        case BOOL:
        case CHAR:
        case BYTE:
        case UBYTE:
        case SHORT:
        case USHORT:
        case INT:
        case UINT:
        case LONG:
        case ULONG:
        case FLOAT:
        case DOUBLE:
          s->s = parse<ScopedDecl>();
          return s;
        default: err("expected statement or declaration, but got keyword " + keywordTable[keyword->kw]);
      }
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
    if(ident)
    {
      //this must succeed when starting with an Ident
      Member* mem = parse<Member>();
      Token* afterMem1 = lookAhead(0);
      Token* afterMem2 = lookAhead(1);
      //VarDecl always starts with type, so only ways for this
      //to still be a VarDecl is:
      // Member Ident ...
      // Member[]...
      // Member | ...
      // Uses 2 tokens of lookahead
      Punct lbrack(LBRACKET);
      Punct rbrack(RBRACKET);
      Oper bor(BOR);
      if(afterMem1->type == IDENTIFIER ||
          (afterMem1->compareTo(&lbrack) && afterMem2->compareTo(&rbrack)) ||
          afterMem1->compareTo(&bor))
      {
        //definitely have a VarDecl, so parse a type given mem
        s = parseVarDeclGivenMember(mem);
        if(semicolon)
          expectPunct(SEMICOLON);
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
    if(lookAhead()->type == OPERATOR)
    {
      s->s = parseAssignGivenExpr12(leadingExpr12);
      if(semicolon)
        expectPunct(SEMICOLON);
      return s;
    }
    else if(leadingExpr12->tail.size() && leadingExpr12->tail.back()->e.is<CallOp*>())
    {
      //call
      s->s = leadingExpr12;
      if(semicolon)
        expectPunct(SEMICOLON);
      return s;
    }
    //parsing leadingExpr12 must have succeeded, so give
    //error message knowing that there was some valid expression there
    err("expected call or assignment, but got some other expression");
    return nullptr;
  }

  StatementNT* parseStatementWithoutSemicolon()
  {
    return parseStatementGeneral(false);
  }

  template<>
  StatementNT* parse<StatementNT>()
  {
    return parseStatementGeneral(true);
  }

  template<>
  Break* parse<Break>()
  {
    Break* b = new Break;
    expectKeyword(BREAK);
    return b;
  }

  template<>
  Continue* parse<Continue>()
  {
    Continue* c = new Continue;
    expectKeyword(CONTINUE);
    return c;
  }

  template<>
  EmptyStatement* parse<EmptyStatement>()
  {
    EmptyStatement* es = new EmptyStatement;
    return es;
  }

  template<>
  Typedef* parse<Typedef>()
  {
    Typedef* td = new Typedef;
    expectKeyword(TYPEDEF);
    td->type = parse<TypeNT>();
    td->ident = ((Ident*) expect(IDENTIFIER))->name;
    return td;
  }

  template<>
  Return* parse<Return>()
  {
    Return* r = new Return;
    expectKeyword(RETURN);
    r->ex = NULL;
    if(!acceptPunct(SEMICOLON))
      r->ex = parse<ExpressionNT>();
    return r;
  }

  template<>
  Switch* parse<Switch>()
  {
    Switch* s = new Switch;
    s->block = new Block;
    auto& stmts = s->block->statements;
    expectKeyword(SWITCH);
    expectPunct(LPAREN);
    s->value = parse<ExpressionNT>();
    expectPunct(RPAREN);
    expectPunct(LBRACE);
    s->defaultPosition = -1;
    while(!acceptPunct(RBRACE))
    {
      if(acceptKeyword(CASE))
      {
        s->labels.emplace_back(stmts.size(), parse<ExpressionNT>());
        expectPunct(COLON);
      }
      else if(acceptKeyword(DEFAULT))
      {
        if(s->defaultPosition != -1)
        {
          err("default label provided more than once in switch statement");
        }
        s->defaultPosition = stmts.size();
        expectPunct(COLON);
      }
      else
      {
        stmts.push_back(parse<StatementNT>());
        if(stmts.back()->s.is<ScopedDecl*>())
        {
          err("declaration directly inside switch statement (fix: enclose it in a block)");
        }
      }
    }
    if(s->defaultPosition == -1)
    {
      //no explicit default, so implicitly put it after all statements
      s->defaultPosition = stmts.size();
    }
    return s;
  }

  template<>
  Match* parse<Match>()
  {
    Match* m = new Match;
    expectKeyword(MATCH);
    m->varName = ((Ident*) expect(IDENTIFIER))->name;
    expectPunct(COLON);
    m->value = parse<ExpressionNT>();
    expectPunct(LBRACE);
    //parse cases until either default or rbrace is found
    while(!acceptPunct(RBRACE))
    {
      expectKeyword(CASE);
      TypeNT* t = parse<TypeNT>();
      expectPunct(COLON);
      m->cases.emplace_back(t, parse<Block>());
    }
    return m;
  }

  template<>
  ForC* parse<ForC>()
  {
    //try to parse C style for loop
    ForC* forC = new ForC;
    expectPunct(LPAREN);
    //all 3 parts of the loop are optional
    forC->decl = nullptr;
    forC->condition = nullptr;
    forC->incr = nullptr;
    if(!acceptPunct(SEMICOLON))
    {
      //statement includes the semicolon
      forC->decl = parse<StatementNT>();
    }
    if(!acceptPunct(SEMICOLON))
    {
      forC->condition = parse<ExpressionNT>();
      expectPunct(SEMICOLON);
    }
    if(!acceptPunct(RPAREN))
    {
      forC->incr = parseStatementWithoutSemicolon();
      expectPunct(RPAREN);
    }
    return forC;
  }

  template<>
  ForOverArray* parse<ForOverArray>()
  {
    ForOverArray* foa = new ForOverArray;
    expectPunct(LBRACKET);
    foa->tup.push_back(((Ident*) expect(IDENTIFIER))->name);
    while(acceptPunct(COMMA))
    {
      foa->tup.push_back(((Ident*) expect(IDENTIFIER))->name);
    }
    expectPunct(RBRACKET);
    expectPunct(COLON);
    foa->expr = parse<ExpressionNT>();
    return foa;
  }

  template<>
  ForRange* parse<ForRange>()
  {
    ForRange* fr = new ForRange;
    fr->name = ((Ident*) expect(IDENTIFIER))->name;
    expectPunct(COLON);
    fr->start = parse<ExpressionNT>();
    expectPunct(COMMA);
    fr->end = parse<ExpressionNT>();
    return fr;
  }

  //New For syntax (3 variations):
  //ForC:         for(<stmt>; <expr>; <stmt>) {body}
  //ForOverArray: for [i, j, k, ..., it] : <expr> {body}
  //ForRange:     for i : lo, hi {body}
  //etc, size of tuple is arbitrary in parser,
  //and checked in middle end against expr num dimensions

  //Can figure out which variation with 1 token lookahead after "for":
  //  ( ----> ForC
  //  [ ----> ForOverArray
  //  Ident ----> ForRange

  template<>
  For* parse<For>()
  {
    For* f = new For;
    expectKeyword(FOR);
    auto t = lookAhead();
    if(dynamic_cast<Ident*>(t))
    {
      f->f = parse<ForRange>();
    }
    else if(Punct* p = dynamic_cast<Punct*>(t))
    {
      if(p->val == LBRACKET)
        f->f = parse<ForOverArray>();
      else if(p->val == LPAREN)
        f->f = parse<ForC>();
      else
        err("invalid for loop");
    }
    f->body = parseBlockWrappedStatement();
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
    t->stmt = parse<StatementNT>();
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
    return b;
  }

  template<>
  VarDecl* parse<VarDecl>()
  {
    VarDecl* vd = new VarDecl;
    vd->isStatic = acceptKeyword(STATIC);
    if(!vd->isStatic)
    {
      vd->composed = acceptOper(BXOR);
    }
    vd->type = nullptr;
    if(!acceptKeyword(AUTO))
    {
      vd->type = parse<TypeNT>();
    }
    vd->name = ((Ident*) expect(IDENTIFIER))->name;
    if(acceptOper(ASSIGN))
    {
      vd->val = parse<ExpressionNT>();
    }
    if(!vd->type && !vd->val)
    {
      err("auto declaration requires initialization");
    }
    //note: semicolon must be handled by caller
    return vd;
  }
  
  VarAssign* parseAssignGivenExpr12(Expr12* target)
  {
    VarAssign* va = new VarAssign;
    va->target = target;
    ExpressionNT* rhs = nullptr;
    int otype = ((Oper*) expect(OPERATOR))->op;
    //unary assign operators don't have rhs
    if(otype != INC && otype != DEC)
    {
      rhs = parse<ExpressionNT>();
    }
    if(otype != ASSIGN &&
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
        va->rhs = rhs;
        return va;
      }
      case INC:
      case DEC:
      {
        Expr12* oneLit = new Expr12;
        oneLit->e = new IntLit(1);
        //addition and subtraction encoded in Expr10
        Expr9* sum = new Expr9(target);
        Expr9RHS* oneRHS = new Expr9RHS;
        oneRHS->rhs = new Expr10(oneLit);
        if(otype == INC)
          oneRHS->op = PLUS;
        else
          oneRHS->op = SUB;
        sum->tail.push_back(oneRHS);
        va->rhs = new ExpressionNT(sum);
        return va;
      }
      case PLUSEQ:
      case SUBEQ:
      {
        Expr9* ex = new Expr9(target);
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
        Expr10* ex = new Expr10(target);
        Expr10RHS* r = new Expr10RHS;
        if(otype == MULEQ)
          r->op = MUL;
        else if(otype == DIVEQ)
          r->op = DIV;
        else
          r->op = MOD;
        r->rhs = new Expr11(new Expr12(rhs));
        ex->tail.push_back(r);
        va->rhs = new ExpressionNT(ex);
        return va;
      }
      case BOREQ:
      {
        Expr3* ex = new Expr3(target);
        Expr3RHS* r = new Expr3RHS;
        r->rhs = new Expr4(new Expr12(rhs));
        ex->tail.push_back(r);
        va->rhs = new ExpressionNT(ex);
        return va;
      }
      case BANDEQ:
      {
        Expr5* ex = new Expr5(target);
        Expr5RHS* r = new Expr5RHS;
        r->rhs = new Expr6(new Expr12(rhs));
        ex->tail.push_back(r);
        va->rhs = new ExpressionNT(ex);
        return va;
      }
      case BXOREQ:
      {
        Expr4* ex = new Expr4(target);
        Expr4RHS* r = new Expr4RHS;
        r->rhs = new Expr5(new Expr12(rhs));
        ex->tail.push_back(r);
        va->rhs = new ExpressionNT(ex);
        return va;
      }
      case SHLEQ:
      case SHREQ:
      {
        Expr8* ex = new Expr8(target);
        Expr8RHS* r = new Expr8RHS;
        r->rhs = new Expr9(new Expr12(rhs));
        if(otype == SHLEQ)
          r->op = SHL;
        else
          r->op = SHR;
        ex->tail.push_back(r);
        va->rhs = new ExpressionNT(ex);
        return va;
      }
      default: INTERNAL_ERROR;
    }
    return va;
  }

  template<>
  VarAssign* parse<VarAssign>()
  {
    //need to determine lvalue and rvalue (target and rhs)
    Expr12* target = parse<Expr12>();
    return parseAssignGivenExpr12(target);
  }

  template<>
  PrintNT* parse<PrintNT>()
  {
    PrintNT* p = new PrintNT;
    expectKeyword(PRINT);
    expectPunct(LPAREN);
    p->exprs = parsePlusComma<ExpressionNT>();
    expectPunct(RPAREN);
    return p;
  }

  template<>
  Parameter* parse<Parameter>()
  {
    Parameter* p = new Parameter;
    //look ahead for Ident followed by ':' (means bounded type)
    //otherwise, just parse regular TypeNT
    Ident* nextID = dynamic_cast<Ident*>(lookAhead(0));
    Punct* punct = dynamic_cast<Punct*>(lookAhead(1));
    if(nextID && punct && punct->val == COLON)
    {
      p->type = parse<BoundedTypeNT>();
    }
    else
    {
      p->type = parse<TypeNT>();
    }
    //optional parameter name
    Ident* paramName = (Ident*) accept(IDENTIFIER);
    if(paramName)
    {
      p->name = paramName->name;
    }
    return p;
  }
  
  template<>
  SubroutineNT* parse<SubroutineNT>()
  {
    SubroutineNT* subr = new SubroutineNT;
    if(acceptKeyword(FUNC))
      subr->isPure = true;
    else if(acceptKeyword(PROC))
      subr->isPure = false;
    else
      err("expected func or proc");
    //get modifiers (can be in any order)
    subr->nonterm = false;
    subr->isStatic = false;
    if(acceptKeyword(NONTERM))
      subr->nonterm = true;
    if(acceptKeyword(STATIC))
      subr->isStatic = true;
    if(acceptKeyword(NONTERM))
    {
      if(subr->nonterm)
        err("nonterm modifier given twice");
      subr->nonterm = true;
    }
    subr->retType = parse<TypeNT>();
    subr->name = ((Ident*) expect(IDENTIFIER))->name;
    expectPunct(LPAREN);
    Punct rparen(RPAREN);
    subr->params = parseStar<Parameter>(rparen);
    subr->body = nullptr;
    if(!acceptPunct(SEMICOLON))
    {
      subr->body = parse<Block>();
    }
    return subr;
  }

  template<>
  SubroutineTypeNT* parse<SubroutineTypeNT>()
  {
    SubroutineTypeNT* st = new SubroutineTypeNT;
    if(acceptKeyword(FUNCTYPE))
      st->isPure = true;
    else if(acceptKeyword(PROCTYPE))
      st->isPure = false;
    else
      err("expected functype or proctype");
    st->nonterm = false;
    st->isStatic = false;
    if(acceptKeyword(NONTERM))
      st->nonterm = true;
    if(acceptKeyword(STATIC))
      st->isStatic = true;
    if(acceptKeyword(NONTERM))
    {
      if(st->nonterm)
        err("nonterm modifier given twice");
      st->nonterm = true;
    }
    st->retType = parse<TypeNT>();
    expectPunct(LPAREN);
    Punct rparen(RPAREN);
    st->params = parseStar<Parameter>(rparen);
    return st;
  }

  template<>
  StructDecl* parse<StructDecl>()
  {
    StructDecl* sd = new StructDecl;
    expectKeyword(STRUCT);
    sd->name = ((Ident*) expect(IDENTIFIER))->name;
    if(acceptPunct(COLON))
    {
      sd->traits = parsePlusComma<Member>();
    }
    expectPunct(LBRACE);
    Punct rbrace(RBRACE);
    sd->members = parseStar<ScopedDecl>(rbrace);
    return sd;
  }

  template<>
  TraitDecl* parse<TraitDecl>()
  {
    TraitDecl* td = new TraitDecl;
    expectKeyword(TRAIT);
    td->name = ((Ident*) expect(IDENTIFIER))->name;
    expectPunct(LBRACE);
    Punct rbrace(RBRACE);
    td->members = parseStar<SubroutineNT>(rbrace);
    return td;
  }

  template<>
  StructLit* parse<StructLit>()
  {
    StructLit* sl = new StructLit;
    expectPunct(LBRACKET);
    sl->vals = parsePlusComma<ExpressionNT>();
    expectPunct(RBRACKET);
    return sl;
  }

  template<>
  Member* parse<Member>()
  {
    Member* m = new Member;
    //get a list of all strings separated by dots
    m->names.push_back(((Ident*) expect(IDENTIFIER))->name);
    while(acceptPunct(DOT))
    {
      m->names.push_back(((Ident*) expect(IDENTIFIER))->name);
    }
    return m;
  }

  template<>
  BoundedTypeNT* parse<BoundedTypeNT>()
  {
    // <ident> : <trait1, trait2, ... traitN>
    // Given the ':', there must be one or more trait names
    BoundedTypeNT* bt = new BoundedTypeNT;
    bt->localName = ((Ident*) expect(IDENTIFIER))->name;
    expectPunct(COLON);
    bt->traits = parsePlusComma<Member>();
    return bt;
  }

  template<>
  TupleTypeNT* parse<TupleTypeNT>()
  {
    TupleTypeNT* tt = new TupleTypeNT;
    expectPunct(LPAREN);
    tt->members = parsePlusComma<TypeNT>();
    expectPunct(RPAREN);
    return tt;
  }

  template<>
  UnionTypeNT* parse<UnionTypeNT>()
  {
    UnionTypeNT* ut = new UnionTypeNT;
    ut->types.push_back(parse<TypeNT>());
    while(acceptOper(BOR))
    {
      ut->types.push_back(parse<TypeNT>());
    }
    return ut;
  }

  template<>
  MapTypeNT* parse<MapTypeNT>()
  {
    MapTypeNT* mt = new MapTypeNT;
    expectPunct(LPAREN);
    mt->keyType = parse<TypeNT>();
    expectPunct(COLON);
    mt->valueType = parse<TypeNT>();
    expectPunct(RPAREN);
    return mt;
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

  Expr1::Expr1(Expr2* ex)
  {
    e = ex;
  }

  Expr1::Expr1(Expr3* ex)
  {
    e = new Expr2(ex);
  }

  Expr1::Expr1(Expr4* ex)
  {
    e = new Expr2(new Expr3(ex));
  }

  Expr1::Expr1(Expr5* ex)
  {
    e = new Expr2(new Expr3(new Expr4(ex)));
  }

  Expr1::Expr1(Expr6* ex)
  {
    e = new Expr2(new Expr3(new Expr4(new Expr5(ex))));
  }

  Expr1::Expr1(Expr7* ex)
  {
    e = new Expr2(new Expr3(new Expr4(new Expr5(new Expr6(ex)))));
  }

  Expr1::Expr1(Expr8* ex)
  {
    e = new Expr2(new Expr3(new Expr4(new Expr5(new Expr6(new Expr7(ex))))));
  }

  Expr1::Expr1(Expr9* ex)
  {
    e = new Expr2(new Expr3(new Expr4(new Expr5(new Expr6(new Expr7(new Expr8(ex)))))));
  }

  Expr1::Expr1(Expr10* ex)
  {
    e = new Expr2(new Expr3(new Expr4(new Expr5(new Expr6(new Expr7(new Expr8(new Expr9(ex))))))));
  }

  Expr1::Expr1(Expr11* ex)
  {
    e = new Expr2(new Expr3(new Expr4(new Expr5(new Expr6(new Expr7( new Expr8(new Expr9(new Expr10(ex)))))))));
  }

  Expr1::Expr1(Expr12* e12)
  {
    e = new Expr2(e12);
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
      unget();
      e1->e = parse<NewArrayNT>();
      //no tail for NewArrayNT (not compatible with any operands)
    }
    else
    {
      e1->e = parse<Expr2>();
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
    Expr2* e2 = new Expr2;
    e2->head = parse<Expr3>();
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
    if(Oper* oper = (Oper*) accept(OPERATOR))
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
    Token* next = lookAhead();
    switch(next->type)
    {
      case PUNCTUATION:
      {
        Punct* punct = (Punct*) next;
        if(punct->val == LPAREN)
        {
          accept();
          //expression in parentheses
          e12->e = parse<ExpressionNT>();
          expectPunct(RPAREN);
        }
        else if(punct->val == LBRACKET)
        {
          //struct lit
          e12->e = parse<StructLit>();
        }
        else
        {
          //nothing else valid
          err(string("invalid punctuation in expression: ") + punctTable[punct->val]);
        }
        break;
      }
      case KEYWORD:
      {
        Keyword* kw = (Keyword*) expect(KEYWORD);
        if(kw->kw == TRUE)
          e12->e = new BoolLit(true);
        else if(kw->kw == FALSE)
          e12->e = new BoolLit(true);
        else if(kw->kw == ERROR_VALUE)
          e12->e = Expr12::Error();
        else if(kw->kw == THIS)
          e12->e = Expr12::This();
        else
          err("invalid keyword in expression");
        break;
      }
      case INT_LITERAL:
        e12->e = (IntLit*) expect(INT_LITERAL);
        break;
      case FLOAT_LITERAL:
        e12->e = (FloatLit*) expect(FLOAT_LITERAL);
        break;
      case CHAR_LITERAL:
        e12->e = (CharLit*) expect(CHAR_LITERAL);
        break;
      case STRING_LITERAL:
        e12->e = (StrLit*) expect(STRING_LITERAL);
        break;
      case IDENTIFIER:
        e12->e = parse<Member>();
        break;
      default:
        err("unexpected token \"" + next->getStr() + "\" (type " + next->getDesc() + ") in expression");
    }
    parseExpr12Tail(e12);
    return e12;
  }

  void parseExpr12Tail(Expr12* head)
  {
    while(true)
    {
      if(acceptPunct(LBRACKET))
      {
        //"[ Expr ]"
        head->tail.push_back(new Expr12RHS);
        head->tail.back()->e = parse<ExpressionNT>();
        expectPunct(RBRACKET);
      }
      else if(acceptPunct(DOT))
      {
        //". Ident"
        head->tail.push_back(new Expr12RHS);
        head->tail.back()->e = ((Ident*) expect(IDENTIFIER))->name;
      }
      else if(acceptPunct(LPAREN))
      {
        //"( Args )" - aka a CallOp
        unget();
        head->tail.push_back(new Expr12RHS);
        head->tail.back()->e = parse<CallOp>();
      }
      else
        break;
    }
  }

  Expr12* parseExpr12GivenMember(Member* mem)
  {
    //first, consume the member by using a chain of "member" operators
    Expr12* e12 = new Expr12;
    e12->e = mem;
    parseExpr12Tail(e12);
    return e12;
  }

  template<>
  CallOp* parse<CallOp>()
  {
    CallOp* co = new CallOp;
    expectPunct(LPAREN);
    Punct term(RPAREN);
    co->args = parseStarComma<ExpressionNT>(term);
    return co;
  }

  template<>
  NewArrayNT* parse<NewArrayNT>()
  {
    NewArrayNT* na = new NewArrayNT;
    expectKeyword(ARRAY);
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

  Token* lookAhead(int n)
  {
    int index = pos + n;
    if(index >= tokens->size())
      return &PastEOF::inst;
    else
      return (*tokens)[index];
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
    assert(pos > 0);
    pos--;
  }
}

ostream& operator<<(ostream& os, const Parser::Member& mem)
{
  for(size_t i = 0; i < mem.names.size(); i++)
  {
    os << mem.names[i];
    if(i != mem.names.size() - 1)
    {
      os << '.';
    }
  }
  return os;
}

ostream& operator<<(ostream& os, const Parser::ParseNode& pn)
{
  os << pn.line << ":" << pn.col;
  return os;
}

