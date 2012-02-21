#include "Vid.hpp"
#include "Show.h"
#include "Movie.h"

std::string VidBase::RarLoc() {
	return std::string(rarLoc);
}

std::string VidBase::RarLoc(VidBase& video ) {
	return std::string(video.rarLoc);
}
