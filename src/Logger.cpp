#include "Logger.hpp"
#include <iostream>
#include "Config.hpp"
#include <boost/filesystem.hpp>
#include <cstdio>

namespace fs = boost::filesystem;

Logger* Logger::m_instance = 0;

FILE*
get_logger(const std::string& prefix, const std::string& filename, int fd, const char* mode) {
  if (filename.empty()) {
    return fdopen(fd, "w");
  } else {
    std::string full_path = prefix + "/" + filename;
    fs::path p(full_path);
    fs::path dir = p.parent_path();
    if (!fs::exists(dir)) fs::create_directories(dir);
    FILE *fp = fopen(full_path.c_str(), mode);
    if (!fp) {
      perror("fopen");
      exit(1);
    }
    return fp;
  }
}

Logger::Logger() {
  m_log_folder = Config::Instance()->GetTmpFolder() + "/log";
  m_default_logger = get_logger(m_log_folder, Config::Instance()->GetOutputDefault(), 1, "w");
  m_debug_logger = get_logger(m_log_folder, Config::Instance()->GetOutputDebug(), 1, "w");
  m_trace_logger = get_logger(m_log_folder, Config::Instance()->GetOutputTrace(), 1, "w");
  m_compile_logger = get_logger(m_log_folder, Config::Instance()->GetOutputCompile(), 2, "w");
  m_data_logger = get_logger(m_log_folder, Config::Instance()->GetOutputData(), 1, "w");
  m_rate_logger = get_logger(m_log_folder, Config::Instance()->GetOutputRate(), 1, "w");
}

void log(const char* s, FILE *fp) {
  fputs(s, fp);
  fflush(fp);
}


void
Logger::Log(const std::string& content) {
  log(content.c_str(), m_default_logger);
}

void
Logger::LogTrace(const std::string& content) {
  log(content.c_str(), m_trace_logger);
}

void
Logger::LogCompile(const std::string& content) {
  log(content.c_str(), m_compile_logger);
}

void
Logger::LogData(const std::string& content) {
  log(content.c_str(), m_data_logger);
}
void
Logger::LogRate(const std::string& content) {
  log(content.c_str(), m_rate_logger);
}

void Logger::LogDebug(const std::string& content) {
  log(content.c_str(), m_debug_logger);
}
