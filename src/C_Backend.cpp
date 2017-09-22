#include "C_Backend.hpp"

using namespace TypeSystem;

map<Type*, string> types;
map<Type*, bool> typesImplemented;
map<Subroutine*, string> subrs;
map<Variable*, string> vars;
//Tables of utility functions
//(all types) get default-initialized value
map<Type*, string> initFuncs;
//(all types) deep copy
map<Type*, string> copyFuncs;
//(all types) print a value
map<Type*, string> printFuncs;
//(array types) take N dimensions and returns an allocated rectangular array
map<Type*, string> arrayAllocFuncs;
size_t identCount;
ofstream c;
//different stringstreams to build the C file (in this order)
Oss typeDecls;      //all typedefs (including forward-declarations as necessary)
Oss varDecls;       //all global/static variables
Oss utilFuncDecls;  //util functions are print, array creation, deep copy, etc
Oss utilFuncDefs;
Oss funcDecls;      //the actual subroutines in the onyx program
Oss funcDefs;

//The C type to use for array indices (could possibly be uint64_t for more portability)
const char* size_type = "uint32_t";

namespace C
{
  void generate(string outputStem, bool keep)
  {
    string cName = outputStem + ".c";
    string exeName = outputStem + ".exe";
    typeDecls = Oss();
    varDecls = Oss();
    utilFuncDecls = Oss();
    utilFuncDefs = Oss();
    funcDecls = Oss();
    funcDefs = Oss();
    genTypeDecls();
    genGlobals();
    genSubroutines();
    c = ofstream(cName);
    c << "//--- " << outputStem << ".c, generated by the Onyx Compiler ---//\n\n";
    //genCommon writes directly to the c stream
    genCommon();
    //write types, vars, func decls, func defs in the ostringstreams
    c.write(typeDecls.str().c_str(), typeDecls.tellp());
    c.write(varDecls.str().c_str(), varDecls.tellp());
    c.write(utilFuncDecls.str().c_str(), utilFuncDecls.tellp());
    c.write(utilFuncDefs.str().c_str(), utilFuncDefs.tellp());
    c.write(funcDecls.str().c_str(), funcDecls.tellp());
    c.write(funcDefs.str().c_str(), funcDefs.tellp());
    c << '\n';
    c.close();
    //wait for cc to terminate
    bool compileSuccess = runCommand(string("gcc") + " --std=c99 -Os -ffast-math -fassociative-math -o " + exeName + ' ' + cName + " &> /dev/null");
    if(!keep)
    {
      remove(cName.c_str());
    }
    if(!compileSuccess)
    {
      ERR_MSG("C compiler encountered error.");
    }
  }

  void genCommon()
  {
    c << "#include \"stdio.h\"\n";
    c << "#include \"stdlib.h\"\n";
    c << "#include \"math.h\"\n";
    c << "#include \"string.h\"\n";
    c << "#include \"stdint.h\"\n";
    c << "#include \"stdbool.h\"\n";
    c << '\n';
    c << "char* strdup_(const char* str) \n";
    c << "{\n";
    c << "char* temp = malloc(1 + strlen(str)); \n";
    c << "strcpy(temp, str);\n";
    c << "return temp;\n";
    c << "}\n\n";
  }

