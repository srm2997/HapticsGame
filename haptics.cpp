#include "haptics.h"
#include <windows.h>
#include <math.h>
#include <sstream>

// Continuous servo callback function
HDLServoOpExitCode ContactCB(void* pUserData)
{
    // Get pointer to haptics object
    HapticsClass* haptics = static_cast< HapticsClass* >( pUserData );

    // Get current state of haptic device
    hdlToolPosition(haptics->m_positionServo);
    hdlToolButton(&(haptics->m_buttonServo));

    // Call the function that does the heavy duty calculations.
    haptics->cubeContact();

    // Send forces to device
    hdlSetToolForce(haptics->m_forceServo);

    // Make sure to continue processing
    return HDL_SERVOOP_CONTINUE;
}

// On-demand synchronization callback function
HDLServoOpExitCode GetStateCB(void* pUserData)
{
    // Get pointer to haptics object
    HapticsClass* haptics = static_cast< HapticsClass* >( pUserData );

    // Call the function that copies data between servo side 
    // and client side
    haptics->synch();

    // Only do this once.  The application will decide when it
    // wants to do it again, and call CreateServoOp with
    // bBlocking = true
    return HDL_SERVOOP_EXIT;
}

// Constructor--just make sure needed variables are initialized.
HapticsClass::HapticsClass( double& xposb, double& yposb)
    : m_lastFace(FACE_NONE),
      m_deviceHandle(HDL_INVALID_HANDLE),
      m_servoOp(HDL_INVALID_HANDLE),
      m_cubeEdgeLength(1),
      m_cubeStiffness(1),
      m_inited(false),
	  m_xpos(xposb),
	  m_ypos(yposb),
	  dobump(0),
	  dojitter(0),
	  doPullDown(0),
	  doPullUp(0)
{
    for (int i = 0; i < 3; i++)
        m_positionServo[i] = 0;
}

// Destructor--make sure devices are uninited.
HapticsClass::~HapticsClass()
{
    uninit();
}



void HapticsClass::init(double a_cubeSize, double a_stiffness, double paddleWidth )
{
	m_paddleWidth = paddleWidth;
    m_cubeEdgeLength = a_cubeSize;
    m_cubeStiffness = a_stiffness;
	//m_xpos = xposb;
	//m_ypos = yposb;


    HDLError err = HDL_NO_ERROR;

    // Passing "DEFAULT" or 0 initializes the default device based on the
    // [DEFAULT] section of HDAL.INI.   The names of other sections of HDAL.INI
    // could be passed instead, allowing run-time control of different devices
    // or the same device with different parameters.  See HDAL.INI for details.
    m_deviceHandle = hdlInitNamedDevice("DEFAULT");
    testHDLError("hdlInitDevice");

    if (m_deviceHandle == HDL_INVALID_HANDLE)
    {
        MessageBox(NULL, "Could not open device", "Device Failure", MB_OK);
        exit(0);
    }

    // Now that the device is fully initialized, start the servo thread.
    // Failing to do this will result in a non-funtional haptics application.
    hdlStart();
    testHDLError("hdlStart");

    // Set up callback function
    m_servoOp = hdlCreateServoOp(ContactCB, this, bNonBlocking);
    if (m_servoOp == HDL_INVALID_HANDLE)
    {
        MessageBox(NULL, "Invalid servo op handle", "Device Failure", MB_OK);
    }
    testHDLError("hdlCreateServoOp");

    // Make the device current.  All subsequent calls will
    // be directed towards the current device.
    hdlMakeCurrent(m_deviceHandle);
    testHDLError("hdlMakeCurrent");

    // Get the extents of the device workspace.
    // Used to create the mapping between device and application coordinates.
    // Returned dimensions in the array are minx, miny, minz, maxx, maxy, maxz
    //                                      left, bottom, far, right, top, near)
    // Right-handed coordinates:
    //   left-right is the x-axis, right is greater than left
    //   bottom-top is the y-axis, top is greater than bottom
    //   near-far is the z-axis, near is greater than far
    // workspace center is (0,0,0)
    hdlDeviceWorkspace(m_workspaceDims);
    testHDLError("hdlDeviceWorkspace");


    // Establish the transformation from device space to app space
    // To keep things simple, we will define the app space units as
    // inches, and set the workspace to approximate the physical
    // workspace of the Falcon.  That is, a 4" cube centered on the
    // origin.  Note the Z axis values; this has the effect of
    // moving the origin of world coordinates toward the base of the
    // unit.
    double gameWorkspace[] = {-2,-2,-2,2,2,3};
    bool useUniformScale = true;
    hdluGenerateHapticToAppWorkspaceTransform(m_workspaceDims,
                                              gameWorkspace,
                                              useUniformScale,
                                              m_transformMat);
    testHDLError("hdluGenerateHapticToAppWorkspaceTransform");


    m_inited = true;
}

// uninit() undoes the setup in reverse order.  Note the setting of
// handles.  This prevents a problem if uninit() is called
// more than once.
void HapticsClass::uninit()
{
    if (m_servoOp != HDL_INVALID_HANDLE)
    {
        hdlDestroyServoOp(m_servoOp);
        m_servoOp = HDL_INVALID_HANDLE;
    }
    hdlStop();
    if (m_deviceHandle != HDL_INVALID_HANDLE)
    {
        hdlUninitDevice(m_deviceHandle);
        m_deviceHandle = HDL_INVALID_HANDLE;
    }
    m_inited = false;
}

// This is a simple function for testing error returns.  A production
// application would need to be more sophisticated than this.
void HapticsClass::testHDLError(const char* str)
{
    HDLError err = hdlGetError();
    if (err != HDL_NO_ERROR)
    {
        MessageBox(NULL, str, "HDAL ERROR", MB_OK);
        abort();
    }
}

