#ifndef VISITOR_H
#define VISITOR_H

// #include "helium/parser/ast_v2.h"

#include <vector>
#include <map>
#include <set>
#include <sstream>

namespace v2 {
  class ASTContext;
  
  class ASTNodeBase;
  class TokenNode;
  class TranslationUnitDecl;
  class FunctionDecl;
  class Decl;
  class Stmt;
  class DeclStmt;
  class ExprStmt;
  class CompoundStmt;
  class ForStmt;
  class WhileStmt;
  class DoStmt;
  class BreakStmt;
  class ContinueStmt;
  class ReturnStmt;
  class IfStmt;
  class SwitchStmt;
  class SwitchCase;
  class CaseStmt;
  class DefaultStmt;
  class Expr;
}


/**
 * \defgroup visitor
 * \ingroup parser
 * Visitors for AST
 */

/**
 * \ingroup visitor
 *
 * visitor interface
 */
class Visitor {
public:
  Visitor() {}
  virtual ~Visitor() {}
  // high level
  virtual void visit(v2::TokenNode *node);
  virtual void visit(v2::TranslationUnitDecl *node);
  virtual void visit(v2::FunctionDecl *node);
  virtual void visit(v2::CompoundStmt *node);
  // condition
  virtual void visit(v2::IfStmt *node);
  virtual void visit(v2::SwitchStmt *node);
  virtual void visit(v2::CaseStmt *node);
  virtual void visit(v2::DefaultStmt *node);
  // loop
  virtual void visit(v2::ForStmt *node);
  virtual void visit(v2::WhileStmt *node);
  virtual void visit(v2::DoStmt *node);
  // single
  virtual void visit(v2::BreakStmt *node);
  virtual void visit(v2::ContinueStmt *node);
  virtual void visit(v2::ReturnStmt *node);
  // expr stmt
  virtual void visit(v2::Expr *node);
  virtual void visit(v2::DeclStmt *node);
  virtual void visit(v2::ExprStmt *node);
};

/**
 * \ingroup visitor
 *  use this to replace the dump method
 */
class Printer : public  Visitor {
public:
  Printer() {}
  ~Printer() {}


  // high level
  virtual void visit(v2::TokenNode *node);
  virtual void visit(v2::TranslationUnitDecl *node);
  virtual void visit(v2::FunctionDecl *node);
  virtual void visit(v2::CompoundStmt *node);
  // condition
  virtual void visit(v2::IfStmt *node);
  virtual void visit(v2::SwitchStmt *node);
  virtual void visit(v2::CaseStmt *node);
  virtual void visit(v2::DefaultStmt *node);
  // loop
  virtual void visit(v2::ForStmt *node);
  virtual void visit(v2::WhileStmt *node);
  virtual void visit(v2::DoStmt *node);
  // single
  virtual void visit(v2::BreakStmt *node);
  virtual void visit(v2::ContinueStmt *node);
  virtual void visit(v2::ReturnStmt *node);
  // expr stmt
  virtual void visit(v2::Expr *node);
  virtual void visit(v2::DeclStmt *node);
  virtual void visit(v2::ExprStmt *node);

  std::string prettyPrint(std::string aststr);

  std::string getString() {
    return prettyPrint(oss.str());
  }
private:
  void pre(v2::ASTNodeBase *node);
  void post();
  std::ostringstream oss;
};


class LevelVisitor : public Visitor {
public:
  // high level
  virtual void visit(v2::TokenNode *node);
  virtual void visit(v2::TranslationUnitDecl *node);
  virtual void visit(v2::FunctionDecl *node);
  virtual void visit(v2::CompoundStmt *node);
  // condition
  virtual void visit(v2::IfStmt *node);
  virtual void visit(v2::SwitchStmt *node);
  virtual void visit(v2::CaseStmt *node);
  virtual void visit(v2::DefaultStmt *node);
  // loop
  virtual void visit(v2::ForStmt *node);
  virtual void visit(v2::WhileStmt *node);
  virtual void visit(v2::DoStmt *node);
  // single
  virtual void visit(v2::BreakStmt *node);
  virtual void visit(v2::ContinueStmt *node);
  virtual void visit(v2::ReturnStmt *node);
  // expr stmt
  virtual void visit(v2::Expr *node);
  virtual void visit(v2::DeclStmt *node);
  virtual void visit(v2::ExprStmt *node);

