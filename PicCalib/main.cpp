#include <iostream>
#include "Thirdparty/ddUtil.h"

#include <opencv2/core.hpp>
#include <opencv2/core/utility.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
using namespace cv;
using namespace std;
#include "Settings.h"

enum { DETECTION = 0, CAPTURING = 1, CALIBRATED = 2 };

bool runCalibrationAndSave(Settings& s, Size imageSize, Mat&  cameraMatrix, Mat& distCoeffs,
	std::vector<std::vector<cv::Point2f> > imagePoints);

int main(int argc, char** argv) {
	std::string cur_dir = exe_path();
	std::cout << cur_dir  <<std::endl;
	//std::cout <<  cv::getBuildInformation().c_str() << endl;
	
	std::string str1 = string_format("%d", 3);
	std::string fnbasis = get_timestring();
	//std::string fnbasis = currentDateTime();// GetTimeString();
	std::string outdir = "../out_"+fnbasis;

	mkdir_if_not_exist(outdir);

	Settings s;
	const std::string inputSettingsFile = "../default.xml";
	FileStorage fs(inputSettingsFile, FileStorage::READ); // Read the settings
	if (!fs.isOpened())
	{
		std::cout << "Could not open the configuration file: \"" << inputSettingsFile << "\"" << std::endl;
		getchar();
		return -1;
	}
	else
	{
		std::cout << "Loaded configuration file: \"" << inputSettingsFile << "\"" << std::endl;
	}

	fs["Settings"] >> s;
	fs.release();                                         // close Settings file
														  //! [file_read]

	//FileStorage fout("settings.yml", FileStorage::WRITE); // write config as YAML
	//fout << "Settings" << s;

	if (!s.goodInput)
	{
		cout << "\n\nInvalid input detected. Application stopping. " << endl;
		getchar();
		return -1;
	}
	vector<vector<Point2f> > imagePoints;
	Mat cameraMatrix, distCoeffs;
	Size imageSize;
	int mode = s.inputType == Settings::IMAGE_LIST ? CAPTURING : DETECTION;
	clock_t prevTimestamp = 0;
	const Scalar RED(0, 0, 255), GREEN(0, 255, 0);
	const char ESC_KEY = 27;

	cout << "inputType = " << mode << endl;
	cout << "boardSize = " << s.boardSize.width<< " x " <<s.boardSize.height << endl;

	for (int i=0;i<s.nFramesTotal;i++)
	{
		Mat view;
		bool blinkOutput = false;

		view = s.nextImage();
		cout <<i <<  "\t" << view.size().width << " x " << view.size().height << " x " << view.channels() ;
		//Mat viewGray, view2;
		Mat view2;
		char key1 = (char)waitKey(1); //s.delay

		if (key1 == ESC_KEY)
			break;

		//continue;

		//-----  If no more image, or got enough, then stop calibration and show result -------------
		if (mode == CAPTURING && imagePoints.size() >= (size_t)s.nrFrames)
		{
			if (runCalibrationAndSave(s, imageSize, cameraMatrix, distCoeffs, imagePoints))
				mode = CALIBRATED;
			else
				mode = DETECTION;

			//break;
		}
		if (view.empty())          // If there are no more images stop the loop
		{
			// if calibration threshold was not reached yet, calibrate now
			if (mode != CALIBRATED && !imagePoints.empty())
				runCalibrationAndSave(s, imageSize, cameraMatrix, distCoeffs, imagePoints);
			break;
		}
		//! [get_input]

		imageSize = view.size();  // Format input image.
		if (s.flipVertical)    flip(view, view, 0);

		//! [find_pattern]
		vector<Point2f> pointBuf;

		bool found;

		int chessBoardFlags = CALIB_CB_ADAPTIVE_THRESH | CALIB_CB_NORMALIZE_IMAGE;

		if (!s.useFisheye) {
			// fast check erroneously fails with high distortions like fisheye
			chessBoardFlags |= CALIB_CB_FAST_CHECK;
		}

		switch (s.calibrationPattern) // Find feature points on the input format
		{
		case Settings::CHESSBOARD:
			found = findChessboardCorners(view, s.boardSize, pointBuf, chessBoardFlags);
			break;
		case Settings::CIRCLES_GRID:
			found = findCirclesGrid(view, s.boardSize, pointBuf);
			break;
		case Settings::ASYMMETRIC_CIRCLES_GRID:
			found = findCirclesGrid(view, s.boardSize, pointBuf, CALIB_CB_ASYMMETRIC_GRID);
			break;
		default:
			found = false;
			break;
		}
		if (0)                // If done with success,
		{
			drawChessboardCorners(view, s.boardSize, Mat(pointBuf), found);
			cvNamedWindow("window");
			imshow("window", view);
			cvWaitKey(30);
		}

		//! [find_pattern]
		//! [pattern_found]
		if (found)                // If done with success,
		{
			// improve the found corners' coordinate accuracy for chessboard
			if (s.calibrationPattern == Settings::CHESSBOARD)
			{
				Mat viewGray;
				cvtColor(view, viewGray, COLOR_BGR2GRAY);
				cvtColor(viewGray, view2, COLOR_GRAY2BGR); //for display
				cornerSubPix(viewGray, pointBuf, Size(11, 11),
					Size(-1, -1), TermCriteria(TermCriteria::EPS + TermCriteria::COUNT, 30, 0.1));
				
			}
			if (mode == CAPTURING &&  // For camera only take new samples after delay time
				(!s.inputCapture.isOpened() || clock() - prevTimestamp > s.delay*1e-3*CLOCKS_PER_SEC))
			{
				imagePoints.push_back(pointBuf);
				prevTimestamp = clock();
				blinkOutput = s.inputCapture.isOpened();
			}
			cout << "... OK" << endl;
			// Draw the corners.
			drawChessboardCorners(view2, s.boardSize, Mat(pointBuf), found);
		}
		else
		{
			Mat viewGray;
			cvtColor(view, viewGray, COLOR_BGR2GRAY);
			cvtColor(viewGray, view2, COLOR_GRAY2BGR); //for display
			cout << endl;
		}
		//! [pattern_found]
		//----------------------------- Output Text ------------------------------------------------
		//! [output_text]
		string msg = (mode == CAPTURING) ? "100/100" :
			mode == CALIBRATED ? "Calibrated" : "Press 'g' to start";
		int baseLine = 0;
		Size textSize = getTextSize(msg, 1, 1, 1, &baseLine);
		Point textOrigin(view.cols - 2 * textSize.width - 10, view.rows - 2 * baseLine - 10);

		if (mode == CAPTURING)
		{
			msg = format("%d/%d....%d", (int)imagePoints.size(), s.nrFrames, found);
		}

		putText(view2, msg, textOrigin, 1, 1, mode == CALIBRATED ? GREEN : RED);
		string fnout = outdir + format("/%d.jpg", i);
		cout << fnout << endl;
		imwrite(fnout, view2);

		imshow("Image View", view2);
		char key = (char)waitKey(s.inputCapture.isOpened() ? 50 : s.delay);

		if (key == ESC_KEY)
			break;

		// Reset
		if (s.inputCapture.isOpened() && key == 'g')
		{
			mode = CAPTURING;
			imagePoints.clear();
		}

		//! [await_input]
	}

	if (s.inputType == Settings::IMAGE_LIST && s.showUndistorsed)
	{
		Mat view, rview, map1, map2;

		if (s.useFisheye)
		{
			Mat newCamMat;
			fisheye::estimateNewCameraMatrixForUndistortRectify(cameraMatrix, distCoeffs, imageSize,
				Matx33d::eye(), newCamMat, 1);
			fisheye::initUndistortRectifyMap(cameraMatrix, distCoeffs, Matx33d::eye(), newCamMat, imageSize,
				CV_16SC2, map1, map2);
		}
		else
		{
			Mat cm1 = getOptimalNewCameraMatrix(cameraMatrix, distCoeffs, imageSize, 1, imageSize, 0);

			initUndistortRectifyMap(
				cameraMatrix, distCoeffs, Mat(),
				cm1, imageSize,
				CV_16SC2, map1, map2);
		}

		for (size_t i = 0; i < s.imageList.size(); i++)
		{
			string fname = s.imageList[i];
			view = imread(fname, IMREAD_COLOR); //
			if (view.empty())
				continue;
			remap(view, rview, map1, map2, INTER_LINEAR);

			string fnout = outdir + format("/%d_undist.jpg", i);
			cout << fnout << endl;
			imwrite(fnout, rview);

			fnout = outdir + format("/%d_.jpg", i);
			cout << fnout << endl;
			imwrite(fnout, view);

			imshow("Image View", rview);
			char c = (char)waitKey();
			if (c == ESC_KEY || c == 'q' || c == 'Q')
				break;
		}
	}
	//! [show_results]

	destroyAllWindows();
	printf("== Main End! ==");
	getchar();

	return 0;
}



