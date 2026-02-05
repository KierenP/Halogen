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
    constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 92, 488, 363, 661, 745, 730, 372, 228, 218, 588, 147,
        27, 623, 758, 762, 742, 563, 350, 731, 639, 382, 703, 585, 710, 132, 319, 721, 552, 631, 137, 530, 456, 694, 48,
        300, 705, 468, 625, 224, 187, 545, 413, 734, 384, 85, 643, 324, 47, 13, 617, 587, 743, 76, 74, 400, 95, 30, 485,
        531, 553, 154, 761, 497, 645, 670, 739, 215, 354, 765, 71, 561, 251, 675, 73, 232, 532, 332, 722, 365, 767, 183,
        540, 186, 122, 615, 726, 223, 409, 427, 423, 470, 737, 558, 401, 591, 529, 534, 97, 759, 150, 478, 110, 106,
        256, 465, 117, 748, 157, 33, 678, 669, 579, 632, 459, 436, 134, 746, 123, 34, 629, 80, 752, 44, 294, 608, 98,
        701, 25, 709, 244, 45, 618, 222, 499, 477, 453, 54, 75, 128, 680, 486, 179, 135, 299, 125, 612, 516, 626, 310,
        686, 8, 171, 571, 559, 668, 28, 298, 448, 115, 437, 689, 476, 461, 3, 573, 681, 102, 610, 261, 39, 140, 230,
        570, 68, 23, 265, 199, 163, 185, 660, 636, 665, 508, 415, 4, 551, 597, 599, 315, 69, 178, 227, 331, 149, 271,
        414, 126, 143, 105, 312, 505, 593, 200, 388, 188, 513, 204, 397, 81, 377, 432, 635, 569, 164, 210, 717, 219,
        504, 492, 38, 211, 212, 443, 611, 291, 684, 273, 466, 644, 217, 439, 736, 21, 392, 330, 366, 114, 239, 303, 696,
        520, 648, 434, 441, 301, 487, 237, 514, 501, 318, 557, 594, 605, 452, 518, 42, 715, 590, 43, 390, 192, 546, 260,
        649, 360, 650, 155, 353, 526, 37, 55, 577, 142, 738, 697, 165, 646, 70, 572, 145, 341, 247, 356, 15, 136, 284,
        406, 253, 181, 386, 166, 285, 713, 424, 213, 389, 280, 50, 433, 732, 716, 351, 333, 5, 226, 234, 642, 533, 153,
        214, 714, 60, 416, 402, 79, 749, 151, 329, 288, 35, 82, 63, 277, 652, 764, 320, 656, 317, 258, 458, 91, 538,
        484, 36, 740, 279, 446, 245, 584, 756, 0, 576, 723, 343, 344, 207, 339, 340, 254, 712, 471, 711, 566, 2, 348,
        56, 429, 287, 699, 359, 246, 101, 129, 235, 182, 274, 720, 94, 370, 598, 195, 410, 7, 374, 83, 225, 491, 744,
        221, 64, 510, 248, 167, 578, 544, 517, 12, 249, 381, 170, 700, 86, 766, 657, 647, 314, 522, 90, 61, 378, 404,
        208, 536, 104, 158, 405, 589, 418, 141, 375, 653, 334, 412, 368, 209, 216, 373, 269, 194, 355, 633, 283, 290,
        619, 496, 84, 463, 718, 296, 255, 595, 442, 205, 190, 475, 419, 503, 267, 295, 725, 138, 52, 120, 467, 58, 16,
        96, 498, 751, 160, 276, 72, 438, 308, 421, 481, 367, 278, 592, 457, 671, 266, 417, 242, 196, 586, 444, 425, 422,
        264, 449, 281, 109, 447, 407, 323, 152, 454, 107, 525, 702, 393, 398, 483, 469, 113, 304, 385, 127, 479, 240,
        201, 161, 26, 543, 428, 691, 462, 614, 243, 241, 272, 352, 309, 307, 328, 130, 159, 292, 673, 259, 511, 229,
        345, 473, 704, 59, 193, 202, 321, 396, 162, 613, 490, 495, 637, 640, 509, 286, 289, 168, 528, 53, 399, 313, 369,
        685, 177, 338, 250, 172, 1, 750, 371, 65, 537, 325, 547, 112, 203, 51, 464, 512, 175, 609, 403, 535, 379, 231,
        89, 57, 562, 100, 198, 40, 184, 335, 683, 472, 763, 527, 676, 6, 541, 408, 692, 220, 698, 753, 574, 146, 507,
        581, 583, 394, 582, 293, 66, 474, 347, 119, 17, 523, 620, 741, 103, 706, 257, 18, 349, 521, 391, 431, 564, 601,
        435, 395, 606, 659, 554, 32, 724, 695, 667, 173, 31, 67, 682, 364, 116, 616, 133, 262, 322, 624, 180, 555, 460,
        489, 252, 236, 602, 690, 337, 268, 628, 548, 708, 539, 634, 49, 99, 9, 306, 641, 124, 270, 542, 679, 451, 282,
        24, 580, 20, 494, 630, 549, 238, 62, 621, 651, 440, 197, 575, 176, 346, 361, 455, 607, 169, 118, 662, 88, 757,
        144, 305, 14, 664, 297, 719, 311, 519, 336, 596, 482, 600, 46, 515, 760, 550, 666, 674, 77, 111, 148, 316, 383,
        108, 78, 655, 87, 604, 93, 502, 560, 357, 506, 233, 567, 654, 693, 189, 156, 22, 430, 191, 420, 342, 754, 627,
        445, 688, 327, 139, 727, 658, 663, 380, 326, 565, 568, 206, 622, 362, 728, 19, 687, 733, 387, 500, 11, 121, 493,
        41, 131, 672, 275, 747, 729, 29, 302, 358, 10, 263, 524, 755, 556, 638, 376, 707, 480, 735, 677, 450, 426, 174,
        603, 411,
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