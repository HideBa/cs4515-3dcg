#include "menu.h"
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/gtc/type_ptr.hpp>
#include <imgui/imgui.h>
#include <nativefiledialog/nfd.h>
DISABLE_WARNINGS_POP()
#include <filesystem>
#include <iostream>

Menu::Menu(Config &config)
    : m_config(config), m_newParticleCount(config.numParticles) {}

void Menu::draw() {
  ImGui::Begin("Debug Controls");

  ImGui::Text("Particle Simulation");
  ImGui::Separator();
  drawParticleSimControls();
  ImGui::Spacing();
  ImGui::Text("Sphere Container");
  ImGui::Separator();
  drawSphereContainerControls();
  ImGui::Spacing();
  ImGui::Text("Particle Coloring");
  ImGui::Separator();
  drawParticleColorControls();
  ImGui::Spacing();
  ImGui::Text("Bounces");
  ImGui::Separator();
  drawBouncesControls();

  ImGui::End();
}

void Menu::drawParticleSimControls() {
  // Parameters
  m_newParticleCount =
      std::max(1, m_newParticleCount); // Ensure that the new number of
                                       // particles is always positive
  ImGui::InputInt("New particle count", &m_newParticleCount);
  ImGui::SliderFloat("Timestep", &m_config.particleSimTimestep, 0.001f, 0.05f,
                     "%.3f");
  ImGui::SliderFloat("Particle radius", &m_config.particleRadius, 0.05f, 1.0f);
  ImGui::Checkbox("Inter-particle collisions",
                  &m_config.particleInterCollision);

  // Flags
  std::string simPlaybackText = m_config.doContinuousSimulation
                                    ? "Pause simulation"
                                    : "Resume simulation";
  if (ImGui::Button(simPlaybackText.c_str())) {
    m_config.doContinuousSimulation = !m_config.doContinuousSimulation;
  }
  ImGui::SameLine();
  m_config.doSingleStep = ImGui::Button("Single simulation step");
  ImGui::SameLine();
  if (ImGui::Button("Reset simulation")) {
    m_config.numParticles = m_newParticleCount;
    m_config.doResetSimulation = true;
  }
}

void Menu::drawSphereContainerControls() {
  constexpr float CENTER_MAX = 10.0f;
  constexpr float RADIUS_MAX = 10.0f;
  constexpr float WIREFRAME_THICKNESS_MAX = 10.0f;

  ImGui::DragFloat3("Center", glm::value_ptr(m_config.sphereCenter), 0.01f,
                    -CENTER_MAX, CENTER_MAX, "%.2f");
  ImGui::DragFloat("Radius", &m_config.sphereRadius, 0.01f, 0.0f, RADIUS_MAX,
                   "%.2f");
  ImGui::ColorEdit3("Color", glm::value_ptr(m_config.sphereColor));
}

void Menu::drawParticleColorControls() {
  ImGui::Checkbox("Shading", &m_config.shading);
  ImGui::Checkbox("Do speed based color", &m_config.doSpeedBasedColor);
  if (m_config.doSpeedBasedColor) {
    ImGui::InputFloat("Max speed", &m_config.maxSpeed, 0.1f, 0.1f, "%.2f");
    ImGui::ColorEdit3("Min color", glm::value_ptr(m_config.particleColorMin));
    ImGui::ColorEdit3("Max color", glm::value_ptr(m_config.particleColorMax));
  }
  ImGui::SliderFloat("Ambient coefficient", &m_config.ambientCoefficient, 0.0f,
                     1.0f, "%.2f");
}

void Menu::drawBouncesControls() {
  ImGui::Checkbox("Use bounce color", &m_config.useBounceColor);
  ImGui::ColorEdit3("Bounce color", glm::value_ptr(m_config.bounceColor));
  ImGui::InputInt("Bounce threshold",
                  reinterpret_cast<int *>(&m_config.bounceThreashold));
  ImGui::InputInt("Bounce frames",
                  reinterpret_cast<int *>(&m_config.bounceFrames));
}