#include "Vid.hpp"
#include "Show.h"

std::string Show::URLoc(VidBase& show) {
	std::stringstream temp;
	temp << show.UDloc << show.name << "\\" << show.name << " Season " << show.season << "\\";
	return temp.str();
}

