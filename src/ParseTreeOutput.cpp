#include "ParseTreeOutput.hpp"

using namespace Parser;

//The stream for writing dotfile (GraphViz) output
FILE* dot = NULL;

int nodeIndex = 0;
int nextNode()
{
  return nodeIndex++;
}

template<typename T>
int emit(T* n)
{
  cout << "ParseTreeOutput didn't implement emit<" << typeid(T).name() << ">\n";
  INTERNAL_ERROR;
  return 0;
}

template<> int emit<Module>(Module* n);

void outputParseTree(Parser::Module* tree, string filename)
{
#ifdef DEBUG
  dot = fopen(filename.c_str(), "w");
  fputs("digraph ParseTree {\n", dot);
  emit<Module>(tree);
  fputs("}\n", dot);
  fclose(dot);
#endif
}

#ifdef DEBUG
//Create a new node with given label
//label can be anything, as long as special chars are escaped
static int node(string label)
{
  int n = nextNode();
  fprintf(dot, "N%i [label=\"%s\"];\n", n, label.c_str());
  return n;
}

//Create an edge from n1 to n2
static void link(int n1, int n2)
{
  fprintf(dot, "N%i -> N%i;\n", n1, n2);
}

template<> int emit<ScopedDecl>(ScopedDecl* n);
template<> int emit<TypeNT>(TypeNT* n);
template<> int emit<StatementNT>(StatementNT* n);
template<> int emit<Typedef>(Typedef* n);
template<> int emit<Return>(Return* n);
template<> int emit<SwitchCase>(SwitchCase* n);
template<> int emit<Switch>(Switch* n);
template<> int emit<ForC>(ForC* n);
template<> int emit<ForOverArray>(ForOverArray* n);
template<> int emit<ForRange>(ForRange* n);
template<> int emit<For>(For* n);
template<> int emit<While>(While* n);
template<> int emit<If>(If* n);
template<> int emit<Assertion>(Assertion* n);
template<> int emit<TestDecl>(TestDecl* n);
template<> int emit<EnumItem>(EnumItem* n);
template<> int emit<Enum>(Enum* n);
template<> int emit<Block>(Block* n);
template<> int emit<VarDecl>(VarDecl* n);
template<> int emit<VarAssign>(VarAssign* n);
template<> int emit<PrintNT>(PrintNT* n);
template<> int emit<CallOp>(CallOp* n);
template<> int emit<Parameter>(Parameter* n);
template<> int emit<SubroutineTypeNT>(SubroutineTypeNT* n);
template<> int emit<FuncDecl>(FuncDecl* n);
template<> int emit<FuncDef>(FuncDef* n);
template<> int emit<FuncTypeNT>(FuncTypeNT* n);
template<> int emit<ProcDecl>(ProcDecl* n);
template<> int emit<ProcDef>(ProcDef* n);
template<> int emit<ProcTypeNT>(ProcTypeNT* n);
template<> int emit<StructMem>(StructMem* n);
template<> int emit<StructDecl>(StructDecl* n);
template<> int emit<UnionDecl>(UnionDecl* n);
template<> int emit<TraitDecl>(TraitDecl* n);
template<> int emit<StructLit>(StructLit* n);
template<> int emit<BoolLit>(BoolLit* n);
template<> int emit<Member>(Member* n);
template<> int emit<BoundedTypeNT>(BoundedTypeNT* n);
template<> int emit<TupleTypeNT>(TupleTypeNT* n);
template<> int emit<Expr1>(Expr1* n);
template<> int emit<Expr2>(Expr2* n);
template<> int emit<Expr3>(Expr3* n);
template<> int emit<Expr4>(Expr4* n);
template<> int emit<Expr5>(Expr5* n);
template<> int emit<Expr6>(Expr6* n);
template<> int emit<Expr7>(Expr7* n);
template<> int emit<Expr8>(Expr8* n);
template<> int emit<Expr9>(Expr9* n);
template<> int emit<Expr10>(Expr10* n);
template<> int emit<Expr11>(Expr11* n);
template<> int emit<Expr12>(Expr12* n);
template<> int emit<NewArrayNT>(NewArrayNT* n);

