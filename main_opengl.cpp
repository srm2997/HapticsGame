// Copyright 2007 Novint Technologies, Inc. All rights reserved.
// Available only under license from Novint Technologies, Inc.
//
//
// Basic application showing use of HDAL.
// Haptic interaction with a ball.
// Graphic display of cursor.
//
//
// Comment the following line to get a console window on startup
#pragma comment( linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"" )

// windows.h must precede glut.h to eliminate the "exit" compiler error in VS2003, VS2005
#include <windows.h>
#include "glut.h"
#include <math.h>
#include "haptics.h"
#include <sstream>
#include <shlobj.h>


double NORTH = 1.0;
double SOUTH = -1.0;
double EAST = 1.5;
double WEST = -1.5;

int SCORE_P1 = 0;
int SCORE_P2 = 0;

double xposb, yposb;
double yposp1, xposp1;
double yposp2, xposp2;
double xmov, ymov;
SYSTEMTIME lasttime;



// Cube parameters
const double gStiffness = 200.0;
const double gCubeEdgeLength = 0.5;

// Some OpenGL values
static GLuint gCursorDisplayList = 0;
static double gCursorRadius = 0.05;
static GLfloat colorRed[]  = {1.0, 0.0, 0.0};
static GLfloat colorTeal[] = {0.0, 0.5, 0.5};
static GLfloat* gCurrentColor;

// The haptics object, with which we must interact
HapticsClass gHaptics(xposb,yposb);

// Forward declarations
void glutDisplay(void);
void glutReshape(int width, int height);
void glutIdle(void);
void glutKeyboard(unsigned char key, int x, int y);
void exitHandler(void);
void initGL();
void initScene();
void drawGraphics();
void drawCursor();

void glutMouseMove(int x, int y);



int main(int argc, char *argv[])
{
	AllocConsole();
    // Normal OpenGL Setup
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(500, 500);
    glutCreateWindow("Basic--OpenGL");
    glutDisplayFunc(glutDisplay);
    glutReshapeFunc(glutReshape);
    glutIdleFunc(glutIdle);
    glutKeyboardFunc(glutKeyboard);
	glutPassiveMotionFunc(glutMouseMove);

    
    // Set up handler to make sure teardown is done.
    atexit(exitHandler);

    // Set up the scene with graphics and haptics
    initScene();

    // Now start the simulation
    glutMainLoop();

    return 0;
}

// drawing callback function
void glutDisplay()
{   
    drawGraphics();
    glutSwapBuffers();
}

// reshape function (handle window resize)
void glutReshape(int width, int height)
{
    static const double kPI = 3.1415926535897932384626433832795;
    static const double kFovY = 40;

    double nearDist, farDist, aspect;

    glViewport(0, 0, width, height);

    nearDist = 1.0 / tan((kFovY / 2.0) * kPI / 180.0);
    farDist = nearDist + 2.0;
    aspect = (double) width / height;
   
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(kFovY, aspect, nearDist, farDist);

    /* Place the camera down the Z axis looking at the origin */
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();            
    gluLookAt(0, 0, nearDist + 2.0,
              0, 0, 0,
              0, 1, 0);
}

// What to do when doing nothing else
void glutIdle()
{
    glutPostRedisplay();
}


// Handle keyboard.  In this case, just the Escape key
void glutKeyboard(unsigned char key, int x, int y)
{
    static bool inited = true;

    if (key == 27) // esc key
    {
        exit(0);
    }
}

// Handle mouse movement
void glutMouseMove( int x, int y){
	yposp2 = (y - 250) / -(500 / 3.0);
}

// Scene setup
void initScene()
{

	xposp1 = EAST + gCubeEdgeLength / 4.0;
	yposp1 = 0;
	xposp2 = WEST - gCubeEdgeLength / 4.0;
	yposp2 = 0;
	xposb = 0;
	yposb = 0;
	xmov = 0.7;
	ymov = 0.7;

    // Call the haptics initialization function
    gHaptics.init(gCubeEdgeLength, gStiffness, gCubeEdgeLength / 2);

    // Set up the OpenGL graphics
    initGL();

    // Some time is required between init() and checking status,
    // for the device to initialize and stabilize.  In a complex
    // application, this time can be consumed in the initGL()
    // function.  Here, it is simulated with Sleep().
    Sleep(100);

    // Tell the user what to do if the device is not calibrated
    if (!gHaptics.isDeviceCalibrated())
        MessageBox(NULL, 
                   // The next two lines are one long string
                   "Please home the device by extending\n"
                   "then pushing the arms all the way in.",
                   "Not Homed",
                   MB_OK);


	GetSystemTime(&lasttime);
}