//! [compute_errors]
static double computeReprojectionErrors(const vector<vector<Point3f> >& objectPoints,
	const vector<vector<Point2f> >& imagePoints,
	const vector<Mat>& rvecs, const vector<Mat>& tvecs,
	const Mat& cameraMatrix, const Mat& distCoeffs,
	vector<float>& perViewErrors, bool fisheye)
{
	vector<Point2f> imagePoints2;
	size_t totalPoints = 0;
	double totalErr = 0, err;
	perViewErrors.resize(objectPoints.size());

	for (size_t i = 0; i < objectPoints.size(); ++i)
	{
		if (fisheye)
		{
			fisheye::projectPoints(objectPoints[i], imagePoints2, rvecs[i], tvecs[i], cameraMatrix,
				distCoeffs);
		}
		else
		{
			projectPoints(objectPoints[i], rvecs[i], tvecs[i], cameraMatrix, distCoeffs, imagePoints2);
		}
		err = norm(imagePoints[i], imagePoints2, NORM_L2);

		size_t n = objectPoints[i].size();
		perViewErrors[i] = (float)std::sqrt(err*err / n);
		totalErr += err*err;
		totalPoints += n;
	}

	return std::sqrt(totalErr / totalPoints);
}
//! [compute_errors]
//! [board_corners]
static void calcBoardCornerPositions(Size boardSize, float squareSize, vector<Point3f>& corners,
	Settings::Pattern patternType /*= Settings::CHESSBOARD*/)
{
	corners.clear();

	switch (patternType)
	{
	case Settings::CHESSBOARD:
	case Settings::CIRCLES_GRID:
		for (int i = 0; i < boardSize.height; ++i)
			for (int j = 0; j < boardSize.width; ++j)
				corners.push_back(Point3f(j*squareSize, i*squareSize, 0));
		break;

	case Settings::ASYMMETRIC_CIRCLES_GRID:
		for (int i = 0; i < boardSize.height; i++)
			for (int j = 0; j < boardSize.width; j++)
				corners.push_back(Point3f((2 * j + i % 2)*squareSize, i*squareSize, 0));
		break;
	default:
		break;
	}
}
//! [board_corners]
static bool runCalibration(Settings& s, Size& imageSize, Mat& cameraMatrix, Mat& distCoeffs,
	vector<vector<Point2f> > imagePoints, vector<Mat>& rvecs, vector<Mat>& tvecs,
	vector<float>& reprojErrs, double& totalAvgErr)
{
	//! [fixed_aspect]
	cameraMatrix = Mat::eye(3, 3, CV_64F);
	if (s.flag & CALIB_FIX_ASPECT_RATIO)
		cameraMatrix.at<double>(0, 0) = s.aspectRatio;
	//! [fixed_aspect]
	if (s.useFisheye) {
		distCoeffs = Mat::zeros(4, 1, CV_64F);
	}
	else {
		distCoeffs = Mat::zeros(8, 1, CV_64F);
	}

	vector<vector<Point3f> > objectPoints(1);
	calcBoardCornerPositions(s.boardSize, s.squareSize, objectPoints[0], s.calibrationPattern);

	objectPoints.resize(imagePoints.size(), objectPoints[0]);

	//Find intrinsic and extrinsic camera parameters
	double rms;

	if (s.useFisheye) {
		Mat _rvecs, _tvecs;
		rms = fisheye::calibrate(objectPoints, imagePoints, imageSize, cameraMatrix, distCoeffs, _rvecs,
			_tvecs, s.flag);

		rvecs.reserve(_rvecs.rows);
		tvecs.reserve(_tvecs.rows);
		for (int i = 0; i < int(objectPoints.size()); i++) {
			rvecs.push_back(_rvecs.row(i));
			tvecs.push_back(_tvecs.row(i));
		}
	}
	else {
		rms = calibrateCamera(objectPoints, imagePoints, imageSize, cameraMatrix, distCoeffs, rvecs, tvecs,
			s.flag);
	}

	
	cout << "Re-projection error reported by calibrateCamera: " << rms << endl;

	bool ok = checkRange(cameraMatrix) && checkRange(distCoeffs);

	totalAvgErr = computeReprojectionErrors(objectPoints, imagePoints, rvecs, tvecs, cameraMatrix,
		distCoeffs, reprojErrs, s.useFisheye);

	
	
	return ok;
}

