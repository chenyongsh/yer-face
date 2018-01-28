
#include "Metrics.hpp"
#include "Utilities.hpp"

using namespace std;
using namespace cv;

namespace YerFace {

Metrics::Metrics(const char *myName, FrameDerivatives *myFrameDerivatives, bool myMetricIsFrames, double myAverageOverSeconds, double myReportEverySeconds) {
	name = (string)myName;
	metricIsFrames = myMetricIsFrames;
	frameDerivatives = myFrameDerivatives;
	if(frameDerivatives == NULL) {
		throw invalid_argument("frameDerivatives cannot be NULL");
	}
	averageOverSeconds = myAverageOverSeconds;
	reportEverySeconds = myReportEverySeconds;
	lastReport = -1.0 - myReportEverySeconds;
	averageTimeSeconds = 0.0;
	worstTimeSeconds = 0.0;
	fps = 0.0;
	snprintf(timesString, METRICS_STRING_LENGTH, "N/A");
	snprintf(fpsString, METRICS_STRING_LENGTH, "N/A");
	string loggerName = "Metrics<" + name + ">";
	logger = new Logger(loggerName.c_str());
	if((myMutex = SDL_CreateMutex()) == NULL) {
		throw runtime_error("Failed creating mutex!");
	}
	logger->debug("Metrics object constructed and ready to go!");
}

Metrics::~Metrics() {
	logger->debug("Metrics object destructing...");
	SDL_DestroyMutex(myMutex);
	delete logger;
}

void Metrics::startClock(void) {
	YerFace_MutexLock(myMutex);
	MetricEntry entry;
	entry.startTime = (double)getTickCount() / (double)getTickFrequency();
	entries.push_front(entry);
	YerFace_MutexUnlock(myMutex);
}

void Metrics::endClock(void) {
	YerFace_MutexLock(myMutex);
	double now = (double)getTickCount() / (double)getTickFrequency();

	double frameTimestamp = frameDerivatives->getWorkingFrameTimestamp();

	entries.front().runTime = now - entries.front().startTime;
	entries.front().frameTimestamp = frameTimestamp;

	while(entries.back().frameTimestamp <= (frameTimestamp - averageOverSeconds)) {
		entries.pop_back();
	}

	averageTimeSeconds = 0.0;
	worstTimeSeconds = 0.0;
	size_t numEntries = entries.size();
	for(MetricEntry entry : entries) {
		averageTimeSeconds = averageTimeSeconds + entry.runTime;
		if(entry.runTime > worstTimeSeconds) {
			worstTimeSeconds = entry.runTime;
		}
	}
	averageTimeSeconds = averageTimeSeconds / (double)numEntries;
	snprintf(timesString, METRICS_STRING_LENGTH, "Times: <Avg %.02fms, Worst %.02fms>", averageTimeSeconds * 1000.0, worstTimeSeconds * 1000.0);
	if(metricIsFrames) {
		fps = 1.0 / ((now - entries.back().startTime) / numEntries);
		snprintf(fpsString, METRICS_STRING_LENGTH, "FPS: <%.02f>", fps);
	}

	if(lastReport + reportEverySeconds <= now) {
		if(metricIsFrames) {
			logger->verbose("%s %s", fpsString, timesString);
		} else {
			logger->verbose("%s", timesString);
		}
		lastReport = now;
	}
	YerFace_MutexUnlock(myMutex);
}

double Metrics::getAverageTimeSeconds(void) {
	YerFace_MutexLock(myMutex);
	double status = averageTimeSeconds;
	YerFace_MutexUnlock(myMutex);
	return status;
}

double Metrics::getWorstTimeSeconds(void) {
	YerFace_MutexLock(myMutex);
	double status = worstTimeSeconds;
	YerFace_MutexUnlock(myMutex);
	return status;
}

double Metrics::getFPS(void) {
	YerFace_MutexLock(myMutex);
	double status = fps;
	YerFace_MutexUnlock(myMutex);
	return status;
}

std::string Metrics::getTimesString(void) {
	YerFace_MutexLock(myMutex);
	std::string str = (std::string)timesString;
	YerFace_MutexUnlock(myMutex);
	return str;
}

std::string Metrics::getFPSString(void) {
	YerFace_MutexLock(myMutex);
	std::string str = (std::string)fpsString;
	YerFace_MutexUnlock(myMutex);
	return str;
}

} //namespace YerFace
