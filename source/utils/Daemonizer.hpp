#pragma once
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <fstream>
#include "StackTraceExcept.hpp"

namespace mylib{
namespace utils{

/**
 * @brief Daemonize current process. it will make new process with 0 ppid.
 * 
 * @return true if the result process of this function is daemon process(ppid is 0).
 * @return false if it doesn't. The current process should be closed.
 * 
 * @throw StackTraceExcept with errno when failed.
 */
bool Daemonizer();
/**
 * @brief Create or Rewrite Pid file.
 * 
 * @param path target file path.
 * 
 * @throw StackTraceExcept with errno when failed to open or create file.
 */
void Create_PidFile(std::string path);

}
}