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

/*
 * Dependancies: boost filesystem, rsync
 * 
 */
 
#include <iostream>

#include "config.hpp"
#include "rctime.hpp"
#include "crawler.hpp"
#include "alert.hpp"
#include <getopt.h>

int main(int argc, char *argv[], char *envp[]){
  int opt;
  int option_ind = 0;
  
  static struct option long_options[] = {
		{"config",		   required_argument, 0, 'c'},
		{"help",         no_argument,       0, 'h'},
		{"verbose",      no_argument,       0, 'v'},
		{"quiet",        no_argument,       0, 'q'},
		{0, 0, 0, 0}
	};
  
  while((opt = getopt_long(argc, argv, "c:hvq", long_options, &option_ind)) != -1){
		switch(opt){
		case 'c':
			config_path = optarg;
			break;
		case 'h':
			usage();
			exit(EXIT_SUCCESS);
			break;
		case 'v':
			config.log_level = 2;
			break;
		case 'q':
			config.log_level = 0;
			break;
		case '?':
			break; // getopt_long prints errors
		default:
			abort();
		}
	}
  
  initDaemon();
  config.env_sz = find_env_size(envp);
  pollBase(config.sender_dir);
  return 0;
}
