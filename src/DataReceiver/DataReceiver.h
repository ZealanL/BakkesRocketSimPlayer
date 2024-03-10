#pragma once
#include "../GameState.h"

namespace DataReceiver {
	constexpr int PORT = 4577;

	bool IsConnected();

	struct Data {
		GameState curState = {}, lastState = {};
		time_point recieveTime = CURTIME();
		double lastRecvInterval = 0;
	};
	Data GetData();
	void Start();
	void Stop();
}