#include "main.h"
#include "glut.h"

#include <cmath>
#include <cstdio>

#include <Windows.h>
#include <Ole2.h>

#include <NuiApi.h>
#include <NuiImageCamera.h>
#include <NuiSensor.h>

// OpenGL Variables
long depthToRgbMap[width*height*2];
// We'll be using buffer objects to store the kinect point cloud
GLuint vboId;
GLuint cboId;

// Kinect variables
HANDLE depthStream;
HANDLE rgbStream;
INuiSensor* sensor;

bool initKinect() {
    // Get a working kinect sensor
    int numSensors;
    if (NuiGetSensorCount(&numSensors) < 0 || numSensors < 1) return false;
    if (NuiCreateSensorByIndex(0, &sensor) < 0) return false;

    // Initialize sensor
    sensor->NuiInitialize(NUI_INITIALIZE_FLAG_USES_DEPTH | NUI_INITIALIZE_FLAG_USES_COLOR);
    sensor->NuiImageStreamOpen(NUI_IMAGE_TYPE_DEPTH, // Depth camera or rgb camera?
        NUI_IMAGE_RESOLUTION_640x480,                // Image resolution
        0,        // Image stream flags, e.g. near mode
        2,        // Number of frames to buffer
        NULL,     // Event handle
        &depthStream);
	sensor->NuiImageStreamOpen(NUI_IMAGE_TYPE_COLOR, // Depth camera or rgb camera?
        NUI_IMAGE_RESOLUTION_640x480,                // Image resolution
        0,      // Image stream flags, e.g. near mode
        2,      // Number of frames to buffer
        NULL,   // Event handle
		&rgbStream);
    return sensor;
}

void getDepthData(GLubyte* dest) {
	float* fdest = (float*) dest;
	long* depth2rgb = (long*) depthToRgbMap;
    NUI_IMAGE_FRAME imageFrame;
    NUI_LOCKED_RECT LockedRect;
    if (sensor->NuiImageStreamGetNextFrame(depthStream, 0, &imageFrame) < 0) return;
    INuiFrameTexture* texture = imageFrame.pFrameTexture;
    texture->LockRect(0, &LockedRect, NULL, 0);
    if (LockedRect.Pitch != 0) {
        const USHORT* curr = (const USHORT*) LockedRect.pBits;
        for (int j = 0; j < height; ++j) {
			for (int i = 0; i < width; ++i) {
				// Get depth of pixel in millimeters
				USHORT depth = NuiDepthPixelToDepth(*curr++);
				// Store coordinates of the point corresponding to this pixel
				Vector4 pos = NuiTransformDepthImageToSkeleton(i, j, depth<<3, NUI_IMAGE_RESOLUTION_640x480);
				*fdest++ = pos.x/pos.w;
				*fdest++ = pos.y/pos.w;
				*fdest++ = pos.z/pos.w;
				// Store the index into the color array corresponding to this pixel
				NuiImageGetColorPixelCoordinatesFromDepthPixelAtResolution(
					NUI_IMAGE_RESOLUTION_640x480, NUI_IMAGE_RESOLUTION_640x480, NULL,
					i, j, depth<<3, depth2rgb, depth2rgb+1);
				depth2rgb += 2;
			}
		}
    }
    texture->UnlockRect(0);
    sensor->NuiImageStreamReleaseFrame(depthStream, &imageFrame);
}

void getRgbData(GLubyte* dest) {
	float* fdest = (float*) dest;
	long* depth2rgb = (long*) depthToRgbMap;
	NUI_IMAGE_FRAME imageFrame;
    NUI_LOCKED_RECT LockedRect;
    if (sensor->NuiImageStreamGetNextFrame(rgbStream, 0, &imageFrame) < 0) return;
    INuiFrameTexture* texture = imageFrame.pFrameTexture;
    texture->LockRect(0, &LockedRect, NULL, 0);
    if (LockedRect.Pitch != 0) {
        const BYTE* start = (const BYTE*) LockedRect.pBits;
        for (int j = 0; j < height; ++j) {
			for (int i = 0; i < width; ++i) {
				// Determine rgb color for each depth pixel
				long x = *depth2rgb++;
				long y = *depth2rgb++;
				// If out of bounds, then don't color it at all
				if (x < 0 || y < 0 || x > width || y > height) {
					for (int n = 0; n < 3; ++n) *(fdest++) = 0.0f;
				}
				else {
					const BYTE* curr = start + (x + width*y)*4;
					for (int n = 0; n < 3; ++n) *(fdest++) = curr[2-n]/255.0f;
				}

			}
		}
    }
    texture->UnlockRect(0);
    sensor->NuiImageStreamReleaseFrame(rgbStream, &imageFrame);
}

void getKinectData() {
	const int dataSize = width*height*3*4;
	GLubyte* ptr;
	glBindBuffer(GL_ARRAY_BUFFER, vboId);
	glBufferData(GL_ARRAY_BUFFER, dataSize, 0, GL_DYNAMIC_DRAW);
	ptr = (GLubyte*) glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	if (ptr) {
		getDepthData(ptr);
	}
	glUnmapBuffer(GL_ARRAY_BUFFER);
	glBindBuffer(GL_ARRAY_BUFFER, cboId);
	glBufferData(GL_ARRAY_BUFFER, dataSize, 0, GL_DYNAMIC_DRAW);
	ptr = (GLubyte*) glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	if (ptr) {
		getRgbData(ptr);
	}
	glUnmapBuffer(GL_ARRAY_BUFFER);
}

void rotateCamera() {
	static double angle = 0.;
	static double radius = 3.;
	double x = radius*sin(angle);
	double z = radius*(1-cos(angle)) - radius/2;
	glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
	gluLookAt(x,0,z,0,0,radius/2,0,1,0);
	angle += 0.05;
}

void drawKinectData() {
	getKinectData();
	rotateCamera();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	glBindBuffer(GL_ARRAY_BUFFER, vboId);
	glVertexPointer(3, GL_FLOAT, 0, NULL);
	
	glBindBuffer(GL_ARRAY_BUFFER, cboId);
	glColorPointer(3, GL_FLOAT, 0, NULL);

	glPointSize(1.f);
	glDrawArrays(GL_POINTS, 0, width*height);
	
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}

int main(int argc, char* argv[]) {
    if (!init(argc, argv)) return 1;
    if (!initKinect()) return 1;

    // OpenGL setup
    glClearColor(0,0,0,0);
    glClearDepth(1.0f);

	// Set up array buffers
	glGenBuffers(1, &vboId);
	glBindBuffer(GL_ARRAY_BUFFER, vboId);
	glGenBuffers(1, &cboId);
	glBindBuffer(GL_ARRAY_BUFFER, cboId);

    // Camera setup
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	gluPerspective(45, width /(GLdouble) height, 0.1, 1000);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
	gluLookAt(0,0,0,0,0,1,0,1,0);

    // Main loop
    execute();
    return 0;
}
