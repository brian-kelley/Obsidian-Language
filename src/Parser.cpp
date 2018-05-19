#include "Parser.hpp"
#include "Meta.hpp"
#include "Scope.hpp"
#include <stack>

struct BlockScope;

//Macros to help parse common patterns
//func should be the whole call, i.e. parseThing(s)
//end should be a token
#define PARSE_STAR(list, type, func, end) \
  while(!accept(end)) \
  { \
    list.push_back(func); \
  }

#define PARSE_STAR_COMMA(list, type, func, end) \
  if(!accept(end)) \
  { \
    while(true) \
    { \
      list.push_back(func); \
      if(accept(end)) \
        break; \
      expectPunct(COMMA); \
    } \
  }

#define PARSE_PLUS_COMMA(list, type, func, end) \
  while(true) \
  { \
    list.push_back(func); \
    if(accept(end)) \
      break; \
    expectPunct(COMMA); \
  }

namespace Parser
{
  size_t pos;
  vector<Token*> tokens;

  std::stack<Scope*> scopeStack;

  template<typename NT>
  NT* parse()
  {
    cout << "FATAL ERROR: non-implemented parse called, for type " << typeid(NT).name() << "\n";
    INTERNAL_ERROR;
    return NULL;
  }

  Module* parseProgram(vector<Token*>& toks)
  {
    pos = 0;
    tokens = &toks;
    Module* globalModule = new Module("", nullptr);
    while(!accept(PastEOF::inst))
    {
      parseScopedDecl(globalModule->scope, true);
    }
    return globalModule;
  }

  Module* parseModule(Scope* s)
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

  void parseScopedDecl(Scope* s, bool semicolon)
  {
    Punct colon(COLON);
    if(Keyword* kw = dynamic_cast<Keyword*>(lookAhead()))
    {
      switch(kw->kw)
      {
        case FUNC:
        case PROC:
          parseSubroutine(s);
          break;
        case EXTERN:
          parseExternalSubroutine(s);
          break;
        case STRUCT:
          parseStruct(s);
          break;
        case TYPEDEF:
          parseAlias(s);
          break;
        case MODULE:
          parseModule(s);
          break;
        case TEST:
          parseTest(s);
          break;
        default:
          INTERNAL_ERROR;
      }
    }
    else if(Ident* id = dynamic_cast<Ident*>(lookAhead()))
    {
      //variable declaration
      parseVarDecl(s);
    }
    INTERNAL_ERROR;
  }

  void parseType(Scope* s)
  {
  }

  void parseSubroutine(Scope* s)
  {
    Node* location = lookAhead();

    //Subroutine(Scope* s, string name, bool isStatic, bool pure, TypeSystem::Type* returnType, vector<string>& argNames, vector<TypeSystem::Type*>& argTypes, Block* body);

    bool pure;
    if(acceptKeyword(FUNC))
    {
      subr->isPure = true;
    }
    else
    {
      expectKeyword(PROC);
      pure = false;
    }
    bool isStatic = false;
    if(acceptKeyword(STATIC))
      isStatic = true;
    Type* retType = parseType(s);
    string name = ((Ident*) expect(IDENTIFIER))->name;
    expectPunct(LPAREN);
    vector<string> argNames;
    vector<Type*> argTypes;
    while(!acceptPunct(RPAREN))
    {
    }
    subr->body = parse<Block>();
    return subr;
  }

  void parseVarDecl(Scope* s, bool semicolon)
  {
    Node* loc = lookAhead();
    Ident* id = (Ident*) expect(IDENTIFIER);
    expectPunct(COLON);
    bool isStatic = false;
    bool compose = false;
    //"static" and "^" are mutually exclusive
    if(acceptPunct(STATIC))
    {
      isStatic = true;
    }
    else if(acceptOper(BXOR))
    {
      compose = true;
    }
    Type* type = parseType(s);
    if(semicolon)
      expectPunct(SEMICOLON);
    //create the variable and add to scope
    Variable* var;
    if(s->node.is<Block*>())
    {
      //local variable uses special constructor
      var = new Variable(id->name, type, s->node.get<Block*>());
    }
    else
    {
      //at parse time, if a variable is static, make sure it's in a struct
      if(!s->getMemberContext() && isStatic)
      {
        err("static variable declared outside any struct");
      }
      var = new Variable(s, id->name, type, isStatic, compose);
    }
    var->setLocation(loc);
    //add variable to scope
    s->addName(var);
  }

