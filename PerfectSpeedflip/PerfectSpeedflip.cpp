#include "pch.h"
#include "PerfectSpeedflip.h"


BAKKESMOD_PLUGIN(PerfectSpeedflip, "write a plugin description here", plugin_version, PLUGINTYPE_FREEPLAY)


void PerfectSpeedflip::onLoad()
{
	cvarManager->log("Plugin loaded!");
}

void PerfectSpeedflip::onUnload()
{
}