#include "Parser.hpp"
//#include "Meta.hpp"
#include "Scope.hpp"
#include "TypeSystem.hpp"
#include "Subroutine.hpp"
#include "Expression.hpp"
#include "Variable.hpp"
#include "SourceFile.hpp"
#include <exception>
#include <limits>

using std::numeric_limits;

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

void parseProgram(string mainSourcePath)
{
  Parser::Stream mainStream(getSourceFile(nullptr, mainSourcePath));
  while(!mainStream.accept(PastEOF::inst))
  {
    mainStream.parseDecl(global->scope, true);
  }
}

namespace Parser
{
  void Stream::parseModule(Scope* s)
  {
    Node* loc = lookAhead();
    expectKeyword(MODULE);
    string name = expectIdent();
    Module* m = new Module(name, s);
    m->setLocation(loc);
    expectPunct(LBRACE);
    while(!acceptPunct(RBRACE))
    {
      parseDecl(m->scope, true);
    }
    s->addName(m);
  }

  void Stream::parseStruct(Scope* s)
  {
    Node* loc = lookAhead();
    expectKeyword(STRUCT);
    auto structType = new StructType(expectIdent(), s);
    structType->setLocation(loc);
    s->addName(structType);
    expectPunct(LBRACE);
    while(!acceptPunct(RBRACE))
    {
      parseDecl(structType->scope, true);
    }
  }

  Statement* Stream::parseDecl(Scope* s, bool semicolon)
  {
    Node* loc = lookAhead();
    Punct colon(COLON);
    Punct lparen(LPAREN);
    Oper composeOperator(BXOR);
    if(acceptPunct(HASH))
    {
      string id = expectIdent();
      if(id == "include")
      {
        if(!s->isNestedModule())
        {
          errMsgLoc(loc, "can only #include files in a module or submodule.\n");
        }
        expectPunct(LPAREN);
        StrLit* str = (StrLit*) expect(STRING_LITERAL);
        expectPunct(RPAREN);
        expectPunct(SEMICOLON);
        SourceFile* includedFile = getSourceFile(loc, str->val);
        Module* module = s->node.get<Module*>();
        if(!module->hasInclude(includedFile))
        {
          module->included.insert(includedFile);
          Stream subStream(includedFile);
          //parse scoped decls until EOF of the included file
          while(!subStream.accept(PastEOF::inst))
          {
            subStream.parseDecl(module->scope, true);
          }
        }
        return nullptr;
      }
      else
      {
        errMsgLoc(loc, "TODO: undefined meta-statement.\n");
      }
    }
    else if(Keyword* kw = dynamic_cast<Keyword*>(lookAhead()))
    {
      bool parsingSubr = false;
      if(kw->kw == FUNC || kw->kw == PROC)
        parsingSubr = true;
      else if(kw->kw == STATIC)
      {
        Keyword* after = dynamic_cast<Keyword*>(lookAhead(1));
        if(after && (after->kw == FUNC || after->kw == PROC))
          parsingSubr = true;
      }
      if(parsingSubr)
      {
        parseSubroutineDecl(s);
        return nullptr;
      }
      //otherwise, "static" precedes a VarDecl
      switch(kw->kw)
      {
        case STRUCT:
          parseStruct(s);
          return nullptr;
        case TYPEDEF:
          parseAlias(s);
          if(semicolon)
            expectPunct(SEMICOLON);
          return nullptr;
        case TYPE:
          {
            parseSimpleType(s);
            if(semicolon)
              expectPunct(SEMICOLON);
            return nullptr;
          }
        case ENUM:
          parseEnum(s);
          return nullptr;
        case MODULE:
          parseModule(s);
          return nullptr;
        case TEST:
          parseTest(s);
          return nullptr;
        case VOID:
        case ERROR:
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
        case FUNCTYPE:
        case PROCTYPE:
        case STATIC:
          {
            auto varInit = parseVarDecl(s);
            if(semicolon)
              expectPunct(SEMICOLON);
            if(varInit)
            {
              //varInit exists, so know scope is a block
              s->node.get<Block*>()->addStatement(varInit);
            }
            return varInit;
          }
        default:
          errMsgLoc(loc, "expected a declaration");
      }
    }
    else if(lookAhead()->type == IDENTIFIER ||
        lookAhead()->compareTo(&composeOperator) ||
        lookAhead()->compareTo(&lparen))
    {
      //variable declaration
      auto varInit = parseVarDecl(s);
      if(semicolon)
        expectPunct(SEMICOLON);
      if(varInit)
      {
        //varInit exists, so know scope is a block
        s->node.get<Block*>()->addStatement(varInit);
      }
      return nullptr;
    }
    else
    {
      errMsgLoc(loc, "expected decl but got " + lookAhead()->getStr() + '\n');
      INTERNAL_ERROR;
    }
    return nullptr;
  }

