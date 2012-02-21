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

#ifndef NDEBUG
#define WINRAR "\"C:\\Program Files\\WinRAR\\Rar.exe\""
#else
#define WINRAR "\"E:\\Program Files\\WinRAR\\Rar.exe\""
#endif

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::vector;
using std::unordered_set;
using std::tr1::regex;
using std::stringstream;
namespace regex_consts = std::tr1::regex_constants;

typedef VidBase (*RegSearchFunc)(const char* ,string&);
// Possibly use a stack?
vector<VidBase> videos;
unordered_set<string> vidType;
	
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

inline bool checkDelimiters(string& input) {
	return (vidType.find(input) != vidType.end());
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
	// return failString, unable to parse
	return failString;
}

VidBase regSearch(const char* str,string &rar) {
	string showname;
	int season,ep;
	//string of win, removes year from show name
	//std::tr1::regex strOfWin("([a-zA-Z\\._-]+)(?:20\\d\\d)?\\.s?([0-9]{1,3})(?:ep?|x)([0-9]{1,3})(?:[_\\.-])?(?:ep?|x)?(?:[0-9]{1,3})?\\.",
								//regex_consts::ECMAScript | regex_consts::icase);
	std::tr1::regex strOfWin("([a-zA-Z\\._-]+)(?:20\\d\\d)?\\.s?([0-9]{1,3})(?:ep?|x)([0-9]{1,3})(?:[\\._-]+((ep?)|x)[0-9]{1,3})?\\.",
								regex_consts::ECMAScript | regex_consts::icase);
	//string format("$1 $2 $4");
	string test;
	if (std::tr1::regex_search(str,strOfWin)) {
		test = std::tr1::regex_replace(string(str),strOfWin,
										string("$1 $2 $3")/*format*/,
										regex_consts::match_default | regex_consts::format_no_copy);
	} else {
		test = "AAA-Uknown 1 1";
	}
	std::stringstream lol(test);
	lol >> showname >> season >> ep;

	showname = std::tr1::regex_replace(showname,std::tr1::regex("[\\._-]+"),string(" "));
	removeInvalidLastChar(showname);
	return Show::create(showname,rar,season,ep);
}

