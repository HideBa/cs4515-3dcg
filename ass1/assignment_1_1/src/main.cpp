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
  int diffuseMode = 0;  // "debug", "lambert", "toon", "x-toon"
  int specularMode = 0; // "none", "phong", "blinn-phong", "toon"
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

enum class LightPlacementValue { Sphere = 0, Shadow = 1, Specular = 2 };
LightPlacementValue interfaceLightPlacement{LightPlacementValue::Sphere};

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

static glm::vec3 userInteractionSphere(const glm::vec3 &selectedPos,
                                       const glm::vec3 &camPos) {
  // RETURN the new light position, defined as follows.
  // selectedPos is a location on the mesh. Use this location to place the light
  // source to cover the location as seen from camPos. Further, the light should
  // be at a distance of 1.5 from the origin of the scene - in other words,
  // located on a sphere of radius 1.5 around the origin.
  float raidus = 1.5f;
  glm::vec3 direction = glm::normalize(selectedPos - camPos);
  return direction * raidus;
}

static glm::vec3 userInteractionShadow(const glm::vec3 &selectedPos,
                                       const glm::vec3 &selectedNormal,
                                       const glm::vec3 &lightPos) {
  float distance = glm::dot(lightPos - selectedPos, selectedNormal);
  glm::vec3 newLightPos = lightPos - distance * selectedNormal;
  return newLightPos;
}

static glm::vec3 userInteractionSpecular(const glm::vec3 &selectedPos,
                                         const glm::vec3 &selectedNormal,
                                         const glm::vec3 &lightPos,
                                         const glm::vec3 &cameraPos) {
  // RETURN the new light position such that a specularity (highlight) will be
  // located at selectedPos, when viewed from cameraPos and lit from lightPos.
  // please ensure also that the light is at a distance of 1 from selectedPos!
  // If the camera is on the wrong side of the surface (normal pointing the
  // other way), then just return the original light position. There is only ONE
  // way of doing this!
  glm::vec3 viewDir = glm::normalize(cameraPos - selectedPos);
  glm::vec3 reflectionDir = glm::reflect(-viewDir, selectedNormal);
  return selectedPos + reflectionDir;
}

static size_t getClosestVertexIndex(const Mesh &mesh, const glm::vec3 &pos);
static std::optional<glm::vec3> getWorldPositionOfPixel(const Trackball &,
                                                        const glm::vec2 &pixel);
static void userInteraction(const glm::vec3 &cameraPos,
                            const glm::vec3 &selectedPos,
                            const glm::vec3 &selectedNormal);
static void printHelp();

void resetLights() {
  lights.clear();
  lights.push_back(Light{glm::vec3(0, 0, 3), glm::vec3(1)});
  selectedLightIndex = 0;
}

void selectNextLight() {
  selectedLightIndex = (selectedLightIndex + 1) % lights.size();
}

