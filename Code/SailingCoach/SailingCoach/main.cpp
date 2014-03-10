//
//  main.cpp
//  SailingCoach
//
//  Created by Andrew Kessler on 3/7/14.
//  Copyright (c) 2014 Andrew Kessler. All rights reserved.
//

#include <iostream>
#include "opencv2/opencv.hpp"
#include "ColorSegmentation.h"
#include "Calibration.h"

using namespace cv;
using namespace std;


/****************************** MODULE DEFINES*********************************/
#define ESC_KEY 27
#define CAM_NUM 1
/****************************** MODULE VARS ***********************************/
//Window names
const string videoFeed = "Video Feed";
const string segFeed = "Segmented Feed";
const string controls = "Controls";

//Control vars
bool trackObjects = true;
bool useMorphOps = true;
bool tuning = false;
bool drawCenter = false;

/*************************** MODULE PROTOTYPES ********************************/
void runColorSegmentation(VideoCapture cap, double dWidth, double dHeight);
void createTrackbars(Threshold_t&, const string);
void morphOps(Mat &);
void drawCenterAxes(Mat&,Size,Scalar);

/**************************** MODULE BODIES ***********************************/
int main(int argc, const char * argv[])
{

    printf("Starting CV SailingCoach!\n");
    printf("Attempting to open camera feed...");
 
    VideoCapture cap(CAM_NUM); //open the first camera on list
    if (!cap.isOpened())
    {
        printf("Couldn't open the camera!\n");
        return 1;
    }
    else
    {
        printf("done!\n");
    }
    
    
    double dWidth = cap.get(CV_CAP_PROP_FRAME_WIDTH); //get the width of video frames
    double dHeight= cap.get(CV_CAP_PROP_FRAME_HEIGHT);//get the height of video frames
    printf("Frame size is %dx%d.\n", (int) dWidth,(int) dHeight);
    
    printf("Calibrate camera? [Y/n] ==>");
    string usr_in;
    cin >> usr_in;

    if (usr_in[0] == 'Y')
    {
        printf("Running calibration...\n");
        Size boardSize;
        boardSize.height = 6;
        boardSize.width = 9;
        
        calibrateCameraFromFeed(cap, 5, boardSize, 1.0f);
    }
    else
    {
        printf("Running segmentation...\n");
        runColorSegmentation(cap,dWidth,dHeight);
    }
    
    cap.release();
    
   
    return 0;
}



/*
 * This function applies morphological operations to the thresholded image.
 * It runs two erodes to reduce noise, and two dilates to enlarge remaining
 * white space.
 */
void morphOps(Mat &thresh){
    
	//create structuring element that will be used to "dilate" and "erode" image.
	//the element chosen here is a 3px by 3px rectangle
    
	Mat erodeElement = getStructuringElement( MORPH_RECT,Size(3,3));
    //dilate with larger element so make sure object is nicely visible
	Mat dilateElement = getStructuringElement( MORPH_RECT,Size(8,8));
    
	erode(thresh,thresh,erodeElement);
	erode(thresh,thresh,erodeElement);
    
    
	dilate(thresh,thresh,dilateElement);
	dilate(thresh,thresh,dilateElement);
    
    
    
}


void runColorSegmentation(VideoCapture cap, double dWidth, double dHeight)
{
    
    // Display the image.
    namedWindow(videoFeed,CV_WINDOW_AUTOSIZE);
    
    //Create settings struct
    Threshold_t color1_limits;
    createTrackbars(color1_limits, controls);
    
    
    Mat thisFrame_rgb;
    Mat thisFrame_hsv;
    Mat thisFrame_seg;
    Mat leftFrame, rightFrame, dispFrame;
    bool breakLoop = false;
    
    while (!breakLoop)
    {
        bool bSuccess = cap.read(thisFrame_rgb); //read new frame
        
        if (!bSuccess)
        {
            printf("Read from video stream failed!\n");
            break;
        }
        
        cvtColor(thisFrame_rgb, thisFrame_hsv, COLOR_BGR2HSV); //convert to HSV
        
        //Now we look for the particular color...
        inRange(thisFrame_hsv,
                Scalar(color1_limits.h_min,color1_limits.s_min,color1_limits.v_min),
                Scalar(color1_limits.h_max,color1_limits.s_max,color1_limits.v_max),
                thisFrame_seg);
        
        //Perform morphological operations on thresholded image to eliminate noise
        if (useMorphOps)
        {
            morphOps(thisFrame_seg);
        }
        
        cvtColor(thisFrame_seg, thisFrame_seg,COLOR_GRAY2BGR); //convert back to RGB
        
        if (drawCenter)
        {
            drawCenterAxes(thisFrame_rgb, Size(dWidth,dHeight),Scalar(255,255,255));
            drawCenterAxes(thisFrame_seg, Size(dWidth,dHeight),Scalar(0,255,0));
        }
        
        //Resize and plot
        resize(thisFrame_rgb, leftFrame, Size(0,0),0.5,0.5);
        resize(thisFrame_seg, rightFrame, Size(0,0),0.5,0.5);
        hconcat(leftFrame, rightFrame, dispFrame);
        imshow(videoFeed, dispFrame);
   
        
        char user_input = (char) waitKey(30);
        
        switch (user_input)
		{
            case '\377':
                //This means nothing was pressed.
                break;
                
            case 'm':
                useMorphOps = !useMorphOps;
                printf("Morphological operations %s.\n", useMorphOps ? "ON" : "OFF");
                break;
            case 't':
                trackObjects = !trackObjects;
                printf("Tracking objects %s.\n", trackObjects ? "ON" : "OFF");
                
                break;
            case 'w':
                tuning = !tuning;
                printf("Tuning %s.\n", tuning ? "ON" : "OFF");
                break;
            case 's':
                saveSettingsToFile(color1_limits, "color1");
                break;
            case 'r':
                readSettingsFromFile(color1_limits, "color1");
                break;
            case 'c':
                drawCenter = !drawCenter;
                printf("Draw center axes %s.\n", drawCenter ? "ON" : "OFF");
                break;
                
            case ESC_KEY:
            case 'q':
                cout << "Quitting!" << endl;
                breakLoop = true;
				break;
                
            default:
                printf("No behavior defined for '%c'.\n",user_input);
		} //end switch
    } //end while loop
    
    
    destroyWindow(videoFeed);
    thisFrame_rgb.release();
    thisFrame_hsv.release();
    thisFrame_seg.release();

}


void drawCenterAxes(Mat &frame, Size theSize,Scalar theColor)
{
    int x_center = theSize.width/2;
    int y_center = theSize.height/2;
    line(frame,Point(0,y_center),Point(theSize.width,y_center),theColor,1);
    line(frame,Point(x_center, 0), Point(x_center, theSize.height),theColor,1);
    circle(frame,Point(x_center,y_center),20,theColor,1);
    
}
