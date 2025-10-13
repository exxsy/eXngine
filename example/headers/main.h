#pragma once
#include <iostream>
#include <Windows.h>
#include <stdlib.h>
#include <cassert>
#include <algorithm>
#include <chrono>

#include <eXngine.h>
#include <glm/glm.hpp>
#include <windows/glfw.h>
#include <utils/fbx-loader.h>
#include <renderers/vulkan/renderer.h>

#include <3rdparty/stb_image/stb_image.h>

#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

#include "file.h"

#ifndef IMGUI_DISABLE
#include <imgui.h>
#include <imconfig.h>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#endif

#define WIN32_LEAN_AND_MEAN
#define GLM_CONFIG_CLIP_CONTROL GLM_CLIP_CONTROL_RH_NO

using namespace eXngine;
using namespace eXngine::Applications;
using namespace eXngine::Renderers;
using namespace eXngine::Renderers::Vulkan;

template <typename T>
std::vector<T> merge(std::vector<T> const& a, std::vector<T> const& b) {
    std::vector<T> result;
    result.reserve(a.size() + b.size());
    result.insert(result.end(), a.begin(), a.end());
    result.insert(result.end(), b.begin(), b.end());
    return result;
}

struct Camera {
    glm::vec3 position;
    glm::vec3 up;
    glm::vec3 front;
    float yaw;
    float pitch;
    float movementSpeed;
    float mouseSensitivity;
    Camera(glm::vec3 startPosition, glm::vec3 startUp, float startYaw, float startPitch) :
        position(startPosition), up(startUp), yaw(startYaw),
        pitch(startPitch), front(glm::vec3(0.0f, 0.0f, 0.0f)),
        movementSpeed(2.5f), mouseSensitivity(0.1f) {

    }
    glm::mat4 GetViewMatrix() {
        return glm::lookAt(position, position + front, up);
    }
    glm::mat4 GetProjectionMatrix(float aspectRatio) {
        return glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);
    }
};

Camera* camera = new Camera(glm::vec3(28.0f, 2.0f, 3.0f), glm::vec3(0.0f, 0.0f, 1.0f), 0.0f, 0.0f);
float m_fZoomFactor = 0.0f;
