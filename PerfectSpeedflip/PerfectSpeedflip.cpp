#include "pch.h"
#include "PerfectSpeedflip.h"


BAKKESMOD_PLUGIN(PerfectSpeedflip, "Trains the perfect speedflip kickoff", plugin_version, PLUGINTYPE_FREEPLAY)

int tick = 0;
int lastKickoffTime = 0;
int averageKickoffTime = 0.0;
int kickoffs = 0;
float xLocation;
float yLocation;
LinearColor overlayColor;
bool enabledOverlay;
std::string enabledVarName = "kickoff_timer_enable";
int spawnChoice = 0;
Vector spawnLocations[] = {
	Vector(-2048, -2560, 19),
	Vector(2048, -2560, 19),
	Vector(-256, -3840, 19),
	Vector(256, -3840, 19),
	Vector(0, -4608, 19)
};

Rotator spawnRotations[] = {
	Rotator(0, 0.25 * CONST_PI_F, 0),
	Rotator(0, 0.75 * CONST_PI_F, 0),
	Rotator(0, 0.5 * CONST_PI_F, 0),
	Rotator(0, 0.5 * CONST_PI_F, 0),
	Rotator(0, 0.5 * CONST_PI_F, 0)
};

void PerfectSpeedflip::onLoad()
{
	cvarManager->log("Plugin loaded!");

	cvarManager->registerCvar(enabledVarName, "0", "enables speedflip trainer", true, true, 0, true, 1)
		.addOnValueChanged([this](std::string, CVarWrapper cvar) {
			if (cvar.getBoolValue()) {
				hookEvents();
			} else {
				unhookEvents();
			}});

	// enables timer overlay
	auto overlayEnableVar = cvarManager->registerCvar("kickoff_timer_enable_overlay",
		"0", "enable timer overlay",
		true, true, 0, true, 1);
	enabledOverlay = overlayEnableVar.getBoolValue();
	overlayEnableVar.addOnValueChanged([this](std::string, CVarWrapper cvar) {
		enabledOverlay = cvar.getBoolValue();
		});

	// sets cvar to move counter's X location
	auto xLocVar = cvarManager->registerCvar("kickoff_timer_x_location",
		"0", "set location of timer X in % of screen",
		true, true, 0.0, true, 1.0);
	xLocation = xLocVar.getFloatValue();
	xLocVar.addOnValueChanged([this](std::string, CVarWrapper cvar) {
		xLocation = cvar.getFloatValue();
		});

	// sets cvar to move counter's Y location
	auto yLocVar = cvarManager->registerCvar("kickoff_timer_y_location",
		"0", "set location of timer Y in % of screen",
		true, true, 0.0, true, 1.0);
	yLocation = yLocVar.getFloatValue();
	yLocVar.addOnValueChanged([this](std::string, CVarWrapper cvar) {
		yLocation = cvar.getFloatValue();
		});

	// chooses overlay's color
	auto colorVar = cvarManager->registerCvar("kickoff_timer_color", "#FFFFFF", "color of overlay");
	overlayColor = colorVar.getColorValue();
	colorVar.addOnValueChanged([this](std::string, CVarWrapper cvar) {
		overlayColor = cvar.getColorValue();
		});

	cvarManager->registerNotifier("kickoff_timer_reset_timer",
		[this](std::vector<std::string> params) {
			lastKickoffTime = 0;
			averageKickoffTime = 0;
			kickoffs = 0;
		}, "resets kickoff timer stats", PERMISSION_ALL);

	cvarManager->registerNotifier("kickoff_timer_score",
		[this](std::vector<std::string> params) {
			if (gameWrapper->IsInGame() && !gameWrapper->IsInOnlineGame()) {
				auto server = gameWrapper->GetGameEventAsServer();
				if (server.IsNull()) return;
				auto ball = server.GetBall();
				ball.SetLocation(Vector(0, 5520, 100));
				gameWrapper->UnhookEventPost("Function TAGame.Car_TA.SetVehicleInput");
				gameWrapper->UnhookEventPost("Function TAGame.Ball_TA.OnCarTouch");
			}
		}, "resets kickoff by scoring goal", PERMISSION_ALL);

	cvarManager->registerNotifier("kickoff_timer_teleport",
		[this](std::vector<std::string> params) {
			if (gameWrapper->IsInGame() && !gameWrapper->IsInOnlineGame()) {
				auto car = gameWrapper->GetLocalCar();
				if (car.IsNull()) return;
				car.SetLocation(spawnLocations[spawnChoice]);
				car.SetRotation(spawnRotations[spawnChoice]);
			}
		}, "moves car to selected spawn location", PERMISSION_ALL);
}

void PerfectSpeedflip::hookEvents() {
	gameWrapper->HookEventPost("Function TAGame.GameEvent_TA.BroadcastGoMessage", [this](std::string) { startKickoff(); });
	gameWrapper->RegisterDrawable([this](auto canvas) { render(canvas); });
	cvarManager->setBind("XboxTypeS_DPad_Up", "kickoff_timer_score");
}

void PerfectSpeedflip::unhookEvents() {
	gameWrapper->UnhookEventPost("Function TAGame.GameEvent_TA.BroadcastGoMessage");
}

void PerfectSpeedflip::startKickoff() {
	if (gameWrapper->IsInGame() || gameWrapper->IsInOnlineGame()) {
		gameWrapper->HookEventPost("Function TAGame.Car_TA.SetVehicleInput", [this](std::string) { tick++; });
		gameWrapper->HookEventPost("Function TAGame.Ball_TA.OnCarTouch", [this](std::string) { hitBall(); });
		tick = 0;
	} else {
		cvarManager->getCvar(enabledVarName).setValue("0");
	}
}

void PerfectSpeedflip::hitBall() {
	gameWrapper->UnhookEventPost("Function TAGame.Car_TA.SetVehicleInput");
	gameWrapper->UnhookEventPost("Function TAGame.Ball_TA.OnCarTouch");

	lastKickoffTime = tick;
	kickoffs++;

	if (kickoffs == 0) {
		averageKickoffTime = tick;
	} else {
		averageKickoffTime *= kickoffs - 1;
		averageKickoffTime += tick;
		averageKickoffTime /= kickoffs;
	}

	cvarManager->log("You hit the ball on tick " + std::to_string(tick));
	cvarManager->log("Tick / 120 = " + std::to_string((float) tick / 120.0) + " seconds");
}

void PerfectSpeedflip::onUnload()
{
}

void PerfectSpeedflip::render(CanvasWrapper canvas) {
	if (!enabledOverlay) {
		return;
	}

	Vector2 screen = canvas.GetSize();

	// in 1080p font size is 2
	// height of size 2 text is approx 20px
	float fontSize = ((float)screen.X / (float)1920) * 2;

	canvas.SetColor(overlayColor);
	Vector2 location = Vector2({ int(screen.X * xLocation), int(screen.Y * yLocation) });
	canvas.SetPosition(location);
	canvas.DrawString("Last Kickoff Time: " + std::to_string(lastKickoffTime) +  " Ticks / " 
		+ std::to_string(lastKickoffTime / 120.0) + " seconds", fontSize, fontSize);
	location += Vector2({ 0, int(fontSize * 11) });
	canvas.SetPosition(location);
	canvas.DrawString("Average Kickoff Time: " + std::to_string(averageKickoffTime) +  " Ticks / " 
		+ std::to_string(averageKickoffTime / 120.0) + " seconds", fontSize, fontSize);
}