// Copyright 2023 Alkaline Games, LLC.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
using UnrealBuildTool;

public class AlkUemScheme : ModuleRules {
  public AlkUemScheme(ReadOnlyTargetRules Target) : base(Target) {
    bLegacyPublicIncludePaths = false;
      // ^ !!! fixes VC error: command line is too long to fit in debug record
    PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
    PrivateDependencyModuleNames.AddRange(new string[] {
      "Core",
      "CoreUObject",
      "Engine"
    });
  }
}
