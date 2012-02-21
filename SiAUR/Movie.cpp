#include "Vid.hpp"
#include "Movie.h"

std::string Movie::URLoc(VidBase& video) {
	std::stringstream temp;
	temp << video.UDloc << video.name << "\\";
	return temp.str();
}
