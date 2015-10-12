#include "Vid.hpp"
#include "Movie.h"

std::string Movie::URLoc(const VidBase& video) {
	std::stringstream temp;
	temp << video.getBaseUDLoc() << video.UDloc << video.getName() << FILE_SEPERATOR;
	return temp.str();
}
