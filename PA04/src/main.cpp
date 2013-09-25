#include <GL/glew.h> // glew must be included before the main gl libs
#include <GL/glut.h> // doing otherwise causes compiler shouting
#include <iostream>
#include <stdio.h>
#include <chrono>
#include <vector>
#include <fstream>
#include <sstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> //Makes passing matrices to shaders easier


//--Data types
//This object will define the attributes of a vertex(position, color, etc...)
struct Vertex
{
    GLfloat position[3];
    GLfloat color[3];
};

//GLUT Fonts
  void * glutFonts[7] = {
  GLUT_BITMAP_9_BY_15,
  GLUT_BITMAP_8_BY_13,
  GLUT_BITMAP_TIMES_ROMAN_10,
  GLUT_BITMAP_TIMES_ROMAN_24,
  GLUT_BITMAP_HELVETICA_10,
  GLUT_BITMAP_HELVETICA_12,
  GLUT_BITMAP_HELVETICA_18 };

//--Evil Global variables
//Just for this example!
int w = 640, h = 480;// Window size
int ROTATION_FLAG = 0;
int SPIN_MOD = 1;
int PLANET_MOD = 1;
float SPEED_MOD = 3;
GLuint program;// The GLSL program handle
GLuint vbo_geometry;// VBO handle for our geometry
unsigned int numVertices=36; 
char *objFileName="assets/models/table.obj";
//uniform locations
GLint loc_mvpmat;// Location of the modelviewprojection matrix in the shader

//attribute locations
GLint loc_position;
GLint loc_color;

//Multiple models
std::vector<glm::mat4> models;

//transform matrices
glm::mat4 model;//obj->world each object should have its own model matrix
glm::mat4 view;//world->eye
glm::mat4 projection;//eye->clip
glm::mat4 mvp;//premultiplied modelviewprojection

//--GLUT Callbacks
void render();
void update();
void reshape(int n_w, int n_h);
void keyboard(unsigned char key, int x_pos, int y_pos);
void keypressSpecial(int key, int x_pos, int y_pos);
void mouse(int button, int state, int x, int y);
void rotation_menu(int id);

//--Resource management
bool initialize();
void cleanUp();

//--Random time things
float getDT();
std::chrono::time_point<std::chrono::high_resolution_clock> t1,t2;

//Shader Loader
const char* loadShaderFromFile(const char* fileName);

//OBJ loader
bool loadOBJ(const char * fileName, std::vector<Vertex> &geometry); 

//String splitter
void split(const std::string &s, std::vector<unsigned int> &elems);

//Text display
void glutPrintText(float x, float y, char* text, void * font, 
                              float r, float g, float b, float a);
//--Main
int main(int argc, char **argv)
{	
	if( argc > 1 )
	{
		objFileName=argv[1];
	}
    // Initialize glut
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(w, h);
    // Name and create the Window
    glutCreateWindow("Object Loading Example");

    // Now that the window is created the GL context is fully set up
    // Because of that we can now initialize GLEW to prepare work with shaders
    GLenum status = glewInit();
    if( status != GLEW_OK)
    {
        std::cerr << "[F] GLEW NOT INITIALIZED: ";
        std::cerr << glewGetErrorString(status) << std::endl;
        return -1;
    }

    // Set all of the callbacks to GLUT that we need
    glutDisplayFunc(render);// Called when its time to display
    glutReshapeFunc(reshape);// Called if the window is resized
    glutIdleFunc(update);// Called if there is nothing else to do
    glutKeyboardFunc(keyboard);// Called if there is keyboard input
    glutSpecialFunc(keypressSpecial);// Called if there is special keyboard input
    glutMouseFunc(mouse); //Called on mouse click
    
    //Add our right click menu
    /*
    glutCreateMenu(rotation_menu);
    glutAddMenuEntry("Start Rotation", 1);
    glutAddMenuEntry("Stop Rotation", 2);
    glutAddMenuEntry("Quit", 3);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
    */
    // Initialize all of our resources(shaders, geometry)
    bool init = initialize();
    if(init)
    {
        t1 = std::chrono::high_resolution_clock::now();
        glutMainLoop();
    }

    // Clean up after ourselves
    cleanUp();
    return 0;
}

