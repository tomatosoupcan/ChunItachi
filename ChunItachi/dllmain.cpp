#include "framework.h"
#include <cpr/cpr.h>
#include <openssl/ssl.h>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using namespace std::chrono;
using namespace rapidxml;
using json = nlohmann::json;
using namespace std;

//intialize memory for hook storage
int addSongID = 0;
int addDifficulty = 0;
int addInSong = 0;
int miss = 0;
int attack = 0;
int justice = 0;
int jcrit = 0;
int score = 0;
int extid = 0;
int barscleared = 0;
int extidload = 0;
int testmenu = 0;
int targetBars = 0;

//initialize variables for user config
bool debug = true;
string gameversion = "";
string targeturl = "";
string apikey = "";
string apiEndpoint = "";
string apiStatus = "";
bool failOverLamp = true;

//initialize search directories vector
vector<std::string> finalSearchDirs;

//function for hooking and jumping back
BOOL HOOK(void * toHook, void * ourFunct, int len) {
	//jump call is going to be 5 long, so if what we overwrite is less than 5 we're boned
	if (len < 5) {
		return false;
	}

	//free the existing memory
	DWORD curProtection;
	VirtualProtect(toHook, len, PAGE_EXECUTE_READWRITE, &curProtection);

	//null the existing memory to NOP
	memset(toHook, 0x90, len);

	//figure out how far to where we go, minus 5 because jump is 5
	DWORD relativeAddress = ((DWORD)ourFunct - (DWORD)toHook) - 5;

	//write jump instruction
	*(BYTE*)toHook = 0xE9;
	*(DWORD*)((DWORD)toHook + 1) = relativeAddress;

	//set the existing memory location back to the previous security
	DWORD temp;
	VirtualProtect(toHook, len, curProtection, &temp);

	//return
	return true;
}

//set jump back addresses before use, create detours, 1 address for each detour to avoid issues
//song detour, sets the current song id
DWORD jmpBackAddy;
void _declspec(naked) songDetour() {
	_asm {
		mov[esi + 0x00000098], edi
		mov [addSongID], edi
		jmp [jmpBackAddy]
	}
}
//diff detour sets your current difficulty, 0 indexed
DWORD jmpBackAddy2;
void _declspec(naked) diffDetour() {
	_asm {
		mov[ebx + 0x0C], al
		mov [addDifficulty], eax
		mov eax, [esi + 10]
		jmp[jmpBackAddy2]
	}
}
//in song detour, flag whether you are in song or not, sets 1
DWORD jmpBackAddy3;
void _declspec(naked) inSongDetour() {
	_asm {
		mov[addInSong], 0x01
		jmp[jmpBackAddy3]
	}
}
//in song detour, flag whether you are in song or not, sets 0
DWORD jmpBackAddy4;
void _declspec(naked) inSongDetour2() {
	_asm {
		mov[addInSong], 0x00
		jmp[jmpBackAddy4]
	}
}
//detour bullshit
DWORD jmpBackAddy5;
void _declspec(naked) bullshitDetour() {
	_asm {
		movd[ebx + 0x58], xmm0
		movd[miss], xmm0
		movd xmm0, [ebx+0x60]
		movd[justice], xmm0
		movd xmm0, [ebx + 0x64]
		movd[jcrit], xmm0
		movd xmm0, [ebx - 0x20]
		movd[attack], xmm0
		movd xmm0, [ebx + 0xF0]
		movd[score], xmm0
		jmp[jmpBackAddy5]
	}
}
//extid detour
DWORD jmpBackAddy6;
void _declspec(naked) extidDetour() {
	_asm {
		mov[ecx + 0x48], eax
		mov[extid], eax
		mov eax, [esp + 0x08]
		jmp[jmpBackAddy6]
	}
}
//clearing detour take 4
DWORD jmpBackAddy7;
void _declspec(naked) clearingDetour() {
	_asm {
		mov[ebx + 0x00002C4], eax
		mov[barscleared], eax
		jmp[jmpBackAddy7]
	}
}
//clearing detour take 4 pt.2
DWORD jmpBackAddy8;
void _declspec(naked) clearingDetour2() {
	_asm {
		mov[esi + 0x48], 0x00
		mov[barscleared], 0x00
		jmp[jmpBackAddy8]
	}
}
//set in test menu
DWORD jmpBackAddy9;
void _declspec(naked) testmenuDetour() {
	_asm {
		mov[eax], 0x0000000
		mov[testmenu], 0x01
		call edi
		jmp[jmpBackAddy9]
	}
}
//set not in test menu
DWORD jmpBackAddy10;
void _declspec(naked) testmenuDetour2() {
	_asm {
		mov[ecx], 0x0000004
		mov[testmenu], 0x00
		jmp[jmpBackAddy10]
	}
}
//set clear bars
DWORD jmpBackAddy11;
void _declspec(naked) clearBarsDetour() {
	_asm {
		movd[ecx], xmm1
		movd[targetBars], xmm1
		movd[ecx + 0x08], xmm0
		jmp[jmpBackAddy11]
	}
}
//set clear bars crystal
DWORD jmpBackAddy12;
void _declspec(naked) clearBarsDetour2() {
	_asm {
		mov[ecx + 0x08], edi
		mov[targetBars], edi
		mov[ecx + 0x0C], ebx
		jmp[jmpBackAddy12]
	}
}