VidBase movieRegSearch(const char* str, string &rar) {
	string movieName;
	std::tr1::regex strOfWin("([a-zA-Z\\d\\._-]+)\\.(19|20)\\d\\d\\.",
								regex_consts::ECMAScript | regex_consts::icase);
	if (std::tr1::regex_search(str,strOfWin)) {
		movieName = std::tr1::regex_replace(string(str),strOfWin,
											string("$1")/*format*/,
											regex_consts::match_default | regex_consts::format_no_copy);
		movieName = checkForVideoType(movieName.c_str(),movieName);
	} else {
		movieName = checkForVideoType(str,string("Unknown-Movie"));
	}
	movieName = std::tr1::regex_replace(movieName,regex("[\\._-]+"),string(" "));
	removeInvalidLastChar(movieName);
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
		if (pack && Findz.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY && (filename != "." && filename != "..")) {
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

#ifndef NDEBUG

void singleMovieTest(const char* raw,const char* name) {
	//Movie temp(movieRegSearch(raw, string()));
	VidBase temp(movieRegSearch(raw, string()));
	cout << name << " test " << ((temp == Movie::create(string(name),string())) ? "succeeded" : ("failed name = \'" + temp.getName() + "\'"));
	cout << "\n";

}

void singleShowTest(const char* raw,const char* name,int season, int ep) {
	VidBase temp(regSearch(raw,string()));
	cout << name << " test " << ((temp == Show::create(string(name),string(),season,ep)) ? "succeeded" : ("failed name = \'" + temp.getName() + "\'"));
	cout << "\n";

}
void test(int argc,char** args) {
	cout << "\n\nTv show testing\n\n\n";
	singleShowTest("Top_Gear.16x04.720p_HDTV_x264-FoV","Top Gear",16,04);
	singleShowTest("The.Simpsons.S22E13.HDTV.XviD-LOL","The Simpsons",22,13);
	singleShowTest("Star.Wars.The.Clone.Wars.2008.S03E19.720p.HDTV.x264-IMMERSE","Star Wars The Clone Wars",03,19);
	singleShowTest("\\\\DARTH-SIDIOUS\\Torrentz\\Fringe.S03E20.720p.HDTV.X264-DIMENSION","Fringe",03,20);
	singleShowTest("Doctor_Who_2005.6x01.The_Impossible_Astronaut_Part1.720p_HDTV_x264-FoV","Doctor Who",06,01);
	singleShowTest("Supernatural.s06e21-e22.720p.hdtv.x264-2hd","Supernatural",06,21);

	// Movie testing
	cout << "\n\nMovie testing\n\n\n";
	singleMovieTest("Rango.2011.EXTENDED.1080p.Bluray.x264-VeDeTT","Rango");
	singleMovieTest("Jackass.3.5.2011.1080p.BluRay.X264-7SinS", "Jackass 3 5");
	singleMovieTest("Ricky.Steamboat.The.Life.Story.of.the.Dragon.2010.DVDRip.XviD-SPRiNTER", "Ricky Steamboat The Life Story of the Dragon");
	singleMovieTest("Living.in.Emergency.Stories.of.Doctors.Without.Borders.2008.DOCU.DVDRip.XviD-SPRiNTER",
					"Living in Emergency Stories of Doctors Without Borders");
	singleMovieTest("The.Lincoln.Lawyer.DVDRip.XviD-TARGET", "The Lincoln Lawyer");
}
#endif

int copyNfo(string &loc,string &dest,const char* name) {
	// copyFile Function
	//http://msdn.microsoft.com/en-us/library/aa363851%28VS.85%29.aspx
	WIN32_FIND_DATA Findz;
	ZeroMemory( &Findz, sizeof(Findz) );
	HANDLE fred;
	ZeroMemory( &fred, sizeof(fred) );

	string nfoLoc ("\\");
	BOOL cont = TRUE;
	fred = FindFirstFile((loc+"\\*").c_str(),&Findz);
	while (fred != INVALID_HANDLE_VALUE && cont) {
		string filename(Findz.cFileName);
		if (filename.rfind(".nfo",filename.size()-1,4) != string::npos) {
			nfoLoc += filename;
			break;
		}
		cont = FindNextFile(fred,&Findz);
	}

	if (fred == INVALID_HANDLE_VALUE) 
		return -1;

#ifndef NDEBUG
	cout << loc+nfoLoc << endl;
	cout << dest+name << endl;
#endif
	if (CopyFile((loc+nfoLoc).c_str(), (dest + name + ".nfo").c_str(), TRUE))
		return 42;

	cout << GetLastError() << endl;

	return 0;
}

int callWinRar(string str) {
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	LPSTR cmd = _tcsdup(str.c_str());

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
			cerr << "CreateProcess failed " << GetLastError() << endl;
			return -1;
	}
	WaitForSingleObject( pi.hProcess,INFINITE);
	//close process and thread handles.
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return 42;
}

int singleShow(int argc,char** args,std::ofstream& log) {
	
	string rarName(rarFind(string(args[3]),false,NULL,string()));
	if (rarName==string("F")) {
		log.close();
		return 1;
	}
	VidBase blah(regSearch(args[2],rarName));
	
	std::stringstream cmd;
	cmd << WINRAR << " e -y " << "\"" << blah.RarLoc() << "\"" << " " << "\"" << Show::URLoc(blah) << "\"";
#ifndef NDEBUG
	cout << cmd.str() << endl;
#else
	if (callWinRar(cmd.str())==42) {
		cout << "Huge Success" << endl;
		log << cmd.str() << "\n" << "Huge Success" << endl;
		return 0;
	}
#endif

	return -1;
}

int singleMovie(int argc,char** args,std::ofstream& log) {

	string rarName(rarFind(string(args[3]),false,NULL,string()));
	if (rarName==string("F")) {
		log.close();
		return 1;
	}
	VidBase blah(movieRegSearch(args[2],rarName));
	
	std::stringstream cmd;
	cmd << WINRAR << " e -y " << "\"" << blah.RarLoc() << "\"" << " " << "\"" << Movie::URLoc(blah) << "\"";
#ifndef NDEBUG
	cout << cmd.str() << endl;
#else
	if (callWinRar(cmd.str())==42) {
		cout << "Huge Success" << endl;
		log << cmd.str() << "\n" << "Huge Success" << endl;
		if (copyNfo(string(args[3]),Movie::URLoc(blah),args[2]) == 42) {
			log << "Nfo copied\n";
		} else {
			log << "Unable to copy nfo\n";
		}
		return 0;
	}
#endif

	return -1;
}


int MoviePack(int argc,char** args,std::ofstream& log) {
	unsigned int status(0);
	rarFind(string(args[3]),true,movieRegSearch,string());
	vector<VidBase>::iterator iter = videos.begin();
	while (iter != videos.end()) {
		std::stringstream cmd;
		cmd << WINRAR << " e -y " << "\"" << iter->RarLoc() << "\"" << " " << "\"" << Movie::URLoc(*iter) << "\"";
#ifndef NDEBUG
		cout << cmd.str() << endl;
#else
		if (callWinRar(cmd.str())==42) {
			cout << "Huge Success\n";
			log << cmd.str() << "\n" << "Huge Success\n";
			copyNfo(iter->rarDir,Movie::URLoc(*iter),iter->sceneName.c_str());
			++status;
		} else {
			cout << "Winrar Failure";
			log << cmd.str() << "\t" << "Failed\n";
		}
#endif
		++iter;
	}

	if (status == videos.size())
		return 0;

	return -1;
}

int ShowPack(int argc,char** args,std::ofstream& log) {
	unsigned int status(0);
	videos.reserve(30);
	rarFind(string(args[3]),true,regSearch,string());
	vector<VidBase>::iterator iter = videos.begin();
	while (iter != videos.end()) {
		std::stringstream cmd;
		cmd << WINRAR << " e -y " << "\"" << iter->RarLoc() << "\"" << " " << "\"" << Show::URLoc(*iter) << "\"";
#ifndef NDEBUG
		cout << cmd.str() << endl;
#else
		if (callWinRar(cmd.str())==42) {
			cout << "Huge Success\n";
			log << cmd.str() << "\n" << "Huge Success\n";
			++status;
		} else {
			cout << "Winrar Failure";
			log << cmd.str() << "\t" << "Failed\n";
		}
#endif
		++iter;
	}
	if (status == videos.size()) {
		return 0;
	}

	return -1;
}


int main(int argc,char** args) {
	setupVidType();
#ifndef NDEBUG
	char* logName("SiUAR.log");
#else
	char* logName("E:\SiUAR.log");
#endif
	std::ofstream log(logName,std::ofstream::app | std::ofstream::out);
	if (argc < 3) {
		if (log) {
			log << "Invalid args" << endl;
			log.close();
		}
		return 1;
	}

	// Check if log creation was successful
	if (log) {
		for (int i = 1;i!=argc;++i) {
			log << args[i] << " ";
		}
		log << endl;
	} else {
		cerr << "unable to create log " << endl;
		return -1;
	}
	
#ifndef NDEBUG
	//testing code
	test(argc,args);
#endif
	int status;
	if (strcmp(args[1],"TV-RSS")==0) {
		status = singleShow(argc,args,log);
	} else if (strcmp(args[1],"MOVIE")==0) {
		status = singleMovie(argc,args,log);
	} else if (strcmp(args[1],"TV-PACK")==0) {
		status = ShowPack(argc,args,log);
	} else if (strcmp(args[1],"MOVIE-PACK")==0) {
		status = MoviePack(argc,args,log);
	} else {
			log << "Invalid args" << endl;
			log.close();
			return 1;
	}

	switch (status) {
	case 0:
		break;
	case 1:
		log << "Unable to find valid show" << endl;
		break;
	case -1:
		log << "Failure (Winrar)" << endl;
		cerr << "Winrar failed\n";
		break;

	}
	log.close();
#ifndef NDEBUG
	std::cin.get();
#endif
	return status;

}