  void genTypeDecls()
  {
    //A list of all singular types (every type except arrays)
    vector<Type*> allTypes;
    for(auto prim : TypeSystem::primitives)
    {
      allTypes.push_back(prim);
    }
    walkScopeTree([&] (Scope* s) -> void
      {
        for(auto t : s->types)
        {
          if(!t->isAlias())
          {
            allTypes.push_back(t);
          }
        }
      });
    for(auto tt : TypeSystem::tuples)
    {
      allTypes.push_back(tt);
    }
    size_t nonArrayTypes = allTypes.size();
    for(size_t i = 0; i < nonArrayTypes; i++)
    {
      for(auto arrType : allTypes[i]->dimTypes)
      {
        allTypes.push_back(arrType);
      }
    }
    //primitives (string is a struct, all others are C primitives)
    types[TypeSystem::primNames["void"]] = "void";
    types[TypeSystem::primNames["bool"]] = "bool";
    types[TypeSystem::primNames["char"]] = "int8_t";
    types[TypeSystem::primNames["uchar"]] = "uint8_t";
    types[TypeSystem::primNames["short"]] = "int16_t";
    types[TypeSystem::primNames["ushort"]] = "uint16_t";
    types[TypeSystem::primNames["int"]] = "int32_t";
    types[TypeSystem::primNames["uint"]] = "uint32_t";
    types[TypeSystem::primNames["long"]] = "int64_t";
    types[TypeSystem::primNames["ulong"]] = "uint64_t";
    types[TypeSystem::primNames["float"]] = "float";
    types[TypeSystem::primNames["double"]] = "double";
    types[TypeSystem::primNames["string"]] = "ostring";
    typeDecls << "typedef struct\n";
    typeDecls << "{\n";
    typeDecls << "char* data;\n";
    typeDecls << "unsigned length;\n";
    typeDecls << "} ostring;\n\n";
    //forward-declare all compound types
    for(auto t : allTypes)
    {
      if(t->isPrimitive())
      {
        //primitives (including aliases of primitives) are already implemented (done above)
        typesImplemented[t] = true;
      }
      else
      {
        //get an identifier for type t
        string ident = getIdentifier();
        types[t] = ident;
        typesImplemented[t] = false;
        //forward-declare the type
        typeDecls << "typedef struct " << ident << ' ' << ident << "; //" << t->getName() << '\n';
      }
    }
    typeDecls << '\n';
    //implement all compound types
    for(auto t : allTypes)
    {
      if(!t->isPrimitive() && !typesImplemented[t])
      {
        generateCompoundType(typeDecls, types[t], t);
      }
    }
    typeDecls << '\n';
  }

  void genGlobals()
  {
    int numGlobals = 0;
    walkScopeTree([&] (Scope* s) -> void
      {
        //only care about vars in module scope
        if(dynamic_cast<ModuleScope*>(s))
        {
          for(auto v : s->vars)
          {
            string ident = getIdentifier();
            vars[v] = ident;
            varDecls << types[v->type] << " " << ident << "; //" << v->name << '\n';
            numGlobals = 0;
          }
        }
      });
    if(numGlobals)
    {
      varDecls << '\n';
    }
  }

  void genSubroutines()
  {
    //forward-declare all subroutines
    walkScopeTree([&] (Scope* s) -> void
      {
        for(auto sub : s->subr)
        {
          string ident;
          //main() is the only subroutine with a special name
          if(sub->name == "main")
            ident = "main";
          else
            ident = getIdentifier();
          subrs[sub] = ident;
          //all C functions except main are static
          if(ident != "main")
            funcDecls << "static ";
          funcDecls << types[sub->retType] << ' ' << ident << '(';
          for(auto arg : sub->argVars)
          {
            if(arg != sub->argVars.front())
            {
              funcDecls << ", ";
            }
            string argName = getIdentifier();
            vars[arg] = ident;
            funcDecls << types[arg->type] << ' ' << argName;
          }
          funcDecls << ");\n";
        }
      });
    //implement all subroutines
    walkScopeTree([&] (Scope* s) -> void
      {
        for(auto sub : s->subr)
        {
          funcDefs << types[sub->retType] << ' ' << subrs[sub] << '(';
          for(auto arg : sub->argVars)
          {
            if(arg != sub->argVars.front())
            {
              funcDefs << ", ";
            }
            string argName = getIdentifier();
            vars[arg] = argName;
            funcDefs << types[arg->type] << ' ' << argName;
          }
          funcDefs << ")\n";
          generateBlock(funcDefs, sub->body);
        }
      });
  }

