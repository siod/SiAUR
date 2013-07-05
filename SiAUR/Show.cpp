#include "Vid.hpp"
#include "Show.h"

std::string Show::URLoc(const VidBase& show) {
	std::stringstream temp;
	temp << show.getBaseUDLoc() << show.UDloc << show.name << "\\" << show.name << " Season " << show.season << "\\";
	return temp.str();
}

