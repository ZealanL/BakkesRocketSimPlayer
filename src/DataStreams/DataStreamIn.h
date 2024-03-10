#pragma once
#include "../Framework.h"

// Abridged version of: https://github.com/ZealanL/RocketSim/blob/main/src/DataStream/DataStreamIn.h
// Basic struct for reading raw data from a file
struct DataStreamIn {
	std::vector<byte> data;
	size_t pos = 0;

	DataStreamIn() = default;

	bool IsDone() const {
		return pos >= data.size();
	}

	bool IsOverflown() const {
		return pos > data.size();
	}

	size_t GetNumBytesLeft() const {
		if (IsDone()) {
			return 0;
		} else {
			return data.size() - pos;
		}
	}

	void ReadBytes(void* out, size_t amount) {
		if (GetNumBytesLeft() >= amount) {
			byte* asBytes = (byte*)out;
			memcpy(asBytes, data.data() + pos, amount);
		}

		pos += amount;
	}

	template <typename T>
	T Read() {
		byte bytes[sizeof(T)];
		ReadBytes(bytes, sizeof(T));
		return *(T*)bytes;
	}

	template <typename T>
	void Read(T& out) {
		out = Read<T>();
	}
};