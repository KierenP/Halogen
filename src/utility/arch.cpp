#include "arch.h"
#include <format>
#include <string_view>

constexpr std::string_view get_platform()
{ // Get current architecture, detectx [sic] nearly every architecture. Coded by Freak
#if defined(__x86_64__) || defined(_M_X64)
    return "x86_64";
#elif defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)
    return "x86_32";
#elif defined(__ARM_ARCH_2__)
    return "ARM2";
#elif defined(__ARM_ARCH_3__) || defined(__ARM_ARCH_3M__)
    return "ARM3";
#elif defined(__ARM_ARCH_4T__) || defined(__TARGET_ARM_4T)
    return "ARM4T";
#elif defined(__ARM_ARCH_5_) || defined(__ARM_ARCH_5E_)
    return "ARM5"
#elif defined(__ARM_ARCH_6T2_) || defined(__ARM_ARCH_6T2_)
    return "ARM6T2";
#elif defined(__ARM_ARCH_6__) || defined(__ARM_ARCH_6J__) || defined(__ARM_ARCH_6K__) || defined(__ARM_ARCH_6Z__)      \
    || defined(__ARM_ARCH_6ZK__)
    return "ARM6";
#elif defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__)      \
    || defined(__ARM_ARCH_7S__)
    return "ARM7";
#elif defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7S__)
    return "ARM7A";
#elif defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7S__)
    return "ARM7R";
#elif defined(__ARM_ARCH_7M__)
    return "ARM7M";
#elif defined(__ARM_ARCH_7S__)
    return "ARM7S";
#elif defined(__aarch64__) || defined(_M_ARM64)
    return "ARM64";
#elif defined(mips) || defined(__mips__) || defined(__mips)
    return "MIPS";
#elif defined(__sh__)
    return "SUPERH";
#elif defined(__powerpc) || defined(__powerpc__) || defined(__powerpc64__) || defined(__POWERPC__) || defined(__ppc__) \
    || defined(__PPC__) || defined(_ARCH_PPC)
    return "POWERPC";
#elif defined(__PPC64__) || defined(__ppc64__) || defined(_ARCH_PPC64)
    return "POWERPC64";
#elif defined(__sparc__) || defined(__sparc)
    return "SPARC";
#elif defined(__m68k__)
    return "M68K";
#else
    return "UNKNOWN";
#endif
}

constexpr std::string_view get_arch()
{
#if defined(USE_AVX512_VNNI)
    static_assert(USE_PEXT);
    return "AVX512_VNNI PEXT";
#elif defined(USE_AVX512)
    static_assert(USE_PEXT);
    return "AVX512 PEXT";
#elif defined(USE_AVX2) and defined(USE_PEXT)
    return "AVX2 PEXT";
#elif defined(USE_AVX2)
    return "AVX2";
#elif defined(USE_AVX)
    return "AVX";
#elif defined(USE_SSE4)
    return "SSE4";
#endif

#if defined(USE_NEON_DOTPROD)
    return "NEON DOTPROD";
#elif defined(USE_NEON)
    return "NEON";
#endif

    return "UNKNOWN";
}

std::string fmt_version_platform_arch(std::string_view version)
{
    static constexpr auto platform = get_platform();
    static constexpr auto arch = get_arch();
    return std::format("Halogen {} {} {}", version, platform, arch);
}