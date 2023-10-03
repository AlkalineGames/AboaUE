// Copyright 2023 Alkaline Games, LLC.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "AlkUemScheme.h"

IMPLEMENT_MODULE(FAlkUemScheme, AlkUemScheme)

void FAlkUemScheme::StartupModule() {
  sessionMutant = std::make_unique<AlkSchemeUeMutant>(
    bootAlkSchemeUe());
}

void FAlkUemScheme::ShutdownModule() {
  if (sessionMutant->s7session)
    s7_free(sessionMutant->s7session);
}

auto FAlkUemScheme::runCodeAtPath(
  FString const & path,
  bool forceReload
) -> FString {
  auto codeiter = codeCacheMutant.find(path);
  if (codeiter == codeCacheMutant.end() || forceReload) {
    auto code = loadSchemeUeCode(path);
    codeCacheMutant.emplace(
      std::make_pair(path, code)); // TODO: $$$ OPTIMIZE COPIES
    return runSchemeUeCode(*sessionMutant, code);
  } else
    return runSchemeUeCode(*sessionMutant, codeiter->second);
}

static
auto accessAlkUemSchemeMutant() -> FAlkUemScheme * {
  return FModuleManager::Get().GetModulePtr<FAlkUemScheme>("AlkUemScheme");
}

auto runCachedSchemeUeCodeAtPath( // declaration in alk-scheme-ue.h
  FString const & path,
  bool forceReload
) -> FString {
  auto uem = accessAlkUemSchemeMutant();
  return uem ? uem->runCodeAtPath(path, forceReload)
             : TEXT("## Failed to access AlkUemScheme");
}
