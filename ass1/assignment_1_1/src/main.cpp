// Disable compiler warnings in third-party code (which we cannot change).
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
#include <stb/stb_image.h>
DISABLE_WARNINGS_POP()
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdlib> // EXIT_FAILURE
#include <framework/mesh.h>
#include <framework/shader.h>
#include <framework/trackball.h>
#include <framework/window.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl2.h>
#include <iostream>
#include <numeric>
#include <optional>
#include <span>
#include <toml/toml.hpp>
#include <vector>

// Configuration
const int WIDTH = 1200;
const int HEIGHT = 800;

bool show_imgui = true;

struct {
  // Diffuse (Lambert)
  glm::vec3 kd{0.5f};
  // Specular (Phong/Blinn Phong)
  glm::vec3 ks{0.5f};
  float shininess = 3.0f;
  // Toon
  int toonDiscretize = 4;
  float toonSpecularThreshold = 0.49f;
} shadingData;

struct {
  int diffuseModel = 0;  // "debug", "lambert", "toon", "x-toon"
  int specularModel = 0; // "none", "phong", "blinn-phong", "toon"
  bool shadows = true;
  bool pcf = true;
} renderSettings;

const char *diffuseModels[] = {"debug", "lambert", "toon", "x-toon"};
const char *specularModels[] = {"none", "phong", "blinn-phong", "toon"};

struct Texture {
  int width;
  int height;
  int channels;
  stbi_uc *texture_data;
};

// Lights
struct Light {
  glm::vec3 position;
  glm::vec3 color;
  bool is_spotlight;
  glm::vec3 direction;
  bool has_texture;
  Texture texture;
};

std::vector<Light> lights{};
size_t selectedLightIndex = 0;

void imgui() {

  // Define UI here
  if (!show_imgui)
    return;

  ImGui::Begin("Final project part 1 : Modern Shading");
  ImGui::Text("Press \\ to show/hide this menu");

  ImGui::Separator();
  ImGui::Text("Material parameters");
  ImGui::SliderFloat("Shininess", &shadingData.shininess, 0.0f, 100.f);

  // Color pickers for Kd and Ks
  ImGui::ColorEdit3("Kd", &shadingData.kd[0]);
  ImGui::ColorEdit3("Ks", &shadingData.ks[0]);

  ImGui::SliderInt("Toon Discretization", &shadingData.toonDiscretize, 1, 10);
  ImGui::SliderFloat("Toon Specular Threshold",
                     &shadingData.toonSpecularThreshold, 0.0f, 1.0f);

  ImGui::Separator();
  ImGui::Text("Render Settings");

  ImGui::Combo("Diffuse Model", &renderSettings.diffuseModel, diffuseModels,
               IM_ARRAYSIZE(diffuseModels));
  ImGui::Text("Current Diffuse Model: %s",
              diffuseModels[renderSettings.diffuseModel]);
  ImGui::Combo("Specular Model", &renderSettings.specularModel, specularModels,
               IM_ARRAYSIZE(specularModels));
  ImGui::Checkbox("Shadows", &renderSettings.shadows);
  ImGui::Checkbox("PCF", &renderSettings.pcf);

  ImGui::Separator();
  ImGui::Text("Lights");

  //   // Display lights in scene
  std::vector<std::string> itemStrings = {};
  for (size_t i = 0; i < lights.size(); i++) {
    auto string = "Light " + std::to_string(i);
    itemStrings.push_back(string);
  }

  std::vector<const char *> itemCStrings = {};
  for (const auto &string : itemStrings) {
    itemCStrings.push_back(string.c_str());
  }

  int tempSelectedItem = static_cast<int>(selectedLightIndex);
  if (ImGui::ListBox("Lights", &tempSelectedItem, itemCStrings.data(),
                     (int)itemCStrings.size(), 4)) {
    selectedLightIndex = static_cast<size_t>(tempSelectedItem);
  }

  if (selectedLightIndex < lights.size()) {
    Light &selectedLight = lights[selectedLightIndex];

    ImGui::ColorEdit3("Light Color", &selectedLight.color[0]);
    ImGui::DragFloat3("Light Position", &selectedLight.position[0], 0.1f);
    ImGui::Checkbox("Is Spotlight", &selectedLight.is_spotlight);
    if (selectedLight.is_spotlight) {
      ImGui::DragFloat3("Spotlight Direction", &selectedLight.direction[0],
                        0.1f);
    }
    ImGui::Checkbox("Has Texture", &selectedLight.has_texture);
    if (selectedLight.has_texture) {
      ImGui::InputInt("Texture Width", &selectedLight.texture.width);
      ImGui::InputInt("Texture Height", &selectedLight.texture.height);
      ImGui::InputInt("Texture Channels", &selectedLight.texture.channels);
    }
  }

  ImGui::End();
  ImGui::Render();
}

