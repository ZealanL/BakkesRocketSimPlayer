#include "Framework.h"

#include "../libsrc/imgui/imgui.h"
#include "GameManager/GameManager.h"
#include "DataReceiver/DataReceiver.h"
#include "Config/Config.h"
#include "Utils/Utils.h"

class BakkesRocketSimPlayer : public BakkesMod::Plugin::BakkesModPlugin, public BakkesMod::Plugin::PluginSettingsWindow {
public:
	bool isConnected = false;
	void hkOnSetVehicleInput(CarWrapper car, void* params, string eventName);

	void hkOnTick(string eventName);
	void OnTick(ServerWrapper server, float deltaTime);

	void hkOnFreeplayReset(string eventName);

	virtual void onLoad();
	virtual void onUnload();

	void RenderSettings() override;
	string GetPluginName() { return "BakkesRocketSimPlayer"; };
	void SetImGuiContext(uintptr_t ctx) override;

	void FixConfig();
};

BAKKESMOD_PLUGIN(BakkesRocketSimPlayer, "BakkesRocketSimPlayer", BRSP_VERSION, PLUGINTYPE_FREEPLAY)

void BakkesRocketSimPlayer::hkOnSetVehicleInput(CarWrapper car, void* params, string eventName) {
	auto server = gameWrapper->GetGameEventAsServer();
	if (!server)
		return;

	if (gameWrapper->IsInFreeplay() && !g_Config["Run In Freeplay"].val)
		return;

	ControllerInput* inputs = (ControllerInput*)params;

	if (DataReceiver::IsConnected()) {
		auto carActions = GameManager::GetCarActions();

		if (carActions.find(car.memory_address) != carActions.end()) {
			auto& action = carActions[car.memory_address];

			inputs->Throttle = action.throttle;
			inputs->Steer = action.steer;

			inputs->Yaw = action.yaw;
			inputs->Pitch = action.pitch;
			inputs->Roll = action.roll;

			inputs->DodgeForward = -inputs->Pitch;
			inputs->DodgeStrafe = inputs->Yaw;

			inputs->Jump = action.jump;
			inputs->ActivateBoost = inputs->HoldingBoost = action.boost;

			if (g_Config["Disable Out-of-Boost Sound"].val) {
				if (auto boostComp = car.GetBoostComponent())
					if (boostComp.GetCurrentBoostAmount() == 0)
						inputs->ActivateBoost = inputs->HoldingBoost = 0;
			}

			inputs->Handbrake = action.handbrake;
		} else {
			*inputs = {};
		}
	}
}

void BakkesRocketSimPlayer::hkOnTick(string eventName) {
	auto server = gameWrapper->GetGameEventAsServer();
	if (!server)
		return;
	
	if (gameWrapper->IsInFreeplay() && !g_Config["Run In Freeplay"].val)
		return;

	static float lastWorldTime = -1;
	float curWorldTime = server.GetWorldInfo().GetTimeSeconds();

	if (lastWorldTime == curWorldTime) {
		// Ignore
	} else {
		float deltaTime = MAX(curWorldTime - lastWorldTime, 0);
		lastWorldTime = curWorldTime;
		OnTick(server, deltaTime);
	}
}

void BakkesRocketSimPlayer::OnTick(ServerWrapper server, float deltaTime) {
	if (DataReceiver::IsConnected()) {
		auto data = DataReceiver::GetData();
		if (data.curState.isValid && data.lastState.isValid) {

			float timeSince = TIME_SINCE(data.recieveTime);
			float interpRatio = CLAMP(timeSince / data.lastRecvInterval, 0, 1);
			if (isnan(interpRatio))
				interpRatio = 1;

			GameManager::SetState(data.lastState, data.curState, interpRatio, data.lastRecvInterval, deltaTime);
		}

		isConnected = true;
	} else {
		isConnected = false;
	}
}

void BakkesRocketSimPlayer::hkOnFreeplayReset(string eventName) {
	
}

