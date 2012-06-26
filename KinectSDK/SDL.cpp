#include <gl/glew.h>
#include <SDL.h>

#include <Windows.h>
#include <Ole2.h>

#include <NuiApi.h>
#include <NuiImageCamera.h>
#include <NuiSensor.h>

#define width 640
#define height 480

// SDL Variables
SDL_Surface* screen;
SDL_Event ev;

// OpenGL Variables
GLuint textureId;
GLubyte data[width*height*4];

// Kinect variables
HANDLE depthStream;
INuiSensor* sensor;

bool initSDL() {
	glewInit();
	SDL_Init(SDL_INIT_EVERYTHING);
	const SDL_VideoInfo* info = SDL_GetVideoInfo();
	if (!info) return false;

	// OpenGL param setup goes here
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);

	screen = SDL_SetVideoMode(width, height, info->vfmt->BitsPerPixel, SDL_HWSURFACE | SDL_GL_DOUBLEBUFFER | SDL_OPENGL);
	if (screen) return true;
	else return false;
}

bool initKinect() {
	// Get a working kinect sensor
	int numSensors;
	if (NuiGetSensorCount(&numSensors) < 0 || numSensors < 1) return false;
	for (int i = 0; i < numSensors; ++i) {
		if (NuiCreateSensorByIndex(i, &sensor) < 0) continue;
		if (sensor->NuiStatus() == 0) break;
		sensor->Release();
	}

	// Initialize sensor
	sensor->NuiInitialize(NUI_INITIALIZE_FLAG_USES_DEPTH | NUI_INITIALIZE_FLAG_USES_COLOR);
	sensor->NuiImageStreamOpen(NUI_IMAGE_TYPE_COLOR,
		NUI_IMAGE_RESOLUTION_640x480,
		0,		// Image stream flags, e.g. near mode
		2,		// Number of frames to buffer
		NULL,
		&depthStream);
	if (sensor) return true;
	return false;
}

void getKinectData(GLubyte* dest) {
	NUI_IMAGE_FRAME imageFrame;
	NUI_LOCKED_RECT LockedRect;
	if (sensor->NuiImageStreamGetNextFrame(depthStream, 0, &imageFrame) < 0) return;
	INuiFrameTexture* texture = imageFrame.pFrameTexture;
	// Lock the frame data so the Kinect knows not to modify it while we're reading it
    texture->LockRect(0, &LockedRect, NULL, 0);

    // Make sure we've received valid data
    if (LockedRect.Pitch != 0)
    {
        const BYTE* curr = (const BYTE*) LockedRect.pBits;
        const BYTE* dataEnd = curr + (width*height)*4;          // end pixel is start + width*height - 1

		while (curr < dataEnd) {
			*dest++ = *curr++;
		}
    }
    // We're done with the texture so unlock it
    texture->UnlockRect(0);
    // Release the frame
    sensor->NuiImageStreamReleaseFrame(depthStream, &imageFrame);
}

void drawKinectData() {
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE, (GLvoid*)data);
	getKinectData(data);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
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

void execute() {
	bool running = true;
	while (running) {
		while (SDL_PollEvent(&ev)) {
			if (ev.type == SDL_QUIT) running = false;
		}
		drawKinectData();
		SDL_GL_SwapBuffers();
	}
}

int main(int argc, char* argv[]) {
	if (!init()) return 1;
	if (!initKinect()) return 1;

    // Initialize textures
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, (GLvoid*) data);
    glBindTexture(GL_TEXTURE_2D, 0);

    // OpenGL setup
	glClearColor(0,0,0,0);
	glClearDepth(1.0f);
	glEnable(GL_TEXTURE_2D);

    // Camera setup
	glViewport(0, 0, 640, 480);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 640, 480, 0, 1, -1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

    // Main loop
    execute();
	return 0;
}
