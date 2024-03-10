#include "GameManager.h"
#include "../Math/Math.h"
#include "../Utils/Utils.h"
#include "../Config/Config.h"

static std::unordered_map<uintptr_t, CarAction> g_CarActions = {};

void GameManager::SetNumPlayers(int numPlayers) {
	ServerWrapper server = g_PluginInst->gameWrapper->GetGameEventAsServer();
	if (!server)
		return;

	auto players = Utils::GetPRIs(server);
	int numSpawnedCars = players.size();
	int numBotsToSpawn = numPlayers - numSpawnedCars;
	if (numBotsToSpawn > 0)
		LOG("GameManager::SetNumPlayers(" << numPlayers << "): > Spawning " << numBotsToSpawn << " bots...");
	
	for (int i = 0; i < numBotsToSpawn; i++) {
		string name = g_Config["Bot Names"].strVal;
		server.SpawnBot(23, name);
	}

	// Show normal freeplay car colors
	g_PluginInst->cvarManager->executeCommand("cl_freeplay_carcolor 1", false);
	g_PluginInst->cvarManager->executeCommand("cl_freeplay_stadiumcolors 1", false);

	// Turn on anonymizer to get rid of goofy bot decals/toppers/etc.
	g_PluginInst->cvarManager->executeCommand("cl_anonymizer_mode_team 1", false);
	g_PluginInst->cvarManager->executeCommand("cl_anonymizer_mode_opponent 1", false);
}

PhysState InterpPhys(PhysState from, PhysState to, float ratio, float interpInterval) {
	PhysState result = from;

	if ((from.pos - to.pos).magnitude() < GameManager::TELEPORT_DIST) {
		result.pos = Math::Interp(from.pos, to.pos, ratio);
	} else {
		result.pos = to.pos;
	}
	
	result.rot = Math::Interp(from.rot, to.rot, ratio);
	result.vel = Math::Interp(from.vel, to.vel, ratio);
	result.angVel = Math::Interp(from.angVel, to.angVel, ratio);

	if (result.pos.Z < 0 || result.pos.magnitude() > 10000) {
		LOG("InterpPhys WARNING: Position outside of bounds: " << result.pos);
	}

	return result;
}

// Interpolate real physics to networked state
void ApplyPhys(RBState& rbs, PhysState ps, float interpRatio, float deltaTime) {
	
	// Scale interp ratio to be more aggresive near edges of network states
	// TODO: This scaling is sub-optimal
	float minRealInterp = deltaTime * g_Config["Min Phys Interp Scale"].val;
	float maxRealInterp = deltaTime * g_Config["Max Phys Interp Scale"].val;
	float interpEdgeRatio = CLAMP(abs(0.5 - interpRatio) * 2, 0, 1);
	float realInterpRatio = Math::Interp(minRealInterp, maxRealInterp, interpEdgeRatio);

	if ((rbs.Location - ps.pos).magnitude() > GameManager::TELEPORT_DIST) {
		realInterpRatio = 1; // Just teleport
	}

	rbs.Location = Math::Interp(rbs.Location, ps.pos, realInterpRatio);
	rbs.Quaternion = Math::Interp(rbs.Quaternion, ps.rot, realInterpRatio);
	rbs.LinearVelocity = Math::Interp(rbs.LinearVelocity, ps.vel, realInterpRatio);
	rbs.AngularVelocity = Math::Interp(rbs.AngularVelocity, ps.angVel, realInterpRatio);
}

