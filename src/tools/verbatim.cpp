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
    constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 138, 537, 682, 756, 450, 760, 76, 748, 3, 342, 430,
        533, 8, 440, 673, 711, 81, 387, 218, 372, 480, 48, 145, 326, 153, 389, 146, 379, 97, 230, 202, 731, 610, 6, 90,
        83, 214, 339, 693, 436, 366, 93, 499, 131, 69, 95, 158, 176, 329, 124, 256, 715, 582, 534, 614, 507, 172, 117,
        720, 697, 360, 337, 64, 708, 238, 382, 757, 560, 657, 604, 556, 285, 626, 612, 623, 545, 736, 595, 135, 225,
        653, 35, 451, 406, 248, 514, 423, 411, 61, 271, 511, 482, 726, 568, 716, 435, 729, 270, 144, 26, 646, 766, 162,
        330, 80, 479, 98, 41, 678, 696, 637, 474, 36, 578, 231, 108, 104, 292, 263, 635, 107, 233, 115, 510, 624, 487,
        723, 72, 710, 215, 531, 355, 611, 441, 344, 691, 22, 116, 38, 130, 134, 515, 96, 32, 197, 298, 631, 17, 704, 33,
        567, 737, 483, 649, 381, 548, 375, 730, 535, 562, 517, 89, 142, 163, 205, 593, 456, 99, 702, 169, 634, 279, 616,
        193, 166, 168, 598, 681, 659, 434, 280, 622, 58, 553, 216, 613, 350, 174, 65, 171, 529, 1, 120, 43, 177, 27, 59,
        368, 500, 181, 467, 660, 629, 275, 240, 82, 223, 19, 585, 540, 561, 552, 428, 112, 209, 103, 705, 332, 207, 5,
        147, 42, 302, 719, 475, 701, 119, 738, 652, 284, 606, 132, 182, 224, 254, 208, 37, 173, 767, 357, 210, 206, 241,
        524, 244, 334, 463, 386, 136, 219, 212, 251, 28, 68, 422, 698, 227, 707, 274, 45, 418, 246, 497, 713, 154, 232,
        247, 57, 91, 261, 278, 725, 530, 654, 118, 551, 291, 484, 336, 628, 259, 287, 443, 105, 555, 191, 672, 266, 618,
        85, 420, 289, 377, 513, 348, 620, 753, 290, 229, 122, 328, 425, 31, 245, 165, 16, 409, 111, 546, 125, 236, 459,
        183, 741, 267, 746, 690, 164, 667, 318, 317, 170, 644, 323, 692, 734, 187, 299, 601, 683, 754, 56, 727, 343,
        234, 315, 615, 640, 180, 78, 633, 143, 485, 325, 576, 269, 304, 362, 281, 661, 220, 349, 580, 417, 378, 54, 44,
        589, 358, 359, 509, 331, 573, 363, 364, 102, 444, 320, 586, 714, 532, 88, 525, 717, 498, 149, 250, 599, 376, 52,
        621, 410, 34, 765, 675, 658, 178, 262, 340, 159, 282, 338, 665, 11, 283, 699, 198, 18, 398, 405, 185, 333, 319,
        432, 415, 404, 400, 324, 70, 728, 468, 655, 243, 286, 352, 516, 521, 199, 211, 186, 670, 71, 662, 301, 539, 276,
        426, 722, 374, 512, 455, 412, 10, 77, 488, 401, 305, 630, 636, 446, 226, 684, 571, 49, 272, 294, 706, 703, 627,
        668, 20, 312, 427, 505, 66, 140, 127, 139, 397, 671, 651, 538, 605, 92, 542, 407, 53, 549, 403, 2, 470, 114,
        448, 253, 148, 579, 632, 477, 73, 192, 106, 296, 255, 461, 0, 469, 617, 476, 228, 645, 258, 126, 429, 303, 460,
        608, 724, 361, 15, 466, 504, 523, 390, 121, 486, 558, 201, 464, 129, 501, 449, 369, 51, 84, 493, 408, 650, 23,
        453, 519, 445, 204, 179, 321, 465, 277, 123, 133, 574, 380, 473, 541, 414, 141, 383, 520, 249, 346, 735, 424,
        495, 351, 94, 237, 365, 663, 14, 547, 155, 217, 384, 718, 413, 503, 327, 101, 373, 264, 492, 67, 584, 526, 522,
        213, 161, 39, 316, 564, 588, 764, 293, 749, 391, 437, 457, 24, 490, 577, 314, 370, 203, 335, 184, 565, 433, 388,
        581, 194, 472, 7, 528, 439, 536, 268, 625, 438, 554, 496, 452, 300, 557, 559, 345, 392, 396, 594, 761, 607, 758,
        221, 109, 421, 572, 273, 87, 156, 550, 395, 353, 100, 481, 688, 222, 385, 175, 563, 367, 609, 592, 402, 639,
        669, 502, 242, 442, 566, 260, 587, 322, 745, 152, 190, 416, 687, 454, 527, 543, 744, 447, 648, 694, 596, 583,
        188, 235, 602, 478, 75, 310, 570, 491, 62, 50, 195, 313, 151, 297, 252, 128, 732, 309, 160, 189, 677, 79, 762,
        12, 751, 239, 200, 354, 341, 347, 680, 150, 508, 110, 356, 157, 295, 759, 656, 752, 4, 462, 647, 307, 308, 288,
        600, 695, 458, 86, 569, 679, 74, 494, 167, 25, 709, 471, 619, 394, 712, 689, 575, 113, 393, 399, 685, 674, 13,
        544, 721, 431, 603, 642, 506, 590, 518, 747, 700, 643, 371, 29, 40, 489, 306, 55, 419, 740, 591, 733, 743, 196,
        30, 763, 21, 676, 46, 686, 666, 257, 641, 311, 47, 597, 755, 137, 742, 750, 664, 9, 60, 63, 739, 638, 265,
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