template<> int emit<Module>(Module* n)
{
  int id = 0;
  if(n->name == "")
  {
    id = node("Program");
  }
  else
  {
    id = node("Module: " + n->name);
  }
  for(auto decl : n->decls)
  {
    link(id, emit(decl));
  }
  return id;
}

template<> int emit<ScopedDecl>(ScopedDecl* n)
{
  if(n->decl.is<Module*>())
    return emit(n->decl.get<Module*>());
  else if(n->decl.is<VarDecl*>())
    return emit(n->decl.get<VarDecl*>());
  else if(n->decl.is<StructDecl*>())
    return emit(n->decl.get<StructDecl*>());
  else if(n->decl.is<UnionDecl*>())
    return emit(n->decl.get<UnionDecl*>());
  else if(n->decl.is<TraitDecl*>())
    return emit(n->decl.get<TraitDecl*>());
  else if(n->decl.is<Enum*>())
    return emit(n->decl.get<Enum*>());
  else if(n->decl.is<Typedef*>())
    return emit(n->decl.get<Typedef*>());
  else if(n->decl.is<FuncDecl*>())
    return emit(n->decl.get<FuncDecl*>());
  else if(n->decl.is<FuncDef*>())
    return emit(n->decl.get<FuncDef*>());
  else if(n->decl.is<ProcDecl*>())
    return emit(n->decl.get<ProcDecl*>());
  else if(n->decl.is<ProcDef*>())
    return emit(n->decl.get<ProcDef*>());
  else if(n->decl.is<TestDecl*>())
    return emit(n->decl.get<TestDecl*>());
  return 0;
}

template<> int emit<TypeNT>(TypeNT* n)
{
  int base;
  if(n->t.is<TypeNT::Prim>())
  {
    switch(n->t.get<TypeNT::Prim>())
    {
      case TypeNT::BOOL:
        base = node("primitive type: bool"); break;
      case TypeNT::CHAR:
        base = node("primitive type: char"); break;
      case TypeNT::BYTE:
        base = node("primitive type: byte"); break;
      case TypeNT::UBYTE:
        base = node("primitive type: ubyte"); break;
      case TypeNT::SHORT:
        base = node("primitive type: short"); break;
      case TypeNT::USHORT:
        base = node("primitive type: ushort"); break;
      case TypeNT::INT:
        base = node("primitive type: int"); break;
      case TypeNT::UINT:
        base = node("primitive type: uint"); break;
      case TypeNT::LONG:
        base = node("primitive type: long"); break;
      case TypeNT::ULONG:
        base = node("primitive type: ulong"); break;
      case TypeNT::FLOAT:
        base = node("primitive type: float"); break;
      case TypeNT::DOUBLE:
        base = node("primitive type: double"); break;
      case TypeNT::VOID:
        base = node("primitive type: void"); break;
      default:;
    }
  }
  else if(n->t.is<Member*>())
  {
    base = emit(n->t.get<Member*>());
  }
  else if(n->t.is<TupleTypeNT*>())
  {
    base = emit(n->t.get<TupleTypeNT*>());
  }
  else if(n->t.is<SubroutineTypeNT*>())
  {
    base = emit(n->t.get<SubroutineTypeNT*>());
  }
  else if(n->t.is<TypeNT::TTypeNT>())
  {
    base = node("T type");
  }
  if(n->arrayDims)
  {
    int arrayNode = node("Array type, " + to_string(n->arrayDims) + " dims");
    link(arrayNode, base);
    return arrayNode;
  }
  else
  {
    return base;
  }
}

