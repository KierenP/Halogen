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
    constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 389, 379, 146, 153, 508, 150, 54, 703, 243, 543, 454,
        527, 147, 417, 118, 676, 764, 253, 579, 749, 300, 445, 597, 745, 11, 407, 529, 72, 67, 151, 293, 722, 519, 767,
        653, 137, 22, 116, 689, 394, 733, 762, 386, 199, 181, 535, 680, 635, 79, 322, 260, 390, 83, 610, 467, 391, 681,
        598, 659, 335, 524, 741, 474, 206, 113, 42, 358, 399, 470, 564, 403, 10, 712, 575, 486, 464, 164, 9, 60, 204,
        615, 679, 221, 382, 515, 698, 400, 376, 52, 499, 599, 479, 437, 721, 35, 451, 697, 104, 292, 263, 477, 469, 632,
        38, 320, 364, 102, 2, 701, 475, 119, 738, 512, 143, 485, 374, 0, 7, 746, 472, 183, 739, 106, 638, 196, 30, 723,
        101, 50, 419, 550, 62, 336, 484, 291, 353, 360, 652, 606, 422, 289, 420, 408, 85, 244, 463, 217, 155, 246, 497,
        304, 498, 290, 753, 209, 455, 589, 591, 743, 629, 518, 157, 17, 708, 189, 707, 45, 227, 563, 571, 553, 637, 145,
        326, 348, 211, 547, 365, 14, 663, 177, 274, 27, 337, 585, 490, 577, 63, 448, 148, 395, 347, 332, 371, 755, 5,
        660, 163, 440, 142, 341, 192, 89, 355, 271, 482, 276, 720, 760, 75, 570, 275, 468, 296, 214, 728, 315, 727, 259,
        219, 491, 65, 18, 46, 636, 446, 208, 224, 705, 590, 207, 700, 247, 239, 742, 232, 186, 121, 450, 48, 648, 127,
        554, 683, 357, 37, 165, 33, 250, 93, 298, 587, 333, 695, 86, 185, 313, 675, 258, 252, 97, 302, 262, 109, 569,
        582, 507, 534, 557, 565, 345, 184, 226, 684, 261, 294, 36, 108, 112, 103, 622, 287, 58, 600, 91, 278, 725, 120,
        572, 759, 129, 562, 373, 665, 283, 179, 47, 641, 581, 299, 539, 125, 236, 426, 494, 558, 425, 435, 43, 643, 461,
        80, 349, 318, 68, 281, 612, 231, 623, 545, 397, 160, 644, 323, 378, 677, 694, 580, 29, 41, 757, 131, 132, 284,
        61, 511, 233, 107, 130, 115, 31, 53, 328, 245, 714, 751, 200, 367, 176, 69, 670, 409, 352, 516, 286, 267, 691,
        384, 344, 359, 16, 737, 254, 662, 175, 149, 717, 366, 266, 555, 672, 191, 541, 645, 449, 380, 633, 70, 257, 325,
        218, 387, 372, 81, 159, 523, 711, 504, 190, 430, 436, 556, 168, 327, 110, 503, 405, 198, 398, 628, 77, 28, 401,
        161, 576, 480, 201, 100, 613, 631, 488, 59, 433, 412, 255, 212, 235, 732, 413, 593, 229, 122, 607, 388, 424,
        346, 249, 500, 416, 604, 194, 747, 601, 288, 56, 307, 321, 166, 538, 193, 66, 140, 690, 319, 124, 329, 256, 133,
        173, 105, 421, 441, 431, 656, 458, 51, 678, 609, 12, 696, 215, 15, 573, 203, 57, 154, 310, 95, 342, 657, 734,
        339, 713, 605, 510, 92, 178, 144, 729, 270, 560, 4, 238, 752, 661, 114, 427, 496, 309, 521, 618, 625, 620, 495,
        172, 377, 94, 237, 453, 650, 452, 78, 334, 763, 241, 210, 223, 614, 654, 530, 754, 551, 216, 174, 578, 350, 117,
        414, 506, 141, 64, 128, 314, 509, 134, 32, 370, 96, 456, 715, 297, 205, 152, 533, 428, 3, 476, 111, 383, 520,
        744, 473, 123, 228, 673, 549, 49, 513, 282, 340, 501, 338, 381, 649, 548, 483, 21, 268, 536, 438, 84, 434, 39,
        316, 306, 55, 699, 489, 459, 269, 317, 546, 492, 87, 156, 273, 26, 443, 331, 505, 99, 439, 356, 8, 411, 248,
        514, 423, 88, 532, 444, 586, 531, 710, 740, 343, 596, 447, 362, 442, 704, 471, 567, 180, 761, 396, 220, 594,
        706, 272, 542, 574, 481, 385, 222, 23, 354, 135, 595, 525, 540, 182, 603, 552, 392, 393, 559, 517, 402, 592,
        330, 766, 19, 82, 478, 240, 305, 225, 295, 630, 90, 234, 602, 40, 74, 303, 167, 429, 195, 71, 324, 301, 682,
        537, 138, 756, 626, 583, 642, 188, 242, 735, 566, 502, 280, 658, 265, 667, 351, 617, 73, 158, 611, 230, 719, 44,
        651, 418, 616, 758, 197, 493, 646, 162, 668, 627, 20, 718, 526, 213, 687, 522, 765, 34, 693, 621, 126, 171, 640,
        1, 279, 170, 702, 169, 584, 432, 688, 363, 666, 686, 312, 369, 619, 25, 76, 709, 671, 308, 251, 139, 544, 457,
        24, 674, 361, 724, 466, 13, 634, 561, 136, 368, 264, 685, 460, 608, 568, 716, 730, 726, 187, 98, 692, 639, 465,
        624, 736, 487, 202, 664, 750, 528, 647, 6, 462, 406, 669, 748, 375, 588, 404, 415, 731, 277, 311, 410, 655, 285,
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