#include "context.h"
#include "options.h"
#include <iostream>
#include "utils.h"
#include "snippet_db.h"
#include "xml_doc_reader.h"
#include "slice_reader.h"
#include "ast_node.h"
#include "global_variable.h"

#include "builder.h"
#include "analyzer.h"
#include "config.h"
#include "tester.h"

#include <gtest/gtest.h>

using namespace ast;
using namespace utils;

/********************************
 * Context
 *******************************/


/**
 * Create the very first context, with the segment itself.
 * The nodes for the context should be empty.
 * But the ast to node map should contain the POI already.
 * UPDATE: OK, I give up, keep the POI also in the context.
 */
Context::Context(Segment *seg) : m_seg(seg) {
  print_trace("Context::Context(Segment *seg)");
  // this is the beginning.
  // use the POI as the first node.
  SetFirstNode(seg->GetFirstNode());
  m_nodes = seg->GetPOI();
  for (ASTNode* node : m_nodes) {
    m_ast_to_node_m[node->GetAST()].insert(node);
  }
}
// copy constructor
/**
 * Do I really need to manually copy these?
 * Maybe the default one already works good.
 */
Context::Context(const Context &rhs) {
  print_trace("Context::Context(const Context &rhs)");
  m_seg = rhs.m_seg;
  m_nodes = rhs.m_nodes;
  m_first = rhs.m_first;
  m_ast_to_node_m = rhs.m_ast_to_node_m;
}

/********************************
 * Modifying
 *******************************/

/**
 * The first node denote which the most recent AST is.
 */
void Context::SetFirstNode(ast::ASTNode* node) {
  m_first = node;
}

void Context::AddNode(ASTNode* node) {
  // insert here
  // but if test shows it should not be in, it will be removed
  m_nodes.insert(node);
  m_ast_to_node_m[node->GetAST()].insert(node);
}

void Context::RemoveNode(ASTNode *node) {
  // remove
  // FIXME assert exist
  m_nodes.erase(node);
  m_ast_to_node_m[node->GetAST()].erase(node);
}



/********************************
 * Debugging
 *******************************/

void Context::dump() {
  // separate nodes by their AST
  // std::map<AST*, std::set<ASTNode*> > asts;
  // for (ASTNode* node : m_nodes) {
  //   asts[node->GetAST()].insert(node);
  // }
  /**
   * Context does not maintain the nodes in POI.
   * Thus when dumping, need to take them into account.
   */
  // assert(m_seg);
  // for (ASTNode* node : m_seg->GetPOI()) {
  //   asts[node->GetAST()].insert(node);
  // }
  // for each AST, print out, or visualize
  // std::cout << "AST number: " << asts.size()  << "\n";
  static int count = 0;
  count ++;
  for (auto m : m_ast_to_node_m) {
    std::cout << "-----------"  << "\n";
    AST *ast = m.first;
    std::set<ASTNode*> nodes = m.second;
    std::string code = ast->GetCode(nodes);
    utils::print(code+"\n", utils::CK_Blue);
    // ast->VisualizeN(nodes, {}, std::to_string(count));
  }
}



/********************************
 * Resolving
 *******************************/

/**
 * TODO
 */
void Context::Resolve() {
  // gather ASTs UPDATE already a member field: m_ast_to_node_m
  // resolve for each AST
  AST *first_ast = m_first->GetAST();
  for (auto m : m_ast_to_node_m) {
    AST *ast = m.first;
    std::set<ASTNode*> nodes = m.second;
    std::set<ASTNode*> complete;
    // if (ast != first_ast) {
    //   // this is inner function.
    //   // this one should not have input variables.
    //   // all the inputs should be resolved such that the input is self-sufficient
    //   // Complete to root to include the function prototype
    //   // FIXME can I modify the map here?
    //   // TODO complete def use
    //   complete = resolveDecl(ast, false);
    // } else {
    //   // this is the first AST
    //   // this one needs input variables
    //   complete = resolveDecl(ast, true);
    // }

    
    // std::set<ASTNode*> selection = m_ast_to_node_m[ast];
    // if (ast == first_ast) {
    //   // if it is first ast, need to add declaration and 
    //   selection = ast->CompleteGene(selection);
    //   selection = ast->RemoveRoot(selection);
    // } else {
    //   // need to add declaration
    //   // need to complete to root
    //   selection = ast->CompleteGeneToRoot(selection);
    // }
    // m_ast_to_node_m[ast] = selection;

    // TODO disabled def use analysis
    complete = resolveDecl(ast, ast == first_ast);
    m_ast_to_node_m[ast] = complete;
    // if (ast == first_ast) {
    //   complete = ast->RemoveRoot(complete);
    //   m_ast_to_node_m[ast] = complete;
    //   getUndefinedVariables(ast);
    // }
    
    // std::cout << complete.size()  << "\n";
    // ast->VisualizeN(complete, {});
    // getchar();
    resolveSnippet(ast);
  }
}

