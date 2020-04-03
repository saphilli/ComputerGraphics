// Windows includes (For Time, IO, etc.)
#define NOMINMAX

#include <windows.h>
#include <mmsystem.h>
#include <iostream>
#include <string>
#include <stdio.h>
#include <math.h>
#include <vector> 
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


// OpenGL includes
#include <GL/glew.h>
#include <GL/freeglut.h>

// Assimp includes
#include <assimp/cimport.h> // scene importer
#include <assimp/scene.h> // collects data
#include <assimp/postprocess.h> // various extra operations

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include  <glm/ext/vector_float1.hpp> 
#include  <glm/ext/vector_float3.hpp>
#include  <glm/ext/vector_float4.hpp>
#include <glm/gtc/type_ptr.hpp> 

// Project includes
#include "maths_funcs.h"

/*----------------------------------------------------------------------------
MESH TO LOAD
----------------------------------------------------------------------------*/

//mesh names
#define EGG_MESH_NAME "eggUVFixed.obj"
#define ALIEN_MESH_NAME "bodyAndLegs.obj"
#define ALIEN_LEGS_UP "AlienArmsUp.obj"
#define LEGS_QUARTER_UP "AlienArmsQuarterUp.obj"
#define LEGS_HALF_UP "AlienArmsHalfUp.obj"
#define LEGS_FULLY_UP "AlienLegsFullyUp.obj"
#define LEGS_ALMOST_FULLY_UP "LegsAlmostFullyUp.obj"
#define CAVE_MESH_NAME "cave.obj"
#define TAIL_BASE_MESH "baseTail.dae"
#define TAIL_MAIN_MESH "mainTail.obj"

//textures,normal and specular maps
#define CAVE_TEXTURE "cave_texture_realistic.jpg"
#define EGG_TEXTURE "alienEggTexture.jpg"
#define ALIEN_TEXTURE "alienSkin.jpg"
#define EGG_SPECULAR "alienEggSpecular.jpg"
#define EGG_NORMALMAP "alienEggNormalMap.jpg"
#define CAVE_NORMALMAP "caveNormalScale5.jpg"
#define CAVE_SPECULAR "caveSpecular.jpg"

/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/

#pragma region SimpleTypes
typedef struct
{
	const char* mesh_name;
	size_t mPointCount = 0;
	std::vector<vec3> mVertices;
	std::vector<vec3> mNormals;
	std::vector<vec2> mTextureCoords;
} ModelData;
#pragma endregion SimpleTypes

using namespace std;
GLuint shaderProgramID1;
GLuint shaderProgramID2;

unsigned int textureIDCave;
unsigned int textureIDEgg;
unsigned int textureIDAlien;
unsigned int textureIDEggSpecular;
unsigned int eggNormalMapID;
unsigned int caveNormalMapID;
unsigned int textureIDCaveSpecular;


int width = 1680.0f;
int height =1200.0f;

//tail local rotation
GLfloat rotate_y = 0.0f, rotate_x = 0.0f , rotate_z = 0.0f;
GLfloat rotate_base_x = 0.0f, rotate_base_y = 0.0f, rotate_base_z = 0.0f;
//object local scale
GLfloat x_scale = 5.0f, y_scale = 5.0f, z_scale = 5.0f;
//camera position points
GLfloat x_pos = 0.0f, y_pos = 20.0f, z_pos = 200.0f;
//alien positining 
GLfloat alien_x = 23.0f, alien_y = 8.0f, alien_z = 50.0f;
//variables for egg scale
GLfloat egg_x = 5.0f, egg_y = 5.0f, egg_z = 5.0f;
GLfloat egg2_x = 5.2f, egg2_y = 5.2f, egg2_z = 5.2f;
//camera variables
GLfloat pitch = 0.0f, yaw = -90.0f;
GLfloat front_x = cos(pitch*ONE_DEG_IN_RAD) * cos(yaw*ONE_DEG_IN_RAD);
GLfloat front_y = sin(pitch*ONE_DEG_IN_RAD*ONE_DEG_IN_RAD);
GLfloat front_z = cos(pitch*ONE_DEG_IN_RAD*ONE_DEG_IN_RAD) * sin(yaw*ONE_DEG_IN_RAD);
GLfloat lastX = width/2.0f, lastY = height/2.0f;

