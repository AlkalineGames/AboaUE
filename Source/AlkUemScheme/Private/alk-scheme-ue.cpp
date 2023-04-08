// Copyright 2023 Alkaline Games, LLC.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "alk-scheme-ue.h"

#include "alk-ue-helper.h"

#include "s7.h"

#include "HAL/PlatformFileManager.h"
#include "Kismet/KismetSystemLibrary.h" // for PrintString(...)

#include <array>
#include <string_view>
#include <variant>

#define ALK_TRACING 0

DECLARE_LOG_CATEGORY_EXTERN(LogAlkScheme, Log, All);
DEFINE_LOG_CATEGORY(LogAlkScheme);

struct ErrorPointer     { s7_pointer const pointer; };
struct ProcedurePointer { s7_pointer const pointer; };

static auto
scheme_arg_c_pointer_or_error(
  s7_scheme *  const s7,
  s7_pointer   const arg,
  int          const index,
  char const * const name
) -> std::variant<ProcedurePointer,ErrorPointer> {
  if (s7_is_c_pointer(arg))
    return ProcedurePointer({arg});
  else
    return ErrorPointer({s7_wrong_type_arg_error(
      s7, name, index, arg, "a C pointer")});
}

static auto
scheme_arg_procedure_or_error(
  s7_scheme *  const s7,
  s7_pointer   const arg,
  int          const index,
  char const * const name
) -> std::variant<ProcedurePointer,ErrorPointer> {
  if (s7_is_procedure(arg))
    return ProcedurePointer({arg});
  else
    return ErrorPointer({s7_wrong_type_arg_error(
      s7, name, index, arg, "a procedure")});
}

static auto
scheme_arg_string_or_error(
  s7_scheme *  const s7,
  s7_pointer   const arg,
  int          const index,
  char const * const name
) -> std::variant<char const *,ErrorPointer> {
  if (s7_is_string(arg))
    return s7_string(arg);
  else
    return ErrorPointer({s7_wrong_type_arg_error(
      s7, name, index, arg, "a string")});
}

static auto
scheme_arg_symbol_or_error(
  s7_scheme *  const s7,
  s7_pointer   const arg,
  int          const index,
  char const * const name
) -> std::variant<char const *,ErrorPointer> {
  if (s7_is_symbol(arg))
    return s7_symbol_name(arg);
  else
    return ErrorPointer({s7_wrong_type_arg_error(
      s7, name, index, arg, "a symbol")});
}

static auto
scheme_arg_symbol_index_or_error(
  s7_scheme *       const s7,
  s7_pointer        const arg,
  int               const index,
  char const *      const name,
  std::string_view  const choices[], // !!! cannot use std::array
  int               const choice_count
) -> std::variant<int,ErrorPointer> {
  auto const symbol = scheme_arg_symbol_or_error(s7, arg, index, name);
  if (symbol.index() == 1)
    return std::get<1>(symbol);
  auto const chars = std::get<0>(symbol);
#if 0 // TODO: @@@ MSVC FAILS TO COMPILE THIS VALID CODE
  auto const result = std::find(
    choices, choices + choice_count,
    [&chars](std::string_view const & item) {
      return item.compare(chars) == 0;
    });
#else
  int mutI;
  for (mutI = 0; mutI < choice_count; mutI++)
     if (choices[mutI].compare(chars) == 0)
        break;
  auto const result = choices + mutI;
#endif
  if (result == choices + choice_count)
    return ErrorPointer({s7_wrong_type_arg_error(
      s7, name, index, arg, "unmatched symbol")});
  else
    return std::distance(choices, result);
  return 0;
}

static auto
call_lambda_with_s7_string(
  s7_scheme *  const s7,
  s7_pointer   const args,
  char const * const name,
  void (* const lambda)(TCHAR const * const)
) -> s7_pointer {
  auto const arg = scheme_arg_string_or_error(s7, s7_car(args), 1, name);
  if (arg.index() == 1)
    return std::get<1>(arg).pointer;
  lambda(ANSI_TO_TCHAR(std::get<0>(arg)));
  return s7_t(s7);
}

static class UInputBinding : public UObject {
  DECLARE_CLASS_INTRINSIC(UInputBinding, UObject, CLASS_MatchedSerializers, TEXT("/Script/CoreUObject"))
  s7_pointer mutHandler;
public:
  void BindEventHandler(
    UInputComponent & inputcomp,
    EInputEvent const event,
    s7_pointer  const proc
  ) {
    if (s7_is_procedure(proc)) {
      mutHandler = proc;
      inputcomp.BindTouch(event, this, &UInputBinding::HandleEvent);
    }
  }
  void HandleEvent(
    ETouchIndex::Type const FingerIndex,
    FVector           const Location
  ) {
    if (s7_is_procedure(mutHandler)) {
      // TODO: @@@ IMPLEMENT
      //call_s7_proc(procs.pressed,  FingerIndex, Location);
    }
  }
};
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UInputBinding)

