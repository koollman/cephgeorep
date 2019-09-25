#pragma once

#include <vector>
#include <boost/filesystem.hpp>

#define SSH_FAIL 255
#define NOT_INSTALLED 127
#define SUCCESS 0
#define PERM_DENY 23

namespace fs = boost::filesystem;

void launch_rsync(std::vector<fs::path> queue);
// fork and exec rsync with flags and pass queue