  void generateExpression(ostream& c, Expression* expr)
  {
    //Expressions in C mostly depend on the subclass of expr
    if(UnaryArith* unary = dynamic_cast<UnaryArith*>(expr))
    {
      c << operatorTable[unary->op];
      c << '(';
      generateExpression(c, unary->expr);
      c << ')';
    }
    else if(BinaryArith* binary = dynamic_cast<BinaryArith*>(expr))
    {
      //emit this so that it works even if onyx uses different
      //operator precedence than C
      c << "((";
      generateExpression(c, binary->lhs);
      c << ')';
      c << operatorTable[binary->op];
      c << '(';
      generateExpression(c, binary->rhs);
      c << "))";
    }
    else if(IntLiteral* intLit = dynamic_cast<IntLiteral*>(expr))
    {
      //all int literals are unsigned
      c << intLit->value << "U";
      if(intLit->value >= 0xFFFFFFFF)
      {
        c << "ULL";
      }
    }
    else if(FloatLiteral* floatLit = dynamic_cast<FloatLiteral*>(expr))
    {
      //all float lits have type double, so never use "f" suffix
      c << floatLit->value;
    }
    else if(StringLiteral* stringLit = dynamic_cast<StringLiteral*>(expr))
    {
      //generate a char[] struct using C struct literal
      c << "((" << types[TypeSystem::primNames["char"]->getArrayType(1)] << ") {strdup_(\"";
      c << stringLit->value << "\"), " << stringLit->value.length() << "})";
    }
    else if(CharLiteral* charLit = dynamic_cast<CharLiteral*>(expr))
    {
      generateCharLiteral(c, charLit->value);
    }
    else if(BoolLiteral* boolLit = dynamic_cast<BoolLiteral*>(expr))
    {
      if(boolLit->value)
        c << "true";
      else
        c << "false";
    }
    else if(TupleLiteral* tupLit = dynamic_cast<TupleLiteral*>(expr))
    {
      //Tuple literal is just a C struct
      c << "((" << types[expr->type] << ") {";
      for(size_t i = 0; i < tupLit->members.size(); i++)
      {
        generateExpression(c, tupLit->members[i]);
        if(i != tupLit->members.size() - 1)
        {
          c << ", ";
        }
      }
    }
    else if(Indexed* indexed = dynamic_cast<Indexed*>(expr))
    {
      //Indexed expression must be either a tuple or array
      auto indexedType = indexed->group->type;
      if(ArrayType* at = dynamic_cast<ArrayType*>(indexedType))
      {
        auto elementType = at->elem;
        if(ArrayType* subArray = dynamic_cast<ArrayType*>(elementType))
        {
          c << "((" << types[indexedType] << ") {";
          //add all dimensions, except highest
          for(int dim = 1; dim < at->dims; dim++)
          {
            generateExpression(c, indexed->group);
            c << ".dim" << dim << ", ";
          }
          //now add the data pointer from expr, but with offset
          generateExpression(c, indexed->group);
          c << ".data + ";
          generateExpression(c, indexed->index);
          //offset is produce of index and all lesser dimensions
          for(int dim = 1; dim < at->dims; dim++)
          {
            c << " * (";
            generateExpression(c, indexed->group);
            c << ".dim" << dim << ")";
          }
          c << "})";
        }
        else
        {
          //just index into data
          c << '(';
          generateExpression(c, indexed->group);
          c << ".data[";
          generateExpression(c, indexed->index);
          c << "])";
        }
      }
      else if(TupleType* tt = dynamic_cast<TupleType*>(indexedType))
      {
        //tuple: simply reference the requested member
        c << '(';
        generateExpression(c, indexed->group);
        //index must be an IntLiteral (has already been checked)
        c << ".mem" << dynamic_cast<IntLiteral*>(indexed)->value << ')';
      }
    }
    else if(CallExpr* call = dynamic_cast<CallExpr*>(expr))
    {
      c << call->subr << '(';
      for(auto arg : call->args)
      {
        generateExpression(c, arg);
      }
      c << ')';
    }
    else if(VarExpr* var = dynamic_cast<VarExpr*>(expr))
    {
      c << vars[var->var];
    }
    else if(NewArray* na = dynamic_cast<NewArray*>(expr))
    {
      //need to call the correct new array function
      //create if it doesn't exist yet
      auto arrayType = (ArrayType*) na->type;
      auto it = arrayAllocFuncs.find(arrayType);
      string newArrayFunc;
      if(it == arrayAllocFuncs.end())
      {
        newArrayFunc = getIdentifier();
        generateNewArrayFunction(utilFuncDefs, newArrayFunc, arrayType);
      }
      else
      {
        newArrayFunc = it->second;
      }
      //now generate the call to newArrayFunc
      c << newArrayFunc << '(';
      for(size_t dim = 0; dim < na->dims.size(); dim++)
      {
        generateExpression(c, na->dims[dim]);
        if(dim != na->dims.size() - 1)
        {
          c << ", ";
        }
      }
      c << ')';
    }
  }

