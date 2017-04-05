#include "helium/resolver/SnippetV2.h"
#include "helium/utils/fs_utils.h"

#include "helium/resolver/SnippetAction.h"

#include "helium/resolver/graph.h"

#include "helium/resolver/resolver.h"
#include "helium/utils/fs_utils.h"
#include <rapidjson/istreamwrapper.h>

using namespace v2;

using std::set;
using std::vector;

GlobalSnippetManager *GlobalSnippetManager::instance = nullptr;

std::string read_file_for_code(fs::path file, SourceLocation begin, SourceLocation end) {
  int l1 = begin.getLine();
  int c1 = begin.getColumn();
  int l2 = end.getLine();
  int c2 = end.getColumn();
  
  std::ifstream is;
  is.open(file.string());
  int l=0;
  std::string ret;
  if (is.is_open()) {
    std::string line;
    while(getline(is, line)) {
      l++;
      if (l < l1 || l > l2) {
      } else if (l>l1 && l < l2) {
        ret += line + "\n";
      } else if (l==l1 && l == l2) {
        ret += line.substr(c1-1, c1+1 - c1);
      } else if (l==l2) {
        ret += line.substr(0, c2);
      } else if (l==l1) {
        ret += line.substr(c1-1) + "\n";
      } else {
        break;
      }
    }
    is.close();
  }
  return ret;
}

std::string read_file_for_code_until_semicolon(fs::path file, SourceLocation begin, SourceLocation end) {
  std::ifstream is;
  is.open(file.string());
  std::string ret;
  assert(is.is_open());
  int line=1,col=0;
  char c;
  while (is.get(c)) {
    if (c=='\n') {line++;col=0;}
    else col++;

    SourceLocation loc(line,col);

    if (loc < begin) ;
    else if (loc >= begin && loc <= end ) ret+=c;
    else {
      is.unget();
      break;
    }
  }
  while (is.get(c)) {
    if (c==';') break;
    ret += c;
  }
  is.close();
  return ret;
}

void v2::FunctionSnippet::readCode() {
  Code = read_file_for_code(File, Begin, End);
}
void v2::RecordSnippet::readCode() {
  Code = read_file_for_code(File, Begin, End);
}
void v2::EnumSnippet::readCode() {
  Code = read_file_for_code(File, Begin, End);
}

/**
 * This performs the char-by-char read.
 * The variable does not stop until a semi-colon
 * I know this is a trick.
 */
void VarSnippet::readCode() {
  // this should read until a semi-colon
  Code = read_file_for_code_until_semicolon(File, Begin, End);
}

void TypedefSnippet::readCode() {
  Code = read_file_for_code_until_semicolon(File, Begin, End);
}


void v2::Snippet::dump(std::ostream &os) {
  rapidjson::Document doc;
  doc.SetObject();
  rapidjson::Value v = saveJson(doc.GetAllocator());
  rapidjson::StringBuffer sb;
  // rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);
  rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
  v.Accept(writer);
  os << sb.GetString() << "\n";
}

void SnippetManager::createOuters() {
  // 1. enclosing
  // - go through all snippets, split them by files
  std::map<std::string, std::vector<Snippet*> > File2SnippetsMap;
  for (Snippet *s : Snippets) {
    File2SnippetsMap[s->getFile()].push_back(s);
  }
  // - order them based on begin loc
  for (auto &m : File2SnippetsMap) {
    std::vector<Snippet*> v = m.second;
    std::sort(v.begin(), v.end(), [](Snippet *lhs, Snippet *rhs) {
        SourceLocation loc1_begin = lhs->getBeginLoc();
        SourceLocation loc1_end = lhs->getEndLoc();
        SourceLocation loc2_begin = rhs->getBeginLoc();
        SourceLocation loc2_end = rhs->getEndLoc();
        if (loc1_begin < loc2_begin) return true;
        else if (loc1_begin == loc2_begin) return loc1_end < loc2_end;
        else return false;
      });
    // - for each, go through and find until the begin loc of the next is larget then its end, record that
    for (int i=0;i<v.size();i++) {
      for (int j=i+1;j<v.size();j++) {
        Snippet *s1 = v[i];
        Snippet *s2 = v[j];
        SourceLocation loc1_begin = s1->getBeginLoc();
        SourceLocation loc1_end = s1->getEndLoc();
        SourceLocation loc2_begin = s2->getBeginLoc();
        SourceLocation loc2_end = s2->getEndLoc();
        if (loc2_begin < loc1_end) {
          // Outers[s2].insert(s1);
          s2->addOuter(s1);
        }
      }
    }
  }
}

