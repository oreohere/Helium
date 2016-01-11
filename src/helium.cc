#include "helium.h"
#include <string>
#include <iostream>
#include "arg_parser.h"
#include "snippet.h"
#include "resolver.h"
#include "config.h"
#include "utils.h"
#include "reader.h"

using namespace utils;

/**
 * read env variable HELIUM_HOME
 * @return std::string home folder without trailing '/'
 */
std::string
load_helium_home() {
  /* read HELIUM_HOME env variable */
  const char *helium_home_env = std::getenv("HELIUM_HOME");
  if(!helium_home_env) {
    std::cerr<<"Please set env variable HELIUM_HOME"<<std::endl;
    exit(1);
  }
  std::string helium_home = helium_home_env;
  while (helium_home.back() == '/') helium_home.pop_back();
  return helium_home;
}

static void
create_tagfile(const std::string& folder, const std::string& file) {
  std::string cmd = "ctags -f ";
  cmd += file;
  cmd += " --languages=c,c++ -n --c-kinds=+x --exclude=heium_result -R ";
  cmd += folder;
  std::system(cmd.c_str());
}


Helium::Helium(int argc, char* argv[]) {
  /* load HELIUM_HOME */
  std::string helium_home = load_helium_home();
  /* parse arguments */
  ArgParser args(argc, argv);



  /* load config */
  Config::Instance()->ParseFile(helium_home+"/helium.conf");
  Config::Instance()->Overwrite(args);

  if (args.Has("print-config")) {
    std::cout << Config::Instance()->ToString() << "\n";
    exit(0);
  }

  /*******************************
   ** BEGIN need folder argument
   *******************************/
  
  // target folder
  m_folder = args.GetString("folder");
  if (m_folder.empty()) {
    args.PrintHelp();
    exit(1);
  }
  while (m_folder.back() == '/') m_folder.pop_back();




  /*******************************
   ** utilities
   *******************************/
  if (args.Has("create-tagfile")) {
    if (args.Has("output")) {
      std::string output_file = args.GetString("output");
      if (output_file.empty()) output_file = "tags";
      create_tagfile(m_folder, output_file);
    }
    exit(0);
  }



  /*******************************
   ** Helium start
   *******************************/

  
  /* load tag file */
  std::string tagfile = args.GetString("tagfile");
  if (tagfile.empty()) {
    // ctags_load(m_folder + "/tags");
    // create tagfile
    std::cout << "creating tag file ..."  << "\n";
    create_tagfile(m_folder, "/tmp/helium.tags");
    std::cout << "done"  << "\n";
    ctags_load("/tmp/helium.tags");
  } else {
    ctags_load(tagfile);
  }

  /* load system tag file */
  SystemResolver::Instance()->Load(helium_home + "/systype.tags");
  HeaderSorter::Instance()->Load(m_folder);

  
  std::string output_folder = Config::Instance()->GetString("output-folder");
  assert(!output_folder.empty() && "output-folder is not set");
  /* prepare folder */
  remove_folder(output_folder);
  create_folder(output_folder);

  /* get files in target folder */
  get_files_by_extension(m_folder, m_files, "c");

  /*******************************
   ** More advanced utils(needs to run some functionality of Helium)
   *******************************/

  if (args.Has("print-segments")) {
    for (auto it=m_files.begin();it!=m_files.end();it++) {
      Reader reader(*it);
      reader.SelectSegments();
      std::cout << "Segment count: " << reader.GetSegmentCount() << "\n";
      reader.PrintSegments();
    }
    exit(0);
  }
  if (args.Has("print-segment-info")) {
    for (auto it=m_files.begin();it!=m_files.end();it++) {
      Reader reader(*it);
      reader.SelectSegments();
      std::cout << "Segment count: " << reader.GetSegmentCount() << "\n";
      std::cout << "segment size: " << reader.GetSegmentLOC() << "\n";
    }
    exit(0);
  }
}
Helium::~Helium() {}

void
Helium::Run() {
  for (auto it=m_files.begin();it!=m_files.end();it++) {
    Reader reader(*it);
    reader.SelectSegments();
    reader.Read();
  }
}


