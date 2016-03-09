#include "reader.h"

#include <stdio.h>

#include <cstring>
#include <signal.h>
#include <setjmp.h>
#include <iostream>

#include "config.h"
#include "builder.h"
#include "tester.h"
#include "analyzer.h"
#include "type.h"
#include "options.h"

#include "utils.h"

#include <time.h>

#include <gtest/gtest.h>

using namespace utils;
using namespace ast;

int Reader::m_skip_segment = -1;
int Reader::m_cur_seg_no = 0;

/**
 * Constructor of Reader should read the filename, and select segments.
 */
Reader::Reader(const std::string &filename) : m_filename(filename) {
  utils::file2xml(filename, m_doc);
  std::string method = Config::Instance()->GetString("code-selection");
  if (method == "loop") {
    getLoopSegments();
  } else if (method == "annotation") {
    getAnnotationSegments();
  } else if (method == "divide") {
    getDivideSegments();
  } else {
    // assert(false && "segment selection method is not recognized: " && method.c_str());
    std::cerr<<"segment selection method is not recognized: " <<method<<"\n";
    assert(false);
  }
}

/**
 * Use line numbers for a segment selection.
 */
Reader::Reader(const std::string &filename, std::vector<int> line_numbers)
  : m_filename(filename)
{
  utils::file2xml(filename, m_doc);
  std::vector<NodeKind> kinds;
  kinds.push_back(NK_DeclStmt);
  kinds.push_back(NK_ExprStmt);
  NodeList nodes = ast::find_nodes_on_lines(m_doc, kinds, line_numbers);
  for (Node n : nodes) {
    Segment seg;
    seg.PushBack(n);
    m_segments.push_back(seg);
  }
}

double get_time() {
  struct timespec ts;
  ts.tv_sec=0;
  ts.tv_nsec=0;
  clock_gettime(CLOCK_REALTIME, &ts);
  double d = (double)ts.tv_sec + 1.0e-9*ts.tv_nsec;
  return d;
}


/*
 * For every segment:
 * - increase context
 * - process
 * - create builder
 * - create tester
 * - create analyzer
 */


void
Reader::Read() {
  for (Segment &seg : m_segments) {
    global_seg_no++;
    if (Config::Instance()->GetInt("skip-to-seg") > global_seg_no) {
      continue;
    }
    if (PrintOption::Instance()->Has(POK_SegNo)) {
      std::cout <<"[Helium] processing segment NO." << global_seg_no  << "\n";
    }
    if (PrintOption::Instance()->Has(POK_Segment)) {
      utils::print(seg.GetSegmentText(), utils::CK_Blue);
    }
    for(;seg.IsValid();) {

      /*******************************
       ** Processing segment
       *******************************/
      // std::cout <<"================"  << "\n";
      // std::cout <<"segment:"  << "\n";
      // std::cout <<seg.GetSegmentText()  << "\n";
      // std::cout <<"context:"  << "\n";
      // std::cout <<seg.GetContextText()  << "\n";
      // std::cout <<utils::CYAN<<seg.GetText()  << utils::RESET << "\n";
      seg.ResolveInput();
      // seg.ResolveOutput();
      seg.ResolveSnippets();

      /** outputing input variables */
      VariableList vars = seg.GetInputVariables();
      // VariableList out_vars = seg.GetOutputVariables();
      // std::cout <<"input vars: "<<vars.size()  << "\n";
      // for (Variable v : vars) {
      //   std::cout <<"\t" << v.Name() << ":" << v.GetType().ToString()  << "\n";
      // }

      /*******************************
       ** Compiling
       *******************************/
      Builder builder(&seg);
      builder.Write();
      if (PrintOption::Instance()->Has(POK_CodeOutputLocation)) {
        std::cout <<"code outputed to " << builder.GetDir()  << "\n";
      }
      builder.Compile();
      if (builder.Success()) {
        g_compile_success_no++;
        if (PrintOption::Instance()->Has(POK_CompileInfo)) {
          utils::print("compile success", utils::CK_Green);
        }
        /*******************************
         ** Testing
         *******************************/
        if (Config::Instance()->GetString("run-test") == "true") {
          Tester tester(builder.GetDir(), &seg);
          // double t1 = get_time();
          tester.Test();
          // double t2 = get_time();
          // printf("%f\n", t2-t1);
          tester.WriteCSV();
        }
      } else {
        g_compile_error_no++;
        if (PrintOption::Instance()->Has(POK_CompileInfo)) {
          utils::print("compile error", utils::CK_Red);
        }
        if (DebugOption::Instance()->Has(DOK_PauseCompileError)) {
          std::cout <<".. print enter to continue .."  << "\n";
          getchar();
        }
      }
      seg.IncreaseContext();
    }
    // std::cout <<seg.GetInvalidReason()  << "\n";
  }
}

