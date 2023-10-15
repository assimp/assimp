// ----------------------------------------------------------------------------
// Another Assimp OpenGL sample including texturing.
// Note that it is very basic and will only read and apply the model's diffuse
// textures (by their material ids)
//
// Don't worry about the "Couldn't load Image: ...dwarf2.jpg" Message.
// It's caused by a bad texture reference in the model file (I guess)
//
// If you intend to _use_ this code sample in your app, do yourself a favour
// and replace immediate mode calls with VBOs ...
//
// Thanks to NeHe on whose OpenGL tutorials this one's based on! :)
// http://nehe.gamedev.net/
// ----------------------------------------------------------------------------
#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <utf8.h>

#ifdef _MSC_VER
#pragma warning(disable: 4100) // Disable warning 'unreferenced formal parameter'
#endif // _MSC_VER

#define STB_IMAGE_IMPLEMENTATION
#include "contrib/stb/stb_image.h"

#ifdef _MSC_VER
#pragma warning(default: 4100) // Enable warning 'unreferenced formal parameter'
#endif // _MSC_VER

#include <fstream>

//to map image filenames to textureIds
#include <string.h>
#include <map>

// assimp include files. These three are usually needed.
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/LogStream.hpp>

// The default hard-coded path. Can be overridden by supplying a path through the command line.
static std::string modelpath = "../../test/models/OBJ/spider.obj";

HGLRC       hRC = nullptr;         // Permanent Rendering Context
HDC         hDC = nullptr;            // Private GDI Device Context
HWND        g_hWnd = nullptr;         // Holds Window Handle
HINSTANCE   g_hInstance = nullptr;    // Holds The Instance Of The Application

bool		keys[256];			// Array used for Keyboard Routine;
bool		active=TRUE;		// Window Active Flag Set To TRUE by Default
bool		fullscreen=TRUE;	// full-screen Flag Set To full-screen By Default

GLfloat		xrot;
GLfloat		yrot;
GLfloat		zrot;


LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);	// Declaration For WndProc
GLboolean abortGLInit(const char*);

const char* windowTitle = "OpenGL Framework";

GLfloat LightAmbient[]= { 0.5f, 0.5f, 0.5f, 1.0f };
GLfloat LightDiffuse[]= { 1.0f, 1.0f, 1.0f, 1.0f };
GLfloat LightPosition[]= { 0.0f, 0.0f, 15.0f, 1.0f };

// the global Assimp scene object
const aiScene* g_scene = nullptr;
GLuint scene_list = 0;
aiVector3D scene_min, scene_max, scene_center;

// images / texture
std::map<std::string, GLuint*> textureIdMap;	// map image filenames to textureIds
GLuint*		textureIds;							// pointer to texture Array

// Create an instance of the Importer class
Assimp::Importer importer;

void createAILogger() {
    // Change this line to normal if you not want to analyze the import process
	Assimp::Logger::LogSeverity severity = Assimp::Logger::VERBOSE;

	// Create a logger instance for Console Output
	Assimp::DefaultLogger::create("",severity, aiDefaultLogStream_STDOUT);

	// Create a logger instance for File Output (found in project folder or near .exe)
	Assimp::DefaultLogger::create("assimp_log.txt",severity, aiDefaultLogStream_FILE);

	// Now I am ready for logging my stuff
	Assimp::DefaultLogger::get()->info("this is my info-call");
}

void destroyAILogger() {
	Assimp::DefaultLogger::kill();
}

void logInfo(const std::string &logString) {
	Assimp::DefaultLogger::get()->info(logString.c_str());
}

void logDebug(const char* logString) {
	Assimp::DefaultLogger::get()->debug(logString);
}


bool Import3DFromFile( const std::string &filename) {
	// Check if file exists
    std::ifstream fin(filename.c_str());
	if(fin.fail()) {
        std::string message = "Couldn't open file: " + filename;
		std::wstring targetMessage;
        //utf8::utf8to16(message.c_str(), message.c_str() + message.size(), targetMessage);
        ::MessageBox(nullptr, targetMessage.c_str(), L"Error", MB_OK | MB_ICONEXCLAMATION);
        logInfo(importer.GetErrorString());
        return false;
	}
    
	fin.close();
	
	g_scene = importer.ReadFile(filename, aiProcessPreset_TargetRealtime_Quality);

	// If the import failed, report it
	if (g_scene == nullptr) {
		logInfo( importer.GetErrorString());
		return false;
	}

	// Now we can access the file's contents.
    logInfo("Import of scene " + filename + " succeeded.");

	// We're done. Everything will be cleaned up by the importer destructor
	return true;
}

