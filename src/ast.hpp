#ifndef AST
#define AST

#include <fstream>
#include <vector>
#include <list>
#include "symbol_table.hpp"

#define UNDEFINED_ASTV                          1000
#define SUBROUTINE_ASTV                         1001
#define SUBROUTINE_ARG_LIST_ASTV                1002
#define SEQUENCE_OF_STATEMENTS_ASTV             1003
#define IF_STATEMENT_ASTV                       1004
#define WHILE_STATEMENT_ASTV                    1005
#define ASSIGNMENT_ASTV                         1006
#define ADD_EXPRESSION_ASTV                     1007
#define MUL_EXPRESSION_ASTV                     1008
#define SIN_EXPRESSION_ASTV                     1009
#define EXP_EXPRESSION_ASTV                     1010
#define SYMBOL_ASTV                             1011
#define CONSTANT_ASTV                           1012
#define COS_EXPRESSION_ASTV                     1013
#define CONDITION_LT_ASTV                       1014
#define CONDITION_EQ_ASTV                       1015
#define SUB_EXPRESSION_ASTV                     1016
#define SCALAR_MEMREF_ASTV                      1024
#define PAR_EXPRESSION_ASTV                     1026
#define DIV_EXPRESSION_ASTV                     1027
#define SQRT_EXPRESSION_ASTV                    1028
#define ATAN_EXPRESSION_ASTV                    1029
#define CONDITION_NEQ_ASTV                      1030
#define CONDITION_GT_ASTV                       1031
#define CONDITION_GE_ASTV                       1032
#define CONDITION_LE_ASTV                       1033
#define ARRAY_MEMREF_ASTV                       1034
#define SUBROUTINE_CALL_ASTV                    1035
#define SUBROUTINE_CALL_ARG_LIST_ASTV           1036
#define CODE_ASTV         	                1037
#define SEQUENCE_OF_DECLARATIONS_ASTV           1038
#define SEQUENCE_OF_SUBROUTINES_ASTV            1039
#define POW_I_EXPRESSION_ASTV                   1040
#define POW_F_EXPRESSION_ASTV                   1041
#define LOG_EXPRESSION_ASTV                     1042
#define TAN_EXPRESSION_ASTV                     1043

#define UNDEF_ARRAY_IDX 10
#define SYMBOL_ARRAY_IDX 11
#define CONSTANT_ARRAY_IDX 12


using namespace std;

/**   
 ast vertex
 */
class ast_vertex {
public:
/**   
 unique index
 */
  int idx;
/**   
 ast vertex type
 */
  int type;
/** 
 parent
 */
  ast_vertex* parent;
/** 
 children
 */
  list<ast_vertex*> children;

  ast_vertex() : type(UNDEFINED_ASTV), parent(0) {
    static int i=0; idx=i++;
    children.clear();
  }
  virtual ~ast_vertex() {
    list<ast_vertex*>::iterator i;
    for (i=children.begin();i!=children.end();i++) delete *i;
  }

public:

/** 
 unparser
 */
  void unparse(ostream&,int);

/**
 * return string representing variable (base + possible offset) on lhs
 * for current assignment in sac
 */
  const string
  lhs_string () const;

/**   
 return handle to SAC variable index in expr_ast_vertex
 */
  virtual int& sac_var_idx() { return idx; }

};

/**   
 ast vertex for expressions
 */
class expr_ast_vertex : public ast_vertex {
public:
/**   
 code list variable index
 */
  int clv_idx;

  expr_ast_vertex() : clv_idx(-1) { }
  ~expr_ast_vertex() {}
/**   
 return handle to SAC variable index
 */
  int& sac_var_idx() { return clv_idx; }
};

/**   
 ast vertex for symbol tokens
 */
class symbol_ast_vertex : public expr_ast_vertex {
public:
/** 
 pointer to token
 */
  symbol* sym;

  symbol_ast_vertex() : sym(0) { type=SYMBOL_ASTV; }
  ~symbol_ast_vertex() {}
};

/**   
 ast vertex for constants
 */
class const_ast_vertex : public expr_ast_vertex {
public:
/** 
 pointer to value
 */
  symbol* value;

  const_ast_vertex() : value(0) { type=CONSTANT_ASTV; }
  ~const_ast_vertex() {}
};

/**   
 ast vertex for scalar variables
 */
class scalar_memref_ast_vertex : public expr_ast_vertex {
public:
/** 
 pointer to base address 
 */
  symbol* base;

  scalar_memref_ast_vertex() : base(0) { type=SCALAR_MEMREF_ASTV; }
  ~scalar_memref_ast_vertex() {}
};

/**   
 ast vertex for array variables
 */
class array_memref_ast_vertex : public scalar_memref_ast_vertex {
public:
/** 
 dimension of array
 */
  int dim;
/** 
 pointer to offset 
 */
  symbol** offsets;

  array_memref_ast_vertex(int d) : dim(d) { 
    type=ARRAY_MEMREF_ASTV;
    offsets=new symbol*[d];
  }
  ~array_memref_ast_vertex() {
    delete [] offsets;
  }
};

/**
 ast vertex for scopes
 */
class scope_ast_vertex : public ast_vertex {
public:
/**
 symbol table
 */
  symbol_table* stab;
  scope_ast_vertex() { }
  ~scope_ast_vertex() {
    delete stab;
  }
};

#define YYSTYPE ast_vertex*

/**   
 ast 
 */
class ast {
public:
/**   
 unique root
 */
  ast_vertex* ast_root;
  ast() : ast_root(0) {};
  ~ast() { if (ast_root) delete ast_root; }
/** 
 unparser
 */
  void unparse(const string&);

};

#endif
