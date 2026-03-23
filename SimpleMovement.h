// SimpleMovement.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SimpleMovement.generated.h"

UCLASS()
class YOURGAME_API ASimpleMovement : public ACharacter
{
    GENERATED_BODY()

public:
    ASimpleMovement();

protected:
    virtual void BeginPlay() override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    // Компоненты камеры от третьего лица
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    USpringArmComponent* CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    UCameraComponent* FollowCamera;

    // Движение
    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);

    // Бег
    void StartSprint();
    void StopSprint();

public:
    // Настройки скоростей
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float WalkSpeed = 400.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float SprintSpeed = 800.0f;

    // Проверка бега
    UFUNCTION(BlueprintCallable, Category = "Movement")
    bool IsSprinting() const { return bIsSprinting; }

private:
    bool bIsSprinting = false;
};