vec3 cameraPos = vec3(x_pos, y_pos, z_pos);
vec3 worldUp = vec3(0.0f, 1.0f, 0.0f);
vec3 cameraFront = vec3(0.0f, 0.0f, -1.0f);
vec3 cameraUp = vec3(0.0f, 1.0f, 0.0f);
vec3 cameraRight;

bool tailMove = true;
bool test_ortho = false;
bool test_persp = true;

unsigned int load_texture(const char* file_name) {

	int width, height,numComponents;
	unsigned char* data = stbi_load(file_name,&width, &height, &numComponents, 3);
	if (data == NULL) std::cerr << "Unable to load texture: " << file_name << std::endl;
	unsigned int m_texture;
	glGenTextures(1, &m_texture);
	glBindTexture(GL_TEXTURE_2D, m_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
	stbi_image_free(data);
	return m_texture;
}

#pragma region MESH LOADING
/*----------------------------------------------------------------------------
MESH LOADING FUNCTION
----------------------------------------------------------------------------*/

ModelData load_mesh(const char* file_name) {
	ModelData modelData;
	modelData.mesh_name = file_name;
	/* Use assimp to read the model file, forcing it to be read as    */
	/* triangles. The second flag (aiProcess_PreTransformVertices) is */
	/* relevant if there are multiple meshes in the model file that   */
	/* are offset from the origin. This is pre-transform them so      */
	/* they're in the right position.                                 */
	const aiScene* scene = aiImportFile(
		file_name, 
		aiProcess_Triangulate | aiProcess_PreTransformVertices
	); 

	if (!scene) {
		fprintf(stderr, "ERROR: reading mesh %s\n", file_name);
		return modelData;
	}

	printf("  %i materials\n", scene->mNumMaterials);
	printf("  %i meshes\n", scene->mNumMeshes);
	printf("  %i textures\n", scene->mNumTextures);

	for (unsigned int m_i = 0; m_i < scene->mNumMeshes; m_i++) {
		const aiMesh* mesh = scene->mMeshes[m_i];
		printf("    %i vertices in mesh\n", mesh->mNumVertices);
		modelData.mPointCount += mesh->mNumVertices;
		for (unsigned int v_i = 0; v_i < mesh->mNumVertices; v_i++) {
			if (mesh->HasPositions()) {
				const aiVector3D* vp = &(mesh->mVertices[v_i]);
				modelData.mVertices.push_back(vec3(vp->x, vp->y, vp->z));
			}
			if (mesh->HasNormals()) {
				const aiVector3D* vn = &(mesh->mNormals[v_i]);
				modelData.mNormals.push_back(vec3(vn->x, vn->y, vn->z));
			}
			if (mesh->HasTextureCoords(0)) {
				const aiVector3D* vt = &(mesh->mTextureCoords[0][v_i]);
				modelData.mTextureCoords.push_back(vec2(vt->x, vt->y));
			}
			if (mesh->HasTangentsAndBitangents()) {

				/* You can extract tangents and bitangents here              */
				/* Note that you might need to make Assimp generate this     */
				/* data for you. Take a look at the flags that aiImportFile  */
				/* can take.                                                 */
			}
		}
	}

	aiReleaseImport(scene);
	return modelData;
}

#pragma endregion MESH LOADING

//calculates the ortho matrix
mat4 ortho(float width, float height, float nearZ, float farZ) {
	mat4 m = zero_mat4();
	m.m[0] = 12.0f / width;
	m.m[5] = 12.0f / height;
	m.m[10] = 1.0f / (nearZ - farZ);
	m.m[14] = -nearZ / (nearZ - farZ);
	m.m[15] = 1.0f;
	return m;
}

//calculates the projection matrices and switch between orthographics/perspective projection 
//takes in a boolean which determines whether the use can switch to orthographic projection
mat4 projection(bool orthoEnabled) {
	mat4 projection;
	if (test_ortho && orthoEnabled) {
		projection = ortho(width, height, 1.0f, 1000.0f);
	}
	else {
		projection = perspective(55.0f, (float)width / (float)height, 1.0f, 1000.0f);
	}
	return(projection);
}



class Object {
public:
	GLuint shaderProgramID;
	const char* mesh_name;
	ModelData  mesh_data;
	unsigned int textureID;
	bool orthoEnabled = false;
	GLuint vao;
	mat4 model;
	vector <Object> childObjects;
	unsigned int specular= 0;
	unsigned int normal = 0;
	GLfloat shininess = 1.0f;
	GLfloat isAlien = 0.0f;
	Object() {
	}
	Object(const char* mesh_name, unsigned int textureId, GLuint shaderProgram) {
		mesh_data = load_mesh(mesh_name);
		textureID = textureId;
		shaderProgramID = shaderProgram;
		GenerateObjectBuffers();
		model = identity_mat4();
	}
	void GenerateObjectBuffers();
	void Display(vec3 position, vec3 rotation, vec3 scale);
	void DisplayChild(int childOffset, vec3 posVec, vec3 rotVec, vec3 scaleVec);
	Object createChildObject(const char* mesh_name);
	void setSpecular(unsigned int specularTextureID, GLfloat newVal);
	void setNormalMap(unsigned int normalMapID);
	void setModel(ModelData mesh_data);
};

void Object::GenerateObjectBuffers() {
	int locA = glGetAttribLocation(shaderProgramID, "vertex_position");
	int locB = glGetAttribLocation(shaderProgramID, "vertex_normal");
	int locC = glGetAttribLocation(shaderProgramID, "vertex_texture");

	//init VBOs for position,normal and texture
	GLuint VBOs[3]; 
	glGenBuffers(3, VBOs);

	glBindBuffer(GL_ARRAY_BUFFER, VBOs[0]);
	glBufferData(GL_ARRAY_BUFFER, mesh_data.mPointCount * sizeof(vec3), &mesh_data.mVertices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, VBOs[1]);
	glBufferData(GL_ARRAY_BUFFER, mesh_data.mPointCount * sizeof(vec3), &mesh_data.mNormals[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, VBOs[2]);
	glBufferData(GL_ARRAY_BUFFER, mesh_data.mPointCount * sizeof(vec2), &mesh_data.mTextureCoords[0], GL_STATIC_DRAW);
	
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glEnableVertexAttribArray(locA);
	glBindBuffer(GL_ARRAY_BUFFER, VBOs[0]);
	glVertexAttribPointer(locA, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	glEnableVertexAttribArray(locB);
	glBindBuffer(GL_ARRAY_BUFFER, VBOs[1]);
	glVertexAttribPointer(locB, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	glEnableVertexAttribArray(locC);
	glBindBuffer(GL_ARRAY_BUFFER, VBOs[2]);
	glVertexAttribPointer(locC, 2, GL_FLOAT, GL_FALSE, 0, NULL);
}

void Object::setSpecular(unsigned int specularID, GLfloat shiny) {
	shininess = shiny;
	specular = specularID;
}
void Object::setModel(ModelData mesh) {
	mesh_data = mesh;
	mesh_name = mesh_data.mesh_name;
	GenerateObjectBuffers();
}

void Object::setNormalMap(unsigned int specularID) {	
	normal = specularID;
}

Object Object::createChildObject(const char* child_mesh_name) {
	Object childObject = Object(child_mesh_name, textureID, shaderProgramID);
	childObject.isAlien = isAlien;
	childObject.setSpecular(specular, shininess);
	childObjects.push_back(childObject);
	return childObject;
}

void Object::Display(vec3 position, vec3 rotation, vec3 scaleFactor) {
	glUseProgram(shaderProgramID);
	int view_mat_location = glGetUniformLocation(shaderProgramID, "view");
	int proj_mat_location = glGetUniformLocation(shaderProgramID, "proj");
	int matrix_location = glGetUniformLocation(shaderProgramID, "model");
	mat4 view = identity_mat4();
	mat4 proj = projection(orthoEnabled);
	model = identity_mat4();
	//position the camera and point at the target 
	cameraPos = vec3(x_pos, y_pos, z_pos);
	view = look_at(cameraPos, cameraPos + cameraFront, cameraUp);
	//perform rotation when the object is at the origin before translation
	model = rotate_x_deg(model, rotation.v[0]);
	model = rotate_y_deg(model, rotation.v[1]);
	model = rotate_z_deg(model, rotation.v[2]);
	model = scale(model, scaleFactor);
	model = translate(model, position);

	//update the shader uniforms
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, proj.m);
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, view.m);
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, model.m);
	glUniform3f(glGetUniformLocation(shaderProgramID, "ViewPos"), x_pos, y_pos, z_pos);

	glUniform3f(glGetUniformLocation(shaderProgramID, "light.direction"),cameraFront.v[0], cameraFront.v[1],cameraFront.v[2]);
	glUniform3f(glGetUniformLocation(shaderProgramID, "light.position"), x_pos, y_pos, z_pos);
	glUniform1f(glGetUniformLocation(shaderProgramID, "light.cutOff"), cos(12.0f*ONE_DEG_IN_RAD));
	glUniform1f(glGetUniformLocation(shaderProgramID, "light.outerCutOff"), cos(17.5f * ONE_DEG_IN_RAD));
	//update material uniforms
	glUniform1i(glGetUniformLocation(shaderProgramID, "material.texture"), 0);
	glUniform1i(glGetUniformLocation(shaderProgramID, "material.specular"), 1);
	glUniform1i(glGetUniformLocation(shaderProgramID, "material.normalMap"), 2);
    glUniform1f(glGetUniformLocation(shaderProgramID, "material.shininess"), shininess);
	glUniform1f(glGetUniformLocation(shaderProgramID, "material.isAlien"), isAlien);
	//bind the image texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureID);
	//bind the specular map
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, specular);
	//bind the normal map
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, normal);
	//bind the VAO and draw the object 
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, mesh_data.mPointCount);
}
void Object::DisplayChild(int childOffset,vec3 posVec, vec3 rotVec, vec3 scaleVec) {
	int matrix_location = glGetUniformLocation(shaderProgramID, "model");
	mat4 child_model = identity_mat4();
	child_model = rotate_x_deg(child_model, rotVec.v[0]);
	child_model = rotate_y_deg(child_model, rotVec.v[1]);
	child_model = rotate_z_deg(child_model, rotVec.v[2]);
	// Apply the parent matrix to the child matrix
	child_model = model * child_model;
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, child_model.m);
	Object child = childObjects[childOffset];
	glBindVertexArray(child.vao);
	glDrawArrays(GL_TRIANGLES, 0, child.mesh_data.mPointCount);
}


