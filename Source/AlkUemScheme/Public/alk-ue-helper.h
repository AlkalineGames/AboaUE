// Copyright 2023 Alkaline Games, LLC.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

class APawn;
class UInputComponent;
class UWorld;

static auto EngineOrError(
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

static auto PrimaryWorldOrError(
  char const * const purpose,
  char const * const info
) -> UWorld const *;

static auto MutPrimaryWorldOrError(
  char const * const purpose,
  char const * const info
) -> UWorld * {
  return const_cast<UWorld*>(PrimaryWorldOrError(purpose, info));
}

static auto PlayerPawnOrError(
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

static auto PlayerInputComponentOrError(
  UWorld       const & world,
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