//--Implementations
void render()
{
    //--Render the scene

    //clear the screen
    glClearColor(0.0, 0.0, 0.2, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    
    for (unsigned int i=0;i<models.size(); i++) 
    {
      
      mvp = projection * view * models[i];

      //enable the shader program
      glUseProgram(program);

      //upload the matrix to the shader
      glUniformMatrix4fv(loc_mvpmat, 1, GL_FALSE, glm::value_ptr(mvp));

      //set up the Vertex Buffer Object so it can be drawn
      glEnableVertexAttribArray(loc_position);
      glEnableVertexAttribArray(loc_color);
      glBindBuffer(GL_ARRAY_BUFFER, vbo_geometry);
      //set pointers into the vbo for each of the attributes(position and color)
      glVertexAttribPointer( loc_position,//location of attribute
                             3,//number of elements
                             GL_FLOAT,//type
                             GL_FALSE,//normalized?
                             sizeof(Vertex),//stride
                             0);//offset

      glVertexAttribPointer( loc_color,
                             3,
                             GL_FLOAT,
                             GL_FALSE,
                             sizeof(Vertex),
                             (void*)offsetof(Vertex,color));

      glDrawArrays(GL_TRIANGLES, 0, numVertices);//mode, starting index, count
    }
    //clean up
    glDisableVertexAttribArray(loc_position);
    glDisableVertexAttribArray(loc_color);
                           
    //swap the buffers
    glutSwapBuffers(); 
    
}

void update()
{
    static float rotAngle = 0.0;
    
    float dt = getDT();// if you have anything moving, use dt.

    rotAngle += dt*90;
    //THIS IS THE OBJECT'S UPDATE
    models[0] = glm::rotate(glm::mat4(1.0f), rotAngle, glm::vec3(0, 1, 0));
	//models[0] = glm::scale(models[0], glm::vec3(5,5,5));
    // Update the state of the scene
    glutPostRedisplay();//call the display callback
}


void reshape(int n_w, int n_h)
{
    w = n_w;
    h = n_h;
    //Change the viewport to be correct
    glViewport( 0, 0, w, h);
    //Update the projection matrix as well
    //See the init function for an explaination
    projection = glm::perspective(45.0f, float(w)/float(h), 0.01f, 100.0f);

}

void keyboard(unsigned char key, int x_pos, int y_pos)
{
    //std::cout << int(key) << std::endl;
    // Handle keyboard input
    if( key == 65 || key == 97 )//a or A
    {
        SPIN_MOD *= -1;
    }
    if( key == 45 || key == 95 ) // - or _
    {
      if( SPEED_MOD > 1 )
        SPEED_MOD -= 0.5;
    }
    if( key == 43 || key == 61) // + or =
    {
      if( SPEED_MOD < 5 )
        SPEED_MOD += 0.5;
    }
    if(key == 27)//ESC
    {
        exit(0);
    }
}
void keypressSpecial (int key, int x, int y)
{	
  glutSetKeyRepeat(0);

	if( key == GLUT_KEY_LEFT ) //turn planet clockwise
  {
    PLANET_MOD=-1;
  }
  if( key == GLUT_KEY_RIGHT ) //turn planet counter clockwise
  {
    PLANET_MOD=1;
  }
	
}
void mouse(int button, int state, int x, int y)
{
  if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
  {
    SPIN_MOD *= -1;
  }
}

bool initialize()
{
    // Initialize basic geometry and shaders for this example
    std::vector<Vertex> geometryVec;
    loadOBJ(objFileName,geometryVec);
    
    Vertex geometry[geometryVec.size()];
    numVertices = geometryVec.size();
    for( unsigned int i=0; i<geometryVec.size(); i++ )
    {
      geometry[i] = geometryVec[i];
    }
    // Create a Vertex Buffer object to store this vertex info on the GPU
    glGenBuffers(1, &vbo_geometry);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_geometry);
    glBufferData(GL_ARRAY_BUFFER, sizeof(geometry), geometry, GL_STATIC_DRAW);

    //--Geometry done

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

    //Shader Sources
    // Now uses the shader loader
    // Given our current file structure, these paths should always work
    const char *vs = loadShaderFromFile("assets/shaders/vs.txt");
    const char *fs = loadShaderFromFile("assets/shaders/fs.txt");
    
    //compile the shaders
    GLint shader_status;

    // Vertex shader first
    glShaderSource(vertex_shader, 1, &vs, NULL);
    glCompileShader(vertex_shader);
    //check the compile status
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &shader_status);
    if(!shader_status)
    {
        std::cerr << "[F] FAILED TO COMPILE VERTEX SHADER!" << std::endl;
        return false;
    }

    // Now the Fragment shader
    glShaderSource(fragment_shader, 1, &fs, NULL);
    glCompileShader(fragment_shader);
    //check the compile status
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &shader_status);
    if(!shader_status)
    {
        std::cerr << "[F] FAILED TO COMPILE FRAGMENT SHADER!" << std::endl;
        return false;
    }

    //Now we link the 2 shader objects into a program
    //This program is what is run on the GPU
    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    //check if everything linked ok
    glGetProgramiv(program, GL_LINK_STATUS, &shader_status);
    if(!shader_status)
    {
        std::cerr << "[F] THE SHADER PROGRAM FAILED TO LINK" << std::endl;
        return false;
    }

    //Now we set the locations of the attributes and uniforms
    //this allows us to access them easily while rendering
    loc_position = glGetAttribLocation(program,
                    const_cast<const char*>("v_position"));
    if(loc_position == -1)
    {
        std::cerr << "[F] POSITION NOT FOUND" << std::endl;
        return false;
    }

    loc_color = glGetAttribLocation(program,
                    const_cast<const char*>("v_color"));
    if(loc_color == -1)
    {
        std::cerr << "[F] V_COLOR NOT FOUND" << std::endl;
        return false;
    }

    loc_mvpmat = glGetUniformLocation(program,
                    const_cast<const char*>("mvpMatrix"));
    if(loc_mvpmat == -1)
    {
        std::cerr << "[F] MVPMATRIX NOT FOUND" << std::endl;
        return false;
    }
    
    //--Init the view and projection matrices
    //  if you will be having a moving camera the view matrix will need to more dynamic
    //  ...Like you should update it before you render more dynamic 
    //  for this project having them static will be fine
    view = glm::lookAt( glm::vec3(0.0, 8.0, -16.0), //Eye Position
                        glm::vec3(0.0, 0.0, 0.0), //Focus point
                        glm::vec3(0.0, 1.0, 0.0)); //Positive Y is up

    projection = glm::perspective( 45.0f, //the FoV typically 90 degrees is good which is what this is set to
                                   float(w)/float(h), //Aspect Ratio, so Circles stay Circular
                                   0.01f, //Distance to the near plane, normally a small value like this
                                   100.0f); //Distance to the far plane, 

    //enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    //load our models
    models.push_back(model);
    //and its done
    return true;
}