// This is the entry point used by the application to synchronize
// data access to the device.  Using this function eliminates the 
// need for the application to worry about threads.
void HapticsClass::synchFromServo()
{
	if ( !m_inited )
		return;
    hdlCreateServoOp(GetStateCB, this, bBlocking);
}

// GetStateCB calls this function to do the actual data movement.
void HapticsClass::synch()
{
    // m_positionApp is set in cubeContact().
    m_buttonApp = m_buttonServo;
}

// A utility function to handle matrix multiplication.  A production application
// would have a full vector/matrix math library at its disposal, but this is a
// simplified example.
void HapticsClass::vecMultMatrix(double srcVec[3], double mat[16], double dstVec[3])
{
    dstVec[0] = mat[0] * srcVec[0] 
        + mat[4] * srcVec[1]
        + mat[8] * srcVec[2]
        + mat[12];
    
    dstVec[1] = mat[1] * srcVec[0]
        + mat[5] * srcVec[1]
        + mat[9] * srcVec[2]
        + mat[13];

    dstVec[2] = mat[2] * srcVec[0]
        + mat[6] * srcVec[1]
        + mat[10] * srcVec[2]
        + mat[14];
}

void HapticsClass::bump(){
	dobump += 20;
}
void HapticsClass::jitter(){
	dojitter += 200;
}

// Here is where the heavy calculations are done.  This function is
// called from ContactCB to calculate the forces based on current
// cursor position and cube dimensions.  A simple spring model is
// used.
void HapticsClass::cubeContact()
{
	// Skip the whole thing if not initialized
    if (!m_inited) return;


    // Convert from device coordinates to application coordinates.
    vecMultMatrix(m_positionServo, m_transformMat, m_positionApp);
    m_forceServo[X] = 0; 
    m_forceServo[Y] = 0; 
    m_forceServo[Z] = 0;

	double paddle_top = m_positionApp[Y] + m_cubeEdgeLength / 2;
	double paddle_bottom = m_positionApp[Y] - m_cubeEdgeLength / 2;

	if( paddle_top > 1 ){
		m_forceServo[Y] = (paddle_top - 1) * -100;
	}
	if( paddle_bottom < -1 ){
		m_forceServo[Y] = ( paddle_bottom + 1) * -100;
	}

	/*char letters[100];
	sprintf( letters, "Force: %f      Y-Pos: %f\n", m_forceServo[Y], m_positionApp[Y]  );
	OutputDebugString( letters );*/


	if( dobump > 0 ){
		// Puck riccoched
		m_forceServo[0] = 10;
		m_forceServo[1] = 0;
		m_forceServo[2] = 0;
		dobump--;
		return;
	}

	if( dojitter > 0 ){
		// Was scored against
		if( dojitter % 40 < 20 ){
			m_forceServo[0] = 10;
			m_forceServo[1] = 0;
			m_forceServo[2] = 0;
		}else{
			m_forceServo[0] = -10;
			m_forceServo[1] = 0;
			m_forceServo[2] = 0;
		}
		dojitter--;
		return;
	}

	
	m_forceServo[Y] += (m_positionApp[Y] - m_ypos) * -5;
	m_forceServo[X] += (m_positionApp[X] - m_xpos - m_paddleWidth * 1.5) * -5;
	return;

	if ( doPullDown > doPullUp ) {
		doPullUp = 0;
	} else {
		doPullDown = 0;
	}

	if ( doPullDown > 0 ) {
		//m_forceServo[Y] += (m_positionApp[Y] - m_ypos) * -5;
		m_forceServo[Y] += -5;
		doPullDown--;
	} else if ( doPullUp > 0 ) {
		m_forceServo[Y] += 5;
		doPullUp--;
	}




//	char letters[100];
//	sprintf( letters, "%f", m_xpos  );
//	OutputDebugString( letters );
//	OutputDebugString("\n" );

	double d = m_xpos * m_xpos + m_ypos * m_ypos;

	if ( m_ypos - m_positionApp[Y] > 0.0 && m_positionApp[Y]-prevY > 0.1 ) {
		char letters[100];
		sprintf( letters, "Moving Toward Up\n");
		OutputDebugString( letters );
		m_forceServo[Y] += -2;
	} else if ( m_ypos - m_positionApp[Y] < 0.0 && m_positionApp[Y]-prevY < 0.1 ) {
		char letters[100];
		sprintf( letters, "Moving Toward Down\n");
		OutputDebugString( letters );
		m_forceServo[Y] += 2;
	} else {
		char letters[100];
		sprintf( letters, "Not Moving Toward\n");
		OutputDebugString( letters );

		// add spring stiffness to force effect
		
		
	}

	m_forceServo[X] += (m_positionApp[X] - m_xpos - m_paddleWidth * 1.5) * -5;

	prevY = m_positionApp[Y];
}

// Interface function to get current position
void HapticsClass::getPosition(double pos[3])
{
    pos[0] = m_positionApp[0];
    pos[1] = m_positionApp[1];
    pos[2] = m_positionApp[2];

}

// Interface function to get button state.  Only one button is used
// in this application.
bool HapticsClass::isButtonDown()
{
    return m_buttonApp;
}

// For this application, the only device status of interest is the
// calibration status.  A different application may want to test for
// HDAL_UNINITIALIZED and/or HDAL_SERVO_NOT_STARTED
bool HapticsClass::isDeviceCalibrated()
{
    unsigned int state = hdlGetState();

    return ((state & HDAL_NOT_CALIBRATED) == 0);
}
