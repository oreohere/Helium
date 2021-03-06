#include "helium/type/type_helper.h"
#include "helium/type/type_common.h"

using namespace utils;

bool search_and_remove(std::string &s, boost::regex reg) {
  if (boost::regex_search(s, reg)) {
    s = boost::regex_replace<boost::regex_traits<char>, char>(s, reg, "");
    return true;
  }
  return false;
}

int count_and_remove(std::string &s, char c) {
  int count = std::count(s.begin(), s.end(), c);
  if (count) {
    s.erase(std::remove(s.begin(), s.end(), c), s.end());
  }
  return count;
}

void fill_storage_specifier(std::string& name, struct storage_specifier& specifier) {
  specifier.is_auto     = search_and_remove(name, boost::regex("\\bauto\\b"))     ? 1 : 0;
  specifier.is_register = search_and_remove(name, boost::regex("\\bregister\\b")) ? 1 : 0;
  specifier.is_static   = search_and_remove(name, boost::regex("\\bstatic\\b"))   ? 1 : 0;
  specifier.is_extern   = search_and_remove(name, boost::regex("\\bextern\\b"))   ? 1 : 0;
  // specifier.is_typedef     = search_and_remove(name, typedef_regex)     ? 1 : 0;
}

void fill_type_specifier(std::string& name, struct type_specifier& specifier) {
  specifier.is_void     = search_and_remove(name, boost::regex("\\bvoid\\b"))     ? 1 : 0;
  specifier.is_char     = search_and_remove(name, boost::regex("\\bchar\\b"))     ? 1 : 0;
  specifier.is_short    = search_and_remove(name, boost::regex("\\bshort\\b"))    ? 1 : 0;
  specifier.is_int      = search_and_remove(name, boost::regex("\\bint\\b"))      ? 1 : 0;
  specifier.is_long     = search_and_remove(name, boost::regex("\\blong\\b"))     ? 1 : 0;
  specifier.is_float    = search_and_remove(name, boost::regex("\\bfloat\\b"))    ? 1 : 0;
  specifier.is_double   = search_and_remove(name, boost::regex("\\bdouble\\b"))   ? 1 : 0;
  specifier.is_signed   = search_and_remove(name, boost::regex("\\bsigned\\b"))   ? 1 : 0;
  specifier.is_unsigned = search_and_remove(name, boost::regex("\\bunsigned\\b")) ? 1 : 0;
  // TODO Hotfix for bool _Bool
  bool b1 = search_and_remove(name, boost::regex("\\bbool\\b")) ? 1 : 0;
  bool b2 = search_and_remove(name, boost::regex("\\b_Bool\\b")) ? 1 : 0;
  specifier.is_bool = (b1 == 1 || b2 == 1);
}
void fill_type_qualifier(std::string& name, struct type_qualifier& qualifier) {
  qualifier.is_const    = search_and_remove(name, boost::regex("\\bconst\\b"))    ? 1 : 0;
  qualifier.is_volatile = search_and_remove(name, boost::regex("\\bvolatile\\b")) ? 1 : 0;
}

void fill_struct_specifier(std::string& name, struct struct_specifier& specifier) {
  specifier.is_struct = search_and_remove(name, boost::regex("\\bstruct\\b")) ? 1 : 0;
  specifier.is_enum   = search_and_remove(name, boost::regex("\\benum\\b"))   ? 1 : 0;
  specifier.is_union  = search_and_remove(name, boost::regex("\\bunion\\b"))  ? 1 : 0;
}



/********************************
 * Helper functions
 *******************************/

/**
 * Usage:
 * - get_scanf_code("%c", "&" + var);
 */
std::string get_scanf_code(std::string format, std::string var) {
  return "scanf(\"" + format + "\", " + var + ");\n";
}

std::string get_malloc_code(std::string var, std::string type, std::string size) {
  return var + " = (" + type + "*)malloc(sizeof(" + type + ")*" + size + ");\n";
}

