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

// parts of broken kickoff trainer
std::string stageOneResults = "Stage 1 Results:\n";
std::string stageTwoResults = "Stage 2 Results:\n";
std::string stageThreeResults = "Stage 3 Results:\n";
std::string stageFourResults = "Stage 4 Results:\n";
std::string stageFiveResults = "Stage 5 Results:\n";
std::string stageSixResults = "Stage 6 Results:\n";
std::string stageSevenResults = "Stage 7 Results:\n";
std::string stageEightResults = "Stage 8 Results:\n";
std::string stageNineResults = "Stage 9 Results:\n";
bool example = false;

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

	
	cvarManager->registerCvar("speedflip_example", "0", "enables speedflip example", true, true, 0, true, 1)
		.addOnValueChanged([this](std::string, CVarWrapper cvar) {
		example = cvar.getBoolValue();
		});
	cvarManager->registerNotifier("speedflip_result_stage_one", 
		[this](auto) { cvarManager->log(stageOneResults); }, "returns result of stage one", PERMISSION_ALL);
	cvarManager->registerNotifier("speedflip_result_stage_two",
		[this](auto) { cvarManager->log(stageTwoResults); }, "returns result of stage two", PERMISSION_ALL);
	cvarManager->registerNotifier("speedflip_result_stage_three",
		[this](auto) { cvarManager->log(stageThreeResults); }, "returns result of stage three", PERMISSION_ALL);
	cvarManager->registerNotifier("speedflip_result_stage_four",
		[this](auto) { cvarManager->log(stageFourResults); }, "returns result of stage four", PERMISSION_ALL);
	cvarManager->registerNotifier("speedflip_result_stage_five",
		[this](auto) { cvarManager->log(stageFiveResults); }, "returns result of stage five", PERMISSION_ALL);
	cvarManager->registerNotifier("speedflip_result_stage_six",
		[this](auto) { cvarManager->log(stageSixResults); }, "returns result of stage six", PERMISSION_ALL);
	cvarManager->registerNotifier("speedflip_result_stage_seven",
		[this](auto) { cvarManager->log(stageSevenResults); }, "returns result of stage seven", PERMISSION_ALL);
	cvarManager->registerNotifier("speedflip_result_stage_eight",
		[this](auto) { cvarManager->log(stageEightResults); }, "returns result of stage eight", PERMISSION_ALL);
	cvarManager->registerNotifier("speedflip_result_stage_nine",
		[this](auto) { cvarManager->log(stageNineResults); }, "returns result of stage nine", PERMISSION_ALL);
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
		gameWrapper->HookEventPost("Function TAGame.Car_TA.SetVehicleInput", [this](std::string) { onTick(); });
		gameWrapper->HookEventPost("Function TAGame.Ball_TA.OnCarTouch", [this](std::string) { hitBall(); });
		tick = 0;
		
		stageOneResults = "Stage 1 Results:\n";
		stageTwoResults = "Stage 2 Results:\n";
		stageThreeResults = "Stage 3 Results:\n";
		stageFourResults = "Stage 4 Results:\n";
		stageFiveResults = "Stage 5 Results:\n";
		stageSixResults = "Stage 6 Results:\n";
		stageSevenResults = "Stage 7 Results:\n";
		stageEightResults = "Stage 8 Results:\n";
		stageNineResults = "Stage 9 Results:\n";
		
	} else {
		cvarManager->getCvar(enabledVarName).setValue("0");
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
		if (example) {
			ControllerInput nextInput;
			nextInput.DodgeForward = 0;
			nextInput.DodgeStrafe = 0;
			nextInput.Handbrake = 0;
			nextInput.Jump = 0;
			nextInput.Jumped = 0;
			nextInput.Pitch = 0;
			nextInput.Roll = 0;
			nextInput.Steer = 0;
			nextInput.Yaw = 0;
			nextInput.Throttle = 1;
			nextInput.HoldingBoost = 1;
			primaryCar.SetInput(nextInput);
			input = nextInput;
		}
		if (!input.HoldingBoost) {
			stageOneResults += "Must hold boost from tick 0 until ball is hit, not boosting on tick " + std::to_string(tick) + "\n";
			//cvarManager->log(results);
		}
		if (input.Throttle < 1.0) {
			stageOneResults += "Must hold max throttle from tick 0 until ball is hit, not max on tick " + 
				std::to_string(tick) + " with throttle " + std::to_string(input.Throttle) + "\n";
			//cvarManager->log(stageOneResults);
		}
		if (input.Steer != 0) {
			stageOneResults += "Must not steer from tick 0 until 49, steered on tick " +
				std::to_string(tick) + " with steer " + std::to_string(input.Steer) + "\n";
		}
		if (input.Pitch != 0) {
			stageOneResults += "Must not pitch up or down from tick 0 until 68, pitched on tick " +
				std::to_string(tick) + " with pitch " + std::to_string(input.Pitch) + "\n";
		}
	// 50 - 54
	// duration 5
	// Steer -0.7
	} else if (tick < 55) {
		if (example) {
			ControllerInput nextInput;
			nextInput.DodgeForward = 0;
			nextInput.DodgeStrafe = 0;
			nextInput.Handbrake = 0;
			nextInput.Jump = 0;
			nextInput.Jumped = 0;
			nextInput.Pitch = 0;
			nextInput.Roll = 0;
			nextInput.Yaw = 0;
			nextInput.Throttle = 1;
			nextInput.HoldingBoost = 1;
			nextInput.Steer = -0.7;
			primaryCar.SetInput(nextInput);
			input = nextInput;
		}
		if (!input.HoldingBoost) {
			stageTwoResults += "Must hold boost from tick 0 until ball is hit, not boosting on tick " + std::to_string(tick) + "\n";
		}
		if (input.Throttle < 1.0) {
			stageTwoResults += "Must hold max throttle from tick 0 until ball is hit, not max on tick " +
				std::to_string(tick) + " with throttle " + std::to_string(input.Throttle) + "\n";
		}
		if (input.Steer < -0.75) {
			stageTwoResults += "Must steer of -0.7 (70% left) from tick 50 until 54, steer too far left on tick " + 
				std::to_string(tick) + " with steer " + std::to_string(input.Steer) + "\n";
		} 
		if (input.Steer > -0.65) {
			stageTwoResults += "Must steer of -0.7 (70% left) from tick 50 until 54, steer too far right on tick " +
				std::to_string(tick) + " with steer " + std::to_string(input.Steer) + "\n";
		}
		if (input.Pitch != 0) {
			stageTwoResults += "Must not move stick up or down from tick 0 until 68, pitched on tick " +
				std::to_string(tick) + " with pitch " + std::to_string(input.Pitch) + "\n";
		}
	//55 - 67
	//Duration 13
	//Steer 0
	//Jump 1
	//Yaw - 1 (same as steer)
	} else if (tick < 68) {
		if (example) {
			ControllerInput nextInput;
			nextInput.DodgeForward = 0;
			nextInput.DodgeStrafe = 0;
			nextInput.Handbrake = 0;
			nextInput.Jumped = 0;
			nextInput.Pitch = 0;
			nextInput.Roll = 0;
			nextInput.Throttle = 1;
			nextInput.HoldingBoost = 1;
			nextInput.Steer = 0;
			nextInput.Jump = 1;
			nextInput.Yaw = -1;
			primaryCar.SetInput(nextInput);
			input = nextInput;
		}
		if (!input.HoldingBoost) {
			stageThreeResults += "Must hold boost from tick 0 until ball is hit, not boosting on tick " + std::to_string(tick) + "\n";
		}
		if (input.Throttle < 1.0) {
			stageThreeResults += "Must hold max throttle from tick 0 until ball is hit, not max on tick " +
				std::to_string(tick) + " with throttle " + std::to_string(input.Throttle) + "\n";
		}
		/* steer is ignored as it often is bound to the same as Yaw
		if (input.Steer != 0) {
			stageThreeResults += "Must not steer from tick 55 until tick 67, steered on tick " +
				std::to_string(tick) + "\n";
		}*/
		if (input.Pitch != 0) {
			stageThreeResults += "Must not pitch up or down from tick 0 until 67, pitched on tick " +
				std::to_string(tick) + " with pitch " + std::to_string(input.Pitch) + "\n";
		}
		if (!input.Jump) {
			stageThreeResults += "Must be jumping from tick 55 until 67, was not jumping on tick " +
				std::to_string(tick) + "\n";
		}
		if (input.Yaw > -1.0) {
			stageThreeResults += "Must yaw of -1 (max air steer left) from tick 55 until 67, not left on tick " +
				std::to_string(tick) + " with yaw " + std::to_string(input.Yaw) + "\n";
		}
	// 68
	//Duration 1
	//Jump 0
	//Yaw 0
	} else if (tick < 69) {
		if (example) {
			ControllerInput nextInput;
			nextInput.DodgeForward = 0;
			nextInput.DodgeStrafe = 0;
			nextInput.Handbrake = 0;
			nextInput.Jumped = 0;
			nextInput.Pitch = 0;
			nextInput.Roll = 0;
			nextInput.Throttle = 1;
			nextInput.HoldingBoost = 1;
			nextInput.Steer = 0;
			nextInput.Jump = 0;
			nextInput.Yaw = 0;
			primaryCar.SetInput(nextInput);
			input = nextInput;
		}
		if (!input.HoldingBoost) {
			stageFourResults += "Must hold boost from tick 0 until ball is hit, not boosting on tick " + std::to_string(tick) + "\n";
		}
		if (input.Throttle < 1.0) {
			stageFourResults += "Must hold max throttle from tick 0 until ball is hit, not max on tick " +
				std::to_string(tick) + " with throttle " + std::to_string(input.Throttle) + "\n";
		}
		/* steer is ignored as it often is bound to the same as Yaw
		if (input.Steer != 0) {
			stageFourResults += "Must not steer from tick 55 until tick 67, steered on tick " +
				std::to_string(tick) + "\n";
		}*/
		if (input.Pitch != 0) {
			stageFourResults += "Must not move pitch up or down from tick 0 until 68, pitched on tick " +
				std::to_string(tick) + " with pitch " + std::to_string(input.Pitch) + "\n";
		}
		if (input.Jump) {
			stageFourResults += "Must not be jumping on tick 68, was jumping on tick " +
				std::to_string(tick) + "\n";
		}
		if (input.Yaw != 0) {
			stageFourResults += "Must yaw of 0 (no air steer) from tick 68 until NA, yaw on tick " +
				std::to_string(tick) + " with yaw " + std::to_string(input.Yaw) + "\n";
		}
	//69
	//Duration 1
	//Pitch - 1.0
	//Roll 0.4
	//Jump 1
	} else if (tick < 70) {
		if (example) {
			ControllerInput nextInput;
			nextInput.DodgeForward = 0;
			nextInput.DodgeStrafe = 0;
			nextInput.Handbrake = 0;
			nextInput.Jumped = 0;
			nextInput.Throttle = 1;
			nextInput.HoldingBoost = 1;
			nextInput.Steer = 0;
			nextInput.Yaw = 0;
			nextInput.Pitch = -1.0;
			nextInput.Roll = 0.4;
			nextInput.Jump = 1;
			primaryCar.SetInput(nextInput);
			input = nextInput;
		}
		if (!input.HoldingBoost) {
			stageFiveResults += "Must hold boost from tick 0 until ball is hit, not boosting on tick " + std::to_string(tick) + "\n";
		}
		if (input.Throttle < 1.0) {
			stageFiveResults += "Must hold max throttle from tick 0 until ball is hit, not max on tick " +
				std::to_string(tick) + " with throttle " + std::to_string(input.Throttle) + "\n";
		}
		/* steer is ignored as it often is bound to the same as Roll
		if (input.Steer != 0) {
			stageFiveResults += "Must not steer from tick 55 until tick 67, steered on tick " +
				std::to_string(tick) + "\n";
		}*/
		if (input.Pitch > -1.0) {
			stageFiveResults += "Must pitch max down on tick 69, pitched on tick " +
				std::to_string(tick) + " with pitch " + std::to_string(input.Pitch) + "\n";
		}
		if (!input.Jump) {
			stageFiveResults += "Must be jumping on tick 69, was not jumping on tick " +
				std::to_string(tick) + "\n";
		}
		if (input.Yaw != 0) {
			stageFiveResults += "Must yaw of 0 (no air steer) from tick 68 until 144, yaw on tick " +
				std::to_string(tick) + " with yaw " + std::to_string(input.Yaw) + "\n";
		}
		if (input.Roll < 0.4) {
			stageFiveResults += "Must roll of 0.4 (slight right) on tick 69, roll too far left on tick " +
				std::to_string(tick) + " with roll " + std::to_string(input.Roll) + "\n";
		}
	//70-144
	//Duration 75
	//Roll 1.
	//Pitch 1.0
	} else if (tick < 145) {
		if (example) {
			ControllerInput nextInput;
			nextInput.DodgeForward = 0;
			nextInput.DodgeStrafe = 0;
			nextInput.Handbrake = 0;
			nextInput.Jumped = 0;
			nextInput.Throttle = 1;
			nextInput.HoldingBoost = 1;
			nextInput.Steer = 0;
			nextInput.Yaw = 0;
			nextInput.Jump = 1;
			nextInput.Roll = 1;
			nextInput.Pitch = 1;
			primaryCar.SetInput(nextInput);
			input = nextInput;
		}
		if (!input.HoldingBoost) {
			stageSixResults += "Must hold boost from tick 0 until ball is hit, not boosting on tick " + std::to_string(tick) + "\n";
		}
		if (input.Throttle < 1.0) {
			stageSixResults += "Must hold max throttle from tick 0 until ball is hit, not max on tick " +
				std::to_string(tick) + " with throttle " + std::to_string(input.Throttle) + "\n";
		}
		/* steer is ignored as it often is bound to the same as Roll
		if (input.Steer != 0) {
			stageSixResults += "Must not steer from tick 55 until tick 67, steered on tick " +
				std::to_string(tick) + "\n";
		}*/
		if (input.Pitch < 1.0) {
			stageSixResults += "Must pitch max up from tick 70 until 246, pitched on tick " +
				std::to_string(tick) + " with pitch " + std::to_string(input.Pitch) + "\n";
		}
		if (input.Jump) {
			stageSixResults += "Must have already flipped before tick 70, was jumping on tick " +
				std::to_string(tick) + "\n";
		}
		if (input.Yaw != 0) {
			stageSixResults += "Must yaw of 0 (no air steer) from tick 68 until 144, yaw on tick " +
				std::to_string(tick) + " with yaw " + std::to_string(input.Yaw) + "\n";
		}
		if (input.Roll < 1.0) {
			stageSixResults += "Must roll of 1.0 (max right) from tick 70 until 196, roll too far left on tick " +
				std::to_string(tick) + " with roll " + std::to_string(input.Roll) + "\n";
		}
	//145-196
	//Duration 52
	//Steer 1
	//Yaw 1
	} else if (tick < 197) {
		if (example) {
			ControllerInput nextInput;
			nextInput.DodgeForward = 0;
			nextInput.DodgeStrafe = 0;
			nextInput.Handbrake = 0;
			nextInput.Jumped = 0;
			nextInput.Throttle = 1;
			nextInput.HoldingBoost = 1;
			nextInput.Jump = 1;
			nextInput.Roll = 1;
			nextInput.Pitch = 1;
			nextInput.Steer = 1;
			nextInput.Yaw = 1;
			primaryCar.SetInput(nextInput);
			input = nextInput;
		}
		if (!input.HoldingBoost) {
			stageSevenResults += "Must hold boost from tick 0 until ball is hit, not boosting on tick " + std::to_string(tick) + "\n";
		}
		if (input.Throttle < 1.0) {
			stageSevenResults += "Must hold max throttle from tick 0 until ball is hit, not max on tick " +
				std::to_string(tick) + " with throttle " + std::to_string(input.Throttle) + "\n";
		}
		if (input.Steer < 1.0) {
			stageSevenResults += "Must steer max right from tick 144 until 196, steered on tick " +
				std::to_string(tick) + "\n";
		}
		if (input.Pitch < 1.0) {
			stageSevenResults += "Must pitch max up from tick 70 until 246, pitched on tick " +
				std::to_string(tick) + " with pitch " + std::to_string(input.Pitch) + "\n";
		}
		if (input.Jump) {
			stageSevenResults += "Must have already flipped before tick 69, was jumping on tick " +
				std::to_string(tick) + "\n";
		}
		if (input.Yaw < 1.0) {
			stageSevenResults += "Must yaw of 0 (no air steer) from tick 68 until 196, yaw on tick " +
				std::to_string(tick) + " with yaw " + std::to_string(input.Yaw) + "\n";
		}
		if (input.Roll < 1.0) {
			stageSevenResults += "Must roll of 1.0 (max right) from tick 68 until 196, roll too far left on tick " +
				std::to_string(tick) + " with roll " + std::to_string(input.Roll) + "\n";
		}
	//197-246
	//Duration 50
	//Steer 0
	//Roll 0
	//Yaw 0
	} else if (tick < 247) {
		if (example) {
			ControllerInput nextInput;
			nextInput.DodgeForward = 0;
			nextInput.DodgeStrafe = 0;
			nextInput.Handbrake = 0;
			nextInput.Jumped = 0;
			nextInput.Throttle = 1;
			nextInput.HoldingBoost = 1;
			nextInput.Jump = 1;
			nextInput.Pitch = 1;
			nextInput.Steer = 0;
			nextInput.Roll = 0;
			nextInput.Yaw = 0;
			primaryCar.SetInput(nextInput);
			input = nextInput;
		}
		if (!input.HoldingBoost) {
			stageEightResults += "Must hold boost from tick 0 until ball is hit, not boosting on tick " + std::to_string(tick) + "\n";
		}
		if (input.Throttle < 1.0) {
			stageEightResults += "Must hold max throttle from tick 0 until ball is hit, not max on tick " +
				std::to_string(tick) + " with throttle " + std::to_string(input.Throttle) + "\n";
		}
		if (input.Steer != 0) {
			stageEightResults += "Must steer max right from tick 144 until 197, steered on tick " +
				std::to_string(tick) + "\n";
		}
		if (input.Pitch < 1.0) {
			stageEightResults += "Must pitch max up from tick 70 until 246, pitched on tick " +
				std::to_string(tick) + " with pitch " + std::to_string(input.Pitch) + "\n";
		}
		if (input.Jump) {
			stageEightResults += "Must have already flipped before tick 69, was jumping on tick " +
				std::to_string(tick) + "\n";
		}
		if (input.Yaw != 0) {
			stageEightResults += "Must yaw of 0 (no air steer) from tick 197 until ball hit, yaw on tick " +
				std::to_string(tick) + " with yaw " + std::to_string(input.Yaw) + "\n";
		}
		if (input.Roll != 0) {
			stageEightResults += "Must not roll from tick 197 until ball hit, roll too far left on tick " +
				std::to_string(tick) + " with roll " + std::to_string(input.Roll) + "\n";
		}
	//247-316
	//Duration 70
	//Pitch 0
	//Steer 0
	} else if (tick < 317) {
		if (example) {
			ControllerInput nextInput;
			nextInput.DodgeForward = 0;
			nextInput.DodgeStrafe = 0;
			nextInput.Handbrake = 0;
			nextInput.Jumped = 0;
			nextInput.Throttle = 1;
			nextInput.HoldingBoost = 1;
			nextInput.Jump = 1;
			nextInput.Roll = 0;
			nextInput.Yaw = 0;
			nextInput.Pitch = 0;
			nextInput.Steer = 0;
			primaryCar.SetInput(nextInput);
			input = nextInput;
		}
		if (!input.HoldingBoost) {
			stageEightResults += "Must hold boost from tick 0 until ball is hit, not boosting on tick " + std::to_string(tick) + "\n";
		}
		if (input.Throttle < 1.0) {
			stageEightResults += "Must hold max throttle from tick 0 until ball is hit, not max on tick " +
				std::to_string(tick) + " with throttle " + std::to_string(input.Throttle) + "\n";
		}
		if (input.Steer != 0) {
			stageEightResults += "Must not steer from tick 247 until ball hit, steered on tick " +
				std::to_string(tick) + "\n";
		}
		if (input.Pitch != 0) {
			stageEightResults += "Must not pitch tick 247 until ball hit, pitched on tick " +
				std::to_string(tick) + " with pitch " + std::to_string(input.Pitch) + "\n";
		}
		if (input.Jump) {
			stageEightResults += "Must have already flipped before tick 69, was jumping on tick " +
				std::to_string(tick) + "\n";
		}
		if (input.Yaw != 0) {
			stageEightResults += "Must yaw of 0 (no air steer) from tick 197 until ball hit, yaw on tick " +
				std::to_string(tick) + " with yaw " + std::to_string(input.Yaw) + "\n";
		}
		if (input.Roll != 0) {
			stageEightResults += "Must not roll from tick 197 until ball hit, roll too far left on tick " +
				std::to_string(tick) + " with roll " + std::to_string(input.Roll) + "\n";
		}
	}
	//Duration 1
	//Boost 0
	//Throttle 0

	tick++;
}

void PerfectSpeedflip::hitBall() {
	gameWrapper->UnhookEventPost("Function TAGame.Car_TA.SetVehicleInput");
	gameWrapper->UnhookEventPost("Function TAGame.Ball_TA.OnCarTouch");
	
	cvarManager->log(stageOneResults);
	cvarManager->log(stageTwoResults);
	cvarManager->log(stageThreeResults);
	cvarManager->log(stageFourResults);
	cvarManager->log(stageFiveResults);
	cvarManager->log(stageSixResults);
	cvarManager->log(stageSevenResults);
	cvarManager->log(stageEightResults);
	cvarManager->log(stageNineResults);
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
	canvas.DrawString("Last Kickoff Time: " + std::to_string(lastKickoffTime) +  " Ticks / " + std::to_string(lastKickoffTime / 120.0) + " seconds", fontSize, fontSize);
	location += Vector2({ 0, int(fontSize * 11) });
	canvas.SetPosition(location);
	canvas.DrawString("Average Kickoff Time: " + std::to_string(averageKickoffTime) +  " Ticks / " + std::to_string(averageKickoffTime / 120.0) + " seconds", fontSize, fontSize);
}