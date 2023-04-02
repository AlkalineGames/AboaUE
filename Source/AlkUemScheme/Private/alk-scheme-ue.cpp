// Copyright 2023 Alkaline Games, LLC.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
#include "alk-scheme-ue.h"

#include "s7.h"

#include "Engine/Engine.h"
#include "HAL/PlatformFileManager.h"
#include "Kismet/KismetSystemLibrary.h" // for PrintString(...)

#define ALK_TRACING 0

DECLARE_LOG_CATEGORY_EXTERN(LogAlkalineScheme, Log, All);
DEFINE_LOG_CATEGORY(LogAlkalineScheme);

static auto
first_world() -> UWorld const * {
  return !GEngine ? nullptr :
         (GEngine->GetWorldContexts().IsEmpty() ? nullptr :
          GEngine->GetWorldContexts()[0].World());
}

static auto
call_with_s7_string(
  s7_scheme* sc,
  s7_pointer args,
  char const *const name,
  void (*func)(TCHAR const *const)
) -> s7_pointer {
  auto car = s7_car(args);
  if (!s7_is_string(car))
    return s7_wrong_type_arg_error(sc, name, 1, car, "a string");
  func(ANSI_TO_TCHAR(s7_string(car)));
  return(s7_t(sc));
}

static auto
ue_log(s7_scheme* sc, s7_pointer args) -> s7_pointer {
  return call_with_s7_string(sc, args, "ue-log",
    [](TCHAR const *const text) {
      UE_LOG(LogAlkalineScheme, Display, TEXT("%s"), text);
    });
}

static auto
ue_print_string(s7_scheme* sc, s7_pointer args) -> s7_pointer {
  return call_with_s7_string(sc, args, "ue-print-string",
    [](TCHAR const *const text) {
      auto* world = first_world();
      if (world)
        UKismetSystemLibrary::PrintString(world, FString(text));
      else
        UE_LOG(LogAlkalineScheme, Error,
          TEXT("No Engine/World found to print string \"%s\""), text);
    });
}

auto bootAlkSchemeUe() -> AlkSchemeUeMutant {
  auto s7session = s7_init();
  if (!s7session) {
    UE_LOG(LogAlkalineScheme, Error, TEXT("Failed to init s7 Scheme"))
    return {};
  }
  s7_define_function(s7session,
    "ue-log", ue_log, 1, 0, false, "(ue-log string)");
  s7_define_function(s7session,
    "ue-print-string", ue_print_string, 1, 0, false, "(ue-print-string string)");

  FString scmPath = FPaths::ConvertRelativePathToFull(FPaths::Combine(
      FPaths::ProjectPluginsDir(),
      TEXT("AlkalineSchemeUE"), TEXT("Source"), TEXT("scm")));
#if ALK_TRACING
  UE_LOG(LogAlkalineScheme, Display,
    TEXT("BEGIN listing scm path %s"), *scmPath);
  UE_LOG(LogAlkalineScheme, Display, TEXT("%s"), *(FString(
    FPlatformFileManager::Get().GetPlatformFile().IterateDirectory(
      *scmPath,
      [] (TCHAR const *const name, bool isDir) {
        UE_LOG(LogAlkalineScheme, Display, TEXT("%s"),
          *(FString(isDir ? "<D> " : "<F> ") +
            FPaths::GetCleanFilename(name)));
        return true;
      })
    ? "..END" : "FAILED")));
#endif
  FString source;
  const FString path = FPaths::Combine(scmPath, TEXT("boot.scm"));
  if (!FFileHelper::LoadFileToString(source, *path, FFileHelper::EHashOptions::None))
    UE_LOG(LogAlkalineScheme, Error, TEXT("Failed to read %s"), *path)
  else {
    auto s7obj = s7_eval_c_string(s7session, TCHAR_TO_ANSI(*source));
    UE_LOG(LogAlkalineScheme, Log, TEXT("Scheme session booted: %s"),
      ANSI_TO_TCHAR(s7_object_to_c_string(s7session, s7obj)));
  }
  return {{scmPath}, s7session};
}