// Set up OpenGL.  Details are left to the reader
void initGL()
{
    static const GLfloat light_model_ambient[] = {0.3f, 0.3f, 0.3f, 1.0f};
    static const GLfloat light0_diffuse[] = {0.9f, 0.9f, 0.9f, 0.9f};   
    static const GLfloat light0_direction[] = {0.0f, -0.4f, 1.0f, 0.0f};    
    
    /* Enable depth buffering for hidden surface removal. */
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);
    
    /* Cull back faces. */
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    
    /* Set lighting parameters */
    glEnable(GL_LIGHTING);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);
    
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_FALSE);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);    
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, light_model_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, light0_direction);
    glEnable(GL_LIGHT0);   
}

// Make sure we exit cleanly
void exitHandler()
{
    gHaptics.uninit();
}

float CalculateTimeLeft(SYSTEMTIME timeTo, SYSTEMTIME timeFrom )
{
    LARGE_INTEGER nFrom, nTo;
    FILETIME timeTemp;
    float fReturn = 0.0f;
 
    SystemTimeToFileTime(&timeFrom, &timeTemp);
    nFrom.HighPart = timeTemp.dwHighDateTime;
    nFrom.LowPart = timeTemp.dwLowDateTime;
 
    SystemTimeToFileTime(&timeTo, &timeTemp);
    nTo.HighPart = timeTemp.dwHighDateTime;
    nTo.LowPart = timeTemp.dwLowDateTime;
 
    if (nTo.QuadPart > nFrom.QuadPart)
    {
        unsigned long long nDiff = nTo.QuadPart - nFrom.QuadPart;
        fReturn = nDiff / 10000.0f / 1000.0f / 60.0f / 60.0f / 24.0f;
    }
     
    return fReturn;
}

long double Delta(const SYSTEMTIME st1, const SYSTEMTIME st2)
{
    union timeunion {
        FILETIME fileTime;
        ULARGE_INTEGER ul;
    } ;
    
    timeunion ft1;
    timeunion ft2;

    SystemTimeToFileTime(&st1, &ft1.fileTime);
    SystemTimeToFileTime(&st2, &ft2.fileTime);

    return (long double)( ft2.ul.QuadPart - ft1.ul.QuadPart ) / 10000.0; // milliseconds
}

void Score(){
	char letters[100];
	sprintf( letters, "%i - %i", SCORE_P2, SCORE_P1  );
	OutputDebugString( letters );
	OutputDebugString("\n" );
	char path[ MAX_PATH ];
	SHGetFolderPathA( NULL, CSIDL_PROFILE, NULL, 0, path );
	strcat(path,"\\Documents\\HapticsGame\\ballOut.wav");
	PlaySound(path, NULL, SND_ASYNC | SND_FILENAME);
}

void BoundCheck( long double i ){

	double halfEdge = gCubeEdgeLength / 2.0;

	double top = yposb + halfEdge ;
	double bottom = yposb - halfEdge;
	double left = xposb - halfEdge;
	double right = xposb + halfEdge;

//	if( right > EAST ){
//		xposb -= xmov * 2 * i / 1000.0;
//		xmov = -xmov;
//	}
	if( top > NORTH ){
		yposb -= ymov * 2 * i / 1000.0;
		ymov = -ymov;
	}
//	if( left < WEST ){
//		xposb -= xmov * 2 * i / 1000.0;
//		xmov = -xmov;
//	}
	if( bottom < SOUTH ){
		yposb -= ymov * 2 * i / 1000.0;
		ymov = -ymov;

	}

	if( right >= EAST ){
		if( top > yposp1 - halfEdge && bottom < yposp1 + halfEdge ){
			xposb -= xmov * 2 * i / 1000.0;
			xmov = -xmov;
			xmov *= 1.1;
			ymov *= 1.1;
			gHaptics.bump();
			char path[ MAX_PATH ];
			SHGetFolderPathA( NULL, CSIDL_PROFILE, NULL, 0, path );
			strcat(path,"\\Documents\\HapticsGame\\rightPaddleHit.wav");
			PlaySound(path, NULL, SND_ASYNC | SND_FILENAME);
		}else{
			xposb = 0;
			xmov = 0.7;
			ymov = 0.7;
			SCORE_P2++;
			Score();
			gHaptics.jitter();
		}
	}

	if( left <= WEST ){
		if( top > yposp2 - halfEdge && bottom < yposp2 + halfEdge ){
			xposb -= xmov * 2 * i / 1000.0;
			xmov = -xmov;
			xmov *= 1.1;
			ymov *= 1.1;
			char path[ MAX_PATH ];
			SHGetFolderPathA( NULL, CSIDL_PROFILE, NULL, 0, path );
			strcat(path,"\\Documents\\HapticsGame\\leftPaddleHit.wav");
			PlaySound(path, NULL, SND_ASYNC | SND_FILENAME);
		}else{
			xposb = 0;
			xmov = -0.7;
			ymov = 0.7;
			SCORE_P1++;
			Score();
		}
	}
}

