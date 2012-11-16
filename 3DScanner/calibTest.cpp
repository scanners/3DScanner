#include <cv.h>
#include <highgui.h>
#include <stdio.h>
#include <stdlib.h>

using namespace cv;

int numBoards = 0; //Will be set by input list
const int boardDT = 10;
int numInternalCornersAcross;
int numInternalCornersDown;

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
	CvMat* image_points      = cvCreateMat(numBoards*numInternalCorners,2,CV_32FC1);
	CvMat* object_points     = cvCreateMat(numBoards*numInternalCorners,3,CV_32FC1);
	CvMat* point_counts      = cvCreateMat(numBoards,1,CV_32SC1);

	CvMat* intrinsic_matrix  = cvCreateMat(3,3,CV_32FC1);
	CvMat* distortion_coeffs = cvCreateMat(4,1,CV_32FC1);


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

		int found = cvFindChessboardCorners(
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
	CvMat* object_points2     = cvCreateMat(successes*numInternalCorners,3,CV_32FC1);
	CvMat* image_points2      = cvCreateMat(successes*numInternalCorners,2,CV_32FC1);
	CvMat* point_counts2      = cvCreateMat(successes,1,CV_32SC1);
	
	//TRANSFER THE POINTS INTO THE CORRECT SIZE MATRICES
	for(int i = 0; i<successes*numInternalCorners; ++i){
		CV_MAT_ELEM(*image_points2, float,i,0) 	=	CV_MAT_ELEM(*image_points, float,i,0);
		CV_MAT_ELEM(*image_points2, float,i,1) 	= 	CV_MAT_ELEM(*image_points, float,i,1);
		CV_MAT_ELEM(*object_points2,float,i,0) = CV_MAT_ELEM(*object_points,float,i,0) ;
		CV_MAT_ELEM(*object_points2,float,i,1) = CV_MAT_ELEM(*object_points,float,i,1) ;
		CV_MAT_ELEM(*object_points2,float,i,2) = CV_MAT_ELEM(*object_points,float,i,2) ;

	} 
	for(int i=0; i<successes; ++i){
		CV_MAT_ELEM(*point_counts2,int,i, 0) = CV_MAT_ELEM(*point_counts, int,i,0);
	}
	cvReleaseMat(&object_points);
	cvReleaseMat(&image_points);
	cvReleaseMat(&point_counts);

	// At this point we have all of the chessboard corners we need.

	// Initialize the intrinsic matrix such that the two focal
	// lengths have a ratio of 1.0
	CV_MAT_ELEM( *intrinsic_matrix, float, 0, 0 ) = 1.0f;
	CV_MAT_ELEM( *intrinsic_matrix, float, 1, 1 ) = 1.0f;
	printf("cvCalibrateCamera2\n");
	cvCalibrateCamera2(
		object_points2,
		image_points2,
		point_counts2,
		cvGetSize( image ),
		intrinsic_matrix,
		distortion_coeffs,
		NULL,
		NULL,
		0//CV_CALIB_FIX_ASPECT_RATIO
		);
	
	// Save our work
	cvSave("Intrinsics.xml",intrinsic_matrix);
	cvSave("Distortion.xml",distortion_coeffs);
	
	// Load test
	CvMat *intrinsic = (CvMat*)cvLoad("Intrinsics.xml");
	CvMat *distortion = (CvMat*)cvLoad("Distortion.xml");

	// Build the undistort map which we will use for all 
	// subsequent frames.
	IplImage* mapx = cvCreateImage( cvGetSize(image), IPL_DEPTH_32F, 1 );
	IplImage* mapy = cvCreateImage( cvGetSize(image), IPL_DEPTH_32F, 1 );
	printf("cvInitUndistortMap\n");
	cvInitUndistortMap(
		intrinsic,
		distortion,
		mapx,
		mapy
		);
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
		cvShowImage( "Calibration", image );
		cvRemap( t, image, mapx, mapy );
		cvReleaseImage(&t);
		cvShowImage("Undistort", image);
		if((cvWaitKey()&0x7F) == 27) break;  
	}

	return 0;
} 
