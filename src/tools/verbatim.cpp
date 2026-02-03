// Load the network file, do some preprocessing, and then save it to the build directory for inclusion in the final
// binary

#include "network/arch.hpp"
#include "network/simd/define.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <ranges>
#include <tuple>
#include <utility>

namespace NN
{

struct raw_network
{
    alignas(64) std::array<std::array<int16_t, FT_SIZE>, INPUT_SIZE * KING_BUCKET_COUNT> ft_weight = {};
    alignas(64) std::array<int16_t, FT_SIZE> ft_bias = {};
    alignas(64) std::array<std::array<std::array<int16_t, FT_SIZE>, L1_SIZE>, OUTPUT_BUCKETS> l1_weight = {};
    alignas(64) std::array<std::array<int16_t, L1_SIZE>, OUTPUT_BUCKETS> l1_bias = {};
    alignas(64) std::array<std::array<std::array<float, L1_SIZE * 2>, L2_SIZE>, OUTPUT_BUCKETS> l2_weight = {};
    alignas(64) std::array<std::array<float, L2_SIZE>, OUTPUT_BUCKETS> l2_bias = {};
    alignas(64) std::array<std::array<float, L2_SIZE / 2>, OUTPUT_BUCKETS> l3_weight = {};
    alignas(64) std::array<float, OUTPUT_BUCKETS> l3_bias = {};
};

auto shuffle_ft_neurons(const decltype(raw_network::ft_weight)& ft_weight,
    const decltype(raw_network::ft_bias)& ft_bias, const decltype(raw_network::l1_weight)& l1_weight)
{
    auto ft_weight_output = std::make_unique<decltype(raw_network::ft_weight)>();
    auto ft_bias_output = std::make_unique<decltype(raw_network::ft_bias)>();
    auto l1_weight_output = std::make_unique<decltype(raw_network::l1_weight)>();

#ifdef NETWORK_SHUFFLE
    *ft_weight_output = ft_weight;
    *ft_bias_output = ft_bias;
    *l1_weight_output = l1_weight;
#else
    // clang-format off
    constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 106, 6, 76, 594, 37, 493, 154, 246, 419, 722, 314, 723,
        271, 673, 403, 598, 109, 357, 206, 625, 384, 424, 397, 619, 58, 59, 40, 231, 707, 309, 648, 701, 752, 509, 174,
        5, 494, 364, 417, 766, 699, 342, 292, 402, 725, 210, 734, 578, 654, 170, 730, 510, 243, 283, 66, 31, 462, 335,
        344, 298, 151, 446, 558, 560, 48, 686, 376, 571, 200, 65, 764, 667, 129, 427, 595, 474, 622, 710, 68, 18, 499,
        318, 218, 331, 521, 377, 43, 167, 16, 234, 240, 614, 527, 349, 582, 293, 450, 519, 692, 719, 261, 634, 84, 385,
        721, 324, 150, 426, 140, 94, 656, 652, 522, 360, 720, 104, 393, 362, 36, 125, 630, 491, 302, 270, 275, 143, 533,
        480, 472, 305, 93, 643, 173, 20, 679, 735, 315, 149, 107, 131, 181, 247, 160, 638, 728, 127, 152, 442, 392, 751,
        557, 46, 555, 698, 609, 171, 225, 52, 138, 159, 682, 216, 758, 253, 9, 209, 646, 410, 382, 186, 653, 121, 79,
        592, 761, 91, 700, 470, 739, 600, 198, 486, 204, 334, 548, 281, 441, 586, 328, 188, 123, 155, 647, 405, 597,
        544, 192, 644, 350, 696, 205, 70, 307, 164, 356, 14, 388, 139, 148, 712, 74, 447, 660, 130, 449, 668, 300, 244,
        54, 531, 399, 476, 264, 641, 542, 69, 226, 87, 554, 659, 561, 655, 422, 579, 564, 97, 661, 703, 296, 687, 114,
        189, 537, 117, 433, 713, 551, 115, 639, 569, 623, 23, 473, 257, 56, 689, 251, 332, 51, 33, 47, 631, 451, 704,
        400, 239, 262, 593, 100, 112, 77, 215, 28, 617, 343, 456, 321, 276, 4, 492, 30, 620, 217, 628, 755, 618, 496,
        25, 706, 746, 581, 590, 303, 288, 235, 277, 254, 180, 322, 166, 2, 146, 67, 219, 512, 366, 306, 118, 562, 273,
        230, 438, 12, 507, 570, 733, 208, 657, 252, 368, 289, 431, 255, 323, 358, 222, 301, 326, 228, 395, 272, 201, 71,
        103, 565, 685, 280, 501, 711, 697, 729, 479, 41, 412, 133, 64, 21, 177, 645, 602, 266, 651, 702, 543, 237, 756,
        60, 29, 248, 352, 347, 61, 355, 39, 53, 227, 265, 740, 420, 390, 92, 310, 50, 161, 238, 134, 765, 336, 178, 90,
        88, 330, 745, 278, 113, 580, 57, 387, 466, 549, 738, 367, 743, 363, 313, 26, 195, 396, 45, 607, 506, 372, 401,
        370, 444, 515, 428, 517, 736, 408, 135, 409, 534, 294, 675, 716, 465, 108, 169, 81, 416, 452, 145, 78, 136, 38,
        425, 632, 337, 286, 126, 297, 514, 391, 224, 338, 1, 182, 695, 616, 666, 269, 691, 429, 62, 584, 371, 111, 213,
        394, 404, 511, 485, 615, 99, 650, 611, 669, 241, 459, 454, 458, 448, 44, 749, 726, 694, 463, 260, 207, 8, 471,
        375, 267, 249, 11, 308, 434, 535, 101, 359, 158, 165, 282, 717, 413, 211, 753, 245, 327, 552, 460, 538, 688, 83,
        443, 732, 731, 498, 197, 605, 464, 505, 503, 340, 223, 194, 279, 671, 489, 229, 3, 361, 502, 389, 528, 35, 513,
        516, 299, 102, 163, 432, 435, 233, 670, 526, 556, 316, 285, 530, 287, 455, 144, 524, 311, 599, 98, 468, 179,
        168, 518, 439, 757, 341, 232, 553, 268, 236, 747, 312, 128, 437, 19, 762, 42, 627, 504, 674, 183, 27, 203, 122,
        383, 539, 421, 365, 508, 141, 320, 411, 658, 55, 274, 574, 718, 345, 73, 85, 608, 153, 559, 105, 380, 214, 637,
        523, 119, 49, 588, 677, 536, 96, 566, 351, 407, 172, 187, 487, 573, 469, 258, 82, 381, 132, 378, 606, 604, 636,
        596, 488, 587, 176, 567, 649, 445, 626, 196, 714, 304, 563, 32, 86, 750, 22, 137, 15, 348, 475, 353, 767, 436,
        495, 633, 325, 193, 142, 545, 550, 414, 575, 603, 185, 423, 75, 568, 635, 72, 80, 0, 10, 24, 540, 221, 199, 329,
        440, 319, 664, 461, 220, 162, 490, 259, 290, 663, 665, 500, 684, 453, 715, 242, 63, 741, 577, 529, 295, 284,
        591, 291, 484, 642, 415, 110, 7, 481, 263, 759, 546, 120, 690, 576, 612, 250, 621, 467, 369, 541, 124, 373, 157,
        693, 175, 116, 398, 34, 672, 256, 624, 709, 676, 724, 640, 147, 754, 13, 572, 662, 202, 520, 532, 95, 418, 742,
        629, 339, 708, 727, 89, 610, 705, 333, 737, 497, 317, 525, 386, 184, 680, 601, 583, 478, 477, 17, 190, 457, 678,
        483, 191, 748, 547, 374, 681, 406, 482, 212, 354, 613, 346, 430, 760, 744, 683, 585, 379, 156, 589, 763,
    };
    // clang-format on

