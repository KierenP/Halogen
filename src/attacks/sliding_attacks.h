#pragma once

#include "attacks/black_magic.h"
#include "attacks/fancy_magic.h"

#ifdef USE_PEXT
#include "attacks/fancy_pdep.h"
#include "attacks/fancy_pext.h"
#endif

#ifdef USE_PEXT
using RookAttackStrategy = RookFancyPDEPStrategy;
using BishopAttackStrategy = BishopFancyPDEPStrategy;
#else
using RookAttackStrategy = RookBlackMagicStrategy;
using BishopAttackStrategy = BishopBlackMagicStrategy;
#endif

const RookAttackStrategy RookSlidingAttacks;
const BishopAttackStrategy BishopSlidingAttacks;
