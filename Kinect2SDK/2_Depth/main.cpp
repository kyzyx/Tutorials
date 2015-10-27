#include "main.h"
#include "glut.h"

#include <Windows.h>
#include <Ole2.h>

#include <Kinect.h>

// OpenGL Variables
GLuint textureId;
GLubyte data[width*height*4];

// Kinect variables
IKinectSensor* sensor;         // Kinect sensor
IDepthFrameReader* reader;     // Kinect depth data source

bool initKinect() {
    if (FAILED(GetDefaultKinectSensor(&sensor))) {
		return false;
	}
	if (sensor) {
		sensor->Open();
		IDepthFrameSource* framesource = NULL;
		sensor->get_DepthFrameSource(&framesource);
		framesource->OpenReader(&reader);
		if (framesource) {
			framesource->Release();
			framesource = NULL;
		}
		return true;
	} else {
		return false;
	}
}

void getKinectData(GLubyte* dest) {
    IDepthFrame* frame = NULL;
    if (SUCCEEDED(reader->AcquireLatestFrame(&frame))) {
        unsigned int sz;
		unsigned short* buf;
		frame->AccessUnderlyingBuffer(&sz, &buf);

		const unsigned short* curr = (const unsigned short*)buf;
		const unsigned short* dataEnd = curr + (width*height);

		while (curr < dataEnd) {
			// Get depth in millimeters
			unsigned short depth = (*curr++);

			// Draw a grayscale image of the depth:
			// B,G,R are all set to depth%256, alpha set to 1.
			for (int i = 0; i < 3; ++i)
				*dest++ = (BYTE)depth % 256;
			*dest++ = 0xff;
		}
    }
    if (frame) frame->Release();
}

void drawKinectData() {
    glBindTexture(GL_TEXTURE_2D, textureId);
    getKinectData(data);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_BGRA_EXT, GL_UNSIGNED_BYTE, (GLvoid*)data);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(0, 0, 0);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(width, 0, 0);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(width, height, 0.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(0, height, 0.0f);
    glEnd();
}

int main(int argc, char* argv[]) {
    if (!init(argc, argv)) return 1;
    if (!initKinect()) return 1;

    // Initialize textures
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, (GLvoid*) data);
    glBindTexture(GL_TEXTURE_2D, 0);

    // OpenGL setup
    glClearColor(0,0,0,0);
    glClearDepth(1.0f);
    glEnable(GL_TEXTURE_2D);

    // Camera setup
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, 1, -1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Main loop
    execute();
    return 0;
}