  static ScopedDecl* parseScopedDeclGeneral(bool semicolon)
  {
    ScopedDecl* sd = new ScopedDecl;
    //peek at next token to check for keywords that start decls
    //use short-circuit evaluation to find the pattern that parses successfully
    Punct hash(HASH);
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
        case EXTERN:
          sd->decl = parse<ExternSubroutineNT>();
          break;
        case TEST:
          sd->decl = parse<TestDecl>();
          break;
        default: found = false;
      }
      if(found)
      {
        return sd;
      }
    }
    else if(lookAhead()->compareTo(&hash))
    {
      //must be a meta variable or meta-subroutine
      Keyword* keywordAfter = dyanamic_cast<Keyword*>(lookAhead(1));
      if(keywordAfter &&
          (keywordAfter->kw == FUNC || keywordAfter->kw == PROC))
      {
        accept();
        auto subr = parse<SubroutineNT>();
        subr->meta = true;
        sd->decl = subr;
      }
      else
      {
        sd->decl = parse<MetaVar>();
      }
    }
    else
    {
      //only other possibility is VarDecl
      sd->decl = parse<VarDecl>();
      if(semicolon)
        expectPunct(SEMICOLON);
    }
    return sd;
  }

  template<>
  ScopedDecl* parse<ScopedDecl>()
  {
    return parseScopedDeclGeneral(true);
  }

  template<>
  TypeNT* parse<TypeNT>()
  {
    TypeNT* type = new TypeNT;
    type->arrayDims = 0;
    //check for keyword
    if(Keyword* keyword = (Keyword*) accept(KEYWORD))
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
      //parens always give the overall type high-precedence,
      //but expect high-precedence type(s) inside
      TypeNT* first = parse<TypeNT>();
      if(acceptPunct(COMMA))
      {
        //tuple
        vector<TypeNT*> tupleMembers;
        tupleMembers.push_back(first);
        do
        {
          tupleMembers.push_back(parse<TypeNT>());
        }
        while(acceptPunct(COMMA));
        expectPunct(RPAREN);
        type->t = new TupleTypeNT(tupleMembers);
      }
      else if(acceptPunct(COLON))
      {
        TypeNT* valueType = parse<TypeNT>();
        type->t = new MapTypeNT(first, valueType);
        expectPunct(RPAREN);
      }
      else if(acceptOper(BOR))
      {
        vector<TypeNT*> unionTypes;
        unionTypes.push_back(type);
        do
        {
          unionTypes.push_back(parse<TypeNT>());
        }
        while(acceptOper(BOR));
        expectPunct(RPAREN);
        TypeNT* wrapper = new TypeNT;
        wrapper->t = new UnionTypeNT(unionTypes);
        return wrapper;
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
    Punct quest(QUESTION);
    //check for square bracket pairs after, indicating array type
    while(lookAhead()->compareTo(&lbrack) ||
        lookAhead()->compareTo(&quest))
    {
      if(acceptPunct(LBRACKET))
      {
        expectPunct(RBRACKET);
        type->arrayDims++;
      }
      else if(acceptPunct(QUESTION))
      {
        //Form a union type with type and Error as its options
        auto ut = new UnionTypeNT;
        TypeNT* errType = new TypeNT;
        errType->t = TypeNT::ERROR;
        ut->types.push_back(type);
        ut->types.push_back(errType);
        type->t = ut;
      }
    }
    return type;
  }

  Statement* parseStatementGeneral(bool semicolon)
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
        case EMIT:
          s->s = parse<EmitNT>();
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
    else if(punct)
    {
      switch(punct->val)
      {
        case LBRACE:
          s->s = parse<Block>();
          return s;
        case LPAREN:
          //some type starting with '(': tuple/union/map
          s->s = parse<ScopedDecl>();
          return s;
        default:
        {
          ERR_MSG("unexpected punctuation " << punct->getStr() << " in statement");
        }
      }
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
    t->block = parseBlockWrappedStatement();
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

  void parseStatementOrDecl(Block* b)
  {
  }

  Block* parseBlock(Scope* s)
  {
    Block* b = new Block;
    expectPunct(LBRACE);
    while(!acceptPunct(RBRACE))
    {
    }
    return b;
  }

  Block* parseBlock(Scope* s)
  {
  }

  Block* parseBlock(Subroutine* s)
  {
  }

  Block* parseBlock(For* f)
  {
  }

  Block* parseBlock(While* w)
  {
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
  MetaVar* parse<MetaVar>()
  {
    MetaVar* mv = new MetaVar;
    expectPunct(HASH);
    mv->type = parse<TypeNT>();
    mv->name = ((Ident*) expect(IDENTIFIER))->name;
    if(acceptOper(ASSIGN))
    {
      mv->val = parse<ExpressionNT>();
    }
    return mv;
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
    p->type = parse<TypeNT>();
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

  template<>
  ExternSubroutineNT* parse<ExternSubroutineNT>()
  {
    ExternSubroutineNT* es = new ExternSubroutineNT;
    es->type = new SubroutineTypeNT;
    expectKeyword(EXTERN);
    if(acceptKeyword(FUNC))
      es->type->isPure = true;
    else if(acceptKeyword(PROC))
      es->type->isPure = false;
    else
      err("expected functype or proctype");
    es->type->nonterm = false;
    //all C functions are static, no matter where they are declared
    es->type->isStatic = false;
    //but nonterm is allowed
    if(acceptKeyword(NONTERM))
      es->type->nonterm = true;
    es->type->retType = parse<TypeNT>();
    expectPunct(LPAREN);
    Punct rparen(RPAREN);
    es->type->params = parseStarComma<Parameter>(rparen);
    es->c = ((Ident*) expect(IDENTIFIER))->name;
    return es;
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
    st->params = parseStarComma<Parameter>(rparen);
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

  Expression* parseExpr1()
  {
    Expression* root = parseExpr2();
    while(true)
    {
      Oper* nextOp = dynamic_cast<Oper*>(lookAhead());
      if(!nextOp || nextOp->op != LOR)
        break;
      accept();
      root = new BinaryArith(root, LOR, parseExpr2());
    }
    root->tryResolve();
    return root;
  }

  Expression* parseExpr2()
  {
    Expression* root = parseExpr3();
    while(true)
    {
      Oper* nextOp = dynamic_cast<Oper*>(lookAhead());
      if(!nextOp || nextOp->op != LAND)
        break;
      accept();
      root = new BinaryArith(root, LAND, parseExpr3());
    }
    root->tryResolve();
    return root;
  }

  Expression* parseExpr3()
  {
    Expression* root = parseExpr4();
    while(true)
    {
      Oper* nextOp = dynamic_cast<Oper*>(lookAhead());
      if(!nextOp || nextOp->op != BOR)
        break;
      accept();
      root = new BinaryArith(root, BOR, parseExpr4());
    }
    root->tryResolve();
    return root;
  }

  Expression* parseExpr4()
  {
    Expression* root = parseExpr5();
    while(true)
    {
      Oper* nextOp = dynamic_cast<Oper*>(lookAhead());
      if(!nextOp || nextOp->op != BXOR)
        break;
      accept();
      root = new BinaryArith(root, BXOR, parseExpr5());
    }
    root->tryResolve();
    return root;
  }

  Expression* parseExpr5()
  {
    Expression* root = parseExpr6();
    while(true)
    {
      Oper* nextOp = dynamic_cast<Oper*>(lookAhead());
      if(!nextOp || nextOp->op != BAND)
        break;
      accept();
      root = new BinaryArith(root, BAND, parseExpr6());
    }
    root->tryResolve();
    return root;
  }

  Expression* parseExpr6()
  {
    Expression* root = parseExpr7();
    while(true)
    {
      Oper* nextOp = dynamic_cast<Oper*>(lookAhead());
      if(!nextOp || (nextOp->op != CMPEQ && nextOp->op != CMPNEQ))
        break;
      accept();
      root = new BinaryArith(root, nextOp->op, parseExpr7());
    }
    root->tryResolve();
    return root;
  }

  Expression* parseExpr7()
  {
    Expression* root = parseExpr8();
    while(true)
    {
      Oper* nextOp = dynamic_cast<Oper*>(lookAhead());
      if(!nextOp || (nextOp->op != CMPL && nextOp->op != CMPLE
            && nextOp->op != CMPG && nextOp->op != CMPGE))
        break;
      accept();
      root = new BinaryArith(root, nextOp->op, parseExpr8());
    }
    root->tryResolve();
    return root;
  }

  Expression* parseExpr8()
  {
    Expression* root = parseExpr9();
    while(true)
    {
      Oper* nextOp = dynamic_cast<Oper*>(lookAhead());
      if(!nextOp || (nextOp->op != SHL && nextOp->op != SHR))
        break;
      accept();
      root = new BinaryArith(root, nextOp->op, parseExpr9());
    }
    root->tryResolve();
    return root;
  }

  Expression* parseExpr9()
  {
    Expression* root = parseExpr10();
    while(true)
    {
      Oper* nextOp = dynamic_cast<Oper*>(lookAhead());
      if(!nextOp || (nextOp->op != PLUS && nextOp->op != SUB))
        break;
      accept();
      root = new BinaryArith(root, nextOp->op, parseExpr10());
    }
    root->tryResolve();
    return root;
  }

  Expression* parseExpr10()
  {
    Expression* root = parseExpr11();
    while(true)
    {
      Oper* nextOp = dynamic_cast<Oper*>(lookAhead());
      if(!nextOp ||
          (nextOp->op != MUL && nextOp->op != DIV && nextOp->op != MOD))
        break;
      accept();
      root = new BinaryArith(root, nextOp->op, parseExpr11());
    }
    root->tryResolve();
    return root;
  }

  Expression* parseExpr11()
  {
    if(Oper* oper = (Oper*) accept(OPERATOR))
    {
      if(oper->op != LNOT && oper->op != BNOT && oper->op != SUB)
      {
        err("expected one of: ! ~ -");
      }
      Expression* ue = new UnaryArith(oper->op, parseExpr11());
      ue->tryResolve();
      return ue;
    }
    return parseExpr12();
  }

  Expression* parseExpr12()
  {
    Expression* root = nullptr;
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
          root = parseExpr1();
          expectPunct(RPAREN);
        }
        else if(punct->val == LBRACKET)
        {
          //struct lit
          root = parseStructLit();
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
          root = new BoolLiteral(true);
        else if(kw->kw == FALSE)
          root = new BoolLiteral(false);
        else if(kw->kw == ERROR_VALUE)
          root = new ErrorVal;
        else if(kw->kw == THIS)
          root = new ThisExpr;
        else if(kw->kw == ARRAY)
          root = parse<NewArray>();
        else
          err("invalid keyword in expression");
        break;
      }
      case INT_LITERAL:
        root = new (IntLit*) expect(INT_LITERAL);
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
        root = new UnresolvedExpr(parseMember());
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
    auto next = lookAhead();
    if(t.compareTo(next))
    {
      pos++;
      return;
    }
    err(string("expected ") + t.getStr() + " but got " + next->getStr());
  }

  Token* expect(int tokType)
  {
    Token* next = lookAhead();
    if(next->getType() == tokType)
    {
      pos++;
    }
    else
    {
      err(string("expected a ") + tokTypeTable[tokType] + " but got " + next->getStr());
    }
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
    {
      return &PastEOF::inst;
    }
    else
    {
      return (*tokens)[index];
    }
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

