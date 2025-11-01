#pragma once

#include "attacks/bmi2.h"
#include "attacks/bmi2_compressed.h"
#include "attacks/fancy_magic.h"

#ifdef USE_PEXT
using RookAttackStrategy = BMI2CompressedStrategy<BMI2RookTraits>; // 201KB
using BishopAttackStrategy = BMI2Strategy<BMI2BishopTraits>; // 42KB
#else
using RookAttackStrategy = FancyMagicStrategy<FancyMagicRookTraits>; // 801KB
using BishopAttackStrategy = FancyMagicStrategy<FancyMagicBishopTraits>; // 42KB
#endif

const RookAttackStrategy RookSlidingAttacks;
const BishopAttackStrategy BishopSlidingAttacks;
