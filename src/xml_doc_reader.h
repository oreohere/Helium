#ifndef XML_DOC_READER_H
#define XML_DOC_READER_H

#include "ast.h"

/**
 * Read the xml document by SrcML.
 * It will make sure there's only one copy of the same XML document
 * This intends to
 *   1. remove the overhead of creating the same document again and again.
 *   2. ensure the same XML node reference is comparable for the same code file.
 * But this also puts some assumptions
 *   1. the XML document should not be modified (or should be changed back after modifying)
 */

class XMLDocReader {
public:
  static XMLDocReader *Instance() {
    if (!m_instance) {
      m_instance = new XMLDocReader();
    }
    return m_instance;
  }
  ~XMLDocReader() {
    for (ast::XMLDoc* doc : m_string_docs) {
      delete doc;
    }
    for (auto m : m_docs) {
      delete m.second;
    }
  }
  ast::XMLDoc* ReadFile(std::string filename);
  ast::XMLDoc* ReadString(const std::string &code);
  static std::vector<std::string> QueryCode(const std::string &code, std::string query);
  static std::string QueryCodeFirst(const std::string &code, std::string query);
  /**
   * These two methods are static, meaning the user is responsible to free the returned document
   * The cache is not used even for the file option, because there's no instance of XMLDocReader involved
   */
  static ast::XMLDoc* CreateDocFromString(const std::string &code);
  static ast::XMLDoc* CreateDocFromFile(std::string filename);
private:
  XMLDocReader() {}
  static XMLDocReader *m_instance;
  std::map<std::string, ast::XMLDoc*> m_docs;
  std::vector<ast::XMLDoc*> m_string_docs;
};

#endif /* XML_DOC_READER_H */