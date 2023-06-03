//#include "kspch.h"
#include "EditorCamera.h"
//#include "Kenshin/Core/Input.h"
#include "WindowsInput.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <gtx/quaternion.hpp>

void EditorCamera::UpdateView()
{
	CalculatePosition();
	auto view = glm::translate(glm::mat4(1.0f), m_Position) * glm::toMat4(GetOrientation());
	m_View = glm::inverse(view);
}

void EditorCamera::UpdateProjection()
{
	m_Projection = glm::perspective(glm::radians(m_Fov), m_AspectRatio, m_Near, m_Far);
}

void EditorCamera::OnUpdate()
{
	if (WindowsInput::IsKeyPressed(Key::LeftAlt))
	{
		const glm::vec2 mousePosition{ WindowsInput::GetMouseX(), WindowsInput::GetMouseY() };
		glm::vec2  delta = (mousePosition - m_InitialMousePostion) * 0.003f;
		m_InitialMousePostion = mousePosition;
		if (WindowsInput::IsMousePressed(Mouse::ButtonLeft))
		{
			MousePan(delta);
		}

		if (WindowsInput::IsMousePressed(Mouse::ButtonMiddle))
		{
			MouseRotate(delta);
		}

		if (WindowsInput::IsMousePressed(Mouse::ButtonRight))
		{
			MouseZoom(delta.y);
		}

		UpdateView();
	}
}

//void EditorCamera::OnEvent(Event& e)
//{
//	EventDispatcher dispatcher(e);
//	dispatcher.Dispatch<MouseScrolledEvent>(BIND_EVENT_FN(EditorCamera::OnMouseScrollEvent, std::placeholders::_1));
//}

//bool EditorCamera::OnMouseScrollEvent(MouseScrolledEvent& e)
//{
//	MouseZoom(e.GetYOffset() * 0.1f);
//	UpdateView();
//	return false;
//}

//void EditorCamera::OnViewportResize(uint32_t width, uint32_t height)
//{
//	m_ViewportWidth = width;
//	m_ViewportHeight = height;
//	m_AspectRatio = (float)m_ViewportWidth / (float)m_ViewportHeight;
//	UpdateProjection();
//}

void EditorCamera::MouseZoom(float delta)
{
	m_Distance -= delta * ZoomSpeed();
	if (m_Distance < 1.0f)
	{
		m_FocalPoint += GetForwardDirection();
		m_Distance = 1.0f;
	}
}

void EditorCamera::MousePan(const glm::vec2& delta)
{
	auto [xSpeed, ySpeed] = PanSpeed();
	m_FocalPoint += delta.x * GetRightDirection() * xSpeed * m_Distance;
	m_FocalPoint += delta.y * GetUpDirection() * ySpeed * m_Distance;
}

void EditorCamera::MouseRotate(const glm::vec2& delta)
{
	float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;
	m_Yaw += yawSign * delta.x * RotateSpeed();
	m_Pitch += delta.y * RotateSpeed();
}

void EditorCamera::CalculatePosition()
{
	m_Position = m_FocalPoint - m_Distance * GetForwardDirection();
}

glm::quat EditorCamera::GetOrientation() const
{
	return glm::quat(glm::vec3(-m_Pitch, -m_Yaw, 0.0f));
}

glm::vec3 EditorCamera::GetUpDirection() const
{
	return glm::rotate(GetOrientation(), glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::vec3 EditorCamera::GetRightDirection() const
{
	return glm::rotate(GetOrientation(), glm::vec3(1.0f, 0.0f, 0.0f));
}

glm::vec3 EditorCamera::GetForwardDirection() const
{
	return glm::rotate(GetOrientation(), glm::vec3(0.0f, 0.0f, -1.0f));
}

std::pair<float, float> EditorCamera::PanSpeed() const
{
	float x = std::min(m_ViewportWidth / 1000.0f, 2.4f);
	float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;
	float y = std::min(m_ViewportHeight / 1000.0f, 2.4f);
	float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;
	return { xFactor, yFactor };
}

float EditorCamera::RotateSpeed() const
{
	return 0.8f;
}

float EditorCamera::ZoomSpeed() const
{
	float distance = std::max(m_Distance * 0.2f, 0.0f);
	float speed = std::min(distance * distance, 100.0f);
	return speed;
}