// Print camera parameters to the output file
static void saveCameraParams(Settings& s, Size& imageSize, Mat& cameraMatrix, Mat& distCoeffs,
	const vector<Mat>& rvecs, const vector<Mat>& tvecs,
	const vector<float>& reprojErrs, const vector<vector<Point2f> >& imagePoints,
	double totalAvgErr)
{
	string fnout;
	if (s.inputType == Settings::IMAGE_LIST)
		fnout = s.input + "/" + s.outputFileName;
	else
		fnout = s.outputFileName;

	FileStorage fs(fnout, FileStorage::WRITE);

	time_t tm;
	time(&tm);
	struct tm *t2 = localtime(&tm);
	char buf[1024];
	strftime(buf, sizeof(buf), "%c", t2);

	fs << "calibration_time" << buf;

	if (!rvecs.empty() || !reprojErrs.empty())
		fs << "nr_of_frames" << (int)std::max(rvecs.size(), reprojErrs.size());
	fs << "image_width" << imageSize.width;
	fs << "image_height" << imageSize.height;
	fs << "board_width" << s.boardSize.width;
	fs << "board_height" << s.boardSize.height;
	fs << "square_size" << s.squareSize;

	if (s.flag & CALIB_FIX_ASPECT_RATIO)
		fs << "fix_aspect_ratio" << s.aspectRatio;

	if (s.flag)
	{
		std::stringstream flagsStringStream;
		if (s.useFisheye)
		{
			flagsStringStream << "flags:"
				<< (s.flag & fisheye::CALIB_FIX_SKEW ? " +fix_skew" : "")
				<< (s.flag & fisheye::CALIB_FIX_K1 ? " +fix_k1" : "")
				<< (s.flag & fisheye::CALIB_FIX_K2 ? " +fix_k2" : "")
				<< (s.flag & fisheye::CALIB_FIX_K3 ? " +fix_k3" : "")
				<< (s.flag & fisheye::CALIB_FIX_K4 ? " +fix_k4" : "")
				<< (s.flag & fisheye::CALIB_RECOMPUTE_EXTRINSIC ? " +recompute_extrinsic" : "");
		}
		else
		{
			flagsStringStream << "flags:"
				<< (s.flag & CALIB_USE_INTRINSIC_GUESS ? " +use_intrinsic_guess" : "")
				<< (s.flag & CALIB_FIX_ASPECT_RATIO ? " +fix_aspectRatio" : "")
				<< (s.flag & CALIB_FIX_PRINCIPAL_POINT ? " +fix_principal_point" : "")
				<< (s.flag & CALIB_ZERO_TANGENT_DIST ? " +zero_tangent_dist" : "")
				<< (s.flag & CALIB_FIX_K1 ? " +fix_k1" : "")
				<< (s.flag & CALIB_FIX_K2 ? " +fix_k2" : "")
				<< (s.flag & CALIB_FIX_K3 ? " +fix_k3" : "")
				<< (s.flag & CALIB_FIX_K4 ? " +fix_k4" : "")
				<< (s.flag & CALIB_FIX_K5 ? " +fix_k5" : "");
		}
		fs.writeComment(flagsStringStream.str());
	}

	fs << "flags" << s.flag;

	fs << "fisheye_model" << s.useFisheye;

	fs << "camera_matrix" << cameraMatrix;
	fs << "distortion_coefficients" << distCoeffs;

	fs << "avg_reprojection_error" << totalAvgErr;
	if (s.writeExtrinsics && !reprojErrs.empty())
		fs << "per_view_reprojection_errors" << Mat(reprojErrs);

	if (s.writeExtrinsics && !rvecs.empty() && !tvecs.empty())
	{
		CV_Assert(rvecs[0].type() == tvecs[0].type());
		Mat bigmat((int)rvecs.size(), 6, CV_MAKETYPE(rvecs[0].type(), 1));
		bool needReshapeR = rvecs[0].depth() != 1 ? true : false;
		bool needReshapeT = tvecs[0].depth() != 1 ? true : false;

		for (size_t i = 0; i < rvecs.size(); i++)
		{
			Mat r = bigmat(Range(int(i), int(i + 1)), Range(0, 3));
			Mat t = bigmat(Range(int(i), int(i + 1)), Range(3, 6));

			if (needReshapeR)
				rvecs[i].reshape(1, 1).copyTo(r);
			else
			{
				//*.t() is MatExpr (not Mat) so we can use assignment operator
				CV_Assert(rvecs[i].rows == 3 && rvecs[i].cols == 1);
				r = rvecs[i].t();
			}

			if (needReshapeT)
				tvecs[i].reshape(1, 1).copyTo(t);
			else
			{
				CV_Assert(tvecs[i].rows == 3 && tvecs[i].cols == 1);
				t = tvecs[i].t();
			}
		}
		fs.writeComment("a set of 6-tuples (rotation vector + translation vector) for each view");
		fs << "extrinsic_parameters" << bigmat;
	}

	if (s.writePoints && !imagePoints.empty())
	{
		Mat imagePtMat((int)imagePoints.size(), (int)imagePoints[0].size(), CV_32FC2);
		for (size_t i = 0; i < imagePoints.size(); i++)
		{
			Mat r = imagePtMat.row(int(i)).reshape(2, imagePtMat.cols);
			Mat imgpti(imagePoints[i]);
			imgpti.copyTo(r);
		}
		fs << "image_points" << imagePtMat;
	}
}

bool runCalibrationAndSave(Settings& s, Size imageSize, Mat& cameraMatrix, Mat& distCoeffs,
	vector<vector<Point2f> > imagePoints)
{
	vector<Mat> rvecs, tvecs;
	vector<float> reprojErrs;
	double totalAvgErr = 0;

	bool ok = runCalibration(s, imageSize, cameraMatrix, distCoeffs, imagePoints, rvecs, tvecs, reprojErrs,
		totalAvgErr);
	cout << (ok ? "Calibration succeeded" : "Calibration failed")
		<< ". avg re projection error = " << totalAvgErr << endl;

	if (ok)
		saveCameraParams(s, imageSize, cameraMatrix, distCoeffs, rvecs, tvecs, reprojErrs, imagePoints,
			totalAvgErr);
	return ok;
}