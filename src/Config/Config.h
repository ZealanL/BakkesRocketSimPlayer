#pragma once
#include "../Framework.h"
#include "../DataStreams/DataStreamOut.h"
#include "../DataStreams/DataStreamIn.h"

struct ConfigVal {
	enum {
		TYPE_INVALID = 0,

		TYPE_BOOL,
		TYPE_INT,
		TYPE_FLOAT,
		TYPE_STR
	};
	int type;

	double val = 0;
	double min = 0, max = 0;

	std::string strVal;

	ConfigVal() {
		this->type = TYPE_INVALID;
	}

	ConfigVal(bool val) {
		this->type = TYPE_BOOL;
		this->val = val;
	}

	ConfigVal(int val, int min, int max) {
		this->type = TYPE_INT;
		this->val = val;
		this->min = min;
		this->max = max;
	}

	ConfigVal(float val, float min, float max) {
		this->type = TYPE_FLOAT;
		this->val = val;
		this->min = min;
		this->max = max;
	}

	ConfigVal(double val, double min, double max) {
		this->type = TYPE_FLOAT;
		this->val = val;
		this->min = min;
		this->max = max;
	}

	ConfigVal(const char* str) {
		this->type = TYPE_STR;
		this->strVal = str;
	}
};

struct Config {
	std::map<std::string, ConfigVal> vars = {
		{ "Bot Names", { "Agent" } },
		{ "Disable Out-of-Boost Sound", { true }},
		{ "Prevent Goal Scoring", { true }},
		{ "Run In Freeplay", { false }},
		{ "Min Phys Interp Scale", { 3.f, 0.f, 20.f }},
		{ "Max Phys Interp Scale", { 4.f, 0.f, 20.f }},
	};

	void Serialize(DataStreamOut& out) const;
	void Deserialize(DataStreamIn& in);

	const ConfigVal& operator[](const string& name) const;
	void Set(const string& name, double val);
};

inline Config g_Config = {};