// Resize And Initialize The GL Window
void ReSizeGLScene(GLsizei width, GLsizei height) {
    // Prevent A Divide By Zero By
	if (height == 0) {
        // Making Height Equal One
        height=1;
	}

	glViewport(0, 0, width, height);					// Reset The Current Viewport

	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glLoadIdentity();							// Reset The Projection Matrix

	// Calculate The Aspect Ratio Of The Window
	gluPerspective(45.0f,(GLfloat)width/(GLfloat)height,0.1f,100.0f);

	glMatrixMode(GL_MODELVIEW);						// Select The Modelview Matrix
	glLoadIdentity();							// Reset The Modelview Matrix
}


std::string getBasePath(const std::string& path) {
	size_t pos = path.find_last_of("\\/");
	return (std::string::npos == pos) ? "" : path.substr(0, pos + 1);
}

void freeTextureIds() {
    // no need to delete pointers in it manually here. (Pointers point to textureIds deleted in next step)
	textureIdMap.clear();

	if (textureIds) {
		delete[] textureIds;
		textureIds = nullptr;
	}
}

int LoadGLTextures(const aiScene* scene) {
	freeTextureIds();

    if (scene->HasTextures()) 
		return 1;

	/* getTexture Filenames and Numb of Textures */
	for (unsigned int m=0; m<scene->mNumMaterials; m++)
	{
		int texIndex = 0;
		aiReturn texFound = AI_SUCCESS;

		aiString path;	// filename

		while (texFound == AI_SUCCESS)
		{
			texFound = scene->mMaterials[m]->GetTexture(aiTextureType_DIFFUSE, texIndex, &path);
			textureIdMap[path.data] = nullptr; //fill map with textures, pointers still NULL yet
			texIndex++;
		}
	}

	const size_t numTextures = textureIdMap.size();

	/* create and fill array with GL texture ids */
	textureIds = new GLuint[numTextures];
	glGenTextures(static_cast<GLsizei>(numTextures), textureIds); /* Texture name generation */

	/* get iterator */
	std::map<std::string, GLuint*>::iterator itr = textureIdMap.begin();

	std::string basepath = getBasePath(modelpath);
	for (size_t i=0; i<numTextures; i++)
	{
		std::string filename = (*itr).first;  // get filename
		(*itr).second =  &textureIds[i];	  // save texture id for filename in map
		itr++;								  // next texture


		std::string fileloc = basepath + filename;	/* Loading of image */
        int x, y, n;
        unsigned char *data = stbi_load(fileloc.c_str(), &x, &y, &n, STBI_rgb_alpha);

		if (nullptr != data )
		{
            // Binding of texture name
            glBindTexture(GL_TEXTURE_2D, textureIds[i]);
			// redefine standard texture values
            // We will use linear interpolation for magnification filter
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
            // We will use linear interpolation for minifying filter
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
            // Texture specification
            glTexImage2D(GL_TEXTURE_2D, 0, n, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);// Texture specification.

            // we also want to be able to deal with odd texture dimensions
            glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
            glPixelStorei( GL_UNPACK_ROW_LENGTH, 0 );
            glPixelStorei( GL_UNPACK_SKIP_PIXELS, 0 );
            glPixelStorei( GL_UNPACK_SKIP_ROWS, 0 );
            stbi_image_free(data);
        } else {
            /* Error occurred */
            const std::string message = "Couldn't load Image: " + fileloc;
            std::wstring targetMessage;
            wchar_t *tmp = new wchar_t[message.size() + 1];
            memset(tmp, L'\0', sizeof(wchar_t) *(message.size() + 1));
            utf8::utf8to16(message.c_str(), message.c_str() + message.size(), tmp);
            targetMessage = tmp;
            delete [] tmp;
            MessageBox(nullptr, targetMessage.c_str(), TEXT("ERROR"), MB_OK | MB_ICONEXCLAMATION);
		}
	}

	return TRUE;
}

