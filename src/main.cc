#include <iostream>
#include <cstdlib>

#include "helium/workflow/helium.h"

#include "helium/resolver/snippet_db.h"
#include "helium/parser/cfg.h"
#include "helium/parser/xml_doc_reader.h"
#include "helium/parser/ast_node.h"

#include "helium/utils/helium_options.h"
#include "helium/parser/point_of_interest.h"
#include "helium/utils/fs_utils.h"
#include "helium/utils/utils.h"

#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>

#include <gtest/gtest.h>

namespace fs = boost::filesystem;

static void
create_tagfile(const std::string& folder, const std::string& file) {
  assert(utils::exists(folder));
  // use full path because I want to have the full path in tag file
  std::string full_path = utils::full_path(folder);
  std::string cmd = "ctags -f ";
  cmd += file;
  cmd += " --languages=c,c++ -n --c-kinds=+x --exclude=heium_result -R ";
  cmd += full_path;
  // std::cout << cmd  << "\n";
  // std::system(cmd.c_str());
  int status;
  utils::exec(cmd.c_str(), &status);
  if (status != 0) {
    std::cerr << "create tagfile failed" << "\n";
    std::cerr << cmd << "\n";
    exit(1);
  }
}

// void check_light_utilities() {
//   if (HeliumOptions::Instance()->Has("print-config")) {
//     std::cout << "DEPRECATED print-config"  << "\n";
//     exit(0);
//   }
//   if (HeliumOptions::Instance()->Has("resolve-system-type")) {
//     std::string type = HeliumOptions::Instance()->GetString("resolve-system-type");
//     std::string output = SystemResolver::Instance()->ResolveType(type);
//     std::cout << output  << "\n";
//     exit(0);
//   }
// }

// void check_target_folder(std::string folder) {
//   if (folder.empty()) {
//     HeliumOptions::Instance()->PrintHelp();
//     exit(1);
//   }
//   if (utils::is_dir(folder)) {
//     while (folder.back() == '/') folder.pop_back();
//   } else if (utils::is_file(folder)) {
//   } else {
//     std::cerr<<"target folder: " << folder << " does not exists." <<'\n';
//     assert(false);
//   }
// }

void create_utilities(std::string folder) {
  if (HeliumOptions::Instance()->Has("create-tagfile")) {
    std::string output_file = HeliumOptions::Instance()->GetString("output");
    if (output_file.empty()) output_file = "tags";
    create_tagfile(folder, output_file);
    exit(0);
  }

  if (HeliumOptions::Instance()->Has("create-snippet-db")) {
    std::string output_folder;
    if (HeliumOptions::Instance()->Has("output")) {
      output_folder = HeliumOptions::Instance()->GetString("output");
    }
    if (output_folder.empty()) output_folder = "snippets";

    if (fs::exists(output_folder)) {
      std::cerr << "Folder " << output_folder << " exists. Remove it before this command." << "\n";
      exit(1);
    }

    
    std::string tagfile;
    if (HeliumOptions::Instance()->Has("tagfile")) {
      tagfile = HeliumOptions::Instance()->GetString("tagfile");
    }
    std::string tmpdir = utils::create_tmp_dir();
    create_tagfile(folder, tmpdir+"/tags");
    tagfile = tmpdir+"/tags";
    std::cout << "created tagfile: " << tagfile  << "\n";
    SnippetDB::Instance()->Create(tagfile, output_folder);
    exit(0);
  }
}

std::vector<std::string> get_c_files(std::string folder) {
  std::vector<std::string> ret;
  /* get ret in target folder */
  if (utils::is_dir(folder)) {
    utils::get_files_by_extension(folder, ret, "c");
  } else {
    // this is just a file.
    ret.push_back(folder);
  }
  return ret;
}