/**
 * This is for the first AST only
 * It resolve the variables to see if it can be resolved.
 * Then see if the Decl already included in selection
 * Otherwise, return as input variables
 *
 * HEBI this must be called after completion
 * HEBI Need to disable the first ast decl resolving
 * @return m_decls
 */
void Context::getUndefinedVariables(AST *ast) {
  print_trace("Context::getUndefinedVariables(AST *ast)");
  std::set<ASTNode*> sel = m_ast_to_node_m[ast];
  for (ASTNode *node : sel) {
    std::set<std::string> ids = node->GetVarIds();
    for (std::string id : ids) {
      if (id.empty()) continue;
      if (is_c_keyword(id)) continue;
      SymbolTable *tbl = node->GetSymbolTable();
      assert(tbl);
      SymbolTableValue *decl = tbl->LookUp(id);
      if (!decl) continue;
      ASTNode *decl_node = decl->GetNode();
      if (decl_node) {
        if (sel.count(decl_node) == 1) continue;
        else {
          m_decls[ast][id] = decl->GetType();
        }
      } else {
        NewType* type = GlobalVariableRegistry::Instance()->LookUp(id);
        if (type) {
          m_decls[ast][id] = type;
        }
      }
    }
  }
  std::cout << "end"  << "\n";
}


std::set<ASTNode*> Context::resolveDecl(AST *ast, bool first_ast_p) {
  print_trace("Context::resolveDecl");
  std::set<ASTNode*> ret = m_ast_to_node_m[ast];
  // FIXME do I need to complete again after I recursively add the dependencies
  if (first_ast_p) {
    ret = ast->CompleteGene(ret);
    // return ret;
  } else {
    ret = ast->CompleteGeneToRoot(ret);
  }
  // ast->VisualizeN(ret, {});
  std::set<ASTNode*> worklist = ret; // m_ast_to_node_m[ast];
  std::set<ASTNode*> done;
  // from the decl node to the variable needed to be declared
  std::map<ASTNode*, std::set<std::string> > decl_input_m;
  // do not need input
  std::map<ASTNode*, std::set<std::string> > decl_m;
  // at the end, need to remove those node that already in m_gene
  while (!worklist.empty()) {
    ASTNode* node = *worklist.begin();
    worklist.erase(node);
    if (done.count(node) == 1) continue;
    done.insert(node);
    // find the var names
    // std::string code;
    // node->GetCode({}, code, true);
    // std::cout << code  << "\n";
    std::set<std::string> ids = node->GetVarIds();
    for (std::string id : ids) {
      if (id.empty()) continue;
      if (is_c_keyword(id)) continue;
      // std::cout <<id << " YYY at " << m_idx_m[node] << "\n";
      // std::cout << id  << "\n";
      SymbolTable *tbl = node->GetSymbolTable();
      // tbl->dump();
      SymbolTableValue *decl = tbl->LookUp(id);
      ASTNode *def = node->LookUpDefinition(id);
      // FIXME I didn't use my new function resolve_type, which takes care of global variable and special variables like optarg
      if (!decl) continue;
      if (def) {
        // utils::print("found def: " + id + "\n", utils::CK_Cyan);
        // need this def, but not necessary to be a new node
        // and also possibly need the decl
        ret.insert(def);
        worklist.insert(def);
        // ast->VisualizeN({def}, {});
        // getchar();
        std::set<ASTNode*> morenodes = ast->CompleteGene(ret);
        ret.insert(morenodes.begin(), morenodes.end());
        worklist.insert(morenodes.begin(), morenodes.end());
        decl_m[decl->GetNode()].insert(id);
      } else {
        // may need decl, and need input
        decl_input_m[decl->GetNode()].insert(id);
      }
    }
  }
  if (first_ast_p) {
    // FIXME Remove root for the outmost AST!
    ret = ast->RemoveRoot(ret);
  }
  // ret is the new complete, so just check the decorations not in ret
  for (auto it=decl_m.begin(), end=decl_m.end();it!=end;) {
    if (ret.count(it->first) == 1) {
      it = decl_m.erase(it);
    } else {
      ++it;
    }
  }
  for (auto it=decl_input_m.begin(), end=decl_input_m.end();it!=end;) {
    if (ret.count(it->first) == 1) {
      it = decl_input_m.erase(it);
    } else {
      ++it;
    }
  }
  // std::cout << decl_input_m.size()  << "\n";
  // std::cout << decl_m.size()  << "\n";
  // remove from the maps those that already in gene
  // for (ASTNode *n : done) {
  //   decl_input_m.erase(n);
  //   decl_m.erase(n);
  // }
  // std::cout << decl_input_m.size()  << "\n";
  // std::cout << decl_m.size()  << "\n";
  // store these two maps
  // m_decl_input_m = decl_input_m;
  // m_decl_m = decl_m;
  m_ast_to_deco_m[ast] = std::make_pair(decl_m, decl_input_m);
  for (auto m : decl_m) {
    for (std::string id : m.second) {
      SymbolTableValue *decl = m.first->GetSymbolTable()->LookUp(id);
      NewType *t = decl->GetType();
      assert(t);
      m_decls[ast][id] = t;
    }
  }
  for (auto m : decl_input_m) {
    for (std::string id : m.second) {
      SymbolTableValue *decl = m.first->GetSymbolTable()->LookUp(id);
      NewType *t = decl->GetType();
      assert(t);
      m_decls[ast][id] = t;
      m_inputs[ast][id] = t;
    }
  }
  // remove duplication of m_decls and m_inputs
  // the duplication is introduced, by the different DEFINITION in the code.
  // So, m_inputs should prominate it.
  return ret;
}