  std::map<v2::ASTNodeBase*, int> getLevels() {return Levels;}
  int getLevel(v2::ASTNodeBase *node);
  v2::ASTNodeBase *getLowestLevelNode(std::set<v2::ASTNodeBase*> nodes);
private:
  void pre(v2::ASTNodeBase* node) {Levels[node]=lvl; lvl++;}
  void post() {lvl--;}
  std::map<v2::ASTNodeBase*, int> Levels;
  int lvl=0;
};

namespace v2 {
  class SymbolTable {
  public:
    SymbolTable() {}
    ~SymbolTable() {}

    void pushScope() {
      Stack.push_back({});
    }
    void popScope() {
      Stack.pop_back();
    }

    /**
     * id is declared in node
     */
    void add(std::string id, v2::ASTNodeBase *node) {
      Stack.back()[id] = node;
    }
    void add(std::set<std::string> ids, v2::ASTNodeBase *node) {
      for (const std::string&id : ids) {
        add(id, node);
      }
    }
    v2::ASTNodeBase* get(std::string id) {
      for (int i=Stack.size()-1;i>=0;i--) {
        if (Stack[i].count(id) == 1) return Stack[i][id];
      }
      return nullptr;
    }
  private:
    std::vector<std::map<std::string, v2::ASTNodeBase*> > Stack;
  };
};

class SymbolTableBuilder : public Visitor {
public:
  SymbolTableBuilder() {}
  virtual ~SymbolTableBuilder() {}
  // high level
  virtual void visit(v2::TokenNode *node);
  virtual void visit(v2::TranslationUnitDecl *node);
  virtual void visit(v2::FunctionDecl *node);
  virtual void visit(v2::CompoundStmt *node);
  // condition
  virtual void visit(v2::IfStmt *node);
  virtual void visit(v2::SwitchStmt *node);
  virtual void visit(v2::CaseStmt *node);
  virtual void visit(v2::DefaultStmt *node);
  // loop
  virtual void visit(v2::ForStmt *node);
  virtual void visit(v2::WhileStmt *node);
  virtual void visit(v2::DoStmt *node);
  // single
  virtual void visit(v2::BreakStmt *node);
  virtual void visit(v2::ContinueStmt *node);
  virtual void visit(v2::ReturnStmt *node);
  // expr stmt
  virtual void visit(v2::Expr *node);
  virtual void visit(v2::DeclStmt *node);
  virtual void visit(v2::ExprStmt *node);
  std::map<v2::ASTNodeBase*,std::set<v2::ASTNodeBase*> > getUse2DefMap() {return Use2DefMap;}
private:
  void insertDefUse(v2::ASTNodeBase *use);
  // this will be empty after traversal
  v2::SymbolTable Table;
  // this is the result
  std::map<v2::ASTNodeBase*,std::set<v2::ASTNodeBase*> > Use2DefMap;
};

/**
 * Token here means the leaf-node of AST, so it is abstract token.
 * The granularity depends on the parser precision.
 */
class TokenVisitor : public Visitor {
public:
  TokenVisitor() {}
  ~TokenVisitor() {}
  // high level
  virtual void visit(v2::TokenNode *node);
  virtual void visit(v2::TranslationUnitDecl *node);
  virtual void visit(v2::FunctionDecl *node);
  virtual void visit(v2::CompoundStmt *node);
  // condition
  virtual void visit(v2::IfStmt *node);
  virtual void visit(v2::SwitchStmt *node);
  virtual void visit(v2::CaseStmt *node);
  virtual void visit(v2::DefaultStmt *node);
  // loop
  virtual void visit(v2::ForStmt *node);
  virtual void visit(v2::WhileStmt *node);
  virtual void visit(v2::DoStmt *node);
  // single
  virtual void visit(v2::BreakStmt *node);
  virtual void visit(v2::ContinueStmt *node);
  virtual void visit(v2::ReturnStmt *node);
  // expr stmt
  virtual void visit(v2::Expr *node);
  virtual void visit(v2::DeclStmt *node);
  virtual void visit(v2::ExprStmt *node);

