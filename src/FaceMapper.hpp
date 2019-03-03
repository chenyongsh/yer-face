#pragma once

#include "Logger.hpp"
#include "SDLDriver.hpp"
#include "FrameServer.hpp"
#include "FaceTracker.hpp"
#include "MarkerTracker.hpp"
#include "Metrics.hpp"

using namespace std;
using namespace cv;

namespace YerFace {

class MarkerTracker;

class FaceMapper {
public:
	FaceMapper(json config, SDLDriver *mySDLDriver, FrameServer *myFrameServer, FaceTracker *myFaceTracker);
	~FaceMapper();
	void processCurrentFrame(void);
	void advanceWorkingToCompleted(void);
	void renderPreviewHUD(void);
	SDLDriver *getSDLDriver(void);
	FrameServer *getFrameServer(void);
	FaceTracker *getFaceTracker(void);
private:
	SDLDriver *sdlDriver;
	FrameServer *frameServer;
	FaceTracker *faceTracker;

	Logger *logger;
	SDL_mutex *myCmpMutex;
	Metrics *metrics;

	MarkerTracker *markerEyelidLeftTop;
	MarkerTracker *markerEyelidRightTop;
	MarkerTracker *markerEyelidLeftBottom;
	MarkerTracker *markerEyelidRightBottom;

	MarkerTracker *markerEyebrowLeftInner;
	MarkerTracker *markerEyebrowLeftMiddle;
	MarkerTracker *markerEyebrowLeftOuter;
	MarkerTracker *markerEyebrowRightInner;
	MarkerTracker *markerEyebrowRightMiddle;
	MarkerTracker *markerEyebrowRightOuter;

	MarkerTracker *markerJaw;

	MarkerTracker *markerLipsLeftCorner;
	MarkerTracker *markerLipsRightCorner;

	MarkerTracker *markerLipsLeftTop;
	MarkerTracker *markerLipsRightTop;
	
	MarkerTracker *markerLipsLeftBottom;
	MarkerTracker *markerLipsRightBottom;

	std::vector<MarkerTracker *> trackers;
};

}; //namespace YerFace
