#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <assert.h>
#include "ast.hpp"
#include "symbol_table.hpp"

using namespace std;

void ast_vertex::unparse(ostream& tgt_file, int indent) {
  if (!this) return;
  list<ast_vertex*>::iterator it;
  switch (type) {
    case CODE_ASTV: {
      it=children.begin();
      (*it)->unparse(tgt_file,indent);
      (*(++it))->unparse(tgt_file,indent);
      break;
    }
    case SEQUENCE_OF_SUBROUTINES_ASTV: {
      for (it=children.begin();it!=children.end();it++) 
        (*it)->unparse(tgt_file,indent);
      break;
    }
    case SUBROUTINE_ASTV: {
      tgt_file << "void ";
      it=children.begin();
      (*it)->unparse(tgt_file,indent);
      tgt_file << "(";
      (*(++it))->unparse(tgt_file,indent);
      tgt_file << ")\n";
      tgt_file << "{\n";
      indent+=3;
      (*(++it))->unparse(tgt_file,indent);
      (*(++it))->unparse(tgt_file,indent);
      indent-=3;
      tgt_file << "}\n";
      break;
    }
    case SUBROUTINE_ARG_LIST_ASTV: {
      int s=1;
      for (it=children.begin();it!=children.end();it++,s++) {
        switch (static_cast<symbol_ast_vertex*>(*it)->sym->type) {
          case FLOAT_ST : {
            if (static_cast<symbol_ast_vertex*>(*it)->sym->shape==0)
              tgt_file << "double& ";
            else if (static_cast<symbol_ast_vertex*>(*it)->sym->shape==1)
              tgt_file << "double* ";
            else if (static_cast<symbol_ast_vertex*>(*it)->sym->shape==2)
              tgt_file << "double** ";
            else {
              cerr << "ERROR: Unknown Symbol Shape" << endl;
              assert(false);
            }
            break;
          }
          case INT_ST : {
            if (static_cast<symbol_ast_vertex*>(*it)->sym->shape==0) {
              if (static_cast<symbol_ast_vertex*>(*it)->sym->intent==0) 
                tgt_file << "int& ";
              else
                tgt_file << "int ";
            }
            else if (static_cast<symbol_ast_vertex*>(*it)->sym->shape==1)
              tgt_file << "int* ";
            else if (static_cast<symbol_ast_vertex*>(*it)->sym->shape==2)
              tgt_file << "int** ";
            else {
              cerr << "ERROR: Unknown Symbol Shape" << endl;
              assert(false);
            }
            break;
          }
          default : {
            cerr << "ERROR: Unknown Symbol Type" << endl;
            assert(false);
          }
        }
        static_cast<symbol_ast_vertex*>(*it)->unparse(tgt_file,indent);
        if (s<children.size()) tgt_file << ", ";
      }
      break;
    }
    case SEQUENCE_OF_DECLARATIONS_ASTV: {
      for (it=children.begin();it!=children.end();it++) 
        static_cast<symbol_ast_vertex*>(*it)->sym->unparse(tgt_file,"double",indent);
      break;
    }
    case SEQUENCE_OF_STATEMENTS_ASTV: {
      for (it=children.begin();it!=children.end();it++) 
        (*it)->unparse(tgt_file,indent);
      break;
    }
    case IF_STATEMENT_ASTV: {
      for (int i=0;i<indent;i++) tgt_file << " ";
      tgt_file << "if (";
      it=children.begin();
      (*it)->unparse(tgt_file,indent);
      tgt_file << ") {\n";
      indent+=3;
      (*(++it))->unparse(tgt_file,indent);
      indent-=3;
      for (int i=0;i<indent;i++) tgt_file << " ";
      tgt_file << "}\n";
      if (children.size()==3) {
        for (int i=0;i<indent;i++) tgt_file << " ";
        tgt_file << "else {\n";
        indent+=3;
        (*(++it))->unparse(tgt_file,indent);
        indent-=3;
        for (int i=0;i<indent;i++) tgt_file << " ";
        tgt_file << "}\n";
      }
      break;
    }
    case WHILE_STATEMENT_ASTV: {
      for (int i=0;i<indent;i++) tgt_file << " ";
      tgt_file << "while (";
      it=children.begin();
      (*it)->unparse(tgt_file,indent);
      tgt_file << ") {\n";
      indent+=3;
      (*(++it))->unparse(tgt_file,indent);
      indent-=3;
      for (int i=0;i<indent;i++) tgt_file << " ";
      tgt_file << "}\n";
      break;
    }
    case CONDITION_LT_ASTV: {     
      it=children.begin();
      (*it)->unparse(tgt_file,indent);
      tgt_file << "<";
      (*(++it))->unparse(tgt_file,indent);
      break;
    }
    case CONDITION_GT_ASTV: {     
      it=children.begin();
      (*it)->unparse(tgt_file,indent);
      tgt_file << ">";
      (*(++it))->unparse(tgt_file,indent);
      break;
    }
    case CONDITION_EQ_ASTV: {     
      it=children.begin();
      (*it)->unparse(tgt_file,indent);
      tgt_file << "==";
      (*(++it))->unparse(tgt_file,indent);
      break;
    }
    case CONDITION_NEQ_ASTV: {     
      it=children.begin();
      (*it)->unparse(tgt_file,indent);
      tgt_file << "!=";
      (*(++it))->unparse(tgt_file,indent);
      break;
    }
    case CONDITION_LE_ASTV: {     
      it=children.begin();
      (*it)->unparse(tgt_file,indent);
      tgt_file << "<=";
      (*(++it))->unparse(tgt_file,indent);
      break;
    }
    case CONDITION_GE_ASTV: {     
      it=children.begin();
      (*it)->unparse(tgt_file,indent);
      tgt_file << ">=";
      (*(++it))->unparse(tgt_file,indent);
      break;
    }
    case ASSIGNMENT_ASTV: {   
      for (int i=0;i<indent;i++) tgt_file << " ";
      it=children.begin();
      (*it)->unparse(tgt_file,indent);
      tgt_file << "=";
      (*(++it))->unparse(tgt_file,indent);
      tgt_file << ";\n";
      break;
    }
    case SUBROUTINE_CALL_ASTV: {
      for (int i=0;i<indent;i++) tgt_file << " ";
      (*(children.begin()))->unparse(tgt_file,indent);
      tgt_file << "(";
      for (it=(*(++(children.begin())))->children.begin();it!=(*(++(children.begin())))->children.end();it++) {
        if (it!=(*(++(children.begin())))->children.begin())
          tgt_file << ',';
        (*it)->unparse(tgt_file,indent);
      }
      tgt_file << ");\n";
      break;
    }
    case ADD_EXPRESSION_ASTV: {
      it=children.begin();
      (*it)->unparse(tgt_file,indent);
      tgt_file << "+";
      (*(++it))->unparse(tgt_file,indent);
      break;
    }
    case SUB_EXPRESSION_ASTV: {
      it=children.begin();
      (*it)->unparse(tgt_file,indent);
      tgt_file << "-";
      (*(++it))->unparse(tgt_file,indent);
      break;
    }
    case MUL_EXPRESSION_ASTV: {
      it=children.begin();
      (*it)->unparse(tgt_file,indent);
      tgt_file << "*";
      (*(++it))->unparse(tgt_file,indent);
      break;
    }
    case DIV_EXPRESSION_ASTV: {
      it=children.begin();
      (*it)->unparse(tgt_file,indent);
      tgt_file << "/";
      (*(++it))->unparse(tgt_file,indent);
      break;
    }
    case PAR_EXPRESSION_ASTV: {
      tgt_file << "(";
      (*(children.begin()))->unparse(tgt_file,indent);
      tgt_file << ")";
      break;
    }
    case SIN_EXPRESSION_ASTV: {
      tgt_file << "sin(";
      (*(children.begin()))->unparse(tgt_file,indent);
      tgt_file << ")";
      break;
    }
    case COS_EXPRESSION_ASTV: {
      tgt_file << "cos(";
      (*(children.begin()))->unparse(tgt_file,indent);
      tgt_file << ")";
      break;
    }
    case EXP_EXPRESSION_ASTV: {
      tgt_file << "exp(";
      (*(children.begin()))->unparse(tgt_file,indent);
      tgt_file << ")";
      break;
    }
    case SQRT_EXPRESSION_ASTV: {
      tgt_file << "sqrt(";
      (*(children.begin()))->unparse(tgt_file,indent);
      tgt_file << ")";
      break;
    }
    case TAN_EXPRESSION_ASTV: {
      tgt_file << "tan(";
      (*(children.begin()))->unparse(tgt_file,indent);
      tgt_file << ")";
      break;
    }
    case ATAN_EXPRESSION_ASTV: {
      tgt_file << "atan(";
      (*(children.begin()))->unparse(tgt_file,indent);
      tgt_file << ")";
      break;
    }
    case LOG_EXPRESSION_ASTV: {
      tgt_file << "log(";
      (*(children.begin()))->unparse(tgt_file,indent);
      tgt_file << ")";
      break;
    }
    case POW_I_EXPRESSION_ASTV: {
      it=children.begin();
      tgt_file << "pow(";
      (*it)->unparse(tgt_file,indent);
      tgt_file << ",";
      (*(++it))->unparse(tgt_file,indent);
      tgt_file << ")";
      break;
    }
    case POW_F_EXPRESSION_ASTV: {
      it=children.begin();
      tgt_file << "pow(";
      (*it)->unparse(tgt_file,indent);
      tgt_file << ",";
      (*(++it))->unparse(tgt_file,indent);
      tgt_file << ")";
      break;
    }
    case SYMBOL_ASTV: {
      tgt_file << static_cast<symbol_ast_vertex*>(this)->sym->name;
      break;
    }
    case CONSTANT_ASTV: {
      tgt_file << static_cast<const_ast_vertex*>(this)->value->name;
      break;
    }
    case SCALAR_MEMREF_ASTV: {
      tgt_file << static_cast<scalar_memref_ast_vertex*>(this)->base->name;
      break;
    }
    case ARRAY_MEMREF_ASTV: {
      tgt_file << static_cast<const array_memref_ast_vertex*>(this)->base->name;
      for (int i=0;i<static_cast<const array_memref_ast_vertex*>(this)->dim;i++)
        tgt_file << "["
                 << static_cast<const array_memref_ast_vertex*>(this)->offsets[i]->name
                 << "]";
      break;
    }
    default: {
      cerr << "Unknown ast vertex type " <<  type << endl;
      exit(-1);
      break;
    }
  }
}

const string
ast_vertex::lhs_string () const {
  assert(type==ASSIGNMENT_ASTV);
  scalar_memref_ast_vertex* lhs=static_cast<scalar_memref_ast_vertex*>(*(children.begin()));
  ostringstream oss;
  switch (lhs->type) {
    case SCALAR_MEMREF_ASTV : {
      oss << lhs->base->name;
      break;
    }
    case ARRAY_MEMREF_ASTV: {
      oss << static_cast<array_memref_ast_vertex*>(lhs)->base->name;
      for (int i=0;i<static_cast<array_memref_ast_vertex*>(lhs)->dim;i++)
        oss << "["
            << static_cast<array_memref_ast_vertex*>(lhs)->offsets[i]->name
            << "]";
      break;
    }
  }
  return oss.str();
}

void ast::unparse(const string& outfile_name) {
  ofstream* tgt_file= new ofstream(outfile_name.c_str());
  if (ast_root) ast_root->unparse(*tgt_file,0);
  delete tgt_file;
}
