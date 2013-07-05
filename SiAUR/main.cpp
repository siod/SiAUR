#include <iostream>
#include <fstream>
#include <cstring>
#include <regex>
#include <sstream>
#include <Windows.h>
#include <tchar.h>
#include <vector>
#include <unordered_set>
#include "Vid.hpp"
#include "Show.h"
#include "Movie.h"

#include "../../SiLib/SiConf/reader.h"
#include "../../SiLib/SiLog/logging.h"
#pragma comment(lib, "SiConf.lib")
#pragma comment(lib, "SiLog.lib")
//#define DRY_TESTING

#define Log Logging::Log
#define LogLine Logging::LogLine

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::wstring;
using std::vector;
using std::unordered_set;
using std::tr1::regex;
using std::stringstream;
namespace regex_consts = std::tr1::regex_constants;

typedef VidBase (*RegSearchFunc)(const char* ,string&);

enum JOB_TYPE {
	TV,
	TV_PACK,
	MOVIE,
	MOVIE_PACK,
	INVALID };

// Possibly use a stack?
vector<VidBase> videos;
unordered_set<string> vidType;
unordered_set<string> bannedFolders;
ConfReader config;
section_map& globalsMap = section_map();
section_map& showsMap = section_map();
section_map& moviesMap = section_map();
regex showRegex;
regex movieRegex;

std::string winrar;

inline void removeInvalidLastChar(string &str) {
	//HACK remove '.' || '-' that's left on some shows
	//TODO FIX REGEX
	if (str[str.size()-1] == ' ')
		str.erase(str.size()-1);
}

void setupVidType() {
	vidType.insert("PROPER");
	vidType.insert("RERiP");
	vidType.insert("1080p");
	vidType.insert("720p");
	vidType.insert("BDRip");
	vidType.insert("DVDRip");
	vidType.insert("NSTC");
	vidType.insert("PAL");
	vidType.insert("UNRATED");
	vidType.insert("INTERNAL");

}

void setupBannedFolders() {
	bannedFolders.insert("Sample");
	bannedFolders.insert("sample");

}

inline bool checkDelimiters(string& input) {
	return (vidType.find(input) != vidType.end());
}

inline bool isBannedFolder(string& input) {
	return ( input[0] == '.' || bannedFolders.find(input) != bannedFolders.end());
}



SiString stringToLower(const SiString& input) {
	SiString lowercase(input);
	for (size_t i = 0; i !=  lowercase.length();i++) {
		lowercase[i] = tolower(lowercase[i]);
	}
	return lowercase;
}

string convert_name(const string& original_name,section_map conversions) {
	SiString lowerName(stringToLower(original_name));

	if (conversions.empty() || conversions.count(lowerName) == 0) {
		// No conversions to perform
		return lowerName;
	}
	return conversions[lowerName];
}

string checkForVideoType(const char* str,string& failString) {
	string tokens;
	tokens.reserve(255);
	string buffer;
	stringstream lol(str);
	char nextchar;

	while (lol.good()) {
		lol >> nextchar;
		switch (nextchar) {
			case '.':
				if (checkDelimiters(buffer))
					return tokens.erase(tokens.size()-1);
				tokens += buffer + " ";
				buffer.clear();
				break;

			case '/':
			case '\\':
			case ':':
				buffer.clear();
				tokens.clear();
				break;

			default:
				buffer +=nextchar;
				break;
		}
	}
	// unable to parse
	return failString;
}

