#include "config.h"
#include <simpleini.h>

CSimpleIniA configIni;

bool GetBooleanValue(char* setting)
{
	configIni.LoadFile(".\\ChunItachi.ini");
	return configIni.GetBoolValue("general", setting);
}