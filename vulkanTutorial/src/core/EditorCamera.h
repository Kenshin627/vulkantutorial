#pragma once
//#include "Kenshin/Renderer/Camera.h"
//#include "Kenshin/Core/TimeStamp.h"
//#include "Kenshin/Events/Event.h"
//#include "Kenshin/Events/KeyEvent.h"
//#include "Kenshin/Events/MouseEvent.h"
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

class EditorCamera
{
public:
	EditorCamera(float fov = 45.0f, float aspectRatio = 1.778f, float nearClip = 0.01f, float farClip = 100.0f) :m_Fov(fov), m_AspectRatio(aspectRatio), m_Near(nearClip), m_Far(farClip)
	{
		m_Projection = glm::perspective(glm::radians(m_Fov), m_AspectRatio, m_Near, m_Far);
		m_Projection[1][1] *= -1;
		UpdateView();
	}
	const glm::mat4& GetViewMatrix() const { return m_View; }
	const glm::mat4& GetProjection() const { return m_Projection; }
	const glm::mat4& GetViewProjectionMatrix() const { return m_Projection * m_View; }

	void UpdateView();
	void UpdateProjection();

	void OnUpdate(/*TimeStamp ts*/);
	//void OnEvent(Event& e);
	//bool OnMouseScrollEvent(MouseScrolledEvent& e);
	//void OnViewportResize(uint32_t width, uint32_t height);

	void MouseZoom(float delta);
	void MousePan(const glm::vec2& delta);
	void MouseRotate(const glm::vec2& delta);

	void CalculatePosition();
	glm::quat GetOrientation() const;
	glm::vec3 GetUpDirection() const;
	glm::vec3 GetRightDirection() const;
	glm::vec3 GetForwardDirection() const;
	std::pair<float, float> PanSpeed() const;
	float RotateSpeed() const;
	float ZoomSpeed() const;

private:
	float m_Fov = 45.0f, m_AspectRatio = 1.778f, m_Near = 0.01f, m_Far = 1000.0f;
	float m_Distance = 1.0f;
	glm::vec3 m_FocalPoint{ 0.0f }, m_Position{ 1.0f };
	uint32_t m_ViewportWidth = 1920, m_ViewportHeight = 1080;
	float m_Pitch = 0.0f, m_Yaw = 0.0f;
	glm::mat4 m_View;
	glm::mat4 m_Projection{ 1.0f };
	glm::vec2 m_InitialMousePostion{ 0.0f };
};
