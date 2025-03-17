// Copyright © 2023 - 2025 Alkaline Games, LLC.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "aboa-ue.h"

#include "aboa-ue-helper.h"
#include "any-cast-ue.h"

#include "aboa-s7.h"

#include "Components/InputComponent.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"

#include <array>
#include <stack>
#include <string>
#include <string_view>
#include <variant>

#define ALK_TRACING 0

DECLARE_LOG_CATEGORY_EXTERN(LogAlkScheme, Log, All);
DEFINE_LOG_CATEGORY(LogAlkScheme);

struct s7pointerError { s7_pointer const pointer; };
struct s7pointerValid { s7_pointer const pointer; };

static auto
scheme_arg_c_pointer_or_error(
  s7_scheme *  const s7,
  s7_pointer   const arg,
  int          const index,
  char const * const name
) -> std::variant<s7pointerValid,s7pointerError> {
  if (s7_is_c_pointer(arg))
    return s7pointerValid({arg});
  else
    return s7pointerError({s7_wrong_type_arg_error(
      s7, name, index, arg, "a C pointer")});
}

static auto
scheme_arg_procedure_or_error(
  s7_scheme *  const s7,
  s7_pointer   const arg,
  int          const index,
  char const * const name
) -> std::variant<s7pointerValid,s7pointerError> {
  if (s7_is_procedure(arg))
    return s7pointerValid({arg});
  else
    return s7pointerError({s7_wrong_type_arg_error(
      s7, name, index, arg, "a procedure")});
}

static auto
scheme_arg_string_or_error(
  s7_scheme *  const s7,
  s7_pointer   const arg,
  int          const index,
  char const * const name
) -> std::variant<char const *,s7pointerError> {
  if (s7_is_string(arg))
    return s7_string(arg);
  else
    return s7pointerError({s7_wrong_type_arg_error(
      s7, name, index, arg, "a string")});
}