void SnippetManager::createDeps() {
  // 2. dep
  // - for each, get a list of dependence key
  for (Snippet *s : Snippets) {
    s->getCode();
    std::string name = s->getName();
    set<std::string> ids = extract_id_to_resolve(s->getCode());
    // do not query its name itself
    ids.erase(name);
    for (auto &id : ids) {
      // - query key map and create the dep
      s->addDep(get(id));
    }
    // remove itself
    s->removeDep(s);
  }
}


/**
 * Disk
 */
// void SnippetManager::saveSnippet(fs::path p) {
//   std::ofstream os;
//   os.open(p.string());
//   assert(os.is_open());
//   for (Snippet *s : Snippets) {
//     s->save(os);
//   }
// }
// void SnippetManager::loadSnippet(fs::path p) {
//   assert(Snippets.empty());
//   std::ifstream is;
//   is.open(p.string());
//   assert(is.is_open());
//   std::string kind;
//   Snippet *s = nullptr;
//   // while (is >> kind) {
//   //   std::cout << kind << "\n";
//   // }
//   while (is >> kind) {
//     if (kind == "FunctionSnippet") s = new FunctionSnippet();
//     else if (kind == "VarSnippet") s = new VarSnippet();
//     else if (kind == "TypedefSnippet") s = new TypedefSnippet();
//     else if (kind == "RecordSnippet") s = new RecordSnippet();
//     else {
//       assert(false);
//     }
//     s->load(is);
//     // while (is >> kind) {
//     //   std::cout << kind << "\n";
//     // }
//     int id = Snippets.size();
//     s->setId(id);
//     Snippets.push_back(s);
//   }
// }

// void SnippetManager::saveDeps(fs::path p) {
//   std::ofstream os;
//   os.open(p.string());
//   assert(os.is_open());
//   for (auto &m : Deps) {
//     os << m.first->getId() << " ";
//     for (Snippet *s : m.second) {
//       os << s->getId() << " ";
//     }
//     os << "\n";
//   }
// }

// void SnippetManager::loadDeps(fs::path p) {
//   std::ifstream is;
//   is.open(p.string());
//   assert(is.is_open());
//   std::string line;
//   while (getline(is, line)) {
//     int from_id,to_id;
//     std::istringstream ss(line);
//     ss >> from_id;
//     while (ss >> to_id) {
//       // IdDeps[from_id].insert(to_id);
//       Deps[Snippets[from_id]].insert(Snippets[to_id]);
//     }
//   }
// }

// void SnippetManager::saveOuters(fs::path p) {
//   std::ofstream os;
//   os.open(p.string());
//   assert(os.is_open());
//   for (auto &m : Outers) {
//     os << m.first->getId() << " ";
//     for (Snippet *s : m.second) {
//       os << s->getId() << " ";
//     }
//     os << "\n";
//   }
// }

// void SnippetManager::loadOuters(fs::path p) {
//   std::ifstream is;
//   is.open(p.string());
//   assert(is.is_open());
//   std::string line;
//   while (getline(is, line)) {
//     int from_id,to_id;
//     std::istringstream ss(line);
//     ss >> from_id;
//     while (ss >> to_id) {
//       // IdDeps[from_id].insert(to_id);
//       Outers[Snippets[from_id]].insert(Snippets[to_id]);
//     }
//   }
// }




void SnippetManager::dumpSnippetsVerbose(std::ostream &os) {
  os << "== Total " << Snippets.size() << " snippets\n";
  for (Snippet *s : Snippets) {
    os << "\t";
    // s->save(os);
    os << s->getCode();
    os << "\n";
  }
}