void BakkesRocketSimPlayer::onLoad() {
	g_PluginInstMutex.lock();
	g_PluginInst = this;
	g_PluginInstMutex.unlock();

	// Create hooks
	gameWrapper->HookEventPost(
		"Function TAGame.GameEvent_Soccar_TA.StartNewRound", 
		std::bind(&BakkesRocketSimPlayer::hkOnFreeplayReset, this, std::placeholders::_1)
	);
	gameWrapper->HookEventPost(
		"Function TAGame.Car_TA.SetVehicleInput", 
		std::bind(&BakkesRocketSimPlayer::hkOnTick, this, std::placeholders::_1)
	);
	gameWrapper->HookEventWithCaller<CarWrapper>(
		"Function TAGame.Car_TA.SetVehicleInput",
		std::bind(&BakkesRocketSimPlayer::hkOnSetVehicleInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
	);

	DataReceiver::Start();
}

void BakkesRocketSimPlayer::onUnload() {
	DataReceiver::Stop();

	g_PluginInstMutex.lock();
	g_PluginInst = NULL;
	g_PluginInstMutex.unlock();
}

bool Setting_Choice(string title, int* curChoiceIndex, vector<string> choices) {
	bool result = false;

	if (ImGui::BeginCombo(title.c_str(), choices[*curChoiceIndex].c_str())) {

		for (int i = 0; i < choices.size(); i++) {
			bool selected = (*curChoiceIndex == i);
			result = result || ImGui::Selectable(choices[i].c_str(), &selected);
			if (selected)
				*curChoiceIndex = i;
		}

		ImGui::EndCombo();
	}

	return result;
}

bool Setting_Int(string label, int* val, int min, int max, string format = "%d") {
	ImGui::Text(label.c_str());
	ImGui::SameLine();
	string invisLabel = "##" + label;
	return ImGui::SliderInt(invisLabel.c_str(), val, min, max, format.c_str());
}

bool Setting_Bool(string label, bool* val) {
	return ImGui::Checkbox(label.c_str(), val);
}

void BakkesRocketSimPlayer::RenderSettings() {
	constexpr const char* DIVIDER = "\n\n======================\n\n";

	{ // Instructions
		string instructions = STR(
			"INSTRUCTIONS:"
			"\nCreate a LAN private match with no bots and unlimited time."
			"\nJoin a team so that the game starts. You can stay on a team if you want the a bot to use your car, otherwise go to spectate."
			"\nThe game will glitch out during the countdown, but this should stop once the game starts."
		);
		instructions += DIVIDER;
		ImGui::Text(instructions.c_str());
	}

	{ // Status
		auto curData = DataReceiver::GetData();
		string statusStr = STR("Connection status: " << (isConnected ? "Connected!" : "Waiting for connection..."));
		if (isConnected) {
			double timeSinceRecv = TIME_SINCE(curData.recieveTime);
			statusStr += STR("\nTime since data received: " << std::fixed << std::setprecision(4) << (float)timeSinceRecv << "s");
			statusStr += STR("\nReceive interval: " << std::fixed << std::setprecision(4) << (float)curData.lastRecvInterval << "s");
			statusStr += STR("\nCar count: " << curData.curState.cars.size());
		}
		statusStr += DIVIDER;
		ImGui::Text(statusStr.c_str());
	}

	for (auto& option : g_Config.vars) {
		if (option.second.type == ConfigVal::TYPE_BOOL) {

			bool val = option.second.val;
			if (ImGui::Checkbox(option.first.c_str(), &val))
				option.second.val = val;

		} else if (option.second.type == ConfigVal::TYPE_INT) {

			int val = option.second.val;

			ImGui::Text(option.first.c_str());
			ImGui::SameLine();
			string invisLabel = "##" + option.first;
			if (ImGui::SliderInt(invisLabel.c_str(), &val, option.second.min, option.second.max))
				option.second.val = val;

		} else if (option.second.type == ConfigVal::TYPE_FLOAT) {

			float val = option.second.val;

			ImGui::Text(option.first.c_str());
			ImGui::SameLine();
			string invisLabel = "##" + option.first;
			if (ImGui::SliderFloat(invisLabel.c_str(), &val, option.second.min, option.second.max))
				option.second.val = val;
		} else if (option.second.type == ConfigVal::TYPE_STR) {
			ImGui::Text(option.first.c_str());
			ImGui::SameLine();
			string invisLabel = "##" + option.first;
			constexpr int BUFFER_SIZE = 1024;
			char buf[BUFFER_SIZE] = {};
			memcpy(buf, option.second.strVal.data(), MIN(option.second.strVal.size(), BUFFER_SIZE - 1));

			if (ImGui::InputText(invisLabel.c_str(), buf, BUFFER_SIZE))
				option.second.strVal = buf;
			

		} else {
			string label = STR(option.first << ": [Unknown var type: " << option.second.type << "]");
			ImGui::Text(label.c_str());
		}
	}

	FixConfig();
}

void BakkesRocketSimPlayer::SetImGuiContext(uintptr_t ctx) {
	ImGui::SetCurrentContext((ImGuiContext*)ctx);
}

void BakkesRocketSimPlayer::FixConfig() {
	static std::mutex fixConfigMutex = {};
	fixConfigMutex.lock();

	{ // Fix min-max phys interp scale
		double
			minPhysInterp = g_Config["Min Phys Interp Scale"].val,
			maxPhysInterp = g_Config["Max Phys Interp Scale"].val;
		g_Config.Set("Min Phys Interp Scale", MIN(minPhysInterp, maxPhysInterp));
		g_Config.Set("Max Phys Interp Scale", MAX(minPhysInterp, maxPhysInterp));
	}

	fixConfigMutex.unlock();
}