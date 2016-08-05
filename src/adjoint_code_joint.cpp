#include <iostream>
#include <cstdlib>
#include <sstream>
#include <cmath>
#include "adjoint_code_joint.hpp"

using namespace std;

extern ast the_ast;

extern string adj_var_prefix;
extern string adj_file_prefix;
extern string sac_var_prefix;
extern string css;
extern string fdss;
extern string idss;
extern string cs;
extern string csc;
extern string fds;
extern string fdsc;
extern string ids;
extern string idsc;

extern string amode_var_name;

unsigned int amode_adjoint=1;
unsigned int amode_store_inputs=2;
unsigned int amode_restore_inputs=3;

void adjoint_code_joint::build(const string& infile_name, 
                         const string& outfile_name) {
  ofstream outfile(outfile_name.c_str());
  vector<list<string> > adj_bbs;
  if (the_ast.ast_root) {
    unparse(the_ast.ast_root,outfile,0,adj_bbs,infile_name);
  }
  outfile.close();
}

void adjoint_code_joint::unparse(ast_vertex* v, ofstream& outfile,
                             int indent, vector<list<string> >& adj_bbs,
                             const string& infile_name) {
  static bool new_bb=true;
  static int bb_idx=-1;
  static list<string> adj_bb;
  static list<string> f_asg;
  static list<string> r_asg;
  if (!v) return;
  list<ast_vertex*>::iterator it;
  switch (v->type) {
    case CODE_ASTV: {
      // unparse local symbol table
      it=v->children.begin();
      unparse(*it,outfile,indent,adj_bbs,infile_name);
      // control stack
      bool n;
      symbol* sym=static_cast<scope_ast_vertex*>(v)->stab->insert(cs);
      sym->kind=GLOBAL_VAR_SK; 
      sym->type=INT_ST;
      sym->shape=1;
      sym->val=css;
      sym->unparse(outfile,"double",indent);

      // control stack counter
      sym=static_cast<scope_ast_vertex*>(v)->stab->insert(csc);
      sym->kind=GLOBAL_VAR_SK; 
      sym->type=INT_ST;
      sym->shape=0;
      sym->val=string("0");
      sym->unparse(outfile,"double",indent);

      // float data stack
      sym=static_cast<scope_ast_vertex*>(v)->stab->insert(fds);
      sym->kind=GLOBAL_VAR_SK; 
      sym->type=FLOAT_ST;
      sym->shape=1;
      sym->val=fdss;
      sym->unparse(outfile,"double",indent);

      // float data stack counter
      sym=static_cast<scope_ast_vertex*>(v)->stab->insert(fdsc);
      sym->kind=GLOBAL_VAR_SK; 
      sym->type=INT_ST;
      sym->shape=0;
      sym->val=string("0");
      sym->unparse(outfile,"double",indent);

      // integer data stack
      sym=static_cast<scope_ast_vertex*>(v)->stab->insert(ids);
      sym->kind=GLOBAL_VAR_SK; 
      sym->type=INT_ST;
      sym->shape=1;
      sym->val=idss;
      sym->unparse(outfile,"double",indent);

      // integer data stack counter
      sym=static_cast<scope_ast_vertex*>(v)->stab->insert(idsc);
      sym->kind=GLOBAL_VAR_SK; 
      sym->type=INT_ST;
      sym->shape=0;
      sym->val=string("0");
      sym->unparse(outfile,"double",indent);

      outfile << "#include \"" << "declare_checkpoints.inc\"" << endl;
      outfile << "#include \"" <<  infile_name <<"\"" << endl;
      unparse(*(++it),outfile,indent,adj_bbs,infile_name);
      break;
    }
    case SEQUENCE_OF_SUBROUTINES_ASTV: {
      for (it=v->children.begin();it!=v->children.end();it++) {
        unparse((*it), outfile,indent,adj_bbs,infile_name);
      }
      break;
    }
    case SUBROUTINE_ASTV: {
      string subroutine_name;
      adj_bbs.clear();
      bb_idx=-1;

      outfile << "void " << adj_var_prefix;
      it=v->children.begin();
      (*it)->unparse(outfile,indent);
      subroutine_name=static_cast<symbol_ast_vertex*>(*it)->sym->name;
      outfile << "(";
      unparse(*(++it),outfile,indent,adj_bbs,infile_name);
      outfile << ")\n";
      outfile << "{\n";
      indent+=3;
      // unparse local symbol table
      unparse(*(++it),outfile,indent,adj_bbs,infile_name);
      for (int ii=0;ii<indent;ii++) outfile << " ";
      outfile << "int save_" << csc << "=0;" << endl;
      for (int ii=0;ii<indent;ii++) outfile << " ";
      outfile << "save_" << csc << "=" << csc << ";" << endl;

      for (int ii=0;ii<indent;ii++) outfile << " ";
      outfile << "if (" << amode_var_name << "=="
               << amode_adjoint << ") {" << endl;
      indent+=3;
      // unparse augmented forward section
      for (int ii=0;ii<indent;ii++) outfile << " ";
      outfile << "// augmented forward section" << endl;
      unparse(*(++it),outfile,indent,adj_bbs,infile_name);

      outfile << "#include \"" << subroutine_name << "_store_results.inc\"" << endl;

      // unparse reverse section
      for (int ii=0;ii<indent;ii++) outfile << " ";
      outfile << "// reverse section" << endl;
      for (int ii=0;ii<indent;ii++) outfile << " ";
      outfile << "while ("
               << csc
               << ">save_" << csc << ") {\n"
      ;
      for (int ii=0;ii<indent;ii++) outfile << " ";
      outfile << "  " << csc << "=" << csc << "-1;\n"; 
      for (int i=0;i<adj_bbs.size();i++)
        if (!adj_bbs[i].empty()) {
          for (int ii=0;ii<indent;ii++) outfile << " ";
          outfile << "  if ("
                   << cs
                   << "["
                   << csc
                   << "]=="
                   << i
                   << ") {\n"
          ;
          list<string>::iterator j;
          for (j=adj_bbs[i].begin();j!=adj_bbs[i].end();j++) {
            for (int ii=0;ii<indent+4;ii++) outfile << " ";
            outfile << *j;
          }
          for (int ii=0;ii<indent;ii++) outfile << " ";
          outfile << "  }\n";
        }
      for (int ii=0;ii<indent;ii++) outfile << " "; outfile << "}\n";
      indent-=3;
      outfile << "#include \"" << subroutine_name << "_restore_results.inc\"" << endl;
      for (int ii=0;ii<indent;ii++) outfile << " ";
      outfile << "}" << endl;

      // store inputs
      for (int ii=0;ii<indent;ii++) outfile << " ";
      outfile << "if (" << amode_var_name << "=="
               << amode_store_inputs << ") {" << endl;
      // user inserts code
      indent+=3;
      for (int ii=0;ii<indent;ii++) outfile << " ";
      outfile << "#include \"" << subroutine_name << "_store_inputs.inc\"" << endl;
      for (int ii=0;ii<indent;ii++) outfile << " ";
      outfile << amode_var_name << "=" << amode_var_name << ";" << endl;
      indent-=3;
      for (int ii=0;ii<indent;ii++) outfile << " ";
      outfile << "}" << endl;
      // restore inputs
      for (int ii=0;ii<indent;ii++) outfile << " ";
      outfile << "if (" << amode_var_name << "=="
               << amode_restore_inputs << ") {" << endl;
      // user inserts code
      indent+=3;
      for (int ii=0;ii<indent;ii++) outfile << " ";
      outfile << "#include \"" << subroutine_name << "_restore_inputs.inc\"" << endl;
      for (int ii=0;ii<indent;ii++) outfile << " ";
      outfile << amode_var_name << "=" << amode_var_name << ";" << endl;
      indent-=3;
      for (int ii=0;ii<indent;ii++) outfile << " ";
      outfile << "}" << endl;
      outfile << "}\n";
      new_bb=true;
      break;
    }
    case SUBROUTINE_ARG_LIST_ASTV: {
      int s=1;
      outfile << "int " << amode_var_name << ", ";
      for (it=v->children.begin();it!=v->children.end();it++,s++) {
        switch (static_cast<symbol_ast_vertex*>(*it)->sym->type) {
          case FLOAT_ST : {
            if (static_cast<symbol_ast_vertex*>(*it)->sym->shape==0)
              outfile << "double& ";
            else {
              outfile << "double";
              for (int i=0; i<static_cast<symbol_ast_vertex*>(*it)->sym->shape; i++)
                outfile << "*";
              outfile << " ";
            }
            break;
          }
          case INT_ST : {
            if (static_cast<symbol_ast_vertex*>(*it)->sym->shape==0) {
              if (static_cast<symbol_ast_vertex*>(*it)->sym->intent==0)
                outfile << "int& ";
              else
                outfile << "int ";
            }
            else {
              outfile << "int";
              for (int i=0; i<static_cast<symbol_ast_vertex*>(*it)->sym->shape; i++)
                outfile << "*";
              outfile << " ";
            }
            break;
          }
          default : {
             cerr << "ERROR: Unknown Symbol Type\n";
            exit (-1);
          }
        }
        (*it)->unparse(outfile,indent);
        if (static_cast<symbol_ast_vertex*>(*it)->sym->type==FLOAT_ST) {
          outfile << ", ";
          if (static_cast<symbol_ast_vertex*>(*it)->sym->shape==0)
            outfile << "double& "
                     << adj_var_prefix
            ;
          else {
            outfile << "double";
            for (int i=0; i<static_cast<symbol_ast_vertex*>(*it)->sym->shape; i++)
              outfile << "*";
            outfile << " " << adj_var_prefix;
          }
          (*it)->unparse(outfile,indent);
        }
        if (s<v->children.size()) outfile << ", ";
      }
      break;
    }
    case SEQUENCE_OF_DECLARATIONS_ASTV: {
      for (it=v->children.begin();it!=v->children.end();it++) {
        if (static_cast<symbol_ast_vertex*>(*it)->sym->kind!=GLOBAL_VAR_SK) 
          static_cast<symbol_ast_vertex*>(*it)->sym->unparse(outfile,"double",indent);
        if (static_cast<symbol_ast_vertex*>(*it)->sym->type==FLOAT_ST) {
          symbol* sym=new symbol;
          sym->name=adj_var_prefix+static_cast<symbol_ast_vertex*>(*it)->sym->name;
          sym->kind=static_cast<symbol_ast_vertex*>(*it)->sym->kind;
          sym->type=static_cast<symbol_ast_vertex*>(*it)->sym->type;
          sym->shape=static_cast<symbol_ast_vertex*>(*it)->sym->shape;
          if (static_cast<symbol_ast_vertex*>(*it)->sym->shape==1) {
            sym->val=static_cast<symbol_ast_vertex*>(*it)->sym->val;
          } else {
            sym->val=string("0");
          }
          sym->unparse(outfile,"double",indent);
          delete sym;
        }
      }
      break;
    }
    case SEQUENCE_OF_STATEMENTS_ASTV: {
      for (it=v->children.begin();it!=v->children.end();it++)
        unparse((*it), outfile,indent,adj_bbs,infile_name);
      break;
    }
    case IF_STATEMENT_ASTV: {
      new_bb=true;
      for (int i=0;i<indent;i++) outfile << " ";
      outfile << "if (";
      it=v->children.begin();
      (*it)->unparse(outfile,indent);
      outfile << ") {\n";
      indent+=3;
      unparse((*(++it)), outfile,indent,adj_bbs,infile_name);
      indent-=3;
      for (int i=0;i<indent;i++) outfile << " ";
      outfile << "}\n";
      if (v->children.size()==3) {
        new_bb=true;
        for (int i=0;i<indent;i++) outfile << " ";
        outfile << "else {\n";
        indent+=3;
        unparse((*(++it)), outfile,indent,adj_bbs,infile_name);
        indent-=3;
        for (int i=0;i<indent;i++) outfile << " ";
        outfile << "}\n";
      }
      new_bb=true;
      break;
    }
    case WHILE_STATEMENT_ASTV: {
      new_bb=true;
      for (int i=0;i<indent;i++) outfile << " ";
      outfile << "while (";
      it=v->children.begin();
      (*it)->unparse(outfile,indent);
      outfile << ") {\n";
      indent+=3;
      unparse((*(++it)), outfile,indent,adj_bbs,infile_name);
      indent-=3;
      for (int i=0;i<indent;i++) outfile << " ";
      outfile << "}\n";
      new_bb=true;
      break;
    }
    case ASSIGNMENT_ASTV: {
      if (new_bb) { // push bb_idx
        new_bb=false;
        adj_bb.clear();
        for (int i=0;i<indent;i++) outfile << " ";
        for (int i=0;i<indent;i++) outfile << " ";
        outfile << cs << "[" << csc << "]=" << ++bb_idx << "; ";
        outfile << csc << "=" << csc << "+1;\n";
      }

      for (int i=0;i<indent;i++) outfile << " ";
      for (int i=0;i<indent;i++) outfile << " ";
      if (static_cast<scalar_memref_ast_vertex*>(*(v->children.begin()))->base->type==FLOAT_ST) {
          outfile << fds << "[" << fdsc << "]=" 
                  << v->lhs_string() << "; ";
        outfile << fdsc << "=" << fdsc << "+1; ";
      }
      else if (static_cast<scalar_memref_ast_vertex*>(*(v->children.begin()))->base->type==INT_ST) {
          outfile << ids << "[" << idsc << "]=" 
                   << v->lhs_string() << "; ";
        outfile << idsc << "=" << idsc << "+1; ";
      }
      else {
        cerr << "Unknown data type " << static_cast<scalar_memref_ast_vertex*>(*(v->children.begin()))->base->type << endl;
        exit(-1);
      }
    
      v->unparse(outfile,0);

      f_asg.clear();
      r_asg.clear();
      string local_sac, local_adj;
      ostringstream local_sacs, local_adjs;
      for (int i=0;i<10;i++) local_adjs << " ";
        // local adjoint
      if (static_cast<scalar_memref_ast_vertex*>(*(v->children.begin()))->base->type==FLOAT_ST) {
        local_adjs << adj_var_prefix << sac_var_prefix 
                     << (*(++(v->children.begin())))->sac_var_idx() << "="
                     << adj_var_prefix << v->lhs_string() << "; " 
                     << adj_var_prefix << v->lhs_string() << "=0;" << endl;
        local_adj=local_adjs.str();
        unparse(*(++(v->children.begin())),outfile,indent,adj_bbs,infile_name);
        r_asg.push_front(local_adj);
      }
      // restore overwritten value
      for (int i=0;i<10;i++) local_sacs << " ";
      if (static_cast<scalar_memref_ast_vertex*>(*(v->children.begin()))->base->type==FLOAT_ST) {
        local_sacs << fdsc << "=" << fdsc << "-1; "; 
          local_sacs << v->lhs_string() << "=" 
                 << fds << "[" << fdsc << "];";
        local_sacs << endl;
        local_sac=local_sacs.str();
      }
      else if (static_cast<scalar_memref_ast_vertex*>(*(v->children.begin()))->base->type==INT_ST) {
        local_sacs << idsc << "=" << idsc << "-1; "; 
          local_sacs << v->lhs_string() << "=" 
                  << ids << "[" << idsc << "];" << endl;
        local_sac=local_sacs.str();
      }
      else {
        cerr << "Unknown data type " << static_cast<scalar_memref_ast_vertex*>(*(v->children.begin()))->base->type << endl;
        exit(-1);
      }
      f_asg.push_front(local_sac);
      

      list<string>::reverse_iterator rit;
        for (rit=r_asg.rbegin();rit!=r_asg.rend();rit++)
          adj_bb.push_front(*rit);
      for (rit=f_asg.rbegin();rit!=f_asg.rend();rit++)
        adj_bb.push_front(*rit);

      if (adj_bbs.size()<=bb_idx)
        adj_bbs.push_back(adj_bb);
      else
        adj_bbs[bb_idx]=adj_bb;
      break;
    }
    case SUBROUTINE_CALL_ASTV: {
      // augmented forward
      if (new_bb) { // push bb_idx
        new_bb=false;
        adj_bb.clear();
        for (int i=0;i<indent;i++) outfile << " ";
        outfile << cs << "[" << csc << "]=" << ++bb_idx << "; "
                  << csc << "=" << csc << "+1;\n";
      }
      for (int i=0;i<indent;i++) outfile << " ";
      // store argument checkpoint
      outfile << adj_var_prefix;
      (*(v->children.begin()))->unparse(outfile,0);
      outfile << "(" << amode_store_inputs << ",";
      for (it=(*(++(v->children.begin())))->children.begin();it!=(*(++(v->children.begin())))->children.end();it++) {
        if (it!=(*(++(v->children.begin())))->children.begin())
          outfile << ',';
        (*it)->unparse(outfile,indent);
        if (static_cast<scalar_memref_ast_vertex*>(*(it))->base->type==FLOAT_ST)
 {
          outfile << "," << adj_var_prefix;
          (*it)->unparse(outfile,indent);
        }
      }
      outfile << ");\n";
      v->unparse(outfile,indent);
      // backward
      ostringstream os;
      // restore argument checkpoint
      for (int i=0;i<indent;i++) os << " ";
      os << adj_var_prefix;
      (*(v->children.begin()))->unparse(os,0);
      os << "(" << amode_restore_inputs << ",";
      for (it=(*(++(v->children.begin())))->children.begin();it!=(*(++(v->children.begin())))->children.end();it++) {
        if (it!=(*(++(v->children.begin())))->children.begin())
          os << ",";
        (*it)->unparse(os,0);
        if (static_cast<scalar_memref_ast_vertex*>(*(it))->base->type==FLOAT_ST)
 {
          os << "," << adj_var_prefix;
          (*it)->unparse(os,indent);
        }
      }
      os << ");\n";
      // augmented forward sweep
      for (int i=0;i<indent;i++) os << " ";
      os << adj_var_prefix;
      (*(v->children.begin()))->unparse(os,0);
      os << "(" << amode_adjoint << ",";
      for (it=(*(++(v->children.begin())))->children.begin();it!=(*(++(v->children.begin())))->children.end();it++) {
        if (it!=(*(++(v->children.begin())))->children.begin())
          os << ",";
        (*it)->unparse(os,0);
        if (static_cast<scalar_memref_ast_vertex*>(*(it))->base->type==FLOAT_ST)
 {
          os << "," << adj_var_prefix;
          (*it)->unparse(os,indent);
        }
      }
      os << ");\n";

      adj_bb.push_front(os.str().c_str());

      if (adj_bbs.size()<=bb_idx)
        adj_bbs.push_back(adj_bb);
      else
        adj_bbs[bb_idx]=adj_bb;

      break;
    }
    case ADD_EXPRESSION_ASTV: {
      // depth_first, post-order AST traversal
      unparse(*(v->children.begin()),outfile,indent,adj_bbs,infile_name);
      unparse(*(++(v->children.begin())),outfile,indent,adj_bbs,infile_name);
      // local single assignment code
      string local_sac;
      ostringstream local_sacs;
      local_sacs << sac_var_prefix << (v)->sac_var_idx() << "=" << sac_var_prefix
                << (*(v->children.begin()))->sac_var_idx() << "+"
                << sac_var_prefix << (*(++(v->children.begin())))->sac_var_idx()
                << ";" << endl;
      local_sac=local_sacs.str(); 
      f_asg.push_back(local_sac);
      // local adjoint
      string local_adj;
      ostringstream local_adjs;
        local_adjs << adj_var_prefix << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() 
                   << "="
                   << adj_var_prefix << sac_var_prefix << (v)->sac_var_idx() 
                   << "; "
                   << adj_var_prefix << sac_var_prefix << (*(++(v->children.begin())))->sac_var_idx() 
                   << "="
                   << adj_var_prefix << sac_var_prefix << (v)->sac_var_idx()
                   << ";" << endl;
      local_adj=local_adjs.str();
      r_asg.push_front(local_adj);
      break;
    }
    case SUB_EXPRESSION_ASTV: {
      // depth_first, post-order AST traversal
      unparse(*(v->children.begin()),outfile,indent,adj_bbs,infile_name);
      unparse(*(++(v->children.begin())),outfile,indent,adj_bbs,infile_name);
      // local single assignment code
      string local_sac;
      ostringstream local_sacs;
      local_sacs << sac_var_prefix << (v)->sac_var_idx() << "=" << sac_var_prefix
                << (*(v->children.begin()))->sac_var_idx() << "-"
                << sac_var_prefix << (*(++(v->children.begin())))->sac_var_idx()
                << ";" << endl;
      local_sac=local_sacs.str(); 
      f_asg.push_back(local_sac);
      // local adjoint
      string local_adj;
      ostringstream local_adjs;
        local_adjs << adj_var_prefix << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() 
                   << "="
                   << adj_var_prefix << sac_var_prefix << (v)->sac_var_idx() 
                   << "; "
                   << adj_var_prefix << sac_var_prefix << (*(++(v->children.begin())))->sac_var_idx() 
                   << "=0-"
                   << adj_var_prefix << sac_var_prefix << (v)->sac_var_idx()
                   << ";" << endl;
      local_adj=local_adjs.str();
      r_asg.push_front(local_adj);
      break;
    }
    case MUL_EXPRESSION_ASTV: {
      // depth_first, post-order AST traversal
      unparse(*(v->children.begin()),outfile,indent,adj_bbs,infile_name);
      unparse(*(++(v->children.begin())),outfile,indent,adj_bbs,infile_name);
      // local single assignment code
      string local_sac;
      ostringstream local_sacs;
      local_sacs << sac_var_prefix << (v)->sac_var_idx() << "=" << sac_var_prefix
                << (*(v->children.begin()))->sac_var_idx() << "*"
                << sac_var_prefix << (*(++(v->children.begin())))->sac_var_idx()
                << ";" << endl;
      local_sac=local_sacs.str(); 
      f_asg.push_back(local_sac);
      // local adjoint
      string local_adj;
      ostringstream local_adjs;
        local_adjs << adj_var_prefix << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() 
                   << "="
                   << sac_var_prefix << (*(++(v->children.begin())))->sac_var_idx() 
                   << "*"
                   << adj_var_prefix << sac_var_prefix << (v)->sac_var_idx() 
                   << "; " 
                   << adj_var_prefix << sac_var_prefix << (*(++(v->children.begin())))->sac_var_idx()
                   << "="
                   << sac_var_prefix << (*(v->children.begin()))->sac_var_idx()
                   << "*"
                   << adj_var_prefix << sac_var_prefix << (v)->sac_var_idx()
                   << ";" << endl;
      local_adj=local_adjs.str();
      r_asg.push_front(local_adj);
      break;
    }
    case DIV_EXPRESSION_ASTV: {
      // depth_first, post-order AST traversal
      unparse(*(v->children.begin()),outfile,indent,adj_bbs,infile_name);
      unparse(*(++(v->children.begin())),outfile,indent,adj_bbs,infile_name);
      // local single assignment code
      string local_sac;
      ostringstream local_sacs;
      local_sacs << sac_var_prefix << (v)->sac_var_idx() << "=" << sac_var_prefix
                << (*(v->children.begin()))->sac_var_idx() << "/"
                << sac_var_prefix << (*(++(v->children.begin())))->sac_var_idx()
                << ";" << endl;
      local_sac=local_sacs.str(); 
      f_asg.push_back(local_sac);
      // local adjoint
      string local_adj;
      ostringstream local_adjs;
        local_adjs << adj_var_prefix << sac_var_prefix << (*(v->children.begin()))->sac_var_idx()
                   << "=1/"
                   << sac_var_prefix << (*(++(v->children.begin())))->sac_var_idx()
                   << "*" 
                   << adj_var_prefix << sac_var_prefix << (v)->sac_var_idx()
                   << "; " 
                   << adj_var_prefix << sac_var_prefix << (*(++(v->children.begin())))->sac_var_idx()
                   << "=0-"
                   << sac_var_prefix << (*(v->children.begin()))->sac_var_idx()
                   << "/("
                   << sac_var_prefix << (*(++(v->children.begin())))->sac_var_idx()
                   << "*"
                   << sac_var_prefix << (*(++(v->children.begin())))->sac_var_idx()
                   << ")*"
                   << adj_var_prefix << sac_var_prefix << (v)->sac_var_idx()
                   << ";" << endl;
      local_adj=local_adjs.str();
      r_asg.push_front(local_adj);
      break;
    }
    case SIN_EXPRESSION_ASTV: {
      // depth_first, post-order AST traversal
      unparse(*(v->children.begin()),outfile,indent,adj_bbs,infile_name);
      // local single assignment code
      string local_sac;
      ostringstream local_sacs;
      local_sacs << sac_var_prefix << (v)->sac_var_idx() 
                << "=sin(" 
                << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() 
                << ");" << endl;
      local_sac=local_sacs.str(); 
      f_asg.push_back(local_sac);
      // local adjoint
      string local_adj;
      ostringstream local_adjs;
        local_adjs << adj_var_prefix << sac_var_prefix << (*(v->children.begin()))->sac_var_idx()
                   << "=cos("
                   << sac_var_prefix << (*(v->children.begin()))->sac_var_idx()
                   << ")*"
                   << adj_var_prefix << sac_var_prefix << (v)->sac_var_idx()
                   << ";" << endl;
      local_adj=local_adjs.str();
      r_asg.push_front(local_adj);
      break;
    }
    case COS_EXPRESSION_ASTV: {
      // depth_first, post-order AST traversal
      unparse(*(v->children.begin()),outfile,indent,adj_bbs,infile_name);
      // local single assignment code
      string local_sac;
      ostringstream local_sacs;
      local_sacs << sac_var_prefix << (v)->sac_var_idx() << "=cos(" << sac_var_prefix
                << (*(v->children.begin()))->sac_var_idx() << ");" << endl;
      local_sac=local_sacs.str(); 
      f_asg.push_back(local_sac);
      // local adjoint
      string local_adj;
      ostringstream local_adjs;
        local_adjs << adj_var_prefix << sac_var_prefix << (*(v->children.begin()))->sac_var_idx()
                   << "=0-sin("
                   << sac_var_prefix << (*(v->children.begin()))->sac_var_idx()
                   << ")*"
                   << adj_var_prefix << sac_var_prefix << (v)->sac_var_idx()
                   << ";" << endl;
      local_adj=local_adjs.str();
      r_asg.push_front(local_adj);
      break;
    }
    case TAN_EXPRESSION_ASTV: {
      // depth_first, post-order AST traversal
      unparse(*(v->children.begin()),outfile,indent,adj_bbs,infile_name);
      // local single assignment code
      string local_sac;
      ostringstream local_sacs;
      local_sacs << sac_var_prefix << (v)->sac_var_idx() << "=tan(" << sac_var_prefix
                << (*(v->children.begin()))->sac_var_idx() << ");" << endl;
      local_sac=local_sacs.str();
      f_asg.push_back(local_sac);
      // local adjoint
      string local_adj;
      ostringstream local_adjs;
        local_adjs << adj_var_prefix << sac_var_prefix << (*(v->children.begin()))->sac_var_idx()
                   << "=1/(cos("
                   << sac_var_prefix << (*(v->children.begin()))->sac_var_idx()
                   << ")*"
                   << "cos("
                   << sac_var_prefix << (*(v->children.begin()))->sac_var_idx()
                   << "))*"
                   << adj_var_prefix << sac_var_prefix << (v)->sac_var_idx()
                   << ";" << endl;
      local_adj=local_adjs.str();
      r_asg.push_front(local_adj);
      break;
    }
    case EXP_EXPRESSION_ASTV: {
      // depth_first, post-order AST traversal
      unparse(*(v->children.begin()),outfile,indent,adj_bbs,infile_name);
      // local single assignment code
      string local_sac;
      ostringstream local_sacs;
      local_sacs << sac_var_prefix << (v)->sac_var_idx() << "=exp(" << sac_var_prefix
                << (*(v->children.begin()))->sac_var_idx() << ");" << endl;
      local_sac=local_sacs.str(); 
      f_asg.push_back(local_sac);
      // local adjoint
      string local_adj;
      ostringstream local_adjs;
        local_adjs << adj_var_prefix << sac_var_prefix << (*(v->children.begin()))->sac_var_idx()
                   << "="
                   << sac_var_prefix << (v)->sac_var_idx()
                   << "*"
                   << adj_var_prefix << sac_var_prefix << (v)->sac_var_idx()
                   << ";" << endl;
      local_adj=local_adjs.str();
      r_asg.push_front(local_adj);
      break;
    }
    case SQRT_EXPRESSION_ASTV: {
      // depth_first, post-order AST traversal
      unparse(*(v->children.begin()),outfile,indent,adj_bbs,infile_name);
      // local single assignment code
      string local_sac;
      ostringstream local_sacs;
      local_sacs << sac_var_prefix << (v)->sac_var_idx() << "=sqrt(" << sac_var_prefix
                << (*(v->children.begin()))->sac_var_idx() << ");" << endl;
      local_sac=local_sacs.str(); 
      f_asg.push_back(local_sac);
      // local adjoint
      string local_adj;
      ostringstream local_adjs;
        local_adjs << adj_var_prefix << sac_var_prefix << (*(v->children.begin()))->sac_var_idx()
                   << "=1/(2*sqrt("
                   << sac_var_prefix << (*(v->children.begin()))->sac_var_idx()
                   << "))*"
                   << adj_var_prefix << sac_var_prefix << (v)->sac_var_idx()
                   << ";" << endl;
      local_adj=local_adjs.str();
      r_asg.push_front(local_adj);
      break;
    }
    case ATAN_EXPRESSION_ASTV: {
      // depth_first, post-order AST traversal
      unparse(*(v->children.begin()),outfile,indent,adj_bbs,infile_name);
      // local single assignment code
      string local_sac;
      ostringstream local_sacs;
      local_sacs << sac_var_prefix << (v)->sac_var_idx() << "=atan(" << sac_var_prefix
                << (*(v->children.begin()))->sac_var_idx() << ");" << endl;
      local_sac=local_sacs.str(); 
      f_asg.push_back(local_sac);
      // local adjoint
      string local_adj;
      ostringstream local_adjs;
        local_adjs << adj_var_prefix << sac_var_prefix << (*(v->children.begin()))->sac_var_idx()
                   << "=1/(1+"
                   << sac_var_prefix << (*(v->children.begin()))->sac_var_idx()
                   << "*"
                   << sac_var_prefix << (*(v->children.begin()))->sac_var_idx()
                   << ")*"
                   << adj_var_prefix << sac_var_prefix << (v)->sac_var_idx()
                   << ";" << endl;
      local_adj=local_adjs.str();
      r_asg.push_front(local_adj);
      break;
    }
    case LOG_EXPRESSION_ASTV: {
      // depth_first, post-order AST traversal
      unparse(*(v->children.begin()),outfile,indent,adj_bbs,infile_name);
      // local single assignment code
      string local_sac;
      ostringstream local_sacs;
      local_sacs << sac_var_prefix << (v)->sac_var_idx() << "=log(" << sac_var_prefix
                << (*(v->children.begin()))->sac_var_idx() << ");" << endl;
      local_sac=local_sacs.str();
      f_asg.push_back(local_sac);
      // local adjoint
      string local_adj;
      ostringstream local_adjs;
        local_adjs << adj_var_prefix << sac_var_prefix << (*(v->children.begin()))->sac_var_idx()
                   << "="
                   << adj_var_prefix << sac_var_prefix << (v)->sac_var_idx()
                   << "/"
                   << sac_var_prefix << (*(v->children.begin()))->sac_var_idx()
                   << ";" << endl;
      local_adj=local_adjs.str();
      r_asg.push_front(local_adj);
      break;
    }
    case POW_I_EXPRESSION_ASTV: {
      // depth_first, post-order AST traversal
      unparse(*(v->children.begin()),outfile,indent,adj_bbs,infile_name);
      // local single assignment code
      string local_sac;
      ostringstream local_sacs;
      local_sacs << sac_var_prefix << (v)->sac_var_idx() << "=pow(" << sac_var_prefix
                << (*(v->children.begin()))->sac_var_idx() << "," 
                << static_cast<symbol_ast_vertex*>(*(++(v->children.begin())))->sym->name << ");" << endl;
      local_sac=local_sacs.str(); 
      f_asg.push_back(local_sac);
      // local adjoint
      string local_adj;
      ostringstream local_adjs;
        local_adjs << adj_var_prefix << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // aux
                   << "="
                   << static_cast<symbol_ast_vertex*>(*(++(v->children.begin())))->sym->name // c
                   << "-1;" << endl; 
        local_adjs << adj_var_prefix << sac_var_prefix << (*(v->children.begin()))->sac_var_idx()
                   << "="
                   << static_cast<symbol_ast_vertex*>(*(++(v->children.begin())))->sym->name // c
                   << "*pow("
                   << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // x
                   << ","
                   << adj_var_prefix << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // aux
                   << ")*"
                   << adj_var_prefix << sac_var_prefix << (v)->sac_var_idx()
                   << ";" << endl;
      local_adj=local_adjs.str();
      r_asg.push_front(local_adj);
      break;
    }
    // y=pow(x,z)
    case POW_F_EXPRESSION_ASTV: {
      // depth_first, post-order AST traversal
      unparse(*(v->children.begin()),outfile,indent,adj_bbs,infile_name);
      // local single assignment code
      string local_sac;
      ostringstream local_sacs;
      local_sacs << sac_var_prefix << (v)->sac_var_idx() << "=pow(" << sac_var_prefix
                << (*(v->children.begin()))->sac_var_idx() << "," 
                << static_cast<symbol_ast_vertex*>(*(++(v->children.begin())))->sym->name << ");" << endl;
      local_sac=local_sacs.str(); 
      f_asg.push_back(local_sac);
      // local adjoint
      string local_adj;
      ostringstream local_adjs;
        local_adjs << adj_var_prefix << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // b_x as aux
                   << "="
                   << static_cast<symbol_ast_vertex*>(*(++(v->children.begin())))->sym->name // z
                   << "-1;" << endl; 
        for (int ii=0;ii<indent;ii++) local_adjs << " ";
        local_adjs << adj_var_prefix << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // b_x
                   << "="
                   << static_cast<symbol_ast_vertex*>(*(++(v->children.begin())))->sym->name // z
                   << "*pow("
                   << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // x
                   << ","
                   << adj_var_prefix << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // b_x as aux
                   << ")*"
                   << adj_var_prefix << sac_var_prefix << (v)->sac_var_idx() // b_y
                   << ";" << endl;
        for (int ii=0;ii<indent;ii++) local_adjs << " ";
        local_adjs << adj_var_prefix << static_cast<symbol_ast_vertex*>(*(++(v->children.begin())))->sym->name // b_z
                   << "="
                   << adj_var_prefix << static_cast<symbol_ast_vertex*>(*(++(v->children.begin())))->sym->name // b_z
                   << "+"
                   << sac_var_prefix << (v)->sac_var_idx() 
                   << "*log("
                   << sac_var_prefix << (*(v->children.begin()))->sac_var_idx() // x
                   << ")*"
                   << adj_var_prefix << sac_var_prefix << (v)->sac_var_idx() // b_y
                   << ";" << endl;
      local_adj=local_adjs.str();
      r_asg.push_front(local_adj);
      break;
    }
    case PAR_EXPRESSION_ASTV: {
      // depth_first, post-order AST traversal
      unparse(*(v->children.begin()),outfile,indent,adj_bbs,infile_name);
      // local single assignment code
      string local_sac;
      ostringstream local_sacs;
      local_sacs << sac_var_prefix << (v)->sac_var_idx() << "=" << sac_var_prefix
                << (*(v->children.begin()))->sac_var_idx() << ";" << endl;
      local_sac=local_sacs.str(); 
      f_asg.push_back(local_sac);
      // local adjoint
      string local_adj;
      ostringstream local_adjs;
        local_adjs << adj_var_prefix << sac_var_prefix << (*(v->children.begin()))->sac_var_idx()
                   << "="
                   << adj_var_prefix << sac_var_prefix << (v)->sac_var_idx()
                   << ";" << endl;
      local_adj=local_adjs.str();
      r_asg.push_front(local_adj);
      break;
    }
    case SCALAR_MEMREF_ASTV: {
      // local single assignment code
      string local_sac;
      ostringstream local_sacs;
      local_sacs << sac_var_prefix << (v)->sac_var_idx() 
                << "=" 
                << static_cast<scalar_memref_ast_vertex*>(v)->base->name 
                << ";" << endl;
      local_sac=local_sacs.str(); 
      f_asg.push_back(local_sac);
      // local adjoint
      string local_adj;
      ostringstream local_adjs;
      if (static_cast<scalar_memref_ast_vertex*>(v)->base->type==FLOAT_ST) {
        local_adjs << adj_var_prefix << static_cast<scalar_memref_ast_vertex*>(v)->base->name
                   << "="
                   << adj_var_prefix << static_cast<scalar_memref_ast_vertex*>(v)->base->name
                   << "+"
                   << adj_var_prefix << sac_var_prefix << (v)->sac_var_idx()
                   << ";" << endl;
      }
      else {
        // local_adjs << adj_var_prefix << static_cast<scalar_memref_ast_vertex*>(v)->base->name << "=0;" << endl;
      }
      local_adj=local_adjs.str();
      r_asg.push_front(local_adj);
      break;
    }
    case ARRAY_MEMREF_ASTV: {
      // local single assignment code
      string local_sac;
      ostringstream local_sacs;
      local_sacs << sac_var_prefix << (v)->sac_var_idx()
                << "="
                << static_cast<array_memref_ast_vertex*>(v)->base->name;
      for (int i=0;i<static_cast<array_memref_ast_vertex*>(v)->dim;i++)
        local_sacs << "["
                  << static_cast<array_memref_ast_vertex*>(v)->offsets[i]->name
                  << "]";
      local_sacs << ";" << endl;
      local_sac=local_sacs.str();
      f_asg.push_back(local_sac);
      // local adjoint
      string local_adj;
      ostringstream local_adjs;
      if (static_cast<array_memref_ast_vertex*>(v)->base->type==FLOAT_ST) {
        local_adjs << adj_var_prefix << static_cast<array_memref_ast_vertex*>(v)->base->name;
        for (int i=0;i<static_cast<array_memref_ast_vertex*>(v)->dim;i++)
          local_adjs << "["
                     << static_cast<array_memref_ast_vertex*>(v)->offsets[i]->name
                     << "]";
        local_adjs << "="
                   << adj_var_prefix << static_cast<array_memref_ast_vertex*>(v)->base->name;
        for (int i=0;i<static_cast<array_memref_ast_vertex*>(v)->dim;i++)
          local_adjs << "["
                   << static_cast<array_memref_ast_vertex*>(v)->offsets[i]->name
                   << "]";
        local_adjs << "+"
                   << adj_var_prefix << sac_var_prefix << (v)->sac_var_idx()
                   << ";" << endl;
        local_adj=local_adjs.str();
      }
      r_asg.push_front(local_adj);
      break;
    }
    case CONSTANT_ASTV: {
      // local single assignment code
      string local_sac;
      ostringstream local_sacs;
      local_sacs << sac_var_prefix << (v)->sac_var_idx() << "=" 
                << static_cast<const_ast_vertex*>(v)->value->name 
                << ";" << endl;
      local_sac=local_sacs.str(); 
      f_asg.push_back(local_sac);
      break;
    }
    default: {
      cerr << "Unknown ast vertex type " << v->type << endl;
      exit(-1);
      break;
    }
  }
}