// void
// Reader::Read() {
//   // signal(SIGALRM, watch_dog);
//   // ualarm(Config::Instance()->GetInt("segment_timeout")*1000, 0);
//   for (auto it=m_spus.begin();it!=m_spus.end();it++) {
//     //    if (setjmp(jmpbuf) != 0) perror("setjmp");
//     // setjmp(jmpbuf);
//     // if (watch_dog_skip) {
//     //   watch_dog_skip = false;
//     //   continue;
//     // }
//     m_cur_seg_no ++;
//     if (m_skip_segment > m_cur_seg_no) {
//       continue;
//     }
//     // process the segment unit.
//     // do input resolve, output resovle, context search, support resolve
//     (*it).Process();
//     do {
//       // library call experiment
//       VariableList inv = (*it).GetInputVariables();
//       VariableList outv = (*it).GetOutputVariables();
      
//       Builder builder(*it);
//       builder.Build();
//       if (!(*it).CanContinue()) {
//         break;
//       }
//       builder.Compile();
//       // if (builder.Success() && Config::Instance()->WillRunTest()) {
//       //   std::shared_ptr<Tester> tester = std::make_shared<Tester>(builder.GetExecutable(), *it);
//       //   tester->Test();
//       //   if (tester->Success() && Config::Instance()->WillRunAnalyze()) {
//       //     std::shared_ptr<Analyzer> analyzer = std::make_shared<Analyzer>(tester->GetOutput());
//       //     analyzer->Analyze();
//       //   }
//       // }
//     } while ((*it).IncreaseContext());
//   }
// }

void Reader::getLoopSegments() {
  std::vector<NodeKind> kinds = {NK_Do, NK_For, NK_While};
  NodeList loops = find_nodes(m_doc, kinds);
  for (Node loop : loops) {
    Segment seg;
    seg.PushBack(loop);
    m_segments.push_back(seg);
  }
}
/**
 * Get segemnt based on annotation in source code.
 * 1. @HeliumStmt
 * 2. @HeliumSegmentBegin -- @HeliumSegmentEnd
 */
void Reader::getAnnotationSegments() {
  // format 1: @HeliumStmt
  NodeList comment_nodes = find_nodes_containing_str(m_doc, NK_Comment, "@HeliumStmt");
  for (Node node : comment_nodes) {
    Segment seg;
    seg.PushBack(helium_next_sibling(node));
    m_segments.push_back(seg);
  }
  // format 2: @HeliumSegmentBegin/End
  comment_nodes = find_nodes_containing_str(m_doc, NK_Comment, "@HeliumSegmentBegin");
  for (Node node : comment_nodes) {
    // do not use helium_xxx version because I need the comment node.
    Node n = next_sibling(node);
    Segment seg;
    for (;;n = next_sibling(n)) {
      if (kind(n) == NK_Comment) {
        std::string text = get_text(n);
        if (text.find("@HeliumSegmentEnd") != std::string::npos) {
          m_segments.push_back(seg);
          break;
        }
      }
      seg.PushBack(n);
    }
  }
}

/**
 * Get segments of the callsite of func_name.
 * Get helium recognized AST of parent of <call> node.
 * Example: strcpy
 */
void Reader::getFuncCallSegments(std::string func_name) {
  NodeList call_nodes = find_nodes(m_doc, NK_Call);
  for (Node n : call_nodes) {
    if (function_get_name(n) == func_name) {
      Segment seg;
      seg.PushBack(helium_parent(n));
      m_segments.push_back(seg);
    }
  }
}

/*
 * for every function, divide code by comments, start of loop, start of branch condition.
 * combine these blocks
 
 treat comment as ast block
for every block, if it is simple statement, combine into a NodeList.
If it is a block of interest(Loop, Condition), treat it singlely as a NodeList.
 */
void
Reader::getDivideSegments() {
  NodeList functions = find_nodes(m_doc, NK_Function);
  for (Node function : functions) {
    Node block = function_get_block(function);
    getDivideRecursive(block_get_nodes(block));
  }
}

static bool is_leaf_node(Node node) {
  switch(ast::kind(node)) {
  case NK_ExprStmt:
  case NK_Return:
  case NK_Continue:
  case NK_Break:
  case NK_Define:
  case NK_IfnDef:
  case NK_IfDef:
  case NK_DefElse:
  case NK_EndIf:
  case NK_DeclStmt: return true;
  case NK_If:
  case NK_Comment:
  case NK_Do:
  case NK_While:
  case NK_Switch:
  case NK_For: return false;
  default:
    std::cerr<<ast::kind_to_name(ast::kind(node)) << " is not recoganized or handled.\n";
    assert(false);
    return true;
  }
}

/**
 * Get body node list of block.
 * This block is not <block>, but AST level.
 * Such as <if>
 * If can have two blocks, <then><block> and <else><block>.
 * The algorithm is to enumerate the type, and get the one we want.
 */