void GameManager::SetState(GameState prevState, GameState curState, float interpRatio, float interpInterval, float deltaTime) {
	ServerWrapper server = g_PluginInst->gameWrapper->GetGameEventAsServer();
	auto ball = server.GetBall();
	if (!ball)
		return;

	if (curState.cars.size() != prevState.cars.size()) {
		LOG("Car amount changed");
		curState = prevState;
		interpRatio = 0;
	}

	if (curState.cars.size() != server.GetCars().Count()) {
		SetNumPlayers(curState.cars.size());
	}

	auto serverCars = Utils::GetPlayerCars(server);

	// Increment of server car indexes
	// Fixes the problem of picking two cars from the same team in a 1v1, if more cars are available
	int inc;
	if (serverCars.size() > curState.cars.size()) {
		inc = MAX(serverCars.size() / curState.cars.size(), 2);
	} else {
		inc = 1;
	}
	
	unordered_set<int> usedServerCars;
	g_CarActions.clear();
	for (int i = 0; i < curState.cars.size(); i++) {
		int carIdx = i * inc;
		if (carIdx >= serverCars.size())
			break;

		usedServerCars.insert(carIdx);

		CarWrapper car = serverCars[carIdx];
		if (!car)
			continue;

		auto& prevCarState = prevState.cars[i];
		auto& curCarState = curState.cars[i];

		if (prevCarState.isDemoed) {
			car.Demolish2(car);
			continue;
		}

		auto rbs = car.GetRBState();
		Vector posBefore = rbs.Location;
		auto carPhys = InterpPhys(prevCarState.phys, curCarState.phys, interpRatio, interpInterval);
		ApplyPhys(rbs, carPhys, interpRatio, deltaTime);
		car.SetRBState(rbs);
		car.SetPhysicsState(rbs);

		g_CarActions[car.memory_address] = prevCarState.action;

		if (auto boostComp = car.GetBoostComponent()) {
			float boostAmount;
			if (curCarState.boostAmount > prevCarState.boostAmount) {
				// Boost collected, don't interp
				boostAmount = prevCarState.boostAmount;
				
			} else {
				boostAmount = Math::Interp(prevCarState.boostAmount, curCarState.boostAmount, interpRatio);
			}

			if (boostAmount < 0.02)
				boostAmount = 0; // Prevent near-zero jitter thing, it makes an annoying sound

			boostComp.SetBoostAmount(boostAmount);
		}
	}

	// Move extra cars in the arena outside
	for (int i = 0; i < serverCars.size(); i++) {
		
		bool used = usedServerCars.find(i) != usedServerCars.end();
		if (used)
			continue;

		auto car = serverCars[i];
		auto rbs = car.GetRBState();
		rbs.Location = Vector(10000, 10000, -3000);
		rbs.LinearVelocity = rbs.AngularVelocity = Vector(0, 0, 0);
		car.SetRBState(rbs);
		car.SetPhysicsState(rbs);
	}

	{ // Set ball
		auto bs = InterpPhys(prevState.ball, curState.ball, interpRatio, interpInterval);
		auto rbs = ball.GetRBState();
		Vector posBefore = rbs.Location;
		ApplyPhys(rbs, bs, interpRatio, deltaTime);

		if (g_Config["Prevent Goal Scoring"].val) {
			constexpr float 
				GOAL_SCORE_THRESH = 5124.25f + 91.25f,
				GOAL_SCORE_MARGIN = 40;
			rbs.Location.Y = MIN(abs(rbs.Location.Y), GOAL_SCORE_THRESH - GOAL_SCORE_MARGIN) * SGN(rbs.Location.Y);
		}

		ball.SetRBState(rbs);
		ball.SetPhysicsState(rbs);

#ifdef INTERP_DEBUG
		static float lastRatio = interpRatio;
		float ratioDelta = interpRatio - lastRatio;
		if (ratioDelta < 0)
			ratioDelta++;
		lastRatio = interpRatio;

		LOG("Ratio: " << interpRatio);
		LOG("Ratio Delta +" << ratioDelta);
		LOG("Moved: " << (posBefore - rbs.Location).magnitude());

		int fromTick = prevState.tickCount;
		int toTick = curState.tickCount;
		float curTick = fromTick + (toTick - fromTick) * interpRatio;
#endif

	}
}

std::unordered_map<uintptr_t, CarAction> GameManager::GetCarActions() {
	return g_CarActions;
}