  std::vector<v2::ASTNodeBase*> getTokens() {return Tokens;}
  std::map<v2::ASTNodeBase*,int> getIdMap() {return IdMap;}
  int getId(v2::ASTNodeBase *node) {
    if (IdMap.count(node) == 1) return IdMap[node];
    return -1;
  }
private:
  int id = 0;
  std::map<v2::ASTNodeBase*,int> IdMap; // ID start from 0
  std::vector<v2::ASTNodeBase*> Tokens; // this is actually ID->Node implemented in vector
};

/**
 * Build a reverse index from children node to its parent
 */
class ParentIndexer : public Visitor {
public:
  ParentIndexer() {}
  ~ParentIndexer() {}

  // high level
  virtual void visit(v2::TokenNode *node);
  virtual void visit(v2::TranslationUnitDecl *node);
  virtual void visit(v2::FunctionDecl *node);
  virtual void visit(v2::CompoundStmt *node);
  // condition
  virtual void visit(v2::IfStmt *node);
  virtual void visit(v2::SwitchStmt *node);
  virtual void visit(v2::CaseStmt *node);
  virtual void visit(v2::DefaultStmt *node);
  // loop
  virtual void visit(v2::ForStmt *node);
  virtual void visit(v2::WhileStmt *node);
  virtual void visit(v2::DoStmt *node);
  // single
  virtual void visit(v2::BreakStmt *node);
  virtual void visit(v2::ContinueStmt *node);
  virtual void visit(v2::ReturnStmt *node);
  // expr stmt
  virtual void visit(v2::Expr *node);
  virtual void visit(v2::DeclStmt *node);
  virtual void visit(v2::ExprStmt *node);

  std::map<v2::ASTNodeBase*, v2::ASTNodeBase*> getParentMap() {return ParentMap;}
  std::set<v2::ASTNodeBase*> getChildren(v2::ASTNodeBase* parent) {
    if (ChildrenMap.count(parent) == 1) {
      return ChildrenMap[parent];
    }
    return std::set<v2::ASTNodeBase*>();
  }

  v2::ASTNodeBase *getParent(v2::ASTNodeBase *node) {
    if (ParentMap.count(node) == 1) return ParentMap[node];
    return nullptr;
  }
private:
  void pre(v2::ASTNodeBase* node);
  void post();
  std::map<v2::ASTNodeBase*, v2::ASTNodeBase*> ParentMap;
  std::map<v2::ASTNodeBase*, std::set<v2::ASTNodeBase*> > ChildrenMap;

  std::vector<v2::ASTNodeBase*> Stack;
};





/**
 * Compute the distribution of tokens, e.g. inside which if, which function
 *
 * This is also going to be bi-direction.
 *
 * map<Node, set> maps a node to all the tokens it contains. Notice that this node should be the representative nodes, like if, switch, loop.
 * set<Node> if_nodes;
 * set<Node> switch_nodes;
 * set<Node> for_nodes;
 * set<Node> do_nodes;
 * set<Node> while_nodes;
 * set<Node> func_nodes;
 */
