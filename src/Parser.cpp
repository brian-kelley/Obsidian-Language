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

  void parseModule(Scope* s)
  {
    Node* location = lookAhead();
    expectKeyword(MODULE);
    string name = expectIdent();
    Module* m = new Module(name, s);
    m->setLocation(location);
    expectPunct(LBRACE);
    while(!acceptPunct(RBRACE))
    {
      parseScopedDecl(m->scope, true);
    }
    s->addName(m);
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

  void parseBlock(Block* b)
  {
    b->setLocation(lookAhead());
    expectPunct(LBRACE);
    while(!acceptPunct(RBRACE))
    {
      parseStatementOrDecl(b);
    }
  }

  Type* parseType(Scope* s)
  {
    UnresolvedType* t = new UnresolvedType;
    t->scope = s;
    t->setLocation(lookAhead());
    type->arrayDims = 0;
    //check for keyword
    if(Keyword* keyword = (Keyword*) accept(KEYWORD))
    {
      bool pure = false;
      //all possible types now (except Callables) are primitive, so set kind
      switch(keyword->kw)
      {
        case BOOL:
          t->t = Prim::BOOL; break;
        case CHAR:
          t->t = Prim::CHAR; break;
        case BYTE:
          t->t = Prim::BYTE; break;
        case SHORT:
          t->t = Prim::SHORT; break;
        case USHORT:
          t->t = Prim::USHORT; break;
        case INT:
          t->t = Prim::INT; break;
        case UINT:
          t->t = Prim::UINT; break;
        case LONG:
          t->t = Prim::LONG; break;
        case ULONG:
          t->t = Prim::ULONG; break;
        case FLOAT:
          t->t = Prim::FLOAT; break;
        case DOUBLE:
          t->t = Prim::DOUBLE; break;
        case VOID:
          t->t = Prim::VOID; break;
        case ERROR_TYPE:
          t->t = Prim::ERROR; break;
        case FUNCTYPE:
          pure = true;
          //fall through!
        case PROCTYPE:
          {
            bool isStatic = acceptKeyword(STATIC);
            Type* retType = parseType(s);
            expectPunct(LPAREN);
            Punct colon(COLON);
            vector<Type*> params;
            while(!acceptPunct(RPAREN))
            {
              //if "IDENT :" are next two tokens, accept and discard
              //(parameter names are optional in callable types)
              if(lookAhead(0)->type == IDENTIFIER && lookAhead(1)->compareTo(&colon))
              {
                accept();
                accept();
              }
              params.push_back(parseType(s));
            }
            t->t = UnresolvedType::Callable(pure, isStatic, retType, params);
            break;
          }
        default:
          err("expected type");
      }
    }
    else if(acceptPunct(LPAREN))
    {
      //parens always give the overall type high-precedence,
      //but expect high-precedence type(s) inside
      Type* first = parseType(s);
      if(acceptPunct(COMMA))
      {
        //tuple
        UnresolvedType::TupleList types;
        types.push_back(first);
        do
        {
          types.push_back(parseType(s));
        }
        while(acceptPunct(COMMA));
        t->t = types;
      }
      else if(acceptPunct(COLON))
      {
        //map
        t->t = UnresolvedType::Map(first, parseType(s));
      }
      else if(acceptOper(BOR))
      {
        //union
        UnresolvedType::UnionList types;
        types.push_back(first);
        do
        {
          types.push_back(parseType(s));
        }
        while(acceptOper(BOR));
        t->t = types;
      }
      else
      {
        err("expected tuple, map or union type");
      }
      expectPunct(RPAREN);
    }
    else if(lookAhead()->getType() == IDENTIFIER)
    {
      //a named type
      t->t = parseMember();
    }
    else
    {
      //unexpected token type
      err("expected a type");
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
        t->arrayDims++;
      }
      else if(acceptPunct(QUESTION))
      {
        UnresolvedType::UnionList optionalTypes;
        optionalTypes.push_back(t);
        optionalTypes.push_back(primitives[Prim::ERROR]);
        t = new UnresolvedType;
        t->scope = s;
        t->t = optionalTypes;
      }
    }
    return t;
  }

  Member* parseMember()
  {
    Member* m = new Member;
    m->setLocation(lookAhead());
    m->names.push_back(expectIdent());
    while(acceptPunct(DOT))
    {
      m->names.push_back(expectIdent());
    }
    return m;
  }

  void parseSubroutine(Scope* s)
  {
    Node* location = lookAhead();
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
    string name = expectIdent();
    expectPunct(LPAREN);
    vector<string> argNames;
    vector<Type*> argTypes;
    while(!acceptPunct(RPAREN))
    {
      //all arguments must be given names
      argNames.push_back(expectIdent());
      expectPunct(COLON);
      argTypes.push_back(parseType(s));
    }
    //Subroutine constructor constructs body
    Subroutine* subr = new Subroutine(s, name, isStatic, pure, retType, argNames, argTypes);
    subr->setLocation(location);
    parseBlock(subr->body);
    s->addName(subr);
  }

  Assign* parseVarDecl(Scope* s, bool semicolon)
  {
    Node* loc = lookAhead();
    Ident* id = expectIdent();
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
    Expression* init = nullptr;
    if(acceptOper(ASSIGN))
    {
      init = parseExpression(s);
    }
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
      var = new Variable(s, id->name, type, init, isStatic, compose);
    }
    if(semicolon)
      expectPunct(SEMICOLON);
    var->setLocation(loc);
    //add variable to scope
    s->addName(var);
    if(s->node.is<Block*>())
    {
      return new Assign(s->node.get<Block*>(), new VarExpr(var), init);
    }
    else
    {
      return nullptr;
    }
  }

  ForC* parseForC(Block* b)
  {
    ForC* fc = new ForC(b);
    fc->setLocation(lookAhead());
    expectKeyword(FOR);
    expectPunct(LPAREN);
    //note: all 3 parts of the ForC are optional
    if(!acceptPunct(SEMICOLON))
    {
      fc->init = parseStatementOrDecl(fc->outer, true);
    }
    if(!acceptPunct(SEMICOLON))
    {
      fc->condition = parseExpression(fc->outer->scope);
    }
    if(!acceptPunct(RPAREN))
    {
      //disallow declarations in the increment
      fc->increment = parseStatement(fc->outer, false);
      expectPunct(RPAREN);
    }
    //now parse the body as a regular statement
    auto body = parseStatement(fc->inner, true);
    fc->inner->addStatement(body);
    return fc;
  }

  ForArray* parseForArray(Block* b)
  {
    ForArray* fa = new ForArray(b);
    fa->setLocation(lookAhead());
    vector<string> tup;
    expectKeyword(FOR);
    expectPunct(LBRACKET);
    tup.push_back(expectIdent());
    while(acceptPunct(COMMA))
    {
      tup.push_back(expectIdent());
    }
    expectPunct(RBRACKET);
    if(tup.size() < 2)
    {
      errMsgLoc(fa, "for over array requires an iterator and at least one counter");
    }
    fa->createIterators(tup);
    auto body = parseStatement(fc->inner, true);
    fc->inner->addStatement(body);
    return fa;
  }

  ForRange* parseForRange(Block* b)
  {
    Node* location = lookAhead();
    expectKeyword(FOR);
    string counterName = expectIdent();
    expectPunct(COLON);
    Expression* begin = parseExpression(b->scope);
    Expression* end = parseExpression(b->scope);
    ForRange* fr = new ForRange(b, counterName, begin, end);
    fr->setLocation(lookAhead());
    auto body = parseStatement(fr->inner, true);
    fr->inner->addStatement(body);
    return fr;
  }

  void parseAlias(Scope* s)
  {
    expectKeyword(TYPEDEF);
    Type* t = parseType(s);
    s->addName(new AliasType(expectIdent(), t));
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
    m->varName = expectIdent();
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

  If(Block* b, Expression* condition, Statement* body);
  If(Block* b, Expression* condition, Statement* tbody, Statement* fbody);

  void parseTest(Scope* s)
  {
    Node* location = lookAhead();
    Block* b = new Block(s);
    parseBlock(b);
    Test* t = new Test(s, b);
    t->setLocation(location);
    return t;
  }

  Statement* parseStatement(Block* b, bool semicolon)
  {
    Token* next = lookAhead();
    if(next->type == KEYWORD)
    {
      switch(((Keyword*) next)->kw)
      {
        case FOR:
          {
            Punct lparen(LPAREN);
            Punct lbrack(LBRACKET);
            Token* next2 = lookAhead(1);
            if(next2->compareTo(&lparen))
              return parseForC(b);
            else if(next2->compareTo(&lbrack))
              return parseForArray(b);
            else
              return parseForRange(b);
          }
        case IF:
          return parseIf(b);
        case WHILE:
          return parseWhile(b);
        case BREAK:
          {
            expectKeyword(BREAK);
            if(semicolon)
              expectPunct(SEMICOLON);
            return new Break(b);
          }
        case CONTINUE:
          {
            expectKeyword(CONTINUE);
            if(semicolon)
              expectPunct(SEMICOLON);
            return new Continue(b);
          }
        case RETURN:
          {
            expectKeyword(RETURN);
            if(semicolon)
              expectPunct(SEMICOLON);
            return new Return;
          }
        case SWITCH:
          return parseSwitch(b);
        case MATCH:
          return parseMatch(b);
      }
    }
    if(next->type == IDENTIFIER)
    {
      //statement must be either a call or an assign
      //in either case, parse an expression first
      Expression* lhs = parseExpression(b);
      if(Oper* op = (Oper*) accept(OPERATOR))
      {
        //op must be compatible with assignment
        //++ and -- don't have explicit RHS, all others do
        if(op->op == INC || op->op == DEC)
          return new Assign(lhs, op->op);
        else
          return new Assign(lhs, op->op, parseExpression(b->scope));
      }
    }
  }

  If* parseIf(Block* b)
  {
    Node* location = lookAhead();
    expectKeyword(IF);
    expectPunct(LPAREN);
    Expression* cond = parseExpression(b->scope);
    expectPunct(RPAREN);
    If* i = nullptr;
    Statement* ifBody = parseStatement(b, true);
    if(acceptKeyword(ELSE))
    {
      Statment* elseBody = parseStatement(b, true);
      i = new If(b, cond, ifBody, elseBody);
    }
    else
    {
      i = new If(b, cond, ifBody);
    }
    i->setLocation(location);
    return i;
  }

  While* parseWhile(Block* b)
  {
    Node* location = lookAhead();
    expectKeyword(WHILE);
    expectPunct(LPAREN);
    Expression* cond = parseExpression(b->scope);
    expectPunct(RPAREN);
    While* w = new While(b, cond);
    w->setLocation(location);
    w->body->addStatement(parseStatement(w->body));
    return w;
  }

  void parseStatementOrDecl(Block* b, bool semicolon)
  {
    Token* next = lookAhead(0);
    Token* next2 = lookAhead(1);
    Punct colon(COLON);
    if(next->type == IDENTIFIER && next2->type->compareTo(&colon))
    {
      //variable declaration
      parseVarDecl(b->scope, semicolon);
    }
    else if(next->type == IDENTIFIER)
    {
      return parseStatement(b);
    }
    else if(next->type == KEYWORD)
    {
      int kw = ((Keyword*) next)->kw;
      switch(kw)
      {
        case STRUCT:
        case FUNC:
        case PROC:
        case TEST:
        case MODULE:
        case TYPEDEF:
        case ENUM:
          {
            parseScopedDecl(b->scope, semicolon);
            break;
          }
        case RETURN:
        case FOR:
        case IF:
        case WHILE:
        case SWITCH:
        case MATCH:
        case PRINT:
          {
            b->addStatement(parseStatement(b));
            break;
          }
        default:
          INTERNAL_ERROR;
      }
    }
    else if(next->type == PUNCTUATION)
    {
      b->addStatement(parseStatement(b));
    }
    else
    {
      err("Expected statement or declaration");
    }
  }

  void parseBlock(Block* b)
  {
    expectPunct(LBRACE);
    while(!acceptPunct(RBRACE))
    {
      Statement* stmt = parseStatementOrDecl(b);
      if(stmt)
        b->addStatement(stmt);
    }
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
    es->c = expectIdent();
    return es;
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

  string expectIdent()
  {
    Ident* i = (Ident*) expect(IDENTIFIER);
    return i->name;
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