void cleanUp()
{
    // Clean up, Clean up
    glDeleteProgram(program);
    glDeleteBuffers(1, &vbo_geometry);
}

//returns the time delta
float getDT()
{
    float ret;
    t2 = std::chrono::high_resolution_clock::now();
    ret = std::chrono::duration_cast< std::chrono::duration<float> >(t2-t1).count();
    t1 = std::chrono::high_resolution_clock::now();
    return ret;
}

//Loads a shader from a text file
const char* loadShaderFromFile(const char* fileName)
{
  std::string fileContents;
  
  std::ifstream in(fileName, std::ios::in | std::ios::binary);
  if (in)
  {
    in.seekg(0, std::ios::end);
    fileContents.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&fileContents[0], fileContents.size());
    in.close();
  }
  else
  {
    std::cout << std::endl << "Could not open shader file: " << fileName << std::endl << std::endl;
    throw;
  }
  
  char * shader = new char[fileContents.size()];
  strcpy(shader, fileContents.c_str());
  return shader;
}

void rotation_menu(int id)
{
  switch(id)
  {
    //Start rotation
    case 1:
      ROTATION_FLAG = 1;
      break;
    //Stop rotation
    case 2:
      ROTATION_FLAG = 0;
      break;
    //Quit
    case 3:
      exit(0);
      break;
  }
  glutPostRedisplay();
}

