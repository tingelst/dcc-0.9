#ifndef SYMBOL_TABLE
#define SYMBOL_TABLE

#include <fstream>
#include <list>

using namespace std;

#define SUBROUTINE_NAME_SK 101
#define SUBROUTINE_ARG_SK 102
#define LOCAL_VAR_SK 103
#define AUX_VAR_SK 104
#define CONST_SK 105
#define GLOBAL_VAR_SK 106

#define VOID_ST 201
#define FLOAT_ST 202
#define INT_ST 203
#define AFLOAT_ST 204

/**
 symbol 
 */
class symbol {
  public:
  string name; int kind; int type; int shape; string val; int intent;
/**
 unparse the symbol 
 */
  void unparse(ostream&, const string&, int);
  
};

/**
 symbol table
 */
class symbol_table {
public:
/**
 symbol table is stored as simple list of strings;
 */
  list<symbol*> tab;
  symbol_table() { }
  ~symbol_table() {
     std::list<symbol*>::iterator it;
     if(!tab.empty()) 
       for (it=tab.begin();it!=tab.end();it++) delete *it; 
  }
/**
 insert a string into the symbol table; checks for duplication.
 */
  symbol* insert(string);
/**
 unparse the symbol table
 */
  void unparse(ostream&, const string&, int);

};

#endif
