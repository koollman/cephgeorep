/*
    Copyright (C) 2019-2020 Joshua Boudreau
    
    This file is part of cephgeorep.

    cephgeorep is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    cephgeorep is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with cephgeorep.  If not, see <https://www.gnu.org/licenses/>.
*/

//#define DEBUG_WS

#include "config.hpp"
#include "alert.hpp"
#include <boost/filesystem.hpp>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>

namespace fs = boost::filesystem;

Config config; // global config struct

std::string config_path;

void loadConfig(void){
  std::string line, key, value;
  
  if(config_path.empty()){
    config_path = DEFAULT_CONFIG_PATH;
    if(config.log_level != 0)
      std::cout << "Using default config path: " << config_path << std::endl;
  }
  
  // open file
  fs::path configPath(config_path);
  std::fstream configFile(configPath.c_str());
  if(!configFile) createConfig(configPath, configFile);
  
  // read file
  while(configFile){
    getline(configFile, line);
    // full line comments:
    if(line.empty() || line.front() == '#')
      continue; // ignore comments
    
    std::stringstream linestream(line);
    getline(linestream, key, '=');
    getline(linestream, value);
    
    strip_whitespace(key);
    strip_whitespace(value);
    
    if(key == "SND_SYNC_DIR"){
      config.sender_dir = fs::path(value);
    }else if(key == "REMOTE_USER"){
      config.remote_user = value;
    }else if(key == "RECV_SYNC_HOST"){
      config.receiver_host = value;
    }else if(key == "RECV_SYNC_DIR"){
      config.receiver_dir = fs::path(value);
    }else if(key == "LAST_RCTIME_DIR"){
      config.last_rctime = fs::path(value).append(LAST_RCTIME_NAME);
    }else if(key == "SYNC_FREQ"){
      try{
        config.sync_frequency = stoi(value);
      }catch(std::invalid_argument){
        config.sync_frequency = -1;
      }
    }else if(key == "IGNORE_HIDDEN"){
      std::istringstream(value) >> std::boolalpha >> config.ignore_hidden >> std::noboolalpha;
    }else if(key == "IGNORE_WIN_LOCK"){
      std::istringstream(value) >> std::boolalpha >> config.ignore_win_lock >> std::noboolalpha;
    }else if(key == "RCTIME_PROP_DELAY"){
      try{
        config.prop_delay_ms = stoi(value);
      }catch(std::invalid_argument){
        config.prop_delay_ms = -1;
      }
    }else if(key == "LOG_LEVEL"){
      if(config.log_level == -1){ // not already set
        try{
          config.log_level = stoi(value);
        }catch(std::invalid_argument){
          config.log_level = -1;
        }
      }
    }else if(key == "EXEC"){
      config.execBin = value;
    }else if(key == "FLAGS"){
      config.execFlags = value;
    }// else ignore entry
  }
  verifyConfig();
  construct_rsync_remote_dest();
}

void strip_whitespace(std::string &str){
#ifdef DEBUG_WS
  std::cout << "Input: \"" << str << "\"" << std::endl;
#endif
  std::size_t strItr;
  // back ws
  if((strItr = str.find('#')) == std::string::npos){ // strItr point to '#' or end
    strItr = str.length();
  }
  strItr--; // point to last character
  while(strItr && (str.at(strItr) == ' ' || str.at(strItr) == '\t')){ // remove whitespace
    strItr--;
  } // strItr points to last character
  str = str.substr(0,strItr + 1);
  // front ws
  strItr = 0;
  while(strItr < str.length() && (str.at(strItr) == ' ' || str.at(strItr) == '\t')){ // remove whitespace
    strItr++;
  } // strItr points to first character
  str = str.substr(strItr, str.length() - strItr);
#ifdef DEBUG_WS
  std::cout << "Output: \"" << str << "\"" << std::endl;
#endif
}
  
void verifyConfig(){
  bool errors = false;
  if(config.sender_dir.empty()){
    std::cerr << "Config does not contain a directory to search (SND_SYNC_DIR)\n";
    errors = true;
  }
  if(config.receiver_host.empty()){
    std::cerr << "Warning: config does not contain a remote host address (RECV_SYNC_HOST)\n";
    // just warning
  }
  if(config.receiver_dir.empty()){
    std::cerr << "Warning: config does not contain a remote host directory (RECV_SYNC_DIR)\n";
    // just warning
  }
  if(config.last_rctime.empty()){
    std::cerr << "Config does not contain a path to store last timestamp (LAST_RCTIME_DIR)\n";
    errors = true;
  }
  if(config.sync_frequency < 0){
    std::cerr << "Config sync frequency must be positive integer (SYNC_FREQ)\n";
    errors = true;
  }
  if(config.prop_delay_ms < 0){
    std::cerr << "Config rctime prop delay must be positive integer (RCTIME_PROP_DELAY)\n";
    errors = true;
  }
  if(config.log_level < 0){
    std::cerr << "Config log level must be positive integer (LOG_LEVEL)\n";
    errors = true;
  }
  if(config.execBin.empty()){
    std::cerr << "Config must contain program to execute! (EXEC)\n";
    errors = true;
  }
  if(config.execBin.empty()){
    std::cerr << "Warning: no execution flags present in config. (FLAGS)\n";
    // just warning
  }
  if(errors){
    std::cerr << "Please fix these mistakes in " << config_path << "." << std::endl;
    exit(1);
  }
}