void print_utilities(std::string folder) {

  if (HeliumOptions::Instance()->Has("show-callgraph")) {
    SnippetDB::Instance()->PrintCG();
    exit(0);
  }
  /**
   * Print src meta.
   * 0. check headers
   * 1. headers used
   * 2. header dependence
   */
  if (HeliumOptions::Instance()->Has("show-meta")) {
    std::cout << "== Helium Meta Data Printer =="  << "\n";
    std::cout << "== Headers"  << "\n";
    // 0
    SystemResolver::check_headers();
    // 1
    // std::string header = SystemResolver::Instance()->GetHeaders();
    // std::cout << header  << "\n";
    // std::cout << "== Headers available on the machine"  << "\n";
    std::set<std::string> avail_headers = SystemResolver::Instance()->GetAvailableHeaders();
    // for (std::string s : avail_headers) {
    //   std::cout << s  << "\n";
    // }
    std::cout << "== Headers used in project"  << "\n";
    std::cout  << "\t";
    std::set<std::string> used_headers = HeaderResolver::Instance()->GetUsedHeaders();
    for (std::string s : used_headers) {
      std::cout << s  << " ";
    }
    std::cout  << "\n";
    // 2
    std::cout << "== Header Dependence"  << "\n";
    // HeaderDep::Instance()->Dump();
    HeaderResolver::Instance()->DumpDeps();
    std::cout << "== Final headers included:"  << "\n";
    std::cout << "\t";
    for (std::string s : avail_headers) {
      if (used_headers.count(s) == 1) {
        std::cout << s  << " ";
      }
    }
    std::cout  << "\n";
    exit(0);
  }

  std::vector<std::string> files = get_c_files(folder);
}

void load_tagfile(std::string folder) {
  if (HeliumOptions::Instance()->Has("tagfile")) {
    std::string tagfile = HeliumOptions::Instance()->GetString("tagfile");
    ctags_load(tagfile);
  } else {
    std::string tmpdir = utils::create_tmp_dir();
    create_tagfile(folder, tmpdir+"/tags");
    std::string tagfile = tmpdir+"/tags";
    ctags_load(tagfile);
  }
}

void load_snippet_db(std::string snippet_db_folder) {
  // std::string snippet_db_folder = HeliumOptions::Instance()->GetString("snippet-db-folder");
  if (snippet_db_folder.empty()) {
    if (HeliumOptions::Instance()->Has("verbose")) {
      std::cout
        << "Using default snippet folder: './snippets/'."
        << "If not desired, set via '-s' option."  << "\n";
    }
    snippet_db_folder = "snippets";
    // assert(false);
  }
  if (!utils::exists(snippet_db_folder)) {
    std::cout << "EE: Snippet folder " << snippet_db_folder << " does not exist."  << "\n";
    exit(1);
  }
  SnippetDB::Instance()->Load(snippet_db_folder);
}

void load_header_resolver(std::string src_folder) {
  // HeaderResolver::Instance()->Load(folder);
  // std::string src_folder = HeliumOptions::Instance()->GetString("src-folder");
  if (src_folder.empty()) {
    if (HeliumOptions::Instance()->Has("verbose")) {
      std::cerr
        << "Using default src folder: 'src'."
        << "If not desired, set via '-c' option."  << "\n";
    }
    src_folder = "src";
  }
  if (!utils::exists(src_folder)) {
    std::cerr << "EE: src folder " << src_folder << " does not exist."  << "\n";
    exit(1);
  }
  HeaderResolver::Instance()->Load(src_folder);
}

void load_slice() {
  if (HeliumOptions::Instance()->Has("slice-file")) {
    std::string slice_file = HeliumOptions::Instance()->GetString("slice-file");
    SimpleSlice::Instance()->SetSliceFile(slice_file);
  }
}

