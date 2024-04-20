// Copyright 2024 Alkaline Games, LLC.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "any-cast-ue.h"

// !!! NOTE:
// We cannot use the exception throwing overloads of std::any_cast
// because packaging for Windows does not compile with exceptions
// and this causes a linking collision with the use of boost::std_any
// by Engine/Source/ThirdParty/OpenVDB/openvdb-8.1.0/openvdb/openvdb/io/Archive.cc
// that does not match the actual std::bad_cast definition, so we get this error:

// LINK : warning LNK4217: symbol '??1bad_cast@std@@UEAA@XZ (public: virtual __cdecl std::bad_cast::~bad_cast(void))' defined in 'libopenvdb.lib(Archive.obj)' is imported by 'Module.AlkUemScheme.cpp.obj' in function '"public: virtual void * __cdecl std::bad_any_cast::`scalar deleting destructor'(unsigned int)" (??_Gbad_any_cast@std@@UEAAPEAXI@Z)'
// Module.AlkUemScheme.cpp.obj : error LNK2019: unresolved external symbol "__declspec(dllimport) public: __cdecl std::bad_cast::bad_cast(char const *)" (__imp_??0bad_cast@std@@QEAA@PEBD@Z) referenced in function "public: __cdecl std::bad_any_cast::bad_any_cast(void)" (??0bad_any_cast@std@@QEAA@XZ)
// Module.AlkUemScheme.cpp.obj : error LNK2001: unresolved external symbol "protected: virtual void __cdecl std::bad_cast::_Doraise(void)const " (?_Doraise@bad_cast@std@@MEBAXXZ)

template <class T> auto fromAny(std::any a) -> T {
  //return std::any_cast<T>(any);
  auto z = reinterpret_cast<const T*>(std::any_cast<T>(&a));
  return z ? T(*z) : T();
}

auto ueStringFromAny(std::any a) -> FString {
  return fromAny<FString>(a);
}

auto ueVectorFromAny(std::any a) -> FVector {
  return fromAny<FVector>(a);
}

auto ueVectorArrayFromAny(std::any a) -> TArray<FVector> {
  return fromAny<TArray<FVector> >(a);
}
