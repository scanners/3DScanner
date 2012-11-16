#include <cv.h>
#include <highgui.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>

using namespace std;
using namespace cv;

int numBoards = 0; //Will be set by input list
const int boardDT = 10;
int numInternalCornersAcross;
int numInternalCornersDown;
int found;

void help(){
	printf("Calibration from disk. Call convention:\n\n"
		"  ch11_ex11_1_fromdisk numInternalCornersAcross numInternalCornersDown image_list\n\n"
		"Where: board_{w,h} are the # of internal corners in the checkerboard\n"
		"       width (numInternalCornersAcross) and height (numInternalCornersDown)\n"
		"       image_list is space separated list of path/filename of checkerboard\n"
		"       images\n\n"
		"Hit 'p' to pause/unpause, ESC to quit.  After calibration, press any other key to step through the images\n\n");
}

int main(int argc, char* argv[]) {

	CvCapture* capture;

	if(argc != 4){
		help();
		return -1;
	}

	help();

	numInternalCornersAcross = atoi(argv[1]);
	numInternalCornersDown = atoi(argv[2]);

	int numInternalCorners  = numInternalCornersAcross * numInternalCornersDown;
	CvSize board_sz = cvSize( numInternalCornersAcross, numInternalCornersDown );
	FILE *fptr = fopen(argv[3],"r");
	char names[2048];

	//COUNT THE NUMBER OF IMAGES:
	while(fscanf(fptr,"%s ",names)==1){
		numBoards++;
	}

	rewind(fptr);

	//ALLOCATE STORAGE
	CvMat* image_points      = cvCreateMat(numBoards*numInternalCorners,2,CV_32FC2);
	CvMat* object_points     = cvCreateMat(numBoards*numInternalCorners,3,CV_32FC2);
	CvMat* point_counts      = cvCreateMat(numBoards,1,CV_32SC2);

	CvMat* intrinsic_matrix  = cvCreateMat(3,3,CV_32FC2);
	CvMat* distortion_coeffs = cvCreateMat(4,1,CV_32FC2);


	IplImage* image = 0;
	IplImage* gray_image = 0; //for subpixel
	CvPoint2D32f* corners = new CvPoint2D32f[ numInternalCorners ];
	int corner_count;
	int successes = 0;
	int step;

	for( int frame=0; frame<numBoards; frame++ ) {
		fscanf(fptr,"%s ",names);

		if(image){
			cvReleaseImage(&image);
			image = 0;
		}
		image = cvLoadImage( names);
		if(gray_image == 0  && image) //We'll need this for subpixel accurate stuff
			gray_image = cvCreateImage(cvGetSize(image),8,1);

		if(!image)
			printf("null image\n");

		found = cvFindChessboardCorners(
			image,
			board_sz,
			corners,
			&corner_count, 
			CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_FILTER_QUADS
			);

		//Get Subpixel accuracy on those corners
		cvCvtColor(image, gray_image, CV_BGR2GRAY);
		cvFindCornerSubPix(gray_image, corners, corner_count, 
			cvSize(11,11),cvSize(-1,-1), cvTermCriteria( CV_TERMCRIT_EPS+CV_TERMCRIT_ITER, 30, 0.1 ));
		//Draw it

		cvDrawChessboardCorners(image, board_sz, corners, corner_count, found);
		cvShowImage( "Calibration", image);

		// If we got a good board, add it to our data
		if( corner_count == numInternalCorners ) {
			step = successes*numInternalCorners;
			for( int i=step, j=0; j<numInternalCorners; ++i,++j ) {
				CV_MAT_ELEM(*image_points, float,i,0) = corners[j].x;
				CV_MAT_ELEM(*image_points, float,i,1) = corners[j].y;
				CV_MAT_ELEM(*object_points,float,i,0) = j/numInternalCornersAcross;
				CV_MAT_ELEM(*object_points,float,i,1) = j%numInternalCornersAcross;
				CV_MAT_ELEM(*object_points,float,i,2) = 0.0f;

			}

			CV_MAT_ELEM(*point_counts, int,successes,0) = numInternalCorners;		
			successes++;
		}

		int c = cvWaitKey(15);
		if(c == 'p') {
			c = 0;
			while(c != 'p' && c != 27){
				c = cvWaitKey(250);
			}
		}
		if(c == 27)
			return 0;
	}

	//ALLOCATE MATRICES ACCORDING TO HOW MANY IMAGES WE FOUND CHESSBOARDS ON
	CvMat* object_points2     = cvCreateMat(successes*numInternalCorners,3,CV_32FC2);
	CvMat* image_points2      = cvCreateMat(successes*numInternalCorners,2,CV_32FC2);
	CvMat* point_counts2      = cvCreateMat(successes,1,CV_32SC1);

	CvMat* points_for_undistortion = cvCreateMat(successes*numInternalCorners,1,CV_32FC2);
	CvMat* undistorted_points = cvCreateMat(successes*numInternalCorners,1,CV_32FC2);

	//TRANSFER THE POINTS INTO THE CORRECT SIZE MATRICES
	for(int i = 0; i<successes*numInternalCorners; ++i){
		CV_MAT_ELEM(*image_points2, float,i,0) 	=	CV_MAT_ELEM(*image_points, float,i,0);
		CV_MAT_ELEM(*image_points2, float,i,1) 	= 	CV_MAT_ELEM(*image_points, float,i,1);
		CV_MAT_ELEM(*object_points2,float,i,0) = CV_MAT_ELEM(*object_points,float,i,0) ;
		CV_MAT_ELEM(*object_points2,float,i,1) = CV_MAT_ELEM(*object_points,float,i,1) ;
		CV_MAT_ELEM(*object_points2,float,i,2) = CV_MAT_ELEM(*object_points,float,i,2) ;
		CV_MAT_ELEM(*points_for_undistortion,CvPoint2D32f,i,0) = corners[i];
		
	} 
	for(int i=0; i<successes; ++i){
		CV_MAT_ELEM(*point_counts2,int,i, 0) = CV_MAT_ELEM(*point_counts, int,i,0);
	}
	cvReleaseMat(&object_points);
	cvReleaseMat(&image_points);
	cvReleaseMat(&point_counts);

	// At this point we have all of the chessboard corners we need.

	// Load test
	CvMat *intrinsic = (CvMat*)cvLoad("Intrinsics.xml");
	CvMat *distortion = (CvMat*)cvLoad("Distortion.xml");

	cvUndistortPoints(
		points_for_undistortion,
		undistorted_points,
		intrinsic,
		distortion
		);

	ofstream myFile;
	ofstream myFile2;
	myFile.open("C:\\Users\\Scanners\\Desktop\\distorted.txt", ios::out);
	myFile2.open("C:\\Users\\Scanners\\Desktop\\undistorted.txt", ios::out);
	for (int i = 0; i < successes*numInternalCorners; i++) {
		myFile << corners[i].x << "," << corners[i].y << "\n";
		myFile2 << CV_MAT_ELEM(*undistorted_points,CvPoint2D32f,i,0).x << "," << CV_MAT_ELEM(*undistorted_points,CvPoint2D32f,i,0).y << "\n";
	}
	myFile.close();
	myFile2.close();

	cvSave("DistortedPoints.xml",points_for_undistortion);
	cvSave("UndistortedPoints.xml",undistorted_points);

	// Just run the camera to the screen, now only showing the undistorted
	// image.
	rewind(fptr);
	cvNamedWindow( "Undistort" );
	printf("\n\nPress any key to step through the images, ESC to quit\n\n");
	while(fscanf(fptr,"%s ",names)==1){
		if(image){
			cvReleaseImage(&image);
			image = 0;
		}  
		image = cvLoadImage( names);
		IplImage *t = cvCloneImage(image);
		cvReleaseImage(&t);
		cvDrawChessboardCorners(image, board_sz, corners, corner_count, found);
		cvShowImage("Undistort", image);
		if((cvWaitKey()&0x7F) == 27) break;  
	}

	return 0;
} 
