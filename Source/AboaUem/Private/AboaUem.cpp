// Copyright © 2023 - 2025 Christopher Augustus
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "AboaUem.h"

IMPLEMENT_MODULE(FAboaUem, AboaUem)

void FAboaUem::StartupModule() {
  sessionMutant = std::make_unique<AboaUeMutant>(
    bootAboaUe());
}

void FAboaUem::ShutdownModule() {
  if (sessionMutant->s7session)
    s7_free(sessionMutant->s7session);
}

auto FAboaUem::callCode( // declaration in aboa-ue.h
  FString         const & callee,
  AboaUeDataDict  const & args
) -> AboaUeDataDict {
  return callAboaUeCode(*sessionMutant, callee, args);
}

auto FAboaUem::runCodeAtPath(
  FString         const & path,
  FString         const & callee,
  AboaUeDataDict  const & args,
  bool                    forceReload
) -> AboaUeDataDict {
  auto codeiter = codeCacheMutant.find(path);
  if (codeiter == codeCacheMutant.end() || forceReload) {
    auto code = loadAboaUeCode(path);
    if (codeiter != codeCacheMutant.end())
      codeCacheMutant.erase(codeiter);
    codeCacheMutant.emplace(
      std::make_pair(path, code)); // TODO: $$$ OPTIMIZE COPIES
    return runAboaUeCode(*sessionMutant, code, callee, args);
  } else
    return runAboaUeCode(*sessionMutant, codeiter->second, callee, args);
}

static
auto accessAboaUemMutant() -> FAboaUem * {
  return FModuleManager::Get().GetModulePtr<FAboaUem>("AboaUem");
}

auto callLoadedAboaUeCode( // declaration in aboa-ue.h
  FString         const & callee,
  AboaUeDataDict  const & args
) -> AboaUeDataDict {
  auto uem = accessAboaUemMutant();
  return uem ? uem->callCode(callee, args)
             : // TODO: TEXT("## Failed to access AboaUem");
               AboaUeDataDict();
}

auto runCachedAboaUeCodeAtPath( // declaration in aboa-ue.h
  FString         const & path,
  FString         const & callee,
  AboaUeDataDict  const & args,
  bool                    forceReload
) -> AboaUeDataDict {
  auto uem = accessAboaUemMutant();
  return uem ? uem->runCodeAtPath(path, callee, args, forceReload)
             : // TODO: TEXT("## Failed to access AboaUem");
               AboaUeDataDict();
}
