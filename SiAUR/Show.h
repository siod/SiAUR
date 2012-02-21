#include "Vid.hpp"

class Show: public VidBase {
public:
	static VidBase create(std::string &str, std::string &rar,
	   	int season, int ep,
	   	std::string urloc = "Tv Shows\\")
	{
		return VidBase(str,rar,urloc,season,ep);
	}

	static std::string URLoc(VidBase&);
private:
	//season number and ep number
	unsigned int season,ep;
};