/**
 * TODO
 */
void Context::resolveSnippet(AST *ast) {
  std::set<std::string> all_ids;
  std::map<ASTNode*, std::set<std::string> > all_decls;
  // Since I changed the decl mechanism to m_decls, I need to change here
  
  // decl_deco decl_m = m_ast_to_deco_m[ast].first;
  // decl_deco decl_input_m = m_ast_to_deco_m[ast].second;
  // all_decls.insert(decl_input_m.begin(), decl_input_m.end());
  // all_decls.insert(decl_m.begin(), decl_m.end());
  // for (auto item : all_decls) {
  //   ASTNode *node = item.first;
  //   std::set<std::string> names = item.second;
  //   for (std::string name : names) {
  //     SymbolTableValue *value = node->GetSymbolTable()->LookUp(name);
  //     std::string type = value->GetType()->Raw();
  //     std::set<std::string> ids = extract_id_to_resolve(type);
  //     all_ids.insert(ids.begin(), ids.end());
  //   }
  // }

  /**
   * I want to resolve the type of the decls
   * And also the char array[MAX_LENGTH], see that macro?
   */
  for (auto m : m_decls[ast]) {
    std::string var = m.first;
    NewType *t = m.second;
    std::string raw = t->Raw();
    // the type raw itself (does not contain dimension suffix)
    std::set<std::string> ids = extract_id_to_resolve(raw);
    all_ids.insert(ids.begin(), ids.end());
    // dimension suffix
    std::string dim = t->DimensionSuffix();
    ids = extract_id_to_resolve(dim);
    all_ids.insert(ids.begin(), ids.end());
  }
  
  // resolve the nodes selected by gene
  std::set<ASTNode*> nodes = m_ast_to_node_m[ast];
  for (ASTNode *n : nodes) {
    std::set<std::string> ids = n->GetIdToResolve();
    all_ids.insert(ids.begin(), ids.end());
  }

  // I need to remove the function here!
  // otherwise the code snippet will get too much
  // For example, I have included a function in main.c, but it turns out to be here
  // even if I filter it out when adding to support.h, I still have all its dependencies in support.h!
  for (auto m : m_ast_to_node_m) {
    AST *ast = m.first;
    std::string func = ast->GetFunctionName();
    all_ids.erase(func);
  }

  // FIXME This output more than I want
  // e.g. Char GetOutputCode HELIUM_POI MAXPATHLEN Od_sizeof Od_strlen d fflush fileptr n printf stdout strcpy strlen tempname
  // some is in the string, e.g. HELIUM_POI
  // some is output variable, e.g. Od_sizeof
  // some is C reserved keywords, e.g. printf
  // std::cout << "ids to resolve snippets:"  << "\n";
  // for (auto id : all_ids) {
  //   std::cout << id  << " ";
  // }
  // std::cout << ""  << "\n";
  
  std::set<int> snippet_ids = SnippetDB::Instance()->LookUp(all_ids);
  // not sure if it should be here ..
  std::set<int> all_snippet_ids = SnippetDB::Instance()->GetAllDep(snippet_ids);
  all_snippet_ids = SnippetDB::Instance()->RemoveDup(all_snippet_ids);
  m_snippet_ids.insert(all_snippet_ids.begin(), all_snippet_ids.end());
}