  void generateBlock(ostream& c, Block* b)
  {
    c << "{\n";
    //introduce local variables
    for(auto local : b->scope->vars)
    {
      string localIdent = getIdentifier();
      vars[local] = localIdent;
      c << types[local->type] << ' ' << localIdent << ";\n";
    }
    for(auto blockStmt : b->stmts)
    {
      generateStatement(c, b, blockStmt);
    }
    c << "}\n";
  }

  void generateStatement(ostream& c, Block* b, Statement* stmt)
  {
    //get the type of statement
    if(Block* blk = dynamic_cast<Block*>(stmt))
    {
      generateBlock(c, blk);
    }
    else if(Assign* a = dynamic_cast<Assign*>(stmt))
    {
      generateAssignment(c, b, a->lvalue, a->rvalue);
    }
    else if(CallStmt* cs = dynamic_cast<CallStmt*>(stmt))
    {
      c << subrs[cs->called] << '(';
      for(size_t i = 0; i < cs->args.size(); i++)
      {
        if(i > 0)
        {
          c << ", ";
        }
        generateExpression(c, cs->args[i]);
      }
      c << ");\n";
    }
    else if(For* f = dynamic_cast<For*>(stmt))
    {
      c << "for(";
      generateStatement(c, b, f->init);
      generateExpression(c, f->condition);
      generateStatement(c, b, f->increment);
      c << ")\n";
      generateBlock(c, f->loopBlock);
    }
    else if(While* w = dynamic_cast<While*>(stmt))
    {
      c << "while(";
      generateExpression(c, w->condition);
      c << ")\n";
      c << "{\n";
      generateBlock(c, w->loopBlock);
      c << "}\n";
    }
    else if(If* i = dynamic_cast<If*>(stmt))
    {
      c << "if(";
      generateExpression(c, i->condition);
      c << ")\n";
      generateStatement(c, b, i->body);
    }
    else if(IfElse* ie = dynamic_cast<IfElse*>(stmt))
    {
      c << "if(";
      generateExpression(c, ie->condition);
      c << ")\n";
      generateStatement(c, b, ie->trueBody);
      c << "else\n";
      generateStatement(c, b, ie->falseBody);
    }
    else if(Return* r = dynamic_cast<Return*>(stmt))
    {
      if(r->value)
      {
        c << "return ";
        generateExpression(c, r->value);
        c << ";\n";
      }
      else
      {
        c << "return;\n";
      }
    }
    else if(dynamic_cast<Break*>(stmt))
    {
      c << "break;\n";
    }
    else if(dynamic_cast<Continue*>(stmt))
    {
      c << "continue;\n";
    }
    else if(Print* p = dynamic_cast<Print*>(stmt))
    {
      //emit printf calls for each expression
      for(auto expr : p->exprs)
      {
        generatePrint(c, expr);
      }
    }
    else if(Assertion* assertion = dynamic_cast<Assertion*>(stmt))
    {
      c << "if(";
      generateExpression(c, assertion->asserted);
      c << ")\n";
      c << "{\n";
      c << "puts(\"Assertion failed.\");\n";
      c << "exit(1);\n";
      c << "}\n";
    }
  }

  void generateAssignment(ostream& c, Block* b, Expression* lhs, Expression* rhs)
  {
  /*
    //if rhs is a compound literal, assignment depends on lhs type
    if(auto compLit = dynamic_cast<CompoundLiteral*>(rhs))
    {
      if(auto at = dynamic_cast<ArrayType*>(lhs->type))
      {
      }
      else if(auto st = dynamic_cast<StructType*>(lhs->type))
      {
        //add assignment of each member individually

      }
      else if(auto tt = dynamic_cast<TupleType*>(lhs->type))
      {
      }
    }
    //if lhs is a tuple literal
    else if(auto tupLit = dynamic_cast<TupleLiteral*>(rhs))
    {
    }
    //other cases of assignment to variable or member can just use simple C assignment
    else if(auto ve = dynamic_cast<VarExpr*>(lhs))
    {
    }
    */
  }