// All Setup For OpenGL goes here
int InitGL()
{
	if (!LoadGLTextures(g_scene))
	{
		return FALSE;
	}

	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);		 // Enables Smooth Shading
	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
	glClearDepth(1.0f);				// Depth Buffer Setup
	glEnable(GL_DEPTH_TEST);		// Enables Depth Testing
	glDepthFunc(GL_LEQUAL);			// The Type Of Depth Test To Do
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculation


	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);    // Uses default lighting parameters
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	glEnable(GL_NORMALIZE);

	glLightfv(GL_LIGHT1, GL_AMBIENT, LightAmbient);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, LightDiffuse);
	glLightfv(GL_LIGHT1, GL_POSITION, LightPosition);
	glEnable(GL_LIGHT1);

	return TRUE;					// Initialization Went OK
}


// Can't send color down as a pointer to aiColor4D because AI colors are ABGR.
void Color4f(const aiColor4D *color)
{
	glColor4f(color->r, color->g, color->b, color->a);
}

void set_float4(float f[4], float a, float b, float c, float d)
{
	f[0] = a;
	f[1] = b;
	f[2] = c;
	f[3] = d;
}

void color4_to_float4(const aiColor4D *c, float f[4])
{
	f[0] = c->r;
	f[1] = c->g;
	f[2] = c->b;
	f[3] = c->a;
}

void apply_material(const aiMaterial *mtl)
{
	float c[4];

	GLenum fill_mode;
	int ret1, ret2;
	aiColor4D diffuse;
	aiColor4D specular;
	aiColor4D ambient;
	aiColor4D emission;
	ai_real shininess, strength;
	int two_sided;
	int wireframe;
	unsigned int max;	// changed: to unsigned

	int texIndex = 0;
	aiString texPath;	//contains filename of texture

	if(AI_SUCCESS == mtl->GetTexture(aiTextureType_DIFFUSE, texIndex, &texPath))
	{
		//bind texture
		unsigned int texId = *textureIdMap[texPath.data];
		glBindTexture(GL_TEXTURE_2D, texId);
	}

	set_float4(c, 0.8f, 0.8f, 0.8f, 1.0f);
	if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_DIFFUSE, &diffuse))
		color4_to_float4(&diffuse, c);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, c);

	set_float4(c, 0.0f, 0.0f, 0.0f, 1.0f);
	if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_SPECULAR, &specular))
		color4_to_float4(&specular, c);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, c);

	set_float4(c, 0.2f, 0.2f, 0.2f, 1.0f);
	if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_AMBIENT, &ambient))
		color4_to_float4(&ambient, c);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, c);

	set_float4(c, 0.0f, 0.0f, 0.0f, 1.0f);
	if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_EMISSIVE, &emission))
		color4_to_float4(&emission, c);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, c);

	max = 1;
	ret1 = aiGetMaterialFloatArray(mtl, AI_MATKEY_SHININESS, &shininess, &max);
	max = 1;
	ret2 = aiGetMaterialFloatArray(mtl, AI_MATKEY_SHININESS_STRENGTH, &strength, &max);
	if((ret1 == AI_SUCCESS) && (ret2 == AI_SUCCESS))
		glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess * strength);
	else {
		glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);
		set_float4(c, 0.0f, 0.0f, 0.0f, 0.0f);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, c);
	}

	max = 1;
	if(AI_SUCCESS == aiGetMaterialIntegerArray(mtl, AI_MATKEY_ENABLE_WIREFRAME, &wireframe, &max))
		fill_mode = wireframe ? GL_LINE : GL_FILL;
	else
		fill_mode = GL_FILL;
	glPolygonMode(GL_FRONT_AND_BACK, fill_mode);

	max = 1;
	if((AI_SUCCESS == aiGetMaterialIntegerArray(mtl, AI_MATKEY_TWOSIDED, &two_sided, &max)) && two_sided)
		glEnable(GL_CULL_FACE);
	else
		glDisable(GL_CULL_FACE);
}


