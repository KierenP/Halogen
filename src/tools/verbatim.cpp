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
    constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 696, 398, 471, 665, 572, 615, 596, 240, 167, 650, 179,
        641, 45, 664, 411, 677, 252, 523, 392, 87, 338, 133, 662, 315, 318, 728, 562, 766, 56, 754, 288, 624, 380, 192,
        377, 616, 747, 625, 400, 362, 278, 629, 270, 553, 272, 592, 465, 172, 733, 486, 686, 409, 438, 507, 205, 170,
        727, 33, 49, 6, 175, 127, 90, 26, 250, 130, 208, 470, 753, 674, 120, 229, 506, 730, 29, 74, 393, 50, 645, 414,
        680, 714, 52, 82, 343, 692, 695, 580, 444, 731, 44, 330, 77, 230, 395, 334, 397, 451, 543, 712, 652, 185, 469,
        299, 84, 37, 226, 238, 410, 213, 341, 742, 467, 212, 247, 729, 236, 544, 603, 116, 504, 119, 105, 178, 195, 490,
        146, 659, 707, 347, 384, 635, 700, 75, 518, 436, 608, 344, 182, 520, 765, 63, 494, 48, 253, 262, 174, 231, 14,
        19, 166, 91, 457, 76, 738, 85, 314, 617, 32, 584, 376, 124, 396, 131, 658, 546, 382, 342, 661, 255, 459, 472,
        637, 336, 710, 62, 488, 429, 224, 433, 746, 759, 102, 147, 191, 115, 402, 21, 86, 89, 673, 670, 415, 164, 138,
        280, 521, 241, 755, 493, 609, 190, 346, 421, 717, 244, 54, 767, 169, 531, 358, 41, 530, 97, 585, 113, 257, 22,
        218, 200, 634, 58, 157, 361, 752, 587, 388, 335, 581, 474, 452, 514, 367, 135, 644, 210, 638, 171, 64, 51, 533,
        176, 140, 636, 35, 428, 10, 110, 142, 640, 512, 439, 498, 383, 0, 134, 589, 655, 129, 648, 12, 121, 227, 34,
        462, 173, 704, 259, 261, 18, 683, 168, 73, 339, 463, 15, 273, 36, 323, 527, 20, 485, 222, 275, 2, 292, 529, 597,
        65, 289, 643, 204, 265, 149, 568, 352, 579, 646, 329, 416, 515, 98, 128, 760, 691, 607, 375, 194, 385, 150, 749,
        237, 681, 305, 233, 313, 25, 356, 136, 156, 757, 724, 38, 197, 633, 126, 283, 1, 308, 763, 211, 285, 582, 660,
        8, 561, 590, 3, 17, 373, 737, 598, 721, 623, 286, 112, 165, 235, 758, 294, 594, 331, 69, 508, 363, 198, 296,
        317, 586, 460, 345, 606, 539, 47, 622, 217, 725, 306, 509, 764, 709, 371, 612, 489, 739, 225, 651, 477, 663,
        675, 736, 481, 500, 734, 360, 423, 295, 66, 484, 478, 177, 337, 282, 206, 31, 542, 188, 364, 144, 480, 555, 269,
        151, 109, 513, 404, 672, 496, 24, 267, 406, 293, 390, 248, 541, 158, 693, 750, 499, 111, 690, 228, 628, 79, 427,
        718, 234, 155, 653, 249, 61, 431, 422, 23, 351, 327, 567, 424, 418, 284, 287, 291, 430, 412, 307, 184, 445, 745,
        434, 545, 505, 99, 67, 443, 141, 455, 181, 676, 715, 125, 263, 630, 71, 80, 534, 180, 762, 310, 756, 437, 316,
        94, 274, 378, 656, 154, 446, 359, 46, 321, 357, 207, 139, 220, 461, 16, 387, 487, 93, 583, 214, 491, 468, 107,
        145, 495, 454, 719, 152, 666, 311, 535, 309, 503, 618, 96, 456, 420, 492, 162, 708, 368, 88, 5, 122, 301, 619,
        13, 320, 43, 560, 403, 260, 161, 28, 254, 350, 381, 266, 199, 466, 106, 117, 642, 476, 386, 108, 554, 528, 391,
        302, 604, 713, 322, 549, 536, 370, 4, 761, 101, 183, 369, 573, 565, 57, 678, 366, 215, 300, 219, 189, 558, 394,
        242, 251, 475, 408, 365, 649, 575, 570, 571, 441, 449, 442, 559, 741, 353, 706, 53, 432, 159, 319, 148, 525,
        464, 39, 9, 30, 556, 576, 510, 548, 699, 72, 426, 232, 497, 627, 611, 328, 435, 743, 705, 447, 324, 221, 60,
        588, 304, 326, 137, 600, 647, 599, 399, 70, 42, 684, 405, 153, 479, 201, 563, 258, 40, 440, 203, 92, 245, 271,
        340, 501, 59, 591, 114, 202, 511, 532, 620, 716, 657, 143, 303, 551, 482, 550, 671, 123, 522, 502, 483, 193,
        246, 574, 100, 374, 726, 132, 702, 517, 239, 602, 7, 735, 685, 538, 667, 639, 669, 610, 256, 95, 332, 578, 103,
        401, 243, 540, 223, 419, 711, 417, 593, 595, 613, 55, 687, 118, 689, 264, 621, 78, 333, 566, 312, 187, 626, 196,
        526, 723, 11, 268, 701, 654, 160, 688, 83, 516, 81, 348, 448, 325, 68, 631, 694, 277, 379, 413, 276, 216, 209,
        450, 697, 354, 524, 186, 682, 577, 298, 458, 453, 349, 279, 703, 564, 605, 748, 519, 614, 355, 407, 668, 632,
        720, 698, 281, 290, 372, 27, 104, 751, 601, 297, 547, 722, 679, 163, 389, 425, 552, 537, 744, 740, 569, 557,
        732, 473,
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