// Shader Functions- click on + to expand
#pragma region SHADER_FUNCTIONS
char* readShaderSource(const char* shaderFile) {
	FILE* fp;
	fopen_s(&fp, shaderFile, "rb");

	if (fp == NULL) { return NULL; }

	fseek(fp, 0L, SEEK_END);
	long size = ftell(fp);

	fseek(fp, 0L, SEEK_SET);
	char* buf = new char[size + 1];
	fread(buf, 1, size, fp);
	buf[size] = '\0';

	fclose(fp);

	return buf;
}

static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
	// create a shader object
	GLuint ShaderObj = glCreateShader(ShaderType);

	if (ShaderObj == 0) {
		std::cerr << "Error creating shader..." << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}
	const char* pShaderSource = readShaderSource(pShaderText);

	// Bind the source code to the shader, this happens before compilation
	glShaderSource(ShaderObj, 1, (const GLchar**)&pShaderSource, NULL);
	// compile the shader and check for errors
	glCompileShader(ShaderObj);
	GLint success;
	// check for shader related errors using glGetShaderiv
	glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLchar InfoLog[1024] = { '\0' };
		glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
		std::cerr << "Error compiling "
			<< (ShaderType == GL_VERTEX_SHADER ? "vertex" : "fragment")
			<< " shader program: " << InfoLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
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
	if (shaderProgramID == 0) {
		std::cerr << "Error creating shader program..." << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}

	// Create two shader objects, one for the vertex, and one for the fragment shader
	AddShader(shaderProgramID, VerxShader, GL_VERTEX_SHADER);
	AddShader(shaderProgramID, FragShader, GL_FRAGMENT_SHADER);

	GLint Success = 0;
	GLchar ErrorLog[1024] = { '\0' };
	// After compiling all shader objects and attaching them to the program, we can finally link it
	glLinkProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		std::cerr << "Error linking shader program: " << ErrorLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}

	// program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
	glValidateProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shaderProgramID, GL_VALIDATE_STATUS, &Success);
	if (!Success) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		std::cerr << "Invalid shader program: " << ErrorLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}
	// Finally, use the linked shader program
	// Note: this program will stay in effect for all draw calls until you replace it with another or explicitly disable its use
	glUseProgram(shaderProgramID);
	return shaderProgramID;
}
#pragma endregion SHADER_FUNCTIONS

