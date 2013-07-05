#include "Vid.hpp"
#include "Movie.h"

std::string Movie::URLoc(const VidBase& video) {
	std::stringstream temp;
	temp << video.getBaseUDLoc() << video.UDloc << video.name << "\\";
	return temp.str();
}