/********************************
 * Getting code
 *******************************/

std::string Context::getMain() {
  print_trace("Context::getMain()");
  std::string ret;
  ret += get_header();
  std::string main_func;
  std::string other_func;
  /**
   * Go through all the ASTs
   * The first AST should be placed in main function.
   * Other ASTs should just output.
   */
  AST *first_ast = m_first->GetAST();
  for (auto m : m_ast_to_node_m) {
    AST *ast = m.first;
    std::set<ASTNode*> nodes = m.second;
    if (ast == first_ast) {
      main_func += "int main() {\n";
      main_func += "int helium_size;\n"; // array size of heap
      // TODO need to call that function
      // ast->SetDecoDecl(m_ast_to_deco_m[ast].first);
      // ast->SetDecoDeclInput(m_ast_to_deco_m[ast].second);


      for (auto m :m_decls[ast]) {
        std::string var = m.first;
        NewType *t = m.second;
        main_func += t->GetDeclCode(var);
        // FIXME didn't not use def use analysis result!
        main_func += t->GetInputCode(var);
      }
      if (PrintOption::Instance()->Has(POK_IOSpec)) {
        utils::print("Input Metrics:\n", utils::CK_Blue);
        for (auto m :m_decls[ast]) {
          std::string var = m.first;
          NewType *t = m.second;
          utils::print(t->ToString() + "\n", utils::CK_Blue);
          utils::print(t->GetInputCode(var) + "\n", utils::CK_Purple);
          utils::print("-------\n", utils::CK_Blue);
          TestInput *input = t->GetTestInputSpec(var);
          // utils::print(t->GetTestInput() + "\n", utils::CK_Purple);
          utils::print(input->GetRaw() + "\n", utils::CK_Purple);
          delete input;
        }
      }
      // for (auto m : m_inputs[ast]) {
      //   std::string var = m.first;
      //   NewType *t = m.second;
      //   main_func += t->GetInputCode(var);
      //   std::cout << t->ToString()  << "\n";
      // }

      // print out the deco, to see if the same variable appear in both "first" and "second"
      // decl_deco deco = m_ast_to_deco_m[ast].first;
      // std::cout << "first"  << "\n";
      // for (auto m : deco) {
      //   for (std::string s : m.second) {
      //     std::cout << s  << "\n";
      //   }
      // }
      // deco = m_ast_to_deco_m[ast].second;
      // std::cout << "second"  << "\n";
      // for (auto m : deco) {
      //   for (std::string s : m.second) {
      //     std::cout << s  << "\n";
      //   }
      // }

      // FIXME TODO this is inside the main function. The main function is declared as int main(int argc, char *argv[])
      // No, because we will generate argc and argv, the main function actually is int main()
      // so the return statement should be a integer.
      // some code may just "return",
      // some may return some strange value
      // If I want to know the return value, I first needs to know its value.
      // So, lets modify all the return statements to be return 35, indicating an error
      // the return value in this can be restricted to a perdefined constant, to indicate such situation.
      std::string code = ast->GetCode(nodes);
      ast->ClearDecl();
      // modify the code, specifically change all return statement to return 35;
      code = replace_return_to_35(code);
      main_func += code;
      main_func += "return 0;";
      main_func += "};\n";
    } else {
      // other functions
      // ast->SetDecl(m_ast_to_deco_m[ast].second, m_ast_to_deco_m[ast].first);
      decl_deco merge;
      decl_deco first = m_ast_to_deco_m[ast].first;
      decl_deco second = m_ast_to_deco_m[ast].second;
      merge.insert(first.begin(), first.end());
      merge.insert(second.begin(), second.end());
      // ast->SetDecoDecl(m_ast_to_deco_m[ast].first);
      // ast->SetDecoDeclInput(m_ast_to_deco_m[ast].second);
      // For other function, only set the necessary decl.
      // Do not get input
      ast->SetDecoDecl(merge);
      std::string code = ast->GetCode(nodes);
      ast->ClearDecl();
      other_func += code;
      other_func += "\n";
    }
  }
  // FIXME delaration of other functions should be included in suport.
  ret += other_func;
  ret += main_func;
  return ret;
}
std::string Context::getSupport() {
  std::vector<int> sorted_snippet_ids = SnippetDB::Instance()->SortSnippets(m_snippet_ids);
  std::string code = "";
  // head
  code += get_head();
  code += SystemResolver::Instance()->GetHeaders();
  code += "\n/****** codes *****/\n";
  // snippets
  std::string code_func_decl;
  std::string code_variable;
  std::string code_func;
  /**
   * Avoid all the functions
   */
  std::set<std::string> avoid_funcs;
  for (auto m : m_ast_to_node_m) {
    avoid_funcs.insert(m.first->GetFunctionName());
  }
  // but, but, we should not avoid the function for the "first node", because it is in the main function
  avoid_funcs.erase(m_first->GetAST()->GetFunctionName());

  // DEBUG printing out avoid functions

  // for (auto &s : avoid_funcs) {
  //   std::cout << s  << "\n";
  // }

  // decl the avoid func here
  for (std::string s : avoid_funcs) {
    std::set<int> ids = SnippetDB::Instance()->LookUp(s, {SK_Function});
    assert(ids.size() == 1);
    int id = *ids.begin();
    std::string code = SnippetDB::Instance()->GetCode(id);
    std::string decl_code = ast::get_function_decl(code);
    code_func_decl +=
      "// Decl Code for " + s + "\n"
      + decl_code + "\n";
    // std::cout << "decl code for " << s << " is: " << decl_code  << "\n";
    // std::cout << "original code is " << code  << "\n";
  }

  
  // std::cout << sorted_snippet_ids.size()  << "\n";
  for (int id : sorted_snippet_ids) {
    SnippetMeta meta = SnippetDB::Instance()->GetMeta(id);
    if (meta.HasKind(SK_Function)) {
      std::string func = meta.GetKey();
      if (avoid_funcs.count(func) == 0) {
        code_func += "/* ID: " + std::to_string(id) + " " + meta.filename + ":" + std::to_string(meta.linum) + "*/\n";
        code_func += SnippetDB::Instance()->GetCode(id) + '\n';
        code_func_decl += get_function_decl(SnippetDB::Instance()->GetCode(id))+"\n";
      } else {
        // assert(false);
        // add only function decls
        // FIXME It seems to be empty string
        // std::cout << func << " avoid detected"  << "\n";
        // std::string decl = get_function_decl(SnippetDB::Instance()->GetCode(id) + "\n");
        // code_func_decl += decl;
      }
    } else if (meta.HasKind(SK_Variable)) {
      // for variable, put it AFTER function decl
      // this is because the variable may be a function pointer decl, and it may use the a funciton.
      // But of course it should be before the function definition itself, because it is
      code_variable += "/* ID: " + std::to_string(id) + " " + meta.filename + ":" + std::to_string(meta.linum) + "*/\n";
      code_variable += SnippetDB::Instance()->GetCode(id) + '\n';
    } else {
      // every other support code(structures) first
      code += "/* ID: " + std::to_string(id) + " " + meta.filename + ":" + std::to_string(meta.linum) + "*/\n";
      code += SnippetDB::Instance()->GetCode(id) + '\n';
    }
  }

  code += "\n// function declarations\n";
  code += code_func_decl;
  code += "\n// variables and function pointers\n";
  code += code_variable;
  code += "\n// functions\n";
  code += code_func;
  // foot
  code += get_foot();
  return code;
}
std::string Context::getMakefile() {
  std::string makefile;
  makefile += ".PHONY: all clean test\n";
  makefile = makefile + "a.out: main.c\n"
    + "\tcc -g -std=c11 main.c " + SystemResolver::Instance()->GetLibs() + "\n"
    + "clean:\n"
    + "\trm -rf *.out\n"
    + "test:\n"
    + "\tbash test.sh";
    
    return makefile;
}

