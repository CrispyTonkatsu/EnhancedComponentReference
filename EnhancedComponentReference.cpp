/**
 * @file EnhancedComponentReference.cpp
 * @author Edgar Jose Donoso Mansilla (e.donosomansilla)
 *
 * @brief This is the implementation for a reference that can be used to connect C++ and BP components in a type
 * safe way by using UE5's generated reflection information.
 *
 * @copyright DigiPen Institute of Technology 2026
 */

#include "Utilities/Property/EnhancedComponentReference.h"

#if WITH_EDITOR
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#endif

UEnhancedComponentReference* UEnhancedComponentReference::Create(
	const TSubclassOf<UActorComponent> Type,
	UObject* Owner,
	const FName Name)
{
	if (!Type->IsValidLowLevel())
	{
		UE_LOG(
			LogEnhancedComponentReference,
			Warning,
			TEXT("The type provided is invalid, check the UClass being provided."));
		return nullptr;
	}

	if (Owner == nullptr)
	{
		UE_LOG(
			LogEnhancedComponentReference,
			Warning,
			TEXT("The owner being provided is null"));
		return nullptr;
	}

	FName ToAssign{Type->GetName() + TEXT("_Ref")};
	if (not Name.IsNone())
	{
		ToAssign = Name;
	}

	UEnhancedComponentReference* Output{Owner->CreateDefaultSubobject<UEnhancedComponentReference>(ToAssign)};
	Output->Type = Type;

	return Output;
}

// This function could be optimized, but I think that there really isn't much need for this
#if WITH_EDITOR
TArray<FString> UEnhancedComponentReference::GetAvailableComponentNames() const
{
	if (bUseOtherAsset)
	{
		if (ProvidedArchetype == nullptr)
		{
			UE_LOG(
				LogEnhancedComponentReference,
				Warning,
				TEXT("The archetype provided is invalid, check the UClass being provided."));
			return {};
		}
	}

	if (!Type->IsValidLowLevel())
	{
		UE_LOG(
			LogEnhancedComponentReference,
			Warning,
			TEXT("The type provided is invalid, check the UClass being provided."));
		return {};
	}

	const UObject* Outer{not bUseOtherAsset ? GetOutermostObject() : ProvidedArchetype->GetOutermostObject(),};
	if (Outer == nullptr)
	{
		UE_LOG(
			LogEnhancedComponentReference,
			Warning,
			TEXT(""));
		return {};
	}

	// Handle the cases where the Outer has the class of the blueprint, and we need to manage from there
	if (Cast<AActor>(Outer) != nullptr)
	{
		Outer = Outer->GetClass();
	}

	const AActor* Actor{ObjToActor(Outer)};
	if (Actor == nullptr)
	{
		UE_LOG(
			LogEnhancedComponentReference,
			Warning,
			TEXT("There is no valid owner to get the components"));
		return {};
	}

	TArray<FString> Output;

	// Checking the BP side of things

	if (const UBlueprintGeneratedClass* BPClass{Cast<UBlueprintGeneratedClass>(Outer)}; BPClass != nullptr)
	{
		const UBlueprint* Blueprint{Cast<UBlueprint>(BPClass->ClassGeneratedBy)};
		if (Blueprint != nullptr)
		{
			const TArray Nodes{Blueprint->SimpleConstructionScript->GetAllNodes()};
			for (const USCS_Node* Node : Nodes)
			{
				const UActorComponent* BPComponent = Cast<UActorComponent>(Node->ComponentTemplate);
				if (BPComponent == nullptr)
				{
					continue;
				}

				if (!BPComponent->GetClass()->IsChildOf(Type))
				{
					continue;
				}

				// We are removing the part that is appended for the serialization by the engine
				const FString Ending{"_GEN_VARIABLE"};
				FString ReadableName{BPComponent->GetName()};
				ReadableName.RemoveFromEnd(Ending);

				Output.Push(ReadableName);
			}
		}
	}

	// Checking the C++ side of things

	TArray<UActorComponent*> Components{};
	Actor->GetComponents(Type, Components);

	for (const UActorComponent* CurrentComponent : Components)
	{
		Output.Push(CurrentComponent->GetName());
	}

	return Output;
}
#endif

UActorComponent* UEnhancedComponentReference::GetComponent(const UObject* InstancedObject) const
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
	if (InstancedObject->GetClass()->IsChildOf(UActorComponent::StaticClass()))
	{
		Actor = Cast<UActorComponent>(InstancedObject)->GetOwner();
	}
	else if (InstancedObject->GetClass()->IsChildOf(AActor::StaticClass()))
	{
		Actor = Cast<AActor>(InstancedObject);
	}

	if (Actor == nullptr)
	{
		UE_LOG(
			LogEnhancedComponentReference,
			Warning,
			TEXT("There is no valid owner to get the components"));
		return nullptr;
	}

	// Checking the C++ components
	TArray<UActorComponent*> Components{};
	Actor->GetComponents(Type, Components);

	for (UActorComponent* CurrentComponent : Components)
	{
		if (CurrentComponent->GetName() == ComponentName)
		{
			return CurrentComponent;
		}
	}

	// Checking the BP Components
	const TArray<UActorComponent*> BPComponents{Actor->BlueprintCreatedComponents};
	for (UActorComponent* CurrentComponent : BPComponents)
	{
		if (CurrentComponent->GetName() == ComponentName)
		{
			if (!CurrentComponent->GetClass()->IsChildOf(Type))
			{
				continue;
			}
			return CurrentComponent;
		}
	}

	return nullptr;
}

const AActor* UEnhancedComponentReference::ObjToActor(const UObject* Object)
{
	// Handling the cases where the class was made in c++
	if (const AActor* Actor{Cast<AActor>(Object)}; Actor != nullptr)
	{
		return Actor;
	}

#if WITH_EDITOR
	if (const UBlueprintGeneratedClass* BPClass{Cast<UBlueprintGeneratedClass>(Object)}; BPClass != nullptr)
	{
		// Traversing all the way up the chain of operations for the 
		const UBlueprint* Generator{Cast<UBlueprint>(BPClass->ClassGeneratedBy)};
		const UClass* ParentClass{Cast<UClass>(Generator->ParentClass)};

		while (ParentClass && not ParentClass->IsNative())
		{
			ParentClass = ParentClass->GetSuperClass();
		}

		return Cast<AActor>(ParentClass->GetDefaultObject());
	}
#endif

	// NOTE: If there are other cases where we need to find the default object,
	// but we can't with any method implemented above, expand the implementation here

	return nullptr;
}
