// Copyright 2023 Alkaline Games, LLC.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
#include "AlkUemScheme.h"

IMPLEMENT_MODULE(FAlkUemScheme, AlkUemScheme)

void
FAlkUemScheme::StartupModule() {
  alkSchemeUeMutant = std::make_unique<AlkSchemeUeMutant>(
    bootAlkSchemeUe());
}

void
FAlkUemScheme::ShutdownModule() {
  if (alkSchemeUeMutant->s7session)
    s7_free(alkSchemeUeMutant->s7session);
}
