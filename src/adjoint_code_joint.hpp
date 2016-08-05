#include <fstream>
#include <vector>
#include <list>
#include "ast.hpp"

using namespace std;

/**
 auxilliary algorithms for construction of adjoint code
 */
class adjoint_code_joint {

  /**
   adjoint code unparser to be called recursively on ast vertices
   */
  void unparse(ast_vertex*, ofstream&, int, 
               vector<list<string> >&, const string&);


public:
  adjoint_code_joint() {};
  ~adjoint_code_joint() {};
  /**
   build adjoint code 
   */
  void build(const string&, const string&);

};
