#include "Vid.hpp"
#include "Show.h"
#include "Movie.h"

std::string VidBase::mBaseLoc("");
std::string VidBase::RarLoc() const {
	return std::string(rarLoc);
}

std::string VidBase::RarLoc(VidBase& video ) const {
	return std::string(video.rarLoc);
}
