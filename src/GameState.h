#pragma once
#include "Framework.h"

struct PhysState {
	Vector pos, vel, angVel;
	Quat rot;
	Vector forward, right, up;
};

struct CarAction {
	float throttle, steer;
	float pitch, yaw, roll;
	float jump, boost, handbrake;
	constexpr static int INPUT_COUNT = 8;

	float operator[](int index) const {
		return (&throttle)[index];
	}

	float& operator[](int index) {
		return (&throttle)[index];
	}

	friend std::ostream& operator<<(std::ostream& stream, const CarAction& action) {
		stream << "[ ";
		for (int i = 0; i < INPUT_COUNT; i++) {
			if (i > 0)
				stream << ", ";
			stream << action[i];
		}
		stream << " ]";
		return stream;
	}
};
static_assert(sizeof(CarAction) == sizeof(float) * CarAction::INPUT_COUNT, "CarAction struct has wrong size");

struct CarState {
	PhysState phys;

	byte teamNum;
	float boostAmount;
	bool onGround;
	bool hasJump;
	bool hasFlip;
	bool isDemoed;
	
	CarAction action;
};

constexpr int MAX_GAMESTATE_PLAYERS = 8;
struct GameState {
	PhysState ball;
	vector<CarState> cars;

	uint32 tickCount = 0;
	bool isValid = false;
};