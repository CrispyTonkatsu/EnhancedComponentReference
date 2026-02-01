# EnhancedComponentReference
Unreal Engine 5.6 Data type to easily reference blueprint created components in C++. This aims to provide a more Unity-like experience when creating designer friendly C++ components.

## Why did I make this?
This was started to meet the need of a component needing to hold a reference to a `UShapeComponent`. However, because Unreal Engine needs to be able to construct the component being used for the default object, the component would need to know the shape at construction time.

Unreal Engine has a type to help with this, it is `FComponentReference`. The main issue that this has is that it is unable to establish invariance for the types allowed and mainly relies on strings to look up the component by name. This allows for several instances of instability in the project. While this would normally be fine for small teams, the project had 18 members of which 8 were only engineers. To add to the risks, the project was being version controlled in Perforce without streams (as the IT department said the student projects were only allowed to have 1 stream).

To deal with the issue, I took advantage of the following `UProperty`, ```c++ meta=(GetOptions="Func")```, which will generate a drop down menu for the array of strings being returned by`Func`. This allowed me to use a `UObject`'s packaging information to find out what components it had.

There is one major issue that it ran into, Blueprint created components and C++ created components were stored in very different ways. This is why the code had to be able to handle 

## How to use it

This tool solves the problem by allowing a C++ script to declare the following in their header file for a component class:
```c++
UPROPERTY(EditDefaultsOnly)
UEnhancedComponentReference* MyShapeRef;
```

And then the folloiwing in their C++ scripts:
```c++
MyComponent::MyComponent(){
    // NOTE: This is to initialize the reference so it shows up in the editor and the drop down can be generated
    MyShapeRef = UEnhancedComponentReference::Create<UShapeComponent>(*this, "MyShape_Ref");
}

void MyComponent::MyFunc(){
    // NOTE: The instance of the component is needed as it is a different one from the one passed in the constructor
    TOption<UShapeComponent*> MyShapeOption{MyShapeRef->GetComponent(*this)};

    // NOTE: This is the same function but it will return a nullptr if it can't be found (meant for blueprints if for whatever reason a designer wants to get it like this)
    UActorComponent* MyShapeAsBase{MyShapeRef->GetComponent(this)};
}
```

## Current limitations
You can't use this type on the `AActor` deriving classes. While this is good for my project where I aimed to keep all the core functionality in components for modularity, it is bad in terms of this tool itself being flexible to different projects and scales (A simple `AActor` like a projectile would normally have all the logic in the `AActor` itself).

## Remark
This tool is being used in the DigiPen student project "Idol On Duty" for the implementation of the player and enemies.
As of now, the code in this repository is under DigiPen Insitute of Technology's ownership and copyright with permission to be uploaded for the purpose of sample code.
