#include <iostream>
#include <fstream>
#include <cstring>
#ifdef __linux__
#include <boost/regex.hpp>
#else
#include <regex>
#endif
#include <sstream>
#include <vector>
#include <unordered_set>
#include <stdlib.h>
#include "Vid.hpp"
#include "Show.h"
#include "Movie.h"

#include "../libs/include/SiConf.h"
#include "../libs/include/SiLog.h"

#ifdef _WIN32
#pragma comment(lib, "SiConf.lib")
#pragma comment(lib, "SiLog.lib")
#endif

#ifndef Log
#define Log Logging::Log
#define LogLine Logging::LogLine
#endif

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::wstring;
using std::vector;
using std::unordered_set;
using std::stringstream;

#ifdef _WIN32
using std::tr1::regex;
using std::smatch;
namespace regex_consts = std::tr1::regex_constants;
#elif defined(__linux__)
using boost::regex;
using boost::smatch;
namespace regex_consts = boost::regex_constants;
#else
using std::regex;
using std::smatch;
namespace regex_consts = std::regex_constants;
#endif


typedef void (*ProcessFunc)(const string& ,const string& ,const string&);

enum JOB_TYPE {
	TV,
	TV_PACK,
	MOVIE,
	MOVIE_PACK,
	INVALID };

// Possibly use a stack?
vector<VidBase> videos;
unordered_set<string> vidType;
ConfReader config;
section_map* globalsMap(NULL);
section_map* showsMap(NULL);
section_map* moviesMap(NULL);
regex showRegex;
regex showDailyRegex;
regex movieRegex;

//OS  defines
bool callUnRar(const string& loc,const string& dest);
bool copyNfo(string dir, const string &dest,const char* name);
bool findFiles(string dir, bool pack,void (*process)(const string&,const string&) ,
			   const string& extension= "");
void setupBannedFolders();
void setUnRarLoc(const string& loc);
// End windows defines

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
	vidType.insert("EXTENDED");
	vidType.insert("LIMITED");
	vidType.insert("REMASTERED");

}

inline bool checkDelimiters(string& input) {
	return (vidType.find(input) != vidType.end());
}

SiString stringToLower(const SiString& input) {
	SiString lowercase(input);
	for (size_t i = 0; i !=  lowercase.length();i++) {
		lowercase[i] = char(tolower(lowercase[i]));
	}
	return lowercase;
}

string convert_name(string& original_name,section_map& conversions) {
	SiString lowerName(stringToLower(original_name));

	if (conversions.empty() || conversions.count(lowerName) == 0) {
		// No conversions to perform
		return original_name;
	}
	return conversions[lowerName];
}

string checkForVideoType(const char* str,const string& failString) {
	LogLine("Fallback video decoding: '" + string(str) + "'",Logging::LOG_DEBUG);
	string name(str);
	std::vector<string> tokens;
	size_t sPos(0);
	size_t ePos(0);
	if (name[sPos] == FILE_SEPERATOR[0])
		++sPos;
	while ((ePos = name.find(FILE_SEPERATOR,sPos)) != string::npos) {
		tokens.push_back(name.substr(sPos,ePos - sPos));
		sPos = ePos +1;
	}
	if (tokens.size() == 0) {
		tokens.push_back(name);
	}

	for (int i(tokens.size()-1); i >= 0; --i) {
		string buffer;
		buffer.reserve(tokens[i].size());
		string decodedName("");
		for (int j(0),len(tokens[i].size());j < len;++j) {
			if (tokens[i][j] == '.') {
				if (checkDelimiters(buffer)) {
					return decodedName.erase(decodedName.size()-1);
				}
				decodedName += buffer + " ";
				buffer.clear();
			} else {
				buffer += tokens[i][j];
			}
		}
	}
	// unable to parse
	return failString;
}