void recursive_render (const struct aiScene *sc, const struct aiNode* nd, float scale)
{
	unsigned int i;
	unsigned int n=0, t;
	aiMatrix4x4 m = nd->mTransformation;

	aiMatrix4x4 m2;
	aiMatrix4x4::Scaling(aiVector3D(scale, scale, scale), m2);
	m = m * m2;

	// update transform
	m.Transpose();
	glPushMatrix();
	glMultMatrixf((float*)&m);

	// draw all meshes assigned to this node
	for (; n < nd->mNumMeshes; ++n)
	{
		const struct aiMesh* mesh = sc->mMeshes[nd->mMeshes[n]];

		apply_material(sc->mMaterials[mesh->mMaterialIndex]);


		if(mesh->mNormals == nullptr)
		{
			glDisable(GL_LIGHTING);
		}
		else
		{
			glEnable(GL_LIGHTING);
		}

		if(mesh->mColors[0] != nullptr)
		{
			glEnable(GL_COLOR_MATERIAL);
		}
		else
		{
			glDisable(GL_COLOR_MATERIAL);
		}

		for (t = 0; t < mesh->mNumFaces; ++t) {
			const struct aiFace* face = &mesh->mFaces[t];
			GLenum face_mode;

			switch(face->mNumIndices)
			{
				case 1: face_mode = GL_POINTS; break;
				case 2: face_mode = GL_LINES; break;
				case 3: face_mode = GL_TRIANGLES; break;
				default: face_mode = GL_POLYGON; break;
			}

			glBegin(face_mode);

			for(i = 0; i < face->mNumIndices; i++)		// go through all vertices in face
			{
				int vertexIndex = face->mIndices[i];	// get group index for current index
				if(mesh->mColors[0] != nullptr)
					Color4f(&mesh->mColors[0][vertexIndex]);
				if(mesh->mNormals != nullptr)

					if(mesh->HasTextureCoords(0))		//HasTextureCoords(texture_coordinates_set)
					{
						glTexCoord2f(mesh->mTextureCoords[0][vertexIndex].x, 1 - mesh->mTextureCoords[0][vertexIndex].y); //mTextureCoords[channel][vertex]
					}

					glNormal3fv(&mesh->mNormals[vertexIndex].x);
					glVertex3fv(&mesh->mVertices[vertexIndex].x);
			}
			glEnd();
		}
	}

	// draw all children
	for (n = 0; n < nd->mNumChildren; ++n)
	{
		recursive_render(sc, nd->mChildren[n], scale);
	}

	glPopMatrix();
}


void drawAiScene(const aiScene* scene)
{
	logInfo("drawing objects");

	recursive_render(scene, scene->mRootNode, 0.5);

}

int DrawGLScene()				//Here's where we do all the drawing
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear The Screen And The Depth Buffer
	glLoadIdentity();				// Reset MV Matrix


	glTranslatef(0.0f, -10.0f, -40.0f);	// Move 40 Units And Into The Screen


	glRotatef(xrot, 1.0f, 0.0f, 0.0f);
	glRotatef(yrot, 0.0f, 1.0f, 0.0f);
	glRotatef(zrot, 0.0f, 0.0f, 1.0f);

	drawAiScene(g_scene);

	yrot += 0.2f;

	return TRUE;					// okay
}


void KillGLWindow()			// Properly Kill The Window
{
	if (fullscreen)					// Are We In Fullscreen Mode?
	{
		ChangeDisplaySettings(nullptr, 0);	// If So Switch Back To The Desktop
		ShowCursor(TRUE);					// Show Mouse Pointer
	}

	if (hRC)					// Do We Have A Rendering Context?
	{
		if (!wglMakeCurrent(nullptr, nullptr))	// Are We Able To Release The DC And RC Contexts?
		{
			MessageBox(nullptr, TEXT("Release Of DC And RC Failed."), TEXT("SHUTDOWN ERROR"), MB_OK | MB_ICONINFORMATION);
		}

		if (!wglDeleteContext(hRC))			// Are We Able To Delete The RC?
		{
			MessageBox(nullptr, TEXT("Release Rendering Context Failed."), TEXT("SHUTDOWN ERROR"), MB_OK | MB_ICONINFORMATION);
		}
		hRC = nullptr;
	}

	if (hDC)
	{
		if (!ReleaseDC(g_hWnd, hDC)) // Are We able to Release The DC?
			MessageBox(nullptr, TEXT("Release Device Context Failed."), TEXT("SHUTDOWN ERROR"), MB_OK | MB_ICONINFORMATION);
		hDC = nullptr;
	}

	if (g_hWnd)
	{
		if (!DestroyWindow(g_hWnd)) // Are We Able To Destroy The Window
			MessageBox(nullptr, TEXT("Could Not Release hWnd."), TEXT("SHUTDOWN ERROR"), MB_OK | MB_ICONINFORMATION);
		g_hWnd = nullptr;
	}

	if (g_hInstance)
	{
		if (!UnregisterClass(TEXT("OpenGL"), g_hInstance)) // Are We Able To Unregister Class
			MessageBox(nullptr, TEXT("Could Not Unregister Class."), TEXT("SHUTDOWN ERROR"), MB_OK | MB_ICONINFORMATION);
		g_hInstance = nullptr;
	}
}

