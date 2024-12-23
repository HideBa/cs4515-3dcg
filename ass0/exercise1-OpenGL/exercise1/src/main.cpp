// Disable compiler warnings in third-party code (which we cannot change).
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glad/glad.h>
// Include glad before glfw3.
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
DISABLE_WARNINGS_POP()
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <filesystem>
#include <framework/mesh.h>
#include <framework/trackball.h>
#include <framework/window.h>
#include <imgui/imgui.h>
#include <iostream>
#include <memory>
#include <numeric>
#include <span>
#include <string>
#include <vector>

// START READING HERE!!!

//////Predefined global variables

// Use the enum values to define different rendering modes
// The mode is used by the function display and the mode is
// chosen during execution with the keys 1-9
enum class DisplayModeType {
  TRIANGLE,
  FACE,
  CUBE,
  ARM,
  MESH,
};

bool show_imgui = true;
bool lighting_enabled = false;
DisplayModeType displayMode = DisplayModeType::TRIANGLE;
constexpr glm::ivec2 resolution{800, 800};
std::unique_ptr<Window> pWindow;
std::unique_ptr<Trackball> pTrackball;

glm::vec4 lightPos{1.0f, 1.0f, 0.4f, 1.0f};
Mesh mesh;

// Declare your own global variables here:
int myVariableThatServesNoPurpose;

////////// Draw Functions

// function to draw coordinate axes with a certain length (1 as a default)
void drawCoordSystem(const float length = 1) {
  // draw simply colored axes

  // remember all states of the GPU
  glPushAttrib(GL_ALL_ATTRIB_BITS);
  // deactivate the lighting state
  glDisable(GL_LIGHTING);
  // draw axes
  glBegin(GL_LINES);
  glColor3f(1, 0, 0);
  glVertex3f(0, 0, 0);
  glVertex3f(length, 0, 0);

  glColor3f(0, 1, 0);
  glVertex3f(0, 0, 0);
  glVertex3f(0, length, 0);

  glColor3f(0, 0, 1);
  glVertex3f(0, 0, 0);
  glVertex3f(0, 0, length);
  glEnd();

  // reset to previous state
  glPopAttrib();
}

/**
 * Several drawing functions for you to work on
 */

void drawTriangle() {
  // A simple example of a drawing function for a triangle
  // 1) Try changing its color to red
  // 2) Try changing its vertex positions
  // 3) Add a second triangle in blue
  // 4) Add a global variable (initialized at 0), which represents the
  //   x-coordinate of the first vertex of each triangle
  // 5) Go to the function animate and increment this variable
  //   by a small value - observe the animation.

  glColor3f(0, 1, 1);
  glNormal3f(0, 0, 1);
  glBegin(GL_TRIANGLES);
  glVertex3f(0, 0, 0);
  glVertex3f(3, 10, 0);
  glVertex3f(1, 1, 0);
  glEnd();
  glBegin(GL_TRIANGLES);
  glVertex3f(0, 0, 0);
  glVertex3f(9, 9, 0);
  glVertex3f(8, 8, 0);
  glEnd();
}

void drawUnitFace(const glm::mat4 &transformMatrix) {
  // 1) Draw a unit quad in the x,y plane oriented along the z axis
  // 2) Make sure the orientation of the vertices is positive (counterclock
  // wise) 3) What happens if the order is inversed? 4) Transform the quad by
  // the given transformation matrix.
  //
  //  For more information on how to use glm (OpenGL Mathematics Library), see:
  //  https://github.com/g-truc/glm/blob/master/manual.md#section1
  //
  //  The documentation of the various matrix transforms can be found here:
  //  https://glm.g-truc.net/0.9.9/api/a00247.html
  //
  //  Please note that the glm matrix operations transform an existing matrix.
  //  To get just a rotation/translation/scale matrix you can pass an identity
  //  matrix (glm::mat4(1.0f)). Also, keep in mind that the rotation angle is
  //  specified in radians. You can use glm::degrees(angleInRadians) to convert
  //  from radians to degrees, and glm::radians(angleInDegrees) for the reverse
  //  operation. Be carefull! these functions only take floating point based
  //  types.
  //
  //  For example (rotate 90 degrees around the x axis):
  //  glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f),
  //  glm::vec3(1, 0, 0));
}

void drawUnitCube(const glm::mat4 &transformMatrix) {
  // 1) Draw a cube using your function drawUnitFace. Use glm::translate(Matrix,
  // Vector)
  //    and glm::rotate(Matrix, Angle, Vector) to create the transformation
  //    matrices passed to drawUnitFace.
  // 2) Transform your cube by the given transformation matrix.
}

// Global variables to control the angles between the arm parts
float upperArmAngle = 0.0f;
float forearmAngle = 0.0f;
float handAngle = 0.0f;

