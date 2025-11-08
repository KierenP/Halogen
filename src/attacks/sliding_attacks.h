#pragma once

#include "attacks/black_magic.h"
#include "attacks/fancy_magic.h"
#include "attacks/fancy_pdep.h"
#include "attacks/fancy_pext.h"

#ifdef USE_PEXT
using RookAttackStrategy = RookFancyPDEPStrategy;
using BishopAttackStrategy = BishopFancyPDEPStrategy;
#else
using RookAttackStrategy = RookBlackMagicStrategy;
using BishopAttackStrategy = BishopBlackMagicStrategy;
#endif

const RookAttackStrategy RookSlidingAttacks;
const BishopAttackStrategy BishopSlidingAttacks;
