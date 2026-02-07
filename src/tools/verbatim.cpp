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
    constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 751, 110, 674, 113, 125, 475, 460, 297, 634, 185, 314,
        599, 539, 622, 531, 760, 374, 734, 652, 767, 666, 350, 739, 323, 114, 252, 477, 683, 573, 75, 685, 188, 371,
        385, 635, 516, 282, 650, 61, 498, 544, 298, 6, 103, 676, 30, 102, 581, 647, 680, 78, 233, 588, 486, 131, 643,
        645, 294, 756, 703, 172, 419, 764, 32, 507, 704, 276, 70, 150, 262, 96, 115, 225, 547, 759, 317, 522, 492, 12,
        534, 107, 586, 42, 732, 167, 85, 242, 163, 571, 165, 111, 133, 740, 88, 58, 411, 174, 518, 765, 407, 515, 311,
        345, 244, 491, 710, 754, 101, 304, 307, 441, 47, 413, 410, 689, 143, 600, 145, 679, 551, 574, 670, 169, 56, 604,
        73, 118, 561, 168, 584, 69, 40, 355, 749, 132, 686, 34, 657, 523, 591, 681, 140, 93, 363, 709, 302, 162, 122,
        548, 147, 274, 15, 8, 741, 97, 9, 119, 529, 490, 677, 329, 458, 361, 342, 535, 718, 277, 82, 351, 55, 7, 719,
        68, 425, 171, 391, 176, 51, 630, 694, 130, 10, 182, 199, 320, 159, 31, 65, 234, 189, 750, 661, 607, 412, 158,
        18, 501, 717, 269, 669, 619, 697, 447, 153, 496, 46, 623, 194, 624, 631, 748, 190, 375, 572, 53, 509, 495, 222,
        258, 364, 540, 79, 616, 54, 315, 519, 177, 227, 672, 135, 542, 91, 422, 708, 175, 214, 403, 137, 211, 549, 324,
        462, 186, 367, 612, 245, 644, 198, 212, 336, 62, 493, 326, 228, 281, 105, 590, 482, 530, 134, 656, 693, 521,
        442, 723, 541, 266, 417, 678, 536, 563, 144, 592, 402, 649, 221, 275, 695, 520, 463, 753, 207, 611, 248, 444,
        330, 117, 757, 453, 94, 5, 558, 273, 553, 595, 295, 240, 215, 19, 289, 483, 476, 343, 421, 21, 259, 204, 104,
        301, 687, 579, 38, 152, 291, 292, 392, 333, 100, 388, 254, 564, 293, 310, 170, 257, 255, 357, 89, 226, 272, 283,
        745, 327, 328, 87, 405, 731, 37, 605, 394, 625, 445, 382, 183, 344, 334, 173, 414, 247, 256, 316, 50, 339, 365,
        67, 354, 390, 121, 546, 203, 675, 290, 472, 451, 428, 299, 229, 197, 640, 380, 636, 601, 594, 146, 464, 337,
        583, 503, 13, 401, 90, 187, 432, 29, 129, 655, 279, 123, 243, 409, 358, 497, 36, 555, 499, 331, 285, 554, 713,
        399, 83, 722, 513, 219, 404, 271, 702, 347, 408, 296, 232, 742, 59, 154, 525, 582, 389, 66, 237, 148, 752, 352,
        300, 280, 246, 426, 373, 305, 264, 76, 278, 241, 512, 161, 136, 306, 431, 166, 438, 353, 313, 729, 617, 128,
        126, 99, 552, 236, 712, 700, 638, 362, 217, 436, 502, 455, 192, 378, 692, 456, 565, 250, 567, 746, 39, 372, 64,
        218, 437, 568, 696, 440, 322, 473, 466, 737, 1, 224, 614, 71, 494, 610, 20, 489, 537, 701, 621, 238, 95, 23,
        157, 459, 249, 25, 366, 220, 400, 349, 514, 471, 633, 261, 562, 213, 383, 84, 479, 659, 265, 206, 260, 368, 216,
        11, 481, 725, 235, 758, 478, 60, 506, 532, 609, 109, 484, 744, 420, 526, 517, 359, 575, 284, 395, 533, 387, 637,
        3, 470, 720, 270, 480, 427, 27, 16, 116, 191, 286, 151, 155, 325, 195, 469, 418, 446, 660, 485, 593, 160, 398,
        707, 570, 560, 528, 356, 124, 653, 33, 80, 308, 429, 338, 682, 312, 639, 239, 156, 86, 41, 180, 715, 690, 538,
        381, 580, 433, 127, 557, 468, 108, 178, 596, 726, 63, 467, 201, 578, 106, 658, 196, 738, 81, 28, 319, 439, 585,
        43, 72, 454, 664, 35, 208, 335, 98, 263, 384, 527, 698, 545, 615, 603, 360, 193, 309, 566, 181, 457, 721, 627,
        397, 465, 504, 205, 4, 606, 346, 393, 149, 761, 210, 559, 92, 200, 303, 488, 505, 268, 711, 550, 608, 642, 613,
        587, 648, 508, 24, 209, 288, 287, 632, 620, 646, 556, 602, 443, 14, 667, 179, 142, 668, 569, 763, 423, 416, 673,
        654, 379, 341, 641, 369, 57, 2, 45, 474, 370, 424, 684, 671, 735, 120, 230, 184, 766, 332, 691, 49, 714, 44,
        139, 500, 651, 376, 730, 450, 348, 202, 705, 22, 728, 628, 340, 597, 251, 435, 662, 618, 452, 77, 386, 138, 449,
        112, 665, 448, 415, 724, 74, 461, 716, 733, 164, 688, 706, 524, 0, 543, 598, 589, 430, 231, 626, 223, 434, 629,
        755, 743, 510, 576, 747, 141, 318, 699, 511, 377, 396, 267, 17, 321, 52, 736, 663, 727, 406, 762, 487, 48, 253,
        26, 577,
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