void drawArm() {
  // Upper arm
  glm::mat4 upperArmTransform = glm::mat4(1.0f);
  upperArmTransform = glm::rotate(
      upperArmTransform, glm::radians(upperArmAngle), glm::vec3(0, 0, 1));
  upperArmTransform = glm::scale(upperArmTransform, glm::vec3(1, 3, 1));
  drawUnitCube(upperArmTransform);

  // Forearm
  glm::mat4 forearmTransform = glm::mat4(1.0f);
  forearmTransform = glm::translate(forearmTransform, glm::vec3(0, 3, 0));
  forearmTransform = glm::rotate(forearmTransform, glm::radians(forearmAngle),
                                 glm::vec3(0, 0, 1));
  forearmTransform = glm::scale(forearmTransform, glm::vec3(1, 2, 1));
  drawUnitCube(forearmTransform);

  // Hand
  glm::mat4 handTransform = glm::mat4(1.0f);
  handTransform = glm::translate(handTransform, glm::vec3(0, 5, 0));
  handTransform =
      glm::rotate(handTransform, glm::radians(handAngle), glm::vec3(0, 0, 1));
  handTransform = glm::scale(handTransform, glm::vec3(1, 1, 1));
  drawUnitCube(handTransform);
}

void drawLight() {
  // 1) Draw a cube at the light's position lightPos using your drawUnitCube
  // function.
  //    To make the light source bright, follow the drawCoordSystem function
  //    To deactivate the lighting temporarily and draw it in yellow.

  // 2) Make the light position controllable via the keyboard function

  // 3) Add normal information to all your faces of the previous functions
  //    and observe the shading after pressing 'L' to activate the lighting.
  //    You can use 'l' to turn it off again.

  // 4) OPTIONAL
  //    Draw a sphere (consisting of triangles) instead of a cube.
}

void drawMesh() {
  // 1) Use the mesh data structure;
  //    Each triangle is defined with 3 consecutive indices in the meshTriangles
  //    table. These indices correspond to vertices stored in the meshVertices
  //    table. Provide a function that draws these triangles.

  // 2) Compute the normals of these triangles

  // 3) Try computing a normal per vertex as the average of the adjacent face
  // normals.
  //    Call glNormal3f with the corresponding values before each vertex.
  //    What do you observe with respect to the lighting?

  // 4) Try loading your own model (export it from Blender as a Wavefront obj)
  // and replace the provided mesh file.
}

void display() {
  // set the light to the right position
  //  glLightfv(GL_LIGHT0, GL_POSITION, glm::value_ptr(lightPos));
  drawLight();

  switch (displayMode) {
  case DisplayModeType::TRIANGLE:
    drawCoordSystem();
    drawTriangle();
    break;
  case DisplayModeType::FACE:
    drawUnitFace(glm::mat4(1.0f)); // mat4(1.0f) = identity matrix
    break;
    // ...
  default:
    break;
  }
}

/**
 * Animation
 */
void animate() {
  static float angle = 0.0f;
  angle += 0.01f; // Increment the angle for animation

  // Update the light position based on the angle
  float radius = 5.0f; // Radius of the circular path
  lightPos = glm::vec3(radius * cos(angle), lightPos.y, radius * sin(angle));

  // Redraw the scene with the updated light position
  display();
}

// Take keyboard input into account.
void keyboard(int key, int /* scancode */, int action, int /* mods */) {
  glm::dvec2 cursorPos = pWindow->getCursorPos();
  std::cout << "Key " << key << " pressed at " << cursorPos.x << ", "
            << cursorPos.y << "\n";

  if (key == '`' && action == GLFW_PRESS) {
    show_imgui = !show_imgui;
  }

  switch (key) {
  case GLFW_KEY_1: {
    displayMode = DisplayModeType::TRIANGLE;
    break;
  }
  case GLFW_KEY_2: {
    displayMode = DisplayModeType::FACE;
    break;
  }
  case GLFW_KEY_3: {
    displayMode = DisplayModeType::CUBE;
    break;
  }
  case GLFW_KEY_4: {
    displayMode = DisplayModeType::ARM;
    break;
  }
  case GLFW_KEY_5: {
    displayMode = DisplayModeType::MESH;
    break;
  }
  case GLFW_KEY_ESCAPE: {
    pWindow->close();
    break;
  }
  case GLFW_KEY_L: {
    // Turn lighting on.
    if (pWindow->isKeyPressed(GLFW_KEY_LEFT_SHIFT) ||
        pWindow->isKeyPressed(GLFW_KEY_RIGHT_SHIFT)) {
      lighting_enabled = true;
      glEnable(GL_LIGHTING);
    } else {
      lighting_enabled = false;
      glDisable(GL_LIGHTING);
    }
    break;
  }
  };
}