static auto
scheme_arg_symbol_or_error(
  s7_scheme *  const s7,
  s7_pointer   const arg,
  int          const index,
  char const * const name
) -> std::variant<char const *,s7pointerError> {
  if (s7_is_symbol(arg))
    return s7_symbol_name(arg);
  else
    return s7pointerError({s7_wrong_type_arg_error(
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
) -> std::variant<int,s7pointerError> {
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
    return s7pointerError({s7_wrong_type_arg_error(
      s7, name, index, arg, "unmatched symbol")});
  else
    return int(std::distance(choices, result));
}

template <class T>
static auto
scheme_arg_typed_or_error(
  s7_scheme *  const s7,
  s7_pointer   const arg,
  int          const index,
  char const * const name
) -> std::variant<T const *,s7pointerError> {
  auto const argcptr = scheme_arg_c_pointer_or_error(s7, arg, index, name);
  if (argcptr.index() == 1)
    return std::get<1>(argcptr);
  auto const typed = const_cast<T const *>(
    reinterpret_cast<T*>(s7_c_pointer(std::get<0>(argcptr).pointer))); // TODO: ### YIKES!
  if (!typed)
    return s7pointerError({s7_wrong_type_arg_error(
      s7, name, index, arg, "no arg")});
  else
    return typed;
}

static auto
scheme_ue_vector(
  s7_scheme * const s7,
  FVector     const & vec
) -> s7_pointer {
  auto s7vec = s7_make_float_vector(s7, 3, 1, nullptr);
  s7_float_vector_set(s7vec, 0, vec.X);
  s7_float_vector_set(s7vec, 1, vec.Y);
  s7_float_vector_set(s7vec, 2, vec.Z);
  //UE_LOG(LogAlkScheme, Warning, TEXT("scheme_ue_vector %f %f %f"), vec.X, vec.Y, vec.Z);
  return s7vec;
}

static auto
scheme_ue_vector_array(
  s7_scheme *     const s7,
  TArray<FVector> const & uevecarray
) -> s7_pointer {
  auto s7vec = s7_make_vector(s7, uevecarray.Num());
  int i = 0;
  for (auto & uevec : uevecarray)
    s7_vector_set(s7, s7vec, i++, scheme_ue_vector(s7, uevec));
  return s7vec;
}

static auto
ue_vector_from_s7(
  s7_pointer const s7pfvec
) -> FVector {
  auto fve = s7_float_vector_elements(s7pfvec);
  return FVector(fve[0], fve[1], fve[2]);
}

static auto
alloc_ue_vector_from_s7(
  s7_pointer const s7pfvec
) -> FVector const & {
  auto fve = s7_float_vector_elements(s7pfvec);
  return *new FVector(fve[0], fve[1], fve[2]);
}

static auto
alloc_ue_vector_array_from_s7(
  s7_scheme * const s7,
  s7_pointer  const s7pvec
) -> TArray<FVector> const & {
  auto arr = new TArray<FVector>(); // TODO: @@@ PRE-ALLOCATE LENGTH
  auto len = s7_vector_length(s7pvec);
  for (int i = 0; i < len; i++)
    arr->Emplace(ue_vector_from_s7(s7_vector_ref(s7, s7pvec, i)));
  return *arr;
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

class UInputBinding : public UObject {
  DECLARE_CLASS_INTRINSIC(UInputBinding, UObject, CLASS_MatchedSerializers, TEXT("/Script/CoreUObject"))
  s7_scheme * s7          = nullptr;
  s7_pointer  mutHandler  = nullptr;
public:
  void BindAction(
    UInputComponent & inputcomp,
    const char *      action,
    EInputEvent const event,
    s7_scheme * const ins7,
    s7_pointer  const proc
  ) {
    if (ins7 && proc && s7_is_procedure(proc)) {
      s7 = ins7;
      mutHandler = proc;
      s7_gc_protect(s7, mutHandler); // TODO: @@@ PROTECTED INDEFINITELY
      inputcomp.BindAction(action, event, this, &UInputBinding::HandleAction);
#if ALK_TRACING
      UE_LOG(LogAlkScheme, Display,
        TEXT("TRACE C++ BindAction %s s7 %d handler %d"),
        event == EInputEvent::IE_Pressed ? TEXT("Pressed") : TEXT("not Pressed"),
        s7, mutHandler);
#endif
    }
  }
  void BindEventHandler(
    UInputComponent & inputcomp,
    EInputEvent const event,
    s7_scheme * const ins7,
    s7_pointer  const proc
  ) {
    if (ins7 && proc && s7_is_procedure(proc)) {
      s7 = ins7;
      mutHandler = proc;
      s7_gc_protect(s7, mutHandler); // TODO: @@@ PROTECTED INDEFINITELY
      inputcomp.BindTouch(event, this, &UInputBinding::HandleEvent);
#if ALK_TRACING
      UE_LOG(LogAlkScheme, Display,
        TEXT("TRACE C++ BindEventHandler %s s7 %d handler %d"),
        event == EInputEvent::IE_Pressed ? TEXT("Pressed") : TEXT("not Pressed"),
        s7, mutHandler);
#endif
    }
  }
  void HandleAction() {
    if (s7 && mutHandler) {
#if ALK_TRACING
    UE_LOG(LogAlkScheme, Display,
      TEXT("TRACE C++ HandleAction s7 %d handler %d"),
      s7, mutHandler);
#endif
      s7_apply_function(s7, mutHandler, s7_nil(s7));
    }
  }
  void HandleEvent(
    ETouchIndex::Type const FingerIndex,
    FVector           const Location
  ) {
    if (s7 && mutHandler) {
#if ALK_TRACING
    UE_LOG(LogAlkScheme, Display,
      TEXT("TRACE C++ HandleEvent s7 %d handler %d"),
      s7, mutHandler);
#endif
      s7_apply_function(s7, mutHandler,
        s7_cons(s7, s7_make_integer(s7, FingerIndex),
          s7_cons(s7, scheme_ue_vector(s7, Location), s7_nil(s7))));
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

static auto const name_ue_bind_input_action = "ue-bind-input-action";
static auto
ue_bind_input_action(s7_scheme * s7, s7_pointer args) -> s7_pointer {
  auto const argpawn = scheme_arg_typed_or_error<APawn>(
    s7, s7_car(args), 1, "pawn");
  if (argpawn.index() == 1)
    return std::get<1>(argpawn).pointer;
  auto const pawn = std::get<0>(argpawn);
  if (!pawn)
    return s7_f(s7); // !!! scheme_arg_typed_or_error already checks for null
  auto const argaction = scheme_arg_string_or_error(
    s7, s7_cadr(args), 2, "action");
  if (argaction.index() == 1)
    return std::get<1>(argaction).pointer;
  auto const argevent = scheme_arg_symbol_index_or_error(
    s7, s7_caddr(args), 3, "event", input_symbols.data(), input_symbols.size());
  if (argevent.index() == 1)
    return std::get<1>(argevent).pointer;
  auto const inputevent = input_events[std::get<0>(argevent)];
  auto const arghandler = scheme_arg_procedure_or_error(
    s7, s7_cadddr(args), 4, "handler");
  if (arghandler.index() == 1)
    return std::get<1>(arghandler).pointer;
  auto const handler = std::get<0>(arghandler).pointer;
  auto const inputcomp = MutPawnInputComponentOrError(
    *pawn, 0, name_ue_bind_input_action, "");
  if (!inputcomp)
    return s7_f(s7); // TODO: @@@ REPORT ERROR TO SCHEME
  auto const binding = NewObject<UInputBinding>();
  binding->BindAction(
    *inputcomp, std::get<0>(argaction), inputevent, s7, handler);
  // TODO: ### binding LEAKS FROM HERE
  return s7_t(s7);
}

static auto const name_ue_bind_input_touch = "ue-bind-input-touch";
static auto
ue_bind_input_touch(s7_scheme * s7, s7_pointer args) -> s7_pointer {
  auto const argworld = scheme_arg_typed_or_error<UWorld>(
    s7, s7_car(args), 1, "world");
  if (argworld.index() == 1)
    return std::get<1>(argworld).pointer;
  auto const world = std::get<0>(argworld);
  if (!world)
    return s7_f(s7); // !!! scheme_arg already checks for null
  auto const argevent = scheme_arg_symbol_index_or_error(
    s7, s7_cadr(args), 2, "event", input_symbols.data(), input_symbols.size());
  if (argevent.index() == 1)
    return std::get<1>(argevent).pointer;
  auto const inputevent = input_events[std::get<0>(argevent)];
  auto const arghandler = scheme_arg_procedure_or_error(
    s7, s7_caddr(args), 3, "handler");
  if (arghandler.index() == 1)
    return std::get<1>(arghandler).pointer;
  auto const handler = std::get<0>(arghandler).pointer;
  auto const inputcomp = MutPlayerInputComponentOrError(
    *world, 0, name_ue_bind_input_touch, "");
  if (!inputcomp)
    return s7_f(s7); // TODO: @@@ REPORT ERROR TO SCHEME
  auto const binding = NewObject<UInputBinding>();
  binding->BindEventHandler(*inputcomp, inputevent, s7, handler);
  // TODO: ### binding LEAKS FROM HERE
  return s7_t(s7);
}

#if 0
static auto
ue_apply_procedure_on_world(
  s7_scheme * const s7,
  s7_pointer  const proc,
  UWorld & mutWorld
) -> void {
#if ALK_TRACING
  UE_LOG(LogAlkScheme, Display,
    TEXT("TRACE C++ ue_apply_procedure_on_world \"%s\""),
    *mutWorld.OriginalWorldName.ToString());
#endif
  s7_apply_function(s7, proc,
    s7_cons(s7, s7_make_c_pointer(s7, &mutWorld), s7_nil(s7)));
}

static auto const name_ue_hook_on_world_added = "ue-hook-on-world-added";
static auto
ue_hook_on_world_added(s7_scheme * s7, s7_pointer args) -> s7_pointer {
  auto const arghandler = scheme_arg_procedure_or_error(
    s7, s7_car(args), 1, "handler");
  if (arghandler.index() == 1)
    return std::get<1>(arghandler).pointer;
  auto const handler = std::get<0>(arghandler).pointer;
  auto * mutEngine = MutEngineOrError(name_ue_hook_on_world_added, "");
  if (!mutEngine)
    return s7_f(s7); // TODO: @@@ REPORT ERROR TO SCHEME
  s7_gc_protect(s7, handler); // TODO: @@@ PROTECTED INDEFINITELY
#if 0 // !!! OnWorldAdded() never gets called except in one case which is useless
  mutEngine->OnWorldAdded().AddLambda(
    [s7, handler](UWorld * mutWorld) {
      if (mutWorld)
        ue_apply_procedure_on_world(s7, handler, *mutWorld);
    });
#endif
  auto const lambda = [s7, handler]() {
    ApplyLambdaOnAllWorlds(
      [s7, handler](UWorld & mutWorld) {
        ue_apply_procedure_on_world(s7, handler, mutWorld);
      });
  };
  if (GUnrealEd) // !!! only way to be notified of new worlds
    GUnrealEd->OnViewportClientListChanged().AddLambda(lambda);
  lambda();
  return s7_t(s7);
}
#endif

static auto const name_ue_hook_on_world_begin_play = "ue-hook-on-world-begin-play";
static auto
ue_hook_on_world_begin_play(s7_scheme * s7, s7_pointer args) -> s7_pointer {
  auto const arghandler = scheme_arg_procedure_or_error(
    s7, s7_car(args), 1, "handler");
  if (arghandler.index() == 1)
    return std::get<1>(arghandler).pointer;
  auto const handler = std::get<0>(arghandler).pointer;
  s7_gc_protect(s7, handler); // TODO: @@@ PROTECTED INDEFINITELY
#if ALK_TRACING
  UE_LOG(LogAlkScheme, Display, TEXT("TRACE C++ %s"),
    ANSI_TO_TCHAR(name_ue_hook_on_world_begin_play));
#endif
  FWorldDelegates::OnWorldInitializedActors.AddLambda(
    [s7, handler](const UWorld::FActorsInitializedParams & params) {
#if ALK_TRACING
      UE_LOG(LogAlkScheme, Display, TEXT("TRACE C++ on world actors initialized"));
#endif
      auto const world = params.World;
      if (world)
        world->OnWorldBeginPlay.AddLambda([s7, handler, world]() {
          s7_apply_function(s7, handler,
            s7_cons(s7, s7_make_c_pointer(s7, world), s7_nil(s7)));
        });
    });
  ApplyLambdaOnAllWorlds([s7, handler](UWorld & mutWorld) {
    if (mutWorld.HasBegunPlay())
      s7_apply_function(s7, handler,
        s7_cons(s7, s7_make_c_pointer(s7, &mutWorld), s7_nil(s7)));
  });
  return s7_t(s7);
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
  auto const argworld = scheme_arg_typed_or_error<UWorld>(
    s7, s7_car(args), 1, "world");
  if (argworld.index() == 1)
    return std::get<1>(argworld).pointer;
  auto const world = std::get<0>(argworld);
  auto const argstring = scheme_arg_string_or_error(
    s7, s7_cadr(args), 2, "string");
  if (argstring.index() == 1)
    return std::get<1>(argstring).pointer;
  PrintStringToScreen(
    FString(ANSI_TO_TCHAR(std::get<0>(argstring))),
    world);
  return s7_t(s7);
}

static auto const name_ue_print_string_primary = "ue-print-string-primary";
static auto
ue_print_string_primary(s7_scheme * s7, s7_pointer args) -> s7_pointer {
  auto const argstring = scheme_arg_string_or_error(
    s7, s7_car(args), 1, "string");
  if (argstring.index() == 1)
    return std::get<1>(argstring).pointer;
  PrintStringToScreen(
    FString(ANSI_TO_TCHAR(std::get<0>(argstring))));
  return s7_t(s7);
}

static auto function_help_string(
  char const * const name,
  char const * const args
) -> std::string {
  return std::string("(") + name + args + ")";
}

auto bootAboaUe() -> AboaUeMutant {
  auto s7session = s7_init();
  if (!s7session) {
    UE_LOG(LogAlkScheme, Error, TEXT("Failed to init s7 Scheme"))
    return {};
  }
  s7_define_function(s7session,
    name_ue_bind_input_action, ue_bind_input_action, 4, 0, false,
    function_help_string(name_ue_bind_input_touch, " input action handler").c_str());
  s7_define_function(s7session,
    name_ue_bind_input_touch, ue_bind_input_touch, 3, 0, false,
    function_help_string(name_ue_bind_input_touch, " input touch handler").c_str());
  s7_define_function(s7session,
    name_ue_hook_on_world_begin_play, ue_hook_on_world_begin_play, 1, 0, false,
    function_help_string(name_ue_hook_on_world_begin_play, " handler").c_str());
  s7_define_function(s7session,
    name_ue_log, ue_log, 1, 0, false,
    function_help_string(name_ue_log, " string").c_str());
  s7_define_function(s7session,
    name_ue_print_string, ue_print_string, 2, 0, false,
    function_help_string(name_ue_print_string, " world string)").c_str());
  s7_define_function(s7session,
    name_ue_print_string_primary, ue_print_string_primary, 1, 0, false,
    function_help_string(name_ue_print_string_primary, " string)").c_str());

  FString const scmPath = PluginSubpath(
    ANSI_TO_TCHAR("AboaUE"),
    ANSI_TO_TCHAR("Source/aboa"));
#if ALK_TRACING
  UE_LOG(LogAlkScheme, Display,
    TEXT("BEGIN listing scm path %s"), *scmPath);
  const FString result =
    FPlatformFileManager::Get().GetPlatformFile().IterateDirectory(
      *scmPath,
      [] (TCHAR const *const name, bool isDir) {
        UE_LOG(LogAlkScheme, Display, TEXT("%s"),
          *(FString(isDir ? "<D> " : "<F> ") +
            FPaths::GetCleanFilename(name)));
        return true;
      })
    ? "..END" : "FAILED";
  UE_LOG(LogAlkScheme, Display, TEXT("%s"), *result);
#endif
  AboaUeMutant const mutant = {{scmPath}, s7session};
  auto const code = loadAboaUeCode(
    FPaths::Combine(scmPath, TEXT("boot.aboa")));
  if (!code.source.IsEmpty()) {
    auto result = runAboaUeCode(mutant, code);
    UE_LOG(LogAlkScheme, Log, TEXT("Scheme session booted: %s"),
      *stringFromAboaUeDataDict(result, "result")
    );
  }
  return mutant;
}

auto loadAboaUeCode(FString const &path) -> AboaUeCode {
  FString mutSource;
  if (!FFileHelper::LoadFileToString(mutSource, *path, FFileHelper::EHashOptions::None))
    UE_LOG(LogAlkScheme, Error, TEXT("Failed to read %s"), *path)
  return {path, mutSource};
}

auto runAboaUeCode(
  AboaUeMutant    const & mutant,
  AboaUeCode      const & code,
  FString         const & callee,
  AboaUeDataDict  const & args
) -> AboaUeDataDict {
  bool const willCall = !callee.IsEmpty();
  std::string mutCallExpr = "(";
  mutCallExpr += TCHAR_TO_ANSI(*callee);
  std::stack<s7_int> mutProtectStack;
  for (auto & arg : args) {
    auto & key = arg.first;
    auto & ref = arg.second;
    s7_pointer s7value = s7_nil(mutant.s7session);
    switch (ref.type) {
      case AboaUeDataType::Bool : {
        // TODO: ### IMPLEMENT TYPE
        break;
      }
      case AboaUeDataType::String : {
        // TODO: ### IMPLEMENT TYPE
        break;
      }
      case AboaUeDataType::Uobject : {
        auto op = ueObjectPtrFromAny(ref.any);
        if (!op) UE_LOG(LogAlkScheme, Error,
          TEXT("runAboaUeCode(...) arg type is not a Uobject"))
        else
          s7value = s7_make_c_pointer(
            mutant.s7session, const_cast<UObject *>(op));
        break;
      }
      case AboaUeDataType::Vector : {
        auto vp = ueVectorPtrFromAny(ref.any);
        if (!vp) UE_LOG(LogAlkScheme, Error,
          TEXT("runAboaUeCode(...) arg type is not Vector"));
        s7value = scheme_ue_vector(
          mutant.s7session, vp ? *vp : FVector());
        break;
      }
      case AboaUeDataType::VectorArray : {
        auto vap = ueVectorArrayPtrFromAny(ref.any);
        if (!vap) UE_LOG(LogAlkScheme, Error,
          TEXT("runAboaUeCode(...) arg type is not VectorArray"));
        s7value = scheme_ue_vector_array(
          mutant.s7session, vap ? *vap : TArray<FVector>());
        break;
      }
    }
    //UE_LOG(LogAlkScheme, Error, TEXT("s7_define_variable %s %d"), *arg.first, &ref.any)
    auto argName = std::string(willCall ? "arg--" : "")
                 + TCHAR_TO_ANSI(*key);
    if (willCall) {
      mutCallExpr += " " ;
      mutCallExpr += argName;
    }
    mutProtectStack.push(
      s7_gc_protect(mutant.s7session,
        s7_define_constant(mutant.s7session, argName.c_str(), s7value)));
  }
  auto s7obj = s7_eval_c_string(
    mutant.s7session, TCHAR_TO_ANSI(*code.source));
  if (willCall) {
    mutCallExpr += ')';
    s7obj = s7_eval_c_string(
      mutant.s7session, mutCallExpr.c_str());
  }
  auto ref =
    s7_is_float_vector(s7obj)
    ? makeAboaUeDataVector( // TODO: ### ALLOCATED
        alloc_ue_vector_from_s7(s7obj))
    : (s7_is_vector(s7obj)
       && (s7_vector_length(s7obj) > 0)
       && s7_is_float_vector(s7_vector_elements(s7obj)[0]))
      ? makeAboaUeDataVectorArray( // TODO: ### ALLOCATED
          alloc_ue_vector_array_from_s7(mutant.s7session, s7obj))
      // ### TODO HANDLE OTHER TYPES
      : makeAboaUeDataString(
          *new FString(ANSI_TO_TCHAR(
            s7_object_to_c_string(mutant.s7session, s7obj))));
  auto result = makeAboaUeDataDict({{"result", ref}});
  while (!mutProtectStack.empty()) {
    s7_gc_unprotect_at(mutant.s7session, mutProtectStack.top());
    mutProtectStack.pop();
  }
  return result;
}

auto makeAboaUeDataDict(
  std::initializer_list<AboaUeDataArg> const & args
) -> AboaUeDataDict {
  auto dict = AboaUeDataDict();
  for (auto & arg : args)
    dict.emplace(std::make_pair(arg.name, arg.ref));
  return dict;
}

auto makeAboaUeDataString(FString const & data) -> AboaUeDataRef {
  return {&data, AboaUeDataType::String};
}

auto makeAboaUeDataVector(FVector const & data) -> AboaUeDataRef {
  return {&data, AboaUeDataType::Vector};
}

auto makeAboaUeDataUobject(UObject const & data) -> AboaUeDataRef {
  return {&data, AboaUeDataType::Uobject};
}

auto makeAboaUeDataVectorArray(TArray<FVector> const & data) -> AboaUeDataRef {
  return {&data, AboaUeDataType::VectorArray};
}

static void logErrorMap(
  char const * const   errorText,
  char const * const   callerName,
  FString      const & key
) {
  UE_LOG(LogAlkScheme, Error,
    TEXT("%s %s(map, \"%s\")"),
    ANSI_TO_TCHAR(errorText),
    ANSI_TO_TCHAR(callerName),
    *key);
}

static auto schemeUeDataRefInDict(
  char const *    const   callerName,
  AboaUeDataDict  const & dict,
  FString         const & key,
  AboaUeDataType          type
) -> AboaUeDataRef const * {
  auto iter = dict.find(key);
  if (iter == dict.end())
    logErrorMap("Failed to find", callerName, key);
  else if (iter->second.type != type)
    logErrorMap("Wrong type for", callerName, key);
  else
    return &iter->second;
  return nullptr;
}

auto stringFromAboaUeDataDict(
  AboaUeDataDict  const & dict,
  FString         const & key
) -> FString {
  auto refOrNull = schemeUeDataRefInDict(
    "stringFromAboaUeDataDict", dict, key,
    AboaUeDataType::String);
  if (refOrNull) {
    //UE_LOG(LogAlkScheme, Warning,
    //  TEXT("stringFromAboaUeDataDict ref->any has_value=%s, type=%s"),
    //  ANSI_TO_TCHAR(refOrNull->any.has_value() ? "true " : "false"),
    //  ANSI_TO_TCHAR(refOrNull->any.type().name()));
    auto sp = ueStringPtrFromAny(refOrNull->any);
    if (sp)
      return *sp;
    else
      UE_LOG(LogAlkScheme, Error,
        TEXT("stringFromAboaUeDataDict(...) arg type is not String"));
  }
  return FString();
}

auto vectorArrayFromAboaUeDataDict(
  AboaUeDataDict const & dict,
  FString             const & key
) -> TArray<FVector> {
  auto refOrNull = schemeUeDataRefInDict(
    "vectorArrayFromAboaUeDataDict", dict, key,
    AboaUeDataType::VectorArray);
  if (refOrNull) {
    auto vap = ueVectorArrayPtrFromAny(refOrNull->any);
    if (vap)
      return *vap;
    else
      UE_LOG(LogAlkScheme, Error,
        TEXT("vectorArrayFromAboaUeDataDict(...) arg type is not VectorArray"));
  }
  return TArray<FVector>();
}