void updateTail(GLfloat *rotate_axis_tail) {

	if (*rotate_axis_tail <= -10.0f) {
		tailMove = true;

	}
	else if (*rotate_axis_tail >= 10.0f) {
		tailMove = false;
	}

	// rotate the base at a slower rate than the main tail
	if (tailMove) {
		*rotate_axis_tail += 0.5f; //rate of change 50.0f * delta;
	}
	else {
		*rotate_axis_tail -= 0.5f;
	}
	//*rotation_axis = fmodf(*rotation_axis, 20.0f); //second param is degree of rotation - keep to 360 otherwise will 'jump'
	// Draw the next frame
	glutPostRedisplay();
}



void updateCamera() {
	GLfloat front_x = cos(ONE_DEG_IN_RAD*yaw) * cos(ONE_DEG_IN_RAD*pitch);
	GLfloat front_y = sin(ONE_DEG_IN_RAD*pitch);
	GLfloat front_z = sin(ONE_DEG_IN_RAD*(yaw)) * cos(ONE_DEG_IN_RAD*pitch);
	cameraFront = vec3(front_x, front_y, front_z);
	// Normalize vectors as the movement will slow as the length nears zero when the camera looks up/down
	cameraFront = normalise(cameraFront);
	cameraRight = normalise(cross(cameraFront, worldUp));
	cameraUp = normalise(cross(cameraRight, cameraFront));
	glutPostRedisplay();
}