  Type* Stream::parseType(Scope* s)
  {
    UnresolvedType* t = new UnresolvedType;
    t->scope = s;
    Node* loc = lookAhead();
    t->arrayDims = 0;
    //check for keyword
    if(Keyword* keyword = (Keyword*) accept(KEYWORD))
    {
      bool pure = false;
      //all possible types now (except Callables) are primitive, so set kind
      switch(keyword->kw)
      {
        case VOID:
          t->t = Prim::VOID; break;
        case ERROR:
          t->t = Prim::ERROR; break;
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
        case FUNCTYPE:
          pure = true;
          //fall through
        case PROCTYPE:
          {
            bool isStatic = acceptKeyword(STATIC);
            Type* retType = parseType(s);
            expectPunct(LPAREN);
            Punct colon(COLON);
            vector<Type*> params;
            while(!acceptPunct(RPAREN))
            {
              params.push_back(parseType(s));
              //accept an optional parameter name
              accept(IDENTIFIER);
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
        cout << "Parsed tuple with " << types.size() << " members\n";
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
        //use an actual UnionType (not an UnresolvedType) here,
        //since only UnionType::resolve can handle circular
        //dependencies correctly
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
    if(!t)
      return nullptr;
    t->setLocation(loc);
    Punct lbrack(LBRACKET);
    Punct rbrack(RBRACKET);
    Punct quest(QUESTION);
    //check for square bracket pairs after, indicating array type
    while((lookAhead(0)->compareTo(&lbrack) && lookAhead(1)->compareTo(&rbrack))
        || lookAhead()->compareTo(&quest))
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
        optionalTypes.push_back(primitives[Prim::VOID]);
        t = new UnresolvedType;
        t->setLocation(loc);
        t->scope = s;
        t->t = UnresolvedType::Union(optionalTypes);
      }
    }
    return t;
  }

  Member* Stream::parseMember()
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

  void Stream::parseSubroutineDecl(Scope* s)
  {
    Node* location = lookAhead();
    bool isStatic = false;
    if(acceptKeyword(STATIC))
      isStatic = true;
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
    auto sd = new SubroutineDecl(expectIdent(), s, pure, isStatic);
    sd->setLocation(location);
    Keyword extrn(EXTERN);
    while(acceptPunct(COLON))
    {
      if(lookAhead()->compareTo(&extrn))
      {
        parseExternalSubroutine(sd);
      }
      else
      {
        parseSubroutine(sd);
      }
    }
    s->addName(sd);
  }

  void Stream::parseSubroutine(SubroutineDecl* sd)
  {
    Subroutine* subr = new Subroutine(sd);
    subr->setLocation(lookAhead());
    sd->overloads.push_back(subr);
    Scope* outer = sd->scope;
    Type* retType = parseType(outer);
    vector<Variable*> params;
    expectPunct(LPAREN);
    while(!acceptPunct(RPAREN))
    {
      Type* paramType = parseType(outer);
      string paramName = expectIdent();
      Node* ploc = lookAhead();
      Variable* param = new Variable(subr->scope, paramName, paramType, nullptr, false);
      subr->scope->addName(param);
      param->setLocation(ploc);
      params.push_back(param);
    }
    subr->setSignature(retType, params);
    //Finally, parse the body statements (body has already been created)
    expectPunct(LBRACE);
    while(!acceptPunct(RBRACE))
    {
      Statement* stmt = parseStatementOrDecl(subr->body, true);
      if(stmt)
        subr->body->addStatement(stmt);
    }
  }

  void Stream::parseExternalSubroutine(SubroutineDecl* sd)
  {
    errMsgLoc(lookAhead(), "External subroutines haven't been implemented yet");
    /*
    Node* loc = lookAhead();
    expectKeyword(EXTERN);
    Type* retType = parseType(sd->scope);
    string name = expectIdent();
    expectPunct(LPAREN);
    vector<string> paramNames;
    vector<Type*> paramTypes;
    while(!acceptPunct(RPAREN))
    {
      borrow.push_back(acceptKeyword(CONST));
      paramTypes.push_back(parseType(s));
      paramNames.push_back(expectIdent());
    }
    string& code = ((StrLit*) expect(STRING_LITERAL))->val;
    ExternalSubroutine* es = new ExternalSubroutine(s, name, retType, paramTypes, paramNames, borrow, code);
    es->setLocation(loc);
    s->addName(es);
    */
  }

  Assign* Stream::parseVarDecl(Scope* s)
  {
    Node* loc = lookAhead();
    bool isStatic = false;
    bool compose = false;
    bool isAuto = false;
    Type* type = nullptr;
    if(acceptOper(BXOR))
    {
      compose = true;
    }
    else if(acceptKeyword(STATIC))
    {
      isStatic = true;
    }
    if(acceptKeyword(AUTO))
      isAuto = true;
    else
      type = parseType(s);
    string name = expectIdent();
    Expression* init = nullptr;
    if(acceptOper(ASSIGN))
    {
      init = parseExpression(s);
    }
    if(!init && isAuto)
    {
      errMsgLoc(loc, "auto-typed variable requires initializing expression");
    }
    if(isAuto)
    {
      type = new ExprType(init);
    }
    //create the variable and add to scope
    Variable* var = nullptr;
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
      //local vars don't store initial value internally
      //so return a statement that initializes it
      if(!init)
      {
        init = new DefaultValueExpr(var->type);
      }
      Assign* a = new Assign(s->node.get<Block*>(), new VarExpr(var), init);
      a->setLocation(loc);
      return a;
    }
    return nullptr;
  }

  ForC* Stream::parseForC(Block* b)
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
      expectPunct(SEMICOLON);
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

  ForArray* Stream::parseForArray(Block* b)
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
    expectPunct(COLON);
    fa->arr = parseExpression(b->scope);
    fa->createIterators(tup);
    Statement* body = parseStatement(fa->inner, true);
    fa->inner->addStatement(body);
    return fa;
  }

