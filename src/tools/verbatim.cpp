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
    constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 357, 487, 194, 637, 342, 177, 443, 413, 370, 335, 388,
        291, 263, 456, 14, 346, 742, 412, 577, 644, 665, 446, 587, 331, 390, 423, 226, 157, 202, 71, 16, 56, 734, 630,
        621, 462, 478, 11, 242, 434, 766, 651, 668, 659, 380, 547, 347, 4, 158, 212, 334, 767, 568, 392, 248, 179, 99,
        674, 49, 551, 667, 352, 590, 249, 589, 348, 227, 167, 521, 58, 643, 356, 377, 261, 66, 399, 134, 149, 364, 741,
        600, 340, 168, 156, 550, 266, 671, 616, 429, 539, 532, 393, 468, 73, 437, 92, 216, 3, 85, 311, 529, 411, 704,
        407, 119, 258, 576, 107, 476, 508, 751, 161, 717, 710, 209, 684, 419, 558, 606, 660, 500, 28, 438, 678, 204,
        642, 506, 705, 41, 128, 454, 760, 663, 516, 435, 417, 141, 40, 33, 84, 301, 118, 486, 210, 647, 166, 540, 608,
        698, 83, 575, 750, 97, 649, 634, 586, 42, 743, 526, 114, 628, 686, 277, 108, 530, 641, 17, 162, 582, 7, 169,
        554, 101, 69, 302, 132, 622, 482, 44, 146, 497, 439, 712, 563, 189, 233, 287, 2, 321, 436, 187, 560, 37, 116,
        126, 605, 354, 254, 713, 137, 70, 648, 5, 165, 231, 185, 10, 314, 345, 192, 652, 510, 329, 711, 736, 27, 32,
        662, 138, 693, 613, 537, 163, 96, 172, 241, 525, 715, 384, 282, 365, 299, 327, 448, 420, 758, 262, 236, 211,
        371, 658, 36, 548, 77, 418, 152, 125, 756, 424, 289, 323, 545, 339, 153, 645, 578, 133, 217, 376, 113, 294, 112,
        740, 74, 264, 29, 219, 238, 224, 285, 234, 19, 46, 481, 273, 475, 315, 465, 270, 567, 718, 394, 281, 284, 274,
        253, 286, 533, 193, 246, 499, 453, 267, 469, 279, 617, 124, 701, 136, 175, 300, 593, 81, 303, 245, 700, 145,
        592, 307, 95, 522, 425, 90, 257, 221, 762, 517, 430, 272, 561, 160, 223, 697, 57, 64, 51, 235, 536, 288, 633,
        319, 198, 610, 275, 696, 353, 180, 59, 45, 320, 214, 61, 271, 332, 173, 247, 640, 552, 564, 94, 244, 729, 333,
        512, 549, 184, 265, 52, 100, 358, 215, 757, 229, 106, 366, 13, 298, 151, 350, 23, 87, 78, 296, 579, 669, 363,
        31, 34, 203, 328, 746, 656, 326, 129, 117, 538, 604, 360, 344, 389, 670, 707, 276, 308, 50, 632, 208, 397, 336,
        596, 9, 458, 442, 503, 479, 318, 79, 322, 218, 473, 367, 355, 714, 733, 54, 722, 676, 369, 467, 624, 256, 612,
        395, 636, 280, 749, 426, 111, 387, 207, 666, 618, 109, 88, 724, 309, 121, 324, 375, 597, 200, 25, 571, 337, 230,
        679, 557, 232, 55, 570, 359, 186, 457, 452, 381, 445, 306, 402, 240, 98, 313, 103, 535, 463, 694, 48, 485, 408,
        142, 105, 297, 228, 761, 415, 731, 639, 524, 681, 528, 703, 252, 122, 295, 626, 484, 765, 338, 623, 492, 427,
        386, 489, 80, 466, 504, 143, 583, 325, 569, 598, 654, 527, 502, 181, 62, 544, 147, 38, 542, 509, 732, 720, 150,
        495, 655, 441, 404, 341, 312, 398, 131, 372, 591, 176, 6, 123, 752, 464, 474, 292, 405, 682, 520, 361, 104, 447,
        687, 20, 68, 12, 383, 433, 368, 543, 706, 182, 422, 755, 431, 171, 531, 599, 460, 449, 53, 505, 745, 196, 373,
        164, 259, 255, 135, 763, 76, 290, 581, 546, 541, 278, 739, 304, 396, 170, 638, 635, 75, 566, 391, 653, 206, 268,
        382, 115, 695, 585, 491, 199, 190, 1, 421, 753, 664, 602, 511, 451, 385, 260, 378, 401, 178, 493, 584, 673, 580,
        483, 379, 553, 67, 222, 374, 603, 65, 611, 614, 414, 494, 82, 588, 93, 601, 498, 470, 139, 625, 154, 514, 747,
        400, 35, 573, 155, 251, 403, 534, 183, 627, 30, 237, 197, 672, 631, 432, 595, 89, 250, 615, 519, 607, 657, 220,
        459, 490, 130, 496, 43, 24, 140, 127, 690, 416, 428, 523, 239, 518, 609, 764, 159, 305, 480, 444, 310, 243, 650,
        677, 72, 688, 39, 148, 461, 572, 410, 174, 471, 349, 406, 0, 683, 26, 562, 225, 574, 691, 565, 738, 472, 102,
        723, 513, 699, 144, 719, 317, 702, 195, 620, 409, 60, 629, 675, 8, 269, 293, 477, 201, 283, 351, 716, 86, 455,
        744, 205, 91, 488, 680, 725, 191, 727, 728, 501, 559, 110, 689, 507, 22, 735, 450, 120, 330, 362, 188, 555, 47,
        15, 721, 343, 18, 759, 316, 594, 730, 737, 692, 556, 754, 661, 63, 685, 619, 213, 726, 708, 440, 515, 748, 709,
        21, 646,
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