int main(int argc, char* argv[]) {
  utils::seed_rand();
  const char *home = getenv("HOME");
  if (!home) {
    std::cerr << "EE: HOME env is not set." << "\n";
    exit(1);
  }

  /* parse arguments */
  HeliumOptions::Instance()->ParseCommandLine(argc, argv);
  HeliumOptions::Instance()->ParseConfigFile("~/.heliumrc");


  if (HeliumOptions::Instance()->Has("help")) {
    HeliumOptions::Instance()->PrintHelp();
    exit(0);
  }


  if (HeliumOptions::Instance()->Has("show-instrument-code")) {
    std::string code = HeliumOptions::Instance()->GetString("show-instrument-code");
    if (code.empty()) {
      std::cerr << "EE: Code is empty." << "\n";
      exit(1);
    }
    if (code.back() != ';') {
      std::cerr << "EE: code must be a decl_stmt, must end with semicolon" << "\n";
      exit(1);
    }
    XMLDoc *doc = XMLDocReader::CreateDocFromString(code, "");
    XMLNode decl_node = find_first_node_bfs(doc->document_element(), "decl");
    Decl *decl = DeclFactory::CreateDecl(decl_node);
    Type *type = decl->GetType();
    std::string var = decl->GetName();
    std::string output = type->GetOutputCode(var);
    std::string input = type->GetInputCode(var);
    std::cout << "// Output:" << "\n";
    std::cout << output << "\n";
    std::cout << "// Input:" << "\n";
    std::cout << input << "\n";
    exit(0);
  }

  fs::path user_home(getenv("HOME"));
  fs::path helium_home = user_home / ".helium.d";
  if (!fs::exists(helium_home)) {
    fs::create_directory(helium_home);
  }
  fs::path systag = helium_home / "systype.tags";
  if (!fs::exists(systag)) {
    if (HeliumOptions::Instance()->Has("setup")) {
      // run ctags to create tags
      std::cout << "Creating ~/systype.tags" << "\n";
      std::string cmd = "ctags -f " + systag.string() +
        " --exclude=boost"
        " --exclude=llvm"
        " --exclude=c++"
        " --exclude=linux"
        " --exclude=xcb"
        " --exclude=X11"
        " --exclude=openssl"
        " --exclude=xorg"
        " -R /usr/include/ /usr/local/include";
      std::cout << "Running " << cmd << "\n";
      utils::exec(cmd.c_str());
      std::cout << "Done" << "\n";
      exit(0);
    } else {
      std::cout << "No systype.tags found. Run helium --setup first." << "\n";
      exit(1);
    }
  }
  SystemResolver::Instance()->Load(systag.string());
  
  // Reading target folder. This is the parameter
  std::string folder = HeliumOptions::Instance()->GetString("folder");
  folder = utils::escape_tide(folder);
  // the folder argument contains:
  // 1. cpped
  // 2. src
  // 3. snippets
  fs::path target(folder);
  // fs::path cpped = target / "cpped";
  // fs::path src = target / "src";
  // fs::path snippets = target / "snippets";
  if (!fs::exists(target)) {
    std::cerr << "EE: target folder or file " << target.string() << " does not exist." << "\n";
    exit(1);
  }


  // change relative to absolute
  std::string cache_dir_name = fs::canonical(target).string();
  std::replace(cache_dir_name.begin(), cache_dir_name.end(), '/', '_');
  fs::path cache_dir(helium_home / "cache" / cache_dir_name);

  if (HeliumOptions::Instance()->Has("extract")) {
    if (fs::exists(cache_dir)) {
      std::cout << "Cache exists: " << cache_dir.string() << "\n";
      std::cout << "Replace? [y/N] " << std::flush;
      char c = getchar();
      if (c != 'y' && c != 'Y') {
        std::cout << "Cancelled." << "\n";
        exit(1);
      }
    }
    fs::remove_all(cache_dir);
    fs::create_directories(cache_dir);
    fs::create_directories(cache_dir / "cpp");
    fs::create_directories(cache_dir / "src");
    fs::create_directories(cache_dir / "snippet");
    // preprocess and put everything in "cpp" folder
    std::cout << "Preprocessing .." << "\n";

    // put source files into "src" folder
    // std::string cmd = "find "
    //   + target.string()
    //   + " -name \"*.[c|h]\" -exec cp \"{}\" "
    //   + cache_dir.string() + "src \\;";
    // utils::new_exec(cmd.c_str());

    fs::recursive_directory_iterator it(target), eod;
    BOOST_FOREACH (fs::path const & p, std::make_pair(it, eod)) {
      if (is_regular_file(p)) {
        if (p.extension() == ".c" || p.extension() == ".h") {
          // std::cout << "Copying from " << p.string() << " to " << "..." << "\n";
          fs::copy_file(p, cache_dir / "src" / p.filename(), fs::copy_option::overwrite_if_exists);
        }
      }
    }
    
    // process and put "cpp" folder
    for (fs::directory_entry &e : fs::directory_iterator(cache_dir / "src")) {
      fs::path p = e.path();
      fs::path filename = p.filename();
      if (p.extension() == ".c") {
        std::string cmd = "clang -E " + p.string()
          + " >> " + (cache_dir / "cpp" / p.filename()).string();
        utils::new_exec(cmd.c_str());
      }
    }
    
    // create tag file
    fs::path tagfile = cache_dir / "tags";
    std::cout << "Creating tagfile .." << "\n";
    create_tagfile((cache_dir / "cpp").string(), (cache_dir / "tags").string());
    std::cout << "Creating Snippet DB .." << "\n";
    // this will call srcml
    SnippetDB::Instance()->Create(tagfile.string(), (cache_dir / "snippet").string());
    exit(0);
  }


  if (!fs::exists(cache_dir)) {
    std::cerr << "The benchmark is not processed. Consider run helium --extract /path/to/benchmark" << "\n";
    exit(1);
  }

  // load_tagfile((cache_dir / "cpp").string());
  ctags_load((cache_dir / "tags").string());
  load_snippet_db((cache_dir / "snippet").string());
  load_header_resolver((cache_dir / "src").string());

  // check if the snippet database has been generated
  

  // load_tagfile(cpped.string());
  // load_snippet_db(snippets.string());
  // load_header_resolver(src.string());
  // load_slice();

  
  // check_light_utilities();
  // check_target_folder(cpped.string());
  if (HeliumOptions::Instance()->Has("create-tagfile")) {
    create_tagfile(folder, "tags");
    exit(0);
  }

  if (HeliumOptions::Instance()->Has("create-snippet-db")) {
    std::string tmpdir = utils::create_tmp_dir();
    std::string tagfile = tmpdir+"/tags";
    create_tagfile(folder, tmpdir+"/tags");
    // std::cout << "created tagfile: " << tagfile  << "\n";
    SnippetDB::Instance()->Create(tagfile, "snippets");
    exit(0);
  }
  // create_utilities(folder);


  if (HeliumOptions::Instance()->Has("show-ast")) {
    if (!utils::file_exists(folder)) {
      std::cerr << "only works for a single file.\n";
      exit(1);
    }
    XMLDoc *doc = XMLDocReader::CreateDocFromFile(folder);
    XMLNodeList func_nodes = find_nodes(*doc, NK_Function);
    for (XMLNode func : func_nodes) {
      AST ast(func);
      ast.Visualize2();
    }
    exit(0);
  }

  if (HeliumOptions::Instance()->Has("show-cfg")) {
    if (!utils::file_exists(folder)) {
      std::cerr << "only works for a single file.\n";
      exit(1);
    }
    XMLDoc *doc = XMLDocReader::CreateDocFromFile(folder);
    XMLNodeList func_nodes = find_nodes(*doc, NK_Function);
    for (XMLNode func : func_nodes) {
      AST ast(func);
      ASTNode *root = ast.GetRoot(); 
      CFG *cfg = CFGFactory::CreateCFG(root);
      cfg->Visualize();
      delete cfg;
    }
    exit(0);
  }

  

  // if (!fs::exists(cpped) || !fs::exists(src) || !fs::exists(snippets)) {
  //   std::cerr << "EE: cpped src snippets folders must exist in " << folder << "\n";
  //   std::cout << cpped << "\n";
  //   std::cout << src << "\n";
  //   std::cout << snippets << "\n";
  //   exit(1);
  // }

  if (HeliumOptions::Instance()->GetBool("print-benchmark-name")) {
    std::cout << "Benchmark Name: " << target.filename().string() << "\n";
  }

  // load_tagfile(cpped.string());
  // load_snippet_db(snippets.string());
  // load_header_resolver(src.string());
  // load_slice();

  // print_utilities(cpped.string());

  // FailurePoint *fp = load_failure_point();
  // if (!fp) {
  //   std::cerr << "EE: error loading failure point"  << "\n";
  //   exit(1);
  // }
  // Helium helium(fp);
  std::string poi_file = HeliumOptions::Instance()->GetString("poi-file");
  poi_file = utils::escape_tide(poi_file);
  // std::vector<PointOfInterest*> pois = create_point_of_interest(target.string(), poi_file);
  std::vector<PointOfInterest*> pois = POIFactory::Create(target.string(), poi_file);

  std::cout << "Number of point of interest: " << pois.size() << "\n";


  int ct=0;
  int valid_poi_ct=0;
  int valid_poi_limit=HeliumOptions::Instance()->GetInt("valid-poi-limit");
  for (PointOfInterest *poi : pois) {
    std::cerr << "Current POI: " << ct << "\n";
    ct++;
    Helium helium(poi);
    if (helium.GetStatus() == HS_Success) {
      valid_poi_ct++;
    }
    if (valid_poi_limit>0 && valid_poi_ct > valid_poi_limit) {
      std::cerr << "Reach Valid POI Limit: " << valid_poi_limit << ". Breaking ..\n";
      break;
    }
  }

  std::cout << "End of Helium" << "\n";


  return 0;
}
