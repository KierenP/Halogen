#include "network.h"

#include "bitboard/define.h"
#include "chessboard/board_state.h"
#include "movegen/move.h"
#include "movegen/movegen.h"
#include "network/accumulator.hpp"
#include "network/arch.hpp"
#include "network/inference.hpp"
#include "network/threat_inputs.h"
#include "third-party/incbin/incbin.h"

#include <algorithm>
#include <bit>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <iostream>

#if defined(USE_SSE4)
// IWYU is confused and thinks we need this
#include <immintrin.h>
#endif

namespace NN
{

#undef INCBIN_ALIGNMENT
#define INCBIN_ALIGNMENT 64
INCBIN(Net, EVALFILE);

const network& net = reinterpret_cast<const network&>(*gNetData);

[[maybe_unused]] auto verify_network_size = []
{
    if (sizeof(network) != gNetSize)
    {
        std::cout << "Error: embedded network is not the expected size. Expected " << sizeof(network)
                  << " bytes actual " << gNetSize << " bytes." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return true;
}();

int get_king_bucket(Square king_sq, Side view)
{
    king_sq = view == WHITE ? king_sq : flip_square_vertical(king_sq);
    return KING_BUCKETS[king_sq];
}

// King-bucketed piece-square index (same as before, in [0, 768 * KING_BUCKET_COUNT))
int index(Square king_sq, Square piece_sq, Piece piece, Side view)
{
    piece_sq = view == WHITE ? piece_sq : flip_square_vertical(piece_sq);
    piece_sq = enum_to<File>(king_sq) <= FILE_D ? piece_sq : flip_square_horizontal(piece_sq);

    auto king_bucket = get_king_bucket(king_sq, view);
    Side relativeColor = static_cast<Side>(view != enum_to<Side>(piece));
    PieceType pieceType = enum_to<PieceType>(piece);

    return king_bucket * 64 * 6 * 2 + relativeColor * 64 * 6 + pieceType * 64 + piece_sq;
}

// Unbucketed piece-square index (in [PSQT_OFFSET, PSQT_OFFSET + 768))
// Uses the same relative color / piece type / square mapping as king-bucketed, but without king bucket offset
// and always from the perspective of the viewing side (vertically flipped for black, no horizontal mirror).
int psqt_index(Square piece_sq, Piece piece, Side view)
{
    piece_sq = view == WHITE ? piece_sq : flip_square_vertical(piece_sq);

    Side relativeColor = static_cast<Side>(view != enum_to<Side>(piece));
    PieceType pieceType = enum_to<PieceType>(piece);

    return PSQT_OFFSET + relativeColor * 64 * 6 + pieceType * 64 + piece_sq;
}

void Add1Sub1(const Accumulator& prev, Accumulator& next, const Input& a1, const Input& s1, Side view)
{
    size_t a1_kb = index(a1.king_sq, a1.piece_sq, a1.piece, view);
    size_t s1_kb = index(s1.king_sq, s1.piece_sq, s1.piece, view);
    size_t a1_ps = psqt_index(a1.piece_sq, a1.piece, view);
    size_t s1_ps = psqt_index(s1.piece_sq, s1.piece, view);

    add1sub1(next.side[view], prev.side[view], net.ft_weight[a1_kb], net.ft_weight[s1_kb]);
    add1sub1(next.side[view], next.side[view], net.ft_weight[a1_ps], net.ft_weight[s1_ps]);
}

void Add1Sub1(const Accumulator& prev, Accumulator& next, const InputPair& add1, const InputPair& sub1)
{
    Add1Sub1(prev, next, { add1.w_king, add1.piece_sq, add1.piece }, { sub1.w_king, sub1.piece_sq, sub1.piece }, WHITE);
    Add1Sub1(prev, next, { add1.b_king, add1.piece_sq, add1.piece }, { sub1.b_king, sub1.piece_sq, sub1.piece }, BLACK);
}

void Add1Sub2(const Accumulator& prev, Accumulator& next, const Input& a1, const Input& s1, const Input& s2, Side view)
{
    size_t a1_kb = index(a1.king_sq, a1.piece_sq, a1.piece, view);
    size_t s1_kb = index(s1.king_sq, s1.piece_sq, s1.piece, view);
    size_t s2_kb = index(s2.king_sq, s2.piece_sq, s2.piece, view);
    size_t a1_ps = psqt_index(a1.piece_sq, a1.piece, view);
    size_t s1_ps = psqt_index(s1.piece_sq, s1.piece, view);
    size_t s2_ps = psqt_index(s2.piece_sq, s2.piece, view);

    add1sub2(next.side[view], prev.side[view], net.ft_weight[a1_kb], net.ft_weight[s1_kb], net.ft_weight[s2_kb]);
    add1sub2(next.side[view], next.side[view], net.ft_weight[a1_ps], net.ft_weight[s1_ps], net.ft_weight[s2_ps]);
}

void Add1Sub2(
    const Accumulator& prev, Accumulator& next, const InputPair& add1, const InputPair& sub1, const InputPair& sub2)
{
    Add1Sub2(prev, next, { add1.w_king, add1.piece_sq, add1.piece }, { sub1.w_king, sub1.piece_sq, sub1.piece },
        { sub2.w_king, sub2.piece_sq, sub2.piece }, WHITE);
    Add1Sub2(prev, next, { add1.b_king, add1.piece_sq, add1.piece }, { sub1.b_king, sub1.piece_sq, sub1.piece },
        { sub2.b_king, sub2.piece_sq, sub2.piece }, BLACK);
}

void Add2Sub2(const Accumulator& prev, Accumulator& next, const Input& a1, const Input& a2, const Input& s1,
    const Input& s2, Side view)
{
    size_t a1_kb = index(a1.king_sq, a1.piece_sq, a1.piece, view);
    size_t a2_kb = index(a2.king_sq, a2.piece_sq, a2.piece, view);
    size_t s1_kb = index(s1.king_sq, s1.piece_sq, s1.piece, view);
    size_t s2_kb = index(s2.king_sq, s2.piece_sq, s2.piece, view);
    size_t a1_ps = psqt_index(a1.piece_sq, a1.piece, view);
    size_t a2_ps = psqt_index(a2.piece_sq, a2.piece, view);
    size_t s1_ps = psqt_index(s1.piece_sq, s1.piece, view);
    size_t s2_ps = psqt_index(s2.piece_sq, s2.piece, view);

    add2sub2(next.side[view], prev.side[view], net.ft_weight[a1_kb], net.ft_weight[a2_kb], net.ft_weight[s1_kb],
        net.ft_weight[s2_kb]);
    add2sub2(next.side[view], next.side[view], net.ft_weight[a1_ps], net.ft_weight[a2_ps], net.ft_weight[s1_ps],
        net.ft_weight[s2_ps]);
}

void Add2Sub2(const Accumulator& prev, Accumulator& next, const InputPair& add1, const InputPair& add2,
    const InputPair& sub1, const InputPair& sub2)
{
    Add2Sub2(prev, next, { add1.w_king, add1.piece_sq, add1.piece }, { add2.w_king, add2.piece_sq, add2.piece },
        { sub1.w_king, sub1.piece_sq, sub1.piece }, { sub2.w_king, sub2.piece_sq, sub2.piece }, WHITE);
    Add2Sub2(prev, next, { add1.b_king, add1.piece_sq, add1.piece }, { add2.b_king, add2.piece_sq, add2.piece },
        { sub1.b_king, sub1.piece_sq, sub1.piece }, { sub2.b_king, sub2.piece_sq, sub2.piece }, BLACK);
}

// --- King-bucketed input operations ---

void Accumulator::add_king_input(const InputPair& input)
{
    add_king_input({ input.w_king, input.piece_sq, input.piece }, WHITE);
    add_king_input({ input.b_king, input.piece_sq, input.piece }, BLACK);
}

void Accumulator::add_king_input(const Input& input, Side view)
{
    size_t idx = index(input.king_sq, input.piece_sq, input.piece, view);
    NN::add1(side[view], net.ft_weight[idx]);
}

void Accumulator::sub_king_input(const InputPair& input)
{
    sub_king_input({ input.w_king, input.piece_sq, input.piece }, WHITE);
    sub_king_input({ input.b_king, input.piece_sq, input.piece }, BLACK);
}

void Accumulator::sub_king_input(const Input& input, Side view)
{
    size_t idx = index(input.king_sq, input.piece_sq, input.piece, view);
    NN::sub1(side[view], net.ft_weight[idx]);
}

// --- Unbucketed piece-square input operations ---

void Accumulator::add_psqt_input(const InputPair& input)
{
    add_psqt_input({ input.w_king, input.piece_sq, input.piece }, WHITE);
    add_psqt_input({ input.b_king, input.piece_sq, input.piece }, BLACK);
}

void Accumulator::add_psqt_input(const Input& input, Side view)
{
    size_t idx = psqt_index(input.piece_sq, input.piece, view);
    NN::add1(side[view], net.ft_weight[idx]);
}

void Accumulator::sub_psqt_input(const InputPair& input)
{
    sub_psqt_input({ input.w_king, input.piece_sq, input.piece }, WHITE);
    sub_psqt_input({ input.b_king, input.piece_sq, input.piece }, BLACK);
}

void Accumulator::sub_psqt_input(const Input& input, Side view)
{
    size_t idx = psqt_index(input.piece_sq, input.piece, view);
    NN::sub1(side[view], net.ft_weight[idx]);
}

// --- Combined input operations (king-bucketed + psqt) ---

void Accumulator::add_input(const InputPair& input)
{
    add_input({ input.w_king, input.piece_sq, input.piece }, WHITE);
    add_input({ input.b_king, input.piece_sq, input.piece }, BLACK);
}

void Accumulator::add_input(const Input& input, Side view)
{
    size_t kb_idx = index(input.king_sq, input.piece_sq, input.piece, view);
    size_t ps_idx = psqt_index(input.piece_sq, input.piece, view);
    NN::add1(side[view], net.ft_weight[kb_idx]);
    NN::add1(side[view], net.ft_weight[ps_idx]);
}

void Accumulator::sub_input(const InputPair& input)
{
    sub_input({ input.w_king, input.piece_sq, input.piece }, WHITE);
    sub_input({ input.b_king, input.piece_sq, input.piece }, BLACK);
}

void Accumulator::sub_input(const Input& input, Side view)
{
    size_t kb_idx = index(input.king_sq, input.piece_sq, input.piece, view);
    size_t ps_idx = psqt_index(input.piece_sq, input.piece, view);
    NN::sub1(side[view], net.ft_weight[kb_idx]);
    NN::sub1(side[view], net.ft_weight[ps_idx]);
}

void Accumulator::recalculate(const BoardState& board_)
{
    recalculate(board_, WHITE);
    recalculate(board_, BLACK);
    acc_is_valid = true;
}

void Accumulator::recalculate(const BoardState& board_, Side view)
{
    side[view] = net.ft_bias;
    auto king = board_.get_king_sq(view);

    for (int i = 0; i < N_PIECES; i++)
    {
        Piece piece = static_cast<Piece>(i);
        uint64_t bb = board_.get_pieces_bb(piece);

        while (bb)
        {
            Square sq = lsbpop(bb);
            add_input({ king, sq, piece }, view);
        }
    }
}

void AccumulatorTable::recalculate(Accumulator& acc, const BoardState& board, Side side, Square king_sq)
{
    // we need to keep a separate entry for when the king is on each side of the board
    auto& entry
        = king_bucket[get_king_bucket(king_sq, side) + (enum_to<File>(king_sq) <= FILE_D ? 0 : KING_BUCKET_COUNT)];
    auto& bb = side == WHITE ? entry.white_bb : entry.black_bb;

    for (const auto& piece : {
             WHITE_PAWN,
             WHITE_KNIGHT,
             WHITE_BISHOP,
             WHITE_ROOK,
             WHITE_QUEEN,
             WHITE_KING,
             BLACK_PAWN,
             BLACK_KNIGHT,
             BLACK_BISHOP,
             BLACK_ROOK,
             BLACK_QUEEN,
             BLACK_KING,
         })
    {
        auto new_bb = board.get_pieces_bb(piece);
        auto& old_bb = bb[piece];

        auto to_add = new_bb & ~old_bb;
        auto to_sub = old_bb & ~new_bb;

        while (to_add)
        {
            auto sq = lsbpop(to_add);
            entry.acc.add_input({ king_sq, sq, piece }, side);
        }

        while (to_sub)
        {
            auto sq = lsbpop(to_sub);
            entry.acc.sub_input({ king_sq, sq, piece }, side);
        }

        old_bb = new_bb;
    }

    acc.side[side] = entry.acc.side[side];
}

void Network::reset_new_search(const BoardState& board, Accumulator& acc)
{
    acc.recalculate(board);

    for (auto& entry : table.king_bucket)
    {
        entry.acc = {};
        entry.acc.side = { net.ft_bias, net.ft_bias };
        entry.white_bb = {};
        entry.black_bb = {};
    }
}

bool Network::verify(const BoardState& board, const Accumulator& acc)
{
    Accumulator expected = {};
    expected.side = { net.ft_bias, net.ft_bias };
    auto w_king = board.get_king_sq(WHITE);
    auto b_king = board.get_king_sq(BLACK);

    for (int i = 0; i < N_PIECES; i++)
    {
        Piece piece = static_cast<Piece>(i);
        uint64_t bb = board.get_pieces_bb(piece);

        while (bb)
        {
            Square sq = lsbpop(bb);
            expected.add_input({ w_king, b_king, sq, piece });
        }
    }

    return expected == acc;
}

void Network::store_lazy_updates(
    const BoardState& prev_move_board, const BoardState& post_move_board, Accumulator& acc, Move move)
{
    assert(move != Move::Uninitialized);

    acc.acc_is_valid = false;
    acc.white_requires_recalculation = false;
    acc.black_requires_recalculation = false;
    acc.adds = {};
    acc.n_adds = 0;
    acc.subs = {};
    acc.n_subs = 0;
    acc.board = {};

    auto stm = prev_move_board.stm;
    auto from_sq = move.from();
    auto to_sq = move.to();
    auto from_piece = prev_move_board.get_square_piece(from_sq);
    auto to_piece = post_move_board.get_square_piece(to_sq);
    auto cap_piece = prev_move_board.get_square_piece(to_sq);

    auto w_king = post_move_board.get_king_sq(WHITE);
    auto b_king = post_move_board.get_king_sq(BLACK);

    if (from_piece == get_piece(KING, stm))
    {
        if (get_king_bucket(from_sq, stm) != get_king_bucket(to_sq, stm)
            || ((enum_to<File>(from_sq) <= FILE_D) ^ (enum_to<File>(to_sq) <= FILE_D)))
        {
            if (stm == WHITE)
            {
                acc.white_requires_recalculation = true;
            }
            else
            {
                acc.black_requires_recalculation = true;
            }

            acc.board = post_move_board;

            if (move.is_castle())
            {
                Square king_start = move.from();
                Square king_end
                    = move.flag() == A_SIDE_CASTLE ? (stm == WHITE ? SQ_C1 : SQ_C8) : (stm == WHITE ? SQ_G1 : SQ_G8);
                Square rook_start = move.to();
                Square rook_end
                    = move.flag() == A_SIDE_CASTLE ? (stm == WHITE ? SQ_D1 : SQ_D8) : (stm == WHITE ? SQ_F1 : SQ_F8);

                acc.n_adds = 2;
                acc.n_subs = 2;

                acc.adds[0] = { w_king, b_king, king_end, from_piece };
                acc.adds[1] = { w_king, b_king, rook_end, cap_piece };

                acc.subs[0] = { w_king, b_king, king_start, from_piece };
                acc.subs[1] = { w_king, b_king, rook_start, cap_piece };
            }
            else if (move.is_capture())
            {
                acc.n_adds = 1;
                acc.n_subs = 2;

                acc.adds[0] = { w_king, b_king, to_sq, from_piece };

                acc.subs[0] = { w_king, b_king, from_sq, from_piece };
                acc.subs[1] = { w_king, b_king, to_sq, cap_piece };
            }
            else
            {
                acc.n_adds = 1;
                acc.n_subs = 1;

                acc.adds[0] = { w_king, b_king, to_sq, from_piece };

                acc.subs[0] = { w_king, b_king, from_sq, from_piece };
            }
        }
        else if (move.is_castle())
        {
            Square king_start = move.from();
            Square king_end
                = move.flag() == A_SIDE_CASTLE ? (stm == WHITE ? SQ_C1 : SQ_C8) : (stm == WHITE ? SQ_G1 : SQ_G8);
            Square rook_start = move.to();
            Square rook_end
                = move.flag() == A_SIDE_CASTLE ? (stm == WHITE ? SQ_D1 : SQ_D8) : (stm == WHITE ? SQ_F1 : SQ_F8);

            acc.n_adds = 2;
            acc.n_subs = 2;

            acc.adds[0] = { w_king, b_king, king_end, from_piece };
            acc.adds[1] = { w_king, b_king, rook_end, cap_piece };

            acc.subs[0] = { w_king, b_king, king_start, from_piece };
            acc.subs[1] = { w_king, b_king, rook_start, cap_piece };
        }
        else if (move.is_capture())
        {
            acc.n_adds = 1;
            acc.n_subs = 2;

            acc.adds[0] = { w_king, b_king, to_sq, from_piece };

            acc.subs[0] = { w_king, b_king, from_sq, from_piece };
            acc.subs[1] = { w_king, b_king, to_sq, cap_piece };
        }
        else
        {
            acc.n_adds = 1;
            acc.n_subs = 1;

            acc.adds[0] = { w_king, b_king, to_sq, from_piece };

            acc.subs[0] = { w_king, b_king, from_sq, from_piece };
        }
    }
    else
    {
        if (move.is_promotion() && move.is_capture())
        {
            acc.n_adds = 1;
            acc.n_subs = 2;

            acc.adds[0] = { w_king, b_king, to_sq, to_piece };

            acc.subs[0] = { w_king, b_king, from_sq, from_piece };
            acc.subs[1] = { w_king, b_king, to_sq, cap_piece };
        }
        else if (move.is_promotion())
        {
            acc.n_adds = 1;
            acc.n_subs = 1;

            acc.adds[0] = { w_king, b_king, to_sq, to_piece };

            acc.subs[0] = { w_king, b_king, from_sq, from_piece };
        }
        else if (move.is_capture() && move.flag() == EN_PASSANT)
        {
            auto ep_capture_sq = get_square(enum_to<File>(move.to()), enum_to<Rank>(move.from()));

            acc.n_adds = 1;
            acc.n_subs = 2;

            acc.adds[0] = { w_king, b_king, to_sq, from_piece };

            acc.subs[0] = { w_king, b_king, from_sq, from_piece };
            acc.subs[1] = { w_king, b_king, ep_capture_sq, get_piece(PAWN, !stm) };
        }
        else if (move.is_capture())
        {
            acc.n_adds = 1;
            acc.n_subs = 2;

            acc.adds[0] = { w_king, b_king, to_sq, from_piece };

            acc.subs[0] = { w_king, b_king, from_sq, from_piece };
            acc.subs[1] = { w_king, b_king, to_sq, cap_piece };
        }
        else
        {
            acc.n_adds = 1;
            acc.n_subs = 1;

            acc.adds[0] = { w_king, b_king, to_sq, from_piece };

            acc.subs[0] = { w_king, b_king, from_sq, from_piece };
        }
    }
}

void Network::apply_lazy_updates(const Accumulator& prev_acc, Accumulator& next_acc)
{
    if (next_acc.acc_is_valid)
    {
        return;
    }

    assert(!(next_acc.white_requires_recalculation && next_acc.black_requires_recalculation));

    if (next_acc.white_requires_recalculation)
    {
        table.recalculate(next_acc, next_acc.board, WHITE, next_acc.board.get_king_sq(WHITE));

        if (next_acc.n_adds == 1 && next_acc.n_subs == 1)
        {
            Add1Sub1(prev_acc, next_acc, { next_acc.adds[0].b_king, next_acc.adds[0].piece_sq, next_acc.adds[0].piece },
                { next_acc.subs[0].b_king, next_acc.subs[0].piece_sq, next_acc.subs[0].piece }, BLACK);
        }
        else if (next_acc.n_adds == 1 && next_acc.n_subs == 2)
        {
            Add1Sub2(prev_acc, next_acc, { next_acc.adds[0].b_king, next_acc.adds[0].piece_sq, next_acc.adds[0].piece },
                { next_acc.subs[0].b_king, next_acc.subs[0].piece_sq, next_acc.subs[0].piece },
                { next_acc.subs[1].b_king, next_acc.subs[1].piece_sq, next_acc.subs[1].piece }, BLACK);
        }
        else if (next_acc.n_adds == 2 && next_acc.n_subs == 2)
        {
            Add2Sub2(prev_acc, next_acc, { next_acc.adds[0].b_king, next_acc.adds[0].piece_sq, next_acc.adds[0].piece },
                { next_acc.adds[1].b_king, next_acc.adds[1].piece_sq, next_acc.adds[1].piece },
                { next_acc.subs[0].b_king, next_acc.subs[0].piece_sq, next_acc.subs[0].piece },
                { next_acc.subs[1].b_king, next_acc.subs[1].piece_sq, next_acc.subs[1].piece }, BLACK);
        }
    }
    else if (next_acc.black_requires_recalculation)
    {
        table.recalculate(next_acc, next_acc.board, BLACK, next_acc.board.get_king_sq(BLACK));

        if (next_acc.n_adds == 1 && next_acc.n_subs == 1)
        {
            Add1Sub1(prev_acc, next_acc, { next_acc.adds[0].w_king, next_acc.adds[0].piece_sq, next_acc.adds[0].piece },
                { next_acc.subs[0].w_king, next_acc.subs[0].piece_sq, next_acc.subs[0].piece }, WHITE);
        }
        else if (next_acc.n_adds == 1 && next_acc.n_subs == 2)
        {
            Add1Sub2(prev_acc, next_acc, { next_acc.adds[0].w_king, next_acc.adds[0].piece_sq, next_acc.adds[0].piece },
                { next_acc.subs[0].w_king, next_acc.subs[0].piece_sq, next_acc.subs[0].piece },
                { next_acc.subs[1].w_king, next_acc.subs[1].piece_sq, next_acc.subs[1].piece }, WHITE);
        }
        else if (next_acc.n_adds == 2 && next_acc.n_subs == 2)
        {
            Add2Sub2(prev_acc, next_acc, { next_acc.adds[0].w_king, next_acc.adds[0].piece_sq, next_acc.adds[0].piece },
                { next_acc.adds[1].w_king, next_acc.adds[1].piece_sq, next_acc.adds[1].piece },
                { next_acc.subs[0].w_king, next_acc.subs[0].piece_sq, next_acc.subs[0].piece },
                { next_acc.subs[1].w_king, next_acc.subs[1].piece_sq, next_acc.subs[1].piece }, WHITE);
        }
    }
    else
    {
        if (next_acc.n_adds == 1 && next_acc.n_subs == 1)
        {
            Add1Sub1(prev_acc, next_acc, next_acc.adds[0], next_acc.subs[0]);
        }
        else if (next_acc.n_adds == 1 && next_acc.n_subs == 2)
        {
            Add1Sub2(prev_acc, next_acc, next_acc.adds[0], next_acc.subs[0], next_acc.subs[1]);
        }
        else if (next_acc.n_adds == 2 && next_acc.n_subs == 2)
        {
            Add2Sub2(prev_acc, next_acc, next_acc.adds[0], next_acc.adds[1], next_acc.subs[0], next_acc.subs[1]);
        }
        else if (next_acc.n_adds == 0 && next_acc.n_subs == 0)
        {
            // null move
            next_acc.side = prev_acc.side;
        }
    }

    next_acc.acc_is_valid = true;
}

int calculate_output_bucket(int pieces)
{
    return (pieces - 2) / (32 / OUTPUT_BUCKETS);
}

// Compute threat features from the board position and add their weight contributions to the accumulator.
// The base accumulator (with king-bucketed + psqt features) is passed in, and this function produces output
// accumulators with threat features applied on top.
void apply_threat_features(const BoardState& board, const std::array<int16_t, FT_SIZE>& base_stm,
    const std::array<int16_t, FT_SIZE>& base_nstm, std::array<int16_t, FT_SIZE>& out_stm,
    std::array<int16_t, FT_SIZE>& out_nstm)
{
    out_stm = base_stm;
    out_nstm = base_nstm;

    auto stm = board.stm;
    uint64_t occ = board.get_pieces_bb();

    // For each piece on the board, compute its attacks and find threatened pieces
    for (int piece_idx = 0; piece_idx < N_PIECES; piece_idx++)
    {
        Piece atk_piece = static_cast<Piece>(piece_idx);
        PieceType atk_pt = enum_to<PieceType>(atk_piece);
        Side atk_color = enum_to<Side>(atk_piece);

        // Side index for this attacker from STM perspective: 0 = STM's piece, 1 = NSTM's piece
        int atk_side_stm = (atk_color == stm) ? 0 : 1;
        // Side index from NSTM perspective: flipped
        int atk_side_nstm = atk_side_stm ^ 1;

        uint64_t atk_bb = board.get_pieces_bb(atk_piece);
        while (atk_bb)
        {
            Square atk_sq = lsbpop(atk_bb);

            // Compute attacks for this piece using actual board occupancy for sliders
            uint64_t attacks;
            switch (atk_pt)
            {
            case PAWN:
                attacks = PawnAttacks[atk_color][atk_sq];
                break;
            case KNIGHT:
                attacks = KnightAttacks[atk_sq];
                break;
            case BISHOP:
                attacks = attack_bb<BISHOP>(atk_sq, occ);
                break;
            case ROOK:
                attacks = attack_bb<ROOK>(atk_sq, occ);
                break;
            case QUEEN:
                attacks = attack_bb<QUEEN>(atk_sq, occ);
                break;
            case KING:
                attacks = KingAttacks[atk_sq];
                break;
            default:
                attacks = 0;
                break;
            }

            // Find pieces being attacked
            uint64_t attacked = attacks & occ;
            while (attacked)
            {
                Square vic_sq = lsbpop(attacked);
                Piece vic_piece = board.get_square_piece(vic_sq);
                PieceType vic_pt = enum_to<PieceType>(vic_piece);
                Side vic_color = enum_to<Side>(vic_piece);
                int vic_side_stm = (vic_color == stm) ? 0 : 1;
                int vic_side_nstm = vic_side_stm ^ 1;

                // --- STM perspective ---
                // Squares must be in STM-relative coordinates (flip if STM is BLACK)
                int stm_atk_idx = atk_pt * 2 + atk_side_stm;
                int stm_vic_idx = vic_pt * 2 + vic_side_stm;
                int stm_atk_sq = (stm == BLACK) ? (atk_sq ^ 56) : atk_sq;
                int stm_vic_sq = (stm == BLACK) ? (vic_sq ^ 56) : vic_sq;
                uint32_t stm_threat = THREAT_TABLE.lookup[stm_atk_idx][stm_atk_sq][stm_vic_idx][stm_vic_sq];

                // --- NSTM perspective ---
                // Squares must be in NSTM-relative coordinates (flip if NSTM is BLACK, i.e. STM is WHITE)
                int nstm_atk_idx = atk_pt * 2 + atk_side_nstm;
                int nstm_vic_idx = vic_pt * 2 + vic_side_nstm;
                int nstm_atk_sq = (stm == WHITE) ? (atk_sq ^ 56) : atk_sq;
                int nstm_vic_sq = (stm == WHITE) ? (vic_sq ^ 56) : vic_sq;
                uint32_t nstm_threat = THREAT_TABLE.lookup[nstm_atk_idx][nstm_atk_sq][nstm_vic_idx][nstm_vic_sq];

                // If the threat is valid (not deduped/excluded), add the weight row
                if (stm_threat != INVALID_THREAT && nstm_threat != INVALID_THREAT)
                {
                    NN::add1(out_stm, net.ft_weight[THREAT_OFFSET + stm_threat]);
                    NN::add1(out_nstm, net.ft_weight[THREAT_OFFSET + nstm_threat]);
                }

                // either threat is present for both perspectives or neither.
                assert(((stm_threat == INVALID_THREAT) ^ (nstm_threat == INVALID_THREAT)) == 0);
            }
        }
    }
}

Score Network::eval(const BoardState& board, const Accumulator& acc)
{
    auto stm = board.stm;
    auto output_bucket = calculate_output_bucket(std::popcount(board.get_pieces_bb()));

    // Apply threat features on top of the incrementally-updated accumulator
    alignas(64) std::array<int16_t, FT_SIZE> threat_stm;
    alignas(64) std::array<int16_t, FT_SIZE> threat_nstm;
    apply_threat_features(board, acc.side[stm], acc.side[!stm], threat_stm, threat_nstm);

    alignas(64) std::array<uint8_t, FT_SIZE> ft_activation;
    alignas(64) std::array<int16_t, FT_SIZE / 4> sparse_ft_nibbles;
    size_t sparse_nibbles_size = 0;
    NN::Features::FT_activation(threat_stm, threat_nstm, ft_activation, sparse_ft_nibbles, sparse_nibbles_size);
    assert(std::all_of(ft_activation.begin(), ft_activation.end(), [](auto x) { return x <= 127; }));

    alignas(64) std::array<float, L1_SIZE * 2> l1_activation;
    NN::Features::L1_activation(ft_activation, net.l1_weight[output_bucket], net.l1_bias[output_bucket],
        sparse_ft_nibbles, sparse_nibbles_size, l1_activation);
    assert(std::all_of(l1_activation.begin(), l1_activation.end(), [](auto x) { return 0 <= x && x <= 1; }));

    alignas(64) std::array<float, L2_SIZE> l2_activation = net.l2_bias[output_bucket];
    NN::Features::L2_activation(l1_activation, net.l2_weight[output_bucket], l2_activation);
    assert(std::all_of(l2_activation.begin(), l2_activation.end(), [](auto x) { return 0 <= x && x <= 1; }));

    float output = net.l3_bias[output_bucket];
    NN::Features::L3_activation(l2_activation, net.l3_weight[output_bucket], output);

    return output * SCALE_FACTOR;
}

Score Network::slow_eval(const BoardState& board)
{
    Accumulator acc;
    acc.recalculate(board);
    return eval(board, acc);
}

}