std::vector<NodeList> get_block_bodies(Node node) {
  std::vector<NodeList> result;
  switch(ast::kind(node)) {
  case NK_If: {
    Node then_block = ast::if_get_then_block(node);
    // TODO should move this into the uppr function, if all blocks here have a <block> tag.
    result.push_back(block_get_nodes(then_block));
    Node else_block = ast::if_get_else_block(node);
    result.push_back(block_get_nodes(else_block));
    break;
  }
  case NK_For: {
    Node block = ast::for_get_block(node);
    result.push_back(block_get_nodes(block));
    break;
  }
  case NK_Switch: {
    NodeList cases = ast::switch_get_cases(node);
    for (Node case_node : cases) {
      result.push_back(ast::case_get_nodes(case_node));
    }
    break;
  }
  case NK_Do: {
    Node block = ast::do_get_block(node);
    result.push_back(block_get_nodes(block));
    break;
  }
  case NK_While: {
    Node block = ast::while_get_block(node);
    result.push_back(block_get_nodes(block));
    break;
  }
  default: ;
  }
  return result;
}


void Reader::getDivideRecursive(NodeList nodes) {
  std::vector<NodeList> seg_cands;
  NodeList block_nodes;
  NodeList tmp_group;

  // step 1
  for (Node node : nodes) {
    if (is_leaf_node(node)) {
      tmp_group.push_back(node);
    } else {
      if (!tmp_group.empty()) seg_cands.push_back(tmp_group);
      tmp_group.clear();
      if (ast::kind(node) != NK_Comment) {
        NodeList l = {node}; // need this in document algorithm?
        seg_cands.push_back(l);
        block_nodes.push_back(node);
      }
    }
  }
  if (!tmp_group.empty()) {
    seg_cands.push_back(tmp_group);
    tmp_group.clear();
  }

  // constructing the final segments from segment candidates
  for (size_t i=0;i<seg_cands.size();i++) {
    for (size_t j=i;j<seg_cands.size();j++) {
      Segment seg;
      for (size_t k=i;k<=j;k++) {
        seg.PushBack(seg_cands[k]);
      }
      m_segments.push_back(seg);
    }
  }

  // process block
  for (Node block : block_nodes) {
    std::vector<NodeList> bodies = get_block_bodies(block);
    for (NodeList body : bodies) {
      getDivideRecursive(body);
    }
  }
  
}




void Reader::PrintSegments() {
  for (Segment &seg : m_segments) {
    std::cout <<"==========="  << "\n";
    std::cout << seg.GetText() << "\n";
  }
}
  


/*******************************
 ** Deprecated code
 *******************************/

/*
 * True if variables contains type "name", "number" types
 */
// bool
// has_variable(std::set<std::shared_ptr<Variable> > variables, std::string name, int number) {
//   std::shared_ptr<Type> type = TypeFactory(name).CreateType();
//   if (number <= 0) return true;
//   for (auto it=variables.begin();it!=variables.end();it++) {
//     std::shared_ptr<Type> vtype = (*it)->GetType();
//     if (type->GetName() == vtype->GetName()) {
//         // && type->GetDimension() == vtype->GetDimension()
//         // && type->GetPointerLevel() == vtype->GetPointerLevel()) {
//       number--;
//       if (number <=0) return true;
//     }
//   }
//   return false;
// }

// bool
// has_variables(std::set<std::shared_ptr<Variable> > variables, std::string);



// static bool watch_dog_skip = false;

// static jmp_buf jmpbuf; // jumbuf for long jump
// static bool skip_file = false;

// void watch_dog(int sig) {
//   global_error_number++;
//   if (global_error_number>100) exit(1);
//   // file_error_number++;
//   // if (file_error_number>30) {
//   //   skip_file = true;
//   // }
//   watch_dog_skip = true;
//   ualarm(Config::Instance()->GetSegmentTimeout()*1000, 0);
//   longjmp(jmpbuf, 1); // jump back to previous stack
// }

// std::string
// get_match_library_name(std::set<std::shared_ptr<Variable> > inv) {
//   std::vector< std::set<std::string> > libraries;
//   libraries.push_back({"char", "int"});
//   for (auto it=libraries.begin();it!=libraries.end();it++) {
//     for (auto jt=it->begin();jt!=it->end();jt++) {
//       // if (!has_variable(inv, *jt, 1))
//     }
//   }
//   if (has_variable(inv, "char*", 1)
//       && has_variable(inv, "int", 1)) {
//     std::cout<<"FOUND segment that has the same input variable as library call"<<std::endl;
//     // std::cout<< "\033[1;30m " << (*it)->GetSegment()->GetText()<< " \033[0m" <<std::endl;
//     // OK, the input variable matches
//   } 
// }

