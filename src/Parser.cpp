#include "Parser.hpp"
//#include "Meta.hpp"
#include "Scope.hpp"
#include "TypeSystem.hpp"
#include "Subroutine.hpp"
#include "Expression.hpp"
#include "Variable.hpp"

extern Module* global;

//Macros to help parse common patterns
//func should be the whole call, i.e. parseThing(s)
//end should be a token
#define PARSE_STAR(list, func, end) \
  while(!accept(end)) \
  { \
    list.push_back(func); \
  }

#define PARSE_STAR_COMMA(list, func, end) \
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

#define PARSE_PLUS_COMMA(list, func, end) \
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

  void parseProgram()
  {
    pos = 0;
    while(!accept(PastEOF::inst))
    {
      parseScopedDecl(global->scope, true);
    }
  }

  void parseModule(Scope* s)
  {
    Node* loc = lookAhead();
    expectKeyword(MODULE);
    string name = expectIdent();
    Module* m = new Module(name, s);
    m->setLocation(loc);
    expectPunct(LBRACE);
    while(!acceptPunct(RBRACE))
    {
      parseScopedDecl(m->scope, true);
    }
    s->addName(m);
  }

  void parseStruct(Scope* s)
  {
    Node* loc = lookAhead();
    expectKeyword(STRUCT);
    auto structType = new StructType(expectIdent(), s);
    structType->setLocation(loc);
    s->addName(structType);
    expectPunct(LBRACE);
    while(!acceptPunct(RBRACE))
    {
      parseScopedDecl(structType->scope, true);
    }
  }

  void parseScopedDecl(Scope* s, bool semicolon)
  {
    cout << "Parsing scoped decl, next: " << lookAhead()->getStr() << '\n';
    Punct colon(COLON);
    if(Keyword* kw = dynamic_cast<Keyword*>(lookAhead()))
    {
      switch(kw->kw)
      {
        case FUNC:
        case PROC:
          parseSubroutine(s);
          return;
        case EXTERN:
          parseExternalSubroutine(s);
          return;
        case STRUCT:
          parseStruct(s);
          return;
        case TYPEDEF:
          parseAlias(s);
          if(semicolon)
            expectPunct(SEMICOLON);
          return;
        case ENUM:
          parseEnum(s);
          return;
        case MODULE:
          parseModule(s);
          return;
        case TEST:
          parseTest(s);
          return;
        default:
          INTERNAL_ERROR;
      }
    }
    else if(lookAhead()->type == IDENTIFIER)
    {
      //variable declaration
      parseVarDecl(s);
      if(semicolon)
        expectPunct(semicolon);
      return;
    }
    INTERNAL_ERROR;
  }

  Type* parseType(Scope* s)
  {
    UnresolvedType* t = new UnresolvedType;
    t->scope = s;
    t->setLocation(lookAhead());
    t->arrayDims = 0;
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
        vector<Type*> types;
        types.push_back(first);
        do
        {
          types.push_back(parseType(s));
        }
        while(acceptPunct(COMMA));
        t->t = UnresolvedType::Tuple(types);
      }
      else if(acceptPunct(COLON))
      {
        //map
        t->t = UnresolvedType::Map(first, parseType(s));
      }
      else if(acceptOper(BOR))
      {
        //union
        vector<Type*> types;
        types.push_back(first);
        do
        {
          types.push_back(parseType(s));
        }
        while(acceptOper(BOR));
        t->t = UnresolvedType::Union(types);
      }
      else
      {
        err("expected tuple, map or union type");
      }
      expectPunct(RPAREN);
    }
    else if(lookAhead()->type == IDENTIFIER)
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
        vector<Type*> optionalTypes;
        optionalTypes.push_back(t);
        optionalTypes.push_back(primitives[Prim::ERROR]);
        t = new UnresolvedType;
        t->scope = s;
        t->t = UnresolvedType::Union(optionalTypes);
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
      pure = true;
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

  void parseExternalSubroutine(Scope* s)
  {
    Node* loc = lookAhead();
    expectKeyword(EXTERN);
    Type* retType = parseType(s);
    string name = expectIdent();
    expectPunct(LPAREN);
    vector<string> argNames;
    vector<Type*> argTypes;
    while(!acceptPunct(RPAREN))
    {
      argNames.push_back(expectIdent());
      expectPunct(COLON);
      argTypes.push_back(parseType(s));
    }
    string& code = ((StrLit*) expect(STRING_LITERAL))->val;
    ExternalSubroutine* es = new ExternalSubroutine(s, name, retType, argTypes, argNames, code);
    es->setLocation(loc);
    s->addName(es);
  }

  Assign* parseVarDecl(Scope* s)
  {
    Node* loc = lookAhead();
    string name = expectIdent();
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
    Type* type = nullptr;
    bool isAuto = false;
    if(acceptKeyword(AUTO))
      isAuto = true;
    else
      type = parseType(s);
    Expression* init = nullptr;
    if(acceptOper(ASSIGN))
    {
      init = parseExpression(s);
    }
    if(!init && isAuto)
    {
      errMsgLoc(loc, "auto-typed variable requires initialization");
    }
    if(isAuto)
    {
      type = new ExprType(init);
    }
    //create the variable and add to scope
    Variable* var;
    if(s->node.is<Block*>())
    {
      //local variable uses special constructor
      var = new Variable(name, type, s->node.get<Block*>());
    }
    else
    {
      //at parse time, if a variable is static, make sure it's in a struct
      if(!s->getMemberContext() && isStatic)
      {
        err("static variable declared outside any struct");
      }
      var = new Variable(s, name, type, init, isStatic, compose);
    }
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
    auto body = parseStatement(fa->inner, true);
    fa->inner->addStatement(body);
    return fa;
  }

  ForRange* parseForRange(Block* b)
  {
    Node* loc = lookAhead();
    expectKeyword(FOR);
    string counterName = expectIdent();
    expectPunct(COLON);
    Expression* begin = parseExpression(b->scope);
    Expression* end = parseExpression(b->scope);
    ForRange* fr = new ForRange(b, counterName, begin, end);
    fr->setLocation(loc);
    auto body = parseStatement(fr->inner, true);
    fr->inner->addStatement(body);
    return fr;
  }

  Switch* parseSwitch(Block* b)
  {
    Node* loc = lookAhead();
    expectKeyword(SWITCH);
    expectPunct(LPAREN);
    Expression* switched = parseExpression(b->scope);
    expectPunct(RPAREN);
    expectPunct(LBRACE);
    vector<Statement*> stmts;
    vector<Expression*> caseValues;
    vector<int> caseIndices;
    int defaultPos = -1;
    Block* block = new Block(b);
    Keyword defaultKW(DEFAULT);
    while(!acceptPunct(RBRACE))
    {
      if(acceptKeyword(CASE))
      {
        caseValues.push_back(parseExpression(b->scope));
        caseIndices.push_back(block->statementCount);
        expectPunct(COLON);
      }
      else if(lookAhead()->compareTo(&defaultKW))
      {
        if(defaultPos >= 0)
        {
          err("default in switch can only be defined once");
        }
        accept();
        expectPunct(COLON);
      }
      else
      {
        block->addStatement(parseStatement(b, true));
      }
    }
    //place implicit "default:" after all statements if not explicit
    if(defaultPos == -1)
    {
      defaultPos = block->statementCount;
    }
    Switch* switchStmt = new Switch(b, switched, caseIndices, caseValues, defaultPos, block);
    switchStmt->setLocation(loc);
    return switchStmt;
  }

  Match* parseMatch(Block* b)
  {
    Node* loc = lookAhead();
    expectKeyword(MATCH);
    string varName = expectIdent();
    expectPunct(COLON);
    Expression* matched = parseExpression(b->scope);
    expectPunct(LBRACE);
    vector<Type*> caseTypes;
    vector<Block*> caseBlocks;
    while(!acceptPunct(RBRACE))
    {
      caseTypes.push_back(parseType(b->scope));
      Block* block = new Block(b);
      parseBlock(block);
      caseBlocks.push_back(block);
    }
    Match* matchStmt = new Match(b, matched, varName, caseTypes, caseBlocks);
    matchStmt->setLocation(loc);
    return matchStmt;
  }

  void parseAlias(Scope* s)
  {
    Node* loc = lookAhead();
    expectKeyword(TYPEDEF);
    Type* t = parseType(s);
    AliasType* aType = new AliasType(expectIdent(), t, s);
    aType->setLocation(loc);
    s->addName(aType);
  }

  void parseEnum(Scope* s)
  {
    EnumType* e = new EnumType(s);
    e->setLocation(lookAhead());
    expectKeyword(ENUM);
    expectPunct(LBRACE);
    while(true)
    {
      string name = expectIdent();
      if(acceptOper(ASSIGN))
      {
        bool sign = acceptOper(SUB);
        int64_t value = ((IntLit*) expect(INT_LITERAL))->val;
        if(sign)
          value = -value;
        e->addValue(name, value);
      }
      else
      {
        e->addValue(name);
      }
      if(!acceptPunct(COMMA))
      {
        expectPunct(RBRACE);
        break;
      }
    }
    s->addName(e);
  }

  void parseTest(Scope* s)
  {
    Node* location = lookAhead();
    Block* b = new Block(s);
    parseBlock(b);
    Test* t = new Test(s, b);
    t->setLocation(location);
    //test constructor adds it to a static list of all tests;
    //it is not added to any scope
  }

  Statement* parseStatementOrDecl(Block* b, bool semicolon)
  {
    cout << "Parsing stmt/decl\n";
    cout << "Next token: " << lookAhead()->getStr() << '\n';
    Token* next = lookAhead(0);
    Token* next2 = lookAhead(1);
    Punct colon(COLON);
    if(next->type == IDENTIFIER && next2->compareTo(&colon))
    {
      //variable declaration
      parseVarDecl(b->scope);
      if(semicolon)
        expectPunct(SEMICOLON);
    }
    else if(next->type == IDENTIFIER)
    {
      return parseStatement(b, semicolon);
    }
    else if(next->type == KEYWORD)
    {
      int kw = ((Keyword*) next)->kw;
      switch(kw)
      {
        case STRUCT:
        case FUNC:
        case PROC:
        case EXTERN:
        case MODULE:
        case TYPEDEF:
        case ENUM:
        case TEST:
          {
            parseScopedDecl(b->scope, semicolon);
            return nullptr;
          }
        case RETURN:
        case FOR:
        case IF:
        case WHILE:
        case SWITCH:
        case MATCH:
        case PRINT:
          {
            return parseStatement(b, semicolon);
          }
        default:
          INTERNAL_ERROR;
      }
    }
    else if(next->type == PUNCTUATION)
    {
      return parseStatement(b, semicolon);
    }
    else
    {
      err("Expected statement or declaration");
    }
    return nullptr;
  }

  Statement* parseStatement(Block* b, bool semicolon)
  {
    cout << "Parsing stmt, next: " << lookAhead()->getStr() << '\n';
    Token* next = lookAhead();
    Punct lbrack(LBRACKET);
    Punct rparen(RPAREN);
    Node* loc = lookAhead();
    if(next->type == KEYWORD)
    {
      switch(((Keyword*) next)->kw)
      {
        case FOR:
          {
            Punct lparen(LPAREN);
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
            if(!semicolon)
            {
              err("can't use break statement here");
            }
            accept();
            expectPunct(SEMICOLON);
            Break* brk = new Break(b);
            brk->setLocation(loc);
            return brk;
          }
        case CONTINUE:
          {
            accept();
            if(!semicolon)
            {
              err("can't use continue statement here");
            }
            expectPunct(SEMICOLON);
            Continue* cont = new Continue(b);
            cont->setLocation(loc);
            return cont;
          }
        case RETURN:
          {
            if(!semicolon)
            {
              err("can't use return statement here");
            }
            accept();
            Return* ret = nullptr;
            if(!acceptPunct(SEMICOLON))
            {
              ret = new Return(b, parseExpression(b->scope));
              expectPunct(SEMICOLON);
            }
            else
            {
              ret = new Return(b);
            }
            ret->setLocation(loc);
            return ret;
          }
        case PRINT:
          {
            accept();
            expectPunct(LPAREN);
            vector<Expression*> exprs;
            PARSE_PLUS_COMMA(exprs, parseExpression(b->scope), rparen);
            if(semicolon)
              expectPunct(SEMICOLON);
            Print* printStmt = new Print(b, exprs);
            printStmt->setLocation(loc);
            return printStmt;
          }
        case SWITCH:
          return parseSwitch(b);
        case MATCH:
          return parseMatch(b);
        default:
          err("expected statement");
      }
    }
    else if(next->type == IDENTIFIER || next->compareTo(&lbrack))
    {
      //statement must be either a call or an assign
      //in either case, parse an expression first
      Expression* lhs = parseExpression(b->scope);
      if(Oper* op = (Oper*) accept(OPERATOR))
      {
        //op must be compatible with assignment
        //++ and -- don't have explicit RHS, all others do
        Assign* assign = nullptr;
        if(op->op == INC || op->op == DEC)
          assign = new Assign(b, lhs, op->op);
        else
          assign = new Assign(b, lhs, op->op, parseExpression(b->scope));
        assign->setLocation(loc);
        return assign;
      }
      else
      {
        CallExpr* ce = dynamic_cast<CallExpr*>(lhs);
        if(!ce)
        {
          errMsgLoc(lhs, "this expression can't be used as statement");
        }
        CallStmt* cs = new CallStmt(b, ce);
        cs->setLocation(loc);
        return cs;
      }
    }
    else if(next->type == PUNCTUATION)
    {
      //only legal statement here is block
      Block* block = new Block(b);
      parseBlock(block);
      return block;
    }
    err("invalid statement");
    return nullptr;
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
      Statement* elseBody = parseStatement(b, true);
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
    w->body->addStatement(parseStatement(w->body, true));
    return w;
  }

  void parseBlock(Block* b)
  {
    b->setLocation(lookAhead());
    expectPunct(LBRACE);
    while(!acceptPunct(RBRACE))
    {
      Statement* stmt = parseStatementOrDecl(b, true);
      if(stmt)
        b->addStatement(stmt);
    }
  }

  Expression* parseExpression(Scope* s, int prec)
  {
    Node* location = lookAhead();
    Punct rparen(RPAREN);
    Punct rbrack(RBRACKET);
    //All expressions are prec >= 0
    //Binary expressions are prec 1-11
    //Unary expressions are prec 12
    //Others are prec 13
    if(prec == 0)
    {
      if(acceptKeyword(ARRAY))
      {
        Type* elem = parseType(s);
        vector<Expression*> dims;
        while(acceptPunct(LBRACKET))
        {
          dims.push_back(parseExpression(s));
          expectPunct(RBRACKET);
        }
        NewArray* na = new NewArray(elem, dims);
        na->setLocation(location);
        return na;
      }
      else
      {
        return parseExpression(s, 1);
      }
    }
    else if(prec >= 1 && prec <= 11)
    {
      Expression* lhs = parseExpression(s, prec + 1);
      while(true)
      {
        Token* next = lookAhead();
        if(next->type != OPERATOR)
          break;
        Oper* op = (Oper*) next;
        if(operatorPrec[op->op] != prec)
          break;
        Expression* rhs = parseExpression(s, prec + 1);
        lhs = new BinaryArith(lhs, op->op, rhs);
        lhs->setLocation(op);
      }
      return lhs;
    }
    else if(prec == 12)
    {
      //unary expressions
      while(lookAhead()->type == OPERATOR)
      {
        int op = ((Oper*) lookAhead())->op;
        if(op == SUB || op == LNOT || op == BNOT)
        {
          UnaryArith* ua = new UnaryArith(op, parseExpression(s, prec));
          ua->setLocation(location);
          return ua;
        }
      }
      return parseExpression(s, prec + 1);
    }
    else
    {
      //highest precedence expressions
      Expression* base = nullptr;
      if(lookAhead()->type == IDENTIFIER)
      {
        base = new UnresolvedExpr(parseMember(), s); 
      }
      else if(acceptKeyword(THIS))
      {
        base = new ThisExpr(s);
      }
      else if(acceptKeyword(TRUE))
      {
        base = new BoolLiteral(true);
      }
      else if(acceptKeyword(FALSE))
      {
        base = new BoolLiteral(false);
      }
      else if(acceptKeyword(ERROR_VALUE))
      {
        base = new ErrorVal;
      }
      else if(auto intLit = (IntLit*) accept(INT_LITERAL))
      {
        base = new IntLiteral(intLit);
      }
      else if(auto floatLit = (FloatLit*) accept(FLOAT_LITERAL))
      {
        base = new FloatLiteral(floatLit);
      }
      else if(auto strLit = (StrLit*) accept(STRING_LITERAL))
      {
        base = new StringLiteral(strLit);
      }
      else if(auto charLit = (CharLit*) accept(CHAR_LITERAL))
      {
        base = new CharLiteral(charLit);
      }
      else if(acceptPunct(LPAREN))
      {
        //any-precedence expression in parentheses
        base = parseExpression(s);
        expectPunct(RPAREN);
      }
      else if(acceptPunct(LBRACKET))
      {
        vector<Expression*> exprs;
        PARSE_PLUS_COMMA(exprs, parseExpression(s), rbrack);
        //allow a single element in CompoundLiteral syntax,
        //but then the expression doesn't need to be a CompoundLiteral
        if(exprs.size() == 1)
          base = exprs[0];
        else
          base = new CompoundLiteral(exprs);
      }
      base->setLocation(location);
      //now that a base expression has been parsed, parse suffixes left->right
      while(true)
      {
        if(acceptPunct(LPAREN))
        {
          //call operator
          vector<Expression*> args;
          PARSE_STAR_COMMA(args, parseExpression(s), rparen);
          base = new CallExpr(base, args);
        }
        else if(acceptPunct(LBRACKET))
        {
          Expression* index = parseExpression(s);
          expectPunct(RBRACKET);
          base = new Indexed(base, index);
        }
        else if(acceptPunct(DOT))
        {
          base = new UnresolvedExpr(base, parseMember(), s);
        }
        else
        {
          break;
        }
        base->setLocation(location);
      }
      return base;
    }
    return nullptr;
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
    bool res = next->type == tokType;
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
    if(next->type == tokType)
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
    if(index >= tokens.size())
    {
      return &PastEOF::inst;
    }
    else
    {
      return tokens[index];
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

ostream& operator<<(ostream& os, const Member& mem)
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