#pragma warning( disable : 6262 ) //disable compiler warning regarding memory usage
//spawn thread after startup to avoid locking application
DWORD WINAPI threadMain(LPVOID lpParam) {
	//intialize variables used in setting status
	bool inSong = 0;
	bool lastInSong = 0;
	int curDiff = 0;
	int lastDiff = 0;
	int curSong = 0;
	int lastSong = 0;
	int inSongCount = 0;
	int songMiss = 0;
	int songJustice = 0;
	int songAttack = 0;
	int songCrit = 0;
	int songScore = 0;
	int songBars = 0;
	string songLamp = "FAILED";
	//initialize strings used in setting status
	string songName;
	string dif2String;
	string genreID;
	string releaseTag;
	//check Kamai connection
	cpr::Response rk = cpr::Get(cpr::Url{ apiStatus });
	cout << "[ChunItachi] Checking connection to Kamaitachi, response code: " << rk.status_code << endl;
	
	while (true) {
		//Run every 1 second
		Sleep(1000); 
		//populate real data

		curDiff = addDifficulty;
		curSong = addSongID;

		//Logical checks to confirm you are in a song.
		if (addInSong == 1) { inSongCount = inSongCount + 1; }
		else { inSongCount = 0; }

		if (inSongCount > 6) { 
			inSong = 1; 
			songMiss = 0;
			songAttack = 0;
			songJustice = 0;
			songCrit = 0;
			songScore = 0;
			songBars = 0;
		}
		else { inSong = 0; }

		if (inSong == 1) {
			songMiss = miss;
			songAttack = attack;
			songJustice = justice;
			songCrit = jcrit;
			songScore = score;
			songBars = barscleared;
		}

		//If previous inSong was 0, and it is now 1, pull extra data
		if (inSong != lastInSong && inSong == 1) {
			//Find file here
			string xmlPath = findXML(curSong);
			//Parse XML here, elements to pull <name><str>, <artistName><str>
			//<fumens><MusicFumenData><type><data> = 0->BASIC, 1->ADVANCED, 2->EXPERT, 3->MASTER, 4->WORLD'S END, 5->TUTORIAL, trace back to
			//<fumens><MusicFumenData><level> and <fumens><MusicFumenData><levelDecimal>
			if (debug) { cout << "Parsing XML " + xmlPath + "...\n"; }
			xml_document doc;
			xml_node<>* root_node;
			ifstream theFile(xmlPath);
			vector<char> buffer((istreambuf_iterator<char>(theFile)), istreambuf_iterator<char>());
			buffer.push_back('\0');
			doc.parse<0>(&buffer[0]);
			root_node = doc.first_node("MusicData");
			stringstream stringBuf;
			//Get Song Name
			stringBuf << root_node->first_node("name")->first_node("str")->value() << endl;
			songName = stringBuf.str();
			songName.erase(songName.find_last_not_of(" \n\r\t") + 1);
			stringBuf.str("");
			stringBuf.clear();
			//Get genre id
			stringBuf << root_node->first_node("genreNames")->first_node("list")->first_node("StringID")->first_node("id")->value() << endl;
			genreID = stringBuf.str();
			genreID.erase(genreID.find_last_not_of(" \n\r\t") + 1);
			stringBuf.str("");
			stringBuf.clear();
			//Get release tag
			stringBuf << root_node->first_node("releaseTagName")->first_node("str")->value() << endl;
			releaseTag = stringBuf.str();
			releaseTag.erase(releaseTag.find_last_not_of(" \n\r\t") + 1);
			stringBuf.str("");
			stringBuf.clear();
			//Get Song Level
			switch (curDiff) {
			case 0:
				dif2String = "BASIC";
				break;
			case 1:
				dif2String = "ADVANCED";
				break;
			case 2:
				dif2String = "EXPERT";
				break;
			case 3:
				dif2String = "MASTER";
				break;
			case 4:
				dif2String = "WORLD'S END";
				break;
			case 5:
				dif2String = "TUTORIAL";
				break;
			default:
				dif2String = "ERROR";
				break;
			}
			string tempHold;
			//Output info
			if (debug) { cout << "[ChunItachi] Song Name: " << songName << "\n"; }
		}
		//Print if a notable change is found
		if (curSong != lastSong || curDiff != lastDiff || inSong != lastInSong)
			if (debug) {
				printf("[ChunItachi] Value Read:\n\tSongID: %d\n\tDifficulty: %d\n\tinSong: %d\n\ttargetBars: %d\n",
					curSong, curDiff, (int)inSong, targetBars/100);
			}

		//Here's where we'll put data upload
		if (lastInSong != inSong and inSong == 0) {
			//give the game a second or two to think, this should also help avoid accidental sends when returning
			//to test menu after I fix that shit
			Sleep(2000);
			releaseTag = releaseTag.substr(3,10);
			//Verify we are sending data from a real genre category, avoid customs, verify the user is the user defined in the config
			if ((extid == extidload or extidload == 0 or extid == 0) and testmenu == 0 and (releaseTag == "1.00.00" or releaseTag == "1.05.00" or releaseTag == "1.10.00" or releaseTag == "1.15.00" or releaseTag == "1.20.00" or releaseTag == "1.25.00" or releaseTag == "1.35.00" or releaseTag == "1.40.00" or releaseTag == "1.45.00" or releaseTag == "1.50.00" or releaseTag == "1.55.00")) {
				if (genreID == "0" or genreID == "99" or genreID == "2" or genreID == "3" or genreID == "6" or genreID == "1" or genreID == "7" or genreID == "8" or genreID == "9" or genreID == "5" or genreID == "10") {
					if (dif2String != "WORLD'S END" and dif2String != "TUTORIAL" and dif2String != "ERROR") {
						//calculate lamp
						if (songBars < targetBars/100 and failOverLamp) {
							songLamp = "FAILED";
						}
						else if (songMiss == 0 and songAttack == 0 and songJustice == 0) {
							songLamp = "ALL JUSTICE CRITICAL";
						}
						else if (songMiss == 0 and songAttack == 0) {
							songLamp = "ALL JUSTICE";
						}
						else if (songMiss == 0) {
							songLamp = "FULL COMBO";
						}
						else if (songBars < targetBars / 100) {
							songLamp = "FAILED";
						}
						else {
							songLamp = "CLEAR";
						}
						//pull the values
						printf("[ChunItachi] Uploading play to KamaItachi:\n");
						//actually send the data here
						
						json outData = {
											{"meta", 
												{
													{"service", "ChunItachi"},
													{"game", "chunithm"}
												}
											},
											{"scores", 
												{
													{
														{"score", songScore},
														{"lamp", songLamp},
														{"matchType", "title"},
														{"identifier", songName},
														{"playtype", "Single"},
														{"difficulty", dif2String},
														{"timeAchieved", duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count()},
														{"judgements",
															{
																{"miss",songMiss},
																{"attack",songAttack},
																{"justice",songJustice},
																{"jcrit",songCrit}
															}
														}
													}
												}
											}
										};
						//curl out
						/*json jsonBody = {
							{"importData", outData.dump()},
							{"serviceLoc", "DIRECT-MANUAL"},
							{"gameLoc", "chunithm"},
							{"noNotif", true},
						};*/
						cout << "[ChunItachi] Sending Data" << endl;
						cpr::Response r = cpr::Post(cpr::Url{ apiEndpoint },
							cpr::Timeout(4 * 1000),
							cpr::Header{ {"Authorization","Bearer " + apikey}, {"Content-Type", "application/json"} },
							//cpr::Body{ jsonBody.dump() });
							cpr::Body{ outData.dump() });
						cout << outData.dump() << endl;
						
						cout << "[ChunItachi] Sent, response code: " << r.status_code << endl;
						cout << "[ChunItachi] Response text: " << r.text << endl;
						//cout << r.text << endl;
						
						
					}
					else {
						cout << "[ChunItachi] Not a valid diff: " << dif2String << "\n";
					}
				}
				else {
					cout << "[ChunItachi] Not a valid genre: " << genreID << "\n";
				}
			}
			else {
				cout << "[ChunItachi] Non matching extid: " << extid << " versus " << extidload << "?\n";
				cout << "[ChunItachi] Test menu flag: " << testmenu << " Release Tag: " << releaseTag << "?\n";
			}
		}

		//populate last variable set
		lastSong = curSong;
		lastDiff = curDiff;
		lastInSong = inSong;
	}
	return true;
}

