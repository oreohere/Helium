#include "helium/utils/Log.h"
#include "helium/utils/Utils.h"

#include "helium/utils/HeliumOptions.h"
#include "helium/utils/FSUtils.h"
#include <iostream>

void helium_log(std::string s) {
  utils::append_file("helium_log.txt", s + "\n");
}

/**
 * dump will not dump new line!
 */
void helium_dump(std::string s) {
  utils::append_file("helium_dump.txt", s);
}

void helium_log_warning(std::string s) {
  utils::append_file("helium_log.txt", "WW: " + s + "\n");
}

void helium_log_trace(std::string s) {
  utils::append_file("helium_log.txt", "TT: " + s + "\n");
}

void helium_dump_compile_error(std::string s) {
  if (HeliumOptions::Instance()->GetBool("dump-compile-error")) {
    utils::append_file("helium_dump_compile_error.txt", s + "\n");
  }
}

void helium_print_trace(std::string s) {
  if (HeliumOptions::Instance()->GetBool("print-trace")) {
    std::cout << "[trace] " << s  << "\n";
  }
}

void helium_print_warning(std::string s) {
  if (HeliumOptions::Instance()->GetBool("print-warning")) {
    std::cerr << "[Warning] " << s  << "\n";
  }
}