    constexpr auto is_valid_permutation = [](auto order) constexpr
    {
        std::ranges::sort(order);
        return std::ranges::equal(order, std::views::iota(size_t { 0 }, FT_SIZE / 2));
    };
    static_assert(is_valid_permutation(shuffle_order), "shuffle_order must be a valid permutation of [0, FT_SIZE / 2)");

    for (size_t i = 0; i < FT_SIZE / 2; i++)
    {
        auto old_idx = shuffle_order[i];

        (*ft_bias_output)[i] = ft_bias[old_idx];
        (*ft_bias_output)[i + FT_SIZE / 2] = ft_bias[old_idx + FT_SIZE / 2];

        for (size_t j = 0; j < INPUT_SIZE * KING_BUCKET_COUNT; j++)
        {
            (*ft_weight_output)[j][i] = ft_weight[j][old_idx];
            (*ft_weight_output)[j][i + FT_SIZE / 2] = ft_weight[j][old_idx + FT_SIZE / 2];
        }

        for (size_t j = 0; j < OUTPUT_BUCKETS; j++)
        {
            for (size_t k = 0; k < L1_SIZE; k++)
            {
                (*l1_weight_output)[j][k][i] = l1_weight[j][k][old_idx];
                (*l1_weight_output)[j][k][i + FT_SIZE / 2] = l1_weight[j][k][old_idx + FT_SIZE / 2];
            }
        }
    }
#endif
    return std::make_tuple(std::move(ft_weight_output), std::move(ft_bias_output), std::move(l1_weight_output));
}