void selectPreviousLight() {
  if (selectedLightIndex == 0)
    selectedLightIndex = lights.size() - 1;
  else
    --selectedLightIndex;
}

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

  ImGui::Combo("Diffuse Model", &renderSettings.diffuseMode, diffuseModels,
               IM_ARRAYSIZE(diffuseModels));
  ImGui::Combo("Specular Model", &renderSettings.specularMode, specularModels,
               IM_ARRAYSIZE(specularModels));
  ImGui::Checkbox("Shadows", &renderSettings.shadows);
  ImGui::Checkbox("PCF", &renderSettings.pcf);

  ImGui::Separator();
  ImGui::Text("Lights");
  if (ImGui::Button("Add Light")) {
    std::cout << "Adding light" << std::endl;
    lights.push_back(Light{glm::vec3(0, 0, 3), glm::vec3(1)});
    selectedLightIndex = lights.size() - 1;
  }

  // Display lights in scene
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

  std::array interactionModeNames{"Sphere", "Shadow", "Specular"};
  int current_mode = static_cast<int>(interfaceLightPlacement);
  ImGui::Combo("User Interaction Mode", &current_mode,
               interactionModeNames.data(), interactionModeNames.size());
  interfaceLightPlacement = static_cast<LightPlacementValue>(current_mode);

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
  std::cout << "arg count: " << argc << std::endl;
  if (argc > 1) {
    std::cout << "arg 1: " << argv[1] << std::endl;
  } else {
    std::cout << "arg 1: not provided" << std::endl;
  }
  // read toml file from argument line (otherwise use default file)
  std::string config_filename =
      argc > 1 ? std::string(argv[1]) : "resources/dragon_plane.toml";
  std::cout << "config file: " << config_filename << std::endl;
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
  renderSettings.diffuseMode = diffuse_model == "debug"     ? 0
                               : diffuse_model == "lambert" ? 1
                               : diffuse_model == "toon"    ? 2
                                                            : 3;
  auto specular_model =
      config["render_settings"]["specular_model"].value<std::string>();
  renderSettings.specularMode = specular_model == "none"          ? 0
                                : specular_model == "phong"       ? 1
                                : specular_model == "blinn-phong" ? 2
                                                                  : 3;
  renderSettings.pcf = config["render_settings"]["pcf"].value<bool>().value();
  renderSettings.shadows =
      config["render_settings"]["shadows"].value<bool>().value();

  Trackball trackball{&window, glm::radians(fovY)};
  trackball.setCamera(look_at, rotations, dist);

  // read mesh
  bool animated = config["mesh"]["animated"].value_or(false);
  auto mesh_path = std::string(RESOURCE_ROOT) +
                   config["mesh"]["path"].value_or("resources/dragon.obj");

  std::cout << mesh_path << std::endl;

  const Mesh mesh = loadMesh(mesh_path)[0];

  window.registerKeyCallback([&](int key, int /* scancode */, int action,
                                 int /* mods */) {
    if (key == '\\' && action == GLFW_PRESS) {
      show_imgui = !show_imgui;
    }

    if (action != GLFW_RELEASE)
      return;

    const bool shiftPressed = window.isKeyPressed(GLFW_KEY_LEFT_SHIFT) ||
                              window.isKeyPressed(GLFW_KEY_RIGHT_SHIFT);
    std::cout << "Key " << key << " pressed" << std::endl;
    std::cout << "Shift pressed: " << shiftPressed << std::endl;
    switch (key) {
    case GLFW_KEY_M: {
      interfaceLightPlacement = static_cast<LightPlacementValue>(
          (static_cast<int>(interfaceLightPlacement) + 1) % 3);
      break;
    }
    case GLFW_KEY_L: {
      std::cout << "Interaction: LightPlacementValue::Sphere" << std::endl;
      if (shiftPressed) {
        std::cout << "Interaction: LightPlacementValue::Sphere" << std::endl;
        lights.push_back(Light{trackball.position(), glm::vec3(1)});
      } else
        lights[selectedLightIndex].position = trackball.position();
      return;
    }
    case GLFW_KEY_MINUS: {
      selectPreviousLight();
      return;
    }
    case GLFW_KEY_EQUAL: {
      if (shiftPressed) // '+' pressed (unless you use a weird keyboard layout).
        selectNextLight();
      return;
    }
    case GLFW_KEY_N: {
      resetLights();
      return;
    }
    case GLFW_KEY_SPACE: {
      const auto optWorldPoint =
          getWorldPositionOfPixel(trackball, window.getCursorPixel());
      if (optWorldPoint) {
        // std::cout << "World point: (" << optWorldPoint->x << ", " <<
        // optWorldPoint->y << ", " << optWorldPoint->z << ")" << std::endl;
        // lights[selectedLightIndex].position = worldPoint;
        const size_t selectedVertexIdx =
            getClosestVertexIndex(mesh, *optWorldPoint);
        if (selectedVertexIdx != 0xFFFFFFFF) {
          const Vertex &selectedVertex = mesh.vertices[selectedVertexIdx];
          userInteraction(trackball.position(), selectedVertex.position,
                          selectedVertex.normal);
        }
      }
      return;
    }
    default:
      return;
    };
    switch (interfaceLightPlacement) {
    case LightPlacementValue::Sphere: {
      std::cout << "Interaction: LightPlacementValue::Sphere" << std::endl;
      break;
    }
    case LightPlacementValue::Shadow: {
      std::cout << "Interaction: LightPlacementValue::Shadow" << std::endl;
      break;
    }
    case LightPlacementValue::Specular: {
      std::cout << "Interaction: LightPlacementValue::Specular" << std::endl;
      break;
    }
    };
  });

  const Shader debugShader =
      ShaderBuilder()
          .addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl")
          .addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/debug_frag.glsl")
          .build();
  const Shader lightShader =
      ShaderBuilder()
          .addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/light_vertex.glsl")
          .addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/light_frag.glsl")
          .build();
  const Shader shader =
      ShaderBuilder()
          .addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl")
          .addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT
                    "shaders/frag.glsl") // Use the combined shader filename
          .build();
  const Shader depthShader =
      ShaderBuilder()
          .addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/depth_vert.glsl")
          .addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/depth_frag.glsl")
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
  // Create VAO and bind it so subsequent creations of VBO and IBO are bound
  // to this VAO
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

  // The position and normal vectors should be retrieved from the specified
  // Vertex Buffer Object. The stride is the distance in bytes between
  // vertices. We use the offset to point to the normals instead of the
  // positions. Tell OpenGL that we will be using vertex attributes 0 and 1.
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);

  // This is where we would set the attribute pointers, if apple supported it.

  glBindVertexArray(0);

  // Enable depth testing.
  glEnable(GL_DEPTH_TEST);

  // Shadow mapping setup
  const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;

  GLuint depthMapFBO;
  glGenFramebuffers(1, &depthMapFBO);

  // Create depth texture
  GLuint depthMap;
  glGenTextures(1, &depthMap);
  glBindTexture(GL_TEXTURE_2D, depthMap);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH,
               SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  // Prevent texture wrapping artifacts
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  float borderColor[] = {1.0, 1.0, 1.0, 1.0};
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

  // Attach depth texture as FBO's depth buffer
  glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                         depthMap, 0);
  // No color output in the bound framebuffer, only depth.
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

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

    // 1. Render depth of scene to texture (from light's perspective)
    glm::mat4 lightProjection, lightView;
    float near_plane = 1.0f, far_plane = 25.0f;
    lightProjection =
        glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
    lightView =
        glm::lookAt(lights[selectedLightIndex].position,
                    glm::vec3(0.0f), // Target the origin or your scene's center
                    glm::vec3(0.0, 1.0, 0.0));
    glm::mat4 lightMVP = lightProjection * lightView;

    // Render scene from light's point of view
    depthShader.bind();
    glUniformMatrix4fv(depthShader.getUniformLocation("lightMVP"), 1, GL_FALSE,
                       glm::value_ptr(lightMVP));

    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    glBindVertexArray(vao);
    // Only position attribute is needed
    glVertexAttribPointer(depthShader.getAttributeLocation("pos"), 3, GL_FLOAT,
                          GL_FALSE, sizeof(Vertex),
                          (void *)offsetof(Vertex, position));

    // Draw the scene
    glDrawElements(GL_TRIANGLES,
                   static_cast<GLsizei>(mesh.triangles.size()) * 3,
                   GL_UNSIGNED_INT, nullptr);

    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 2. Render scene as normal
    glViewport(0, 0, window.getWindowSize().x, window.getWindowSize().y);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shader.bind();

    // Set the model/view/projection matrix
    glUniformMatrix4fv(shader.getUniformLocation("mvp"), 1, GL_FALSE,
                       glm::value_ptr(mvp));

    // Pass lightMVP matrix to shader
    glUniformMatrix4fv(shader.getUniformLocation("lightMVP"), 1, GL_FALSE,
                       glm::value_ptr(lightMVP));

    // Set lighting uniforms
    glUniform3fv(shader.getUniformLocation("lightPos"), 1,
                 glm::value_ptr(lights[selectedLightIndex].position));
    glUniform3fv(shader.getUniformLocation("viewPos"), 1,
                 glm::value_ptr(cameraPos));
    glUniform3fv(shader.getUniformLocation("lightColor"), 1,
                 glm::value_ptr(lights[selectedLightIndex].color));
    glUniform3fv(shader.getUniformLocation("ks"), 1,
                 glm::value_ptr(shadingData.ks));
    glUniform3fv(shader.getUniformLocation("kd"), 1,
                 glm::value_ptr(shadingData.kd));
    glUniform1f(shader.getUniformLocation("shininess"), shadingData.shininess);
    glUniform1f(shader.getUniformLocation("toonSpecularThreshold"),
                shadingData.toonSpecularThreshold);
    glUniform1i(shader.getUniformLocation("toonDiscretize"),
                shadingData.toonDiscretize);

    // Set mode uniforms
    glUniform1i(shader.getUniformLocation("diffuseMode"),
                renderSettings.diffuseMode);
    glUniform1i(shader.getUniformLocation("specularMode"),
                renderSettings.specularMode);

    // Set shadow and PCF uniforms
    glUniform1i(shader.getUniformLocation("shadows"),
                renderSettings.shadows ? 1 : 0);
    glUniform1i(shader.getUniformLocation("pcf"), renderSettings.pcf ? 1 : 0);
    glUniform1i(shader.getUniformLocation("lightMode"),
                lights[selectedLightIndex].is_spotlight ? 1 : 0);
    // Bind the shadow map
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glUniform1i(shader.getUniformLocation("shadowMap"), 0);

    // Bind vertex data.
    glBindVertexArray(vao);

    // Set up vertex attributes
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

    lightShader.bind();
    {
      const glm::vec4 screenPos =
          mvp * glm::vec4(lights[selectedLightIndex].position, 1.0f);
      const glm::vec3 color{1, 1, 0};

      glPointSize(40.0f);
      glUniform4fv(lightShader.getUniformLocation("pos"), 1,
                   glm::value_ptr(screenPos));
      glUniform3fv(lightShader.getUniformLocation("color"), 1,
                   glm::value_ptr(color));
      glBindVertexArray(vao);
      glDrawArrays(GL_POINTS, 0, 1);
      glBindVertexArray(0);
    }

    // Present result to the screen.
    window.swapBuffers();
  }

  // Be a nice citizen and clean up after yourself.
  glDeleteBuffers(1, &vbo);
  glDeleteBuffers(1, &ibo);
  glDeleteVertexArrays(1, &vao);
  glDeleteFramebuffers(1, &depthMapFBO);
  glDeleteTextures(1, &depthMap);

  return 0;
}

