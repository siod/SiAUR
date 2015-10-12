#ifndef VIDBASE
#define VIDBASE
#include <string>
#include <sstream>
#include "defines.h"

class VidBase {
	friend inline bool operator==(const VidBase& rhs,const VidBase& lhs);
public:
	VidBase(const std::string &_name,
		const std::string &rar,
		const std::string &Dloc,
		int y = -1,
		int s = -1,
		int e = -1)
		:UDloc( Dloc ), rarLoc(rar),rarDir(""),name(_name),
		sceneName(""), sourceDir(""),
		year(y),season(s), ep(e) {}

	std::string getName() const { 
		if (year == -1) {
			return name;
		}
		return getNameWithYear();
	}
	std::string getNameWithYear() const;
	std::string RarLoc(VidBase& video ) const;
	std::string RarLoc() const;

	
	static const std::string& getBaseUDLoc() {
		return mBaseLoc;
	}

	static void initBaseLoc(std::string& baseLoc) {
		mBaseLoc = std::string(baseLoc);
	}

//protected:
	//base urar directory to use
	static std::string mBaseLoc;
	std::string UDloc;
	//lcoation of rar
	std::string rarLoc;
	std::string rarDir;

	//name of the show
	std::string name;
	std::string sceneName;
	std::string sourceDir;
	//season number and ep number
	int year;
	int season,ep;
};

inline bool operator==(const VidBase& rhs,const VidBase& lhs) {
	return (rhs.name == lhs.name
			&& rhs.season == lhs.season
			&& rhs.ep == lhs.ep);
}

#endif