std::optional<glm::vec3> tomlArrayToVec3(const toml::array *array) {
  glm::vec3 output{};

  if (array) {
    int i = 0;
    array->for_each([&](auto &&elem) {
      if (elem.is_number()) {
        if (i > 2)
          return;
        output[i] = static_cast<float>(elem.as_floating_point()->get());
        i += 1;
      } else {
        std::cerr << "Error: Expected a number in array, got " << elem.type()
                  << std::endl;
        return;
      }
    });
  }

  return output;
}

// Program entry point. Everything starts here.
int main(int argc, char **argv) {

  // read toml file from argument line (otherwise use default file)
  std::string config_filename =
      argc == 2 ? std::string(argv[1]) : "resources/default_scene.toml";

  // parse initial scene config
  toml::table config;
  try {
    config = toml::parse_file(std::string(RESOURCE_ROOT) + config_filename);
  } catch (const toml::parse_error &) {
    std::cerr << "parsing failed" << std::endl;
  }

  // read material data
  shadingData.kd = tomlArrayToVec3(config["material"]["kd"].as_array()).value();
  shadingData.ks = tomlArrayToVec3(config["material"]["ks"].as_array()).value();
  shadingData.shininess = config["material"]["shininess"].value_or(0.0f);
  shadingData.toonDiscretize =
      (int)config["material"]["toonDiscretize"].value_or(0);
  shadingData.toonSpecularThreshold =
      config["material"]["toonSpecularThreshold"].value_or(0.0f);

  // read lights
  lights = std::vector<Light>{};
  if (!config.contains("lights")) {
    std::cerr << "Lights not provided, using default light" << std::endl;
    lights = std::vector<Light>{
        Light{glm::vec3(0, 0, 3), glm::vec3(1)},
    };
  }
  if (!config["lights"]["positions"].is_array()) {
    std::cerr << "Light positions must be array of vectors" << std::endl;
    exit(EXIT_FAILURE);
  }

  if (!config["lights"]["colors"].is_array()) {
    std::cerr << "Light colors must be array of vectors" << std::endl;
    exit(EXIT_FAILURE);
  }

  size_t num_lights = config["lights"]["positions"].as_array()->size();
  std::cout << num_lights << std::endl;

  for (size_t i = 0; i < num_lights; ++i) {
    auto pos =
        tomlArrayToVec3(config["lights"]["positions"][i].as_array()).value();
    auto color =
        tomlArrayToVec3(config["lights"]["colors"][i].as_array()).value();
    bool is_spotlight =
        config["lights"]["is_spotlight"][i].value<bool>().value();
    auto direction =
        tomlArrayToVec3(config["lights"]["direction"][i].as_array()).value();
    bool has_texture = config["lights"]["has_texture"][i].value<bool>().value();

    auto tex_path = std::string(RESOURCE_ROOT) +
                    config["mesh"]["path"].value_or("resources/dragon.obj");
    int width = 0, height = 0,
        sourceNumChannels =
            0; // Number of channels in source image. pixels will always be the
               // requested number of channels (3).
    stbi_uc *pixels = nullptr;
    if (has_texture) {
      pixels = stbi_load(tex_path.c_str(), &width, &height, &sourceNumChannels,
                         STBI_rgb);
    }

    lights.emplace_back(Light{pos,
                              color,
                              is_spotlight,
                              direction,
                              has_texture,
                              {width, height, sourceNumChannels, pixels}});
  }

  // Create window
  Window window{"Shading", glm::ivec2(WIDTH, HEIGHT), OpenGLVersion::GL41};

  // read camera settings
  auto look_at = tomlArrayToVec3(config["camera"]["lookAt"].as_array()).value();
  auto rotations =
      tomlArrayToVec3(config["camera"]["rotations"].as_array()).value();
  float fovY = config["camera"]["fovy"].value_or(50.0f);
  float dist = config["camera"]["dist"].value_or(1.0f);

  auto diffuse_model =
      config["render_settings"]["diffuse_model"].value<std::string>();
  renderSettings.diffuseModel = diffuse_model == "debug"     ? 0
                                : diffuse_model == "lambert" ? 1
                                : diffuse_model == "toon"    ? 2
                                                             : 3;
  auto specular_model =
      config["render_settings"]["specular_model"].value<std::string>();
  renderSettings.specularModel = specular_model == "none"          ? 0
                                 : specular_model == "phong"       ? 1
                                 : specular_model == "blinn-phong" ? 2
                                                                   : 3;
  bool do_pcf = config["render_settings"]["pcf"].value<bool>().value();
  bool do_shadows = config["render_settings"]["shadows"].value<bool>().value();

  Trackball trackball{&window, glm::radians(fovY)};
  trackball.setCamera(look_at, rotations, dist);

  // read mesh
  bool animated = config["mesh"]["animated"].value_or(false);
  auto mesh_path = std::string(RESOURCE_ROOT) +
                   config["mesh"]["path"].value_or("resources/dragon.obj");

  std::cout << mesh_path << std::endl;

  const Mesh mesh = loadMesh(mesh_path)[0];

  window.registerKeyCallback(
      [&](int key, int /* scancode */, int action, int /* mods */) {
        if (key == '\\' && action == GLFW_PRESS) {
          show_imgui = !show_imgui;
        }

        if (action != GLFW_RELEASE)
          return;
      });

  const Shader debugShader =
      ShaderBuilder()
          .addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl")
          .addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/debug_frag.glsl")
          .build();
  const Shader diffuseShader =
      ShaderBuilder()
          .addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl")
          .addStage(GL_FRAGMENT_SHADER,
                    RESOURCE_ROOT "shaders/diffuse_frag.glsl")
          .build();
  const Shader specularShader =
      ShaderBuilder()
          .addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl")
          .addStage(GL_FRAGMENT_SHADER,
                    RESOURCE_ROOT "shaders/specular_frag.glsl")
          .build();
  // Create Vertex Buffer Object and Index Buffer Objects.
  GLuint vbo;

  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER,
               static_cast<GLsizeiptr>(mesh.vertices.size() * sizeof(Vertex)),
               mesh.vertices.data(), GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  GLuint ibo;
  // Create index buffer object (IBO)
  glGenBuffers(1, &ibo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
  glBufferData(
      GL_ELEMENT_ARRAY_BUFFER,
      static_cast<GLsizeiptr>(mesh.triangles.size() *
                              sizeof(decltype(Mesh::triangles)::value_type)),
      mesh.triangles.data(), GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  // Bind vertex data to shader inputs using their index (location).
  // These bindings are stored in the Vertex Array Object.
  GLuint vao;
  // Create VAO and bind it so subsequent creations of VBO and IBO are bound to
  // this VAO
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

  // The position and normal vectors should be retrieved from the specified
  // Vertex Buffer Object. The stride is the distance in bytes between vertices.
  // We use the offset to point to the normals instead of the positions. Tell
  // OpenGL that we will be using vertex attributes 0 and 1.
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);

  // This is where we would set the attribute pointers, if apple supported it.

  glBindVertexArray(0);

  // Enable depth testing.
  glEnable(GL_DEPTH_TEST);

  // Main loop.
  while (!window.shouldClose()) {
    window.updateInput();

    imgui();

    // Clear the framebuffer to black and depth to maximum value (ranges from
    // [-1.0 to +1.0]).
    glViewport(0, 0, window.getWindowSize().x, window.getWindowSize().y);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set model/view/projection matrix.
    const glm::vec3 cameraPos = trackball.position();
    const glm::mat4 model{1.0f};

    const glm::mat4 view = trackball.viewMatrix();
    const glm::mat4 projection = trackball.projectionMatrix();
    const glm::mat4 mvp = projection * view * model;

    auto render = [&](const Shader &shader) {
      // Set the model/view/projection matrix that is used to transform the
      // vertices in the vertex shader.
      glUniformMatrix4fv(shader.getUniformLocation("mvp"), 1, GL_FALSE,
                         glm::value_ptr(mvp));

      // Bind vertex data.
      glBindVertexArray(vao);

      // We tell OpenGL what each vertex looks like and how they are mapped to
      // the shader using the names NOTE: Usually this can be stored in the VAO,
      // since the locations would be the same in all shaders by using the
      // layout(location = ...) qualifier in the shaders, however this does not
      // work on apple devices.
      glVertexAttribPointer(shader.getAttributeLocation("pos"), 3, GL_FLOAT,
                            GL_FALSE, sizeof(Vertex),
                            (void *)offsetof(Vertex, position));
      glVertexAttribPointer(shader.getAttributeLocation("normal"), 3, GL_FLOAT,
                            GL_FALSE, sizeof(Vertex),
                            (void *)offsetof(Vertex, normal));

      // Execute draw command.
      glDrawElements(GL_TRIANGLES,
                     static_cast<GLsizei>(mesh.triangles.size()) * 3,
                     GL_UNSIGNED_INT, nullptr);

      glBindVertexArray(0);
    };
    if (renderSettings.diffuseModel == 0) {
      debugShader.bind();
      render(debugShader);
    } else if (renderSettings.diffuseModel == 1 ||
               renderSettings.diffuseModel == 2 ||
               renderSettings.diffuseModel == 3) {
      diffuseShader.bind();
      glUniform3fv(diffuseShader.getUniformLocation("lightPos"), 1,
                   glm::value_ptr(lights[selectedLightIndex].position));
      glUniform3fv(diffuseShader.getUniformLocation("viewPos"), 1,
                   glm::value_ptr(cameraPos));
      glUniform3fv(diffuseShader.getUniformLocation("lightColor"), 1,
                   glm::value_ptr(lights[selectedLightIndex].color));
      glUniform3fv(diffuseShader.getUniformLocation("ks"), 1,
                   glm::value_ptr(shadingData.ks));
      glUniform3fv(diffuseShader.getUniformLocation("kd"), 1,
                   glm::value_ptr(shadingData.kd));
      // glUniform1f(diffuseShader.getUniformLocation("shininess"),
      //             shadingData.shininess);
      glUniform1f(diffuseShader.getUniformLocation("toonSpecularThreshold"),
                  shadingData.toonSpecularThreshold);
      glUniform1i(diffuseShader.getUniformLocation("toonDiscretize"),
                  shadingData.toonDiscretize);
      glUniform1i(diffuseShader.getUniformLocation("mode"),
                  renderSettings.diffuseModel);
      render(diffuseShader);
    }
    if (renderSettings.specularModel != 0) {
      specularShader.bind();
      std::cout << "Specular model: " << renderSettings.specularModel
                << std::endl;
      glUniform3fv(specularShader.getUniformLocation("lightPos"), 1,
                   glm::value_ptr(lights[selectedLightIndex].position));
      glUniform3fv(specularShader.getUniformLocation("viewPos"), 1,
                   glm::value_ptr(cameraPos));
      glUniform3fv(specularShader.getUniformLocation("lightColor"), 1,
                   glm::value_ptr(lights[selectedLightIndex].color));
      glUniform3fv(specularShader.getUniformLocation("ks"), 1,
                   glm::value_ptr(shadingData.ks));
      glUniform3fv(specularShader.getUniformLocation("kd"), 1,
                   glm::value_ptr(shadingData.kd));
      glUniform1f(specularShader.getUniformLocation("shininess"),
                  shadingData.shininess);
      // glUniform1f(specularShader.getUniformLocation("toonSpecularThreshold"),
      //             shadingData.toonSpecularThreshold);
      glUniform1i(specularShader.getUniformLocation("toonDiscretize"),
                  shadingData.toonDiscretize);
      glUniform1i(specularShader.getUniformLocation("mode"),
                  renderSettings.specularModel);
      render(specularShader);
    }
    debugShader.bind();
    render(debugShader);

    // Present result to the screen.
    window.swapBuffers();
  }

  // Be a nice citizen and clean up after yourself.
  glDeleteBuffers(1, &vbo);
  glDeleteBuffers(1, &ibo);
  glDeleteVertexArrays(1, &vao);

  return 0;
}