  void generateAllInitFuncs()
  {
    for(auto type : types)
    {
      Type* t = type.first;
      string typeName = type.second;
      string func = "init_" + typeName;
      utilFuncDecls << typeName << ' ' << func << "();\n";
      utilFuncDefs << typeName << ' ' << func << "()\n{\n";
      if(t->isNumber() || t->isChar())
      {
        utilFuncDefs << "return 0;\n";
      }
      else if(t->isBool())
      {
        utilFuncDefs << "return false;\n";
      }
      else if(t->isStruct() || t->isTuple())
      {
        auto st = dynamic_cast<StructType*>(t);
        auto tt = dynamic_cast<TupleType*>(t);
        utilFuncDefs << typeName << " temp_;\n";
        if(st)
        {
          for(size_t i = 0; i < st->members.size(); i++)
          {
            utilFuncDefs << "temp_." << st->memberNames[i] << " = " << "init_" << types[st->members[i]] << "();\n";
          }
        }
        else
        {
          for(size_t i = 0; i < tt->members.size(); i++)
          {
            utilFuncDefs << "temp_.mem" << i << " = " << "init_" << types[tt->members[i]] << "();\n";
          }
        }
        utilFuncDefs << "return temp_;\n";
      }
      else if(t->isArray())
      {
        //empty array doesn't need any allocation (leave data null)
        utilFuncDefs << "return ((" << typeName << ") {NULL, 0});\n";
      }
      utilFuncDefs << "}\n";
    }
  }

  void generateAllPrintFuncs()
  {
    //declare print functions for every non-primitive
    for(auto type : types)
    {
      Type* t = type.first;
      if(!t->isPrimitive())
      {
        string func = "print_" + type.second;
        printFuncs[t] = func;
        utilFuncDecls << "void " << func << '(' << type.second << " data_);\n";
        utilFuncDecls << "//print function for " << t->getName() << '\n';
      }
    }
    for(auto type : types)
    {
      Type* t = type.first;
      if(!t->isPrimitive())
      {
        string func = printFuncs[t];
        string argname = getIdentifier();
        utilFuncDefs << "void " << func << '(' << type.second << ' ' << argname << ")\n";
        utilFuncDefs << "{\n";
        if(ArrayType* at = dynamic_cast<ArrayType*>(t))
        {
          string count = getIdentifier();
          utilFuncDefs << "for(uint64_t " << count << " = 0; ";
          utilFuncDefs << count << " < data_.dim; " << count << "++)\n";
        }
        else if(TupleType* tt = dynamic_cast<TupleType*>(t))
        {
        }
        else if(StructType* st = dynamic_cast<StructType*>(t))
        {
        }
        else if(UnionType* ut = dynamic_cast<UnionType*>(t))
        {
        }
        utilFuncDefs << "}\n";
      }
    }
  }

  void generatePrint(ostream& c, Expression* expr)
  {
    auto type = expr->type;
    if(IntegerType* intType = dynamic_cast<IntegerType*>(type))
    {
      //printf format code
      string fmt;
      switch(intType->size)
      {
        case 1:
          fmt = intType->isSigned ? "hhd" : "hhu";
          break;
        case 2:
          fmt = intType->isSigned ? "hd" : "hu";
          break;
        case 4:
          fmt = intType->isSigned ? "d" : "u";
          break;
        case 8:
          fmt = intType->isSigned ? "lld" : "llu";
          break;
        default:
          INTERNAL_ERROR;
      }
      c << "printf(\"%" << fmt << "\", ";
      generateExpression(c, expr);
      c << ");\n";
    }
    else if(dynamic_cast<CharType*>(type))
    {
      //note: same printf code %f used for both float and double
      c << "printf(\"%c\", ";
      generateExpression(c, expr);
      c << ");\n";
    }
    else if(dynamic_cast<FloatType*>(type))
    {
      //note: same printf code %f used for both float and double
      c << "printf(\"%f\", ";
      generateExpression(c, expr);
      c << ");\n";
    }
    else if(dynamic_cast<VoidType*>(type))
    {
      c << "printf(\"void\");\n";
    }
    else if(dynamic_cast<BoolType*>(type))
    {
      c << "if(";
      generateExpression(c, expr);
      c << ")\n";
      c << "printf(\"true\");\n";
      c << "else\n";
      c << "printf(\"false\");\n";
    }
    else
    {
      //compound type: use a pregenerated function to print
      c << printFuncs[type] << '(';
      generateExpression(c, expr);
      c << ");\n";
    }
  }

