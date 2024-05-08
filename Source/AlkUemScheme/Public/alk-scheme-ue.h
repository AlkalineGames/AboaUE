// Copyright 2023 - 2024 Alkaline Games, LLC.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

// NOTE: !!! std::any fails to cast across shared libraries on macOS
#define ALK_SCHEME_UE_STD_ANY 1
#if ALK_SCHEME_UE_STD_ANY
#include <any>
typedef std::any AlkSchemeUeDataAny;
#else
typedef void * AlkSchemeUeDataAny;
  // ^ TODO: @@@ placeholder for possible rewrite
#endif

#include <map>

struct s7_scheme;

struct AlkSchemeUeCode {
  FString const path;
  FString const source;
};

enum struct AlkSchemeUeDataType {
  Nothing, Bool, String, Vector, VectorArray
};

struct AlkSchemeUeDataRef {
  AlkSchemeUeDataAny  const any;
  AlkSchemeUeDataType const type;
};

struct AlkSchemeUeDataArg {
  FString             const name;
  AlkSchemeUeDataAny  const any;
  AlkSchemeUeDataType const type;
};

typedef std::map<FString, AlkSchemeUeDataRef>
  AlkSchemeUeDataDict;

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
  AlkSchemeUeMutant   const & mutant,
  AlkSchemeUeCode     const & code,
  FString             const & callee = "",
  AlkSchemeUeDataDict const & args = AlkSchemeUeDataDict()
) -> AlkSchemeUeDataDict;

auto ALKUEMSCHEME_API
makeSchemeUeDataDict(
  std::initializer_list<AlkSchemeUeDataArg> const &
) -> AlkSchemeUeDataDict;

auto ALKUEMSCHEME_API
stringFromSchemeUeDataDict(
  AlkSchemeUeDataDict const & dict,
  FString             const & key
) -> FString;

auto ALKUEMSCHEME_API
vectorArrayFromSchemeUeDataDict(
  AlkSchemeUeDataDict const & dict,
  FString             const & key
) -> TArray<FVector>;

auto ALKUEMSCHEME_API
runCachedSchemeUeCodeAtPath(
  FString             const & path,
  FString             const & callee,
  AlkSchemeUeDataDict const & args = AlkSchemeUeDataDict(),
  bool                        forceReload = false
) -> AlkSchemeUeDataDict;
  // ^ caches and auto-loads observed file changes