TEST(SegmentTestCase, ExecTest) {
  std::string cmd = "/tmp/helium-test-tmp.4B0xG7/a.out";
  std::string input = utils::read_file("/tmp/helium-test-tmp.4B0xG7/input/2.txt");
  int status = 0;
  std::string output = utils::exec_in(cmd.c_str(), input.c_str(), &status, 10);
  std::cout << status  << "\n";
  std::cout << output  << "\n";
}


void Context::Test() {
  std::cout << "============= Context::Test() ================="  << "\n";
  Builder builder;
  std::string code_main = getMain();
  std::string code_sup = getSupport();
  std::string code_make = getMakefile();
  builder.SetMain(code_main);
  builder.SetSupport(code_sup);
  builder.SetMakefile(code_make);
  builder.Write();
  if (PrintOption::Instance()->Has(POK_CodeOutputLocation)) {
    std::cout << "Code output to "  << builder.GetDir() << "\n";
  }
  if (PrintOption::Instance()->Has(POK_Main)) {
    std::cout << code_main  << "\n";
  }
  builder.Compile();
  if (builder.Success()) {
    g_compile_success_no++;
    if (PrintOption::Instance()->Has(POK_CompileInfo)) {
      utils::print("compile success\n", utils::CK_Green);
    }
    if (PrintOption::Instance()->Has(POK_CompileInfoDot)) {
      utils::print(".", utils::CK_Green);
      std::cout << std::flush;
    }

    // decl_deco decl_input_m = m_ast_to_deco_m[m_first->GetAST()].second;

    int test_number = Config::Instance()->GetInt("test-number");
    /**
     * Testing!
     */
    AST *first_ast = m_first->GetAST();
    InputMetrics metrics = m_inputs[first_ast];
    // std::vector<std::string> test_suites(TEST_NUMBER);
    // for (auto metric : metrics) {
    //   std::string var = metric.first;
    //   NewType *type = metric.second;
    //   std::vector<std::string> inputs = type->GetTestInput(TEST_NUMBER);
    //   assert(inputs.size() == TEST_NUMBER);
    //   for (int i=0;i<TEST_NUMBER;i++) {
    //     test_suites[i] += " " + inputs[i];
    //   }
    // }

    /**
     * Using TestInput
     */

    // when generating inputs, I need to monitor if the main file has the getopt staff
    // 	while( ( o = getopt( argc, argv, "achtvf:" ) ) != -1 ){
    // if yes, I need to get the spec
    // then when generating argc and argv, I need to be careful to cover each case
    // also, I need to mark the inputs as: argv_a, argv_c, argv_h
    // argv_a is binary
    // argv_f is a string!
    ArgCV argcv;
    if (code_main.find("getopt") != std::string::npos) {
      std::string opt = code_main.substr(code_main.find("getopt"));
      std::vector<std::string> lines = utils::split(opt, '\n');
      assert(lines.size() > 0);
      opt = lines[0];
      assert(opt.find("\"") != std::string::npos);
      opt = opt.substr(opt.find("\"")+1);
      assert(opt.find("\"") != std::string::npos);
      opt = opt.substr(0, opt.find("\""));
      assert(opt.find("\"") == std::string::npos);
      // print out the opt
      utils::print(opt, utils::CK_Cyan);
      // set the opt
      argcv.SetOpt(opt);
    }

    // I should also capture the argc and argv variable used, but I can currently assume these variables here
    // Also, for regular argc and argv, I need also care about them, e.g. sizeof(argv) = argc, to avoid crashes.
    
    // I'm going to pre-generate argc and argv.
    // so that if later the metrics have that, I don't need to implement the match, just query
    std::vector<std::pair<TestInput*, TestInput*> > argcv_inputs = argcv.GetTestInputSpec(test_number);
    // used for freeing these inputs
    bool argc_used = false;
    bool argv_used = false;
    
    std::vector<std::vector<TestInput*> > test_suite(test_number);
    for (auto metric : metrics) {
      std::string var = metric.first;
      NewType *type = metric.second;
      std::vector<TestInput*> inputs;
      if (var == "argc") {
        argc_used = true;
        for (auto p : argcv_inputs) {
          inputs.push_back(p.first);
        }
      } else if (var == "argv") {
        argv_used = true;
        for (auto p : argcv_inputs) {
          inputs.push_back(p.second);
        }
      } else {
        inputs = type->GetTestInputSpec(var, test_number);
      }
      assert((int)inputs.size() == test_number);
      for (int i=0;i<(int)inputs.size();i++) {
        test_suite[i].push_back(inputs[i]);
      }
    }

    // free when not used, to avoid memory leak
    if (!argc_used) {
      for (auto p : argcv_inputs) {
        delete p.first;
      }
    }
    if (!argv_used) {
      for (auto p : argcv_inputs) {
        delete p.second;
      }
    }

    // this is the other use place of test suite other than the execution of the executable itself
    // create the test result!
    // This will supply the input spec for the precondition and transfer function generation
    // The used method is ToString()
    NewTestResult test_result(test_suite);


    // std::string test_dir = utils::create_tmp_dir();
    utils::create_folder(builder.GetDir() + "/input");
    for (int i=0;i<test_number;i++) {
      // std::string test_file = test_dir + "/test" + std::to_string(i) + ".txt";
      // utils::write_file(test_file, test_suite[i]);
      // std::string cmd = builder.GetExecutable() + "< " + test_file + " 2>/dev/null";
      // std::cout << cmd  << "\n";
      std::string cmd = builder.GetExecutable();
      int status;
      // FIXME some command cannot be controled by time out!
      // std::string output = utils::exec(cmd.c_str(), &status, 1);
      std::string input;
      for (TestInput *in : test_suite[i]) {
        input += in->GetRaw() + " ";
      }

      if (PrintOption::Instance()->Has(POK_IOSpec)) {
        utils::print("TestinputMetrics:\n", CK_Blue);
        for (TestInput *in : test_suite[i]) {
          utils::print(in->dump() + "\n", CK_Purple);
        }
      }
      // std::string output = utils::exec_in(cmd.c_str(), test_suite[i].c_str(), &status, 10);
      // I'm also going to write the input file in the executable directory
      utils::write_file(builder.GetDir() + "/input/" + std::to_string(i) + ".txt", input);
      std::string output = utils::exec_in(cmd.c_str(), input.c_str(), &status, 10);
      if (status == 0) {
        if (PrintOption::Instance()->Has(POK_TestInfo)) {
          utils::print("test success\n", utils::CK_Green);
        }
        if (PrintOption::Instance()->Has(POK_TestInfoDot)) {
          utils::print(".", utils::CK_Green);
        }
        test_result.AddOutput(output, true);
      } else {
        if (PrintOption::Instance()->Has(POK_TestInfo)) {
          utils::print("test failure\n", utils::CK_Red);
        }
        if (PrintOption::Instance()->Has(POK_TestInfoDot)) {
          utils::print(".", utils::CK_Red);
        }
        test_result.AddOutput(output, false);
      }
      
      // std::cout << "output:"  << "\n";
      // std::cout << output  << "\n";
    }
    if (PrintOption::Instance()->Has(POK_TestInfoDot)) {
      std::cout << "\n";
    }

    test_result.PrepareData();
    // std::string i_csv = test_result.GenerateCSV("I", "S");
    // std::string o_csv = test_result.GenerateCSV("O", "S");
    // HEBI Generating CSV file
    std::string csv = test_result.GenerateCSV("IO", "SF");
    // std::cout << "icsv"  << "\n";
    // std::cout << i_csv  << "\n";
    // std::cout << "ocsv"  << "\n";
    // std::cout << o_csv  << "\n";
    // std::cout << "csv"  << "\n";
    // std::cout << csv  << "\n";
    /**
     * Save to file, and output file name.
     */
    // std::string tmp_dir = utils::create_tmp_dir();
    // utils::write_file(tmp_dir + "/i.csv", i_csv);
    // utils::write_file(tmp_dir + "/o.csv", o_csv);
    // std::string csv_file = tmp_dir + "/io.csv";
    std::string csv_file = builder.GetDir() + "/io.csv";
    utils::write_file(csv_file, csv);
    std::cout << "Output to: " << csv_file   << "\n";
    test_result.GetInvariants();
    test_result.GetPreconditions();
    test_result.GetTransferFunctions();

    Analyzer analyzer(csv_file);
    std::vector<std::string> invs = analyzer.GetInvariants();
    std::vector<std::string> pres = analyzer.GetPreConditions();
    std::vector<std::string> trans = analyzer.GetTransferFunctions();
    if (PrintOption::Instance()->Has(POK_AnalysisResult)) {
      std::cout << "------ invariants ------"  << "\n";
      for (auto &s : invs) {
        std::cout << "| " << s  << "\n";
      }
      std::cout << "------ pre condtions ------"  << "\n";
      for (auto &s : pres) {
        std::cout << "| " << s  << "\n";
      }
      std::cout << "------ transfer functions ------"  << "\n";
      for (auto &s : trans) {
        std::cout << "| " << s  << "\n";
      }
      std::cout << "------------------------------"  << "\n";

      // std::string cmd = "compare.py -f " + csv_file;
      // std::string inv = utils::exec(cmd.c_str());
      // std::cout << inv  << "\n";
    }


    std::cout << "---- resolveQuery -----"  << "\n";
    m_query_resolved = resolveQuery(invs, pres, trans);
    std::cout << "------ end of query resolving -----"  << "\n";
    
    /**
     * FIXME Free the space of TestInput*
     */
    for (std::vector<TestInput*> &v : test_suite) {
      for (TestInput* in : v) {
        delete in;
      }
    }

  } else {
    g_compile_error_no++;
    if (PrintOption::Instance()->Has(POK_CompileInfo)) {
      utils::print("compile error\n", utils::CK_Red);
    }
    if (PrintOption::Instance()->Has(POK_CompileInfoDot)) {
      utils::print(".", utils::CK_Red);
      std::cout << std::flush;
    }
    if (DebugOption::Instance()->Has(DOK_PauseCompileError)) {
      std::cout <<".. print enter to continue .."  << "\n";
      getchar();
    }

    // remove this first node
    RemoveNode(m_first);
  }
  // TODO return good or not, to decide whether to keep the newly added statements
}


