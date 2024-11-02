#include "stars.h"

#include <cmath>
#include <iostream>

#include "framework/shader.h"

void checkGLError(const char *operation);

Stars::Stars(float orbit_radius, float celestial_radius)
    : m_orbitRadius(orbit_radius), m_celestialRadius(celestial_radius),
      m_orbitAngle(0.0f), m_rotationSpeed(0.1f) // Radians per second
{
  // Initialize colors and intensities
  m_sunColor = glm::vec3(1.0f, 0.95f, 0.8f); // Warm white
  m_moonColor = glm::vec3(0.6f, 0.6f, 0.8f); // Cool blue-ish
  m_sunIntensity = 1.0f;
  m_moonIntensity = 0.3f;

  generateSphere();
  setupBuffers();
  createShader();
  updatePositions();
}

Stars::~Stars() {
  glDeleteVertexArrays(1, &m_sphereVAO);
  glDeleteBuffers(1, &m_sphereVBO);
  glDeleteBuffers(1, &m_sphereEBO);
}

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

void Stars::setupBuffers() {
  // Clear any previous errors
  while (glGetError() != GL_NO_ERROR)
    ;

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

  // Setup vertex attributes
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);

  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                        (void *)(3 * sizeof(float)));

  glEnableVertexAttribArray(1);

  // Unbind VAO to prevent accidental modifications
  glBindVertexArray(0);
}

void Stars::createShader() {
  try {
    m_sunShader =
        ShaderBuilder()
            .addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/stars_vert.glsl")
            .addStage(GL_FRAGMENT_SHADER,
                      RESOURCE_ROOT "shaders/stars_frag.glsl")
            .build();

  } catch (const std::exception &e) {
    std::cerr << "Shader creation failed: " << e.what() << std::endl;
  }
}

void Stars::updatePositions() {
  m_sunPosition = glm::vec3(m_orbitRadius * std::cos(m_orbitAngle),
                            m_orbitRadius * std::sin(m_orbitAngle), 0.0f);

  m_moonPosition = glm::vec3(
      m_orbitRadius * std::cos(m_orbitAngle + glm::pi<float>()),
      m_orbitRadius * std::sin(m_orbitAngle + glm::pi<float>()), 0.0f);
}

void Stars::update(float deltaTime) {
  m_orbitAngle += m_rotationSpeed * deltaTime;
  if (m_orbitAngle > 2.0f * glm::pi<float>())
    m_orbitAngle -= 2.0f * glm::pi<float>();

  updatePositions();
}

void Stars::bind() const {

  m_sunShader.bind();

  glBindVertexArray(m_sphereVAO);
}

void Stars::draw(const glm::mat4 &viewMatrix,
                 const glm::mat4 &projectionMatrix) {
  glUniformMatrix4fv(m_sunShader.getUniformLocation("view"), 1, GL_FALSE,
                     &viewMatrix[0][0]);
  glUniformMatrix4fv(m_sunShader.getUniformLocation("projection"), 1, GL_FALSE,
                     &projectionMatrix[0][0]);

  // Draw sun
  glm::mat4 sunModel = glm::mat4(1.0f);
  sunModel = glm::translate(sunModel, m_sunPosition);
  sunModel = glm::scale(sunModel, glm::vec3(m_celestialRadius));
  glUniformMatrix4fv(m_sunShader.getUniformLocation("model"), 1, GL_FALSE,
                     &sunModel[0][0]);
  glUniform3fv(m_sunShader.getUniformLocation("color"), 1, &m_sunColor[0]);
  glDrawElements(GL_TRIANGLES, m_sphereIndices.size(), GL_UNSIGNED_INT, 0);
}

void checkGLError(const char *operation) {
  GLenum error;
  while ((error = glGetError()) != GL_NO_ERROR) {
    std::cerr << "OpenGL error after " << operation << ": " << error
              << std::endl;
  }
}