template<> int emit<StatementNT>(StatementNT* n)
{
  if(n->s.is<ScopedDecl*>())
  {
    return emit(n->s.get<ScopedDecl*>());
  }
  else if(n->s.is<VarAssign*>())
  {
    return emit(n->s.get<VarAssign*>());
  }
  else if(n->s.is<PrintNT*>())
  {
    return emit(n->s.get<PrintNT*>());
  }
  else if(n->s.is<Expr12*>())
  {
    return emit(n->s.get<Expr12*>());
  }
  else if(n->s.is<Block*>())
  {
    return emit(n->s.get<Block*>());
  }
  else if(n->s.is<ScopedDecl*>())
  {
    return emit(n->s.get<ScopedDecl*>());
  }
  else if(n->s.is<Return*>())
  {
    return emit(n->s.get<Return*>());
  }
  else if(n->s.is<Continue*>())
  {
    return node("Continue");
  }
  else if(n->s.is<Break*>())
  {
    return node("Break");
  }
  else if(n->s.is<Switch*>())
  {
    return emit(n->s.get<Switch*>());
  }
  else if(n->s.is<For*>())
  {
    return emit(n->s.get<For*>());
  }
  else if(n->s.is<While*>())
  {
    return emit(n->s.get<While*>());
  }
  else if(n->s.is<If*>())
  {
    return emit(n->s.get<If*>());
  }
  else if(n->s.is<Assertion*>())
  {
    return emit(n->s.get<Assertion*>());
  }
  else if(n->s.is<EmptyStatement*>())
  {
    return node("empty statement");
  }
  else
  {
    INTERNAL_ERROR;
  }
  return 0;
}

template<> int emit<Typedef>(Typedef* n)
{
  int root = node("Typedef");
  link(root, emit(n->type));
  link(root, node(n->ident));
  return root;
}

template<> int emit<Return>(Return* n)
{
  int root = node("Return");
  if(n->ex)
  {
    int ex = emit(n->ex);
    link(root, ex);
  }
  return root;
}

template<> int emit<SwitchCase>(SwitchCase* n)
{
  int root = node("Case");
  link(root, emit(n->matchVal));
  link(root, emit(n->s));
  return root;
}

template<> int emit<Switch>(Switch* sw)
{
  int root = node("Switch");
  //first, add the switched expression
  link(root, emit(sw->sw));
  //then emit cases
  for(auto cas : sw->cases)
    link(root, emit(cas));
  if(sw->defaultStatement)
  {
    int def = node("Default");
    link(root, def);
    link(def, emit(sw->defaultStatement));
  }
  return root;
}

template<> int emit<ForC>(ForC* fc)
{
  int root = node("For loop (C style)");
  link(root, emit(fc->decl));
  link(root, emit(fc->condition));
  link(root, emit(fc->incr));
  return root;
}

template<> int emit<ForOverArray>(ForOverArray* foa)
{
  int root = node("For loop over array");
  link(root, emit(foa->tup));
  link(root, emit(foa->expr));
  return root;
}

template<> int emit<ForRange>(ForRange* fr)
{
  int root = node("For loop over range");
  link(root, node(fr->name->name));
  link(root, emit(fr->start));
  link(root, emit(fr->end));
  return root;
}

template<> int emit<For>(For* f)
{
  int root = 0;
  if(f->f.is<ForC*>())
  {
    root = emit(f->f.get<ForC*>());
  }
  else if(f->f.is<ForOverArray*>())
  {
    root = emit(f->f.get<ForOverArray*>());
  }
  else if(f->f.is<ForRange*>())
  {
    root = emit(f->f.get<ForRange*>());
  }
  link(root, emit(f->body));
  return root;
}

template<> int emit<While>(While* w)
{
  int root = node("While loop");
  link(root, emit(w->cond));
  link(root, emit(w->body));
  return root;
}

template<> int emit<If>(If* i)
{
  int root = node("If");
  int cond = emit(i->cond);
  link(root, cond);
  link(cond, emit(i->ifBody));
  if(i->elseBody)
  {
    int els = node("Else");
    link(root, els);
    link(els, emit(i->elseBody));
  }
  return root;
}

template<> int emit<Assertion>(Assertion* n)
{
  int root = node("Assertion");
  link(root, emit(n->expr));
  return root;
}

template<> int emit<TestDecl>(TestDecl* n)
{
  int root = node("Test Statement");
  link(root, emit(n->stmt));
  return root;
}