  ForRange* Stream::parseForRange(Block* b)
  {
    Node* loc = lookAhead();
    expectKeyword(FOR);
    string counterName = expectIdent();
    expectPunct(COLON);
    Expression* begin = parseExpression(b->scope);
    expectPunct(COMMA);
    Expression* end = parseExpression(b->scope);
    ForRange* fr = new ForRange(b, counterName, begin, end);
    fr->setLocation(loc);
    Statement* body = parseStatement(fr->inner, true);
    fr->inner->addStatement(body);
    return fr;
  }

  Switch* Stream::parseSwitch(Block* b)
  {
    Node* loc = lookAhead();
    expectKeyword(SWITCH);
    expectPunct(LPAREN);
    Expression* switched = parseExpression(b->scope);
    expectPunct(RPAREN);
    expectPunct(LBRACE);
    vector<Statement*> stmts;
    vector<int> caseIndices;
    Block* block = new Block(b);
    Switch* switchStmt = new Switch(b, switched, block);
    switchStmt->defaultPosition = -1;
    block->breakable = switchStmt;
    Keyword defaultKW(DEFAULT);
    while(!acceptPunct(RBRACE))
    {
      if(acceptKeyword(CASE))
      {
        switchStmt->caseValues.push_back(parseExpression(block->scope));
        switchStmt->caseLabels.push_back(block->stmts.size());
        expectPunct(COLON);
      }
      else if(acceptKeyword(DEFAULT))
      {
        if(switchStmt->defaultPosition >= 0)
        {
          err("default in switch can only be defined once");
        }
        switchStmt->defaultPosition = block->stmts.size();
        expectPunct(COLON);
      }
      else
      {
        block->addStatement(parseStatement(block, true));
      }
    }
    //place implicit "default:" after all statements if not explicit
    if(switchStmt->defaultPosition == -1)
    {
      switchStmt->defaultPosition = block->stmts.size();
    }
    switchStmt->setLocation(loc);
    return switchStmt;
  }