void SnippetManager::dumpLight(std::ostream &os) {
  os << "Snippets: " << "\n";
  int func_ct = 0;
  int var_ct = 0;
  int record_ct = 0;
  int typedef_ct = 0;
  int enum_ct = 0;
  for (Snippet *s : Snippets) {
    if (dynamic_cast<FunctionSnippet*>(s)) func_ct++;
    else if (dynamic_cast<TypedefSnippet*>(s)) typedef_ct++;
    else if (dynamic_cast<EnumSnippet*>(s)) enum_ct++;
    else if (dynamic_cast<VarSnippet*>(s)) var_ct++;
    else if (dynamic_cast<RecordSnippet*>(s)) record_ct++;
  }
  // type of snippet
  os << "\t" << "Function: " << func_ct << "\n";
  os << "\t" << "Record: " << record_ct << "\n";
  os << "\t" << "Typedef: " << typedef_ct << "\n";
  os << "\t" << "Enum: " << enum_ct << "\n";
  os << "\t" << "Var: " << var_ct << "\n";

  // int deps_ct = 0;
  // for (auto &m : Deps) {
  //   deps_ct += m.second.size();
  // }
  // os << "Deps: " << deps_ct << "\n";

  // int outers_ct = 0;
  // for (auto &m : Outers) {
  //   outers_ct += m.second.size();
  // }
  // os << "Deps: " << outers_ct << "\n";
}

void SnippetManager::dump(std::ostream &os) {

  os << "== Total " << Snippets.size() << " snippets\n";
  os << "== KeyMap: " << KeyMap.size() << "\n";
  for (Snippet *s : Snippets) {
    // s->dump(os);
    // os << "\n";
    // s->save(os);
    // os << s->getCode();
    os << "\t";
    s->dump(os);
    os << "\n";
  }
  // os << "== Dependence:" << "\n";
  // // std::map<Snippet*, std::set<Snippet*> > Deps;
  // // std::map<Snippet*, std::set<Snippet*> > Outer;
  // for (auto &m : Deps) {
  //   Snippet *from = m.first;
  //   std::set<Snippet*> to = m.second;
  //   os << "\t";
  //   from->dump(os);
  //   os << " ==> ";
  //   for (Snippet *s : to) {
  //     s->dump(os);
  //     os << ", ";
  //   }
  //   os << "\n";
  // }
  // os << "== Outers:" << "\n";
  // for (auto &m : Outers) {
  //   Snippet *from = m.first;
  //   std::set<Snippet*> outers = m.second;
  //   os << "\t";
  //   from->dump(os);
  //   os << " ==> ";
  //   for (Snippet *s : outers) {
  //     s->dump(os);
  //     os << ", ";
  //   }
  //   os << "\n";
  // }
}


std::set<v2::Snippet*> v2::Snippet::getAllDeps() {
  std::set<Snippet*> worklist;
  std::set<Snippet*> done;
  std::set<Snippet*> ret;
  worklist.insert(this);
  while (!worklist.empty()) {
    Snippet *s = *worklist.begin();
    worklist.erase(s);
    if (done.count(s) == 1) continue;
    std::set<Snippet*> dep = s->getDeps();
    worklist.insert(dep.begin(), dep.end());
    ret.insert(dep.begin(), dep.end());
    done.insert(s);
  }
  ret.erase(this);
  return ret;
}

std::set<v2::Snippet*> SnippetManager::replaceNonOuters(std::set<Snippet*> ss) {
  std::set<Snippet*> ret;
  std::vector<Snippet*> worklist(ss.begin(), ss.end());
  std::set<Snippet*> done;
  while (!worklist.empty()) {
    Snippet *s = worklist.back();
    worklist.pop_back();
    done.insert(s);
    std::set<Snippet*> outers = s->getOuters();
    if (outers.empty()) ret.insert(s);
    for (Snippet *o : outers) {
      if (done.count(o) == 0) worklist.push_back(o);
    }
  }
  return ret;
}


void SnippetManager::traverseDir(fs::path dir) {
  fs::recursive_directory_iterator it(dir), eod;
  BOOST_FOREACH(fs::path const &p, std::make_pair(it, eod)) {
    if (p.extension() == ".c" || p.extension() == ".h") {
      std::vector<v2::Snippet*> snippets = createSnippets(p);
      add(snippets);
    }
  }
  process();
}



/**
 * Today I want to try a different approach.
 * 1. use the snippet got, infer the header dependence
 * 2. use that header dependence to sort the files
 */
