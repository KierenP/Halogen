#pragma once

#include "attacks/black_magic.h"
#include "attacks/fancy_magic.h"
#include "attacks/fancy_pdep.h"
#include "attacks/fancy_pext.h"

#ifdef USE_PEXT
using RookAttackStrategy = RookFancyPDEPStrategy;
using BishopAttackStrategy = BishopFancyPEXTStrategy;
#else
using RookAttackStrategy = RookFancyMagicStrategy;
using BishopAttackStrategy = BishopFancyMagicStrategy;
#endif

const RookAttackStrategy RookSlidingAttacks;
const BishopAttackStrategy BishopSlidingAttacks;
