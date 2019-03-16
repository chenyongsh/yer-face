#pragma once

#include "Logger.hpp"
#include "FrameServer.hpp"
#include "FaceTracker.hpp"
#include "MarkerTracker.hpp"
#include "SDLDriver.hpp"
#include "EventLogger.hpp"
#include "Utilities.hpp"
#include "Status.hpp"
#include "WorkerPool.hpp"

#define ASIO_STANDALONE
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <set>

#include <fstream>

using namespace std;

namespace YerFace {

class EventLogger;

class OutputFrameContainer {
public:
	bool isReady(void);
	bool frameIsDraining;
	bool outputProcessed;
	FrameTimestamps frameTimestamps;
	json waitingOn;
	json frame;
};

class OutputDriver {
public:
	OutputDriver(json config, String myOutputFilename, Status *myStatus, FrameServer *myFrameServer, FaceTracker *myFaceTracker, SDLDriver *mySDLDriver);
	~OutputDriver() noexcept(false);
	void setEventLogger(EventLogger *myEventLogger);
	void registerLateFrameData(string key);
	// void updateLateFrameData(FrameNumber frameNumber, string key, json value);
	// void insertCompletedFrameData(string key, json value);
private:
	// void handleNewBasisEvent(bool generatedByUserInput);
	// bool handleReplayBasisEvent(json sourcePacket);
	static int launchWebSocketServer(void* data);
	void serverOnOpen(websocketpp::connection_hdl handle);
	void serverOnClose(websocketpp::connection_hdl handle);
	void serverOnTimer(websocketpp::lib::error_code const &ec);
	void handleOutputFrame(OutputFrameContainer *outputFrame);
	void serverSetQuitPollTimer(void);
	void outputNewFrame(json frame, bool replay = false);
	static bool workerHandler(WorkerPoolWorker *worker);
	static void handleFrameStatusChange(void *userdata, WorkingFrameStatus newStatus, FrameTimestamps frameTimestamps);

	String outputFilename;
	Status *status;
	FrameServer *frameServer;
	FaceTracker *faceTracker;
	SDLDriver *sdlDriver;
	EventLogger *eventLogger;
	Logger *logger;

	ofstream outputFilestream;

	SDL_mutex *connectionListMutex;
	int websocketServerPort;
	bool websocketServerEnabled;
	websocketpp::server<websocketpp::config::asio> server;
	std::set<websocketpp::connection_hdl,std::owner_less<websocketpp::connection_hdl>> connectionList;

	SDL_Thread *serverThread;

	SDL_mutex *streamFlagsMutex;
	bool autoBasisTransmitted, basisFlagged;
	json lastBasisFrame;

	list<string> lateFrameWaitOn;

	WorkerPool *workerPool;
	SDL_mutex *workerMutex;
	unordered_map<FrameNumber, OutputFrameContainer> pendingFrames;
};

}; //namespace YerFace