VidBase regSearch(const char* str,string &rar) {
	string showname;
	int season,ep;
	//regex string that decodes tv show name and removes year from show name
	//  "([a-zA-Z\\._-]+)(?:20\\d\\d)?\\.s?([0-9]{1,3})(?:ep?|x)([0-9]{1,3})(?:[_\\.-])?(?:ep?|x)?(?:[0-9]{1,3})?\\."
	// retired in favor of 
	// "([a-zA-Z0-9\\._-]+)\\.s?([0-9]{1,3})(?:ep?|x)([0-9]{1,3})(?:[\\._-]+((ep?)|x)[0-9]{1,3})?\\."

	//regex strOfWin("([a-zA-Z0-9\\._-]+)\\.s?([0-9]{1,3})(?:ep?|x)([0-9]{1,3})(?:[\\._-]+((ep?)|x)[0-9]{1,3})?\\.",
								//regex_consts::ECMAScript | regex_consts::icase);
	//string format("$1 $2 $4");
	string decoded_showinfo;
	if (regex_search(str,showRegex)) {
		decoded_showinfo = regex_replace(string(str),showRegex,
										string("$1 $2 $3")/*format*/,
										regex_consts::match_default | regex_consts::format_no_copy);
	} else {
		decoded_showinfo = "AAA-Uknown 1 1";
	}
	std::stringstream lol(decoded_showinfo);
	lol >> showname >> season >> ep;

	showname = regex_replace(showname,regex("[\\._-]+"),string(" "));
	removeInvalidLastChar(showname);
	showname = convert_name(showname,showsMap);
	return Show::create(showname,rar,season,ep);
}

VidBase movieRegSearch(const char* str, string &rar) {
	string movieName;
	//regex strOfWin("([a-zA-Z\\d\\._-]+)\\.(19|20)\\d\\d\\.",
								//regex_consts::ECMAScript | regex_consts::icase);
	if (regex_search(str,movieRegex)) {
		movieName = regex_replace(string(str),movieRegex,
											string("$1")/*format*/,
											regex_consts::match_default | regex_consts::format_no_copy);
		movieName = checkForVideoType(movieName.c_str(),movieName);
	} else {
		movieName = checkForVideoType(str,string("Unknown-Movie"));
	}
	movieName = regex_replace(movieName,regex("[\\._-]+"),string(" "));
	removeInvalidLastChar(movieName);
	movieName = convert_name(movieName,moviesMap);
	return Movie::create(movieName,rar);
}

void MultiProcessor(RegSearchFunc process,string& dir,string& rar,string& dirName) {
	VidBase temp(process( dir.c_str(), rar));
	temp.rarDir = dir;
	temp.sceneName = dirName;
	videos.push_back(temp);
}

std::string rarFind(string &dir, bool pack,RegSearchFunc process,string& dirName) {
	WIN32_FIND_DATA Findz;
	ZeroMemory( &Findz, sizeof(Findz) );
	HANDLE fred;
	ZeroMemory( &fred, sizeof(fred) );

	dir += "\\";
#ifndef NDEBUG
	cout << dir << endl;
#endif
	fred = FindFirstFileEx((dir+"*").c_str(),FindExInfoStandard,&Findz,FindExSearchNameMatch,NULL,0);
	BOOL cont(TRUE);
	bool foundRar(false);
	string filename,foundRarN;

	while (fred != INVALID_HANDLE_VALUE && cont)  {
		filename = Findz.cFileName;

		// deal with packs
		if (pack && Findz.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY && !isBannedFolder(filename)) {
			rarFind(dir+filename,true,process,filename);
		}
		

		if (filename.rfind(".rar",filename.size()-1,4) == string::npos) {
			cont = FindNextFile(fred,&Findz);
			continue;
		} else if (filename.rfind("part01.rar",filename.size()-1,10) != string::npos) {
#ifndef NDEBUG
			cout << Findz.cFileName << endl;
#endif
			if (pack) {
				MultiProcessor(process,dir,dir+filename,dirName);
				return string();
			}
			return dir + filename;
		} else {
			foundRar = true;
			foundRarN = filename;
			cont = FindNextFile(fred,&Findz);
		}
	} 

	if (foundRar) {
		//only for packs
		if (pack) {
				MultiProcessor(process,dir,dir+foundRarN,dirName);
				return string();
		}
		return dir + foundRarN;
	}
	std::cerr << "rar not found " << endl;
	return string("F");
}