template<> int emit<EnumItem>(EnumItem* n)
{
  if(n->value)
    return node(n->name + ": " + to_string(n->value->val));
  else
    return node(n->name + ": auto");
}

template<> int emit<Enum>(Enum* n)
{
  int root = node("Enum " + n->name);
  for(auto item : n->items)
    link(root, emit(item));
  return root;
}

template<> int emit<Block>(Block* n)
{
  int root = node("Block");
  for(auto stmt : n->statements)
  {
    int s = emit(stmt);
    link(root, s);
  }
  return root;
}

template<> int emit<VarDecl>(VarDecl* n)
{
  int root = 0;
  if(n->isStatic)
    root = node("Variable " + n->name + " (static)");
  else
    root = node("Variable " + n->name);
  if(n->type)
    link(root, emit(n->type));
  else
    link(root, node("auto type"));
  if(n->val)
    link(root, emit(n->val));
  return root;
}

template<> int emit<VarAssign>(VarAssign* n)
{
  int root = node("Assignment");
  int lhs = emit(n->target);
  int rhs = emit(n->rhs);
  link(root, lhs);
  link(root, rhs);
  return root;
}

template<> int emit<PrintNT>(PrintNT* n)
{
  int root = node("Print");
  for(auto expr : n->exprs)
  {
    int e = emit(expr);
    link(root, e);
  }
  return root;
}

template<> int emit<CallOp>(CallOp* n)
{
  int root = node("Call operation");
  for(auto arg : n->args)
  {
    link(root, emit(arg));
  }
  return root;
}

template<> int emit<Parameter>(Parameter* n)
{
  int root = node("Parameter");
  if(n->type.is<TypeNT*>())
  {
    link(root, emit(n->type.get<TypeNT*>()));
  }
  else if(n->type.is<BoundedTypeNT*>())
  {
    link(root, emit(n->type.get<BoundedTypeNT*>()));
  }
  if(n->name)
    link(root, node(n->name->name));
  return root;
}

template<> int emit<SubroutineTypeNT>(SubroutineTypeNT* n)
{
  auto ft = dynamic_cast<FuncTypeNT*>(n);
  auto pt = dynamic_cast<ProcTypeNT*>(n);
  if(ft)
    return emit(ft);
  else if(pt)
    return emit(pt);
  INTERNAL_ERROR;
  return 0;
}

template<> int emit<FuncDecl>(FuncDecl* n)
{
  int root = node("Func decl");
  link(root, node(n->name));
  link(root, emit(&n->type));
  return root;
}

template<> int emit<FuncDef>(FuncDef* n)
{
  int root = node("Func def");
  link(root, node(n->name));
  link(root, emit(&n->type));
  link(root, emit(n->body));
  return root;
}

template<> int emit<FuncTypeNT>(FuncTypeNT* n)
{
  int root = 0;
  if(n->isStatic)
    root = node("Function type");
  else
    root = node("Function type (static)");
  int retType = node("Return type");
  link(root, retType);
  link(retType, emit(n->retType));
  if(n->params.size() == 0)
  {
    link(root, node("No parameters"));
  }
  for(auto param : n->params)
  {
    link(root, emit(param));
  }
  return root;
}

template<> int emit<ProcDecl>(ProcDecl* n)
{
  int root = node("Proc decl");
  link(root, node(n->name));
  link(root, emit(&n->type));
  return root;
}

template<> int emit<ProcDef>(ProcDef* n)
{
  int root = node("Proc def");
  link(root, node(n->name));
  link(root, emit(&n->type));
  link(root, emit(n->body));
  return root;
}

template<> int emit<ProcTypeNT>(ProcTypeNT* n)
{
  string qualifiers = "(";
  if(n->isStatic)
    qualifiers = "static";
  if(n->nonterm)
  {
    if(n->isStatic)
      qualifiers += ", ";
    qualifiers += "non-terminating ";
  }
  qualifiers += ")";
  int root = 0;
  if(qualifiers.length() == 2)
    root = node("Procedure type");
  else
    root = node("Procedure type " + qualifiers);
  int retType = node("Return type");
  link(root, retType);
  link(retType, emit(n->retType));
  if(n->params.size() == 0)
  {
    link(root, node("No parameters"));
  }
  for(auto param : n->params)
  {
    link(root, emit(param));
  }
  return root;
}

