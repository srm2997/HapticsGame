// Copyright 2007 Novint Technologies, Inc. All rights reserved.
// Available only under license from Novint Technologies, Inc.

// Make sure this header is included only once
#ifndef HAPTICS_H
#define HAPTICS_H

#include <hdl/hdl.h>
#include <hdlu/hdlu.h>

// Know which face is in contact
enum RS_Face {
    FACE_NONE = -1,
    FACE_NEAR, FACE_RIGHT, 
    FACE_FAR, FACE_LEFT, 
    FACE_TOP, FACE_BOTTOM,
    FACE_DEFAULT,
    FACE_LAST               // reserved to allow iteration over faces
    };

enum HapCoord {
	X = 0,
	Y = 1,
	Z = 2
	};


// Blocking values
const bool bNonBlocking = false;
const bool bBlocking = true;

class HapticsClass 
{

// Define callback functions as friends
friend HDLServoOpExitCode ContactCB(void *data);
friend HDLServoOpExitCode GetStateCB(void *data);

public:
    // Constructor
    HapticsClass( double& xposb, double& yposb );

    // Destructor
    ~HapticsClass();

    // Initialize
    void init(double a_cubeSize, double a_stiffness, double edgeLength );

    // Clean up
    void uninit();

    // Get position
    void getPosition(double pos[3]);

    // Get state of device button
    bool isButtonDown();

    // synchFromServo() is called from the application thread when it wants to exchange
    // data with the HapticClass object.  HDAL manages the thread synchronization
    // on behalf of the application.
    void synchFromServo();

    // Get ready state of device.
    bool isDeviceCalibrated();

	void bump();
	void jitter();
	void fire();

private:
    // Move data between servo and app variables
    void synch();

    // Calculate contact force with cube
    void cubeContact();

    // Matrix multiply
    void vecMultMatrix(double srcVec[3], double mat[16], double dstVec[3]);

    // Check error result; display message and abort, if any
    void testHDLError(const char* str);

    // Nothing happens until initialization is done
    bool m_inited;

    // Transformation from Device coordinates to Application coordinates
    double m_transformMat[16];
    
    // Variables used only by servo thread
    double m_positionServo[3];
    bool   m_buttonServo;
    double m_forceServo[3];

    // Variables used only by application thread
    double m_positionApp[3];
    bool   m_buttonApp;

    // Keep track of last face to have contact
    int    m_lastFace;

    // Handle to device
    HDLDeviceHandle m_deviceHandle;

    // Handle to Contact Callback 
    HDLServoOpExitCode m_servoOp;

    // Device workspace dimensions
    double m_workspaceDims[6];

    // Size of cube
    double m_cubeEdgeLength;

    // Stiffness of cube
    double m_cubeStiffness;

	double& m_xpos;
	double& m_ypos;
	double m_paddleWidth;

	int dobump;
	double prevY;
	int dojitter;
	int doPullDown;
	int doPullUp;
	int doFire;
};

#endif // HAPTICS_H