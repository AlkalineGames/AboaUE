// Copyright 2024 Alkaline Games, LLC.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <any>

auto ueStringFromAny(       std::any) -> FString;
auto ueVectorFromAny(       std::any) -> FVector;
auto ueVectorArrayFromAny(  std::any) -> TArray<FVector>;
