// Copyright 2024 Alkaline Games, LLC.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <any>

auto ueStringPtrFromAny(       std::any const &) -> FString         const *;
auto ueVectorPtrFromAny(       std::any const &) -> FVector         const *;
auto ueVectorArrayPtrFromAny(  std::any const &) -> TArray<FVector> const *;
