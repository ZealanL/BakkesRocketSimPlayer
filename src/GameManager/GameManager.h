#pragma once
#include "../GameState.h"

namespace GameManager {

	// If an object moves this fast from one state to another, don't interp
	constexpr float TELEPORT_DIST = 1000;
	
	void SetNumPlayers(int numPlayers);
	void SetState(GameState prevState, GameState curState, float interpRatio, float interpTime, float deltaTime);
	std::unordered_map<uintptr_t, CarAction> GetCarActions();
};
