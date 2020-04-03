
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include  <glm/ext/vector_float1.hpp> 
#include  <glm/ext/vector_float3.hpp>
#include  <glm/ext/vector_float4.hpp>
#include <glm/gtc/type_ptr.hpp>

// Macro for indexing vertex buffer
#define BUFFER_OFFSET(i) ((char *)NULL + (i))



using namespace std;

// Vertex Shader (for convenience, it is defined in the main here, but we will be using text files for shaders in future)
// Note: Input to this shader is the vertex positions that we specified for the triangle. 
// Note: gl_Position is a special built-in variable that is supposed to contain the vertex position (in X, Y, Z, W)
// Since our triangle vertices were specified as vec3, we just set W to 1.0.

static const char* pVS = "                                                    \n\
#version 330                                                                  \n\
                                                                              \n\
in vec3 vPosition;															  \n\
in vec4 vColor;																  \n\
out vec4 color;																\n\
uniform mat4 model;                                                            \n\
uniform mat4 view;                                                            \n\
uniform mat4 projection;                                                         \n\
                                                                               \n\
void main()                                                                     \n\
{                                                                                \n\
    gl_Position = vec4(vPosition, 1.0);                \n\
	color = vColor;							                                     \n\
}";

// Fragment Shader 1
//Update: input color is the output color coming from the Vertex shader. 
// Note: no input in this shader, it just outputs the colour of all fragments, in this case set to red (format: R, G, B, A).
static const char* pFS = "                                              \n\
#version 330                                                            \n\
                                                                        \n\
in vec4 color;														 \n\
out vec4 FragColor;                                                      \n\
                                                                          \n\
void main()                                                               \n\
{                                                                          \n\
FragColor = color;															 \n\
}";

//Vertex Shader 2
static const char* pVS2 = "                                                    \n\
#version 330                                                                  \n\
                                                                              \n\
in vec3 vPosition;															  \n\
in vec4 vColor;																  \n\
out vec4 color;																 \n\
                                                                              \n\
                                                                               \n\
void main()                                                                     \n\
{                                                                                \n\
    gl_Position = vec4(vPosition.x, vPosition.y, vPosition.z, 1.0);                \n\
	color = vColor;							                                     \n\
}";
// Fragment Shader 2
//Update: input color is the output color coming from the Vertex shader. 
// Note: no input in this shader, it just outputs the colour of all fragments, in this case set to red (format: R, G, B, A).
static const char* pFS2 = "                                              \n\
#version 330                                                            \n\
                                                                        \n\
in vec4 color;														 \n\
out vec4 FragColor;                                                      \n\
                                                                          \n\
void main()                                                               \n\
{                                                                          \n\
FragColor = color;														 \n\
}";


GLuint VAOs[2];
GLuint VBOs[2];
GLuint shaderProgramID1;
GLuint shaderProgramID2;
int numVertices = 3;

// Shader Functions- click on + to expand
#pragma region SHADER_FUNCTIONS
static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
	// create a shader object
    GLuint ShaderObj = glCreateShader(ShaderType);

    if (ShaderObj == 0) {
        fprintf(stderr, "Error creating shader type %d\n", ShaderType);
        exit(0);
    }
	// Bind the source code to the shader, this happens before compilation
	glShaderSource(ShaderObj, 1, (const GLchar**)&pShaderText, NULL);
	// compile the shader and check for errors
    glCompileShader(ShaderObj);
    GLint success;
	// check for shader related errors using glGetShaderiv
    glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar InfoLog[1024];
        glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
        fprintf(stderr, "Error compiling shader type %d: '%s'\n", ShaderType, InfoLog);
        exit(1);
    }
	// Attach the compiled shader object to the program object
    glAttachShader(ShaderProgram, ShaderObj);
}

GLuint CompileShaders(const char * FragShader, const char * VerxShader)
{
	//Start the process of setting up our shaders by creating a program ID
	//Note: we will link all the shaders together into this ID
	GLuint shaderProgramID = glCreateProgram();

    if ( shaderProgramID == 0) {
        fprintf(stderr, "Error creating shader program\n");
        exit(1);
    }
	
	//Create two shader objects
	AddShader(shaderProgramID, VerxShader, GL_VERTEX_SHADER);
	AddShader(shaderProgramID, FragShader, GL_FRAGMENT_SHADER);

    GLint Success = 0;
    GLchar ErrorLog[1024] = { 0 };


	// After compiling all shader objects and attaching them to the program, we can finally link it
  //  glLinkProgram(shaderProgramID);
	glLinkProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
    glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		fprintf(stderr, "Error linking shader program: '%s'\n", ErrorLog);
        exit(1);
	}

	// program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
	glValidateProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
    glGetProgramiv(shaderProgramID, GL_VALIDATE_STATUS, &Success);
    if (!Success) {
        glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
        fprintf(stderr, "Invalid shader program: '%s'\n", ErrorLog);
        exit(1);
    }
	// Finally, use the linked shader program
	// Note: this program will stay in effect for all draw calls until you replace it with another or explicitly disable its use
	
	return shaderProgramID;
}
#pragma endregion SHADER_FUNCTIONS

