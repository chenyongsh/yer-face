#pragma once

#include "Logger.hpp"
#include "FrameDerivatives.hpp"
#include "FaceTracker.hpp"
#include "MarkerTracker.hpp"
#include "SDLDriver.hpp"
#include "EventLogger.hpp"
#include "Utilities.hpp"

#define ASIO_STANDALONE
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <set>

#include <fstream>

using namespace std;

namespace YerFace {

#define OUTPUTDRIVER_RINGBUFFER_SIZE 3600

class EventLogger;

class OutputFrameContainer {
public:
	bool isReady(void);
	json waitingOn;
	json frame;
};

class OutputDriver {
public:
	OutputDriver(json config, String myOutputFilename, FrameDerivatives *myFrameDerivatives, FaceTracker *myFaceTracker, SDLDriver *mySDLDriver);
	~OutputDriver();
	void setEventLogger(EventLogger *myEventLogger);
	void handleCompletedFrame(void);
	void drainPipelineDataNow(void);
	void registerLateFrameData(string key);
	void updateLateFrameData(signed long frameNumber, string key, json value);
	void insertCompletedFrameData(string key, json value);
private:
	void handleBasisEvent(void);
	static int launchWebSocketServer(void* data);
	static int writeOutputBufferToFile(void *data);
	void serverOnOpen(websocketpp::connection_hdl handle);
	void serverOnClose(websocketpp::connection_hdl handle);
	void serverOnTimer(websocketpp::lib::error_code const &ec);
	void serverSetQuitPollTimer(void);

	String outputFilename;
	FrameDerivatives *frameDerivatives;
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

	SDL_mutex *writerMutex;
	SDL_cond *writerCond;
	SDL_Thread *writerThread;
	bool writerThreadRunning;

	SDL_mutex *streamFlagsMutex;
	bool autoBasisTransmitted, basisFlagged;
	json lastBasisFrame;

	array<OutputFrameContainer *, OUTPUTDRIVER_RINGBUFFER_SIZE> outputBuf;
	unsigned long outputBufWriterThreadPosition, outputBufFrameHandlerPosition;
	SDL_mutex *outputBufMutex;

	list<string> lateFrameWaitOn;

	json extraFrameData;
};

}; //namespace YerFace
