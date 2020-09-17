#include "pch.h"
#include "PerfectSpeedflip.h"


BAKKESMOD_PLUGIN(PerfectSpeedflip, "Trains the perfect speedflip kickoff", plugin_version, PLUGINTYPE_FREEPLAY)

int tick = 0;
std::string stageOneResults = "Stage 1 Results:\n";
std::string stageTwoResults = "Stage 2 Results:\n";

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

	auto primaryCar = gameWrapper->GetLocalCar();
	if (primaryCar.IsNull()) return;

	ControllerInput input = primaryCar.GetInput();
	
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

	// checking inputs vs expected
	/*
	Duration 50
		Throttle 1
		Boost 1
	Duration 5
		Steer -0.7
	Duration 13
		Steer 0
		Jump 1
		Yaw -1
	Duration 1
		Jump 0
		Yaw 0
	Duration 1
		Pitch -1.0
		Roll 0.4
		Jump 1
	Duration 75
		Roll 1.
		Pitch 1.0
	Duration 52
		Steer 1
		Yaw 1
	Duration 50
		Steer 0
		Roll 0
		Yaw 0
	Duration 70
		Pitch 0
		Steer 0
	Duration 1
		Boost 0
		Throttle 0
	
	*/
	// 0-49
	/* Duration 50
		Throttle 1
		Boost 1 */
	if (tick < 50) {
		if (!input.HoldingBoost) {
			stageOneResults += "Must hold boost from tick 0 until tick 50, not boosting on tick " + std::to_string(tick) + "\n";
			//cvarManager->log(results);
		}
		if (input.Throttle < 1.0) {
			stageOneResults += "Must hold max throttle from tick 0 until tick 50, not max on tick " + 
				std::to_string(tick) + " with throttle " + std::to_string(input.Throttle) + "\n";
			//cvarManager->log(stageOneResults);
		}
		if (input.Steer != 0) {
			stageOneResults += "Must not steer from tick 0 until tick 50, steered on tick " +
				std::to_string(tick) + "\n";
		}
	// 50 - 54
	// Steer -0.7
	} else if (tick < 55) {
		if (input.Steer < -0.65) {
			stageTwoResults += "Must steer of -0.7 (down left) from tick 50 until tick 54, steer too far left on tick " + 
				std::to_string(tick) + " with steer " + std::to_string(input.Steer) + "\n";
		} 
		if (input.Steer > -0.75) {
			stageTwoResults += "Must steer of -0.7 (down left) from tick 50 until tick 54, steer too far down on tick " +
				std::to_string(tick) + " with steer " + std::to_string(input.Steer) + "\n";
		}
	}

	tick++;
}

void PerfectSpeedflip::hitBall() {
	gameWrapper->UnhookEventPost("Function TAGame.Car_TA.SetVehicleInput");
	gameWrapper->UnhookEventPost("Function TAGame.Ball_TA.OnCarTouch");

	cvarManager->log("Hit ball on tick " + std::to_string(tick));
	cvarManager->log(stageOneResults);
	cvarManager->log(stageTwoResults);
	tick = 0;

	stageOneResults = "Stage 1 Results:\n";
	stageTwoResults = "Stage 2 Results:\n";
}

void PerfectSpeedflip::onUnload()
{
}