void glutPrintText(float x, float y, char* text, void * font, 
                              float r, float g, float b, float a)
{
    // disable shaders
    glUseProgram(0);

    // set color of text
    glColor3f(r,g,b);

    // set position of text
    glRasterPos2f(x,y);
    
    // iterate through string to print text
    while (*text) {
        glutBitmapCharacter(font, *text);
        text++;
    }
 
    // re-enable shaders
    glUseProgram(program);
}

bool loadOBJ(const char * fileName, std::vector<Vertex> &geometry) 
{
    geometry.clear();
    std::vector<unsigned int> vertexIndices;
    std::vector<glm::vec3> temp_vertices; 

    FILE * file = fopen(fileName, "r");
    if ( file == NULL ) {
        printf("ERROR: Object file not found!!");
        exit(-1);
    }

    while (true) {

        char lineHeader[128]; 

        //quit when end of line
        if (fscanf(file, "%s", lineHeader) == EOF)
            break;

    //get vertices
        if ( strcmp( lineHeader, "v" ) == 0 ) 
        {
            glm::vec3 vertex;
            fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z );
            temp_vertices.push_back(vertex);
        }
      
        //get faces
        else if ( strcmp( lineHeader, "f" ) == 0 ) 
        {
            std::vector<unsigned int> lineIndices;
            char buff[1024];
            
            //Get the list of vertices
            fgets(buff,1024,file);
            
            //Split the list into a vector
            split(buff,lineIndices);
            
            //Triangulate the face
            for( unsigned int i=1; i < lineIndices.size()-1; i++ )
            {
              vertexIndices.push_back(lineIndices[0]);
              vertexIndices.push_back(lineIndices[i]);
              vertexIndices.push_back(lineIndices[i+1]);
            }
        }



        else {
            //junk? or something we haven't learned yet
            char buff[1000];
            fgets(buff, 1000, file);
        }

    }
    srand(time(NULL));
    //for all the vertices
    for ( unsigned int i=0; i<vertexIndices.size(); i++ ) 
    {

        //get index
        unsigned int vertexIndex = vertexIndices[i];

        Vertex newVertex; 
        
        glm::vec3 tmpVec = temp_vertices[ vertexIndex-1 ];
        newVertex.position[0] = (fabs(tmpVec.x) < 1e-20)? 0 : tmpVec.x;
        newVertex.position[1] = (fabs(tmpVec.y) < 1e-20)? 0 : tmpVec.y;
        newVertex.position[2] = (fabs(tmpVec.z) < 1e-20)? 0 : tmpVec.z;
        newVertex.color[0] = (float)rand()/(float)RAND_MAX;
        newVertex.color[1] = (float)rand()/(float)RAND_MAX;
        newVertex.color[2] = (float)rand()/(float)RAND_MAX;
        
        geometry.push_back(newVertex);
    }

    //std::cout << "done!"<<std::endl;
    return true;

}
void split(const std::string &s, std::vector<unsigned int> &elems) 
{
    elems.clear();
    std::istringstream is( s );
    unsigned int n;
	if( s.find('/',0) !=std::string::npos )
	{
		char dummy;
		unsigned int dummyN;
		while( is >> n ) 
		{
			elems.push_back(n);
			is >> dummy;
			is >> dummyN;
			is >> dummy;
			is >> dummyN;	
		}
	}
	else
	{
		while( is >> n ) 
		{
		     elems.push_back(n);
		}
	}
}