template<> int emit<StructMem>(StructMem* n)
{
  int root = 0;
  if(n->compose)
  {
    root = node("Composed member");
    link(root, emit(n->sd));
  }
  else
    root = emit(n->sd);
  return root;
}

template<> int emit<StructDecl>(StructDecl* n)
{
  int root = node("Struct " + n->name);
  if(n->traits.size())
  {
    int traitList = node("Traits");
    link(root, traitList);
    for(auto trait : n->traits)
    {
      link(traitList, emit(trait));
    }
  }
  for(auto mem : n->members)
  {
    link(root, emit(mem));
  }
  return root;
}

template<> int emit<UnionDecl>(UnionDecl* n)
{
  int root = node("Union " + n->name);
  for(auto t : n->types)
    link(root, emit(t));
  return root;
}

template<> int emit<TraitDecl>(TraitDecl* n)
{
  int root = node("Trait " + n->name);
  for(auto& mem : n->members)
  {
    if(mem.is<FuncDecl*>())
      link(root, emit(mem.get<FuncDecl*>()));
    else if(mem.is<ProcDecl*>())
      link(root, emit(mem.get<ProcDecl*>()));
  }
  return root;
}

template<> int emit<StructLit>(StructLit* n)
{
  int root = node("Compound literal");
  for(auto expr : n->vals)
    link(root, emit(expr));
  return root;
}

template<> int emit<BoolLit>(BoolLit* n)
{
  if(n->val)
    return node("Bool literal: true");
  else
    return node("Bool literal: false");
}

template<> int emit<Member>(Member* mem)
{
  string label;
  for(auto t : mem->head)
  {
    label += t->name;
    label += ".";
  }
  label += mem->tail->name;
  return node(label);
}

template<> int emit<BoundedTypeNT>(BoundedTypeNT* n)
{
  int root = node("Bounded type " + n->localName);
  for(auto t : n->traits)
    link(root, emit(t));
  return root;
}

template<> int emit<TupleTypeNT>(TupleTypeNT* n)
{
  int root = node("Tuple type");
  for(auto mem : n->members)
    link(root, emit(mem));
  return root;
}

template<> int emit<Expr1>(Expr1* n)
{
  if(n->e.is<NewArrayNT*>())
    return emit(n->e.get<NewArrayNT*>());
  int base = emit(n->e.get<Expr2*>());
  for(auto rhs : n->tail)
  {
    int newBase = node("||");
    link(newBase, base);
    link(newBase, emit(rhs->rhs));
    base = newBase;
  }
  return base;
}

template<> int emit<Expr2>(Expr2* n)
{
  int base = emit(n->head);
  for(auto rhs : n->tail)
  {
    int newBase = node("&&");
    link(newBase, base);
    link(newBase, emit(rhs->rhs));
    base = newBase;
  }
  return base;
}

template<> int emit<Expr3>(Expr3* n)
{
  int base = emit(n->head);
  for(auto rhs : n->tail)
  {
    int newBase = node("|");
    link(newBase, base);
    link(newBase, emit(rhs->rhs));
    base = newBase;
  }
  return base;
}

template<> int emit<Expr4>(Expr4* n)
{
  int base = emit(n->head);
  for(auto rhs : n->tail)
  {
    int newBase = node("^");
    link(newBase, base);
    link(newBase, emit(rhs->rhs));
    base = newBase;
  }
  return base;
}

template<> int emit<Expr5>(Expr5* n)
{
  int base = emit(n->head);
  for(auto rhs : n->tail)
  {
    int newBase = node("&");
    link(newBase, base);
    link(newBase, emit(rhs->rhs));
    base = newBase;
  }
  return base;
}

template<> int emit<Expr6>(Expr6* n)
{
  int base = emit(n->head);
  for(auto rhs : n->tail)
  {
    int newBase = node(operatorTable[rhs->op]);
    link(newBase, base);
    link(newBase, emit(rhs->rhs));
    base = newBase;
  }
  return base;
}