GLboolean abortGLInit(const char* abortMessage)
{
    // Reset Display
	KillGLWindow();
    const std::string message = abortMessage;
    std::wstring targetMessage;
    const size_t len = std::strlen(abortMessage) + 1;
    wchar_t *tmp = new wchar_t[len];
    memset(tmp, L'\0', len);
    utf8::utf8to16(message.c_str(), message.c_str() + message.size(), tmp);
    targetMessage = tmp;
    delete [] tmp;

	MessageBox(nullptr, targetMessage.c_str(), TEXT("ERROR"), MB_OK|MB_ICONEXCLAMATION);
	return FALSE;									// quit and return False
}

BOOL CreateGLWindow(const char* title, int width, int height, int bits, bool fullscreenflag)
{
	GLuint		PixelFormat;		// Hold the result after searching for a match
	WNDCLASS	wc;					// Window Class Structure
	DWORD		dwExStyle;			// Window Extended Style
	DWORD		dwStyle;			// Window Style
	RECT		WindowRect;			// Grabs Rectangle Upper Left / Lower Right Values
	WindowRect.left		= (long)0;
	WindowRect.right	= (long)width;
	WindowRect.top		= (long)0;
	WindowRect.bottom	= (long)height;

	fullscreen = fullscreenflag;

	g_hInstance = GetModuleHandle(nullptr);	// Grab An Instance For Our Window
	wc.style		= CS_HREDRAW | CS_VREDRAW | CS_OWNDC; // Redraw On Move, And Own DC For Window
	wc.lpfnWndProc	= (WNDPROC) WndProc;		// WndProc handles Messages
	wc.cbClsExtra	= 0;	// No Extra Window Data
	wc.cbWndExtra	= 0;	// No Extra Window Data
	wc.hInstance	= g_hInstance;
	wc.hIcon		= LoadIcon(nullptr, IDI_WINLOGO);	// Load The Default Icon
	wc.hCursor		= LoadCursor(nullptr, IDC_ARROW);	// Load the default arrow
	wc.hbrBackground= nullptr;						// No Background required for OpenGL
	wc.lpszMenuName	= nullptr;						// No Menu
	wc.lpszClassName= TEXT("OpenGL");		        // Class Name

	if (!RegisterClass(&wc))
	{
		MessageBox(nullptr, TEXT("Failed to register the window class"), TEXT("ERROR"), MB_OK | MB_ICONEXCLAMATION);
		return FALSE;		//exit and return false
	}

	if (fullscreen)		// attempt fullscreen mode
	{
		DEVMODE dmScreenSettings;								// Device Mode
		memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));	// Make Sure Memory's Cleared
		dmScreenSettings.dmSize = sizeof(dmScreenSettings);		// Size Of the devmode structure
		dmScreenSettings.dmPelsWidth	= width;				// Selected Screen Width
		dmScreenSettings.dmPelsHeight	= height;				// Selected Screen Height
		dmScreenSettings.dmBitsPerPel	= bits;					// bits per pixel
		dmScreenSettings.dmFields		= DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;

		// Try To Set Selected Mode and Get Results. NOTE: CDS_FULLSCREEN Gets Rid Of Start Bar.
		if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL)
		{
			// If The Mode Fails, Offer Two Options.  Quit Or Run In A Window.
			if (MessageBox(nullptr,TEXT("The Requested Fullscreen Mode Is Not Supported By\nYour Video Card. Use Windowed Mode Instead?"),
				TEXT("NeHe GL"),MB_YESNO|MB_ICONEXCLAMATION)==IDYES)
			{
				fullscreen = FALSE;		// Select Windowed Mode (Fullscreen = FALSE)
			}
			else
			{
				//Popup Messagebox: Closing
				MessageBox(nullptr, TEXT("Program will close now."), TEXT("ERROR"), MB_OK|MB_ICONSTOP);
				return FALSE; //exit, return false
			}
		}
	}

	if (fullscreen)		// when mode really succeeded
	{
		dwExStyle=WS_EX_APPWINDOW;		// Window Extended Style
		dwStyle=WS_POPUP;
		ShowCursor(FALSE);
	}
	else
	{
		dwExStyle=WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;	// Window extended style
		dwStyle=WS_OVERLAPPEDWINDOW;					// Windows style
	}

	AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);		// Adjust Window To True Requested Size

    const size_t len = std::strlen(title) + 1;
    wchar_t *tmp = new wchar_t[len];
    memset(tmp, L'\0', sizeof(wchar_t) * len);
    utf8::utf8to16(title, title+len, tmp);
    std::wstring targetMessage = tmp;
    delete[] tmp;

	if (nullptr == (g_hWnd = CreateWindowEx(dwExStyle,			// Extended Style For The Window
								TEXT("OpenGL"),						// Class Name
								targetMessage.c_str(), // Window Title
								WS_CLIPSIBLINGS |				// Required Window Style
								WS_CLIPCHILDREN |				// Required Window Style
								dwStyle,						// Selected WIndow Style
								0, 0,							// Window Position
								WindowRect.right-WindowRect.left, // Calc adjusted Window Width
								WindowRect.bottom-WindowRect.top, // Calc adjustes Window Height
								nullptr,						// No Parent Window
								nullptr,						// No Menu
								g_hInstance,					// Instance
								nullptr )))						// Don't pass anything To WM_CREATE
	{
		abortGLInit("Window Creation Error.");
		return FALSE;
	}

	static	PIXELFORMATDESCRIPTOR pfd=					// pfd Tells Windows How We Want Things To Be
	{
		sizeof(PIXELFORMATDESCRIPTOR),					// Size Of This Pixel Format Descriptor
		1,												// Version Number
		PFD_DRAW_TO_WINDOW |							// Format Must Support Window
		PFD_SUPPORT_OPENGL |							// Format Must Support OpenGL
		PFD_DOUBLEBUFFER,								// Must Support Double Buffering
		PFD_TYPE_RGBA,									// Request An RGBA Format
		BYTE(bits),											// Select Our Color Depth
		0, 0, 0, 0, 0, 0,								// Color Bits Ignored
		0,												// No Alpha Buffer
		0,												// Shift Bit Ignored
		0,												// No Accumulation Buffer
		0, 0, 0, 0,										// Accumulation Bits Ignored
		16,												// 16Bit Z-Buffer (Depth Buffer)
		0,												// No Stencil Buffer
		0,												// No Auxiliary Buffer
		PFD_MAIN_PLANE,									// Main Drawing Layer
		0,												// Reserved
		0, 0, 0											// Layer Masks Ignored
	};

	if (nullptr == (hDC=GetDC(g_hWnd)))					// Did we get the Device Context?
	{
		abortGLInit("Can't Create A GL Device Context.");
		return FALSE;
	}

	if (0 == (PixelFormat=ChoosePixelFormat(hDC, &pfd))) // Did We Find a matching pixel Format?
	{
		abortGLInit("Can't Find Suitable PixelFormat");
		return FALSE;
	}

	if (!SetPixelFormat(hDC, PixelFormat, &pfd))
	{
		abortGLInit("Can't Set The PixelFormat");
		return FALSE;
	}

	if (nullptr == (hRC=wglCreateContext(hDC)))
	{
		abortGLInit("Can't Create A GL Rendering Context.");
		return FALSE;
	}

	if (!(wglMakeCurrent(hDC,hRC)))						// Try to activate the rendering context
	{
		abortGLInit("Can't Activate The Rendering Context");
		return FALSE;
	}

	//// *** everything okay ***

	ShowWindow(g_hWnd, SW_SHOW);	// Show The Window
	SetForegroundWindow(g_hWnd);	// Slightly Higher Prio
	SetFocus(g_hWnd);				// Sets Keyboard Focus To The Window
	ReSizeGLScene(width, height);	// Set Up Our Perspective GL Screen

	if (!InitGL())
	{
		abortGLInit("Initialization failed");
		return FALSE;
	}

	return TRUE;
}