VidBase regSearch(const char* encoded_name,const string &rar) {
	string showname;
	int season,ep;
	//regex string that decodes tv show name and removes year from show name
	// "([a-z0-9\._-]+)\.s?([0-9]{1,3})(?:ep?|x)([0-9]{1,3})(?:[\._-]*((ep?)|x)[0-9]{1,3})?(\.)"
	string season_s;
	string episode_s;

	string str(encoded_name);
	smatch m;
	if (regex_search(str,m,showRegex) && m.size() >= 4) {
		showname = m[1];
		season_s = m[2];
		episode_s = m[3];
		//If this was a double ep, the 2nd ep's number would be in m[4]
	} else if (regex_search(str,m,showDailyRegex) && m.size() >= 4) {
		showname = m[1];
		//Map season to year?
		season_s = m[2];
		//Episode isn't currently used in determining unrar location
		episode_s = "1";
	} else {
		//Fallback 
		return Show::create("AAA Unkown",rar,1,1);
	}
	season = atoi(season_s.c_str());
	ep = atoi(episode_s.c_str());

	showname = regex_replace(showname,regex("[\\._-]+"),string(" "));
	showname = convert_name(showname,*showsMap);
	return Show::create(showname,rar,season,ep);
}

VidBase movieRegSearch(const char* encoded_name, const string &rar) {
	string movieName;
	int year(-1);
	string s_year;
	string str(encoded_name);
	// default "([a-z\d\._-]+\.)((19|20)\d\d)(\.)"
	smatch m;
	if (regex_search(str,m,movieRegex) && m.size() >= 3) {
		movieName = m[1];
		s_year = m[2];
	} else {
		movieName = checkForVideoType(str.c_str(),string("000 Unknown Movie"));
	}
	year = atoi(s_year.c_str());
	movieName = regex_replace(movieName,regex("[\\._-]+"),string(" "));
	while (movieName[movieName.size() -1] == ' ') {
		movieName.erase(movieName.size() -1);
	}
	movieName = convert_name(movieName,*moviesMap);
	return Movie::create(movieName,year,rar);
}

bool extractShow(VidBase show) {
	if (callUnRar(show.RarLoc(),Show::URLoc(show))) {
		LogLine(string("SUCCESS : " + show.name + "\t from " + show.RarLoc() + "\t to " + Show::URLoc(show)),Logging::LOG_INFO);
		return true;
	} else {
		LogLine(string("FAILURE : " + show.name + "\t from " + show.RarLoc() + "\t to " + Show::URLoc(show)),Logging::LOG_INFO);
		return false;
	}
}

int singleShow(const char* dir) {
	
	if (!findFiles(string(dir),false,
		[](const string& rardir,const string& rar) {
			VidBase temp(regSearch(rardir.c_str(),rar));
			temp.rarDir = rardir;
			temp.sceneName = "";
			videos.push_back(temp);
	})) {
		LogLine("FAILURE : unable to find files",Logging::LOG_INFO);
		return 1;
	}
	if (extractShow(videos[0]))
		return 0;
	else
		return -1;
}

int ShowPack(const char* dir) {
	unsigned int status(0);
	videos.reserve(30);
	findFiles(string(dir),true,
		[](const string& rardir,const string& rar) {
			VidBase temp(regSearch(rardir.c_str(),rar));
			temp.rarDir = rardir;
			temp.sceneName = "";
			videos.push_back(temp);
		});
	vector<VidBase>::iterator iter = videos.begin();
	while (iter != videos.end()) {
		if (extractShow(*iter))
			++status;
		++iter;
	}
	return (status == videos.size()) ? 0 : -1;
}

