#include "reader.h"


#include "builder.h"
#include "tester.h"
#include "analyzer.h"
#include "context.h"
#include "resource.h"



#include "helium_options.h"


#include "utils/log.h"
#include "utils/utils.h"
#include "utils/dump.h"



#include "parser/slice_reader.h"
#include "parser/xml_doc_reader.h"



#include <stdio.h>
#include <cstring>
#include <signal.h>
#include <setjmp.h>
#include <iostream>

/*******************************
 ** portable timing
 *******************************/
#include <time.h>

#include <gtest/gtest.h>

#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>


// #include "seg.h"
namespace fs = boost::filesystem;


using namespace utils;

int Reader::m_skip_segment = -1;
int Reader::m_cur_seg_no = 0;

/**
 * Process the segment
 */
void ProcessSeg(Segment *seg) {
  helium_print_trace("ProcessSeg()");
  int count = 0;
  int limit = HeliumOptions::Instance()->GetInt("context-search-limit");
  // int skip_to_seg = Config::Instance()->GetInt("skip-to-seg");
  while(seg->ContinueNextContext()) {
    // if (skip_to_seg < count) continue;
    seg->TestNextContext();
    count++;
    if (limit >= 0 && count > limit) return;
  }
}


/**
 * Constructor of Reader should read the filename, and select segments.
 */
Reader::Reader(const std::string &filename) : m_filename(filename) {
  helium_print_trace("Reader: " + filename);
  // utils::file2xml(filename, m_doc);
  m_doc = XMLDocReader::Instance()->ReadFile(filename);
  std::string method = HeliumOptions::Instance()->GetString("code-selection");
  if (method == "annot-loop") {
    Segment *seg = getAnnotLoop();
    if (seg) {
      ProcessSeg(seg);
      delete seg;
    }
  } else if (method == "loop") {
    // getLoopSegments();
    assert(false);
  } else if (method == "annotation") {
    // getAnnotationSegments();
    Segment *seg = getAnnotSeg();
    if (seg) {
      ProcessSeg(seg);
      delete seg;
    }
  } else if (method == "divide") {
    // getDivideSegments();
    assert(false);
  } else if (method == "function") {
    // use GA. Do not use segment.
    // GA();
    // DO NOT call READ()
    return;
  } else {
    // assert(false && "segment selection method is not recognized: " && method.c_str());
    std::cerr<<"segment selection method is not recognized: " <<method<<"\n";
    assert(false);
  }
  // calling the read
  // this is for the old segment system, not for the new one.
  // Read();
  // assert(false);
}
/**
 * For now, only the first statment marked by @HeliumStmt
 */
Segment* Reader::getAnnotSeg() {
  helium_print_trace("Reader::getAnnotSeg");
  XMLNodeList comment_nodes = find_nodes_containing_str(*m_doc, NK_Comment, "@HeliumStmt");
  if (comment_nodes.size() != 1) {
    // std::cerr << "Error: Currently only support ONE single statement.";
    // std::cerr << "But Found: " << comment_nodes.size() << "\n";
    return NULL;
  }
  XMLNode node = helium_next_sibling(comment_nodes[0]);
  assert(node);
  // FIXME seg should be free-d outside
  Segment *seg = new Segment(node);
  return seg;
}

/**
 * Get annotationed loop.
 * The loop is marked by comment // @HeliumLoop right before the loop start
 * This is the pre-condition, otherwise undefined behavior will happen. FIXME
 */
Segment* Reader::getAnnotLoop() {
  helium_print_trace("Reader::getAnnotLoop");
  XMLNodeList comment_nodes = find_nodes_containing_str(*m_doc, NK_Comment, "@HeliumLoop");
  if (comment_nodes.size() != 1) {
    return NULL;
  }
  XMLNode node = helium_next_sibling(comment_nodes[0]);
  assert(node);
  Segment *seg = new Segment(node, SegKind_Loop);
  return seg;
}




















#if 0

/**
 * 
  , {NK_Stmt, ANK_Stmt}
  , {NK_ExprStmt, ANK_Stmt}
  , {NK_DeclStmt, ANK_Stmt}
  , {NK_Return, ANK_Stmt}
  , {NK_Break, ANK_Stmt}
  , {NK_Continue, ANK_Stmt}
  , {NK_Return, ANK_Stmt}
 */
Reader::Reader(std::string filename, POISpec poi) {
  helium_print_trace(std::string("Reader::Reader(std::string filename, POISpec poi)") + " " + filename);
  // poi contains linum
  // read the file, use line marker to get the true linum
  m_doc = XMLDocReader::Instance()->ReadFile(filename);
  int linum = get_true_linum(filename, poi.linum);
  std::cout << "true line number: " <<  linum  << "\n";
  if (poi.type == "stmt") {
    // get the single statement
    XMLNode node = find_node_on_line(m_doc->document_element(),
                                          {NK_Stmt, NK_ExprStmt, NK_DeclStmt, NK_Return, NK_Break, NK_Continue, NK_Return},
                                          linum);
    assert(node);
    XMLNode func = get_function_node(node);
    std::string func_name = function_get_name(func);
    AST *ast = Resource::Instance()->GetAST(func_name);



    /**
     * This node is important. This is the failure point.
     */
    Segment *init = new Segment(node);
    
    if (seg) {
      ProcessSeg(seg);
      delete seg;
    }
  } else if (poi.type == "loop") {
    // get the whole loop
    /**
     *   , {NK_While, ANK_While}
     , {NK_For, ANK_For}
     , {NK_Do, ANK_Do}

     */
    XMLNode node = find_node_on_line(m_doc->document_element(),
                                          {NK_While, NK_For, NK_Do},
                                          linum
                                          );
    assert(node);
    Segment *seg = new Segment(node, SegKind_Loop);
    if (seg) {
      ProcessSeg(seg);
      delete seg;
    }
  }
}


#endif
