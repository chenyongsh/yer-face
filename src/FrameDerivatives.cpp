
#include "FrameDerivatives.hpp"
#include "Utilities.hpp"
#include <exception>
#include <cstdio>

using namespace std;

namespace YerFace {

FrameDerivatives::FrameDerivatives(json config, bool myLowLatency) {
	logger = new Logger("FrameDerivatives");
	lowLatency = myLowLatency;
	if((myMutex = SDL_CreateMutex()) == NULL) {
		throw runtime_error("Failed creating mutex!");
	}
	string lowLatencyKey = "LowLatency";
	if(!lowLatency) {
		lowLatencyKey = "Offline";
	}
	classificationBoundingBox = config["YerFace"]["FrameDerivatives"][lowLatencyKey]["classificationBoundingBox"];
	if(classificationBoundingBox < 0) {
		throw invalid_argument("Classification Bounding Box is invalid.");
	}
	classificationScaleFactor = config["YerFace"]["FrameDerivatives"][lowLatencyKey]["classificationScaleFactor"];
	if(classificationScaleFactor < 0.0 || classificationScaleFactor > 1.0) {
		throw invalid_argument("Classification Scale Factor is invalid.");
	}
	metrics = new Metrics(config, "FrameDerivatives");
	logger->debug("FrameDerivatives constructed and ready to go!");
}

FrameDerivatives::~FrameDerivatives() {
	logger->debug("FrameDerivatives object destructing...");
	// FIXME - Actually tear everything down...
	SDL_DestroyMutex(myMutex);
	delete metrics;
	delete logger;
}

void FrameDerivatives::setWorkingFrame(VideoFrame *videoFrame) {
	YerFace_MutexLock(myMutex);
	MetricsTick tick = metrics->startClock();

	WorkingFrame workingFrame;

	workingFrame.frame = videoFrame->frameCV.clone();
	workingFrame.frameSet = true;

	frameSize = workingFrame.frame.size();
	frameSizeSet = true;

	workingFrame.frameTimestamps = videoFrame->timestamp;

	if(classificationBoundingBox > 0) {
		if(frameSize.width >= frameSize.height) {
			classificationScaleFactor = (double)classificationBoundingBox / (double)frameSize.width;
		} else {
			classificationScaleFactor = (double)classificationBoundingBox / (double)frameSize.height;
		}
	}

	resize(workingFrame.frame, workingFrame.classificationFrame, Size(), classificationScaleFactor, classificationScaleFactor);

	static bool reportedScale = false;
	if(!reportedScale) {
		logger->debug("Scaled current frame <%dx%d> down to <%dx%d> for classification", frameSize.width, frameSize.height, workingFrame.classificationFrame.size().width, workingFrame.classificationFrame.size().height);
		reportedScale = true;
	}

	workingFrame.previewFrameSet = false;

	frameStore[workingFrame.frameTimestamps.frameNumber] = workingFrame;
	logger->verbose("Inserted new working frame %ld into frame store. Frame store size is now %lu", workingFrame.frameTimestamps.frameNumber, frameStore.size());

	metrics->endClock(tick);
	YerFace_MutexUnlock(myMutex);
}

// Mat FrameDerivatives::getWorkingFrame(void) {
// 	YerFace_MutexLock(myMutex);
// 	if(!workingFrameSet) {
// 		YerFace_MutexUnlock(myMutex);
// 		throw runtime_error("getWorkingFrame() called, but no working frame set");
// 	}
// 	Mat value = workingFrame;
// 	YerFace_MutexUnlock(myMutex);
// 	return value;
// }

// Mat FrameDerivatives::getCompletedFrame(void) {
// 	YerFace_MutexLock(myMutex);
// 	if(!completedFrameSet) {
// 		YerFace_MutexUnlock(myMutex);
// 		throw runtime_error("getCompletedFrame() called, but no completed frame set");
// 	}
// 	Mat value = completedFrame;
// 	YerFace_MutexUnlock(myMutex);
// 	return value;
// }

// void FrameDerivatives::advanceWorkingFrameToCompleted(void) {
// 	YerFace_MutexLock(myMutex);
// 	if(!workingFrameSet) {
// 		YerFace_MutexUnlock(myMutex);
// 		throw runtime_error("advanceWorkingFrameToCompleted() called, but no working frame set");
// 	}
// 	completedFrame = workingFrame;
// 	completedFrameSet = true;
// 	workingFrameSet = false;
// 	completedFrameTimestamps = workingFrameTimestamps;
// 	if(workingPreviewFrameSet) {
// 		completedPreviewFrameSource = workingPreviewFrame;
// 	} else {
// 		completedPreviewFrameSource = completedFrame;
// 	}
// 	completedPreviewFrameSet = false;
// 	workingPreviewFrameSet = false;
// 	YerFace_MutexUnlock(myMutex);
// }

// ClassificationFrame FrameDerivatives::getClassificationFrame(void) {
// 	YerFace_MutexLock(myMutex);
// 	ClassificationFrame result;
// 	result.timestamps.set = false;
// 	result.set = false;
// 	if(!workingFrameSet || !workingFrameTimestamps.set) {
// 		YerFace_MutexUnlock(myMutex);
// 		return result;
// 	}
// 	result.timestamps = workingFrameTimestamps;
// 	result.frame = classificationFrame;
// 	result.scaleFactor = classificationScaleFactor;
// 	result.set = true;
// 	YerFace_MutexUnlock(myMutex);
// 	return result;
// }

// Mat FrameDerivatives::getWorkingPreviewFrame(void) {
// 	YerFace_MutexLock(myMutex);
// 	if(!workingFrameSet) {
// 		YerFace_MutexUnlock(myMutex);
// 		throw runtime_error("getWorkingPreviewFrame() called, but no working frame set");
// 	}
// 	if(!workingPreviewFrameSet) {
// 		workingPreviewFrame = workingFrame.clone();
// 		workingPreviewFrameSet = true;
// 	}
// 	Mat value = workingPreviewFrame;
// 	YerFace_MutexUnlock(myMutex);
// 	return value;
// }

// Mat FrameDerivatives::getCompletedPreviewFrame(void) {
// 	YerFace_MutexLock(myMutex);
// 	if(!completedFrameSet) {
// 		YerFace_MutexUnlock(myMutex);
// 		throw runtime_error("getCompletedPreviewFrame() called, but no completed frame set");
// 	}
// 	if(!completedPreviewFrameSet) {
// 		completedPreviewFrame = completedPreviewFrameSource.clone();
// 		completedPreviewFrameSet = true;
// 	}
// 	Mat value = completedPreviewFrame;
// 	YerFace_MutexUnlock(myMutex);
// 	return value;
// }

// void FrameDerivatives::resetCompletedPreviewFrame(void) {
// 	YerFace_MutexLock(myMutex);
// 	completedPreviewFrameSet = false;
// 	YerFace_MutexUnlock(myMutex);
// }

// Size FrameDerivatives::getWorkingFrameSize(void) {
// 	YerFace_MutexLock(myMutex);
// 	if(!workingFrameSizeSet) {
// 		YerFace_MutexUnlock(myMutex);
// 		throw runtime_error("getWorkingFrameSize() called, but no cached working frame size");
// 	}
// 	Size size = workingFrameSize;
// 	YerFace_MutexUnlock(myMutex);
// 	return size;
// }

// FrameTimestamps FrameDerivatives::getWorkingFrameTimestamps(void) {
// 	YerFace_MutexLock(myMutex);
// 	if(!workingFrameTimestamps.set) {
// 		YerFace_MutexUnlock(myMutex);
// 		throw runtime_error("getWorkingFrameTimestamps() called, but no working frame timestamps available");
// 	}
// 	FrameTimestamps timestamps = workingFrameTimestamps;
// 	YerFace_MutexUnlock(myMutex);
// 	return timestamps;
// }

// FrameTimestamps FrameDerivatives::getCompletedFrameTimestamps(void) {
// 	YerFace_MutexLock(myMutex);
// 	if(!completedFrameTimestamps.set) {
// 		YerFace_MutexUnlock(myMutex);
// 		throw runtime_error("getCompletedFrameTimestamps() called, but no timestamps set");
// 	}
// 	FrameTimestamps timestamps = completedFrameTimestamps;
// 	YerFace_MutexUnlock(myMutex);
// 	return timestamps;
// }

// bool FrameDerivatives::getCompletedFrameSet(void) {
// 	YerFace_MutexLock(myMutex);
// 	bool status = completedFrameSet;
// 	YerFace_MutexUnlock(myMutex);
// 	return status;
// }

}; //namespace YerFace
