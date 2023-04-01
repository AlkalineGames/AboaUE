// Copyright 2023 Alkaline Games, LLC.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
#include "alk-scheme-ue.h"

#include "s7.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAlkalineScheme, Log, All);
DEFINE_LOG_CATEGORY(LogAlkalineScheme);

class DirectoryVisitor : public IPlatformFile::FDirectoryVisitor {
public:
  bool Visit(TCHAR const * const name, bool isDirectory) override {
    // TODO: @@@ UE_LOG DOES NOT TAKE A VARIABLE Format
    //auto label = isDirectory ? TEXT(" dir: %s") : TEXT("file: %s");
    if (isDirectory)
         UE_LOG(LogAlkalineScheme, Warning, TEXT(" dir: %s"), name)
    else UE_LOG(LogAlkalineScheme, Warning, TEXT("file: %s"), name);
    return true;
  }
};

auto bootAlkSchemeUe() -> AlkSchemeUeMutant {
  auto s7session = s7_init();
  if (!s7session) {
    UE_LOG(LogAlkalineScheme, Error, TEXT("Failed to init s7 Scheme"))
    return {};
  }
  FString scmPath = FPaths::ConvertRelativePathToFull(FPaths::Combine(
      FPaths::ProjectPluginsDir(),
      TEXT("AlkalineSchemeUE"), TEXT("Source"), TEXT("scm")));
#if 0
  auto& platformFile = FPlatformFileManager::Get().GetPlatformFile();
  DirectoryVisitor dirVisitor;
  UE_LOG(LogAlkalineScheme, Warning, TEXT("BEGIN listing path %s"), *scmPath);
  if (platformFile.IterateDirectory(*scmPath, dirVisitor))
       UE_LOG(LogAlkalineScheme, Warning, TEXT("..END"))
  else UE_LOG(LogAlkalineScheme, Warning, TEXT("FAILED"));
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
