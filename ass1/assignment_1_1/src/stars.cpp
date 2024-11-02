#include "stars.h"

#include <cmath>
#include <iostream>

#include "framework/shader.h"
#include "glm/fwd.hpp"

#include <glm/gtc/matrix_inverse.hpp> // For inverse matrix calculations

void checkGLError(const char *operation);

// Constructor
Stars::Stars(float orbit_radius, float celestial_radius)
    : m_orbitRadius(orbit_radius), m_celestialRadius(celestial_radius),
      m_orbitAngle(0.0f), m_rotationSpeed(0.1f) // Radians per second
{
  // Initialize colors and intensities
  m_sunColor = glm::vec3(1.0f, 0.95f, 0.8f); // Warm white
  m_moonColor = glm::vec3(0.6f, 0.6f, 0.8f); // Cool blue-ish
  m_sunIntensity = 1.0f;
  m_moonIntensity = 0.3f;

  generateSphere(
      32); // Pass a valid number of segments (e.g., 32 for smoothness)
  setupBuffers();
  createShader();
  updatePositions();
}

// Destructor
Stars::~Stars() {
  glDeleteVertexArrays(1, &m_sphereVAO);
  glDeleteBuffers(1, &m_sphereVBO);
  glDeleteBuffers(1, &m_sphereEBO);
}

// Sphere Generation
void Stars::generateSphere(int segments) {
  m_sphereVertices.clear();
  m_sphereIndices.clear();

  for (int y = 0; y <= segments; y++) {
    for (int x = 0; x <= segments; x++) {
      float xSegment = (float)x / (float)segments;
      float ySegment = (float)y / (float)segments;
      float xPos = std::cos(xSegment * 2.0f * M_PI) * std::sin(ySegment * M_PI);
      float yPos = std::cos(ySegment * M_PI);
      float zPos = std::sin(xSegment * 2.0f * M_PI) * std::sin(ySegment * M_PI);

      // Position
      m_sphereVertices.push_back(xPos);
      m_sphereVertices.push_back(yPos);
      m_sphereVertices.push_back(zPos);
      // Normal
      m_sphereVertices.push_back(xPos);
      m_sphereVertices.push_back(yPos);
      m_sphereVertices.push_back(zPos);
    }
  }

  // Generate indices
  for (int y = 0; y < segments; y++) {
    for (int x = 0; x < segments; x++) {
      m_sphereIndices.push_back((y + 1) * (segments + 1) + x);
      m_sphereIndices.push_back(y * (segments + 1) + x);
      m_sphereIndices.push_back(y * (segments + 1) + x + 1);

      m_sphereIndices.push_back((y + 1) * (segments + 1) + x);
      m_sphereIndices.push_back(y * (segments + 1) + x + 1);
      m_sphereIndices.push_back((y + 1) * (segments + 1) + x + 1);
    }
  }
}

