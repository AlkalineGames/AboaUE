// Copyright 2023 Alkaline Games, LLC.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

struct s7_scheme;

struct AlkSchemeUeCode {
  FString const path;
  FString const source;
};

struct AlkSchemeUeState {
  FString const scmPath;
};

struct AlkSchemeUeMutant {
  AlkSchemeUeState const state;
  s7_scheme * const s7session;
    // ^ we cannot use std::shared_ptr<s7_scheme>
    //              or std::unique_ptr<s7_scheme>
    // because struct s7_scheme is incomplete in s7.h
};

auto bootAlkSchemeUe() -> AlkSchemeUeMutant;

auto loadSchemeUeCode(
  FString const &path) -> AlkSchemeUeCode;

auto runSchemeUeCode(
  AlkSchemeUeMutant const &mutant,
  AlkSchemeUeCode   const &code) -> FString;

auto ALKUEMSCHEME_API runCachedSchemeUeCodeAtPath(
  FString const & path,
  bool forceReload = false) -> FString;
  // ^ caches and auto-loads observed file changes