void free_binary_formula(std::vector<BinaryFormula*> bfs) {
  for (BinaryFormula *bf : bfs) {
    delete bf;
  }
}


/**
 * I want to start from invs, try to use pres and trans to derive it.
 * Then I need to check the pres to see if those variables are entry point (argv, argc, optarg)
 */
bool Context::resolveQuery(std::vector<std::string> str_invs, std::vector<std::string> str_pres, std::vector<std::string> str_trans) {
  // Construct binary forumlas
  std::vector<BinaryFormula*> invs;
  std::vector<BinaryFormula*> pres;
  std::vector<BinaryFormula*> trans;
  // create BinaryFormula here. CAUTION need to free them
  for (std::string &s : str_invs) {
    invs.push_back(new BinaryFormula(s));
  }
  for (std::string &s : str_pres) {
    pres.push_back(new BinaryFormula(s));
  }
  for (std::string &s : str_trans) {
    trans.push_back(new BinaryFormula(s));
  }
  if (invs.empty() || trans.empty() || pres.empty()) return false;
  // how to identify the key invariant?
  // the constant invariants is used for derive
  // the relationship invariants are used as key
  // any relationship invariants is enough, for now
  BinaryFormula *key_inv = get_key_inv(invs);
  std::cout << "| selected the key invariants: " << key_inv->dump()  << "\n";
  
  // STEP Then, find the variables used in the key invariant
  // FIXME No, I need the whole header, e.g. strlen(fileptr). The sizeof(fileptr) is not useful at all.
  // find the related transfer functions
  // std::set<std::string> vars = key_inv->GetVars();
  std::vector<BinaryFormula*> related_trans; // = get_related_trans(trans, vars);
  for (BinaryFormula *bf : trans) {
    // std::string output_var = bf->GetLHSVar();
    // if (vars.count(output_var) == 1) {
    //   related_trans.push_back(bf);
    // }
    std::string item = bf->GetLHS();
    if (item == key_inv->GetLHS() || item == key_inv->GetRHS()) {
      related_trans.push_back(bf);
    }
  }

  // find the preconditions that define the input variables used in the transfer function
  std::set<std::string> related_input_items;
  for (BinaryFormula* bf : related_trans) {
    // FIXME y = x + z; I only want x
    related_input_items.insert(bf->GetRHS());
  }
  std::vector<BinaryFormula*> related_pres;
  for (BinaryFormula *bf : pres) {
    if (related_input_items.count(bf->GetLHS()) == 1 || related_input_items.count(bf->GetRHS()) == 1) {
      related_pres.push_back(bf);
    }
  }
  // use the preconditions and transfer funcitons to derive the invariants
  BinaryFormula* used_pre = derive_key_inv(related_pres, related_trans, key_inv);
  if (used_pre) {
    std::cout << "| Found the precondition that can derive to the invariant: " << used_pre->dump()  << "\n";
    // examine the variables in those preconditons, to see if they are entry points
    std::set<std::string> used_vars = used_pre->GetVars();
    // FIXME ugly
    free_binary_formula(pres);
    free_binary_formula(trans);
    free_binary_formula(invs);
    // for (BinaryFormula *bf : used_pres) {
    //   used_vars.insert(bf->GetLHSVar());
    //   used_vars.insert(bf->GetRHSVar());
    // }
    std::cout << "| The variables used: "  << "\n";
    for (std::string var : used_vars) {
      std::cout << "| " << var  << "\n";
      // this is argv:f!
      if (var.find(':') != std::string::npos) {
        var = var.substr(0, var.find(':'));
      }
      if (var != "argv" && var != "argc" && var != "optarg") {
        return false;
      }
      if (var == "argv" || var == "argc") {
        // the argv should come from main
        if (m_first->GetAST()->GetFunctionName() != "main") {
          return false;
        }
      }
    }
    utils::print("Context Searching Success!\n", utils::CK_Green);
    return true;
  } else {
    free_binary_formula(pres);
    free_binary_formula(trans);
    free_binary_formula(invs);
    return false;
  }
}