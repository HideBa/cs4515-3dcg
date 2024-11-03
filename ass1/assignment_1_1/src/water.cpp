// Disable compiler warnings in third-party code (which we cannot change).
#include "water.h"
#include <framework/disable_all_warnings.h>
#include <framework/opengl_includes.h>
DISABLE_WARNINGS_PUSH()
// Include glad before glfw3
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
// #define STB_IMAGE_IMPLEMENTATION
DISABLE_WARNINGS_POP()

#include <framework/shader.h>

#include <iostream>

Water::Water(float size, int resolution)
    : shader(ShaderBuilder()
                 .addStage(GL_VERTEX_SHADER,
                           RESOURCE_ROOT "shaders/water_vert.glsl")
                 .addStage(GL_FRAGMENT_SHADER,
                           RESOURCE_ROOT "shaders/water_frag.glsl")
                 .build()) {
  createMesh(size, resolution);
}

Water::~Water() {
  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);
  glDeleteBuffers(1, &ibo);
}

void Water::createMesh(float size, int resolution) {
  // Generate grid vertices
  float step = size / resolution;
  for (int z = 0; z <= resolution; z++) {
    for (int x = 0; x <= resolution; x++) {
      Vertex vertex;
      vertex.position =
          glm::vec3(x * step - size / 2, // Center the plane at origin
                    0.0f,                // Initial height
                    z * step - size / 2  // Center the plane at origin
          );
      vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f); // Up vector
      vertices.push_back(vertex);
    }
  }

  // Generate indices for triangles
  for (int z = 0; z < resolution; z++) {
    for (int x = 0; x < resolution; x++) {
      int topLeft = z * (resolution + 1) + x;
      int topRight = topLeft + 1;
      int bottomLeft = (z + 1) * (resolution + 1) + x;
      int bottomRight = bottomLeft + 1;

      // First triangle
      indices.push_back(topLeft);
      indices.push_back(bottomLeft);
      indices.push_back(topRight);

      // Second triangle
      indices.push_back(topRight);
      indices.push_back(bottomLeft);
      indices.push_back(bottomRight);
    }
  }

  // Create and bind VAO
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  // Create and bind VBO
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
               vertices.data(), GL_STATIC_DRAW);

  std::cout << "vertices are binded" << std::endl;

  // Create and bind IBO
  glGenBuffers(1, &ibo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
               indices.data(), GL_STATIC_DRAW);

  std::cout << "indices are binded" << std::endl;

  glEnableVertexAttribArray(0); // position
  glEnableVertexAttribArray(1); // normal

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)offsetof(Vertex, position));
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)offsetof(Vertex, normal));

  glBindVertexArray(0);
}

void Water::render(const glm::mat4 &model, const glm::mat4 &view,
                   const glm::mat4 &projection) {
  shader.bind();

  glUniformMatrix4fv(shader.getUniformLocation("model"), 1, GL_FALSE,
                     glm::value_ptr(model));
  glUniformMatrix4fv(shader.getUniformLocation("view"), 1, GL_FALSE,
                     glm::value_ptr(view));
  glUniformMatrix4fv(shader.getUniformLocation("projection"), 1, GL_FALSE,
                     glm::value_ptr(projection));

  float currentTime = static_cast<float>(glfwGetTime());
  glUniform1f(shader.getUniformLocation("time"),
              currentTime); // Relative time since start

  // Wave parameters
  glUniform1f(shader.getUniformLocation("amplitude1"), 0.3f);
  glUniform1f(shader.getUniformLocation("frequency1"), 2.0f);
  glUniform2fv(shader.getUniformLocation("direction1"), 1,
               glm::value_ptr(glm::vec2(1.0f, 0.0f)));
  glUniform1f(shader.getUniformLocation("speed1"), 2.0f);

  glUniform1f(shader.getUniformLocation("amplitude2"), 0.15f);
  glUniform1f(shader.getUniformLocation("frequency2"), 3.0f);
  glUniform2fv(shader.getUniformLocation("direction2"), 1,
               glm::value_ptr(glm::vec2(0.0f, 0.5f)));
  glUniform1f(shader.getUniformLocation("speed2"), 1.5f);

  // Draw water
  glBindVertexArray(vao);
  glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
}

void Water::setWaveParameters(float amp1, float freq1, const glm::vec2 &dir1,
                              float speed1, float amp2, float freq2,
                              const glm::vec2 &dir2, float speed2) {
  shader.bind();
  glUniform1f(shader.getUniformLocation("amplitude1"), amp1);
  glUniform1f(shader.getUniformLocation("frequency1"), freq1);
  glUniform2fv(shader.getUniformLocation("direction1"), 1,
               glm::value_ptr(dir1));
  glUniform1f(shader.getUniformLocation("speed1"), speed1);

  glUniform1f(shader.getUniformLocation("amplitude2"), amp2);
  glUniform1f(shader.getUniformLocation("frequency2"), freq2);
  glUniform2fv(shader.getUniformLocation("direction2"), 1,
               glm::value_ptr(dir2));
  glUniform1f(shader.getUniformLocation("speed2"), speed2);
}

void Water::cleanUp() {
  if (vao == 0) {
    return;
  }
  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);
  glDeleteBuffers(1, &ibo);
}