bool copyNfo(const string &dir, const string &dest,const char* name) {
	// copyFile Function
	//http://msdn.microsoft.com/en-us/library/aa363851%28VS.85%29.aspx

#ifdef DRY_TESTING
	return true;
#endif
	WIN32_FIND_DATA Findz;
	ZeroMemory( &Findz, sizeof(Findz) );
	HANDLE fred;
	ZeroMemory( &fred, sizeof(fred) );

	string nfoDir ("\\");
	BOOL cont = TRUE;
	fred = FindFirstFile((dir+"\\*").c_str(),&Findz);
	while (fred != INVALID_HANDLE_VALUE && cont) {
		string filename(Findz.cFileName);
		if (filename.rfind(".nfo",filename.size()-1,4) != string::npos) {
			nfoDir += filename;
			break;
		}
		cont = FindNextFile(fred,&Findz);
	}

	if (fred == INVALID_HANDLE_VALUE) 
		return false;

#ifndef NDEBUG
	cout << dir+nfoDir << endl;
	cout << dest+name << endl;
#endif
	if (CopyFile((dir+nfoDir).c_str(), (dest + name + ".nfo").c_str(), TRUE))
		return true;


	DWORD error(GetLastError());
	if (error == 0x50) {
		LogLine("File already exists",Logging::LOG_INFO);
		return true;
	} else {
		LogLine("Unable to copy nfo error code: " +
				std::to_string(static_cast<_ULonglong>(error)),Logging::LOG_ERROR);
	}

	return false;
}

bool callWinRar(const string& loc,const string& dest) {
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	string comString(winrar + " e -y \"" + loc + "\" \"" + dest + "\"");
	LPSTR cmd = _tcsdup(comString.c_str());

#ifndef NDEBUG
	cout << comString << endl;
#endif

#ifdef DRY_TESTING
	return true;
#endif

	ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );

	if (!CreateProcess(NULL,
		cmd,
		NULL,
		NULL,
		FALSE,
		0,
		NULL,
		NULL,
		&si,
		&pi )) {
			LogLine("Unable to create winrar process error code: " +
					std::to_string(static_cast<_ULonglong>(GetLastError()))
					,Logging::LOG_ERROR);
			return false;
	}
	WaitForSingleObject( pi.hProcess,INFINITE);
	DWORD exitCode (254);
	/*	Valid return codes from winrar
	0	Successful
	1	Warning
	2	fatal error
	3	CRC error
	4	Attempt to modify locked archive
	5	Write error
	6	File open error
	7	Wrong commandline options
	8	Not enough memory
	9	File create error
	255 User Break



	*/
	GetExitCodeProcess(pi.hProcess,&exitCode);
	if (exitCode != 0) {
		LogLine(string("Winrar error :" + std::to_string(static_cast<_ULonglong>(exitCode)))
			,Logging::LOG_ERROR);
	}
	//close process and thread handles.
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return (exitCode == 0);
}

bool extractShow(VidBase show) {
	if (callWinRar(show.RarLoc(),Show::URLoc(show))) {
		LogLine(string("SUCCESS : " + show.name + "\t from " + show.RarLoc() + "\t to " + Show::URLoc(show)),Logging::LOG_INFO);
		return true;
	} else {
		LogLine(string("FAILURE : " + show.name + "\t from " + show.RarLoc() + "\t to " + Show::URLoc(show)),Logging::LOG_INFO);
		return false;
	}
}

int singleShow(const char* dir,const char* name) {
	
	string rarName(rarFind(string(dir),false,NULL,string()));
	if (rarName==string("F")) {
		return 1;
	}
	VidBase blah(regSearch(name,rarName));
	if (extractShow(blah))
		return 0;
	else
		return -1;
}

int ShowPack(const char* dir,const char* name) {
	unsigned int status(0);
	videos.reserve(30);
	rarFind(string(dir),true,regSearch,string());
	vector<VidBase>::iterator iter = videos.begin();
	while (iter != videos.end()) {
		if (extractShow(*iter))
			++status;
		++iter;
	}
	return (status == videos.size()) ? 0 : -1;
}

bool extractMovie(const VidBase& movie,const char* sourceDir,const char* sceneName) {
	if (callWinRar(movie.RarLoc(),Movie::URLoc(movie))) {
		LogLine("SUCCESS : " + movie.name + "\t from " + movie.RarLoc() + 
				"\t to " + Movie::URLoc(movie),Logging::LOG_INFO);
			if (copyNfo(sourceDir,Movie::URLoc(movie),sceneName)) {
				LogLine("SUCCESS : nfo copied",Logging::LOG_INFO);
			} else {
				LogLine("FAILURE : nfo not copied",Logging::LOG_INFO);
			}
		return true;
	} else {
		LogLine("FAILURE : " + movie.name + "\t from " + movie.RarLoc() + 
				"\t to " + Movie::URLoc(movie),Logging::LOG_INFO);
		return false;
	}
}

