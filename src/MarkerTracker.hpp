#pragma once

#include <string>
#include <tuple>
#include <cstdlib>
#include <list>

#include "opencv2/objdetect.hpp"
#include "opencv2/tracking.hpp"

#include "MarkerType.hpp"
#include "FaceMapper.hpp"
#include "FrameDerivatives.hpp"
#include "TrackerState.hpp"
#include "MarkerSeparator.hpp"

using namespace std;
using namespace cv;

namespace YerFace {

class MarkerCandidate {
public:
	RotatedRect marker;
	unsigned int markerListIndex;
	double distanceFromPointOfInterest;
	double sqrtArea;
};

class FaceMapper;

class MarkerTracker {
public:
	MarkerTracker(MarkerType myMarkerType, FaceMapper *myFaceMapper, FrameDerivatives *myFrameDerivatives, MarkerSeparator *myMarkerSeparator, float myTrackingBoxPercentage = 1.5, float myMaxTrackerDriftPercentage = 0.75);
	~MarkerTracker();
	MarkerType getMarkerType(void);
	TrackerState processCurrentFrame(void);
	void renderPreviewHUD(bool verbose = true);
	TrackerState getTrackerState(void);
	tuple<Point2d, bool> getMarkerPoint(void);
	static vector<MarkerTracker *> *getMarkerTrackers(void);
	static MarkerTracker *getMarkerTrackerByType(MarkerType markerType);
	static bool sortMarkerCandidatesByDistanceFromPointOfInterest(const MarkerCandidate a, const MarkerCandidate b);
	static bool sortMarkerCandidatesByAngleFromPointOfInterest(const MarkerCandidate a, const MarkerCandidate b);
	static bool sortMarkerCandidatesByAngleFromPointOfInterestInverted(const MarkerCandidate a, const MarkerCandidate b);
private:
	void performTrackToSeparatedCorrelation(void);
	void performDetection(void);
	void performInitializationOfTracker(void);
	bool performTracking(void);
	bool trackerDriftingExcessively(void);
	bool claimMarkerCandidate(MarkerCandidate markerCandidate);
	bool claimFirstAvailableMarkerCandidate(list<MarkerCandidate> *markerCandidateList);
	void assignMarkerPoint(void);
	void generateMarkerCandidateList(list<MarkerCandidate> *markerCandidateList, Point2d pointOfInterest, Rect2d *boundingRect = NULL);
	
	static vector<MarkerTracker *> markerTrackers;

	MarkerType markerType;
	FaceMapper *faceMapper;
	FrameDerivatives *frameDerivatives;
	MarkerSeparator *markerSeparator;
	float trackingBoxPercentage;
	float maxTrackerDriftPercentage;

	Ptr<Tracker> tracker;
	vector<MarkerSeparated> *markerList;
	TrackerState trackerState;
	MarkerCandidate markerDetected;
	bool markerDetectedSet;
	Rect2d trackingBox;
	bool trackingBoxSet;
	Point2d markerPoint;
	bool markerPointSet;
};

}; //namespace YerFace
