#include "pch.h"
#include "PerfectSpeedflip.h"


BAKKESMOD_PLUGIN(PerfectSpeedflip, "Trains the perfect speedflip kickoff", plugin_version, PLUGINTYPE_FREEPLAY)

int tick = 0;

void PerfectSpeedflip::onLoad()
{
	cvarManager->log("Plugin loaded!");

	cvarManager->registerCvar("speedflip_enable", "0", "enables speedflip trainer", true, true, 0, true, 1)
		.addOnValueChanged([this](std::string, CVarWrapper cvar) {
			if (cvar.getBoolValue()) {
				hookEvents();
			} else {
				unhookEvents();
			}});
}

void PerfectSpeedflip::hookEvents() {
	//gameWrapper->HookEventPost("Function TAGame.GameEvent_Soccar_TA.Countdown.EndState", [this](std::string) { startKickoff(); });
	gameWrapper->HookEventPost("Function TAGame.GameEvent_TA.BroadcastGoMessage", [this](std::string) { startKickoff(); });
}

void PerfectSpeedflip::unhookEvents() {
	gameWrapper->UnhookEventPost("Function TAGame.GameEvent_TA.BroadcastGoMessage");
}

void PerfectSpeedflip::startKickoff() {
	if (gameWrapper->IsInGame() && !gameWrapper->IsInOnlineGame()) {
		gameWrapper->HookEventPost("Function TAGame.Car_TA.SetVehicleInput", [this](std::string) { onTick(); });
		gameWrapper->HookEventPost("Function TAGame.Ball_TA.OnCarTouch", [this](std::string) { hitBall(); });
	} else {
		cvarManager->getCvar("speedflip_enable").setValue("0");
		unhookEvents();
	}
}

void PerfectSpeedflip::onTick() {
	if (!gameWrapper->IsInGame() || gameWrapper->IsInOnlineGame()) {
		gameWrapper->UnhookEventPost("Function TAGame.Car_TA.SetVehicleInput");
		return;
	}

	auto sw = gameWrapper->GetGameEventAsServer();
	if (sw.IsNull()) return;

	auto primaryPRI = sw.GetLocalPrimaryPlayer();
	if (primaryPRI.IsNull()) return;

	ControllerInput input = primaryPRI.GetVehicleInput();
	
	cvarManager->log("Input on tick " + std::to_string(tick));
	cvarManager->log("Activate Boost = " + std::to_string(input.ActivateBoost));
	cvarManager->log("Dodge Forward = " + std::to_string(input.DodgeForward));
	cvarManager->log("Dodge Strafe = " + std::to_string(input.DodgeStrafe));
	cvarManager->log("Handbrake = " + std::to_string(input.Handbrake));
	cvarManager->log("HoldingBoost = " + std::to_string(input.HoldingBoost));
	cvarManager->log("Jump = " + std::to_string(input.Jump));
	cvarManager->log("Jumped = " + std::to_string(input.Jumped));
	cvarManager->log("Pitch = " + std::to_string(input.Pitch));
	cvarManager->log("Roll = " + std::to_string(input.Roll));
	cvarManager->log("Steer = " + std::to_string(input.Steer));
	cvarManager->log("Throttle = " + std::to_string(input.Throttle));
	cvarManager->log("Yaw = " + std::to_string(input.Yaw));

	tick++;
}

void PerfectSpeedflip::hitBall() {
	gameWrapper->UnhookEventPost("Function TAGame.Car_TA.SetVehicleInput");
	gameWrapper->UnhookEventPost("Function TAGame.Ball_TA.OnCarTouch");

	cvarManager->log("Hit ball on tick " + std::to_string(tick));
	tick = 0;
}

void PerfectSpeedflip::onUnload()
{
}