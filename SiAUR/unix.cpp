#include <unordered_set>
#include <string>
#include <dirent.h>
#include <iostream>
#include <algorithm>
#include <fcntl.h>
#include <unistd.h>
#include "defines.h"

#define BUFSIZE 4096

using std::unordered_set;
using std::string;

static string UNRAR_LOC;

bool callUnRar(const string& loc,const string& dest) {
	string cmd(string(UNRAR_LOC + " e -y \"" + loc + "\" \"" + dest + "\" "));

#ifndef NDEBUG
			std::cout << cmd << std::endl;
#endif
	int status = system(cmd.c_str());
	return (status == 0);
}

inline bool isBannedFolder(string& input) {
	return (input[0] == '.' ||
		   	input == "sample" ||
		   	input == "Sample"
		);
}

bool copyNfo(string dir, const string &dest,const char* name) {
	DIR * dp;
	struct dirent *dirp;
	if ((dp = opendir(dir.c_str())) == NULL) {
#ifndef NDEBUG
		std::cout << "\nUnable to open dir\n";
#endif
		return false;
	}

	if (dir[dir.size()-1] != FILE_SEPERATOR[0])
		dir += FILE_SEPERATOR;

	string filename,nfo;
	while ((dirp = readdir(dp)) != NULL) {
		filename = dirp->d_name;
		if (filename.rfind(".nfo",filename.size()-1,4) != string::npos) {
			nfo = dir + filename;
			break;
		}
	}
	if (dirp == NULL) {
#ifndef NDEBUG
		std::cout << "Unable to find nfo";
#endif
		return false;
	}
#ifndef NDEBUG
	std::cout << nfo << "\n";
	std::cout << dest << name <<  ".nfo\n";
#endif


	int read_fd(open(nfo.c_str(),O_RDONLY));
	int write_fd(open((dest + name + ".nfo").c_str(),O_WRONLY | O_CREAT,0644));

	char buf[BUFSIZE];
	size_t size;

	while ((size = read(read_fd,buf,BUFSIZE)) > 0) {
		write(write_fd,buf,size);
	}
	//sendfile(write_fd,read_fd,&offset,stat_buf.st_size);

	close(read_fd);
	close(write_fd);
	return true;
}

bool findFiles(string dir, bool pack,void (*process)(const string&,const string&) ,
			   const string& extension= "") {
	DIR * dp;
	struct dirent *dirp;
	if ((dp = opendir(dir.c_str())) == NULL) {
		return false;
	}

	if (dir[dir.size()-1] != FILE_SEPERATOR[0])
		dir += FILE_SEPERATOR;

	bool found(false);
	string filename,foundFilename;

	while ((dirp = readdir(dp)) != NULL) {
		filename = dirp->d_name;
		if (pack && (dirp->d_type == DT_DIR) && !isBannedFolder(filename)) {
			findFiles(dir + filename,pack,process,extension);
		}

		if (filename.rfind(".rar",filename.size()-1,4) == string::npos) {
			continue;
		} else if (filename.rfind("part01.rar",filename.size()-1,10) != string::npos) {
#ifndef NDEBUG
			std::cout << filename << std::endl;
#endif
			process(dir,dir+filename);
			return true;
		} else {
			if (filename.rfind("subs.rar",filename.size()-1,8) == string::npos) {
#ifndef NDEBUG
			std::cout << "Match: " << filename << std::endl;
#endif
				found = true;
				foundFilename = filename;
			}
		}
	}

	if (found) {
		process(dir,dir+foundFilename);
		return true;
	}
	return false;
}

void setupBannedFolders() {
}

void setUnRarLoc(const string& loc) {
	UNRAR_LOC = loc;
}
