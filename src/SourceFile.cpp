#include "SourceFile.hpp"
#include "Scope.hpp"
#include "AST.hpp"
#include "Lexer.hpp"

static int fileCounter = 0;

//files, in order of ID
vector<SourceFile*> fileList;
//files, looked up by path
map<string, SourceFile*> fileTable;

SourceFile::SourceFile(Node* includeLoc, string path_)
{
  //TODO: convert to absolute, canonical path
  //Doesn't affect correctness but may avoid redundant loads
  id = fileCounter++;
  path = path_;
  FILE* f = fopen(path.c_str(), "rb");
  if(!f)
  {
    if(!includeLoc)
    {
      errMsg("Could not load main file \"" + path + "\"");
    }
    else
    {
      errMsgLoc(includeLoc,
          string("Could not load included file \"") + path + "\"");
    }
  }
  fseek(f, 0, SEEK_END);
  size_t size = ftell(f);
  rewind(f);
  string source;
  source.resize(size);
  fread((void*) source.c_str(), 1, size, f);
  fclose(f);
  tokens = lex(source, id);
}

SourceFile* getSourceFile(Node* includeLoc, string path)
{
  auto it = fileTable.find(path);
  if(it == fileTable.end())
  {
    SourceFile* newFile = new SourceFile(includeLoc, path);
    fileList.push_back(newFile);
    fileTable[path] = newFile;
    return newFile;
  }
  return it->second;
}

SourceFile* sourceFileFromID(int id)
{
  INTERNAL_ASSERT(id < fileList.size());
  return fileList[id];
}

