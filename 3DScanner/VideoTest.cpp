#include <cv.h>
#include <highgui.h>
#include <stdio.h>  
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <tchar.h>
#include "Serial.h"

using namespace std;

using namespace cv;

enum { EOF_Char = 27 };

int ShowError (LONG lError, LPCTSTR lptszMessage)
{
	// Generate a message text
	TCHAR tszMessage[256];
	wsprintf(tszMessage,_T("%s\n(error code %d)"), lptszMessage, lError);

	// Display message-box and return with an error-code
	::MessageBox(0,tszMessage,_T("Hello world"), MB_ICONSTOP|MB_OK);
	return 1;
}

bool serialSetupAndComm(){
	CSerial serial;
	LONG lLastError = ERROR_SUCCESS;

	// Attempt to open the serial port (COM1)
    lLastError = serial.Open(_T("COM1"),0,0,false);
	if (lLastError != ERROR_SUCCESS)
		return ::ShowError(serial.GetLastError(), _T("Unable to open COM-port"));
	
	//Setup the Serial Port
	lLastError = serial.Setup(CSerial::EBaud9600,CSerial::EData8,CSerial::EParNone,CSerial::EStop1);
	if (lLastError != ERROR_SUCCESS)
		return ::ShowError(serial.GetLastError(), _T("Unable to set COM-port setting"));

	// Setup handshaking
    lLastError = serial.SetupHandshaking(CSerial::EHandshakeHardware);
	if (lLastError != ERROR_SUCCESS)
		return ::ShowError(serial.GetLastError(), _T("Unable to set COM-port handshaking"));

	// Use 'non-blocking' reads, because we don't know how many bytes
	// will be received. This is normally the most convenient mode
	// (and also the default mode for reading data).
    lLastError = serial.SetupReadTimeouts(CSerial::EReadTimeoutNonblocking);
	if (lLastError != ERROR_SUCCESS)
		return ::ShowError(serial.GetLastError(), _T("Unable to set COM-port read timeout."));

	// Keep reading data, until an EOF (CTRL-Z) has been received
	bool fContinue = true;
	do {
		// Wait for an event
		lLastError = serial.WaitEvent();
		if (lLastError != ERROR_SUCCESS)
			return ::ShowError(serial.GetLastError(), _T("Unable to wait for a COM-port event."));

		// Save event
		const CSerial::EEvent eEvent = serial.GetEventType();

		// Handle data receive event
		if (eEvent & CSerial::EEventRecv) {
			// Read data, until there is nothing left
			DWORD dwBytesRead = 0;
			char szBuffer[11];
			do {
				// Read data from the COM-port
				lLastError = serial.Read(szBuffer,sizeof(szBuffer)-1,&dwBytesRead);
				if (lLastError != ERROR_SUCCESS)
					return ::ShowError(serial.GetLastError(), _T("Unable to read from COM-port."));

				if (dwBytesRead > 0){
					// Finalize the data, so it is a valid string
					szBuffer[dwBytesRead] = '\0';

					// Display the data
					printf("%s", szBuffer);

					// Check if EOF (CTRL+'[') has been specified
					if (strchr(szBuffer,EOF_Char))
						fContinue = false;
				}
			} while (dwBytesRead == sizeof(szBuffer)-1);
		}
	} while (fContinue);

    // The serial port is now ready and we can send/receive data. If
	// the following call blocks, then the other side doesn't support
	// hardware handshaking.
    lLastError = serial.Write("scan\n");
	if (lLastError != ERROR_SUCCESS)
		return ::ShowError(serial.GetLastError(), _T("Unable to send data"));
}

// A Simple Camera Capture Framework 
int main() {
	serialSetupAndComm();
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

		int hsvFrameWidth = hsvFrame->width;
		int hsvFrameHeight = hsvFrame->height;

		int pixelRow = hsvFrameHeight / 2;

		CvScalar s;

		// Do not release the frame!
		//If ESC key pressed, Key=0x10001B under OpenCV 0.9.7(linux version),
		//remove higher bits using AND operator
		if ( (cvWaitKey(10) & 255) == 27 ) {
			ofstream myFile;
			myFile.open("C:\\Users\\Scanners\\Desktop\\intensity.txt", ios::out);
			for (int i = 0; i < hsvFrameWidth; i++) {
			s = cvGet2D(frame,pixelRow,i);
			myFile << s.val[2] << "," << "\n";
		}
			myFile.close();
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