void cleanup()
{
	freeTextureIds();

	destroyAILogger();

	if (g_hWnd)
		KillGLWindow();
}

LRESULT CALLBACK WndProc(HWND hWnd,				// Handles for this Window
						 UINT uMsg,				// Message for this Window
						 WPARAM wParam,			// additional message Info
						 LPARAM lParam)			// additional message Info
{
	switch (uMsg)				// check for Window Messages
	{
		case WM_ACTIVATE:				// Watch For Window Activate Message
			{
				if (!HIWORD(wParam))	// Check Minimization State
				{
					active=TRUE;
				}
				else
				{
					active=FALSE;
				}

				return 0;				// return To The Message Loop
			}

		case WM_SYSCOMMAND:			// Interrupt System Commands
			{
				switch (wParam)
				{
					case SC_SCREENSAVE:		// Screen-saver trying to start
					case SC_MONITORPOWER:	// Monitor trying to enter power-safe
					return 0;
				}
				break;
			}

		case WM_CLOSE:			// close message received?
			{
				PostQuitMessage(0);	// Send WM_QUIT quit message
				return 0;			// Jump Back
			}

		case WM_KEYDOWN:		// Is a key pressed?
			{
				keys[wParam] = TRUE;	// If so, Mark it as true
				return 0;
			}

		case WM_KEYUP:			// Has Key Been released?
			{
				keys[wParam] = FALSE;	// If so, Mark It As FALSE
				return 0;
			}

		case WM_SIZE:			// Resize The OpenGL Window
			{
				ReSizeGLScene(LOWORD(lParam), HIWORD(lParam));	// LoWord-Width, HiWord-Height
				return 0;
			}
	}

	// Pass All unhandled Messaged To DefWindowProc
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int WINAPI WinMain( HINSTANCE /*hInstance*/,     // The instance
				   HINSTANCE /*hPrevInstance*/,  // Previous instance
				   LPSTR /*lpCmdLine*/,          // Command Line Parameters
				   int /*nShowCmd*/ )            // Window Show State
{
	MSG msg = {};
	BOOL done=FALSE;

	createAILogger();
	logInfo("App fired!");

	// Check the command line for an override file path.
	int argc;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (argv != nullptr && argc > 1)
	{
		std::wstring modelpathW(argv[1]);
        char *tmp = new char[modelpathW.size() + 1];
        memset(tmp, '\0', modelpathW.size() + 1);
        utf8::utf16to8(modelpathW.c_str(), modelpathW.c_str() + modelpathW.size(), tmp);
        modelpath = tmp;
        delete[]tmp;
	}

	if (!Import3DFromFile(modelpath))
	{
		cleanup();
		return 0;
	}

	logInfo("=============== Post Import ====================");

	if (MessageBox(nullptr, TEXT("Would You Like To Run In Fullscreen Mode?"), TEXT("Start Fullscreen?"), MB_YESNO|MB_ICONEXCLAMATION)==IDNO)
	{
		fullscreen=FALSE;
	}

	if (!CreateGLWindow(windowTitle, 640, 480, 16, fullscreen))
	{
		cleanup();
		return 0;
	}

	while(!done)	// Game Loop
	{
		if (PeekMessage(&msg, nullptr, 0,0, PM_REMOVE))
		{
			if (msg.message==WM_QUIT)
			{
				done = TRUE;
			}
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else
		{
			// Draw The Scene. Watch For ESC Key And Quit Messaged From DrawGLScene()
			if (active)
			{
				if (keys[VK_ESCAPE])
				{
					done=TRUE;
				}
				else
				{
					DrawGLScene();
					SwapBuffers(hDC);
				}
			}

			if (keys[VK_F1])
			{
				keys[VK_F1]=FALSE;
				KillGLWindow();
				fullscreen=!fullscreen;
				if (!CreateGLWindow(windowTitle, 640, 480, 16, fullscreen))
				{
					cleanup();
					return 0;
				}
			}
		}
	}

	// *** cleanup ***
	cleanup();
	return static_cast<int>(msg.wParam);
}
