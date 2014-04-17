#include <Windows.h>
#include "../../SiLib/SiConf/reader.h"
#include "../../SiLib/SiLog/logging.h"
#include <unordered_set>
#include <tchar.h>

#ifndef Log
#define Log Logging::Log
#define LogLine Logging::LogLine
#endif

using std::unordered_set;
using std::string;

unordered_set<string> bannedFolders;
std::string WINRAR_LOC;
const string WINRAR_ERRORS[] = { "Succesful",
	"Warning",
	"fatal error",
	"CRC error",
	"Attempt to modify locked archive",
	"Write error",
	"File open error",
	"Wrong commandline options",
	"Not enough memory",
	"File create error",
	"User Break" };

const string winrar_getErrorText(int errorCode) {
	if (errorCode == 255) errorCode = 10;
	return WINRAR_ERRORS[errorCode];
}

void setUnRarLoc(const string& loc) {
	WINRAR_LOC = loc;
}

void setupBannedFolders() {
	bannedFolders.insert("Sample");
	bannedFolders.insert("sample");

}

inline bool isBannedFolder(string& input) {
	return ( input[0] == '.' || bannedFolders.find(input) != bannedFolders.end());
}

bool callUnRar(const string& loc,const string& dest) {
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	string comString(WINRAR_LOC + " e -y \"" + loc + "\" \"" + dest + "\"");
	LPSTR cmd = _tcsdup(comString.c_str());

#ifndef NDEBUG
	std::cout << comString << std::endl;
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
		LogLine(string("Winrar error :" + winrar_getErrorText(exitCode))
			,Logging::LOG_ERROR);
	}
	//close process and thread handles.
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return (exitCode == 0);
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
	std::cout << dir+nfoDir << std::endl;
	std::cout << dest+name << std::endl;
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


bool findFiles(string dir, bool pack,void (*process)(const string&,const string&) ,
			   const string& extension= "") {
	WIN32_FIND_DATA Findz;
	ZeroMemory( &Findz, sizeof(Findz) );
	HANDLE fred;
	ZeroMemory( &fred, sizeof(fred) );

	dir += "\\";
#ifndef NDEBUG
	std::cout << dir << std::endl;
#endif
	fred = FindFirstFileEx((dir+"*").c_str(),FindExInfoStandard,&Findz,FindExSearchNameMatch,NULL,0);
	BOOL cont(TRUE);
	bool found(false);
	string filename,foundRarN;

	while (fred != INVALID_HANDLE_VALUE && cont)  {
		filename = Findz.cFileName;
#ifndef NDEBUG
		std::cout << filename << std::endl;
#endif
		// deal with packs
		if (pack && Findz.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY && !isBannedFolder(filename)) {
			findFiles(dir+filename,true,process,extension,filename);
		}
		

		if (filename.rfind(".rar",filename.size()-1,4) == string::npos) {
			cont = FindNextFile(fred,&Findz);
			continue;
		} else if (filename.rfind("part01.rar",filename.size()-1,10) != string::npos) {
#ifndef NDEBUG
			std::cout << Findz.cFileName << std::endl;
#endif
			process(dir,dir+filename);
			return true;
		} else {
#ifndef NDEBUG
			std::cout << "Possible Match: " << filename << std::endl;
#endif
			if (filename.rfind("subs.rar",filename.size()-1,8) == string::npos) {
#ifndef NDEBUG
			std::cout << "Match: " << filename << std::endl;
#endif
				found = true;
				foundRarN = filename;
			}
			cont = FindNextFile(fred,&Findz);
		}
	} 

	if (found) {
		process(dir,dir+foundRarN);
		return true;
	}
	return false;
}
