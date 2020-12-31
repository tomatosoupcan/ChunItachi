#include "filefunctions.h"
#include "framework.h"
#include <iostream>

using namespace std;
namespace fs = std::filesystem;

//finds XMLs in the setup search directories, overwrites as it finds newer files in similar fashion to Chuni
string findXML(int songID) {
	string output = "";
	string paddedSong = to_string(songID);
	paddedSong.insert(paddedSong.begin(), 4 - paddedSong.length(), '0');
	for (int i = 0; i < finalSearchDirs.size(); i++) {
		if (fs::exists(finalSearchDirs[i] + "\\music\\music" + paddedSong + "\\Music.xml")) {
			output = finalSearchDirs[i] + "\\music\\music" + paddedSong + "\\Music.xml";
		}
	}
	return output;
}
