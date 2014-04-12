#include <unordered_set>
#include <string>
#include <dirent.h>
#include <iostream>
#include <algorithm>
using std::unordered_set;
using std::string;
#ifndef FILE_SEPERATOR
#define FILE_SEPERATOR '/'
#endif

static string UNRAR_LOC;

bool callUnRar(const string& loc,const string& dest) {
	string cmd(string(UNRAR_LOC + " e \"" + loc + "\" \"" + dest + "\" "));

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

bool copyNfo(const string &dir, const string &dest,const char* name) {
	return false;
}

bool findFiles(string dir, bool pack,void (*process)(const string&,const string&,const string&) ,
			   const string& extension= "",const string& dirName = "") {
	DIR * dp;
	struct dirent *dirp;
	if ((dp = opendir(dir.c_str())) == NULL) {
		return false;
	}

	if (dir[dir.size()-1] != (char)FILE_SEPERATOR)
		dir += FILE_SEPERATOR;

	bool found(false);
	string filename,foundFilename;

	while ((dirp = readdir(dp)) != NULL) {
		filename = dirp->d_name;
		if (pack && (dirp->d_type == DT_DIR) && !isBannedFolder(filename)) {
			findFiles(dir + filename,pack,process,extension,filename);
		}

		if (filename.rfind(".rar",filename.size()-1,4) == string::npos) {
			continue;
		} else if (filename.rfind("part01.rar",filename.size()-1,10) != string::npos) {
#ifndef NDEBUG
			std::cout << filename << std::endl;
#endif
			process(dir,dir+filename,dirName);
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
		process(dir,dir+foundFilename,dirName);
		return true;
	}
	return false;
}

void setupBannedFolders() {
}

void setUnRarLoc(const string& loc) {
	UNRAR_LOC = loc;
}
