// Copyright 2023 Alkaline Games, LLC.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
#include "alk-scheme-ue.h"

#include "s7.h"

#include "HAL/PlatformFileManager.h"

#define ALK_TRACING 0

DECLARE_LOG_CATEGORY_EXTERN(LogAlkalineScheme, Log, All);
DEFINE_LOG_CATEGORY(LogAlkalineScheme);

auto bootAlkSchemeUe() -> AlkSchemeUeMutant {
  auto s7session = s7_init();
  if (!s7session) {
    UE_LOG(LogAlkalineScheme, Error, TEXT("Failed to init s7 Scheme"))
    return {};
  }
  FString scmPath = FPaths::ConvertRelativePathToFull(FPaths::Combine(
      FPaths::ProjectPluginsDir(),
      TEXT("AlkalineSchemeUE"), TEXT("Source"), TEXT("scm")));
#if ALK_TRACING
  UE_LOG(LogAlkalineScheme, Display,
    TEXT("BEGIN listing scm path %s"), *scmPath);
  UE_LOG(LogAlkalineScheme, Display, TEXT("%s"), *(FString(
    FPlatformFileManager::Get().GetPlatformFile().IterateDirectory(
      *scmPath,
      [] (TCHAR const * const name, bool isDir) {
        UE_LOG(LogAlkalineScheme, Display, TEXT("%s"),
          *(FString(isDir ? "<D> " : "<F> ") +
            FPaths::GetCleanFilename(name)));
        return true;
      })
    ? "..END" : "FAILED")));
#endif
  FString code;
  FString path = FPaths::Combine(scmPath, TEXT("boot.scm"));
  if (!FFileHelper::LoadFileToString(code, *path, FFileHelper::EHashOptions::None))
    UE_LOG(LogAlkalineScheme, Error, TEXT("Failed to read %s"), *path)
  else {
    auto s7obj = s7_eval_c_string(s7session, TCHAR_TO_ANSI(*code));
    UE_LOG(LogAlkalineScheme, Log, TEXT("Scheme session booted: %s"),
      ANSI_TO_TCHAR(s7_object_to_c_string(s7session, s7obj)));
  }
  return {{scmPath}, s7session};
}
