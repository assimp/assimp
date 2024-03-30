// Based on Sascha WIllems' Vulkan example.
#pragma once

// Use <tinyudsz>/src/external/linalg.h instead of glm
#include "external/linalg.h"

//#define GLM_FORCE_RADIANS
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//#include <glm/glm.hpp>
//#include <glm/gtc/quaternion.hpp>
//#include <glm/gtc/matrix_transform.hpp>


namespace example {

using vec2 = linalg::vec<float, 2>; 
using vec3 = linalg::vec<float, 3>; 
using vec4 = linalg::vec<float, 4>; 
using mat4 = linalg::mat<float, 4, 4>; 

constexpr float pi = 3.141592f;

inline float radians(float deg) {
  return pi * deg / 180.0f;
}

/*
* Basic camera class
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/


class Camera
{

private:
	float fov{45.0f}; // in degree
	float znear{0.01f}, zfar{1000.0f};

	void updateViewMatrix()
	{
		mat4 rotM = linalg::identity;
		mat4 transM;

		rotM = linalg::mul(rotM, linalg::rotation_matrix(linalg::rotation_quat(vec3(1.0f, 0.0f, 0.0f), radians(rotation.x * (flipY ? -1.0f : 1.0f)))));
		rotM = linalg::mul(rotM, linalg::rotation_matrix(linalg::rotation_quat(vec3(0.0f, 1.0f, 0.0f), radians(rotation.y))));
		rotM = linalg::mul(rotM, linalg::rotation_matrix(linalg::rotation_quat(vec3(0.0f, 0.0f, 1.0f), radians(rotation.z))));

		vec3 translation = position;
		if (flipY) {
			translation.y *= -1.0f;
		}
		transM = linalg::translation_matrix(translation);

		if (type == CameraType::firstperson)
		{
			matrices.view = linalg::mul(rotM, transM);
		}
		else
		{
			matrices.view = linalg::mul(transM, rotM);
		}

		viewPos = vec4(position, 0.0f) * vec4(-1.0f, 1.0f, -1.0f, 1.0f);

		updated = true;
	};

public:
	enum CameraType { lookat, firstperson };
	CameraType type = CameraType::lookat;

	vec3 rotation = vec3(); // in degree
	vec3 position = vec3();
	vec4 viewPos = vec4();

	float rotationSpeed = 1.0f;
	float movementSpeed = 1.0f;

	bool updated = false;
	bool flipY = false;

	struct
	{
		mat4 perspective{linalg::identity};
		mat4 view{linalg::identity};
	} matrices;

	struct
	{
		bool left = false;
		bool right = false;
		bool up = false;
		bool down = false;
	} keys;

	bool moving()
	{
		return keys.left || keys.right || keys.up || keys.down;
	}

	float getNearClip() { 
		return znear;
	}

	float getFarClip() {
		return zfar;
	}

	void setPerspective(float fov, float aspect, float znear, float zfar)
	{
		this->fov = fov;
		this->znear = znear;
		this->zfar = zfar;
		matrices.perspective = linalg::perspective_matrix(radians(fov), aspect, znear, zfar);
		if (flipY) {
			matrices.perspective[1][1] *= -1.0f;
		}
	};

	void updateAspectRatio(float aspect)
	{
		matrices.perspective = linalg::perspective_matrix(radians(fov), aspect, znear, zfar);
		if (flipY) {
			matrices.perspective[1][1] *= -1.0f;
		}
	}

	void setPosition(vec3 position)
	{
		this->position = position;
		updateViewMatrix();
	}

	void setRotation(vec3 rotation)
	{
		this->rotation = rotation;
		updateViewMatrix();
	}

	void rotate(vec3 delta)
	{
		this->rotation += delta;
		updateViewMatrix();
	}

	void setTranslation(vec3 translation)
	{
		this->position = translation;
		updateViewMatrix();
	};

	void translate(vec3 delta)
	{
		this->position += delta;
		updateViewMatrix();
	}

	void setRotationSpeed(float rotationSpeed)
	{
		this->rotationSpeed = rotationSpeed;
	}

	void setMovementSpeed(float movementSpeed)
	{
		this->movementSpeed = movementSpeed;
	}

	void update(float deltaTime)
	{
		updated = false;
		if (type == CameraType::firstperson)
		{
			if (moving())
			{
				vec3 camFront;
				camFront.x = -cos(radians(rotation.x)) * sin(radians(rotation.y));
				camFront.y = sin(radians(rotation.x));
				camFront.z = cos(radians(rotation.x)) * cos(radians(rotation.y));
				camFront = linalg::normalize(camFront);

				float moveSpeed = deltaTime * movementSpeed;

				if (keys.up)
					position += camFront * moveSpeed;
				if (keys.down)
					position -= camFront * moveSpeed;
				if (keys.left)
					position -= linalg::normalize(linalg::cross(camFront, vec3(0.0f, 1.0f, 0.0f))) * moveSpeed;
				if (keys.right)
					position += linalg::normalize(linalg::cross(camFront, vec3(0.0f, 1.0f, 0.0f))) * moveSpeed;

				updateViewMatrix();
			}
		}
	};

	// Update camera passing separate axis data (gamepad)
	// Returns true if view or position has been changed
	bool updatePad(vec2 axisLeft, vec2 axisRight, float deltaTime)
	{
		bool retVal = false;

		if (type == CameraType::firstperson)
		{
			// Use the common console thumbstick layout		
			// Left = view, right = move

			const float deadZone = 0.0015f;
			const float range = 1.0f - deadZone;

			vec3 camFront;
			camFront.x = -cos(radians(rotation.x)) * sin(radians(rotation.y));
			camFront.y = sin(radians(rotation.x));
			camFront.z = cos(radians(rotation.x)) * cos(radians(rotation.y));
			camFront = linalg::normalize(camFront);

			float moveSpeed = deltaTime * movementSpeed * 2.0f;
			float rotSpeed = deltaTime * rotationSpeed * 50.0f;
			 
			// Move
			if (std::fabs(axisLeft.y) > deadZone)
			{
				float pos = (std::fabs(axisLeft.y) - deadZone) / range;
				position -= camFront * pos * ((axisLeft.y < 0.0f) ? -1.0f : 1.0f) * moveSpeed;
				retVal = true;
			}
			if (std::fabs(axisLeft.x) > deadZone)
			{
				float pos = (std::fabs(axisLeft.x) - deadZone) / range;
				position += linalg::normalize(linalg::cross(camFront, vec3(0.0f, 1.0f, 0.0f))) * pos * ((axisLeft.x < 0.0f) ? -1.0f : 1.0f) * moveSpeed;
				retVal = true;
			}

			// Rotate
			if (std::fabs(axisRight.x) > deadZone)
			{
				float pos = (std::fabs(axisRight.x) - deadZone) / range;
				rotation.y += pos * ((axisRight.x < 0.0f) ? -1.0f : 1.0f) * rotSpeed;
				retVal = true;
			}
			if (std::fabs(axisRight.y) > deadZone)
			{
				float pos = (std::fabs(axisRight.y) - deadZone) / range;
				rotation.x -= pos * ((axisRight.y < 0.0f) ? -1.0f : 1.0f) * rotSpeed;
				retVal = true;
			}
		}
		else
		{
			// todo: move code from example base class for look-at
		}

		if (retVal)
		{
			updateViewMatrix();
		}

		return retVal;
	}

};


} // example
