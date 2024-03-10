#pragma once
#include "../Framework.h"

// Abridged version of: https://github.com/ZealanL/RocketSim/blob/main/src/DataStream/DataStreamOut.h
// Basic struct for writing raw data to a file
struct DataStreamOut {
	std::vector<byte> data;
	size_t pos = 0;

	DataStreamOut() = default;

	void WriteBytes(const void* ptr, size_t amount) {
		data.reserve(amount);
		data.insert(data.end(), (byte*)ptr, (byte*)ptr + amount);

		pos += amount;
	}

	template <typename T>
	void Write(const T& val) {
		WriteBytes(&val, sizeof(T));
	}

	bool WriteToFile(std::filesystem::path filePath) {
		std::ofstream fileStream = std::ofstream(filePath, std::ios::binary);
		if (!fileStream.good()) {
			THROW_ERR("Cannot write to file " << filePath);
			return false;
		}

		if (!data.empty())
			fileStream.write((char*)data.data(), data.size());

		return true;
	}
};