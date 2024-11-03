#pragma once

#include <framework/disable_all_warnings.h>
#include <framework/opengl_includes.h>
DISABLE_WARNINGS_PUSH()
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
DISABLE_WARNINGS_POP()
#include <framework/shader.h>
#include <vector>

class Water {
public:
  Water(float size = 10.0f, int resolution = 100);
  ~Water();

  void render(const glm::mat4 &model, const glm::mat4 &view,
              const glm::mat4 &projection);
  void setWaveParameters(float amp1, float freq1, const glm::vec2 &dir1,
                         float speed1, float amp2, float freq2,
                         const glm::vec2 &dir2, float speed2);
  void cleanUp();

private:
  struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
  };

  void createMesh(float size, int resolution);

  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;
  GLuint vao, vbo, ibo;
  Shader shader;
};