// Buffer Setup
void Stars::setupBuffers() {

  // Generate and bind VAO
  glGenVertexArrays(1, &m_sphereVAO);
  glBindVertexArray(m_sphereVAO);

  // Generate and setup VBO
  glGenBuffers(1, &m_sphereVBO);
  glBindBuffer(GL_ARRAY_BUFFER, m_sphereVBO);
  glBufferData(GL_ARRAY_BUFFER, m_sphereVertices.size() * sizeof(float),
               m_sphereVertices.data(), GL_STATIC_DRAW);

  // Generate and setup EBO
  glGenBuffers(1, &m_sphereEBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_sphereEBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               m_sphereIndices.size() * sizeof(unsigned int),
               m_sphereIndices.data(), GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  glBindBuffer(GL_ARRAY_BUFFER, m_sphereVBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_sphereEBO);

  // Setup vertex attributes
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  // Unbind VAO to prevent accidental modifications
  glBindVertexArray(0);
}

// Shader Creation
void Stars::createShader() {
  try {
    ShaderBuilder sunShaderBuilder;
    sunShaderBuilder.addStage(GL_VERTEX_SHADER,
                              RESOURCE_ROOT "shaders/stars_vert.glsl");
    sunShaderBuilder.addStage(GL_FRAGMENT_SHADER,
                              RESOURCE_ROOT "shaders/stars_frag.glsl");
    m_sunShader = sunShaderBuilder.build();

    // Optionally, print uniform locations for debugging
    std::cout << "Sun Shader Uniform Locations:" << std::endl;
    std::cout << "view: " << m_sunShader.getUniformLocation("view")
              << std::endl;
    std::cout << "projection: " << m_sunShader.getUniformLocation("projection")
              << std::endl;
    std::cout << "model: " << m_sunShader.getUniformLocation("model")
              << std::endl;
    std::cout << "color: " << m_sunShader.getUniformLocation("color")
              << std::endl;

  } catch (const std::exception &e) {
    std::cerr << "Shader creation failed: " << e.what() << std::endl;
  }
}

// Update Positions (Sun and Moon)
void Stars::updatePositions() {
  m_sunPosition = glm::vec3(m_orbitRadius * std::cos(m_orbitAngle),
                            m_orbitRadius * std::sin(m_orbitAngle), 0.0f);

  m_moonPosition = glm::vec3(
      m_orbitRadius * std::cos(m_orbitAngle + glm::pi<float>()),
      m_orbitRadius * std::sin(m_orbitAngle + glm::pi<float>()), 0.0f);
}

// Update Method (Animation)
void Stars::update(float deltaTime) {
  m_orbitAngle += m_rotationSpeed * deltaTime;
  if (m_orbitAngle > 2.0f * glm::pi<float>())
    m_orbitAngle -= 2.0f * glm::pi<float>();

  updatePositions();
}

// Draw Method with Orthogonal Projection Integration
void Stars::draw(const glm::mat4 &viewMatrix, const glm::mat4 &projectionMatrix,
                 const glm::vec3 &cameraPos, float screenWidth,
                 float screenHeight) {
  // // Save current OpenGL state
  // GLboolean depthMaskPrev;
  // glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMaskPrev);
  // GLint depthFuncPrev;
  // glGetIntegerv(GL_DEPTH_FUNC, &depthFuncPrev);
  // GLboolean cullFacePrev = glIsEnabled(GL_CULL_FACE);

  // // Setup orthographic projection
  // float orthoLeft = 0.0f;
  // float orthoRight = screenWidth;
  // float orthoBottom = 0.0f;
  // float orthoTop = screenHeight;
  // float orthoNear = -1.0f;
  // float orthoFar = 1.0f;

  // glm::mat4 sunProjection = glm::ortho(orthoLeft, orthoRight, orthoBottom,
  //                                      orthoTop, orthoNear, orthoFar);

  // // Calculate screen position
  // glm::vec4 sunWorldPos = glm::vec4(m_sunPosition, 1.0f);
  // glm::mat4 mvp = projectionMatrix * viewMatrix;
  // glm::vec4 sunClipSpace = mvp * sunWorldPos;
  // glm::vec3 sunNDC = glm::vec3(sunClipSpace) / sunClipSpace.w;
  // glm::vec2 sunScreenPos = glm::vec2((sunNDC.x + 1.0f) / 2.0f * screenWidth,
  //                                    (sunNDC.y + 1.0f) / 2.0f *
  //                                    screenHeight);

  // // Setup model matrix
  // glm::mat4 sunModel = glm::mat4(1.0f);
  // sunModel = glm::translate(sunModel, glm::vec3(sunScreenPos, 0.0f));
  // float sunSize = 40.0f;
  // sunModel = glm::scale(sunModel, glm::vec3(sunSize, sunSize, sunSize));

  // // Setup view matrix
  // glm::mat4 sunView = glm::mat4(1.0f);

  // // Bind shader and set uniforms
  // m_sunShader.bind();
  // glUniformMatrix4fv(m_sunShader.getUniformLocation("view"), 1, GL_FALSE,
  //                    glm::value_ptr(sunView));
  // glUniformMatrix4fv(m_sunShader.getUniformLocation("projection"), 1,
  // GL_FALSE,
  //                    glm::value_ptr(sunProjection));
  // glUniformMatrix4fv(m_sunShader.getUniformLocation("model"), 1, GL_FALSE,
  //                    glm::value_ptr(sunModel));
  // glUniform3fv(m_sunShader.getUniformLocation("color"), 1,
  //              glm::value_ptr(m_sunColor));

  // // Setup rendering state
  // glDisable(GL_CULL_FACE); // Disable face culling
  // glDepthMask(GL_FALSE);   // Disable depth writing
  // glDepthFunc(GL_ALWAYS);  // Always pass depth test

  // // Draw the sun
  // glBindVertexArray(m_sphereVAO);
  // glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_sphereIndices.size()),
  //                GL_UNSIGNED_INT, 0);
  // glBindVertexArray(0);

  // // Restore previous OpenGL state
  // glDepthMask(depthMaskPrev);
  // glDepthFunc(depthFuncPrev);
  // if (cullFacePrev) {
  //   glEnable(GL_CULL_FACE);
  // }

  m_sunShader.bind();
  // For debugging, simply draw point at sun position
  const glm::vec4 screenPos = projectionMatrix * viewMatrix *
                              glm::mat4({1.0f}) *
                              glm::vec4(m_sunPosition, 1.0f);

  std::cout << "Sun Position: " << m_sunPosition.x << ", " << m_sunPosition.y
            << ", " << m_sunPosition.z << std::endl;
  glPointSize(40.0f);
  glUniform4fv(m_sunShader.getUniformLocation("pos"), 1,
               glm::value_ptr(glm::vec4(m_sunPosition, 1.0f)));
  glUniform3fv(m_sunShader.getUniformLocation("color"), 1,
               glm::value_ptr(m_sunColor));
  glBindVertexArray(m_sphereVAO);
  glad_glDrawArrays(GL_POINTS, 0, 1);
  glBindVertexArray(0);
}

// OpenGL Error Checking Function
void checkGLError(const char *operation) {
  GLenum error;
  while ((error = glGetError()) != GL_NO_ERROR) {
    std::cerr << "OpenGL error after " << operation << ": " << error
              << std::endl;
  }
}
