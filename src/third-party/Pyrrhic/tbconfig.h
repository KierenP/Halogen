/*
 * (c) 2015 basil, all rights reserved,
 * Modifications Copyright (c) 2016-2019 by Jon Dart
 * Modifications Copyright (c) 2020-2024 by Andrew Grant
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

/*
 * You are in charge of defining each of these macros. The macros already
 * defined here are simply an example of what to do. This configuration is
 * used by Ethereal to implement Pyrrhic.
 *
 * See Ethereal's source <https://github.com/AndyGrant/Ethereal> if it is
 * not readily clear what these definfitions mean. The relevant files are
 * are the ones included below.
 *
 * Note that for the Pawn Attacks, we invert the colour. This is because
 * Pyrrhic defines White as 1, where as Ethereal (any many others) choose
 * to define White as 0 and Black as 1.
 */

#include "MoveGeneration.h"

#define PYRRHIC_POPCOUNT(x)              (popcount(x))
#define PYRRHIC_LSB(x)                   (lsb(x))
#define PYRRHIC_POPLSB(x)                (lsbpop(*x))

#define PYRRHIC_PAWN_ATTACKS(sq, c)      (PawnAttacks[c][sq])
#define PYRRHIC_KNIGHT_ATTACKS(sq)       (AttackBB<KNIGHT>(static_cast<Square>(sq)))
#define PYRRHIC_BISHOP_ATTACKS(sq, occ)  (AttackBB<BISHOP>(static_cast<Square>(sq), occ))
#define PYRRHIC_ROOK_ATTACKS(sq, occ)    (AttackBB<ROOK>(static_cast<Square>(sq), occ))
#define PYRRHIC_QUEEN_ATTACKS(sq, occ)   (AttackBB<QUEEN>(static_cast<Square>(sq), occ))
#define PYRRHIC_KING_ATTACKS(sq)         (AttackBB<KING>(static_cast<Square>(sq)))
