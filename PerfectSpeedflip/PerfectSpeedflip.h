#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"

constexpr auto plugin_version = "1.0";

class PerfectSpeedflip: public BakkesMod::Plugin::BakkesModPlugin
{

	//Boilerplate
	virtual void onLoad();
	virtual void onUnload();

	void hookEvents();
	void unhookEvents();

	void startKickoff();
	void hitBall();

	// renders plugin
	void render(CanvasWrapper canvas);
};