void UpdatePos(){
	SYSTEMTIME str_t;
	GetSystemTime(&str_t);
	
	long double i = Delta( lasttime, str_t );

//	char letters[100];
//	sprintf( letters, "%f", i  );
//	OutputDebugString( letters );
//	OutputDebugString("\n" );

	xposb += xmov * i / 1000.0;
	yposb += ymov * i / 1000.0;

    BoundCheck( i );

//	sprintf( letters, "%f", xmov * i / 1000 );
//	OutputDebugString( letters );
//	OutputDebugString("\n" );



	lasttime = str_t;
}

// Draw the cursor and the cube.  In a real application,
// this function would be much more complex.
void drawGraphics()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);           

    drawCursor();

	
	double cp[3];
	gHaptics.synchFromServo();
	gHaptics.getPosition( cp );

	yposp1 = cp[1];

	// Draw right paddle (p1)
	glTranslatef( xposp1, yposp1, 0 );
	glScalef( 0.5, 1, 1 );
	glutSolidCube(gCubeEdgeLength);
	glScalef( 2.0, 1, 1 );
	glTranslatef( -xposp1, -yposp1, 0 );

	// Draw left paddle (p2)
	glTranslatef( xposp2, yposp2, 0 );
	glScalef( 0.5, 1, 1 );
	glutSolidCube(gCubeEdgeLength);
	glScalef( 2.0, 1, 1 );	
	glTranslatef( -xposp2, -yposp2, 0 );

	// Draw puck
    glTranslatef( xposb, yposb, 0);
    //glutSolidCube(gCubeEdgeLength);
	glutSolidSphere( gCubeEdgeLength / 2, 20, 10 );
	glTranslatef(-xposb, -yposb, 0);

	// Draw top
	//glDisable(GL_DEPTH_TEST);
	////glDepthMask(GL_FALSE);

	////Define the color (blue)
	//glColor3ub(255, 0, 0);
	//glBegin(GL_QUADS);

	//  //Draw our four points, clockwise.
	//  glVertex2d(0, 0);
	//  glVertex2d(10, 0);
	//  glVertex2d(10, 10);
	//  glVertex2d(0, 10);
	//glEnd();

	//glEnable(GL_DEPTH_TEST);
	////glDepthMask(!GL_FALSE);
	double width = ( EAST - WEST ) * 2;
	glTranslatef( 0, NORTH + width / 2, 0 );
	glScalef( 1, 1, 1 / width / 2 );
	glutSolidCube(width);
	glScalef( 1, 1, width * 2 );
	glTranslatef( 0, -(NORTH + width / 2), 0 );

	glTranslatef( 0, (SOUTH - width / 2), 0 );
	glScalef( 1, 1, 1 / width / 2 );
	glutSolidCube( width );
	glScalef( 1, 1, width * 2 );
	glTranslatef( 0, -(SOUTH - width / 2), 0 );

	// Update puck position
	UpdatePos();
}

// Draw the cursor
void drawCursor()
{
    static const int kCursorTess = 15;

    // Haptic cursor position in "world coordinates"
    double cursorPosWC[3];

    // Must synch before data is valid
    gHaptics.synchFromServo();
    gHaptics.getPosition(cursorPosWC);

    // The color will depend on the button state.
    gCurrentColor = gHaptics.isButtonDown() ? colorRed : colorTeal;

    GLUquadricObj *qobj = 0;

    glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_LIGHTING_BIT);
    glPushMatrix();

    // Page 505-506 of The Red Book recommends using a display list
    // for static quadrics such as spheres
    if (!gCursorDisplayList)
    {
        gCursorDisplayList = glGenLists(1);
        glNewList(gCursorDisplayList, GL_COMPILE);
        qobj = gluNewQuadric();

        gluSphere(qobj, gCursorRadius, kCursorTess, kCursorTess);
        
        gluDeleteQuadric(qobj);
        glEndList();
    }

    glTranslatef(cursorPosWC[0], cursorPosWC[1], cursorPosWC[2]);

    glEnable(GL_COLOR_MATERIAL);
    glColor3fv(gCurrentColor);

    glCallList(gCursorDisplayList);

    glPopMatrix(); 
    glPopAttrib();
}
