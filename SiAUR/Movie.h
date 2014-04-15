#include "Vid.hpp"

class Movie : public VidBase {
	Movie();
public:
	static VidBase create(const std::string &str, const std::string &rar, const std::string urloc = "Movies" FILE_SEPERATOR)  {
		return VidBase(str,rar,urloc,-1,-1);
	}
	static std::string URLoc(const VidBase&);

};