class Distributor : public Visitor {
public:
  Distributor() {}
  ~Distributor() {}
  // high level
  virtual void visit(v2::TokenNode *node);
  virtual void visit(v2::TranslationUnitDecl *node);
  virtual void visit(v2::FunctionDecl *node);
  virtual void visit(v2::CompoundStmt *node);
  // condition
  virtual void visit(v2::IfStmt *node);
  virtual void visit(v2::SwitchStmt *node);
  virtual void visit(v2::CaseStmt *node);
  virtual void visit(v2::DefaultStmt *node);
  // loop
  virtual void visit(v2::ForStmt *node);
  virtual void visit(v2::WhileStmt *node);
  virtual void visit(v2::DoStmt *node);
  // single
  virtual void visit(v2::BreakStmt *node);
  virtual void visit(v2::ContinueStmt *node);
  virtual void visit(v2::ReturnStmt *node);
  // expr stmt
  virtual void visit(v2::Expr *node);
  virtual void visit(v2::DeclStmt *node);
  virtual void visit(v2::ExprStmt *node);

  void pre(v2::ASTNodeBase *node);
  void post();

  /**
   * get how many functions enclosing these nodes
   * Note these nodes may not belong to this AST
   */
  int getDistFunc(std::set<v2::ASTNodeBase*> sel) {
    return getDist(sel, func_nodes);
  }
  int getDistIf(std::set<v2::ASTNodeBase*> sel) {
    return getDist(sel, if_nodes);
  }
  int getDistLoop(std::set<v2::ASTNodeBase*> sel) {
    std::set<v2::ASTNodeBase*> loop_nodes;
    loop_nodes.insert(for_nodes.begin(), for_nodes.end());
    loop_nodes.insert(while_nodes.begin(), while_nodes.end());
    loop_nodes.insert(do_nodes.begin(), do_nodes.end());
    return getDist(sel, loop_nodes);
  }
  int getDistSwitch(std::set<v2::ASTNodeBase*> sel) {
    return getDist(sel, switch_nodes);
  }

  /**
   * Generic function for get distribution
   */
  int getDist(std::set<v2::ASTNodeBase*> sel, std::set<v2::ASTNodeBase*> nodes) {
    int ret=0;
    for (auto *node : nodes) {
      for (auto *s : sel) {
        if (ContainMap[node].count(s)) {
          ret++;
          break;
        }
      }
    }
    return ret;
  }
  
private:
  std::map<v2::ASTNodeBase*, std::set<v2::ASTNodeBase*> > ContainMap;
  std::set<v2::ASTNodeBase*> if_nodes;
  std::set<v2::ASTNodeBase*> switch_nodes;
  std::set<v2::ASTNodeBase*> for_nodes;
  std::set<v2::ASTNodeBase*> do_nodes;
  std::set<v2::ASTNodeBase*> while_nodes;
  std::set<v2::ASTNodeBase*> func_nodes;

  std::vector<v2::ASTNodeBase*> Stack;
};





/**
 * Code Generator
 */
class Generator : public Visitor {
public:
  Generator(std::set<v2::ASTNodeBase*> selection) : selection(selection) {}
  ~Generator() {}
  // high level
  virtual void visit(v2::TokenNode *node);
  virtual void visit(v2::TranslationUnitDecl *node);
  virtual void visit(v2::FunctionDecl *node);
  virtual void visit(v2::CompoundStmt *node);
  // condition
  virtual void visit(v2::IfStmt *node);
  virtual void visit(v2::SwitchStmt *node);
  virtual void visit(v2::CaseStmt *node);
  virtual void visit(v2::DefaultStmt *node);
  // loop
  virtual void visit(v2::ForStmt *node);
  virtual void visit(v2::WhileStmt *node);
  virtual void visit(v2::DoStmt *node);
  // single
  virtual void visit(v2::BreakStmt *node);
  virtual void visit(v2::ContinueStmt *node);
  virtual void visit(v2::ReturnStmt *node);
  // expr stmt
  virtual void visit(v2::Expr *node);
  virtual void visit(v2::DeclStmt *node);
  virtual void visit(v2::ExprStmt *node);


  void setSelection(std::set<v2::ASTNodeBase*> sel) {
    selection = sel;
  }

  std::string getProgram() {
    return Prog;
  }
private:
  std::string Prog;
  std::set<v2::ASTNodeBase*> selection;
};







#endif /* VISITOR_H */
