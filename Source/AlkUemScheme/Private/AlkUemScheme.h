// Copyright 2023 Alkaline Games, LLC.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
#pragma once

#include "alk-scheme-ue.h"

#include <Modules/ModuleInterface.h>

#include <memory>

class FAlkUemScheme : public IModuleInterface {
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
private:
    std::unique_ptr<AlkSchemeUeMutant> alkSchemeUeMutant;
};
