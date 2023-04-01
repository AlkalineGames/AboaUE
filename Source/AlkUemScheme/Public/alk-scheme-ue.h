// Copyright 2023 Alkaline Games, LLC.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
#pragma once

struct s7_scheme;

struct AlkSchemeUeState {
  const FString scmPath;
};

struct AlkSchemeUeMutant {
  const AlkSchemeUeState state;
  const s7_scheme* s7session;
      // ^ we cannot use std::shared_ptr<s7_scheme>
      //              or std::unique_ptr<s7_scheme>
      // because struct s7_scheme is incomplete in s7.h
};

auto bootAlkSchemeUe() -> AlkSchemeUeMutant;
