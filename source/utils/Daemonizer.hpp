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

bool Daemonizer();
void Create_PidFile(std::string);

}
}