std::vector<v2::Snippet*> SnippetManager::sort(std::set<Snippet*> snippets) {
  // DONE ELSEWHERE get header dependence
  // DONE ELSEWHERE sort within file
  // sort across file
  std::vector<v2::Snippet*> ret;
  for (v2::Snippet *s : SnippetV) {
    if (snippets.count(s) == 1) ret.push_back(s);
  }
  return ret;
}

bool is_type_snippet(v2::Snippet *s) {
  if (!s) return false;
  return (dynamic_cast<RecordSnippet*>(s)
          || dynamic_cast<TypedefSnippet*>(s)
          || dynamic_cast<EnumSnippet*>(s));
}

void SnippetManager::sortFiles() {
  // create File2SnippetMap
  FileMap.clear();
  FileDep.clear();
  // std::map<std::string, std::set<Snippet*> > FileMap;
  for (Snippet *s : Snippets) {
    FileMap[s->getFile()].insert(s);
  }
  // use deps, get file deps
  std::map<std::string, std::map<std::string, int> > mat;
  // std::map<Snippet*, std::set<Snippet*> > Deps;
  for (Snippet *from : Snippets) {
    if (!is_type_snippet(from)) continue;
    std::string from_str = from->getFile();
    std::set<Snippet*> deps = from->getDeps();
    for (auto *to : deps) {
      if (!is_type_snippet(to)) continue;
      std::string to_str = to->getFile();
      if (mat.count(from_str) == 1) {
        mat[from_str][to_str]++;
        mat[to_str][from_str]--;
      } else {
        mat[from_str];
        mat[from_str][to_str]=1;
        mat[to_str];
        mat[to_str][from_str]=-1;
      }
    }
  }
  // std::map<std::string, std::set<std::string> > FileDep;
  for (auto &m : mat) {
    for (auto &n : m.second) {
      if (n.second > 0) {
        FileDep[m.first].insert(n.first);
      }
    }
  }
  // check if file dep is DAG (Directed Acyclic Graph)
  // Use Topological sorting to get a vector of files
  // FileV.clear();
  // recursively get nodes with no incoming edges (Kahn's algorithm)

  // std::map<std::string, std::set<Snippet*> > FileMap;
  // std::map<std::string, std::set<std::string> > FileDep;
  // for (auto &m : FileMap) {
  //   std::string file = m.first;
  //   if (FileDep.count(file) == 0) {
  //     FileV.push_back(file);
  //   }
  // }
  topoSortFiles();
}

void SnippetManager::topoSortFiles() {
  FileV.clear();
  SnippetV.clear();
  hebigraph::Graph<std::string> graph;
  // std::map<std::string, std::set<Snippet*> > FileMap;
  // std::map<std::string, std::set<std::string> > FileDep;
  for (auto &m : FileMap) {
    graph.addNode(m.first);
  }
  for (auto &m : FileDep) {
    for (std::string s : m.second) {
      graph.addEdge(m.first, s);
    }
  }
  std::vector<std::string> sorted = graph.topoSort();
  // TODO examine sorted !!!
  FileV = sorted;
  // sort also the snippets
  for (std::string file : FileV) {
    std::set<Snippet*> snippets = FileMap[file];
    std::vector<Snippet*> v(snippets.begin(), snippets.end());
    std::sort(v.begin(), v.end(), [](Snippet *lhs, Snippet *rhs){
        SourceLocation loc1 = lhs->getBeginLoc();
        SourceLocation loc2 = rhs->getBeginLoc();
        return loc1 < loc2;
      });
    SnippetV.insert(SnippetV.end(), v.begin(), v.end());
  }
}




/**
 * Scheme
[
    {
        "name": "foo",
        "ID": 0,
        "kind": "Record",
        "file": "/path/to/file",
        "begin": {
            "line": 8,
            "col": 10
        },
        "end": {
            "line": 8,
            "col": 10
        },
        "deps": [2,5,8],
        "outers": [3,8]
    }
]

 */

/**
 * push back
 */
