// Copyright © 2023 - 2025 Christopher Augustus
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
#pragma once

#include "aboa-ue.h"

#include <memory>

#include <Modules/ModuleInterface.h>

class FAboaUem : public IModuleInterface {
public:
  virtual void StartupModule()  override;
  virtual void ShutdownModule() override;

  auto callCode(
    FString         const & callee = "",
    AboaUeDataDict  const & args = AboaUeDataDict()
  ) -> AboaUeDataDict;

  auto runCodeAtPath(
    FString         const & path,
    FString         const & callee = "",
    AboaUeDataDict  const & args = AboaUeDataDict(),
    bool                    forceReload = false
  ) -> AboaUeDataDict;
    // ^ caches and auto-loads observed file changes

private:
  std::unique_ptr<AboaUeMutant> sessionMutant;
  std::map<FString, AboaUeCode> codeCacheMutant;
};