auto adjust_for_packus(const decltype(raw_network::ft_weight)& ft_weight, const decltype(raw_network::ft_bias)& ft_bias)
{
    auto permuted_weight = std::make_unique<decltype(raw_network::ft_weight)>();
    auto permuted_bias = std::make_unique<decltype(raw_network::ft_bias)>();

#if defined(USE_AVX2)
    // shuffle around ft weights to match packus interleaving
    for (size_t i = 0; i < INPUT_SIZE * KING_BUCKET_COUNT; i++)
    {
        for (size_t j = 0; j < FT_SIZE; j += SIMD::vec_size)
        {
#if defined(USE_AVX512)
            constexpr std::array mapping = { 0, 4, 1, 5, 2, 6, 3, 7 };
#else
            constexpr std::array mapping = { 0, 2, 1, 3 };
#endif
            for (size_t x = 0; x < SIMD::vec_size; x++)
            {
                (*permuted_weight)[i][j + mapping[x / 8] * 8 + x % 8] = ft_weight[i][j + x];
                (*permuted_bias)[j + mapping[x / 8] * 8 + x % 8] = ft_bias[j + x];
            }
        }
    }
#else
    (*permuted_weight) = ft_weight;
    (*permuted_bias) = ft_bias;
#endif
    return std::make_tuple(std::move(permuted_weight), std::move(permuted_bias));
}

auto rescale_l1_bias(const decltype(raw_network::l1_bias)& input)
{
    auto output = std::make_unique<decltype(raw_network::l1_bias)>();

    // rescale l1 bias to account for FT_activation adjusting to 0-127 scale
    for (size_t i = 0; i < OUTPUT_BUCKETS; i++)
    {
        for (size_t j = 0; j < L1_SIZE; j++)
        {
            (*output)[i][j] = input[i][j] * 127 / FT_SCALE;
        }
    }

    return output;
}

auto interleave_for_l1_sparsity(const decltype(raw_network::l1_weight)& input)
{
    auto output = std::make_unique<std::array<std::array<int16_t, FT_SIZE * L1_SIZE>, OUTPUT_BUCKETS>>();

#if defined(SIMD_ENABLED)
    // transpose and interleave the weights in blocks of 4 FT activations
    for (size_t i = 0; i < OUTPUT_BUCKETS; i++)
    {
        for (size_t j = 0; j < L1_SIZE; j++)
        {
            for (size_t k = 0; k < FT_SIZE; k++)
            {
                // we want something that looks like:
                //[bucket][nibble][output][4]
                (*output)[i][(k / 4) * (4 * L1_SIZE) + (j * 4) + (k % 4)] = input[i][j][k];
            }
        }
    }
#else
    for (size_t i = 0; i < OUTPUT_BUCKETS; i++)
    {
        for (size_t j = 0; j < L1_SIZE; j++)
        {
            for (size_t k = 0; k < FT_SIZE; k++)
            {
                (*output)[i][j * FT_SIZE + k] = input[i][j][k];
            }
        }
    }
#endif
    return output;
}

