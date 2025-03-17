// Copyright © 2023 - 2025 Alkaline Games, LLC.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "aboa-ue-helper.h"

//#include "Engine/Engine.h" // TODO: @@@ UNUSED
#include "Components/InputComponent.h"
#include "Kismet/GameplayStatics.h" // for GetPlayerPawn()
#include "Kismet/KismetSystemLibrary.h" // for PrintString(...)
//#include "UnrealEdGlobals.h" // TODO: @@@ UNUSED

#include <set>

#define ALK_TRACING 0

DECLARE_LOG_CATEGORY_EXTERN(LogAlkUeHelper, Log, All);
DEFINE_LOG_CATEGORY(LogAlkUeHelper);

auto EngineOrError(
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

auto PrimaryWorldOrError(
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
#if WITH_EDITOR // !!! because the editor has additional worlds
        auto mutName = mutWorld->OriginalWorldName.ToString();
        if (mutName != "None")
#endif
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

auto PawnInputComponentOrError(
  APawn        const & pawn,
  int          const index,
  char const * const purpose,
  char const * const info
) -> UInputComponent const * {
  auto const * const input = pawn.InputComponent.Get();
  if (!input)
    UE_LOG(LogAlkUeHelper, Error,
      TEXT("No InputComponent found for %s of %s"),
      ANSI_TO_TCHAR(purpose),
      ANSI_TO_TCHAR(info));
  return input;
}

auto PlayerInputComponentOrError(
  UWorld       const & world,
  int          const index,
  char const * const purpose,
  char const * const info
) -> UInputComponent const * {
  auto const * const pawn = PlayerPawnOrError(world, index, purpose, info);
  if (!pawn) { return nullptr; }
  return PawnInputComponentOrError(*pawn, index, purpose, info);
}

auto PlayerPawnOrError(
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

auto PluginSubpath(
  FString const & pluginName,
  FString const & subpath
) -> FString {
  return FPaths::ConvertRelativePathToFull(
    FPaths::Combine(FPaths::ProjectPluginsDir(), *pluginName, *subpath));
}

auto PluginFilePath(
  FString const & pluginName,
  FString const & subpath,
  FString const & filename
) -> FString {
  return FPaths::ConvertRelativePathToFull(
    FPaths::Combine(PluginSubpath(*pluginName, *subpath), *filename));
}

static auto const name_PrintStringToScreen = "PrintStringToScreen";
auto
PrintStringToScreen(
  FString const &       string,
  UWorld  const * const world
) -> bool {
  auto wor = world;
  if (!wor) {
    wor = PrimaryWorldOrError(
      name_PrintStringToScreen,
      TCHAR_TO_ANSI(*string));
    if (!wor)
      return false;
  }
#if ALK_TRACING
  UE_LOG(LogAlkUeHelper, Display, TEXT("TRACE C++ %s world %s \"%s\""),
    ANSI_TO_TCHAR(name_PrintStringToScreen),
    *(wor->OriginalWorldName.ToString()),
    *string);
#endif
  UKismetSystemLibrary::PrintString(wor, string);
  return true;
}
