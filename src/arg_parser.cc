#include "arg_parser.h"
#include <iostream>

#include <gtest/gtest.h>

ArgParser::ArgParser(int argc, char* argv[])
{
  // create options_description
  po::options_description options("Arguments");
  // add options to it
  options.add_options()
    ("help,h", "produce help message") // --help, -h
    ("config,f", po::value<std::string>(), "config file")
    ("tagfile,c", po::value<std::string>(), "tag file")
    ("verbose,v", "verbose output")
    ("output,o", po::value<std::string>(), "output location")
    ("conf", po::value<std::string>(), "key=value pair of configure to owerwrite items in helium.conf")
    ;

  po::options_description util_options("Utils");
  util_options.add_options()
    ("create-tagfile", "create tag file")
    ("create-srcml", "create xml from C source file")
    ("print-config", "print current config")
    ("print-segments", "print segments and exit")
    ("print-segment-info", "print segment count, segment size LOC in total, for every file")
    ;
    

  po::options_description hidden("Hidden options");
  hidden.add_options()
    ("folder", "project folder")
    // config options. Should be kept the same as in helium.conf file
    // ("analyze-timeout", po::value<std::string>())
    // ("analyze-type", po::value<std::string>())
    // ("code-selection", po::value<std::string>())
    // ("context-search-method", po::value<std::string>())
    // ("context-search-value", po::value<std::string>())
    // ("instrument-position", po::value<std::string>())
    // ("instrument-type", po::value<std::string>())
    // ("max-context-size", po::value<std::string>())
    // ("max-segment-size", po::value<std::string>())
    // ("max-snippet-number", po::value<std::string>())
    // ("max-snippet-size", po::value<std::string>())
    // ("output-folder", po::value<std::string>())
    // ("segment-timeout", po::value<std::string>())
    // ("test-generation-method", po::value<std::string>())
    // ("run-test", po::value<std::string>())
    // ("test-number", po::value<std::string>())
    // ("test-timeout", po::value<std::string>())
    ;
  // positional options: used for the option that don't need to use a --prefix
  po::positional_options_description positional;
  // this "folder" option include only one item
  positional.add("folder", 1);

  // Further group
  // all options avaliable for command line options
  m_cmdline_options
    .add(options)
    .add(util_options)
    .add(hidden)
    ;

  // this is the message that will show for help messages
  m_help_options
    .add(options)
    .add(util_options)
    ;

  /* run parser and store in m_vm */
  po::store(
            po::command_line_parser(argc, argv)
            .options(m_cmdline_options) // add cmdline options
            .positional(positional)     // add positional options
            .run(),                     // run the parser
            // store into m_vm
            m_vm
            );
  po::notify(m_vm);

  // /* validate the options */
  // if (!validate()) {
  //   PrintHelp();
  //   exit(1);
  // }
}

ArgParser::~ArgParser() {}

void ArgParser::PrintHelp() {
  std::cout<< "Usage: helium [options] <folder>" <<std::endl;
  std::cout<< m_help_options << std::endl;
}

bool ArgParser::Has(std::string name) {
  if (m_vm.count(name)) {
    return true;
  } else {
    return false;
  }
}

/*
 * Get content from vm
 * m_vm["folder"].as<std::string>();
 * m_vm["include-files"].as< std::vector<std::string> > ();
 */
std::string ArgParser::GetString(std::string name) {
  if (Has(name)) {
    return m_vm[name].as<std::string>();
  } else {
    return "";
  }
}

// bool ArgParser::validate() {
//   if (Has("folder")) return true;
//   else return false;
// }

/**
 * commented out to suppress warning.
 */
// TEST(arg_parser_test_case, DISABLED_arg_parser) {
//   int argc = 3;
//   char *argv[] =
//     {"helium", "--context-search-method=hello-world", "folder"};
//   ArgParser args(argc, argv);
//   EXPECT_FALSE(args.Has("context-search-value"));
//   EXPECT_TRUE(args.Has("context-search-method"));
//   EXPECT_EQ(args.GetString("context-search-method"), "hello-world");
// }
