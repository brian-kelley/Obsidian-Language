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
  path = path_;  FILE* f = fopen(path.c_str(), "rb");
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
#ifdef ONYX_TESTING
  //For testing purposes, just use filename (no directory).
  //This way, error/warning messages in gold output files
  //can be diffed exactly.
  size_t filenameStart = 0;
  filenameStart = std::max(path.rfind('\\') + 1, filenameStart);
  filenameStart = std::max(path.rfind('/') + 1, filenameStart);
  path = path.substr(filenameStart);
#endif
  fseek(f, 0, SEEK_END);
  size_t size = ftell(f);
  rewind(f);
  string source;
  source.resize(size);
  fread((void*) source.c_str(), 1, size, f);
  fclose(f);
  if(!includeLoc && source.length() > 2 && source.substr(0, 2) == "#!")
  {
    //skip shebang line in main file
    source = source.substr(source.find('\n'));
  }
  fileList.push_back(this);
  fileTable[path] = this;
  tokens = lex(source, id);
}

SourceFile* findSourceFile(string path)
{
  auto it = fileTable.find(path);
  if(it == fileTable.end())
    return nullptr;
  return it->second;
}

SourceFile* addSourceFile(Node* includeLoc, string path)
{
  SourceFile* sf = new SourceFile(includeLoc, path);
  fileTable[path] = sf;
  return sf;
}

SourceFile* sourceFileFromID(int id)
{
  INTERNAL_ASSERT(id < (int) fileList.size());
  return fileList[id];
}

