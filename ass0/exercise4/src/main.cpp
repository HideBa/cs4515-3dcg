#include "camera.h"
// Suppress warnings in third-party code.
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
// Include GLEW before GLFW
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/vec3.hpp>
// Library for loading an image
#include <stb/stb_image.h>
DISABLE_WARNINGS_POP()
#include <array>
#include <framework/mesh.h>
#include <framework/shader.h>
#include <framework/window.h>
#include <iostream>
#include <span>
#include <vector>

// Configuration
constexpr int WIDTH = 800;
constexpr int HEIGHT = 600;

int main() {
  Window window{"Shadow Mapping", glm::ivec2(WIDTH, HEIGHT),
                OpenGLVersion::GL41};

  Camera camera{&window, glm::vec3(1.2f, 1.1f, 0.9f),
                -glm::vec3(1.2f, 1.1f, 0.9f)};
  Camera camera2{&window, glm::vec3(0.2f, 1.1f, 0.9f),
                 -glm::vec3(0.2f, 1.1f, 0.9f)};
  constexpr float fov = glm::pi<float>() / 4.0f;
  constexpr float aspect =
      static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);
  const glm::mat4 mainProjectionMatrix =
      glm::perspective(fov, aspect, 0.1f, 30.0f);

  // === Modify for exercise 1 ===
  // Key handle function
  Camera *activeCam = &camera;
  Camera *inactiveCam = &camera2;
  window.registerKeyCallback(
      [&](int key, int /* scancode */, int action, int /* mods */) {
        if (action == GLFW_PRESS) {
          switch (key) {
          case GLFW_KEY_1:
            camera2.setUserInteraction(false);
            camera.setUserInteraction(true);
            activeCam = &camera;
            inactiveCam = &camera2;
            break;
          case GLFW_KEY_2:
            camera2.setUserInteraction(true);
            camera.setUserInteraction(false);
            activeCam = &camera2;
            inactiveCam = &camera;
            break;
          default:
            break;
          }
        }
      });

  const Shader mainShader =
      ShaderBuilder()
          .addStage(GL_VERTEX_SHADER, "shaders/shader_vert.glsl")
          .addStage(GL_FRAGMENT_SHADER, "shaders/shader_frag.glsl")
          .build();

  // const Shader shadowShader =
  //     ShaderBuilder()
  //         .addStage(GL_VERTEX_SHADER, "shaders/shadow_vert.glsl")
  //         .build();

  // === Load a texture for exercise 5 ===
  int texWidth, texHeight, texChannels;
  stbi_uc *pixels =
      stbi_load("resources/smiley.png", &texWidth, &texHeight, &texChannels, 3);
  if (!pixels) {
    std::cerr << "Failed to load texture!" << std::endl;
    return -1;
  }

  GLuint texLight;
  glCreateTextures(GL_TEXTURE_2D, 1, &texLight);
  glTextureStorage2D(texLight, 1, GL_RGB8, texWidth, texHeight);
  glTextureSubImage2D(texLight, 0, 0, 0, texWidth, texHeight, GL_RGB,
                      GL_UNSIGNED_BYTE, pixels);

  // Free the loaded texture data
  stbi_image_free(pixels);

  // Set behaviour for when texture coordinates are outside the [0, 1] range.
  glTextureParameteri(texLight, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTextureParameteri(texLight, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  // Set interpolation for texture sampling (GL_NEAREST for no interpolation).
  glTextureParameteri(texLight, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTextureParameteri(texLight, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // Load mesh from disk.
  const Mesh mesh = mergeMeshes(loadMesh("resources/scene.obj"));
  if (mesh.vertices.empty()) {
    std::cerr << "Failed to load mesh!" << std::endl;
    return -1;
  }

  // Create Element(Index) Buffer Object and Vertex Buffer Objects.
  GLuint ibo;
  glCreateBuffers(1, &ibo);
  glNamedBufferStorage(
      ibo,
      static_cast<GLsizeiptr>(mesh.triangles.size() *
                              sizeof(decltype(Mesh::triangles)::value_type)),
      mesh.triangles.data(), 0);

  GLuint vbo;
  glCreateBuffers(1, &vbo);
  glNamedBufferStorage(
      vbo, static_cast<GLsizeiptr>(mesh.vertices.size() * sizeof(Vertex)),
      mesh.vertices.data(), 0);

  // Bind vertex data to shader inputs using their index (location).
  GLuint vao;
  glCreateVertexArrays(1, &vao);

  glVertexArrayElementBuffer(vao, ibo);
  glVertexArrayVertexBuffer(vao, 0, vbo, offsetof(Vertex, position),
                            sizeof(Vertex));
  glVertexArrayVertexBuffer(vao, 1, vbo, offsetof(Vertex, normal),
                            sizeof(Vertex));
  glEnableVertexArrayAttrib(vao, 0);
  glEnableVertexArrayAttrib(vao, 1);

  // === Create Shadow Texture ===
  GLuint texShadow;
  const int SHADOWTEX_WIDTH = 1024;
  const int SHADOWTEX_HEIGHT = 1024;
  glCreateTextures(GL_TEXTURE_2D, 1, &texShadow);
  glTextureStorage2D(texShadow, 1, GL_DEPTH_COMPONENT32F, SHADOWTEX_WIDTH,
                     SHADOWTEX_HEIGHT);

  glTextureParameteri(texShadow, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTextureParameteri(texShadow, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTextureParameteri(texShadow, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTextureParameteri(texShadow, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  // === Create framebuffer for extra texture ===
  GLuint framebuffer;
  glCreateFramebuffers(1, &framebuffer);
  glNamedFramebufferTexture(framebuffer, GL_DEPTH_ATTACHMENT, texShadow, 0);

  // Check if framebuffer is complete
  if (glCheckNamedFramebufferStatus(framebuffer, GL_FRAMEBUFFER) !=
      GL_FRAMEBUFFER_COMPLETE) {
    std::cerr << "Framebuffer is not complete!" << std::endl;
    return -1;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // Main loop
  while (!window.shouldClose()) {
    window.updateInput();

    // === Render the shadow map ===
    // shadowShader.bind();
    // glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    // glClearDepth(1.0);
    // glClear(GL_DEPTH_BUFFER_BIT);
    // glEnable(GL_DEPTH_TEST);

    // // Set the light-space matrix for the shadow map rendering
    // const glm::mat4 lightSpaceMatrix =
    //     mainProjectionMatrix * inactiveCam->viewMatrix();
    // glUniformMatrix4fv(shadowShader.getUniformLocation("lightSpaceMatrix"),
    // 1,
    //                    GL_FALSE, glm::value_ptr(lightSpaceMatrix));

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES,
                   static_cast<GLsizei>(mesh.triangles.size() * 3),
                   GL_UNSIGNED_INT, nullptr);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // === Render the main image ===
    mainShader.bind();
    activeCam->updateInput();
    if (!pixels) {
      std::cerr << "Failed to load texture!" << std::endl;
      return -1; // Exit on failure
    }

    if (mainShader.getUniformLocation("mvp") == -1) {
      std::cerr << "Uniform 'mvp' not found in shader!" << std::endl;
      return -1;
    }

    if (mainShader.getUniformLocation("texShadow") == -1) {
      std::cerr << "Uniform 'texShadow' not found in shader!" << std::endl;
      return -1;
    }

    const glm::mat4 mvp = mainProjectionMatrix * activeCam->viewMatrix();
    glUniformMatrix4fv(mainShader.getUniformLocation("mvp"), 1, GL_FALSE,
                       glm::value_ptr(mvp));

    const glm::vec3 cameraPos = activeCam->cameraPos();
    glUniform3fv(mainShader.getUniformLocation("viewPos"), 1,
                 glm::value_ptr(cameraPos));

    const glm::mat4 lightMvp = mainProjectionMatrix * inactiveCam->viewMatrix();
    glUniformMatrix4fv(mainShader.getUniformLocation("lightMVP"), 1, GL_FALSE,
                       glm::value_ptr(lightMvp));

    const glm::vec3 lightPos = inactiveCam->cameraPos();
    glUniform3fv(mainShader.getUniformLocation("lightPos"), 1,
                 glm::value_ptr(lightPos));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texShadow);
    glUniform1i(mainShader.getUniformLocation("texShadow"),
                0); // 0 for GL_TEXTURE0

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, texLight);
    glUniform1i(mainShader.getUniformLocation("texLight"),
                2); // 2 for GL_TEXTURE2

    glViewport(0, 0, WIDTH, HEIGHT);
    glClearDepth(1.0);
    glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES,
                   static_cast<GLsizei>(mesh.triangles.size() * 3),
                   GL_UNSIGNED_INT, nullptr);

    window.swapBuffers();
  }

  // Be a nice citizen and clean up after yourself.
  glDeleteFramebuffers(1, &framebuffer);
  glDeleteTextures(1, &texShadow);
  glDeleteTextures(1, &texLight);
  glDeleteBuffers(1, &ibo);
  glDeleteBuffers(1, &vbo);
  glDeleteVertexArrays(1, &vao);

  return 0;
}