template<> int emit<Expr7>(Expr7* n)
{
  int base = emit(n->head);
  for(auto rhs : n->tail)
  {
    int newBase = node(operatorTable[rhs->op]);
    link(newBase, base);
    link(newBase, emit(rhs->rhs));
    base = newBase;
  }
  return base;
}

template<> int emit<Expr8>(Expr8* n)
{
  int base = emit(n->head);
  for(auto rhs : n->tail)
  {
    int newBase = node(operatorTable[rhs->op]);
    link(newBase, base);
    link(newBase, emit(rhs->rhs));
    base = newBase;
  }
  return base;
}

template<> int emit<Expr9>(Expr9* n)
{
  int base = emit(n->head);
  for(auto rhs : n->tail)
  {
    int newBase = node(operatorTable[rhs->op]);
    link(newBase, base);
    link(newBase, emit(rhs->rhs));
    base = newBase;
  }
  return base;
}

template<> int emit<Expr10>(Expr10* n)
{
  int base = emit(n->head);
  for(auto rhs : n->tail)
  {
    int newBase = node(operatorTable[rhs->op]);
    link(newBase, base);
    link(newBase, emit(rhs->rhs));
    base = newBase;
  }
  return base;
}

template<> int emit<Expr11>(Expr11* n)
{
  if(n->e.is<Expr11::UnaryExpr>())
  {
    auto ue = n->e.get<Expr11::UnaryExpr>();
    int root = node(operatorTable[ue.op]);
    link(root, emit(ue.rhs));
    return root;
  }
  return emit(n->e.get<Expr12*>());
}

template<> int emit<Expr12>(Expr12* n)
{
  int root = 0;
  if(n->e.is<IntLit*>())
  {
    root = node("Integer literal " + to_string(n->e.get<IntLit*>()->val));
  }
  else if(n->e.is<CharLit*>())
  {
    char c = n->e.get<CharLit*>()->val;
    if(isgraph(c))
      root = node(string("Char literal '") + c + "'");
    else
    {
      char buf[16];
      sprintf(buf, "%#02hhx", c);
      root = node(string("Char literal ") + buf);
    }
  }
  else if(n->e.is<StrLit*>())
  {
    root = node(string("String literal \"") + n->e.get<StrLit*>()->val + "\"");
  }
  else if(n->e.is<FloatLit*>())
  {
    root = node(string("Float literal \"") + to_string(n->e.get<FloatLit*>()->val));
  }
  else if(n->e.is<BoolLit*>())
  {
    root = emit(n->e.get<BoolLit*>());
  }
  else if(n->e.is<ExpressionNT*>())
  {
    root = emit(n->e.get<ExpressionNT*>());
  }
  else if(n->e.is<StructLit*>())
  {
    root = emit(n->e.get<StructLit*>());
  }
  else if(n->e.is<Member*>())
  {
    root = emit(n->e.get<Member*>());
  }
  //apply all operands, left to right
  for(auto rhs : n->tail)
  {
    if(rhs->e.is<Ident*>())
    {
      int newRoot = node("Member");
      link(newRoot, root);
      link(newRoot, node(rhs->e.get<Ident*>()->name));
      root = newRoot;
    }
    else if(rhs->e.is<CallOp*>())
    {
      int newRoot = node("Call");
      link(newRoot, root);
      link(newRoot, emit(rhs->e.get<CallOp*>()));
      root = newRoot;
    }
    else if(rhs->e.is<ExpressionNT*>())
    {
      int newRoot = node("Array index");
      link(newRoot, root);
      link(newRoot, emit(rhs->e.get<ExpressionNT*>()));
      root = newRoot;
    }
  }
  return root;
}

template<> int emit<NewArrayNT>(NewArrayNT* n)
{
  int root = node("New array");
  link(root, emit(n->elemType));
  for(auto dim : n->dimensions)
    link(root, emit(dim));
  return root;
}

#endif //DEBUG

