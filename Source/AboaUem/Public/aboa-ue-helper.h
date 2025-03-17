// Copyright © 2023 - 2025 Alkaline Games, LLC.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

class APawn;
class UInputComponent;
class UWorld;

auto EngineOrError(
  char const * const purpose,
  char const * const info
) -> UEngine const *;

static auto MutEngineOrError(
  char const * const purpose,
  char const * const info
) -> UEngine * {
  return const_cast<UEngine*>(EngineOrError(purpose, info));
}

static auto CurrentPlayWorld()
-> UWorld const *;

static auto MutWorldContexts()
-> TIndirectArray<FWorldContext> const *;

static auto ApplyLambdaOnAllWorlds(
  std::function<void (UWorld &)> const &
) -> void;

auto PrimaryWorldOrError(
  char const * const purpose,
  char const * const info
) -> UWorld const *;

static auto MutPrimaryWorldOrError(
  char const * const purpose,
  char const * const info
) -> UWorld * {
  return const_cast<UWorld*>(PrimaryWorldOrError(purpose, info));
}

auto PlayerPawnOrError(
  UWorld       const & world,
  int          const index,
  char const * const purpose,
  char const * const info
) -> APawn const *;

static auto MutPlayerPawnOrError(
  UWorld       const & world,
  int          const index,
  char const * const purpose,
  char const * const info
) -> APawn * {
  return const_cast<APawn*>(PlayerPawnOrError(world, index, purpose, info));
}

auto PawnInputComponentOrError(
  APawn        const &,
  int          const index,
  char const * const purpose,
  char const * const info
) -> UInputComponent const *;

static auto MutPawnInputComponentOrError(
  APawn        const & pawn,
  int          const index,
  char const * const purpose,
  char const * const info
) -> UInputComponent * {
  return const_cast<UInputComponent*>(PawnInputComponentOrError(pawn, index, purpose, info));
}

auto PlayerInputComponentOrError(
  UWorld       const &,
  int          const index,
  char const * const purpose,
  char const * const info
) -> UInputComponent const *;

static auto MutPlayerInputComponentOrError(
  UWorld       const & world,
  int          const index,
  char const * const purpose,
  char const * const info
) -> UInputComponent * {
  return const_cast<UInputComponent*>(PlayerInputComponentOrError(world, index, purpose, info));
}

auto PluginSubpath(
  FString const & pluginName,
  FString const & subpath
) -> FString;

auto ABOAUEM_API PluginFilePath(
  FString const & pluginName,
  FString const & subpath,
  FString const & filename
) -> FString;

auto ABOAUEM_API PrintStringToScreen(
  FString const &       string,
  UWorld  const * const world = nullptr
) -> bool;
