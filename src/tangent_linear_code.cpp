#include <iostream>
#include <cstdlib>
#include <sstream>
#include <assert.h>
#include "tangent_linear_code.hpp"
#include "ast.hpp"

extern ast the_ast;

extern string tl_var_prefix;
extern string sac_var_prefix;
extern string tl_file_prefix;

void tangent_linear_code::build(const string& outfile_name) {
  ofstream tgtfile(outfile_name.c_str());
  // unparse tangent-linear code
  if (the_ast.ast_root) unparse(the_ast.ast_root,tgtfile,0);
  tgtfile.close();
}

void tangent_linear_code::unparse(ast_vertex* v, ofstream& tgtfile, int indent) {
  if (!v) return;
  list<ast_vertex*>::const_iterator it;
  switch (v->type) {
    case CODE_ASTV: {
      it=v->children.begin();
      unparse(*it,tgtfile,indent);
      unparse(*(++it),tgtfile,indent);
      break;
    }
    case SEQUENCE_OF_SUBROUTINES_ASTV: {
      for (it=v->children.begin();it!=v->children.end();it++) 
        unparse((*it),tgtfile,indent);
      break;
    }
    case SUBROUTINE_ASTV: {

      tgtfile << "void " << tl_var_prefix;
      it=v->children.begin();
      (*it)->unparse(tgtfile,indent);
      tgtfile << "(";
      unparse(*(++it),tgtfile,indent);
      tgtfile << ")\n";
      tgtfile << "{\n";
      indent+=3;
      unparse(*(++it),tgtfile,indent);
      unparse(*(++it),tgtfile,indent);
      indent-=3;
      tgtfile << "}\n";
      break;
    }
    case SUBROUTINE_ARG_LIST_ASTV: {
      int s=1;
      for (it=v->children.begin();it!=v->children.end();it++,s++) {
        switch (static_cast<symbol_ast_vertex*>(*it)->sym->type) {
          case FLOAT_ST : {
            if (static_cast<symbol_ast_vertex*>(*it)->sym->shape==0) 
              tgtfile << "double& ";
            else {
              tgtfile << "double";
              for (int i=0; i<static_cast<symbol_ast_vertex*>(*it)->sym->shape; i++) 
                tgtfile << "*";
              tgtfile << " ";
            }
            break;
          }
          case INT_ST : {
            if (static_cast<symbol_ast_vertex*>(*it)->sym->shape==0) {
              if (static_cast<symbol_ast_vertex*>(*it)->sym->intent==0)
                tgtfile << "int& ";
              else
                tgtfile << "int ";
            }
            else {
              tgtfile << "int";
              for (int i=0; i<static_cast<symbol_ast_vertex*>(*it)->sym->shape; i++) 
                tgtfile << "*";
              tgtfile << " ";
            }
            break;
          }
          default : {
            cerr << "ERROR: Unknown Symbol Type " << static_cast<symbol_ast_vertex*>(*it)->sym->type << endl;
            assert(false);
          }
        }
        (*it)->unparse(tgtfile,indent);
        if (static_cast<symbol_ast_vertex*>(*it)->sym->type==FLOAT_ST) {
          tgtfile << ", ";
          if (static_cast<symbol_ast_vertex*>(*it)->sym->shape==0) 
            tgtfile << "double& "  << tl_var_prefix;
          else {
            tgtfile << "double";
            for (int i=0; i<static_cast<symbol_ast_vertex*>(*it)->sym->shape; i++) 
              tgtfile << "*";
            tgtfile << " " << tl_var_prefix;
          }
          (*it)->unparse(tgtfile,indent);
        }
        if (s<v->children.size()) 
          tgtfile << ", ";
      }
      break;
    }
    case SEQUENCE_OF_DECLARATIONS_ASTV: {
      for (it=v->children.begin();it!=v->children.end();it++) {
        static_cast<symbol_ast_vertex*>(*it)->sym->unparse(tgtfile,"double",indent);
        if (static_cast<symbol_ast_vertex*>(*it)->sym->type==FLOAT_ST) {
          symbol* sym=new symbol;
          sym->name=tl_var_prefix+static_cast<symbol_ast_vertex*>(*it)->sym->name;
          sym->kind=static_cast<symbol_ast_vertex*>(*it)->sym->kind;
          sym->type=static_cast<symbol_ast_vertex*>(*it)->sym->type;
          sym->shape=static_cast<symbol_ast_vertex*>(*it)->sym->shape;
          if (static_cast<symbol_ast_vertex*>(*it)->sym->shape==1) {
            sym->val=static_cast<symbol_ast_vertex*>(*it)->sym->val;
          } else {
            sym->val=string("0");
          }
          sym->unparse(tgtfile,"double",indent);
          delete sym;
        }
      }
      break;
    }
    case SEQUENCE_OF_STATEMENTS_ASTV: {
      for (it=v->children.begin();it!=v->children.end();it++) 
        unparse((*it),tgtfile,indent);
      break;
    }
    case IF_STATEMENT_ASTV: {
      for (int i=0;i<indent;i++) 
        tgtfile << " ";
      tgtfile << "if (";
      it=v->children.begin();
      (*it)->unparse(tgtfile,indent);
      tgtfile << ") {\n";
      indent+=3;
      unparse((*(++it)),tgtfile,indent);
      indent-=3;
      for (int i=0;i<indent;i++) 
        tgtfile << " ";
      tgtfile << "}\n";
      if (v->children.size()==3) {
        for (int i=0;i<indent;i++) 
          tgtfile << " ";
        tgtfile << "else {\n";
        indent+=3;
        unparse(*(++it),tgtfile,indent);
        indent-=3;
        for (int i=0;i<indent;i++) tgtfile << " ";
        tgtfile << "}\n";
      }
      break;
    }
    case WHILE_STATEMENT_ASTV: {
      for (int i=0;i<indent;i++) tgtfile << " ";
      tgtfile << "while (";
      it=v->children.begin();
      (*it)->unparse(tgtfile,indent);
      tgtfile << ") {\n";
      indent+=3;
      unparse((*(++it)),tgtfile,indent);
      indent-=3;
      for (int i=0;i<indent;i++) tgtfile << " ";
      tgtfile << "}\n";
      break;
    }
    case ASSIGNMENT_ASTV: {
      if (static_cast<scalar_memref_ast_vertex*>(*(v->children.begin()))->base->type==FLOAT_ST) {
          unparse((*(++(v->children.begin()))),tgtfile,indent);
          for (int i=0;i<indent;i++) tgtfile << " ";
          tgtfile << tl_var_prefix;
          (*(v->children.begin()))->unparse(tgtfile,0);
          tgtfile << "="
                   << tl_var_prefix << sac_var_prefix << (*(++(v->children.begin())))->sac_var_idx() << ";";
          tgtfile << "\n"; 
          for (int i=0;i<indent;i++) tgtfile << " ";
          (*(v->children.begin()))->unparse(tgtfile,0);
          tgtfile << "="
                   << sac_var_prefix << (*(++(v->children.begin())))->sac_var_idx() << ";";
          tgtfile << "\n"; 
      } else {
          v->unparse(tgtfile,indent);
      }  
      break; 
    }
    case SUBROUTINE_CALL_ASTV: {
      for (int i=0;i<indent;i++) tgtfile << " ";
      tgtfile << tl_var_prefix;
      (*(v->children.begin()))->unparse(tgtfile,indent);
      tgtfile << "(";
      for (it=(*(++(v->children.begin())))->children.begin();it!=(*(++(v->children.begin())))->children.end();it++) {
        if (it!=(*(++(v->children.begin())))->children.begin())
          tgtfile << ',';
        (*it)->unparse(tgtfile,indent);
        if (static_cast<scalar_memref_ast_vertex*>(*(it))->base->type==FLOAT_ST) {
          tgtfile << "," << tl_var_prefix;
          (*it)->unparse(tgtfile,indent);
        }
      }
      tgtfile << ");\n";
      break;
    }
    // y=x1+x2
    case ADD_EXPRESSION_ASTV: {
      unparse((*(v->children.begin())),tgtfile,indent);
      unparse((*(++(v->children.begin()))),tgtfile,indent);
      // tangent-linear code
      for (int i=0;i<indent;i++) tgtfile << " ";
        tgtfile << tl_var_prefix << sac_var_prefix << v->sac_var_idx() // d_y 
                << "="
                << tl_var_prefix << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // d_x1
                << "+" 
                << tl_var_prefix << sac_var_prefix << (*(++(v->children.begin())))->sac_var_idx() // d_x2
                << ";";
      tgtfile << endl; 
      for (int i=0;i<indent;i++) tgtfile << " ";
      tgtfile << sac_var_prefix << v->sac_var_idx() // y 
               << "=" 
               << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // x1
               << "+" 
               << sac_var_prefix << (*(++(v->children.begin())))->sac_var_idx() // x2
               << ";";
      tgtfile << endl; 
      break;
    }
    // y=x1-x2
    case SUB_EXPRESSION_ASTV: {
      unparse((*(v->children.begin())),tgtfile,indent);
      unparse((*(++(v->children.begin()))),tgtfile,indent);
      // tangent-linear code
      for (int i=0;i<indent;i++) tgtfile << " ";
        tgtfile << tl_var_prefix << sac_var_prefix << v->sac_var_idx() // d_y 
                << "=" 
                << tl_var_prefix << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // d_x1
                << "-" 
                << tl_var_prefix << sac_var_prefix << (*(++(v->children.begin())))->sac_var_idx() // d_x2
                << ";";

      tgtfile << endl; 
      for (int i=0;i<indent;i++) tgtfile << " ";
      tgtfile << sac_var_prefix << v->sac_var_idx() // y 
               << "=" 
               << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // x1
               << "-" 
               << sac_var_prefix << (*(++(v->children.begin())))->sac_var_idx() // x2
               << ";";
      tgtfile << endl; 
      break;
    }
    // y=x1*x2
    case MUL_EXPRESSION_ASTV: {
      unparse((*(v->children.begin())),tgtfile,indent);
      unparse((*(++(v->children.begin()))),tgtfile,indent);
      // tangent-linear code
      for (int i=0;i<indent;i++) tgtfile << " ";
        tgtfile << tl_var_prefix << sac_var_prefix << v->sac_var_idx() // d_y 
                << "=" 
                << sac_var_prefix << (*(++(v->children.begin())))->sac_var_idx() // x2
                << "*" 
                << tl_var_prefix << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // d_x1
                << "+" 
                << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // x1
                << "*" 
                << tl_var_prefix << sac_var_prefix << (*(++(v->children.begin())))->sac_var_idx() // d_x2
                << ";";
      tgtfile << endl; 
      for (int i=0;i<indent;i++) tgtfile << " ";
      tgtfile << sac_var_prefix << v->sac_var_idx() // y 
               << "=" 
               << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // x1
               << "*" 
               << sac_var_prefix << (*(++(v->children.begin())))->sac_var_idx() // x2
               << ";";
      tgtfile << endl; 
      break;
    }
    // y=x1/x2
    case DIV_EXPRESSION_ASTV: {
      unparse((*(v->children.begin())),tgtfile,indent);
      unparse((*(++(v->children.begin()))),tgtfile,indent);
      // tangent-linear code
      for (int i=0;i<indent;i++) tgtfile << " ";
      tgtfile << sac_var_prefix << v->sac_var_idx() // y 
               << "=1/" 
               << sac_var_prefix << (*(++(v->children.begin())))->sac_var_idx() // x2
               << "; ";
      for (int i=0;i<indent;i++) tgtfile << " ";
        tgtfile << tl_var_prefix << sac_var_prefix << v->sac_var_idx() // d_y 
                << "=" 
                << sac_var_prefix << v->sac_var_idx() // y
                << "*" 
                << tl_var_prefix << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // d_x1
                << "-" 
                << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // x1
                << "*" 
                << sac_var_prefix << v->sac_var_idx() // y
                << "*" 
                << sac_var_prefix << v->sac_var_idx() // y
                << "*" 
                << tl_var_prefix << sac_var_prefix << (*(++(v->children.begin())))->sac_var_idx() // d_x2
                << ";";
      tgtfile << endl; 
      for (int i=0;i<indent;i++) tgtfile << " ";
      tgtfile << sac_var_prefix << v->sac_var_idx() // y 
               << "=" 
               << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // x1
               << "*" 
               << sac_var_prefix << v->sac_var_idx() // y
               << ";";
      tgtfile << endl; 
      break;
    }
    // y=(x);
    case PAR_EXPRESSION_ASTV: {
      unparse((*(v->children.begin())),tgtfile,indent);
      // tangent-linear code
      for (int i=0;i<indent;i++) tgtfile << " ";
        tgtfile << tl_var_prefix << sac_var_prefix << v->sac_var_idx() // d_y 
                << "=" 
                << tl_var_prefix << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // d_x
                << ";";
      tgtfile << endl; 
      for (int i=0;i<indent;i++) tgtfile << " ";
      tgtfile << sac_var_prefix << v->sac_var_idx() // y 
              << "=" 
              << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // x
              << ";";
      tgtfile << endl; 
      break;
    }
    // y=sin(x);
    case SIN_EXPRESSION_ASTV: {
      unparse((*(v->children.begin())),tgtfile,indent);
      // tangent-linear code
      for (int i=0;i<indent;i++) tgtfile << " ";
        tgtfile << tl_var_prefix << sac_var_prefix << v->sac_var_idx() // d_y 
                << "=" 
                << "cos(" 
                << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // x
                << ")*" 
                << tl_var_prefix << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // d_x
                << ";";
      tgtfile << endl; 
      for (int i=0;i<indent;i++) tgtfile << " ";
      tgtfile << sac_var_prefix << v->sac_var_idx() // y 
              << "=sin(" 
              << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // x
              << ");";
      tgtfile << endl; 
      break;
    }
    // y=cos(x);
    case COS_EXPRESSION_ASTV: {
      unparse((*(v->children.begin())),tgtfile,indent);
      // tangent-linear code
      for (int i=0;i<indent;i++) tgtfile << " ";
        tgtfile << tl_var_prefix << sac_var_prefix << v->sac_var_idx() // d_y 
                 << "=" 
                 << "0-sin(" 
                 << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // x
                 << ")*" 
                 << tl_var_prefix << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // d_x
                 << ";";
      tgtfile << endl; 
      for (int i=0;i<indent;i++) tgtfile << " ";
      tgtfile << sac_var_prefix << v->sac_var_idx() // y 
               << "=cos(" 
               << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // x
               << ");";
      tgtfile << endl; 
      break;
    }
    // y=tan(x);
    case TAN_EXPRESSION_ASTV: {
      unparse((*(v->children.begin())),tgtfile,indent);
      // tangent-linear code
      for (int i=0;i<indent;i++) tgtfile << " ";
        tgtfile << tl_var_prefix << sac_var_prefix << v->sac_var_idx() // d_y
                 << "=1/("
                 << "cos("
                 << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // x
                 << ")*"
                 << "cos("
                 << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // x
                 << "))*"
                 << tl_var_prefix << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // d_x
                 << ";";
      tgtfile << endl;
      for (int i=0;i<indent;i++) tgtfile << " ";
      tgtfile << sac_var_prefix << v->sac_var_idx() // y
               << "=tan("
               << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // x
               << ");";
      tgtfile << endl;
      break;
    }

    // y=exp(x);
    case EXP_EXPRESSION_ASTV: {
      unparse((*(v->children.begin())),tgtfile,indent);
      // tangent-linear code
      for (int i=0;i<indent;i++) tgtfile << " ";
      tgtfile << sac_var_prefix << v->sac_var_idx() // y 
              << "=exp(" 
              << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // x
              << ");";
      tgtfile << endl; 
      for (int i=0;i<indent;i++) tgtfile << " ";
        tgtfile << tl_var_prefix << sac_var_prefix << v->sac_var_idx() // d_y 
                << "=" 
                << sac_var_prefix << v->sac_var_idx() // y
                << "*" 
                << tl_var_prefix << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // d_x
                << ";";
      tgtfile << endl; 
      break;
    }
    // y=sqrt(x);
    case SQRT_EXPRESSION_ASTV: {
      unparse((*(v->children.begin())),tgtfile,indent);
      // tangent-linear code
      for (int i=0;i<indent;i++) tgtfile << " ";
        tgtfile << tl_var_prefix << sac_var_prefix << v->sac_var_idx() // d_y 
                << "=" 
                << "1/(2*sqrt(" 
                << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // x
                << "))*" 
                << tl_var_prefix << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // d_x
                << ";";
      tgtfile << endl; 
      for (int i=0;i<indent;i++) tgtfile << " ";
      tgtfile << sac_var_prefix << v->sac_var_idx() // y 
               << "=sqrt(" 
               << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // x
               << ");";
      tgtfile << endl; 
      break;
    }
    // y=atan(x);
    case ATAN_EXPRESSION_ASTV: {
      unparse((*(v->children.begin())),tgtfile,indent);
      // tangent-linear code
      for (int i=0;i<indent;i++) tgtfile << " ";
        tgtfile << tl_var_prefix << sac_var_prefix << v->sac_var_idx() // d_y 
                << "=" 
                << "1/(1+" 
                << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // x
                << "*" 
                << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // x
                << ")*" 
                << tl_var_prefix << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // d_x
                << ";";
      tgtfile << endl; 
      for (int i=0;i<indent;i++) tgtfile << " ";
      tgtfile << sac_var_prefix << v->sac_var_idx() // y 
               << "=atan(" 
               << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // x
               << ");";
      tgtfile << endl; 
      break;
    }
    // y=log(x);
    case LOG_EXPRESSION_ASTV: {
      unparse((*(v->children.begin())),tgtfile,indent);
      // tangent-linear code
      for (int i=0;i<indent;i++) tgtfile << " ";
        tgtfile << tl_var_prefix << sac_var_prefix << v->sac_var_idx() // d_y
                << "="
                << tl_var_prefix << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // d_x
                << "/"
                << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // x
                << ";";
      tgtfile << endl;
      for (int i=0;i<indent;i++) tgtfile << " ";
      tgtfile << sac_var_prefix << v->sac_var_idx() // y
               << "=log("
               << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // x
               << ");";
      tgtfile << endl;
      break;
    }

    // y=pow(x,c); c integer
    case POW_I_EXPRESSION_ASTV: {
      unparse((*(v->children.begin())),tgtfile,indent);
      // tangent-linear code
      for (int i=0;i<indent;i++) tgtfile << " ";
        tgtfile << tl_var_prefix << sac_var_prefix << v->sac_var_idx() // d_y as aux
                << "=" 
                << static_cast<symbol_ast_vertex*>(*(++(v->children.begin())))->sym->name // c
                << "-1; ";
        tgtfile << tl_var_prefix << sac_var_prefix << v->sac_var_idx() // d_y 
                << "=" 
                << static_cast<symbol_ast_vertex*>(*(++(v->children.begin())))->sym->name // c
                << "*pow(" 
                << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // x
                << "," 
                << tl_var_prefix << sac_var_prefix << v->sac_var_idx() // d_y as aux
                << ")*" 
                << tl_var_prefix << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // d_x
                << ";";
      tgtfile << endl; 
      for (int i=0;i<indent;i++) tgtfile << " ";
      tgtfile << sac_var_prefix << v->sac_var_idx() // y 
               << "=pow(" 
               << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // x
               << ","
               << static_cast<symbol_ast_vertex*>(*(++(v->children.begin())))->sym->name // c
               << ");";
      tgtfile << endl; 
      break;
    }
    // y=pow(x,z)
    case POW_F_EXPRESSION_ASTV: {
      unparse((*(v->children.begin())),tgtfile,indent);
      // tangent-linear code
      for (int i=0;i<indent;i++) tgtfile << " ";
      tgtfile << sac_var_prefix << v->sac_var_idx() // y
               << "=pow("
               << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // x
               << ","
               << static_cast<symbol_ast_vertex*>(*(++(v->children.begin())))->sym->name // z
               << ");";
      tgtfile << endl;
      for (int i=0;i<indent;i++) tgtfile << " ";
        tgtfile << tl_var_prefix << sac_var_prefix << v->sac_var_idx() // d_y as aux
                << "="
                << static_cast<symbol_ast_vertex*>(*(++(v->children.begin())))->sym->name // z
                << "-1; ";
        tgtfile << tl_var_prefix << sac_var_prefix << v->sac_var_idx() // d_y
                << "="
                << static_cast<symbol_ast_vertex*>(*(++(v->children.begin())))->sym->name // z
                << "*pow("
                << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // x
                << ","
                << tl_var_prefix << sac_var_prefix << v->sac_var_idx() // d_y as aux
                << ")*"
                << tl_var_prefix << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // d_x
                << "+"
                << sac_var_prefix << v->sac_var_idx() // y
                << "*log("
                << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // x
                << ")*"
                << tl_var_prefix << static_cast<symbol_ast_vertex*>(*(++(v->children.begin())))->sym->name // d_z
                << ";";
      tgtfile << endl;
      break;
    }

    // y=x;
    case SCALAR_MEMREF_ASTV: {
      // tangent-linear code
      for (int i=0;i<indent;i++) tgtfile << " ";
      tgtfile << tl_var_prefix << sac_var_prefix << v->sac_var_idx() // d_y 
               << "="; 
      if (static_cast<scalar_memref_ast_vertex*>(v)->base->type==FLOAT_ST)
        tgtfile  << tl_var_prefix << static_cast<scalar_memref_ast_vertex*>(v)->base->name; // d_x
      else 
        tgtfile << "0";
      tgtfile << ";\n"; 
      for (int i=0;i<indent;i++) tgtfile << " ";
      tgtfile << sac_var_prefix << v->sac_var_idx() // y 
               << "=" 
               << static_cast<scalar_memref_ast_vertex*>(v)->base->name // x
               << ";";
      tgtfile << "\n"; 
      break;
    }
    // y=x[i]...[j];
    case ARRAY_MEMREF_ASTV: {
      // tangent-linear code
      for (int i=0;i<indent;i++) tgtfile << " ";
      tgtfile << tl_var_prefix << sac_var_prefix << v->sac_var_idx() // d_y
               << "=";
      if (static_cast<array_memref_ast_vertex*>(v)->base->type==FLOAT_ST) {
        tgtfile << tl_var_prefix << static_cast<array_memref_ast_vertex*>(v)->base->name; // d_x
        for (int i=0;i<static_cast<array_memref_ast_vertex*>(v)->dim;i++)
          tgtfile << "["
                  << static_cast<array_memref_ast_vertex*>(v)->offsets[i]->name // i
                   << "]";
      }
      else 
        tgtfile << "0";
      tgtfile << ";\n"; 
      for (int i=0;i<indent;i++) tgtfile << " ";
      tgtfile << sac_var_prefix << v->sac_var_idx() // y
               << "="
               << static_cast<array_memref_ast_vertex*>(v)->base->name; // x
      for (int i=0;i<static_cast<array_memref_ast_vertex*>(v)->dim;i++)
        tgtfile << "["
                 << static_cast<array_memref_ast_vertex*>(v)->offsets[i]->name // i
                 << "]";
      tgtfile << ";";
      tgtfile << "\n"; 
      break;
    }
    case CONSTANT_ASTV: {
      // tangent-linear code
      for (int i=0;i<indent;i++) tgtfile << " ";
      tgtfile << tl_var_prefix << sac_var_prefix << v->sac_var_idx() // d_y 
               << "=0;"; 
      tgtfile << "\n"; 
      for (int i=0;i<indent;i++) tgtfile << " ";
      tgtfile << sac_var_prefix << v->sac_var_idx() // y 
               << "=" 
               << static_cast<const_ast_vertex*>(v)->value->name
               << ";";
      tgtfile << "\n"; 
      break;
    }
    default: {
      cerr << "Unknown ast vertex type " << v->type;
      exit(-1);
      break;
    }
  }
}


