#include "network.h"

#include "bitboard/define.h"
#include "bitboard/enum.h"
#include "chessboard/board_state.h"
#include "movegen/move.h"
#include "network/accumulator/king_bucket.h"
#include "network/accumulator/threat.h"
#include "network/arch.hpp"
#include "network/inference.hpp"
#include "third-party/incbin/incbin.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>

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

void Accumulator::recalculate(const BoardState& board_)
{
    king_bucket.recalculate_from_scratch(board_, net);
    threats.recalculate_from_scratch(board_, net);

    assert(king_bucket.acc_is_valid);
    assert(threats.acc_is_valid);

    acc_is_valid = true;
}

void Network::reset_new_search(const BoardState& board, Accumulator& acc)
{
    acc.recalculate(board);
    table.reset_table(net.ft_bias);
}

bool Network::verify(const BoardState& board, const Accumulator& acc)
{
    Accumulator expected = {};
    expected.king_bucket.recalculate_from_scratch(board, net);
    expected.threats.recalculate_from_scratch(board, net);
    expected.acc_is_valid = true;

    assert(acc.king_bucket == expected.king_bucket);
    assert(acc.threats == expected.threats);

    return expected == acc;
}

void Network::store_lazy_updates(
    const BoardState& prev_move_board, const BoardState& post_move_board, Accumulator& acc, Move move)
{
    acc.acc_is_valid = false;

    acc.king_bucket.store_lazy_updates(prev_move_board, post_move_board, move);

    uint64_t sub_bb = 0;
    for (size_t i = 0; i < acc.king_bucket.n_subs; i++)
        sub_bb |= SquareBB[acc.king_bucket.subs[i].piece_sq];
    uint64_t add_bb = 0;
    for (size_t i = 0; i < acc.king_bucket.n_adds; i++)
        add_bb |= SquareBB[acc.king_bucket.adds[i].piece_sq];

    acc.threats.white_threats_requires_recalculation = false;
    acc.threats.black_threats_requires_recalculation = false;

    // don't use move.from() and move.to() because castle moves are encoded as KxR
    auto stm = prev_move_board.stm;
    auto old_king_sq = prev_move_board.get_king_sq(stm);
    auto new_king_sq = post_move_board.get_king_sq(stm);

    if ((enum_to<File>(old_king_sq) <= FILE_D) ^ (enum_to<File>(new_king_sq) <= FILE_D))
    {
        if (stm == WHITE)
        {
            acc.threats.white_threats_requires_recalculation = true;
        }
        else
        {
            acc.threats.black_threats_requires_recalculation = true;
        }
    }

    acc.threats.store_lazy_updates(prev_move_board, post_move_board, sub_bb, add_bb);
}

void Network::apply_lazy_updates(const Accumulator& prev_acc, Accumulator& next_acc)
{
    if (next_acc.acc_is_valid)
    {
        return;
    }

    next_acc.king_bucket.apply_lazy_updates(prev_acc.king_bucket, table, net);
    next_acc.threats.apply_lazy_updates(prev_acc.threats, next_acc.king_bucket.board, net); // is board correct here??

    assert(next_acc.king_bucket.acc_is_valid);
    assert(next_acc.threats.acc_is_valid);

    next_acc.acc_is_valid = true;
}

int calculate_output_bucket(int pieces)
{
    return (pieces - 2) / (32 / OUTPUT_BUCKETS);
}

Score Network::eval(const BoardState& board, const Accumulator& acc)
{
    auto output_bucket = calculate_output_bucket(std::popcount(board.get_pieces_bb()));
    auto stm = board.stm;

    alignas(64) std::array<uint8_t, FT_SIZE> ft_activation;
    alignas(64) std::array<int16_t, FT_SIZE / 4> sparse_ft_nibbles;
    size_t sparse_nibbles_size = 0;
    NN::Features::FT_activation(acc.king_bucket.side[stm], acc.king_bucket.side[!stm], acc.threats.side[stm],
        acc.threats.side[!stm], ft_activation, sparse_ft_nibbles, sparse_nibbles_size);
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