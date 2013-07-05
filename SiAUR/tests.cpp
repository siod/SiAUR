
#include <iostream>
#include "Vid.hpp"
#include "Show.h"
#include "Movie.h"

using std::cout;
using std::string;
using std::cin;

VidBase movieRegSearch(const char* str, string &rar);
VidBase regSearch(const char* str,string &rar);

#ifndef NDEBUG

void singleMovieTest(const char* raw,const char* name) {
	VidBase temp(movieRegSearch(raw, string()));
	cout << name << " test " << ((temp == Movie::create(string(name),string())) ? "succeeded" : ("failed name = \'" + temp.getName() + "\'"));
	cout << "\n";

}

void singleShowTest(const char* raw,const char* name,int season, int ep) {
	VidBase temp(regSearch(raw,string()));
	cout << name << " test " << ((temp == Show::create(string(name),string(),season,ep)) ? "succeeded" : ("failed name = \'" + temp.getName() + "\'"));
	cout << "\n";

}
bool test() {
	cout << "\n\nTv show testing\n\n\n";
	singleShowTest("Top_Gear.16x04.720p_HDTV_x264-FoV","Top Gear",16,4);
	singleShowTest("The.Simpsons.S22E13.HDTV.XviD-LOL","The Simpsons",22,13);
	singleShowTest("Star.Wars.The.Clone.Wars.S03E19.720p.HDTV.x264-IMMERSE","Star Wars The Clone Wars",3,19);
	singleShowTest("\\\\DARTH-SIDIOUS\\Torrentz\\Fringe.S03E20.720p.HDTV.X264-DIMENSION","Fringe",3,20);
	singleShowTest("Doctor_Who_2005.6x01.The_Impossible_Astronaut_Part1.720p_HDTV_x264-FoV","Doctor Who 2005",6,1);
	singleShowTest("Supernatural.s06e21-e22.720p.hdtv.x264-2hd","Supernatural",6,21);
	singleShowTest("Burn.Notice.S06E17E18.720p.HDTV.x264-IMMERSE","Burn Notice",6,17);

	cout << "rename testing\n";
	singleShowTest("Dr_Who_2005.6x01.The_Impossible_Astronaut_Part1.720p_HDTV_x264-FoV","Doctor Who 2005",6,1);
	singleShowTest("Adventure.Time.with.Finn.and.Jake.S05E09.720p.HDTV.x264-2HD","Adventure Time",5,9);

	// Movie testing
	cout << "\n\nMovie testing\n\n\n";
	singleMovieTest("Rango.2011.EXTENDED.1080p.Bluray.x264-VeDeTT","Rango 2011");
	singleMovieTest("Jackass.3.5.2011.1080p.BluRay.X264-7SinS", "Jackass 3 5 2011");
	singleMovieTest("Ricky.Steamboat.The.Life.Story.of.the.Dragon.2010.DVDRip.XviD-SPRiNTER", "Ricky Steamboat The Life Story of the Dragon 2010");
	singleMovieTest("Living.in.Emergency.Stories.of.Doctors.Without.Borders.2008.DOCU.DVDRip.XviD-SPRiNTER",
					"Living in Emergency Stories of Doctors Without Borders 2008");
	singleMovieTest("The.Lincoln.Lawyer.DVDRip.XviD-TARGET", "The Lincoln Lawyer");


	cout << "continue?";
	char answer;
	answer = cin.get();
	cin.ignore();
	return (answer == 'y' ||
			answer == 'Y');
}
#endif
