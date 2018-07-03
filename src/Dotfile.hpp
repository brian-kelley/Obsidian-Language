#ifndef DOTFILE_H
#define DOTFILE_H

#include "Common.hpp"

//GraphViz dotfile interface

//create nodes, create edges and
//output to file or stream on demand

struct Dotfile
{
  static int autoNumbering;

  Dotfile()
  {
    name = "graph" + to_string(autoNumbering++);
  }

  Dotfile(string n) : name(n) {}

  int createNode(string s)
  {
    nodes.push_back(s);
    return nodes.size() - 1;
  }

  void createEdge(int n1, int n2)
  {
    edges.emplace_back(n1, n2);
  }

  void write(string filename)
  {
    ofstream os(filename);
    write(os);
    os.close();
  }

  void write(ostream& os)
  {
    os << "digraph " << name << " {\n";
    for(size_t i = 0; i < nodes.size(); i++)
    {
      os << "N" << i << " [label=\"" << nodes[i] << "\"];\n";
    }
    os << '\n';
    for(auto& e : edges)
    {
      os << "N" << e.first << " -> N" << e.second << ";\n";
    }
    fputs("}\n", dot);
  }

  void clear()
  {
    nodes.clear();
    edges.clear();
  }

  string name;
  vector<string> nodes;
  vector<pair<int, int>> edges;
};

#endif