// VBO Functions - click on + to expand
#pragma region VBO_FUNCTION
GLuint generateObjectBuffer(float vertices[], glm::vec4 colors[]) {
	GLuint numVertices = 3;
	// Generate 1 generic buffer object, called VBO
	// In OpenGL, we bind (make active) the handle to a target name  and then execute commands on that target
	// Buffer will contain an array of vertices 
	// After binding, we now fill our object with data, everything in "Vertices" goes to the GPU
	glBufferData(GL_ARRAY_BUFFER, numVertices*7*sizeof(GLfloat),NULL, GL_STATIC_DRAW);
	// if you have more data besides vertices (e.g., vertex colours or normals), use glBufferSubData to tell the buffer when the vertices array ends and when the colors start
	glBufferSubData (GL_ARRAY_BUFFER, 0, numVertices*3*sizeof(GLfloat), vertices);
	glBufferSubData (GL_ARRAY_BUFFER, numVertices*3*sizeof(GLfloat), numVertices*4*sizeof(GLfloat), colors);	

	return VBOs[0];
}



void linkCurrentBuffertoShader(GLuint shaderProgramID){
	GLuint numVertices = 3;
	// find the location of the variables that we will be using in the shader program
	GLuint positionID = glGetAttribLocation(shaderProgramID, "vPosition");
	GLuint colorID = glGetAttribLocation(shaderProgramID, "vColor");
	// Have to enable this
	glEnableVertexAttribArray(positionID);
	// Tell it where to find the position data in the currently active buffer (at index positionID)
    glVertexAttribPointer(positionID, 3, GL_FLOAT, GL_FALSE, 0, 0);
	// Similarly, for the color data.
	glEnableVertexAttribArray(colorID);
	glVertexAttribPointer(colorID, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(numVertices*3*sizeof(GLfloat)));

}
#pragma endregion VBO_FUNCTIONS





void init()
{
	//model matrix setup
	

	glm::mat4 model = glm::mat4(1.0f); //initialize matrix to identity matrix first
	glm::mat4 view = glm::mat4(1.0f);
	glm::mat4 projection = glm::mat4(1.0f);

	model = glm::rotate(model, glm::radians(-55.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));
	projection = glm::perspective(glm::radians(45.0f), (float)1000 / (float)800, 0.1f, 100.0f);
	
	// note that we're translating the scene in the reverse direction of where we want to move
	GLuint modelLoc = glGetUniformLocation(shaderProgramID1, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	GLuint viewLoc = glGetUniformLocation(shaderProgramID1, "view");
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

	GLuint projLoc = glGetUniformLocation(shaderProgramID1, "projection");
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	// triangle vertices and colours
	float vertices1[] = {
		-0.5f, -0.5f, -0.5f,
		 0.5f, -0.5f, -0.5f,
		 0.5f,  0.5f, -0.5f
	};
	 glm::vec4 colors1[] = {
		 glm::vec4(1.0f, 1.0f, 0.75f, 1.0f),
		 glm::vec4(1.0f, 0.0f, 1.0f, 1.0f),
		 glm::vec4(1.0f, 0.0f, 1.0f, 1.0f)
	};
	// Set up the shaders
	shaderProgramID1 = CompileShaders(pFS, pVS);
	//send matrices to the shader
	
	//shaderProgramID1::setMat4("view", view);
	//shaderProgramID1::setMat4("projection", projection);

	// Put the vertices and colors into a vertex buffer object
	glGenVertexArrays(2, VAOs);
	glGenBuffers(2, VBOs);
	glBindVertexArray(VAOs[0]);
	glBindBuffer(GL_ARRAY_BUFFER, VBOs[0]);
	generateObjectBuffer(vertices1, colors1);
	// Link the current buffer to the shader
	linkCurrentBuffertoShader(shaderProgramID1);

	
	


}

void display() {
	
	
	// NB: Make the call to draw the geometry in the currently activated vertex buffer. This is where the GPU starts to work!
	//Bind VAO for first triangle and draw

	glBindVertexArray(VAOs[0]);
	glUseProgram(shaderProgramID1);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	// Second triangle
//	glBindVertexArray(VAOs[1]);
//	glUseProgram(shaderProgramID2);
//	glDrawArrays(GL_TRIANGLES, 0, 3);
	glutSwapBuffers();

	glBindVertexArray(0);
	
}	
int main(int argc, char** argv){

	// Set up the window
	glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
    glutInitWindowSize(1000, 800);
    glutCreateWindow("Hello Triangle");
	// Tell glut where the display function is
	glutDisplayFunc(display);
	 // A call to glewInit() must be done after glut is initialized!
    GLenum res = glewInit();
	// Check for any errors
    if (res != GLEW_OK) {
      fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
      return 1;
    }
	// Set up your objects and shaders
	init();
	// Begin infinite event loop
	glutMainLoop();
    return 0;
}











