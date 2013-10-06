#include "Vid.hpp"

class Show: public VidBase {
	Show();
public:
	static VidBase create(const std::string &str, const std::string &rar,
	   	int season, int ep,
	   	std::string urloc = "Tv Shows\\")
	{
		return VidBase(str,rar,urloc,season,ep);
	}

	static std::string URLoc(const VidBase&);
private:
	//season number and ep number
	unsigned int season,ep;
};
