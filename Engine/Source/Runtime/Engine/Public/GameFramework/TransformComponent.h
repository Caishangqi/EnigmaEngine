// Copyright EnigmaEngine. All Rights Reserved.

#pragma once

/// @file TransformComponent.h
/// @brief Built-in transform component for every FGameObject.

#include "GameFramework/Component.h"
#include "Math/Transform.h"
#include "Math/Vector.h"
#include "Math/Rotator.h"

namespace Enigma
{

/// @brief Built-in transform component providing position, rotation, and scale.
///
/// Every FGameObject automatically owns one FTransformComponent as a value member.
/// This is a thin wrapper over FTransform, exposing convenience accessors.
///
/// Not stored in the m_components vector -- accessed via FGameObject::GetTransform().
///
/// Unity equivalent: Transform (built-in, always present)
/// UE equivalent: USceneComponent::GetRelativeTransform()
class ENGINE_API FTransformComponent : public FComponent
{
public:
	// ----- Type Identification -----

	static FName GetStaticName() { return FName("TransformComponent"); }
	FName GetName() const override { return GetStaticName(); }

	// ----- Transform Access -----

	/// Get mutable reference to the underlying FTransform.
	FTransform& GetTransform() { return m_transform; }

	/// Get const reference to the underlying FTransform.
	const FTransform& GetTransform() const { return m_transform; }

	// ----- Position -----

	void SetPosition(const FVector& position) { m_transform.SetTranslation(position); }
	[[nodiscard]] FVector GetPosition() const { return m_transform.GetTranslation(); }

	// ----- Rotation -----

	void SetRotation(const FRotator& rotation) { m_transform.SetRotation(rotation.Quaternion()); }
	[[nodiscard]] FRotator GetRotation() const { return FRotator(m_transform.GetRotation()); }

	// ----- Scale -----

	void SetScale(const FVector& scale) { m_transform.SetScale3D(scale); }
	[[nodiscard]] FVector GetScale() const { return m_transform.GetScale3D(); }

private:
	FTransform m_transform;
};

} // namespace Enigma
