#include <cv.h>
#include <highgui.h>
#include <stdio.h>  
#include <stdlib.h>

using namespace cv;

// A Simple Camera Capture Framework 
int main() {
	CvCapture* capture = cvCaptureFromCAM( CV_CAP_ANY );

	if ( !capture ) {
		fprintf( stderr, "ERROR: capture is NULL \n" );
		getchar();
		return -1;
	}

	IplImage* frame = cvQueryFrame( capture );
	IplImage* hsvFrame = cvCreateImage(cvGetSize(frame), 8, 3);
	IplImage* redFrame = cvCreateImage(cvGetSize(frame), 8, 1);

	// Create a window in which the captured images will be presented
	cvNamedWindow( "mywindow", CV_WINDOW_AUTOSIZE );
	cvNamedWindow( "mywindowhsv", CV_WINDOW_AUTOSIZE );
	cvNamedWindow( "mywindowred", CV_WINDOW_AUTOSIZE );
	// Show the image captured from the camera in the window and repeat
	while ( 1 ) {
		// Get one frame
		if ( !frame ) {
			fprintf( stderr, "ERROR: frame is null...\n" );
			getchar();
			break;
		}

		cvShowImage( "mywindow", frame );

		cvCvtColor(frame, hsvFrame, CV_BGR2HSV);
		cvShowImage("mywindowhsv", hsvFrame);

		cvInRangeS(hsvFrame, cvScalar(0, 100, 100), cvScalar(35, 240, 240), redFrame); 
		cvShowImage("mywindowred", redFrame);
		// Do not release the frame!
		//If ESC key pressed, Key=0x10001B under OpenCV 0.9.7(linux version),
		//remove higher bits using AND operator
		if ( (cvWaitKey(10) & 255) == 27 ) {
			break;
		}
		frame = cvQueryFrame( capture );
	}
	// Release the capture device housekeeping
	cvReleaseCapture( &capture );
	cvDestroyWindow( "mywindow" );
	cvDestroyWindow( "mywindowhsv");
	cvDestroyWindow( "mywindowred");
	return 0;
}