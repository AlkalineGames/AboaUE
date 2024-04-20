// Copyright 2023 - 2024 Alkaline Games, LLC.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
using UnrealBuildTool;

public class AlkUemScheme : ModuleRules {
  public AlkUemScheme(ReadOnlyTargetRules Target) : base(Target) {
    DefaultBuildSettings = BuildSettingsVersion.V2;
    IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_1;
    PrivateDependencyModuleNames.AddRange(new string[] {
      "Core",
      "CoreUObject",
      "Engine"
    });
  }
}
