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
    constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 718, 230, 763, 680, 731, 567, 435, 218, 412, 481, 764,
        529, 546, 209, 10, 493, 618, 665, 744, 9, 160, 349, 441, 107, 673, 712, 456, 179, 309, 385, 169, 574, 91, 564,
        535, 537, 626, 341, 512, 422, 362, 394, 722, 663, 373, 580, 240, 703, 23, 96, 669, 478, 339, 462, 201, 236, 488,
        591, 443, 171, 498, 41, 592, 31, 270, 58, 296, 690, 56, 687, 325, 277, 692, 149, 601, 43, 442, 4, 256, 399, 629,
        548, 87, 51, 198, 743, 76, 15, 312, 475, 88, 158, 224, 741, 27, 163, 170, 16, 267, 636, 730, 543, 266, 215, 666,
        387, 749, 707, 490, 19, 2, 14, 146, 330, 210, 20, 273, 477, 84, 228, 582, 589, 383, 350, 675, 765, 642, 645,
        501, 487, 706, 577, 144, 302, 68, 614, 505, 132, 178, 746, 238, 566, 142, 137, 596, 534, 104, 409, 565, 372,
        374, 587, 655, 694, 151, 623, 661, 732, 173, 753, 0, 569, 290, 733, 206, 318, 207, 85, 729, 440, 742, 677, 237,
        101, 301, 59, 497, 75, 660, 450, 515, 552, 307, 204, 136, 338, 324, 664, 759, 568, 608, 112, 616, 738, 653, 195,
        613, 5, 79, 311, 406, 585, 249, 188, 576, 251, 340, 368, 185, 519, 678, 520, 652, 298, 126, 735, 113, 354, 502,
        575, 445, 80, 684, 400, 86, 316, 196, 359, 18, 308, 526, 221, 539, 39, 233, 223, 472, 71, 586, 235, 82, 670, 30,
        705, 175, 7, 220, 181, 447, 166, 242, 250, 748, 211, 147, 327, 751, 583, 50, 231, 605, 131, 468, 563, 263, 28,
        129, 99, 322, 734, 245, 495, 120, 216, 646, 473, 49, 200, 424, 454, 415, 516, 717, 92, 283, 506, 125, 174, 110,
        656, 282, 320, 649, 740, 485, 247, 654, 229, 93, 615, 390, 40, 278, 544, 194, 609, 89, 323, 289, 46, 272, 728,
        484, 70, 292, 190, 121, 138, 747, 336, 122, 305, 226, 500, 510, 187, 438, 275, 762, 724, 480, 276, 686, 162,
        594, 197, 693, 631, 757, 641, 212, 711, 74, 329, 60, 164, 375, 674, 710, 264, 244, 647, 595, 471, 619, 119, 356,
        123, 395, 165, 460, 279, 243, 363, 239, 434, 154, 12, 382, 94, 486, 371, 551, 271, 342, 118, 260, 376, 366, 433,
        683, 353, 37, 429, 111, 34, 98, 335, 214, 116, 203, 549, 702, 713, 401, 95, 364, 313, 398, 672, 562, 246, 219,
        25, 700, 620, 760, 635, 334, 255, 310, 143, 315, 413, 517, 358, 463, 333, 303, 33, 300, 657, 420, 90, 449, 105,
        508, 42, 106, 439, 726, 29, 659, 306, 369, 183, 13, 436, 427, 213, 161, 397, 554, 632, 156, 32, 584, 431, 176,
        719, 361, 426, 411, 407, 671, 572, 423, 103, 384, 284, 421, 417, 360, 184, 699, 159, 404, 297, 457, 455, 469,
        225, 232, 704, 268, 64, 476, 593, 408, 321, 459, 291, 444, 135, 257, 299, 241, 328, 348, 604, 437, 418, 633,
        386, 287, 167, 617, 545, 281, 556, 177, 100, 446, 503, 134, 63, 610, 492, 716, 344, 326, 139, 513, 130, 410,
        451, 489, 467, 448, 453, 509, 3, 21, 377, 269, 248, 337, 402, 482, 523, 365, 343, 192, 286, 345, 514, 26, 36,
        538, 217, 332, 536, 392, 396, 370, 304, 541, 227, 688, 679, 55, 625, 128, 378, 48, 504, 253, 464, 261, 542, 639,
        379, 667, 153, 662, 133, 389, 54, 643, 388, 351, 697, 274, 547, 573, 155, 681, 182, 528, 507, 624, 367, 558,
        317, 560, 73, 259, 222, 6, 755, 644, 578, 530, 150, 258, 531, 607, 148, 8, 561, 725, 152, 570, 603, 721, 393,
        606, 294, 470, 494, 727, 525, 293, 720, 78, 202, 17, 698, 265, 47, 736, 285, 114, 588, 45, 600, 295, 612, 102,
        234, 630, 689, 521, 127, 186, 465, 83, 758, 637, 11, 66, 708, 754, 1, 696, 750, 579, 466, 262, 254, 331, 511,
        77, 555, 479, 648, 141, 314, 658, 145, 634, 599, 621, 117, 533, 140, 650, 638, 280, 191, 67, 559, 499, 62, 745,
        180, 668, 72, 425, 527, 357, 391, 319, 651, 627, 189, 761, 115, 430, 81, 715, 676, 108, 458, 590, 628, 403, 97,
        685, 157, 352, 701, 380, 622, 428, 557, 723, 405, 38, 414, 767, 691, 709, 496, 714, 739, 61, 524, 419, 611, 355,
        598, 65, 483, 432, 52, 461, 695, 44, 550, 347, 35, 199, 581, 193, 452, 532, 737, 682, 172, 53, 553, 124, 168,
        491, 474, 540, 205, 416, 22, 640, 597, 346, 252, 756, 69, 752, 208, 518, 109, 24, 381, 571, 288, 57, 522, 766,
        602,
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