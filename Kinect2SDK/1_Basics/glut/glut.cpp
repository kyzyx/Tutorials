#include <Windows.h>
#include <Ole2.h>

#include <gl/GL.h>
#include <gl/GLU.h>
#include <gl/glut.h>

#include <Kinect.h>

#define width 1920
#define height 1080

// OpenGL Variables
GLuint textureId;              // ID of the texture to contain Kinect RGB Data
GLubyte data[width*height*4];  // BGRA array containing the texture data

// Kinect variables
IKinectSensor* sensor;         // Kinect sensor
IColorFrameReader* reader;     // Kinect color data source

void draw(void);

bool init(int argc, char* argv[]) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(width,height);
    glutCreateWindow("Kinect SDK Tutorial");
    glutDisplayFunc(draw);
    glutIdleFunc(draw);
    return true;
}

bool initKinect() {
    if (FAILED(GetDefaultKinectSensor(&sensor))) {
		return false;
	}
	if (sensor) {
		sensor->Open();
		IColorFrameSource* framesource = NULL;
		sensor->get_ColorFrameSource(&framesource);
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
    IColorFrame* frame = NULL;
    if (SUCCEEDED(reader->AcquireLatestFrame(&frame))) {
        frame->CopyConvertedFrameDataToArray(width*height*4, data, ColorImageFormat_Bgra);
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

void draw() {
   drawKinectData();
   glutSwapBuffers();
}

void execute() {
    glutMainLoop();
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