static std::array input_symbols {
  std::string_view { "pressed" },
  std::string_view { "released" },
  std::string_view { "repeated" }
};
static std::array input_events {
  EInputEvent::IE_Pressed,
  EInputEvent::IE_Released,
  EInputEvent::IE_Repeat
};

static auto const name_ue_bind_input_touch = "ue-bind-input-touch";
static auto
ue_bind_input_touch(s7_scheme * s7, s7_pointer args) -> s7_pointer {
  auto const eventindex = scheme_arg_symbol_index_or_error(
    s7, s7_car(args), 1, "event", input_symbols.data(), input_symbols.size());
  if (eventindex.index() == 1)
    return std::get<1>(eventindex).pointer;
  auto const inputevent = input_events[std::get<0>(eventindex)];
  auto const inputcomp = MutFirstInputComponentOrError(
    name_ue_bind_input_touch, "");
  if (!inputcomp)
    return s7_nil(s7); // TODO: @@@ RETURN ERROR INDICATOR
  auto const binding = NewObject<UInputBinding>();
  binding->BindEventHandler(*inputcomp, inputevent, {s7_cadr(args)});
  // TODO: FROM HERE @@@ binding LEAKS FROM HERE
  return s7_nil(s7);
}
#if 0
static class USchemeInvoker : public UObject {
  DECLARE_CLASS_INTRINSIC(USchemeInvoker, UObject, CLASS_MatchedSerializers, TEXT("/Script/CoreUObject"))
  s7_scheme * scheme   = nullptr;
  s7_pointer procedure = nullptr;
public:
  void Bind(s7_scheme * const s7, s7_pointer const proc) {
    if (s7_is_procedure(proc)) {
      scheme = s7;
      procedure = proc;
    }
  }
  UFUNCTION()
  void Invoke() const {
    if (s7_is_procedure(procedure))
      s7_apply_function(scheme, procedure, s7_nil(scheme));
  }
};
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(USchemeInvoker)
#endif

static auto
ue_apply_procedure_on_world(
  s7_scheme * s7,
  s7_pointer proc,
  UWorld & mutWorld
) -> void {
  UE_LOG(LogAlkScheme, Display,
    TEXT("TRACE C++ ue_apply_procedure_on_world \"%s\""),
    *mutWorld.OriginalWorldName.ToString());
  s7_apply_function(s7, proc,
    s7_cons(s7, s7_make_c_pointer(s7, &mutWorld), s7_nil(s7)));
}

static auto const name_ue_hook_on_world_added = "ue-hook-on-world-added";
static auto
ue_hook_on_world_added(s7_scheme * s7, s7_pointer args) -> s7_pointer {
  auto const arg = scheme_arg_procedure_or_error(s7, s7_car(args), 1, "handler");
  if (arg.index() == 1)
    return std::get<1>(arg).pointer;
  auto const handler = std::get<0>(arg).pointer;
  auto * mutEngine = MutEngineOrError(name_ue_hook_on_world_added, "");
  if (mutEngine) {
    mutEngine->OnWorldAdded().AddLambda(
      [s7, handler](UWorld * mutWorld) {
        if (mutWorld)
          ue_apply_procedure_on_world(s7, handler, *mutWorld);
      });
    ApplyLambdaOnWorlds(
      [s7, handler](UWorld & mutWorld) {
        ue_apply_procedure_on_world(s7, handler, mutWorld);
      });
    return s7_t(s7);
  }
  else
    return s7_f(s7);
}

static auto const name_ue_hook_on_world_begin_play = "ue-hook-on-world-begin-play";
static auto
ue_hook_on_world_begin_play(s7_scheme * s7, s7_pointer args) -> s7_pointer {
  auto const arg1 = scheme_arg_c_pointer_or_error(s7, s7_car(args), 1, "world");
  if (arg1.index() == 1)
    return std::get<1>(arg1).pointer;
  auto const arg1pointer = std::get<0>(arg1).pointer;
  auto const cpointer = s7_c_pointer(arg1pointer);
  auto const arg2 = scheme_arg_procedure_or_error(s7, s7_cadr(args), 2, "handler");
  if (arg2.index() == 1)
    return std::get<1>(arg2).pointer;
  auto const handler = std::get<0>(arg2).pointer;
  auto const mutWorld = reinterpret_cast<UWorld*>(cpointer); // TODO: @@@ YIKES!
  if (mutWorld) {
    //auto const invoker = NewObject<USchemeInvoker>();
    //invoker->Bind(s7, handler);
    // TODO: @@@ DEAL WITH LIFETIME OF handler
    auto const lambda = [s7, arg1pointer, handler]() {
      UE_LOG(LogAlkScheme, Display, TEXT("TRACE C++ OnWorldBeginPlay"));
      s7_apply_function(s7, handler, s7_cons(s7, arg1pointer, s7_nil(s7)));
    };
    if (mutWorld->HasBegunPlay())
      lambda();
    else
      mutWorld->OnWorldBeginPlay.AddLambda(lambda);
    //world->OnWorldBeginPlay.AddUObject(invoker, &USchemeInvoker::Invoke);
    UE_LOG(LogAlkScheme, Display, TEXT("TRACE C++ ue_hook_on_world_begin_play"));
    return s7_t(s7);
  }
  else
    return s7_f(s7);
}

