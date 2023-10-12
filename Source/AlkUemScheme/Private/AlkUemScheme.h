// Copyright 2023 Alkaline Games, LLC.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
#pragma once

#include "alk-scheme-ue.h"

#include <Modules/ModuleInterface.h>

#include <map>
#include <memory>

class FAlkUemScheme : public IModuleInterface {
public:
  virtual void StartupModule() override;
  virtual void ShutdownModule() override;

  auto runCodeAtPath(
    FString             const & path,
    FString             const & callee = "",
    AlkSchemeUeDataDict const & args = AlkSchemeUeDataDict(),
    bool                        forceReload = false
  ) -> AlkSchemeUeDataDict;
    // ^ caches and auto-loads observed file changes

private:
  std::unique_ptr<AlkSchemeUeMutant> sessionMutant;
  std::map<FString, AlkSchemeUeCode> codeCacheMutant;
};
