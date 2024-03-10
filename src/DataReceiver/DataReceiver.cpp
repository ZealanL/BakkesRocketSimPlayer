#include "DataReceiver.h"

#include "../DataStreams/DataStreamIn.h"
#include "../Math/Math.h"

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include<winsock2.h>
#include <Ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

using namespace DataReceiver;

std::thread* g_ServerThread = NULL;
bool g_ShouldThreadRun = false;
bool g_IsThreadRunning = false;
bool g_IsConnected = false;
bool g_WSAInit = false;
uintptr_t g_SocketID = INVALID_SOCKET;

Data g_Data = {};
std::mutex g_DataMutex;

PhysState ReadPhysState(DataStreamIn& in) {
	static_assert(sizeof(Vector) == sizeof(float) * 3, "Bakkesmod Vector struct is not the size of 3 floats");
	
	PhysState state = {};
	state.pos = in.Read<Vector>();
	state.forward = in.Read<Vector>();
	state.right = in.Read<Vector>();
	state.up = in.Read<Vector>();
	state.rot = Math::RotMatToQuat(state.forward, state.right, state.up);
	state.vel = in.Read<Vector>();
	state.angVel = in.Read<Vector>();

	return state;
}

CarState ReadCarState(DataStreamIn& in) {
	CarState state = {};

	state.teamNum = in.Read<byte>();

	state.phys = ReadPhysState(in);

	state.boostAmount = in.Read<float>();

	state.onGround = in.Read<bool>();
	state.hasJump = in.Read<bool>();
	state.hasFlip = in.Read<bool>();
	state.isDemoed = in.Read<bool>();

	state.action = in.Read<CarAction>();

	return state;
}

void _ReceiveCallback(void* data, size_t size) {
	constexpr const char* ERR_PREFIX = "BSMP gamestate deserialize failed: ";

	DataStreamIn in = {};
	in.data = std::vector<byte>((byte*)data, (byte*)data + size);

	GameState state = {};
	state.tickCount = in.Read<uint32>();
	
	uint32 numPlayers = in.Read<uint32>();
	if (numPlayers > MAX_GAMESTATE_PLAYERS) {
		THROW_ERR(ERR_PREFIX << "Player count of " << numPlayers << " exceeds maximum of " << MAX_GAMESTATE_PLAYERS);
		return;
	}

	for (int i = 0; i < numPlayers; i++)
		state.cars.push_back(ReadCarState(in));
	
	// Make blue cars first, orange cars second
	std::stable_sort(state.cars.begin(), state.cars.end(), 
		[](const CarState& a, const CarState& b) {
			return a.teamNum < b.teamNum;
		}
	);

	state.ball = ReadPhysState(in);

	if (in.IsOverflown()) {
		THROW_ERR(ERR_PREFIX << "Data overflown");
		return;
	}

	if (in.GetNumBytesLeft() > 0) {
		THROW_ERR(ERR_PREFIX << "Data under-read, have " << in.GetNumBytesLeft() << " bytes left");
		return;
	}

	state.isValid = true;

	g_DataMutex.lock();
	g_Data.lastState = g_Data.curState;
	g_Data.curState = state;
	g_Data.lastRecvInterval = TIME_SINCE(g_Data.recieveTime);
	g_Data.recieveTime = CURTIME();
	g_DataMutex.unlock();

	g_IsConnected = true;
}

void _RunServer() {
	g_IsThreadRunning = true;

	while (true) {
		try {
			if (!g_WSAInit) {
				WSADATA wsa;
				if (WSAStartup(MAKEWORD(2, 2), &wsa)) {
					THROW_ERR("Error initializing Winsock " << WSAGetLastError());
				}
				g_WSAInit = true;
			}

			{
				g_SocketID = socket(AF_INET, SOCK_DGRAM, 0);
				if (g_SocketID == INVALID_SOCKET)
					THROW_ERR("Could not create socket");

				sockaddr_in sockAddr = {};
				sockAddr.sin_family = AF_INET;

				sockAddr.sin_port = htons(PORT);

				if (!inet_pton(AF_INET, "0.0.0.0", &sockAddr.sin_addr)) {
					THROW_ERR("UDP inet_pton error: " << WSAGetLastError());
				}

				if (bind(g_SocketID, (sockaddr*)(&sockAddr), sizeof(sockAddr)) == SOCKET_ERROR)
					THROW_ERR("UDP bind error: " << WSAGetLastError());

				sockaddr_in client;
				socklen_t slen = sizeof(client);

				constexpr size_t MSG_BUFFER_SIZE = 512 * 1024;
				char* msgBuffer = new char[MSG_BUFFER_SIZE];

				DWORD timeout = 500;
				setsockopt(g_SocketID, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

				while (g_ShouldThreadRun) {
					int64_t recvLen = recvfrom(g_SocketID, msgBuffer, MSG_BUFFER_SIZE, 0, (sockaddr*)(&client), &slen);
					if (recvLen > 0) {
						try {
							_ReceiveCallback(msgBuffer, recvLen);
						} catch (std::exception& e) {
							LOG("Data receive exception: " << e.what());
						}
					}
				}

				delete[] msgBuffer;
			}
		} catch (std::exception& e) {
			LOG("Data receiver setup exception: " << e.what());
			constexpr int RETRY_INTERVAL = 2000;
			LOG("Trying again in " << RETRY_INTERVAL << "ms...");
			Sleep(RETRY_INTERVAL);

			if (g_SocketID != INVALID_SOCKET)
				closesocket(g_SocketID);

			continue;
		}
		break;
	}

	if (g_SocketID != INVALID_SOCKET)
		closesocket(g_SocketID);

	g_IsThreadRunning = false;
}

void DataReceiver::Start() {
	g_ShouldThreadRun = true;
	g_ServerThread = new std::thread(_RunServer);

	g_ServerThread->detach();
}

void DataReceiver::Stop() {
	g_ShouldThreadRun = false;

	while (g_ShouldThreadRun)
		Sleep(10); // Wait for shutdown

	// TODO: Can cause crashes
	//delete g_ServerThread;
	g_ServerThread = NULL;
}

Data DataReceiver::GetData() {
	g_DataMutex.lock();
	Data result = g_Data;
	g_DataMutex.unlock();
	return result;
}

bool DataReceiver::IsConnected() {
	return g_IsConnected;
}