//find AXXX directories in data directories
vector<std::string> indexDirectories(string searchDir, vector<std::string> output) {
	for (auto& p : fs::directory_iterator(searchDir)) {
		try {
			string filename = p.path().string();
			string filename2 = filename.substr(filename.length() - 4, filename.length());
			if (filename2.substr(0, 1) == "A" && p.path().filename().string().length() == 4) { output.push_back(filename); }
		}
		catch (...) {}
	}
	return output;
}

#pragma warning( disable : 6031 ) //Disable compiler warning about ignoring return
//main dll entrypoint
BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD ul_reason_for_call,
	LPVOID lpReserved)
{
	//set hook variables
	int hookLength = 6;
	DWORD hookAddress = 0x0;

	//set memory offsets
	HMODULE chun = GetModuleHandle(NULL);
	intptr_t baseAddress;
	intptr_t boostAddress;
	intptr_t boostAddress2;
	intptr_t opSongAddress;
	intptr_t opDiffAddress;
	intptr_t opInSongAddress;
	intptr_t opInSongAddress2;
	intptr_t bullshitAddress;
	intptr_t extidAddress;
	intptr_t clearingAddress;
	intptr_t clearingAddress2;
	intptr_t testmenuAddress;
	intptr_t testmenuAddress2;
	intptr_t clearBarsAddress;
	DWORD threadID;
	HANDLE threadHandle;

	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		//load up ini settings
		debug = GetBooleanValue((char*)"showDebug");
		failOverLamp = GetBooleanValue((char*)"failOverLamp");

		//get the directories that contain AXXX folders, then find paths to AXXX folders and put them in a vector
		char startDirChar[MAX_PATH];
		string startDir;
		string searchDir1;
		string searchDir2;
		GetCurrentDirectoryA(MAX_PATH, startDirChar);
		startDir = startDirChar;
		int startLen = startDir.length();
		string tempDir = startDir.substr(0, startLen - 3);
		searchDir1 = tempDir + "data\\";
		//searchDir2 = tempDir + "app\\option\\";

		//ini reading, pull option directory from segatools
		CSimpleIniA ini;
		ini.LoadFile(".\\segatools.ini");
		searchDir2 = ini.GetValue("vfs", "option");
		ini.LoadFile(".\\ChunItachi.ini");
		extidload = stoi(ini.GetValue("general", "extID"));
		cout << "[ChunItachi] Read extID from config: " << extidload << "\n";
		gameversion = ini.GetValue("general", "game");
		apikey = ini.GetValue("kamaitachi", "apikey");
		apiEndpoint = ini.GetValue("kamaitachi", "apiEndpoint");
		apiStatus = ini.GetValue("kamaitachi", "apiStatus");

		if (debug) {
			AllocConsole();
			freopen("CONOUT$", "w", stdout);
		}

		//set up for felxibility later on
		if (gameversion == "amazon") {
			baseAddress = (intptr_t)chun + 0xC00;
			boostAddress = (intptr_t)chun + 0x467100;
			boostAddress2 = (intptr_t)chun + 0x72C400;
			opSongAddress = boostAddress + 0x2450C8;
			opDiffAddress = boostAddress + 0x1696AA;
			opInSongAddress = boostAddress + 0x26812F;
			opInSongAddress2 = boostAddress + 0x268138;
			bullshitAddress = boostAddress + 0x112963;
			extidAddress = (intptr_t)chun + 0x39BA34;
			clearingAddress = boostAddress + 0x1D4457;
			clearingAddress2 = boostAddress + 0x1D2F0A;
			testmenuAddress = boostAddress2 + 0x1466DB;
			testmenuAddress2 = boostAddress2 + 0x1433E8;
			clearBarsAddress = boostAddress + 0x11387B;
		}
		else if (gameversion == "amazonplus") {
			baseAddress = (intptr_t)chun + 0xC00;
			boostAddress = (intptr_t)chun + 0x472FD0;
			boostAddress2 = (intptr_t)chun + 0x74F6B0;
			opSongAddress = boostAddress + 0x25C478;
			opDiffAddress = boostAddress + 0x17A0EA;
			opInSongAddress = boostAddress + 0x27F35F;
			opInSongAddress2 = boostAddress + 0x27F368;
			bullshitAddress = boostAddress + 0x116C13;
			extidAddress = (intptr_t)chun + 0x3A5454;
			clearingAddress = boostAddress + 0x1E6CD7;
			clearingAddress2 = boostAddress + 0x1E557A;
			testmenuAddress = boostAddress2 + 0x14662B;
			testmenuAddress2 = boostAddress2 + 0x143338;
			clearBarsAddress = boostAddress + 0x117B2B;
		}
		else if (gameversion == "crystal") {
			baseAddress = (intptr_t)chun + 0xC00;
			boostAddress = (intptr_t)chun + 0x48B2D0;
			boostAddress2 = (intptr_t)chun + 0x775830;
			opSongAddress = boostAddress + 0x2695A8;
			opDiffAddress = boostAddress + 0x17EC1A;
			opInSongAddress = boostAddress + 0x28D04F;
			opInSongAddress2 = boostAddress + 0x28D058;
			bullshitAddress = boostAddress + 0x11B3A3;
			extidAddress = (intptr_t)chun + 0x3AF664;
			clearingAddress = boostAddress + 0x1F023D;
			clearingAddress2 = boostAddress + 0x1EEA0A;
			testmenuAddress = boostAddress2 + 0x14662B;
			testmenuAddress2 = boostAddress2 + 0x143338;
			clearBarsAddress = boostAddress + 0x11C335;
		}
		else if (gameversion == "paradise" or gameversion == "paradiselost") {
			baseAddress = (intptr_t)chun + 0xC00;
			boostAddress = (intptr_t)chun + 0x0;
			boostAddress2 = (intptr_t)chun + 0x0;
			opSongAddress = boostAddress + 0x72B098;
			opDiffAddress = boostAddress + 0x63D06A;
			opInSongAddress = boostAddress + 0x74F2AF;
			opInSongAddress2 = boostAddress + 0x74F2B8;
			bullshitAddress = boostAddress + 0x5D6CD3;
			extidAddress = (intptr_t)chun + 0x3C62F4;
			clearingAddress = boostAddress + 0x6B004D;
			clearingAddress2 = boostAddress + 0x6AE81A;
			testmenuAddress = boostAddress2 + 0x96B94B;
			testmenuAddress2 = boostAddress2 + 0x968658;
			clearBarsAddress = boostAddress + 0x5D7C65;
		}
		else {
			cout << "[ChunItachi] Game version is unsupported, compatability needs to be added, currently supports [amazon,amazonplus,crystal]" << endl;
			system("pause");
			return FALSE;
		}

		//detour song id
		if (debug) { printf("[ChunItachi] Detouring Song ID\n"); }
		jmpBackAddy = opSongAddress + hookLength;
		HOOK((void*)opSongAddress, songDetour, hookLength);
		//detour diff id
		if (debug) { printf("[ChunItachi] Detouring Difficulty ID\n"); }
		jmpBackAddy2 = opDiffAddress + hookLength;
		HOOK((void*)opDiffAddress, diffDetour, hookLength);
		//detour in song
		if (debug) { printf("[ChunItachi] Detouring InSong\n"); }
		jmpBackAddy3 = opInSongAddress + hookLength;
		HOOK((void*)opInSongAddress, inSongDetour, hookLength);
		jmpBackAddy4 = opInSongAddress2 + 10;
		HOOK((void*)opInSongAddress2, inSongDetour2, 10);
		//detour bullshit
		if (debug) { printf("[ChunItachi] Detouring Bullshit\n"); }
		jmpBackAddy5 = bullshitAddress + 5;
		HOOK((void*)bullshitAddress, bullshitDetour, 5);
		//detour extid
		if (debug) { printf("[ChunItachi] Detouring ExtID\n"); }
		jmpBackAddy6 = extidAddress + 7;
		HOOK((void*)extidAddress, extidDetour, 7);
		//detour clearing
		if (debug) { printf("[ChunItachi] Detouring Clearing Flag\n"); }
		jmpBackAddy7 = clearingAddress + 6;
		HOOK((void*)clearingAddress, clearingDetour, 6);
		jmpBackAddy8 = clearingAddress2 + 7;
		HOOK((void*)clearingAddress2, clearingDetour2, 7);
		//detour testmenu
		if (debug) { printf("[ChunItachi] Detouring testMenu\n"); }
		jmpBackAddy9 = testmenuAddress + 8;
		HOOK((void*)testmenuAddress, testmenuDetour, 8);
		jmpBackAddy10 = testmenuAddress2 + 6;
		HOOK((void*)testmenuAddress2, testmenuDetour2, 6);
		//detour clearbars
		if (gameversion == "crystal" or gameversion == "paradise" or gameversion == "paradiselost") {
			if (debug) { printf("[ChunItachi] Detouring clearBars\n"); }
			jmpBackAddy12 = clearBarsAddress + 6;
			HOOK((void*)clearBarsAddress, clearBarsDetour2, 6);
		}
		else {
			if (debug) { printf("[ChunItachi] Detouring clearBars\n"); }
			jmpBackAddy11 = clearBarsAddress + 9;
			HOOK((void*)clearBarsAddress, clearBarsDetour, 9);
		}
		

		if (debug) { printf("[ChunItachi] Set Search Directories\n"); }
		//need to find the AXXX folders
		finalSearchDirs = indexDirectories(searchDir1, finalSearchDirs);
		if (!searchDir2.empty())
			finalSearchDirs = indexDirectories(searchDir2, finalSearchDirs);

		//Additional debug information
		if (debug) {
			printf("[ChunItachi] ChunItachi v0.1.0 \n");
			printf("[ChunItachi] Base Address: %x\n", baseAddress);
			printf("[ChunItachi] Boost Address: %x\n", boostAddress);
			printf("[ChunItachi] Boost Address2: %x\n", boostAddress2);
			printf("[ChunItachi] Target Opcode Addresses:\n\tSong ID: %x\n\tDifficulty: %x\n\tIn Song 1: %x\n\tIn Song 2: %x\n\tBullshit: %x\n\tExtID: %x\n\tClearing 1: %x\n\tClearing 2: %x\n\tTest Menu 1: %x\n\tTest Menu 2: %x\n",
				opSongAddress, opDiffAddress, opInSongAddress, opInSongAddress2, bullshitAddress, extidAddress, clearingAddress, clearingAddress2, testmenuAddress, testmenuAddress2 );
			printf("[ChunItachi] DLL Parity Memory Addresses:\n\tSongID: %p\n\tDifficulty: %p\n\tIn Song: %p\n", &addSongID, &addDifficulty, &addInSong);
			
			

			cout << "[ChunItachi] XML Search Directories: " << endl;
			for (unsigned int i = 0; i < finalSearchDirs.size(); i++) {
				cout << "\t" << finalSearchDirs[i] << endl;
			}
			//system("pause");
		}
		//spawn thread separate from game process
		threadHandle = CreateThread
		(
			NULL,
			0,
			threadMain,
			NULL,
			0,
			&threadID
		);
	}
	return TRUE;
}