//updates the egg scale in order to animate it
bool eggGrowing = true;
void updateEgg() {

	float delta = 0.0003f;
	if (egg_x > 5.2f) {
		eggGrowing = false;
	}
	else if (egg_x < 5.0f) {
		eggGrowing = true;
	}
	if (eggGrowing) {
		egg_x += delta;
		egg_y += delta;
		egg_y += delta;
		egg2_x -= delta;
		egg2_y -= delta;
		egg2_z -= delta;
	}
	else {
		egg_x -= delta;
		egg_y -= delta;
		egg_y -= delta;
		egg2_x += delta;
		egg2_y += delta;
		egg2_z += delta;
		
	}
}

Object cave, egg, tail_base, body;
Object LegsFullyUp, LegsAlmostFullyUp,LegsUp, LegsQuarterUp, LegsHalfUp;
vector<Object> keyFrames;

void init()
{
	// Set up the shaders
	shaderProgramID1 = CompileShaders("simpleFragmentShader.txt", "simpleVertexShader.txt");
	//load textures
	textureIDEgg = load_texture(EGG_TEXTURE);
	textureIDCave = load_texture(CAVE_TEXTURE);
	textureIDAlien = load_texture(ALIEN_TEXTURE);
	textureIDEggSpecular = load_texture(EGG_SPECULAR);
	textureIDCaveSpecular = load_texture(CAVE_SPECULAR);
	eggNormalMapID = load_texture(EGG_NORMALMAP);
	caveNormalMapID = load_texture(CAVE_NORMALMAP);
	//create each object, buffers are generated on creation of each object
	cave = Object(CAVE_MESH_NAME, textureIDCave, shaderProgramID1);
	cave.setSpecular(textureIDCaveSpecular, 1.0f);
	cave.setNormalMap(caveNormalMapID);
	egg = Object(EGG_MESH_NAME, textureIDEgg, shaderProgramID1);
	egg.setSpecular(textureIDEggSpecular,32.0f);
	egg.setNormalMap(eggNormalMapID);
	body = Object(ALIEN_MESH_NAME, textureIDAlien, shaderProgramID1);
	body.setSpecular(textureIDEggSpecular, 50.0f);
	body.isAlien = 1.0f;
	
	//create the alien objects for keyframes
	LegsFullyUp = Object(LEGS_FULLY_UP, textureIDAlien, shaderProgramID1);
	LegsAlmostFullyUp = Object(LEGS_ALMOST_FULLY_UP, textureIDAlien, shaderProgramID1);
	LegsQuarterUp = Object(LEGS_QUARTER_UP, textureIDAlien, shaderProgramID1);
	LegsHalfUp = Object(LEGS_HALF_UP, textureIDAlien, shaderProgramID1);
	LegsUp = Object(ALIEN_LEGS_UP, textureIDAlien, shaderProgramID1);

	//set material properties
	LegsFullyUp.setSpecular(textureIDEggSpecular, 50.0f);
	LegsFullyUp.isAlien = 1.0f;
	LegsAlmostFullyUp.setSpecular(textureIDEggSpecular, 50.0f);
	LegsAlmostFullyUp.isAlien = 1.0f;
	LegsUp.setSpecular(textureIDEggSpecular, 50.0f);
	LegsUp.isAlien = 1.0f;
	LegsQuarterUp.setSpecular(textureIDEggSpecular, 50.0f);
	LegsQuarterUp.isAlien = 1.0f;
	LegsHalfUp.setSpecular(textureIDEggSpecular, 50.0f);
	LegsHalfUp.isAlien = 1.0f;
	//push to vector in order the models will be drawn in
	keyFrames.push_back(body);
	keyFrames.push_back(LegsQuarterUp);
	keyFrames.push_back(LegsHalfUp);
	keyFrames.push_back(LegsUp);
	keyFrames.push_back(LegsAlmostFullyUp);
	keyFrames.push_back(LegsFullyUp);
	//create tail hierarchy
	tail_base = Object(TAIL_BASE_MESH, textureIDAlien, shaderProgramID1);
	tail_base.isAlien = 1.0f;
	tail_base.setSpecular(textureIDEggSpecular, 50.0f);
	tail_base.createChildObject(TAIL_MAIN_MESH);
}

