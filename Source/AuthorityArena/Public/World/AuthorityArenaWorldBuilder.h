#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AuthorityArenaWorldBuilder.generated.h"

class USceneComponent;
class UStaticMeshComponent;

UCLASS(NotBlueprintable)
class AUTHORITYARENA_API AAuthorityArenaWorldBuilder : public AActor
{
    GENERATED_BODY()

public:
    AAuthorityArenaWorldBuilder();

private:
    UStaticMeshComponent* CreateArenaBlock(
        FName Name,
        const FVector& RelativeLocation,
        const FVector& RelativeScale,
        bool bCollisionEnabled);

    UPROPERTY(VisibleAnywhere, Category = "Arena")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, Category = "Arena")
    TObjectPtr<UStaticMeshComponent> Floor;

    UPROPERTY(VisibleAnywhere, Category = "Arena")
    TObjectPtr<UStaticMeshComponent> NorthWall;

    UPROPERTY(VisibleAnywhere, Category = "Arena")
    TObjectPtr<UStaticMeshComponent> SouthWall;

    UPROPERTY(VisibleAnywhere, Category = "Arena")
    TObjectPtr<UStaticMeshComponent> EastWall;

    UPROPERTY(VisibleAnywhere, Category = "Arena")
    TObjectPtr<UStaticMeshComponent> WestWall;

    UPROPERTY(VisibleAnywhere, Category = "Arena")
    TObjectPtr<UStaticMeshComponent> CenterMarker;
};