/**
 * For the printfs, the prefix will tell:
 * - I/O
 * - type:
 *   + d: int
 *   + p: address, hex
 *   + n: NULL, !NULL
 *   + c: char
 *   + x: an arbitrary string
 */

std::string get_sizeof_printf_code(std::string var) {
  std::string ret;
  // ret += "printf(\"int_" + var + ".size = %d\\n\", ";
  ret += "printf(\"sizeof(" + var + ") = %d\\n\", ";
  ret += "sizeof(" + var + "));\n" + flush_output;
  return ret;
}
std::string get_strlen_printf_code(std::string var) {
  std::string ret;
  // ret += "printf(\"int_" + var + ".strlen = %ld\\n\", ";
  ret += "printf(\"strlen(" + var + ") = %ld\\n\", ";
  ret += "strlen(" + var + "));\n" + flush_output;
  return ret;
}

std::string get_addr_printf_code(std::string var) {
  std::string ret;
  ret += "printf(\"addr_" + var + " = %p\\n\", ";
  ret += "(void*)" + var + ");\n" + flush_output;
  return ret;
}

/**
 * branch construction for NULL of a variable
 */
std::string get_check_null(std::string var, std::string true_branch, std::string false_branch) {
  std::string ret;
  ret += "if (" + var + " == NULL) {\n";
  ret += true_branch;
  ret += "} else {\n";
  ret += false_branch;
  ret += "}\n";
  return ret;
}

/**
 * Check whether helium_size is 0
 */
std::string get_helium_size_branch(std::string true_branch, std::string false_branch) {
  std::string ret;
  ret += "if (helium_size == 0) {\n";
  ret += true_branch;
  ret += "} else {\n";
  ret += false_branch;
  ret += "}\n";
  return ret;
}

/**
 * Get the raw ID of the type
 */
std::string get_id(std::string raw_type) {
  count_and_remove(raw_type, '*');
  struct storage_specifier ss;
  fill_storage_specifier(raw_type, ss);
  struct type_specifier ts;
  fill_type_specifier(raw_type, ts);
  struct type_qualifier tq;
  fill_type_qualifier(raw_type, tq);
  struct struct_specifier ss2;
  fill_struct_specifier(raw_type, ss2);
  trim(raw_type);
  return raw_type;
}

bool is_primitive(std::string s) {
  if (s.find('[') != std::string::npos) {
    s = s.substr(0, s.find('['));
  }
  count_and_remove(s, '*');
  struct storage_specifier storage_specifier;
  struct type_specifier type_specifier;
  struct type_qualifier type_qualifier;
  struct struct_specifier struct_specifier;
  fill_storage_specifier(s, storage_specifier);
  fill_type_specifier(s, type_specifier);
  fill_type_qualifier(s, type_qualifier);
  fill_struct_specifier(s, struct_specifier);
  trim(s);
  if (s.empty()) return true;
  return false;
}


/**
 * Heap size: 
 */
std::string GetHeliumHeapCode(std::string var, std::string body) {
  std::string ret;
  // std::cout << "Current levl: " << m_level << "\n";
  // use the last suffix ...
  ret += "helium_heap_target_size = -1;\n";
  ret += "for (int i=0;i<helium_heap_top;i++) {\n";
  ret += "  if (" + var + " == helium_heap_addr[i]) {\n";
  ret += "    helium_heap_target_size = helium_heap_size[i];\n";
  ret += "    break;\n";
  ret += "  }\n";
  ret += "}\n";
  ret += "if (helium_heap_target_size != -1) {\n";
  ret += "  printf(\"int_" + var + "_heapsize = %d\\n\", ";
  ret += "helium_heap_target_size);\n";
  ret += flush_output;
  ret += "  for (int i=0;i<helium_heap_target_size;i++) {\n";
  ret += "// HeliumHeapCode body\n";
  ret += body;
  ret += " }\n";
  ret += "}\n";
  return ret;
}