void construct_rsync_remote_dest(){
  std::string str = config.receiver_dir.string();
  if(!config.receiver_host.empty()){
    str = config.receiver_host + ":" + str;
  }
  if(!config.remote_user.empty()){
    str = config.remote_user + "@" + str;
  }
  config.sync_remote_dest = new char[str.length()+1];
  std::strcpy(config.sync_remote_dest, str.c_str());
}

void createConfig(const fs::path &configPath, std::fstream &configFile){
  fs::create_directories(configPath.parent_path(), ec);
  if(ec) error(PATH_CREATE, ec);
  std::ofstream f(configPath.c_str());
  if(!f) error(OPEN_CONFIG);
  f <<
    "# local backup settings\n"
    "SND_SYNC_DIR=               # full path to directory to backup\n"
    "IGNORE_HIDDEN=false         # ignore files beginning with \".\"\n"
    "IGNORE_WIN_LOCK=true        # ignore files beginning with \"~$\"\n"
    "\n"
    "# remote settings\n"
    "REMOTE_USER=                # user on remote backup machine (optional)\n"
    "RECV_SYNC_HOST=             # remote backup machine address/host\n"
    "RECV_SYNC_DIR=              # directory in remote backup\n"
    "\n"
    "# daemon settings\n"
    "EXEC=rsync                  # program to use for syncing - rsync or scp\n"
    "FLAGS=-a --relative         # execution flags for above program (space delim)\n"
    "LAST_RCTIME_DIR=/var/lib/ceph/cephfssync/\n"
    "SYNC_FREQ=10                # time in seconds between checks for changes\n"
    "RCTIME_PROP_DELAY=100       # time in milliseconds between snapshot and sync\n"
    "LOG_LEVEL=1\n"
    "# 0 = minimum logging\n"
    "# 1 = basic logging\n"
    "# 2 = debug logging\n"
    "# If no remote user is specified, the daemon will sync remotely as root user.\n"
    "# Propagation delay is to account for the limit that Ceph can\n"
    "# propagate the modification time of a file all the way back to\n"
    "# the root of the sync directory.\n"
    "# Only use compression (-z) if your network connection to your\n"
    "# backup server is slow.\n";
  f.close();
  configFile.open(configPath.c_str()); // seek to beginning of file for input
  if(!configFile) error(OPEN_CONFIG);
}

void dumpConfig(void){
  std::cout << "configuration:" << std::endl;
  std::cout << "SND_SYNC_DIR=" << config.sender_dir.string() << std::endl;
  std::cout << "REMOTE_USER=" << config.remote_user << std::endl;
  std::cout << "REMOTE_HOST=" << config.receiver_host << std::endl;
  std::cout << "RECV_SYNC_HOST=" << config.receiver_host << std::endl;
  std::cout << "RECV_SYNC_DIR=" << config.receiver_dir.string() << std::endl;
  std::cout << "EXEC=" << config.execBin << std::endl;
  std::cout << "FLAGS=" << config.execFlags << std::endl;
  std::cout << "LAST_RCTIME_DIR=" << config.last_rctime.string() << std::endl;
  std::cout << "SYNC_FREQ=" << config.sync_frequency << std::endl;
  std::cout << "IGNORE_HIDDEN=" << std::boolalpha << config.ignore_hidden << std::endl;
  std::cout << "IGNORE_WIN_LOCK=" << std::boolalpha << config.ignore_win_lock << std::endl;
  std::cout << "RCTIME_PROP_DELAY=" << config.prop_delay_ms << std::endl;
  std::cout << "LOG_LEVEL=" << config.log_level << std::endl;
  std::cout << "rsync will sync to: " << config.sync_remote_dest << std::endl;
}

size_t find_env_size(char *envp[]){
  size_t size = 0;
  while(*envp){
    size += strlen(*envp++) + 1;
    size += sizeof(char *);
  }
  return size;
}
