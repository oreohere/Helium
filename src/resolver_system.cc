#include "resolver.h"
#include "utils.h"
#include <boost/filesystem.hpp>
#include <iostream>

namespace fs = boost::filesystem;
using namespace utils;

SystemResolver* SystemResolver::m_instance = 0;

static bool header_exists(const std::string header) {
  fs::path p("/usr/include/"+header);
  if (fs::exists(p)) return true;
  p = "/usr/local/include/" + header;
  if (fs::exists(p)) return true;
  return false;
}

SystemResolver::SystemResolver() {
  std::string s = getenv("HELIUM_HOME");
  std::ifstream is;
  is.open(s+"/headers.conf");
  if (is.is_open()) {
    std::string line;
    std::string flag;
    while (getline(is, line)) {
      trim(line);
      flag = "";
      if (!line.empty() && line[0] != '#') {
        if (line.find(' ') != std::string::npos) {
          flag = line.substr(line.find(' '));
          line = line.substr(0, line.find(' '));
          trim(flag);
        }
        if (header_exists(line)) {
          m_headers.insert(line);
          if (!flag.empty()) {
            m_libs.insert(flag);
          }
        } else {
        }
      }
    }
  }
}

/**
 * Check headers in headers.conf exist or not.
 * Print out!
 */
void SystemResolver::check_headers() {
  std::string s = getenv("HELIUM_HOME");
  std::ifstream is;
  is.open(s+"/headers.conf");
  std::vector<std::string> suc;
  std::vector<std::string> err;
  if (is.is_open()) {
    std::string line;
    std::string flag;
    while (getline(is, line)) {
      trim(line);
      flag = "";
      if (!line.empty() && line[0] != '#') {
        if (line.find(' ') != std::string::npos) {
          flag = line.substr(line.find(' '));
          line = line.substr(0, line.find(' '));
          trim(flag);
        }
        if (header_exists(line)) {
          suc.push_back(line);
        } else {
          err.push_back(line);
        }
      }
    }
  }
  for (std::string s : suc) {
    utils::print(s + " ", CK_Green);
  }
  for (std::string e : err) {
    utils::print(e + " ", CK_Red);
  }

}

std::string
SystemResolver::GetHeaders() const {
  std::string code;
  for (auto it=m_headers.begin();it!=m_headers.end();it++) {
    code += "#include <" + *it + ">\n";
  }
  return code;
}

std::string
SystemResolver::GetLibs() const {
  std::string flags;
  for (auto it=m_libs.begin();it!=m_libs.end();it++) {
    flags += *it + " ";
  }
  return flags;
}

// resolve to primitive type
// uint8_t => unsigned char"
std::string
SystemResolver::ResolveType(const std::string& name) {
  std::vector<CtagsEntry> entries = Parse(name, "t");
  for (auto it=entries.begin();it!=entries.end();it++) {
    std::string pattern = it->GetPattern();
    // /^} FILE;$/
    if (pattern.find("typedef") == std::string::npos || pattern.rfind(';') == std::string::npos) continue;
    pattern = pattern.substr(pattern.find("typedef"));
    pattern = pattern.substr(0, pattern.rfind(';'));
    // FIXME typedef xxx xxx yyy ;
    std::vector<std::string> vs = split(pattern);
    // to_type is the middle part of split
    std::string to_type;
    for (size_t i=1;i<vs.size()-1;i++) {
      to_type += vs[i]+' ';
    }
    to_type.pop_back();
    // if it is primitive type, return
    if (is_primitive(to_type)) {
      return to_type;
    }
    // depth first resolve
    std::string new_type = ResolveType(to_type);
    if (!new_type.empty()) return new_type;
  }
  return "";
}

bool
SystemResolver::Has(const std::string& name) {
  // only consider type:
  //    g: enum
  //    s: struct
  //    u: union
  //    t: typedef
  std::vector<CtagsEntry> entries = Parse(name, "gsut");
  if (entries.empty()) return false;
  else {
    return true;
  }
}

void
SystemResolver::Load(const std::string& tagfile) {
  tagFileInfo *info = (tagFileInfo*)malloc(sizeof(tagFileInfo));
  m_tagfile = tagsOpen (tagfile.c_str(), info);
  if (info->status.opened != true) {
    std::cerr << "[SystemResolver::Load] cannot load tagfile: " << tagfile
    << ". Did you forget to make systype.tags file?" << std::endl;
    exit(1);
  }
  m_entry = (tagEntry*)malloc(sizeof(tagEntry));
}

std::vector<CtagsEntry>
SystemResolver::Parse(const std::string& name) {
  std::vector<CtagsEntry> vc;
  assert(m_tagfile != NULL);
  tagResult result = tagsFind(m_tagfile, m_entry, name.c_str(), TAG_FULLMATCH);
  while (result == TagSuccess) {
    if (m_entry->kind) {
      vc.push_back(CtagsEntry(m_entry));
    }
    // find next
    result = tagsFindNext(m_tagfile, m_entry);
  }
  return vc;
}


std::vector<CtagsEntry>
SystemResolver::Parse(const std::string& name, const std::string& type) {
  std::vector<CtagsEntry> vc;
  assert(m_tagfile != NULL);
  tagResult result = tagsFind(m_tagfile, m_entry, name.c_str(), TAG_FULLMATCH);
  while (result == TagSuccess) {
    if (m_entry->kind && type.find(*(m_entry->kind)) != std::string::npos) {
      vc.push_back(CtagsEntry(m_entry));
    }
    // find next
    result = tagsFindNext(m_tagfile, m_entry);
  }
  return vc;
}
