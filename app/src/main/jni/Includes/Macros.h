#ifndef ANDROID_MOD_MENU_MACROS_H
#define ANDROID_MOD_MENU_MACROS_H

#if defined(__aarch64__) //Compile for arm64 lib only
#include <And64InlineHook/And64InlineHook.hpp>

#else //Compile for armv7 lib only. Do not worry about greyed out highlighting code, it still works
#include <Substrate/SubstrateHook.h>
#include <Substrate/CydiaSubstrate.h>

#endif

#define targetLibName OBFUSCATE("libil2cpp.so")

#define HOOK(offset, ptr, orig) MSHookFunction((void *)getAbsoluteAddress(targetLibName, string2Offset(OBFUSCATE(offset))), (void *)ptr, (void **)&orig)
#define HOOK_LIB(lib, offset, ptr, orig) MSHookFunction((void *)getAbsoluteAddress(OBFUSCATE(lib), string2Offset(OBFUSCATE(offset))), (void *)ptr, (void **)&orig)

#define HOOK_NO_ORIG(offset, ptr) MSHookFunction((void *)getAbsoluteAddress(targetLibName, string2Offset(OBFUSCATE(offset))), (void *)ptr, NULL)
#define HOOK_LIB_NO_ORIG(lib, offset, ptr) MSHookFunction((void *)getAbsoluteAddress(OBFUSCATE(lib), string2Offset(OBFUSCATE(offset))), (void *)ptr, NULL)

#define HOOKSYM(sym, ptr, org) MSHookFunction(dlsym(dlopen(targetLibName, 4), OBFUSCATE(sym)), (void *)ptr, (void **)&org)
#define HOOKSYM_LIB(lib, sym, ptr, org) MSHookFunction(dlsym(dlopen(OBFUSCATE(lib), 4), OBFUSCATE(sym)), (void *)ptr, (void **)&org)

#define HOOKSYM_NO_ORIG(sym, ptr)  MSHookFunction(dlsym(dlopen(targetLibName, 4), OBFUSCATE(sym)), (void *)ptr, NULL)
#define HOOKSYM_LIB_NO_ORIG(lib, sym, ptr)  MSHookFunction(dlsym(dlopen(OBFUSCATE(lib), 4), OBFUSCATE(sym)), (void *)ptr, NULL)

// Obfuscate offset
#define OBFUSCATEOFFSET(str) string2Offset(OBFUSCATE(str)) // Encrypt offset

#define OBFUSCATE_BNM(str) str // const char *
#define OBFUSCATES_BNM(str) std::string(OBFUSCATE_BNM(str)) // std::string
#define OBFUSCATE_KEY_BNM(str, key) str // const char *
#define OBFUSCATES_KEY_BNM(str, key) std::string(OBFUSCATE_KEY_BNM(str, key)) // std::string

#define PATCHOFFSET(offset, hex) patchOffset(string2Offset(OBFUSCATE(offset)), OBFUSCATE(hex))
#define PATCHOFFSET_LIB(lib, offset, hex) patchOffset(OBFUSCATE(lib), string2Offset(OBFUSCATE(offset)), OBFUSCATE(hex))

#endif //ANDROID_MOD_MENU_MACROS_H
