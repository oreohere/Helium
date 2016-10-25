#include "helium.h"
#include <string>
#include <iostream>

#include "helium_utils.h"

#include "helium_options.h"
#include "parser/xml_doc_reader.h"
#include "parser/xmlnode.h"
#include "parser/ast_node.h"
#include "parser/cfg.h"

#include "utils/utils.h"
#include "utils/dump.h"

#include "resolver/snippet.h"
#include "resolver/resolver.h"
#include "resolver/snippet_db.h"
#include "parser/resource.h"
#include "utils/log.h"
#include "builder.h"

#include <gtest/gtest.h>

#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include "helper.h"
#include "tester.h"
#include "analyzer.h"

namespace fs = boost::filesystem;


using namespace utils;




Helium::Helium(PointOfInterest *poi) {
  std::cout << "Starting Helium on point of interest: " << poi->GetPath() << ":" << poi->GetLinum() << "\n";
  assert(poi);
  m_poi = poi;

  XMLDoc *doc = XMLDocReader::Instance()->ReadFile(poi->GetPath());
  int linum = get_true_linum(poi->GetPath(), poi->GetLinum());
  std::cout << "Converted linum after preprocessing: " << linum << "\n";

  XMLNode xmlnode;
  if (poi->GetType() == "stmt") {
    xmlnode = find_node_on_line(doc->document_element(),
                                     {NK_Stmt, NK_ExprStmt, NK_DeclStmt,
                                         NK_Return, NK_Break, NK_Continue},
                                     linum);
  } else if (poi->GetType() == "loop") {
    xmlnode = find_node_on_line(doc->document_element(),
                                     {NK_Do, NK_While, NK_For}, linum);
  } else {
    std::cerr << "Only Support Stmt and Loop as POI." << "\n";
  }
  if (!xmlnode) {
    std::cerr << "EE: Cannot find SrcML node based on POI." << "\n";
    exit(1);
  }

  XMLNode func = get_function_node(xmlnode);
  std::string func_name = function_get_name(func);
  int func_linum = get_node_line(func);
  std::set<int> ids = SnippetDB::Instance()->LookUp(func_name, {SK_Function});
  if (ids.empty()) {
    std::cerr << "EE: Cannot find the function " << func_name << " enclosing POI." << "\n";
    exit(1);
  }

  for (int id : ids) {
    std::cout << id << "\n";

    SnippetMeta meta = SnippetDB::Instance()->GetMeta(id);
    std::string filename = fs::path(meta.filename).filename().string();
    if (filename == poi->GetFilename()) {
      // construct
      AST *ast = Resource::Instance()->GetAST(id);
      if (ast) {
        ASTNode *root = ast->GetRoot();
        if (root) {
          int ast_linum = root->GetBeginLinum();
          int target_linum = linum - func_linum + ast_linum;
          ASTNode *target = ast->GetNodeByLinum(target_linum);
          if (target) {
            CFG *cfg = Resource::Instance()->GetCFG(target->GetAST());
            CFGNode *target_cfgnode = cfg->ASTNodeToCFGNode(target);
            // (HEBI: Set Failure Point)
            target->SetFailurePoint();
            Segment::SetPOI(target_cfgnode);
            Segment *init_query = new Segment(target);
            m_worklist.push_back(init_query);
          }
        }
      }
    }
  }
  // initial query:
  if (m_worklist.empty()) {
    std::cerr << "Cannot construct the initial query." << "\n";
    exit(1);
  }
  std::cout << "Initial query: " << m_worklist.size() << "\n";
  process();
}


/**
 * Initialize the query as the segment, into worklist
 * Traverse the worklist:
 * - Build the query
 * - Oracle judge for BS
 * - If bs, merge(BS, bs)
 * - else
 *   - Selector(q) into worklist
 * TODO workflow
 */
