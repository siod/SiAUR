#ifndef VIDBASE
#define VIDBASE
#include <string>
#include <sstream>

class VidBase {
	friend inline bool operator==(const VidBase& rhs,const VidBase& lhs);
public:
	VidBase(const std::string &_name, const std::string &rar, const std::string &Dloc,int s = -1, int e = -1)
		:UDloc( Dloc ), rarLoc(rar),rarDir(""),name(_name),sceneName(""), season(s), ep(e) {}

	const std::string& getName() const { return name; }

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
	//season number and ep number
	int season,ep;
};

inline bool operator==(const VidBase& rhs,const VidBase& lhs) {
	return (rhs.name == lhs.name
			&& rhs.season == lhs.season
			&& rhs.ep == lhs.ep);
}

#endif
