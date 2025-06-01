// Copyright © 2025 Christopher Augustus
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "AboaActorComponent.generated.h"

UCLASS(Blueprintable, meta=(ShortTooltip="aboa code for an actor"))
class ABOAUEM_API UAboaActorComponent : public UActorComponent
{
  GENERATED_BODY()

public:
  UAboaActorComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
  virtual ~UAboaActorComponent();

  // blueprintables that provide the source file path
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AboaActorComponent)
    FString AboaDirPlugin;
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AboaActorComponent)
    FString AboaDirSource;
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AboaActorComponent)
    FString AboaFilename;
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AboaActorComponent)
    FString AboaNamespace;

  // UActorComponent overrides
  virtual void InitializeComponent();
  //virtual void ReadyForReplication(); TODO: do we need this?
  virtual void BeginPlay();
  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason);
  virtual void UninitializeComponent();
  virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction);
};