int currentFrame = 0;
bool armsGoingUp = true;
void display() {

	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//draw each model, passing in vectors to determine position, rotation and scale respectively.
	cave.Display(vec3(0,53, 0), vec3(0, 0, 0), vec3(4, 4, 4));
	egg.Display(vec3(23, 0, 70), vec3(0, 0, 0), vec3(egg_x, egg_y, egg_z));
	egg.Display(vec3(-23, 0, -128), vec3(0, 40.0f, 0), vec3(egg_x, egg_y, egg_z));
	egg.Display(vec3(18, 0, -30), vec3(0, -156.0f, 0), vec3(egg_x, egg_y, egg_z));
	egg.Display(vec3(-23, -0.3, 40), vec3(2, 215.0f, 0), vec3(egg2_x, egg2_y, egg2_z));
	egg.Display(vec3(-30, 0, -80), vec3(0, 220.0f, 0), vec3(egg2_x, egg2_y, egg2_z));
	keyFrames[currentFrame].Display(vec3(alien_x, alien_y, alien_z), vec3(0, 0, 0), vec3(x_scale, y_scale, z_scale));
	tail_base.Display(vec3(alien_x, alien_y, alien_z), vec3(rotate_x, rotate_y, rotate_z), vec3(x_scale, y_scale, z_scale));
	tail_base.DisplayChild(0, vec3(alien_x, alien_y, alien_z), vec3(rotate_x, rotate_y, rotate_z), vec3(x_scale, y_scale, z_scale));
	glutSwapBuffers();
	updateEgg();
}