bool extractMovie(const VidBase& movie,const char* sourceDir,string sceneName) {
	if (callUnRar(movie.RarLoc(),Movie::URLoc(movie))) {
		LogLine("SUCCESS : " + movie.name + "\t from " + movie.RarLoc() + 
				"\t to " + Movie::URLoc(movie),Logging::LOG_INFO);
			if (copyNfo(sourceDir,Movie::URLoc(movie),sceneName.c_str())) {
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

	if (!findFiles(string(dir),false,
		[](const string& rardir,const string& rar) {
			VidBase temp(movieRegSearch(rardir.c_str(),rar));
			temp.rarDir = rardir;
			temp.sceneName = "";
			videos.push_back(temp);
	})) {
		LogLine("FAILURE : unable to find files",Logging::LOG_INFO);
		return 1;
	}
	if (extractMovie(videos[0],dir,string(name)))
		return 0;
	else
		return -1;
}


int MoviePack(const char* dir) {
	unsigned int status(0);
	findFiles(string(dir),true,
		[](const string& rardir,const string& rar) {
			VidBase temp(movieRegSearch(rardir.c_str(),rar));
			temp.rarDir = rardir;
			string sourceDir;
			size_t nameStart = rardir.rfind(temp.name.substr(0,temp.name.find(" ")));
			size_t nameEnd = rardir.find(FILE_SEPERATOR,nameStart);
			if (nameStart != string::npos && nameEnd != string::npos) {
				temp.sceneName = string(rardir,nameStart,nameEnd - nameStart);
				temp.sourceDir = string(rardir,0,nameEnd +1);
#ifndef NDEBUG
				std::cout << temp.sceneName << "\n";
				std::cout << temp.sourceDir << "\n";
#endif
			} else {
				temp.sceneName = "";
				temp.sourceDir = "";
			}
			videos.push_back(temp);
	});

	vector<VidBase>::iterator iter = videos.begin();
	while (iter != videos.end()) {
		const char* sourceDir = (iter->sourceDir != "") ? iter->sourceDir.c_str() : dir ;
		if (extractMovie(*iter,sourceDir,iter->sceneName))
			++status;
		++iter;
	}

	return (status == videos.size()) ? 0 : -1;
}


JOB_TYPE getJobType(const char* sJobType) {
	if (strcmp(sJobType,"TV")==0) {
		return TV;
	} 
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

bool setup(const string& iniName) {
	if (!config.init(iniName.c_str(),false)) {
		cerr << iniName << " is not a valid config file" << endl;
		return false;
	}
	setupVidType();
	setupBannedFolders();
	globalsMap = config.get_section("globals");
	if (!globalsMap || globalsMap->empty()) {
		cerr << "config file " << iniName << " empty" << endl;
		return false;
	}
	showsMap = config.safe_get_section("show_conversions");
	moviesMap = config.safe_get_section("movie_conversions");

	VidBase::initBaseLoc((*globalsMap)["base_loc"]);
	string raw = (*globalsMap)["show_regex"];
	if (raw != "") {
		showRegex = regex(raw, regex_consts::ECMAScript | regex_consts::icase);
	}
	raw = (*globalsMap)["show_daily_regex"];
	if (raw != "") {
		showDailyRegex = regex(raw, regex_consts::ECMAScript | regex_consts::icase);
	}
	raw = (*globalsMap)["movie_regex"];
	if (raw != "") {
		movieRegex = regex(raw, regex_consts::ECMAScript | regex_consts::icase);
	}
	setUnRarLoc((*globalsMap)["unrar"]);

	return true;
}

#ifndef NDEBUG
 bool test();
#endif
int main(int argc,char** args) {
#ifndef NDEBUG
	std::string iniName("SiAUR.ini");
#else
	std::string iniName(string(getenv("HOME")) + "/.SiAUR.config");
#endif
	if (!setup(iniName))
		return -1;

#ifndef NDEBUG
	Logging::init(Logging::LOG_DEBUG,true,true,(*globalsMap)["log_name"]);
#else
	Logging::init(Logging::LOG_DEBUG,false,true,(*globalsMap)["log_name"]);
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

#ifndef NDEBUG
	//testing code
	if (!test())
		return 0;
#endif

	if (argc < 3) {
		LogLine("Use: <JobType> <name> <location>",Logging::LOG_ERROR);
		Logging::destroy();
		return 1;
	}

	int status = -1;
	JOB_TYPE jobType = getJobType(args[1]);
	switch (jobType) {
		case TV:
			status = singleShow(args[3]);
			break;
		case TV_PACK:
			status = ShowPack(args[3]);
			break;
		case MOVIE:
			status = singleMovie(args[3],args[2]);
			break;
		case MOVIE_PACK:
			status = MoviePack(args[3]);
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