static auto const name_ue_log = "ue-log";
static auto
ue_log(s7_scheme * s7, s7_pointer args) -> s7_pointer {
  return call_lambda_with_s7_string(s7, args, name_ue_log,
    [](TCHAR const * const text) {
      UE_LOG(LogAlkScheme, Display, TEXT("%s"), text);
    });
}

static auto const name_ue_print_string = "ue-print-string";
static auto
ue_print_string(s7_scheme * s7, s7_pointer args) -> s7_pointer {
  return call_lambda_with_s7_string(s7, args, name_ue_print_string,
    [](TCHAR const * const text) {
      const auto * const world = PrimaryWorldOrError(
        name_ue_print_string, TCHAR_TO_ANSI(text));
      if (world) {
        UE_LOG(LogAlkScheme, Display, TEXT("TRACE C++ %s world %s \"%s\""),
          ANSI_TO_TCHAR(name_ue_print_string),
          *(world->OriginalWorldName.ToString()),
          text);
        UKismetSystemLibrary::PrintString(world, FString(text));
      }
    });
}

auto function_help_string(
  char const * const name,
  char const * const args
) -> std::string {
    return std::string("(") + name + args + ")";
}

auto bootAlkSchemeUe() -> AlkSchemeUeMutant {
  auto s7session = s7_init();
  if (!s7session) {
    UE_LOG(LogAlkScheme, Error, TEXT("Failed to init s7 Scheme"))
    return {};
  }
  s7_define_function(s7session,
    name_ue_bind_input_touch, ue_bind_input_touch, 2, 0, false,
    function_help_string(name_ue_bind_input_touch, " event handler").c_str());
  s7_define_function(s7session,
    name_ue_hook_on_world_added, ue_hook_on_world_added, 1, 0, false,
    function_help_string(name_ue_hook_on_world_added, " handler").c_str());
  s7_define_function(s7session,
    name_ue_hook_on_world_begin_play, ue_hook_on_world_begin_play, 2, 0, false,
    function_help_string(name_ue_hook_on_world_begin_play, " world handler").c_str());
  s7_define_function(s7session,
    name_ue_log, ue_log, 1, 0, false,
    function_help_string(name_ue_log, " string").c_str());
  s7_define_function(s7session,
    name_ue_print_string, ue_print_string, 1, 0, false,
    function_help_string(name_ue_print_string, " string)").c_str());

  FString const scmPath = FPaths::ConvertRelativePathToFull(FPaths::Combine(
      FPaths::ProjectPluginsDir(),
      TEXT("AlkalineSchemeUE"), TEXT("Source"), TEXT("scm")));
#if ALK_TRACING
  UE_LOG(LogAlkScheme, Display,
    TEXT("BEGIN listing scm path %s"), *scmPath);
  UE_LOG(LogAlkScheme, Display, TEXT("%s"), *(FString(
    FPlatformFileManager::Get().GetPlatformFile().IterateDirectory(
      *scmPath,
      [] (TCHAR const *const name, bool isDir) {
        UE_LOG(LogAlkScheme, Display, TEXT("%s"),
          *(FString(isDir ? "<D> " : "<F> ") +
            FPaths::GetCleanFilename(name)));
        return true;
      })
    ? "..END" : "FAILED")));
#endif
  FString mutSource;
  FString const path = FPaths::Combine(scmPath, TEXT("boot.scm"));
  if (!FFileHelper::LoadFileToString(mutSource, *path, FFileHelper::EHashOptions::None))
    UE_LOG(LogAlkScheme, Error, TEXT("Failed to read %s"), *path)
  else {
    auto s7obj = s7_eval_c_string(s7session, TCHAR_TO_ANSI(*mutSource));
    UE_LOG(LogAlkScheme, Log, TEXT("Scheme session booted: %s"),
      ANSI_TO_TCHAR(s7_object_to_c_string(s7session, s7obj)));
  }
  return {{scmPath}, s7session};
}
