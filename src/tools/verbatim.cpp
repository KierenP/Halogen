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
    alignas(64) std::array<std::array<float, L2_SIZE>, OUTPUT_BUCKETS> l3_weight = {};
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
    constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 479, 446, 78, 51, 601, 599, 677, 172, 66, 481, 133,
        727, 113, 77, 269, 393, 692, 170, 141, 749, 560, 699, 282, 272, 65, 398, 708, 313, 571, 378, 159, 759, 50, 14,
        701, 657, 167, 121, 148, 556, 406, 645, 240, 212, 555, 751, 31, 369, 358, 687, 728, 225, 580, 548, 762, 55, 738,
        33, 522, 253, 485, 371, 23, 652, 410, 27, 545, 767, 136, 694, 68, 71, 230, 700, 74, 710, 674, 234, 0, 558, 314,
        596, 733, 83, 679, 569, 547, 41, 486, 125, 621, 433, 243, 325, 94, 273, 724, 589, 474, 740, 287, 458, 198, 347,
        604, 178, 330, 90, 475, 625, 754, 702, 102, 616, 675, 387, 21, 135, 673, 356, 493, 425, 561, 681, 306, 32, 126,
        534, 429, 194, 312, 157, 472, 440, 451, 551, 590, 278, 295, 37, 105, 758, 116, 682, 602, 422, 342, 255, 706,
        613, 142, 698, 231, 736, 470, 256, 115, 176, 632, 435, 327, 718, 86, 308, 197, 92, 348, 24, 688, 464, 367, 651,
        168, 100, 174, 76, 233, 119, 521, 67, 707, 202, 684, 538, 650, 124, 58, 26, 401, 140, 290, 508, 289, 644, 110,
        195, 266, 128, 271, 132, 352, 704, 442, 45, 391, 1, 144, 146, 577, 54, 587, 262, 19, 678, 57, 53, 432, 503, 663,
        421, 359, 208, 249, 161, 246, 213, 737, 414, 690, 214, 386, 181, 394, 107, 624, 129, 91, 455, 512, 741, 715,
        300, 44, 353, 553, 220, 668, 9, 726, 311, 165, 106, 199, 147, 4, 229, 232, 29, 117, 204, 257, 201, 221, 48, 108,
        711, 61, 554, 492, 514, 691, 211, 38, 317, 506, 275, 364, 667, 252, 277, 247, 186, 59, 461, 335, 665, 416, 222,
        491, 731, 384, 280, 63, 64, 695, 18, 305, 244, 575, 405, 362, 139, 122, 649, 466, 158, 104, 15, 397, 250, 697,
        504, 430, 488, 301, 626, 69, 85, 489, 720, 399, 228, 152, 437, 163, 324, 322, 423, 245, 328, 578, 258, 205, 428,
        640, 597, 757, 331, 750, 120, 338, 279, 517, 443, 149, 189, 566, 339, 583, 114, 354, 207, 628, 285, 284, 717,
        291, 321, 151, 752, 471, 355, 17, 11, 154, 765, 270, 224, 714, 263, 524, 52, 261, 732, 477, 375, 349, 370, 540,
        622, 84, 381, 716, 319, 187, 411, 341, 259, 483, 669, 160, 703, 568, 683, 173, 518, 383, 420, 3, 366, 385, 426,
        334, 131, 507, 460, 392, 22, 549, 390, 766, 49, 495, 28, 242, 511, 647, 482, 75, 294, 111, 299, 490, 82, 705,
        337, 413, 764, 332, 303, 179, 723, 164, 633, 298, 265, 388, 379, 525, 177, 6, 576, 654, 763, 496, 584, 276, 444,
        742, 190, 408, 326, 452, 502, 454, 99, 445, 441, 88, 404, 449, 400, 655, 463, 153, 567, 185, 546, 594, 350, 46,
        530, 206, 407, 281, 344, 343, 476, 743, 735, 345, 373, 118, 670, 309, 292, 666, 615, 603, 585, 267, 536, 639,
        434, 494, 612, 544, 89, 527, 427, 722, 180, 25, 192, 431, 581, 210, 283, 241, 62, 97, 501, 760, 40, 316, 499,
        634, 478, 235, 34, 227, 12, 368, 323, 505, 686, 526, 412, 296, 605, 183, 520, 592, 450, 533, 620, 656, 223, 515,
        537, 550, 761, 193, 200, 13, 439, 333, 539, 81, 498, 513, 70, 552, 542, 519, 288, 396, 346, 509, 382, 500, 497,
        562, 563, 209, 175, 459, 680, 2, 418, 510, 395, 47, 307, 184, 438, 238, 447, 572, 623, 734, 532, 403, 453, 188,
        637, 217, 109, 30, 5, 557, 8, 653, 535, 137, 595, 696, 693, 95, 415, 389, 600, 79, 756, 409, 725, 559, 753, 586,
        609, 543, 216, 155, 304, 755, 614, 719, 607, 593, 611, 36, 239, 56, 582, 631, 627, 103, 484, 218, 467, 318, 260,
        268, 215, 10, 748, 98, 169, 20, 642, 573, 156, 462, 130, 689, 237, 618, 643, 196, 361, 456, 363, 72, 60, 635,
        516, 264, 402, 310, 419, 7, 648, 523, 630, 286, 610, 16, 203, 424, 588, 248, 659, 528, 143, 641, 372, 730, 629,
        564, 480, 638, 570, 661, 42, 315, 376, 360, 469, 529, 658, 254, 87, 636, 123, 436, 617, 145, 293, 598, 236, 351,
        93, 531, 457, 565, 101, 374, 672, 685, 579, 80, 541, 448, 171, 712, 664, 138, 219, 746, 606, 660, 251, 226, 357,
        713, 646, 274, 591, 676, 297, 671, 73, 150, 127, 162, 35, 465, 96, 329, 380, 721, 619, 191, 336, 340, 302, 608,
        747, 468, 745, 739, 182, 417, 166, 112, 473, 744, 662, 709, 729, 320, 377, 134, 39, 574, 43, 487, 365,
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