  Match* Stream::parseMatch(Block* b)
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
      expectKeyword(CASE);
      caseTypes.push_back(parseType(b->scope));
      expectPunct(COLON);
      Block* block = new Block(b);
      parseBlock(block);
      caseBlocks.push_back(block);
    }
    Match* matchStmt = new Match(b, matched, varName, caseTypes, caseBlocks);
    matchStmt->setLocation(loc);
    return matchStmt;
  }

  void Stream::parseAlias(Scope* s)
  {
    Node* loc = lookAhead();
    expectKeyword(TYPEDEF);
    Type* t = parseType(s);
    AliasType* aType = new AliasType(expectIdent(), t, s);
    aType->setLocation(loc);
    s->addName(aType);
  }

  void Stream::parseSimpleType(Scope* s)
  {
    Node* loc = lookAhead();
    expectKeyword(TYPE);
    string name = expectIdent();
    auto st = new SimpleType(name);
    st->setLocation(loc);
    s->addName(st);
  }

  void Stream::parseEnum(Scope* s)
  {
    Node* loc = lookAhead();
    expectKeyword(ENUM);
    string enumName = expectIdent();
    EnumType* e = new EnumType(enumName, s);
    e->setLocation(loc);
    expectPunct(LBRACE);
    while(true)
    {
      Node* valueLocation = lookAhead();
      string name = expectIdent();
      if(acceptOper(ASSIGN))
      {
        bool sign = acceptOper(SUB);
        uint64_t rawValue = ((IntLit*) expect(INT_LITERAL))->val;
        if(sign)
        {
          if(rawValue > (uint64_t) numeric_limits<int64_t>::max())
            errMsgLoc(valueLocation, "negative enum value can't fit in a long");
          //otherwise, is safe to convert to int64
          e->addNegativeValue(name, -rawValue, valueLocation);
        }
        else
          e->addPositiveValue(name, rawValue, valueLocation);
      }
      else
      {
        //let the enum automatically choose value
        e->addAutomaticValue(name, valueLocation);
      }
      if(!acceptPunct(COMMA))
      {
        expectPunct(RBRACE);
        break;
      }
    }
    s->addName(e);
  }

  void Stream::parseTest(Scope* s)
  {
    Node* location = lookAhead();
    Block* b = new Block(s);
    parseBlock(b);
    Test* t = new Test(s, b);
    t->setLocation(location);
    //test constructor adds it to a static list of all tests;
    //it is not added to any scope
  }

  Statement* Stream::parseStatementOrDecl(Block* b, bool semicolon)
  {
    Token* next = lookAhead();
    Punct lbrace(LBRACE);
    Punct lparen(LPAREN);
    Punct lbracket(LBRACKET);
    if(next->type == IDENTIFIER)
    {
      Stream s1(*this);
      Stream s2(*this);
      //with errors disabled, parsing failure will just throw
      s1.emitErrors = false;
      s2.emitErrors = false;
      Type* t = nullptr;
      Expression* e = nullptr;
      try
      {
        t = s1.parseType(b->scope);
      }
      catch(...) {}
      try
      {
        e = s2.parseExpression(b->scope);
      }
      catch(...) {}
      Ident* id;
      if(t && (id = (Ident*) s1.accept(IDENTIFIER)))
      {
        //parse a VarDecl in the original stream
        auto stmt = parseVarDecl(b->scope);
        if(semicolon)
          expectPunct(SEMICOLON);
        return stmt;
      }
      else if(e)
      {
        //parse an assign or call
        return parseStatement(b, semicolon);
      }
      else
      {
        err("Expected statement or declaration.");
      }
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
        case TYPE:
        case ENUM:
        case TEST:
        case VOID:
        case ERROR:
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
        case FUNCTYPE:
        case PROCTYPE:
        case STATIC:
          {
            return parseDecl(b->scope, semicolon);
          }
        case RETURN:
        case BREAK:
        case CONTINUE:
        case FOR:
        case IF:
        case WHILE:
        case SWITCH:
        case MATCH:
        case PRINT:
        case ASSERT:
          {
            return parseStatement(b, semicolon);
          }
        default:;
      }
    }
    else if(lookAhead()->compareTo(&lbrace) ||
        lookAhead()->compareTo(&lbracket))
    {
      return parseStatement(b, semicolon);
    }
    else if(lookAhead()->compareTo(&lparen))
    {
      //start of union/tuple type
      return parseDecl(b->scope, semicolon);
    }
    err("Expected statement or declaration");
    return nullptr;
  }

  Statement* Stream::parseStatement(Block* b, bool semicolon)
  {
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
        case ASSERT:
          {
            accept();
            expectPunct(LPAREN);
            Assertion* as = new Assertion(b, parseExpression(b->scope));
            as->setLocation(loc);
            expectPunct(RPAREN);
            if(semicolon)
              expectPunct(SEMICOLON);
            return as;
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
        if(semicolon)
          expectPunct(SEMICOLON);
        assign->setLocation(loc);
        return assign;
      }
      else
      {
        CallExpr* ce = dynamic_cast<CallExpr*>(lhs);
        if(!ce)
        {
          errMsgLoc(lhs, "side-effect free expression can't be used as statement");
        }
        CallStmt* cs = new CallStmt(b, ce);
        if(semicolon)
          expectPunct(SEMICOLON);
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

  If* Stream::parseIf(Block* b)
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

  While* Stream::parseWhile(Block* b)
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

  void Stream::parseBlock(Block* b)
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

  Expression* Stream::parseExpression(Scope* s, int prec)
  {
    Node* location = lookAhead();
    Punct rparen(RPAREN);
    Punct rbrack(RBRACKET);
    //All expressions are prec >= 0
    //"is", "as" and "array" are prec 0
    //  NOTE:
    //  Array must be low prec, since otherwise
    //  an array index operator after it would be ambiguous.
    //  No reason to want to use operators with it anyway, so
    //  just don't allow that in the syntax
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
      return parseExpression(s, 1);
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
        accept();
        Expression* rhs = parseExpression(s, prec + 1);
        lhs = new BinaryArith(lhs, op->op, rhs);
        lhs->setLocation(op);
      }
      return lhs;
    }
    else if(prec == 12)
    {
      Expression* base = nullptr;
      //unary expressions (-!~), left to right
      if(lookAhead()->type == OPERATOR)
      {
        int op = ((Oper*) lookAhead())->op;
        if(op == SUB || op == LNOT || op == BNOT)
        {
          accept();
          UnaryArith* ua = new UnaryArith(op, parseExpression(s, prec));
          ua->setLocation(location);
          base = ua;
        }
        else
        {
          err("Expected unary operator (-, !, ~)");
        }
      }
      else
      {
        //parse innermost, highest precedence expr
        base = parseExpression(s, prec + 1);
      }
      //as/is suffixes, also left to right
      while(true)
      {
        if(acceptKeyword(IS))
        {
          IsExpr* ie = new IsExpr(base, parseType(s));
          ie->setLocation(base);
          base = ie;
        }
        else if(acceptKeyword(AS))
        {
          AsExpr* ae = new AsExpr(base, parseType(s));
          ae->setLocation(base);
          base = ae;
        }
        else
          break;
      }
      return base;
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
        base = new BoolConstant(true);
      }
      else if(acceptKeyword(FALSE))
      {
        base = new BoolConstant(false);
      }
      else if(acceptKeyword(VOID))
      {
        base = ((SimpleType*) primitives[Prim::VOID])->val;
      }
      else if(acceptKeyword(ERROR))
      {
        base = ((SimpleType*) primitives[Prim::ERROR])->val;
      }
      else if(auto intLit = (IntLit*) accept(INT_LITERAL))
      {
        base = new IntConstant(intLit);
      }
      else if(auto floatLit = (FloatLit*) accept(FLOAT_LITERAL))
      {
        base = new FloatConstant(floatLit);
      }
      else if(auto strLit = (StrLit*) accept(STRING_LITERAL))
      {
        base = new StringConstant(strLit);
      }
      else if(auto charLit = (CharLit*) accept(CHAR_LITERAL))
      {
        base = new CharConstant(charLit);
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
        base = new CompoundLiteral(exprs);
      }
      else
      {
        err("Expected expression");
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

  void Stream::accept()
  {
    pos++;
  }

  bool Stream::accept(Token& t)
  {
    bool res = lookAhead()->compareTo(&t);
    if(res)
      pos++;
    return res;
  }

  Token* Stream::accept(int tokType)
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

  bool Stream::acceptKeyword(int type)
  {
    Keyword kw(type);
    return accept(kw);
  }

  bool Stream::acceptOper(int type)
  {
    Oper op(type);
    return accept(op);
  }

  bool Stream::acceptPunct(int type)
  {
    Punct p(type);
    return accept(p);
  }

  void Stream::expect(Token& t)
  {
    auto next = lookAhead();
    if(t.compareTo(next))
    {
      pos++;
      return;
    }
    err(string("expected ") + t.getStr() + " but got " + next->getStr());
  }

  Token* Stream::expect(int tokType)
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

  void Stream::expectKeyword(int type)
  {
    Keyword kw(type);
    expect(kw);
  }

  void Stream::expectOper(int type)
  {
    Oper op(type);
    expect(op);
  }

  void Stream::expectPunct(int type)
  {
    Punct p(type);
    expect(p);
  }

  string Stream::expectIdent()
  {
    Ident* i = (Ident*) expect(IDENTIFIER);
    return i->name;
  }

  Token* Stream::lookAhead(int n)
  {
    int index = pos + n;
    if(index >= (int) tokens->size())
    {
      return &PastEOF::inst;
    }
    else
    {
      return (*tokens)[index];
    }
  }

  void Stream::err(string msg)
  {
    if(emitErrors)
    {
      string fullMsg = string("Syntax error at line ") + to_string(lookAhead()->line) + ", column " + to_string(lookAhead()->col);
      if(msg.length())
        fullMsg += string(": ") + msg;
      else
        fullMsg += '.';
      //display error and terminate
      errAndQuit(fullMsg);
    }
    else
    {
      //can't proceed with parsing, so just throw
      throw std::logic_error("");
    }
  }

  Stream::Stream(SourceFile* file)
  {
    pos = 0;
    emitErrors = true;
    tokens = &file->tokens;
  }
  Stream::Stream(const Stream& s)
  {
    pos = s.pos;
    emitErrors = s.emitErrors;
    tokens = s.tokens;
  }
  Stream& Stream::operator=(const Stream& s)
  {
    pos = s.pos;
    emitErrors = s.emitErrors;
    tokens = s.tokens;
    return *this;
  }
  bool Stream::operator==(const Stream& s)
  {
    return pos == s.pos;
  }
  bool Stream::operator!=(const Stream& s)
  {
    return pos != s.pos;
  }
  bool Stream::operator<(const Stream& s)
  {
    return pos < s.pos;
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