auto cast_l1_weight_int8(const std::array<std::array<int16_t, FT_SIZE * L1_SIZE>, OUTPUT_BUCKETS>& input)
{
    auto output = std::make_unique<std::array<std::array<int8_t, FT_SIZE * L1_SIZE>, OUTPUT_BUCKETS>>();

    for (size_t i = 0; i < OUTPUT_BUCKETS; i++)
    {
        for (size_t j = 0; j < FT_SIZE * L1_SIZE; j++)
        {
            (*output)[i][j] = static_cast<int8_t>(input[i][j]);
        }
    }

    return output;
}

auto cast_l1_bias_int32(const decltype(raw_network::l1_bias)& input)
{
    auto output = std::make_unique<std::array<std::array<int32_t, L1_SIZE>, OUTPUT_BUCKETS>>();

    for (size_t i = 0; i < OUTPUT_BUCKETS; i++)
    {
        for (size_t j = 0; j < L1_SIZE; j++)
        {
            (*output)[i][j] = static_cast<int32_t>(input[i][j]);
        }
    }

    return output;
}

auto transpose_l2_weights(const decltype(raw_network::l2_weight)& input)
{
    auto output = std::make_unique<std::array<std::array<std::array<float, L2_SIZE>, L1_SIZE * 2>, OUTPUT_BUCKETS>>();

    for (size_t i = 0; i < OUTPUT_BUCKETS; i++)
    {
        for (size_t j = 0; j < L1_SIZE * 2; j++)
        {
            for (size_t k = 0; k < L2_SIZE; k++)
            {
                (*output)[i][j][k] = input[i][k][j];
            }
        }
    }

    return output;
}

}

using namespace NN;

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cout << "Error: expected 2 command line arguments <input> and <output>" << std::endl;
        return EXIT_FAILURE;
    }

    if (auto size = std::filesystem::file_size(argv[1]); size != sizeof(raw_network))
    {
        std::cout << "Error: expected to read " << sizeof(raw_network) << " bytes but file size is " << size << " bytes"
                  << std::endl;
        return EXIT_FAILURE;
    }

    auto raw_net = std::make_unique<raw_network>();
    std::ifstream in(argv[1], std::ios::binary);
    in.read(reinterpret_cast<char*>(raw_net.get()), sizeof(raw_network));

    auto [ft_weight, ft_bias, l1_weight] = shuffle_ft_neurons(raw_net->ft_weight, raw_net->ft_bias, raw_net->l1_weight);
    auto [ft_weight2, ft_bias2] = adjust_for_packus(*ft_weight, *ft_bias);
    auto final_net = std::make_unique<network>();
    final_net->ft_weight = *ft_weight2;
    final_net->ft_bias = *ft_bias2;
    final_net->l1_weight = *cast_l1_weight_int8(*interleave_for_l1_sparsity(*l1_weight));
    final_net->l1_bias = *cast_l1_bias_int32(*rescale_l1_bias(raw_net->l1_bias));
    final_net->l2_weight = *transpose_l2_weights(raw_net->l2_weight);
    final_net->l2_bias = raw_net->l2_bias;
    final_net->l3_weight = raw_net->l3_weight;
    final_net->l3_bias = raw_net->l3_bias;

    std::ofstream out(argv[2], std::ios::binary);
    out.write(reinterpret_cast<const char*>(final_net.get()), sizeof(network));

    std::cout << "Created embedded network" << std::endl;
}