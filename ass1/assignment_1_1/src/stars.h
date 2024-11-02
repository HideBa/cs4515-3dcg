#pragma once

#include <framework/disable_all_warnings.h>

DISABLE_WARNINGS_PUSH()
#include <glad/glad.h>
// Include glad before glfw3
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
DISABLE_WARNINGS_POP()
#include <framework/shader.h>

class Stars {
public:
  Stars(float orbitRadius = 100.0f, float celestialRadius = 5.0f);
  ~Stars();

  void update(float deltaTime);
  void bind() const;
  void draw(const glm::mat4 &viewMatrix, const glm::mat4 &projectionMatrix);

  // Getters for lighting information
  glm::vec3 getSunPosition() const { return m_sunPosition; }
  glm::vec3 getMoonPosition() const { return m_moonPosition; }
  glm::vec3 getSunColor() const { return m_sunColor; }
  glm::vec3 getMoonColor() const { return m_moonColor; }
  float getSunIntensity() const { return m_sunIntensity; }
  float getMoonIntensity() const { return m_moonIntensity; }

private:
  Shader m_sunShader;
  Shader m_moonShader;
  // Orbit properties
  float m_orbitRadius;
  float m_orbitAngle;
  float m_rotationSpeed;

  // Celestial body properties
  float m_celestialRadius;
  std::vector<float> m_sphereVertices;
  std::vector<unsigned int> m_sphereIndices;

  // OpenGL objects
  unsigned int m_sphereVAO, m_sphereVBO, m_sphereEBO;
  // unsigned int m_shaderProgram;

  // Position and lighting properties
  glm::vec3 m_sunPosition;
  glm::vec3 m_moonPosition;
  glm::vec3 m_sunColor;
  glm::vec3 m_moonColor;
  float m_sunIntensity;
  float m_moonIntensity;

  // Helper functions
  void generateSphere(int segments = 32);
  void setupBuffers();
  void createShader();
  void updatePositions();
};
