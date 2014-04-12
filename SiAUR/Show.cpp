#include "Vid.hpp"
#include "Show.h"


std::string Show::URLoc(const VidBase& show) {
	std::stringstream temp;
	temp << show.getBaseUDLoc() << show.UDloc << show.name << FILE_SEPERATOR << show.name << " Season " << show.season << FILE_SEPERATOR;
	return temp.str();
}