int singleMovie(const char* dir,const char* name) {

	string rarName(rarFind(string(dir),false,NULL,string()));
	if (rarName==string("F")) {
		return 1;
	}
	VidBase blah(movieRegSearch(name,rarName));
	if (extractMovie(blah,dir,name))
		return 0;
	else
		return -1;
}


int MoviePack(const char* dir,const char* name) {
	unsigned int status(0);
	rarFind(string(dir),true,movieRegSearch,string());
	vector<VidBase>::iterator iter = videos.begin();
	while (iter != videos.end()) {
		if (extractMovie(*iter,dir,iter->sceneName.c_str()))
			++status;
		++iter;
	}

	return (status == videos.size()) ? 0 : -1;
}


JOB_TYPE getJobType(const char* sJobType) {
	if (strcmp(sJobType,"TV-RSS")==0) {
		return TV;
	} 
	if (strcmp(sJobType,"MOVIE")==0) {
		return MOVIE;
	}
	if (strcmp(sJobType,"TV-PACK")==0) {
		return TV_PACK;
	}
	if (strcmp(sJobType,"MOVIE-PACK")==0) {
		return MOVIE_PACK;
	}

	return INVALID;
}

bool setup(const char* iniName) {
	if (!config.init(iniName))
		return false;
	setupVidType();
	setupBannedFolders();
	globalsMap = *config.safe_get_section("globals");
	if (globalsMap.empty()) {
		cerr << "no config file" << endl;
		return false;
	}
	showsMap = *config.safe_get_section("show_conversions");
	moviesMap = *config.safe_get_section("movie_conversions");

	VidBase::initBaseLoc(globalsMap["base_loc"]);
	showRegex = regex(globalsMap["show_regex"], regex_consts::ECMAScript | regex_consts::icase);
	movieRegex = regex(globalsMap["movie_regex"], regex_consts::ECMAScript | regex_consts::icase);
	winrar = globalsMap["winrar"];

	return true;
}

#ifndef NDEBUG
 bool test();
#endif
int main(int argc,char** args) {
#ifndef NDEBUG
	const char* iniName("SiAUR.ini");
#else
	const char* iniName("E:\\SiAUR.ini");
#endif
	if (!setup(iniName))
		return -1;

#ifndef NDEBUG
	Logging::init(Logging::LOG_DEBUG,true,true,globalsMap["log_name"]);
#else
	Logging::init(Logging::LOG_DEBUG,false,true,globalsMap["log_name"]);
#endif

	// Check if log creation was successful
	if (!Logging::good()) {
		cerr << "ERROR: Unable to create log " << endl;
		return -1;
	}

	for (int i = 1;i!=argc;++i) {
		Log(args[i],Logging::LOG_INFO);
		Log(" ",Logging::LOG_INFO);
	}
	Log("\n",Logging::LOG_INFO);

	if (argc < 3) {
		LogLine("Use: <JobType> <name> <location>",Logging::LOG_ERROR);
		Logging::destroy();
		return 1;
	}

	
#ifndef NDEBUG
	//testing code
	if (!test())
		return 0;
#endif

	int status = -1;
	JOB_TYPE jobType = getJobType(args[1]);
	switch (jobType) {
		case TV:
			status = singleShow(args[3],args[2]);
			break;
		case TV_PACK:
			status = ShowPack(args[3],args[2]);
			break;
		case MOVIE:
			status = singleMovie(args[3],args[2]);
			break;
		case MOVIE_PACK:
			status = MoviePack(args[3],args[2]);
			break;

		case INVALID:
			LogLine("Invalid job type",Logging::LOG_ERROR);
			Logging::destroy();
			return 1;

		default:
			break;
	}
	Logging::destroy();
#ifndef NDEBUG
	std::cin.get();
#endif
	return status;

}
