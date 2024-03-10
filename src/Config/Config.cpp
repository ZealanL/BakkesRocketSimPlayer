#include "Config.h"

void Config::Serialize(DataStreamOut& out) const{

	out.Write<uint32_t>(vars.size());

	for (auto var : vars) {
		out.Write<uint32_t>(var.first.size());
		out.WriteBytes(var.first.data(), var.first.size());

		out.Write<int32_t>(var.second.type);

		if (var.second.type == ConfigVal::TYPE_STR) {
			out.Write<uint32_t>(var.second.strVal.size());
			out.WriteBytes(var.second.strVal.data(), var.second.strVal.size());
		} else {
			out.Write<double>(var.second.val);
		}

		// Don't need to serialize min and max
	}
}

void Config::Deserialize(DataStreamIn& in) {
	constexpr const char* ERR_PREFIX = "Config::Deserialize(): ";
	constexpr uint32_t MAX_NUM_VARS = 1024;
	constexpr uint32_t MAX_STR_LEN = 1024;

	uint32_t numVars = in.Read<uint32_t>();

	if (numVars == 0 || numVars > MAX_NUM_VARS)
		THROW_ERR(ERR_PREFIX << "Invalid number of vars: " << numVars);

	for (int i = 0; i < numVars; i++) {
		uint32_t nameLen = in.Read<uint32_t>();
		if (nameLen == 0 || nameLen > MAX_STR_LEN)
			THROW_ERR(ERR_PREFIX << "Variable name length invalid: " << nameLen);

		string varName;
		varName.resize(nameLen, 0);
		in.ReadBytes(varName.data(), nameLen);

		int32_t type = in.Read<int32_t>();
		double val = 0;
		string strVal;
		if (type == ConfigVal::TYPE_STR) {

			uint32_t valLen = in.Read<uint32_t>();
			if (valLen == 0 || valLen > MAX_STR_LEN)
				THROW_ERR(ERR_PREFIX << "Variable string value length invalid: " << nameLen);

			strVal.resize(valLen, 0);
			in.ReadBytes(strVal.data(), valLen);

		} else {
			val = in.Read<double>();
		}
		
		auto varItr = vars.find(varName);
		if (varItr != vars.end()) {
			varItr->second.type = type;
			varItr->second.val = val;
		} else {
			LOG(ERR_PREFIX << "Unknown variable \"" << varName << "\"");
		}
	}
}

const ConfigVal& Config::operator[](const string& name) const {
	auto itr = vars.find(name);

	if (itr != vars.end()) {
		return itr->second;
	} else {
		// Would rather do this than crash
		LOG("Config::operator[](): No variable with name \"" << name << "\" exists.");
		return ConfigVal();
	}
}

void Config::Set(const string& name, double val) {
	auto itr = vars.find(name);

	if (itr != vars.end()) {
		itr->second.val = val;
	} else {
		// Would rather do this than crash
		LOG("Config::Set(): No variable with name \"" << name << "\" exists.");
	}
}