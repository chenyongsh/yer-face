
#include "FaceTracker.hpp"
#include "Utilities.hpp"

#include "opencv2/calib3d.hpp"
#include "opencv2/highgui.hpp"

#include <exception>
#include <cmath>
#include <cstdio>
#include <cstdlib>

using namespace std;
using namespace dlib;

namespace YerFace {

FaceTracker::FaceTracker(string myModelFileName, FrameDerivatives *myFrameDerivatives) {
	modelFileName = myModelFileName;
	trackerState = DETECTING;
	classificationBoxSet = false;
	facialFeaturesSet = false;
	facialFeatures3dSet = false;
	cameraModelSet = false;
	poseSet = false;

	frameDerivatives = myFrameDerivatives;
	if(frameDerivatives == NULL) {
		throw invalid_argument("frameDerivatives cannot be NULL");
	}
	frontalFaceDetector = get_frontal_face_detector();
	deserialize(modelFileName.c_str()) >> shapePredictor;
	fprintf(stderr, "FaceTracker object constructed and ready to go!\n");
}

FaceTracker::~FaceTracker() {
	fprintf(stderr, "FaceTracker object destructing...\n");
}

TrackerState FaceTracker::processCurrentFrame(void) {
	Mat classificationFrame = frameDerivatives->getClassificationFrame();
	dlibClassificationFrame = cv_image<bgr_pixel>(classificationFrame);

	doClassifyFace();

	if(!classificationBoxSet) {
		return trackerState;
	}

	doIdentifyFeatures();

	doCalculateFacialTransformation();

	return trackerState;
}

void FaceTracker::doClassifyFace(void) {
	classificationBoxSet = false;
	//Using dlib's built-in HOG face detector instead of a CNN-based detector because it trades off accuracy for speed.
	std::vector<dlib::rectangle> faces = frontalFaceDetector(dlibClassificationFrame);

	int largestFace = -1;
	int largestFaceArea = -1;
	size_t facesCount = faces.size();
	for(size_t i = 0; i < facesCount; i++) {
		if((int)faces[i].area() > largestFaceArea) {
			largestFace = i;
			largestFaceArea = faces[i].area();
		}
	}
	if(largestFace >= 0) {
		trackerState = TRACKING;
		classificationBox.x = faces[largestFace].left();
		classificationBox.y = faces[largestFace].top();
		classificationBox.width = faces[largestFace].right() - classificationBox.x;
		classificationBox.height = faces[largestFace].bottom() - classificationBox.y;
		double classificationScaleFactor = frameDerivatives->getClassificationScaleFactor();
		classificationBoxNormalSize = Utilities::scaleRect(classificationBox, 1.0 / classificationScaleFactor);
		classificationBoxDlib = faces[largestFace];
		classificationBoxSet = true;
	} else {
		if(trackerState != DETECTING) {
			trackerState = LOST;
		}
	}
}

void FaceTracker::doIdentifyFeatures(void) {
	facialFeaturesSet = false;

	full_object_detection result = shapePredictor(dlibClassificationFrame, classificationBoxDlib);

	Mat prevFrame = frameDerivatives->getPreviewFrame();

	facialFeatures.clear();
	if(!facialFeatures3dSet) {
		facialFeatures3d.clear();
	}
	dlib::point part;
	std::vector<int> featureIndexes = {IDX_NOSE_SELLION, IDX_EYE_RIGHT_OUTER_CORNER, IDX_EYE_LEFT_OUTER_CORNER, IDX_JAW_RIGHT_TOP, IDX_JAW_LEFT_TOP, IDX_NOSE_TIP, IDX_MENTON};
	for(int featureIndex : featureIndexes) {
		part = result.part(featureIndex);
		Point2d partPoint;
		if(!doConvertLandmarkPointToImagePoint(&part, &partPoint)) {
			trackerState = LOST;
			return;
		}

		facialFeatures.push_back(partPoint);
		if(!facialFeatures3dSet) {
			switch(featureIndex) {
				default:
					throw logic_error("bad facial feature index");
				case IDX_NOSE_SELLION:
					facialFeatures3d.push_back(VERTEX_NOSE_SELLION);
					break;
				case IDX_EYE_RIGHT_OUTER_CORNER:
					facialFeatures3d.push_back(VERTEX_EYE_RIGHT_OUTER_CORNER);
					break;
				case IDX_EYE_LEFT_OUTER_CORNER:
					facialFeatures3d.push_back(VERTEX_EYE_LEFT_OUTER_CORNER);
					break;
				case IDX_JAW_RIGHT_TOP:
					facialFeatures3d.push_back(VERTEX_RIGHT_EAR);
					break;
				case IDX_JAW_LEFT_TOP:
					facialFeatures3d.push_back(VERTEX_LEFT_EAR);
					break;
				case IDX_NOSE_TIP:
					facialFeatures3d.push_back(VERTEX_NOSE_TIP);
					break;
				case IDX_MENTON:
					facialFeatures3d.push_back(VERTEX_MENTON);
					break;
			}
		}
	}

	//Stommion needs a little extra help.
	part = result.part(IDX_MOUTH_CENTER_INNER_TOP);
	Point2d mouthTop;
	if(!doConvertLandmarkPointToImagePoint(&part, &mouthTop)) {
		trackerState = LOST;
		return;
	}
	part = result.part(IDX_MOUTH_CENTER_INNER_BOTTOM);
	Point2d mouthBottom;
	if(!doConvertLandmarkPointToImagePoint(&part, &mouthBottom)) {
		trackerState = LOST;
		return;
	}
	facialFeatures.push_back((mouthTop + mouthBottom) * 0.5);
	if(!facialFeatures3dSet) {
		facialFeatures3d.push_back(VERTEX_STOMMION);
		facialFeatures3dSet = true;
	}
	facialFeaturesSet = true;
}

void FaceTracker::doInitializeCameraModel(void) {
	//Totally fake, idealized camera.
	Size frameSize = frameDerivatives->getCurrentFrameSize();
	double focalLength = frameSize.width;
	Point2d center = Point2d(frameSize.width / 2, frameSize.height / 2);
	cameraMatrix = (Mat_<double>(3, 3) <<
			focalLength, 0.0, center.x,
			0.0, focalLength, center.y,
			0.0, 0.0, 1.0);
	distortionCoefficients = Mat::zeros(4, 1, DataType<double>::type);
	cameraModelSet = true;
}

// Pose recovery approach largely informed by the following sources:
//  - https://www.learnopencv.com/head-pose-estimation-using-opencv-and-dlib/
//  - https://github.com/severin-lemaignan/gazr/
void FaceTracker::doCalculateFacialTransformation(void) {
	if(!facialFeaturesSet) {
		return;
	}
	if(!cameraModelSet) {
		doInitializeCameraModel();
	}

	solvePnP(facialFeatures3d, facialFeatures, cameraMatrix, distortionCoefficients, poseRotationVector, poseTranslationVector);
	poseSet = true;

	Mat rot_mat;
	Rodrigues(poseRotationVector, rot_mat);
	Vec3d angles = Utilities::rotationMatrixToEulerAngles(rot_mat);
	fprintf(stderr, "pose angle: <%.02f, %.02f, %.02f>\n", angles[0], angles[1], angles[2]);
}

bool FaceTracker::doConvertLandmarkPointToImagePoint(dlib::point *src, Point2d *dst) {
	static double classificationScaleFactor = frameDerivatives->getClassificationScaleFactor();
	if(*src == OBJECT_PART_NOT_PRESENT) {
		return false;
	}
	*dst = Point2d(src->x(), src->y());
	dst->x /= classificationScaleFactor;
	dst->y /= classificationScaleFactor;
	return true;
}

void FaceTracker::renderPreviewHUD(bool verbose) {
	Mat frame = frameDerivatives->getPreviewFrame();
	if(verbose) {
		if(classificationBoxSet) {
			cv::rectangle(frame, classificationBoxNormalSize, Scalar(0, 255, 0), 1);
		}
	}
	if(facialFeaturesSet) {
		size_t featuresCount = facialFeatures.size();
		for(size_t i = 0; i < featuresCount; i++) {
			Utilities::drawX(frame, facialFeatures[i], Scalar(0, 255, 0));
		}
	}
	if(poseSet) {
		std::vector<Point3d> gizmo3d(6);
		std::vector<Point2d> gizmo2d;
		gizmo3d[0] = Point3d(-50,0.0,0.0);
		gizmo3d[1] = Point3d(50,0.0,0.0);
		gizmo3d[2] = Point3d(0.0,-50,0.0);
		gizmo3d[3] = Point3d(0.0,50,0.0);
		gizmo3d[4] = Point3d(0.0,0.0,-50);
		gizmo3d[5] = Point3d(0.0,0.0,50);
		projectPoints(gizmo3d, poseRotationVector, poseTranslationVector, cameraMatrix, distortionCoefficients, gizmo2d);
		line(frame, gizmo2d[0], gizmo2d[1], Scalar(0, 0, 255), 1);
		line(frame, gizmo2d[2], gizmo2d[3], Scalar(0, 255, 0), 1);
		line(frame, gizmo2d[4], gizmo2d[5], Scalar(255, 0, 0), 1);
	}
}

TrackerState FaceTracker::getTrackerState(void) {
	return trackerState;
}

}; //namespace YerFace
