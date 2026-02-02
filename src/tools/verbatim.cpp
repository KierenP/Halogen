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
    constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 426, 234, 584, 539, 353, 249, 55, 541, 297, 42, 719, 8,
        387, 703, 372, 218, 559, 99, 439, 417, 600, 650, 307, 288, 504, 523, 159, 666, 131, 40, 35, 6, 300, 711, 714,
        118, 534, 685, 243, 721, 317, 125, 270, 554, 712, 575, 675, 340, 225, 637, 135, 595, 258, 328, 31, 723, 683,
        689, 569, 394, 7, 664, 750, 0, 415, 395, 324, 404, 682, 436, 138, 756, 614, 507, 582, 550, 82, 524, 206, 210,
        762, 733, 199, 386, 542, 609, 230, 262, 336, 119, 475, 474, 418, 730, 375, 501, 351, 746, 720, 172, 495, 236,
        459, 546, 23, 273, 642, 742, 381, 483, 548, 389, 613, 552, 633, 51, 60, 321, 538, 9, 22, 116, 130, 729, 150,
        580, 527, 356, 192, 341, 463, 244, 252, 228, 186, 121, 96, 32, 134, 376, 117, 141, 414, 697, 108, 36, 578, 112,
        758, 497, 168, 616, 146, 649, 379, 153, 532, 626, 586, 88, 247, 496, 57, 232, 260, 322, 286, 624, 739, 759, 562,
        129, 489, 306, 450, 419, 670, 89, 409, 632, 638, 330, 102, 162, 333, 185, 695, 86, 601, 319, 56, 132, 343, 327,
        740, 13, 492, 46, 65, 179, 24, 528, 202, 151, 109, 256, 467, 570, 573, 511, 26, 61, 70, 468, 693, 728, 27, 38,
        177, 309, 493, 197, 427, 154, 456, 620, 205, 348, 700, 590, 29, 137, 707, 227, 274, 45, 677, 503, 378, 521, 217,
        155, 673, 302, 239, 246, 223, 741, 320, 408, 181, 59, 692, 515, 540, 187, 123, 751, 126, 200, 660, 429, 303,
        440, 287, 101, 115, 425, 188, 69, 602, 235, 699, 407, 53, 72, 690, 572, 66, 164, 382, 767, 653, 221, 207, 705,
        585, 526, 224, 208, 182, 509, 166, 173, 709, 193, 194, 47, 416, 581, 764, 64, 731, 749, 142, 163, 33, 145, 485,
        161, 143, 423, 561, 301, 364, 71, 316, 39, 84, 77, 211, 167, 74, 679, 727, 176, 219, 315, 583, 443, 284, 681,
        310, 659, 598, 335, 332, 299, 641, 5, 11, 373, 665, 283, 352, 458, 516, 250, 680, 220, 281, 349, 253, 347, 318,
        525, 124, 357, 329, 133, 17, 10, 63, 350, 702, 127, 170, 279, 18, 491, 195, 383, 215, 355, 591, 743, 4, 647,
        752, 462, 621, 765, 537, 747, 359, 691, 344, 611, 684, 113, 294, 571, 517, 393, 452, 392, 449, 555, 266, 672,
        314, 652, 360, 606, 744, 49, 473, 380, 342, 214, 410, 734, 43, 15, 178, 144, 519, 453, 191, 629, 198, 398, 628,
        405, 543, 48, 81, 454, 34, 556, 533, 428, 365, 368, 434, 646, 499, 451, 639, 98, 421, 441, 334, 105, 204, 445,
        291, 484, 391, 748, 457, 437, 196, 763, 30, 21, 636, 651, 605, 446, 603, 37, 662, 19, 482, 674, 276, 271, 430,
        257, 3, 190, 465, 736, 226, 676, 469, 476, 477, 617, 366, 80, 305, 52, 753, 290, 455, 209, 420, 535, 156, 87,
        169, 478, 634, 269, 557, 615, 78, 640, 565, 184, 345, 203, 413, 622, 58, 265, 471, 619, 713, 442, 461, 296, 255,
        488, 171, 529, 1, 241, 518, 157, 577, 490, 510, 92, 140, 487, 377, 472, 710, 531, 280, 667, 396, 422, 285, 325,
        311, 152, 326, 645, 574, 369, 536, 268, 725, 438, 648, 694, 715, 367, 385, 222, 481, 688, 67, 513, 264, 110,
        374, 512, 412, 411, 100, 201, 95, 464, 593, 732, 312, 277, 466, 724, 25, 361, 403, 470, 363, 2, 500, 544, 576,
        671, 370, 136, 183, 106, 553, 354, 563, 735, 766, 592, 120, 213, 447, 362, 400, 596, 73, 242, 158, 480, 594,
        174, 761, 607, 75, 738, 180, 189, 618, 745, 597, 520, 494, 79, 128, 390, 331, 560, 238, 567, 14, 50, 62, 547,
        267, 160, 323, 698, 28, 631, 401, 68, 402, 669, 630, 295, 432, 289, 85, 564, 599, 240, 371, 643, 292, 263, 104,
        635, 229, 388, 122, 212, 431, 433, 656, 506, 558, 259, 505, 486, 76, 275, 444, 97, 658, 701, 704, 175, 706, 686,
        272, 147, 20, 668, 627, 313, 251, 587, 298, 93, 338, 282, 54, 508, 545, 231, 623, 612, 41, 655, 479, 522, 139,
        308, 397, 608, 148, 549, 448, 579, 610, 83, 90, 406, 678, 696, 245, 12, 530, 654, 754, 551, 708, 337, 718, 384,
        717, 498, 149, 304, 261, 625, 91, 278, 16, 644, 165, 254, 588, 293, 722, 460, 716, 726, 568, 435, 737, 94, 237,
        111, 502, 566, 663, 346, 44, 589, 399, 358, 657, 604, 339, 687, 216, 514, 248, 103, 760, 424, 661, 114, 233,
        755, 757, 107,
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