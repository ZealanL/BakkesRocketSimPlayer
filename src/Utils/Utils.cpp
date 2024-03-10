#include "Utils.h"

vector<CarWrapper> Utils::GetPlayerCars(ServerWrapper server) {
	vector<CarWrapper> result;
	auto pris = GetPRIs(server);
	for (auto pri : pris)
		result.push_back(pri.GetCar());

	return result;
}

vector<PriWrapper> Utils::GetPRIs(ServerWrapper server) {
	vector<PriWrapper> result;

	for (auto player : server.GetPRIs())
		if (!player.IsSpectator() && player.GetTeam())
			result.push_back(player);

	std::stable_sort(result.begin(), result.end(), 
		[](PriWrapper p1, PriWrapper p2) {
			return p1.GetTeam().GetTeamNum() < p2.GetTeam().GetTeamNum();
		}
	);

	return result;
}