#include <fstream>
#include "ast.hpp"

/**
 auxilliary algorithms for construction of tangent-linear code
 */
class tangent_linear_code{

  /**
   tangent-linear code unparser to be called recursively on ast vertices
   */
  void 
  unparse(ast_vertex*, ofstream&, int);

public:
  tangent_linear_code() {};
  ~tangent_linear_code() {};
  /**
   build tangent-linear code
   */
  void build(const string&);

};
