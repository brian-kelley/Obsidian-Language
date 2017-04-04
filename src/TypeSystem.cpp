#include "TypeSystem.hpp"

using namespace std;
using namespace Parser;

vector<Type*> Type::table;

void createTypeTable(Parser::ModuleDef& globalModule)
{
}

//Get the type table entry, given the local usage name and current scope
Type* getType(string localName, Scope* usedScope)
{
}

Type* getType(Parser::Member& localName, Scope* usedScope)
{
}

