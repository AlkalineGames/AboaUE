// Copyright 2023 Alkaline Games, LLC.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "alk-ue-helper.h"

//#include "Engine/Engine.h" // TODO: @@@ UNUSED
#include "Components/InputComponent.h"
#include "Kismet/GameplayStatics.h" // for GetPlayerPawn()
//#include "UnrealEdGlobals.h" // TODO: @@@ UNUSED

#include <set>

#define ALK_TRACING 0

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

static auto FindAllMutWorlds(
) -> std::set<UWorld *> {
  std::set<UWorld *> worlds;
  auto const * const contexts = MutWorldContexts();
  if (contexts)
    for (auto mutIter = contexts->begin(); mutIter != contexts->end(); ++mutIter) {
      auto const world = (*mutIter).World();
      if (world) {
#if ALK_TRACING
        UE_LOG(LogAlkUeHelper, Display,
          TEXT("TRACE C++ found world context %s"),
          *(world->OriginalWorldName.ToString()));
#endif
        worlds.insert(world);
      }
    }
#if 0 // TODO: @@@ UNUSED
  if (GUnrealEd) { // TODO: @@@ ARE THERE REALLY MORE WORLDS THAN ABOVE?
    auto const clients = GUnrealEd->GetAllViewportClients();
    for (auto mutIter = clients.begin(); mutIter != clients.end(); ++mutIter) {
      auto const client = *mutIter;
      if (client) {
        auto const world = client->GetWorld();
        if (world) {
#if ALK_TRACING
          UE_LOG(LogAlkUeHelper, Display,
            TEXT("TRACE C++ found viewport client world %s"),
            *(world->OriginalWorldName.ToString()));
#endif
          worlds.insert(world);
        }
      }
    }
  }
#endif
  return worlds;
}

static auto ApplyLambdaOnAllWorlds(
  std::function<void (UWorld &)> const & lambda
) -> void {
  auto worlds = FindAllMutWorlds();
  std::for_each(worlds.begin(), worlds.end(),
    [&lambda](UWorld * const & world) {
      if (world) {
#if ALK_TRACING
        UE_LOG(LogAlkUeHelper, Display,
          TEXT("TRACE C++ applying lambda to world %s"),
          *(world->OriginalWorldName.ToString()));
#endif
        lambda(*world);
      }
    });
}

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
#if ALK_TRACING
        UE_LOG(LogAlkUeHelper, Display, TEXT("TRACE C++ found World %s"), *mutName);
#endif
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

static auto PlayerPawnOrError(
  UWorld       const & world,
  int          const index,
  char const * const purpose,
  char const * const info
) -> APawn const * {
  auto const * const pawn = UGameplayStatics::GetPlayerPawn(&world, index);
  if (!pawn)
    UE_LOG(LogAlkUeHelper, Error,
      TEXT("No PlayerPawn found for %s of %s"),
      ANSI_TO_TCHAR(purpose),
      ANSI_TO_TCHAR(info));
  return pawn;
}

static auto PlayerInputComponentOrError(
  UWorld       const & world,
  int          const index,
  char const * const purpose,
  char const * const info
) -> UInputComponent const * {
  auto const * const pawn = PlayerPawnOrError(world, index, purpose, info);
  if (!pawn) { return nullptr; }
  auto const * const input = pawn->InputComponent.Get();
  if (!input)
    UE_LOG(LogAlkUeHelper, Error,
      TEXT("No InputComponent found for %s of %s"),
      ANSI_TO_TCHAR(purpose),
      ANSI_TO_TCHAR(info));
  return input;
}