static void userInteraction(const glm::vec3 &cameraPos,
                            const glm::vec3 &selectedPos,
                            const glm::vec3 &selectedNormal) {
  std::cout << "User Interaction" << std::endl;
  switch (interfaceLightPlacement) {
  case LightPlacementValue::Sphere: {
    std::cout << "Sphere add----" << std::endl;
    lights[selectedLightIndex].position =
        userInteractionSphere(selectedPos, cameraPos);
    break;
  }
  case LightPlacementValue::Shadow: {
    std::cout << "shadow add----" << std::endl;
    lights[selectedLightIndex].position = userInteractionShadow(
        selectedPos, selectedNormal, lights[selectedLightIndex].position);
    break;
  }
  case LightPlacementValue::Specular: {
    lights[selectedLightIndex].position =
        userInteractionSpecular(selectedPos, selectedNormal,
                                lights[selectedLightIndex].position, cameraPos);
    break;
  }
  }
}

static size_t getClosestVertexIndex(const Mesh &mesh, const glm::vec3 &pos) {
  const auto iter =
      std::min_element(std::begin(mesh.vertices), std::end(mesh.vertices),
                       [&](const Vertex &lhs, const Vertex &rhs) {
                         return glm::length(lhs.position - pos) <
                                glm::length(rhs.position - pos);
                       });
  return (size_t)std::distance(std::begin(mesh.vertices), iter);
}

