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
    constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 117, 443, 692, 74, 255, 261, 764, 7, 319, 82, 221, 557,
        14, 36, 3, 765, 714, 506, 465, 263, 733, 364, 279, 105, 132, 553, 676, 239, 486, 370, 228, 358, 456, 673, 186,
        476, 86, 487, 499, 517, 433, 126, 442, 688, 696, 545, 170, 119, 20, 516, 744, 141, 35, 731, 605, 303, 81, 627,
        753, 59, 88, 667, 608, 546, 759, 127, 445, 589, 559, 555, 493, 284, 464, 339, 713, 4, 575, 611, 562, 252, 410,
        63, 362, 113, 335, 85, 645, 422, 707, 574, 290, 369, 399, 754, 324, 165, 140, 288, 248, 366, 607, 529, 310, 539,
        591, 735, 662, 417, 313, 760, 282, 242, 77, 203, 651, 95, 416, 473, 41, 150, 147, 705, 97, 121, 124, 470, 162,
        154, 217, 60, 755, 519, 37, 94, 388, 706, 404, 481, 294, 2, 245, 106, 142, 185, 48, 139, 474, 337, 548, 511,
        525, 47, 177, 722, 494, 699, 396, 582, 180, 573, 675, 161, 199, 739, 477, 711, 164, 671, 64, 23, 30, 421, 87,
        526, 341, 293, 636, 51, 387, 558, 321, 716, 107, 414, 497, 674, 725, 317, 613, 701, 174, 50, 479, 385, 138, 518,
        601, 108, 419, 403, 237, 21, 758, 62, 322, 524, 49, 343, 532, 208, 209, 251, 703, 386, 763, 726, 405, 538, 118,
        402, 757, 345, 102, 214, 342, 655, 243, 200, 12, 679, 219, 229, 767, 756, 400, 502, 383, 715, 587, 0, 577, 196,
        515, 235, 22, 38, 740, 53, 356, 193, 654, 304, 610, 309, 331, 206, 488, 131, 509, 660, 432, 738, 183, 286, 378,
        91, 159, 431, 371, 325, 350, 210, 411, 289, 163, 595, 452, 152, 690, 273, 628, 728, 449, 249, 438, 312, 643,
        418, 527, 585, 54, 471, 292, 270, 514, 618, 172, 576, 352, 262, 436, 231, 233, 547, 254, 606, 5, 641, 454, 182,
        702, 302, 89, 285, 463, 68, 677, 204, 729, 543, 415, 46, 318, 640, 115, 700, 192, 336, 752, 197, 226, 158, 634,
        491, 395, 745, 373, 522, 277, 550, 469, 580, 624, 623, 617, 207, 571, 326, 504, 347, 423, 646, 338, 297, 512,
        329, 435, 709, 355, 301, 114, 26, 116, 361, 682, 225, 1, 650, 151, 278, 736, 269, 241, 718, 222, 316, 394, 274,
        211, 148, 298, 295, 344, 190, 450, 578, 365, 307, 42, 712, 390, 287, 275, 698, 171, 72, 391, 272, 92, 666, 447,
        684, 603, 27, 720, 717, 93, 57, 458, 537, 503, 76, 446, 334, 181, 583, 681, 567, 259, 426, 351, 541, 187, 332,
        424, 155, 175, 134, 425, 257, 572, 427, 377, 11, 742, 129, 353, 430, 359, 19, 16, 167, 398, 220, 401, 614, 265,
        168, 28, 409, 389, 460, 630, 563, 43, 31, 455, 407, 305, 565, 566, 110, 393, 626, 642, 247, 201, 99, 55, 98,
        693, 291, 198, 656, 638, 101, 149, 528, 510, 612, 609, 462, 69, 271, 429, 80, 169, 708, 306, 453, 490, 34, 281,
        29, 508, 346, 483, 561, 498, 52, 296, 33, 420, 615, 751, 505, 145, 78, 620, 413, 668, 218, 737, 412, 75, 40,
        459, 485, 260, 594, 202, 232, 689, 330, 599, 17, 157, 90, 440, 216, 178, 166, 360, 480, 380, 434, 44, 746, 406,
        549, 741, 122, 323, 84, 104, 441, 58, 123, 354, 678, 691, 437, 246, 256, 224, 227, 333, 379, 327, 533, 669, 25,
        65, 492, 451, 600, 501, 629, 45, 697, 500, 250, 111, 556, 544, 625, 311, 100, 39, 230, 236, 687, 570, 596, 320,
        680, 661, 581, 381, 579, 109, 535, 212, 375, 619, 13, 584, 376, 659, 24, 622, 482, 523, 238, 253, 195, 457, 10,
        734, 507, 205, 495, 244, 267, 299, 472, 484, 616, 349, 160, 189, 723, 15, 188, 300, 215, 194, 686, 71, 213, 761,
        727, 66, 372, 593, 631, 120, 685, 588, 597, 357, 348, 653, 704, 604, 644, 568, 308, 176, 83, 363, 621, 534, 8,
        652, 137, 367, 657, 223, 478, 103, 489, 530, 61, 649, 444, 719, 637, 721, 112, 564, 96, 179, 635, 670, 496, 153,
        762, 665, 276, 6, 632, 328, 133, 240, 695, 184, 128, 551, 461, 569, 315, 283, 382, 266, 475, 747, 663, 32, 590,
        743, 156, 672, 513, 521, 70, 314, 683, 639, 664, 79, 136, 560, 191, 531, 724, 592, 732, 694, 67, 143, 264, 144,
        466, 408, 428, 710, 633, 552, 648, 125, 602, 536, 766, 730, 18, 520, 73, 368, 234, 542, 598, 384, 586, 397, 392,
        173, 258, 467, 748, 554, 340, 448, 439, 135, 374, 130, 658, 56, 280, 540, 9, 749, 146, 750, 268, 647, 468,
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