rapidjson::Value v2::Snippet::saveJson(rapidjson::Document::AllocatorType &allocator) {
  // rapidjson::Document::AllocatorType &allocator = document.GetAllocator();
  rapidjson::Value array(rapidjson::kObjectType);
  rapidjson::Value obj(rapidjson::kObjectType);
  obj.AddMember("name", rapidjson::StringRef(Name.c_str()), allocator);
  obj.AddMember("id", ID, allocator);
  obj.AddMember("file", rapidjson::StringRef(File.c_str()), allocator);
  // begin & end
  rapidjson::Value begin(rapidjson::kObjectType);
  rapidjson::Value end(rapidjson::kObjectType);
  begin.AddMember("line", Begin.getLine(), allocator);
  begin.AddMember("col", Begin.getColumn(), allocator);
  end.AddMember("line", End.getLine(), allocator);
  end.AddMember("col", End.getColumn(), allocator);
  obj.AddMember("begin", begin, allocator);
  obj.AddMember("end", end, allocator);

  // // dep & outer
  rapidjson::Value deps(rapidjson::kArrayType);
  rapidjson::Value outers(rapidjson::kArrayType);
  for (Snippet *dep : Deps) {
    deps.PushBack(dep->getId(), allocator);
  }
  for (Snippet *outer : Outers) {
    outers.PushBack(outer->getId(), allocator);
  }
  obj.AddMember("deps", deps, allocator);
  obj.AddMember("outers", outers, allocator);
  return obj;
  // document.PushBack(obj, allocator);
 }

void v2::Snippet::loadJson(rapidjson::Value &obj) {
  assert(obj.IsObject());
  Name = obj["name"].GetString();
  ID = obj["id"].GetInt();
  File = obj["file"].GetString();
  Begin = {obj["begin"]["line"].GetInt(),
           obj["begin"]["col"].GetInt()};
  End = {obj["end"]["line"].GetInt(),
           obj["end"]["col"].GetInt()};
  // deps and outers have to be add later
  // for (auto &v : obj["deps"].GetArray()) {
  //   Deps.insert(v.GetInt());
  // }
  // for (auto &v : obj["outers"].GetArray()) {
  //   Outers.insert(v.GetInt());
  // }
}


// rapidjson::Value FunctionSnippet::saveJson(rapidjson::Document::AllocatorType &allocator) {
//   rapidjson::Value v = Snippet::saveJson(allocator);
//   v.AddMember("kind", "FunctionSnippet", allocator);
//   return v;
// }


void SnippetManager::saveJson(fs::path p) {
  rapidjson::Document document;
  rapidjson::Document::AllocatorType &allocator = document.GetAllocator();
  document.SetArray();
  assert(document.IsArray());
  for (Snippet *s : Snippets) {
    rapidjson::Value obj = s->saveJson(allocator);
    document.PushBack(obj, allocator);
  }

  rapidjson::StringBuffer sb;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);
  document.Accept(writer);
  // sb.GetString();

  utils::write_file(p, sb.GetString());
}
void SnippetManager::loadJson(fs::path p) {
  rapidjson::Document document;
  std::ifstream ifs(p.string());
  rapidjson::IStreamWrapper isw(ifs);
  document.ParseStream(isw);
  assert(document.IsArray());
  // load snippets
  for (auto &v : document.GetArray()) {
    Snippet *s = nullptr;
    std::string kind = v["kind"].GetString();
    if (kind == "FunctionSnippet") s = new FunctionSnippet();
    else if (kind == "VarSnippet") s = new VarSnippet();
    else if (kind == "TypedefSnippet") s = new TypedefSnippet();
    else if (kind == "RecordSnippet") s = new RecordSnippet();
    else if (kind == "EnumSnippet") s = new EnumSnippet();
    else {assert(false);}

    s->loadJson(v);
    Snippets.push_back(s);
  }
  // load deps and outers
  for (auto &v : document.GetArray()) {
    int id = v["id"].GetInt();
    assert(id < Snippets.size());
    for (auto &dep : v["deps"].GetArray()) {
      int depId = dep.GetInt();
      assert(depId < Snippets.size());
      Snippets[id]->addDep(Snippets[depId]);
    }
    for (auto &outer : v["outers"].GetArray()) {
      int outerId = outer.GetInt();
      assert(outerId < Snippets.size());
      Snippets[id]->addOuter(Snippets[outerId]);
    }
  }
}