void imgui() {

  if (!show_imgui)
    return;

  ImGui::Begin("Practical 1: Intro to OpenGL");
  ImGui::Text("Press ` to show/hide this menu");

  // Declare display modes and names
  std::array displayModeNames{"1: TRIANGLE", "2: FACE", "3: CUBE", "4: ARM",
                              "5: MESH"};

  const std::array displayModes{DisplayModeType::TRIANGLE,
                                DisplayModeType::FACE, DisplayModeType::CUBE,
                                DisplayModeType::ARM, DisplayModeType::MESH};

  // get the index of the current display mode, as current mode
  int current_mode = static_cast<int>(displayMode);

  // update current mode based on menu
  ImGui::Combo("Display Mode", &current_mode, displayModeNames.data(),
               displayModeNames.size());

  // set display mode
  displayMode = displayModes[current_mode];

  ImGui::Checkbox("Lighting Enabled", &lighting_enabled);

  if (lighting_enabled) {
    glEnable(GL_LIGHTING);
  } else {
    glDisable(GL_LIGHTING);
  }

  ImGui::Separator();
  ImGui::Text("Use this UI as an example, feel free to implement custom UI if "
              "it is useful");

  // Checkbox
  static bool checkbox;

  ImGui::Checkbox("Example Checkbox", &checkbox);

  // Slider
  static float sliderValue = 0.0f;
  ImGui::SliderFloat("Float Slider", &sliderValue, 0.0f, 1.0f);

  static int intSliderValue = 0.0f;
  ImGui::SliderInt("Int Slider", &intSliderValue, 0, 10);

  // Color Picker
  static glm::vec3 color = glm::vec3(0.45f, 0.55f, 0.60f);
  ImGui::ColorEdit3(
      "Color Picker",
      glm::value_ptr(color)); // Use glm::value_ptr to get the float*

  if (ImGui::Button("Button")) {
    std::cout << "Button Clicked" << std::endl;
  }

  static char text[128] = "Some Text";
  ImGui::InputText("Input Text", text, IM_ARRAYSIZE(text));

  static int intValue = 0;
  ImGui::InputInt("Input Int", &intValue);

  static float floatValue = 0.0f;
  ImGui::InputFloat("Input Float", &floatValue);

  ImGui::End();
  ImGui::Render();
}

// Nothing needed below this point
// STOP READING //STOP READING //STOP READING

void displayInternal(void);
void reshape(const glm::ivec2 &);
void init() {
  // Initialize viewpoint
  pTrackball->printHelp();
  reshape(resolution);

  glDisable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_COLOR_MATERIAL);
  glEnable(GL_NORMALIZE);

  // int MatSpec [4] = {1,1,1,1};
  //    glMaterialiv(GL_FRONT_AND_BACK,GL_SPECULAR,MatSpec);
  //    glMateriali(GL_FRONT_AND_BACK,GL_SHININESS,10);

  // Enable Depth test
  glEnable(GL_DEPTH_TEST);

  // Draw frontfacing polygons as filled.
  glPolygonMode(GL_FRONT, GL_FILL);
  // Draw backfacing polygons as outlined.
  glPolygonMode(GL_BACK, GL_LINE);
  glShadeModel(GL_SMOOTH);
  mesh = loadMesh("David.obj", true)[0];
}

// Program entry point. Everything starts here.
int main(int /* argc */, char **argv) {
  pWindow = std::make_unique<Window>(argv[0], resolution, OpenGLVersion::GL2);
  pTrackball = std::make_unique<Trackball>(pWindow.get(), glm::radians(50.0f));
  pWindow->registerKeyCallback(keyboard);
  pWindow->registerWindowResizeCallback(reshape);

  init();

  while (!pWindow->shouldClose()) {
    pWindow->updateInput();
    imgui();

    animate();
    displayInternal();

    pWindow->swapBuffers();
  }
}

// OpenGL helpers. You don't need to touch these.
void displayInternal(void) {
  // Clear screen
  glViewport(0, 0, pWindow->getWindowSize().x, pWindow->getWindowSize().y);
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Load identity matrix
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // Load up view transforms
  const glm::mat4 viewTransform = pTrackball->viewMatrix();
  glMultMatrixf(glm::value_ptr(viewTransform));

  // Your rendering function
  animate();
  display();
}
void reshape(const glm::ivec2 &size) {
  // Called when the window is resized.
  // Update the viewport and projection matrix.
  glViewport(0, 0, size.x, size.y);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  const glm::mat4 perspectiveMatrix = pTrackball->projectionMatrix();
  glLoadMatrixf(glm::value_ptr(perspectiveMatrix));
  glMatrixMode(GL_MODELVIEW);
}
