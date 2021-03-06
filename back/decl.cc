#include "helium/parser/ast_node.h"
#include "helium/utils/log.h"


Decl::~Decl() {
  if (m_type) {
    delete m_type;
  }
}

Decl* DeclFactory::CreateDecl(XMLNode decl_node) {
  // m_type
  std::string type = decl_get_type(decl_node);
  std::vector<std::string> dims = decl_get_dimension(decl_node);
  std::string name = decl_get_name(decl_node);
  // FIXME NOW dims (HEBI: dims char*argv[])
  // Will be free-d in ~Decl
  // Type *t = TypeFactory::CreateType(type);
  Type *t = TypeFactory::CreateType(decl_node);
  if (t) {
    return new Decl(t, name, decl_node);
  } else {
    // helium_log_warning("Decl create failed because of Type create failure: " + type + " " + name);
    std::cerr << "Decl create failed because of Type create failure: " + type + " " + name << "\n";
    return NULL;
  }
}
