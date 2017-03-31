#ifndef SOURCE_MANAGER_H
#define SOURCE_MANAGER_H

#include <vector>
#include "helium/parser/ast_v2.h"
#include "helium/utils/common.h"

#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>

#include <gtest/gtest.h>

namespace fs = boost::filesystem;


/**
 * \ingroup parser
 */
class SourceManager {
public:
  /**
   * Precondition: p should be the cache/XXX/cpp folder
   */
  SourceManager(fs::path cppfolder);
  ~SourceManager() {}
  // void processProject(fs::path cppfolder);
  // v2::ASTNodeBase *getNodeById(int id) {
  //   return Nodes[id];
  // }
  // int getIdByNode(v2::ASTNodeBase *node) {
  //   if (IDs.count(node) != 0) {
  //     return IDs[node];
  //   } else {
  //     return -1;
  //   }
  // }
  // void dumpASTs();
  // dump tokens
  // void dumpTokens();
  // dump AST, and also dump its location
  void dumpASTs();
  // void select(std::map<std::string, std::set<std::pair<int,int> > > selection);
  // void select(std::set<v2::ASTNodeBase*> selection) {
  //   this->selection = selection;
  // }
  std::set<v2::ASTNodeBase*> generateRandomSelection();

  /**
   * Perform grammar patch based on this->selection.
   * Thus you need to call select first.
   */
  std::set<v2::ASTNodeBase*> grammarPatch(std::set<v2::ASTNodeBase*> sel);
  /**
   * Def use analysis
   * If some variable is used, include the node containing its declaration.
   */
  std::set<v2::ASTNodeBase*> defUse(std::set<v2::ASTNodeBase*> sel);

  /**
   * Generate program based on selection of nodes.
   * These nodes might be in different AST
   */
  std::string generateProgram(std::set<v2::ASTNodeBase*>);

  /**
   * DEPRECATED
   * Get the UUID of a node.
   * This will be: filename_ID
   * If the node is an internal node of AST, the ID will be -1
   */
  std::string getTokenUUID(v2::ASTNodeBase* node);
  fs::path getTokenFile(v2::ASTNodeBase* node);
  int getTokenId(v2::ASTNodeBase* node);
  /**
   * load selection from file.
   * The format is:
   *
   * #file
   * line column
   * line column
   * TODO change such utility function to static to avoid mis-use
   */
  std::set<v2::ASTNodeBase*> loadSelection(fs::path sel_file);
  /**
   * Dump selection into a file, and can be later load.
   * The dump information is more than the hand written one. It will contain the ID of the token.
   * The format will be:
   *
   * #file
   * line column ID
   * line column
   */
  void dumpSelection(std::set<v2::ASTNodeBase*> selection, std::ostream &os);
  void analyzeDistribution(std::set<v2::ASTNodeBase*> selection,
                           std::set<v2::ASTNodeBase*> patch,
                           std::ostream &os);
private:
  /**
   * Match a file in files and return the best match. Empty if no match.
   */
  fs::path matchFile(fs::path file);
  int getDistFile(std::set<v2::ASTNodeBase*> sel);
  int getDistProc(std::set<v2::ASTNodeBase*> sel);
  int getDistIf(std::set<v2::ASTNodeBase*> sel);
  int getDistLoop(std::set<v2::ASTNodeBase*> sel);
  int getDistSwitch(std::set<v2::ASTNodeBase*> sel);



  // this class holds all ASTs
  // it also determines the ID of ast nodes
  // each AST node should have an unique ID
  // std::vector<v2::ASTContext*> ASTs;
  fs::path cppfolder;
  // std::vector<fs::path> files;

  std::map<fs::path, v2::ASTContext*> File2ASTMap;
  std::map<v2::ASTContext*, fs::path> AST2FileMap;

  // token visitor, always available because it is useful
  std::map<v2::ASTContext*, TokenVisitor*> AST2TokenVisitorMap;
  std::map<v2::ASTContext*, Distributor*> AST2DistributorMap;
  
  // std::vector<v2::ASTNodeBase*> Nodes;
  // std::map<v2::ASTNodeBase*,int> IDs;

  // std::set<v2::ASTNodeBase*> selection;
};


#endif /* SOURCE_MANAGER_H */