// keypress user input
void keypress(unsigned char key, int x, int y) {
	switch (key) {
	case 'x': {
		updateTail(&rotate_x);
		break;
	}
	case 'y': {
		updateTail(&rotate_y);
		break;
	}
	case 'z': {
		updateTail(&rotate_z);
		break;
	}
	case 's': {
		x_pos -= cameraFront.v[0];
		z_pos -= cameraFront.v[2];
		break;
	}
	case 'w': {
		if (alien_x > 0.0f) {
			alien_x -= 1.0f;
		}
		x_pos += cameraFront.v[0];
		z_pos += cameraFront.v[2];
		break;
	}
	case 'd': {
		x_pos += cameraRight.v[0];
		z_pos += cameraRight.v[2];
		break;
	}
	case 'a': {
		x_pos -= cameraRight.v[0];
		z_pos -= cameraRight.v[2];
		break;
	}
	case 'q': {
		y_pos -= 1.0f;
		break;
	}
	case 'e': {
		y_pos += 1.0f;
		break;
	}
	case '+': {
		x_scale += 1.0f;
		y_scale += 1.0f;
		z_scale += 1.0f;
		break;
	}
	case '-': {
		x_scale -= 1.0f;
		y_scale -= 1.0f;
		z_scale -= 1.0f;
		break;
	}
	case 'o': {
		test_ortho = true;
		break;
	}
	case 'p': {
		test_ortho = false;
		break;
	}
	//determines what keyframe is being drawn for the keyframe animation of the alien
	case 'i': {
		if (currentFrame == keyFrames.size()-1) {
			armsGoingUp = false;
		}
		else if (currentFrame == 0) {
			armsGoingUp = true;
		}
		if (currentFrame < keyFrames.size()-1 && armsGoingUp ) {
			currentFrame++;
			glutPostRedisplay();
		}
		else if (currentFrame > 0 && !armsGoingUp) {
			currentFrame--;
			glutPostRedisplay();
		}
		break;
	}
     //if ESC key is pressed (27), glut will exit the main loop and the program will terminate 
	case 27: {
		glutLeaveMainLoop();
		break;
	}
	}
	glutPostRedisplay();

}
//keep cursor in center to allow for more degrees of freedom 
void centerCursor() {
	glutWarpPointer(width / 2, height / 2);
	lastX = width / 2;
	lastY = height / 2;
}

void mouseCallback(int xpos, int ypos) {

	//pin the cursor to the center of the screen 
	centerCursor();
	GLfloat xoffset = xpos - lastX;
	GLfloat yoffset = lastY - ypos; // reversed since y-coordinates range from bottom to top
	lastX = xpos;
	lastY = ypos;

	GLfloat sensitivity = 0.05f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	//limit the pitch so the camera degrees of freedom resembles a person looking down or up
	if(pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;
	updateCamera();
}


int main(int argc, char** argv) {

	// Set up the window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(width, height);
	glutCreateWindow("Final Graphics Project");
	// Tell glut where the display function is
	glutDisplayFunc(display);
	//glutIdleFunc(updateScene);
	glutKeyboardFunc(keypress);
	glutSetCursor(GLUT_CURSOR_NONE);
	centerCursor();
	glutPassiveMotionFunc(mouseCallback);
	glutMotionFunc(mouseCallback);
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