  void generateNewArrayFunction(ostream& c, string ident, ArrayType* at)
  {
    c << types[at] << ' ' << ident << '(';
    for(int i = 0; i < at->dims; i++)
    {
      c << size_type << " d" << i;
      if(i != at->dims - 1)
      {
        c << ", ";
      }
    }
    c << ") //allocation of  " << at->getName();
    c << "\n{\n";
    c << types[at] << " arr;\n";
    for(int i = 0; i < at->dims; i++)
    {
      c << "arr.dim" << i << " = dim" << i << ";\n";
    }
    c << "arr.data = malloc(";
    for(int i = 0; i < at->dims; i++)
    {
      c << "dim" << i;
      if(i != at->dims - 1)
      {
        c << " * ";
      }
    }
    c << ");\n";
    c << "return arr;\n}\n";
  }

  string getIdentifier()
  {
    //use a base-36 encoding of identCount using 0-9 A-Z
    char buf[32];
    buf[31] = 0;
    auto val = identCount;
    int iter = 30;
    for(;; iter--)
    {
      int digit = val % 36;
      if(digit < 10)
      {
        buf[iter] = '0' + digit;
      }
      else
      {
        buf[iter] = 'A' + (digit - 10);
      }
      val /= 36;
      if(val == 0)
        break;
    }
    //now buf + iter is the string
    identCount++;
    return string("o") + (buf + iter) + '_';
  }

  template<typename F>
  void walkScopeTree(F f)
  {
    vector<Scope*> visit;
    visit.push_back(global);
    while(visit.size())
    {
      Scope* s = visit.back();
      f(s);
      visit.pop_back();
      for(auto child : s->children)
      {
        visit.push_back(child);
      }
    }
  }

  void generateCompoundType(ostream& c, string cName, TypeSystem::Type* t)
  {
    auto at = dynamic_cast<ArrayType*>(t);
    auto st = dynamic_cast<StructType*>(t);
    auto ut = dynamic_cast<UnionType*>(t);
    auto tt = dynamic_cast<TupleType*>(t);
    auto et = dynamic_cast<EnumType*>(t);
    //first, make sure all necessary types have already been defined
    if(at)
    {
      if(!typesImplemented[at->elem])
      {
        generateCompoundType(c, types[at->elem], at->elem);
      }
    }
    else if(st)
    {
      for(auto mem : st->members)
      {
        if(!typesImplemented[mem])
        {
          generateCompoundType(c, types[mem], mem);
        }
      }
    }
    else if(tt)
    {
      for(auto mem : tt->members)
      {
        if(!typesImplemented[mem])
        {
          generateCompoundType(c, types[mem], mem);
        }
      }
    }
    if(et)
    {
      //enum type always represented as signed integer
      c << "typedef int" << 8 * et->bytes << "_t " << cName << ";\n";
    }
    else
    {
      //open a struct declaration
      c << "struct " << cName << "\n{\n";
      if(at)
      {
        //add dims
        c << size_type << " dim;\n";
        //add pointer to element type
        c << types[at->subtype] << "* data;\n";
      }
      else if(st)
      {
        //add all members (as pointer)
        //  since there is no possible name collision among the member names, don't
        //  need to replace them with mangled identifiers
        for(size_t i = 0; i < st->members.size(); i++)
        {
          c << types[st->members[i]] << ' ' << st->memberNames[i] << ";\n";
        }
      }
      else if(ut)
      {
        c << "void* data;\n";
        c << "int option;\n";
      }
      else if(tt)
      {
        for(size_t i = 0; i < tt->members.size(); i++)
        {
          //tuple members are anonymous so just use memN as the name
          c << types[tt->members[i]] << " mem" << i << ";\n";
        }
      }
      c << "};\n";
    }
    typesImplemented[t] = true;
  }

  void generateCharLiteral(ostream& c, char character)
  {
    c << '\'';
    switch(character)
    {
      case 0:
        c << "\\0";
        break;
      case '\n':
        c << "\\n";
        break;
      case '\t':
        c << "\\t";
        break;
      case '\r':
        c << "\\r";
        break;
      default:
        c << character;
    }
    c << '\'';
  }
}

