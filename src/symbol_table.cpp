#include <assert.h>
#include <iostream>
#include <cstdlib>
#include "symbol_table.hpp"

symbol* symbol_table::insert(string s) {
  list<symbol*>::iterator tab_it;
  for (tab_it=tab.begin();tab_it!=tab.end();tab_it++)
    if ((*tab_it)->name==s) return *tab_it;
  symbol* sym=new symbol;
  sym->name=s;
  sym->val="0";
  sym->type=0;
  sym->shape=0;
  sym->intent=0;
  tab.push_back(sym);
  return sym;
}

void symbol_table::unparse(ostream& tgt_file, const string& active_type_name, int indent) {
  list<symbol*>::iterator tab_it;
  for (tab_it=tab.begin();tab_it!=tab.end();tab_it++) 
    (*tab_it)->unparse(tgt_file, active_type_name, indent);
}

void symbol::unparse(ostream& tgt_file, const string& float_type_name, int indent) {
    switch (type) {
      case FLOAT_ST : {
        for (int i=0;i<indent;i++) tgt_file << " ";
        // if (kind==GLOBAL_VAR_SK) tgt_file << "static ";
        if (shape==0)
          tgt_file << float_type_name << " " << name 
                   << "=" << val  << "; \n";
        else if (shape==1) 
          tgt_file << float_type_name << " " 
                   << name << "[" << val  << "]; \n";
        else {
          cerr << "ERROR: Unknown Symbol Shape" << endl;
          assert(false);
        }
        break;
      }
      case INT_ST : {
        for (int i=0;i<indent;i++) tgt_file << " ";
        // if (kind==GLOBAL_VAR_SK) tgt_file << "static ";
        if (shape==0)
          tgt_file << "int " << name 
                 << "=" << val << "; \n";
        else if (shape==1)
          tgt_file << "int " << name 
                     << "[" << val  << "]; \n";
        else {
          cerr << "ERROR: Unknown Symbol Shape" << endl;
          assert(false);
        }
        break;
      }
      default : {
        cerr << "ERROR: Unknown Symbol Type for " << name << endl;
        assert(false);
      }
    }
}

