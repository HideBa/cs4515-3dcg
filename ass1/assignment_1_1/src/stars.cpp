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

  generateSphere(32);
  setupBuffers();
  setupTempBuffers();
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

  // Generate vertices
  for (int y = 0; y <= segments; y++) {
    float phi = float(y) * M_PI / float(segments);
    for (int x = 0; x <= segments; x++) {
      float theta = float(x) * 2.0f * M_PI / float(segments);

      // Calculate position
      float xPos = m_celestialRadius * sin(phi) * cos(theta);
      float yPos = m_celestialRadius * cos(phi);
      float zPos = m_celestialRadius * sin(phi) * sin(theta);

      // Position
      m_sphereVertices.push_back(xPos);
      m_sphereVertices.push_back(yPos);
      m_sphereVertices.push_back(zPos);

      // Normal (normalized position for a sphere)
      glm::vec3 normal = glm::normalize(glm::vec3(xPos, yPos, zPos));
      m_sphereVertices.push_back(normal.x);
      m_sphereVertices.push_back(normal.y);
      m_sphereVertices.push_back(normal.z);
    }
  }

  // Generate indices
  for (int y = 0; y < segments; y++) {
    for (int x = 0; x < segments; x++) {
      int current = y * (segments + 1) + x;
      int next = current + 1;
      int bottom = (y + 1) * (segments + 1) + x;
      int bottomNext = bottom + 1;

      // First triangle
      m_sphereIndices.push_back(current);
      m_sphereIndices.push_back(bottom);
      m_sphereIndices.push_back(next);

      // Second triangle
      m_sphereIndices.push_back(next);
      m_sphereIndices.push_back(bottom);
      m_sphereIndices.push_back(bottomNext);
    }
  }
}

// Temporary Buffer Setup
void Stars::setupTempBuffers() {
  glGenVertexArrays(1, &m_tempVAO);
  glBindVertexArray(m_tempVAO);

  glBindVertexArray(0);
}

// Buffer Setup
void Stars::setupBuffers() {

  // Generate and bind VAO
  glGenVertexArrays(1, &m_sphereVAO);
  glBindVertexArray(m_sphereVAO);

  // Generate and bind VBO
  glGenBuffers(1, &m_sphereVBO);
  glBindBuffer(GL_ARRAY_BUFFER, m_sphereVBO);
  glBufferData(GL_ARRAY_BUFFER, m_sphereVertices.size() * sizeof(float),
               m_sphereVertices.data(), GL_STATIC_DRAW);

  // Generate and bind EBO
  glGenBuffers(1, &m_sphereEBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_sphereEBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               m_sphereIndices.size() * sizeof(unsigned int),
               m_sphereIndices.data(), GL_STATIC_DRAW);

  // Setup vertex attributes
  // Position attribute
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  // Normal attribute
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  // Unbind VAO (but keep EBO bound)
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

  m_sunShader.bind();

  // Calculate model matrix for sun position
  glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), m_sunPosition);

  // Send matrices to shader
  glUniformMatrix4fv(m_sunShader.getUniformLocation("model"), 1, GL_FALSE,
                     glm::value_ptr(modelMatrix));
  glUniformMatrix4fv(m_sunShader.getUniformLocation("view"), 1, GL_FALSE,
                     glm::value_ptr(viewMatrix));
  glUniformMatrix4fv(m_sunShader.getUniformLocation("projection"), 1, GL_FALSE,
                     glm::value_ptr(projectionMatrix));

  // Send color
  glUniform4fv(m_sunShader.getUniformLocation("color"), 1,
               glm::value_ptr(glm::vec4(m_sunColor, 1.0f)));

  // Draw sphere
  glBindVertexArray(m_sphereVAO);
  glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_sphereIndices.size()),
                 GL_UNSIGNED_INT, 0);
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
