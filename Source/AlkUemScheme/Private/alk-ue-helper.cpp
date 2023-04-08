// Copyright 2023 Alkaline Games, LLC.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "alk-ue-helper.h"

#include "Engine/Engine.h"
#include "Components/InputComponent.h"
#include "Kismet/GameplayStatics.h" // for GetPlayerPawn()

DECLARE_LOG_CATEGORY_EXTERN(LogAlkUeHelper, Log, All);
DEFINE_LOG_CATEGORY(LogAlkUeHelper);

static auto EngineOrError(
  char const * const purpose,
  char const * const info
) -> UEngine const * {
  if (!GEngine)
    UE_LOG(LogAlkUeHelper, Error,
      TEXT("No Engine found for %s of %s"),
      ANSI_TO_TCHAR(purpose),
      ANSI_TO_TCHAR(info));
  return GEngine;
}

static auto CurrentPlayWorld() -> UWorld const * {
  return !GEngine ? nullptr : GEngine->GetCurrentPlayWorld();
}

static auto MutWorldContexts() -> TIndirectArray<FWorldContext> const * {
  return !GEngine ? nullptr : &GEngine->GetWorldContexts();
}

static auto ApplyLambdaOnWorlds(
  std::function<void (UWorld &)> const & lambda
) -> void {
  auto const * const contexts = MutWorldContexts();
  if (contexts)
    for (auto mutIter = contexts->begin(); mutIter != contexts->end(); ++mutIter) {
      auto const mutWorld = (*mutIter).World();
      if (mutWorld)
        lambda(*mutWorld);
    }
}

// TODO: @@@ DEPRECATE IF NOT USED
static auto PrimaryWorldOrError(
  char const * const purpose,
  char const * const info
) -> UWorld const * {
  UWorld const * mutPrimary = nullptr;
#if 1
  auto const * const contexts = MutWorldContexts();
  if (contexts)
    for (auto mutIter = contexts->begin(); mutIter != contexts->end(); ++mutIter) {
      auto const mutWorld = (*mutIter).World();
      if (mutWorld) {
        auto mutName = mutWorld->OriginalWorldName.ToString();
        if (mutName != "None")
          mutPrimary = mutWorld;
        UE_LOG(LogAlkUeHelper, Display, TEXT("TRACE C++ found World %s"), *mutName);
      }
    }
#else
  mutPrimary = CurrentPlayWorld();
#endif
  if (!mutPrimary)
    UE_LOG(LogAlkUeHelper, Error,
      TEXT("No Engine/World found for %s of %s"),
      ANSI_TO_TCHAR(purpose),
      ANSI_TO_TCHAR(info));
  return mutPrimary;
}

static auto FirstPlayerPawnOrError(
  char const * const purpose,
  char const * const info
) -> APawn const * {
  auto const * const world = PrimaryWorldOrError(purpose, info);
  if (!world) { return nullptr; }
  auto const * const pawn = UGameplayStatics::GetPlayerPawn(world, 0);
  if (!pawn)
    UE_LOG(LogAlkUeHelper, Error,
      TEXT("No PlayerPawn found for %s of %s"),
      ANSI_TO_TCHAR(purpose),
      ANSI_TO_TCHAR(info));
  return pawn;
}

static auto FirstInputComponentOrError(
  char const * const purpose,
  char const * const info
) -> UInputComponent const * {
  auto const * const pawn = FirstPlayerPawnOrError(purpose, info);
  if (!pawn) { return nullptr; }
  auto const * const input = pawn->InputComponent.Get();
  if (!input)
    UE_LOG(LogAlkUeHelper, Error,
      TEXT("No InputComponent found for %s of %s"),
      ANSI_TO_TCHAR(purpose),
      ANSI_TO_TCHAR(info));
  return input;
}