void Helium::process() {
  // helium_print_trace("process");
  std::cout << "Helium Processing ..." << "\n";

  while (!m_worklist.empty()) {
    // std::cout << "size of worklist: " << m_worklist.size()  << "\n";
    Segment *segment = m_worklist.front();
    m_worklist.pop_front();
    if (!segment->IsValid()) {
      std::cout << "Segment Invalid due to removing of callsite." << "\n";
      continue;
    }
    std::string label = segment->Head()->GetLabel();
    if (label.size() > 10) {
      label = label.substr(0,10);
      label += "...";
    }
    std::cout << CYAN << "Processing query with the head node: "
              << label << "."
              << m_worklist.size() << " remaining in worklist.""\n"
              << RESET;

    // reach the function definition, continue search, do not test, because will dont need, and will compile error
    if (segment->Head()->GetASTNode()->Kind() == ANK_Function) {
      std::vector<Segment*> queries = select(segment);
      m_worklist.insert(m_worklist.end(), queries.begin(), queries.end());
      continue;
    }


    segment->PatchGrammar();
    
    segment->ResolveInput();
    segment->GenCode();
    
    Builder builder;
    builder.SetMain(segment->GetMain());
    builder.SetSupport(segment->GetSupport());
    builder.SetMakefile(segment->GetMakefile());
    builder.Write();
    builder.Compile();
    if (HeliumOptions::Instance()->GetBool("print-code-output-location")) {
      std::cout << "\t" << "Code Written to: " << builder.GetDir()  << "\n";
    }
    if (HeliumOptions::Instance()->GetBool("print-compile-info")) {
      std::cout << "\t" << "Compile: " << (builder.Success() ? "true" : "false") << "\n";
    }
    if (!builder.Success()) {
      std::cerr << utils::RED << "compile error"<< utils::RESET << "\n";
      if (HeliumOptions::Instance()->GetBool("pause-compile-error")) {
        std::cout << "Paused, press enter to continue ..." << std::flush;
        getchar();
      }
      segment->Remove(segment->New());
      m_worklist.push_back(segment);
      continue;
    }
    std::cerr << utils::GREEN << "compile success" << utils::RESET << "\n";
    std::string executable = builder.GetExecutable();
    if (HeliumOptions::Instance()->GetBool("run-test")) {
      Tester tester(builder.GetDir(), builder.GetExecutableName(), segment->GetInputs());
      tester.Test();
      if (fs::exists(builder.GetDir() + "/result.txt")) {
        Analyzer analyzer(builder.GetDir());
        analyzer.GetCSV();
        analyzer.AnalyzeCSV();
        analyzer.ResolveQuery(m_poi->GetFailureCondition());

        // (HEBI: remove branch if not covered)
        if (!analyzer.IsCovered()) {
          std::cout << utils::RED << "POI is not covered" << utils::RESET << "\n";
          // DEBUG remove branch or not
          if (HeliumOptions::Instance()->GetBool("remove_branch_if_not_covered")) {
            segment->RemoveNewBranch();
            m_worklist.push_back(segment);
          } else {
            std::vector<Segment*> queries = select(segment);
            m_worklist.insert(m_worklist.end(), queries.begin(), queries.end());
          }
        } else {
          std::cout << utils::GREEN << "POI is covered" << utils::RESET << "\n";
          std::vector<Segment*> queries = select(segment);
          m_worklist.insert(m_worklist.end(), queries.begin(), queries.end());
        }

        
        if (analyzer.IsBugTriggered()) {
          std::cout << utils::GREEN << "Bug is triggered." << utils::RESET << "\n";
        } else {
          std::cout << utils::RED << "Bug is not triggered." << utils::RESET << "\n";
        }
      }
    } else {
      // didn't run test ...
      // but still do context search, for build rate test
      std::vector<Segment*> queries = select(segment);
      m_worklist.insert(m_worklist.end(), queries.begin(), queries.end());
    }
  }
}



/**
 * context search
 */
std::vector<Segment*> Helium::select(Segment *query) {
  helium_print_trace("select");
  std::vector<Segment*> ret;
  // branch
  // TODO merge paths
  CFGNode *cfgnode = query->Head();
  ASTNode *astnode = cfgnode->GetASTNode();
  if (astnode->Kind() == ANK_If) {
    std::set<Segment*> queries = find_mergable_query(cfgnode, query);
    if (!queries.empty()) {
      for (Segment *q : queries) {
        // modify in place
        q->Merge(query);
      }
      return ret;
    } else {
      // add it self to the waiting list
      m_waiting_quries[cfgnode].insert(query);
    }
  }
  // Now, the query should not be mergeable to reach here
  // predecessor
  CFG *cfg = Resource::Instance()->GetCFG(astnode->GetAST());
  if (!cfg) return ret;
  // FIXME if the predecessor is interprocedure, and the compile failed,
  // should not continue this line because the callsite has to be removed.
  std::set<CFGNode*> preds = cfg->GetPredecessors(cfgnode);
  if (!preds.empty()) {
    for (CFGNode *pred : preds) {
      // FIXME this will not work on loops
      if (query->ContainNode(pred)) continue;
      // continue here, easier..
      // UPDATE I don't really need to check this here, the compilation will tell this is bad.
      // if (Segment::IsBad(pred)) continue;
      Segment *q = new Segment(*query);
      q->Add(pred);
      ret.push_back(q);
    }
  } else {
    // inter procedure
    preds = cfg->GetInterPredecessors(cfgnode);
    for (CFGNode *pred : preds) {
      if (query->ContainNode(pred)) continue;
      Segment *q = new Segment(*query);
      // add and MARK as inter procedure
      q->Add(pred, true);
      ret.push_back(q);
    }
  }

  // update g_propagating_queries
  m_propagating_queries[query].insert(ret.begin(), ret.end());
  return ret;
}


std::set<Segment*> Helium::find_mergable_query(CFGNode *node, Segment *orig_query) {
  helium_print_trace("find_mergable_query");
  std::set<Segment*> ret;
  if (m_waiting_quries.count(node) == 0) {
    return ret;
  }
  std::set<Segment*> queries = m_waiting_quries[node];
  // follow propagation path for the most recent position
  std::set<Segment*> worklist;
  worklist.insert(queries.begin(), queries.end());
  std::set<Segment*> candidates;
  while (!worklist.empty()) {
    Segment *q = *worklist.begin();
    worklist.erase(q);
    if (m_propagating_queries.count(q) == 1) {
      worklist.insert(m_propagating_queries[q].begin(), m_propagating_queries[q].end());
    } else {
      candidates.insert(q);
    }
  }
  // for all the candidates that have same transfer function as orig_query
  for (Segment *q : candidates) {
    // use random here
    // if (utils::rand_bool()) {
    //   ret.insert(q);
    // }
    ret.insert(q);
  }
  return ret;
}