static std::optional<glm::vec3>
getWorldPositionOfPixel(const Trackball &trackball, const glm::vec2 &pixel) {
  float depth;
  glReadPixels(static_cast<int>(pixel.x), static_cast<int>(pixel.y), 1, 1,
               GL_DEPTH_COMPONENT, GL_FLOAT, &depth);

  if (depth == 1.0f) {
    // This is a work around for a bug in GCC:
    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80635
    //
    // This bug will emit a warning about a maybe uninitialized value when
    // writing: return {};
    constexpr std::optional<glm::vec3> tmp;
    return tmp;
  }

  // Coordinates convert from pixel space to OpenGL screen space (range from -1
  // to +1)
  const glm::vec3 win{pixel, depth};

  // View matrix
  const glm::mat4 view = trackball.viewMatrix();
  const glm::mat4 projection = trackball.projectionMatrix();

  const glm::vec4 viewport{0, 0, WIDTH, HEIGHT};
  return glm::unProject(win, view, projection, viewport);
}

static void printHelp() {
  Trackball::printHelp();
  std::cout << std::endl;
  std::cout << "Program Usage:" << std::endl;
  std::cout << "______________________" << std::endl;
  std::cout << "m - Change Interaction Mode - to influence light source"
            << std::endl;
  std::cout << "l - place the light source at the current camera position"
            << std::endl;
  std::cout << "L - add an additional light source" << std::endl;
  std::cout << "+ - choose next light source" << std::endl;
  std::cout << "- - choose previous light source" << std::endl;
  std::cout << "N - clear all light sources and reinitialize with one"
            << std::endl;
  // std::cout << "s - show selected vertices" << std::endl;
  std::cout << "SPACE - call your light placement function with the current "
               "mouse position"
            << std::endl;
}
