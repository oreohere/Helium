#ifndef CONTEXT_H
#define CONTEXT_H

#include "segment.h"
#include "builder.h"
#include "tester.h"

/**
 * The Context class.
 * This class must be associated with a Segment.
 * The nodes selection is the AST nodes, held by Segment.
 */
class Context {
public:
  ~Context();
  Context(Segment *seg);
  // copy constructor
  Context(const Context &rhs);
  Segment* GetSegment() {
    return m_seg;
  }
  ASTNode *GetFirstNode() {
    return m_first;
  }
  void SetFirstNode(ASTNode* node);
  void SetLast(Context *ctx) {
    m_last = ctx;
  }
  // not valid if:
  // 2. newly added node already selected
  // So, set when AddNode
  // @return whether it is valid
  bool AddNode(ASTNode* node);
  void RemoveNode(ASTNode *node);
  std::set<ASTNode*> GetNodes() {
    return m_nodes;
  }

  /**
   * Test dynamic properties, to decide the first node to retain or not.
   * It is pretty weird here, because the old design is to test in reader.
   * This will incur the builder and tester, and backend.
   * The result of test:
   * 1. whether the first node will be in the selection of this or not
   * 2. the first node will not be affected by the result, because it decides where to search from for next context
   */
  void Test();
  void dump();
  bool IsResolved() {
    return m_query_resolved;
  }

  std::map<std::string, Type*> GetInputVariables();


  /**
   * The resolving, code output related methods
   * 1. resolve input
   * 2. resolve snippet
   * For each of the AST!
   */
  void Resolve();
  /**
   * Procedure count, aka AST count
   */
  int GetProcedureNum() {
    return m_ast_to_node_m.size();
  }
  
private:
  /**
   * The input shoudl be associated with AST the first node belongs.
   * All other ASTs should not contain any input code?
   * Need to instrument the POI
   * No need to instrument input, because that's our generated inputs.
   */
  /**
   * Tasks:
   * 1. find the def, recursively, and include
   * 2. find the input variables, their type
   */
  std::set<ASTNode*> resolveDecl(AST *ast, bool to_root);
  void getUndefinedVariables(AST *ast);
  /**
   * No magic, the old one suffices.
   */
  void resolveSnippet(AST *ast);
  /**
   * Try to Resolve the query.
   * @return true, or false. CAUTION this return value should be set to m_query_resolved
   */
  bool resolveQuery(std::vector<std::string> invs, std::vector<std::string> pres, std::vector<std::string> trans);
  /**
   * Code getting
   */
  std::string getMain();
  std::string getSupport();
  std::string getSupportBody();
  std::string getMakefile();

  std::string getSigMain();


  void simplify();

  bool compile();
  void createTestCases();
  TestResult* test();
  void analyze(TestResult *result);

  void freeTestSuite();



  
  Segment *m_seg = NULL;
  Context *m_last = NULL; // last context

  Builder *m_builder = NULL;
  std::vector<std::vector<InputSpec*> > m_test_suite;
  std::string m_sig_dir;
  /**
   * Storage
   */
  std::set<ASTNode*> m_nodes; // the selection of this context
  std::map<AST*, std::set<ASTNode*> > m_ast_to_node_m;
  ASTNode *m_first;
  // decoration of declaration and input on AST
  typedef std::map<ASTNode*, std::set<std::string> > decl_deco;
  // local storage for the decoration of each AST
  std::map<AST*, std::pair<decl_deco, decl_deco> > m_ast_to_deco_m;

  // this is another system for input code generation
  // this does not decorate the AST
  // instead, get the variables out, and insert at the beginning
  // typedef std::vector<std::pair<Type*, std::string> > InputMetrics;

  /**
   * This is a map from variable name to its type
   */
  typedef std::map<std::string, Type*> InputMetrics;
  // the following two can overlap, i.e. need both declaration and input
  std::map<AST*, InputMetrics> m_decls; // only need declaraion
  std::map<AST*, InputMetrics> m_inputs; // only need input

  // global variables used
  // would be added into the input code for the first AST
  InputMetrics m_globals;
  
  std::set<int> m_snippet_ids; // only need one copy of snippet ids, for all the ASTs
  
  bool m_query_resolved = false;
  int m_search_time;
};



#endif /* CONTEXT_H */
