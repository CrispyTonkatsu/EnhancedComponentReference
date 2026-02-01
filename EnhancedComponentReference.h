/**
 * @file EnhancedComponentReference.cpp
 * @author Edgar Jose Donoso Mansilla (e.donosomansilla)
 *
 * @brief This is the implementation for a reference that can be used to connect C++ and BP components in a type
 * safe way by using UE5's generated reflection information.
 *
 * @copyright DigiPen Institute of Technology 2026
 */

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "EnhancedComponentReference.generated.h"

template <typename T>
concept ComponentClass = std::is_base_of_v<UActorComponent, T>;

DECLARE_LOG_CATEGORY_CLASS(LogEnhancedComponentReference, Warning, Warning)

/**
 * @brief Type to refer to another component in the same actor.
 * This allows for C++ to get references to BP added components.
 */
UCLASS()
class IDOLONDUTY_API UEnhancedComponentReference : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * @brief Factory method for an enhanced reference (CONSTRUCTOR ONLY)
	 * @param Type The type that this reference will work with
	 * @param Owner The owner of this reference (almost always it should be this)
	 * @param Name The name of the reference in the editor
	 * @return Pointer to the new reference
	 */
	UFUNCTION()
	[[nodiscard]] static UEnhancedComponentReference* Create(
		const TSubclassOf<UActorComponent> Type,
		UObject* Owner,
		const FName Name = "");

	/**
	 * @brief Factory method for an enhanced reference (CONSTRUCTOR ONLY)
	 * @tparam T The type that this reference will work with
	 * @param Owner The owner of this reference (almost always it should be this)
	 * @return Pointer to the new reference
	 */
	template <ComponentClass T>
	[[nodiscard]] static UEnhancedComponentReference* Create(UObject& Owner, const FName Name = "");
	
	UPROPERTY(EditAnywhere, meta=(GetOptions="GetAvailableComponentNames"))
	FName ComponentName;

	UPROPERTY(VisibleAnywhere)
	TSubclassOf<UActorComponent> Type;

	// TODO: Store the default archetype to be able to tell whether the object provided for getting the component
	// is of the type said at creation
	// UPROPERTY(VisibleAnywhere)
	// TSubclassOf<AActor> DefaultArchetype;

	/**
	 * Whether to use this asset as the Component Holder or whether to receive it from other assets that would be of
	 * the type provided by `ProvidedArchetype`
	 */
	UPROPERTY(EditDefaultsOnly)
	bool bUseOtherAsset{false};

	/**
	 * The Archetype to look into for the reference name
	 */
	UPROPERTY(EditDefaultsOnly, meta=(EditCondition="bUseOtherAsset"))
	TSubclassOf<AActor> ProvidedArchetype;

#if WITH_EDITOR
	UFUNCTION()
	[[nodiscard]] TArray<FString> GetAvailableComponentNames() const;
#endif

	// TODO: Check whether the provided UObject is of the type that was used at declaration time
	
	/**
	 * @brief This is the getter for the component that has been referenced.
	 * @param InstancedObject The owner that we should be fetching from. This should be the instance that was created from the blueprint
	 * @return Pointer to the component
	 */
	UFUNCTION()
	[[nodiscard]] UActorComponent* GetComponent(const UObject* InstancedObject) const;

	/**
	 * @brief This is the getter for the component that has been referenced.
	 * @tparam T The component type to try and cast it to
	 * @param InstancedObject The owner that we should be fetching from
	 * @return Pointer to the component
	 */
	template <ComponentClass T>
	[[nodiscard]] TOptional<T*> GetComponent(const UObject& InstancedObject) const;

private:
	static const AActor* ObjToActor(const UObject* Object);
};

template <ComponentClass T>
UEnhancedComponentReference* UEnhancedComponentReference::Create(UObject& Owner, const FName Name)
{
	FName ToAssign{T::StaticClass()->GetName() + TEXT("_Ref")};
	if (not Name.IsNone())
	{
		ToAssign = Name;
	}

	UEnhancedComponentReference* Output{Owner.CreateDefaultSubobject<UEnhancedComponentReference>(ToAssign)};
	Output->Type = T::StaticClass();

	return Output;
}

template <ComponentClass T>
TOptional<T*> UEnhancedComponentReference::GetComponent(const UObject& InstancedObject) const
{
	if (Type.Get() == nullptr)
	{
		UE_LOG(
			LogEnhancedComponentReference,
			Warning,
			TEXT(
				"The type is not valid (are you creating the reference in the constructor?, if so, try resetting the value to it's default)"
			));
		return nullptr;
	}

	const AActor* Actor{nullptr};
	if (InstancedObject.GetClass()->IsChildOf(UActorComponent::StaticClass()))
	{
		Actor = Cast<UActorComponent>(&InstancedObject)->GetOwner();
	}
	else if (InstancedObject.GetClass()->IsChildOf(AActor::StaticClass()))
	{
		Actor = Cast<AActor>(&InstancedObject);
	}

	if (Actor == nullptr)
	{
		UE_LOG(
			LogEnhancedComponentReference,
			Warning,
			TEXT("There is no valid owner to get the components"));
		return {};
	}

	// Checking the C++ components
	TArray<T*> Components{};
	Actor->GetComponents(Type, Components);

	for (T* CurrentComponent : Components)
	{
		if (CurrentComponent->GetName() == ComponentName)
		{
			T* Output = Cast<T>(CurrentComponent);
			if (Output == nullptr)
			{
				continue;
			}
			return Output;
		}
	}

	// Checking the BP components
	const TArray<UActorComponent*> BPComponents{Actor->BlueprintCreatedComponents};
	for (UActorComponent* CurrentComponent : BPComponents)
	{
		if (CurrentComponent->GetName() == ComponentName)
		{
			T* Output = Cast<T>(CurrentComponent);
			if (Output == nullptr)
			{
				continue;
			}
			return Output;
		}
	}

	return NullOpt;
}
