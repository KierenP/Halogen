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
    constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 595, 8, 395, 559, 60, 412, 201, 187, 606, 202, 688,
        671, 319, 623, 586, 58, 489, 64, 337, 765, 657, 175, 742, 145, 693, 316, 37, 300, 250, 223, 191, 581, 674, 387,
        289, 120, 224, 150, 555, 82, 121, 359, 517, 347, 707, 339, 725, 601, 592, 741, 459, 494, 470, 523, 364, 247,
        263, 138, 599, 754, 288, 260, 335, 63, 66, 99, 213, 731, 65, 93, 441, 763, 698, 734, 273, 135, 372, 598, 405,
        687, 88, 216, 393, 677, 500, 404, 153, 279, 384, 710, 681, 549, 468, 447, 48, 309, 356, 471, 484, 446, 717, 659,
        411, 469, 716, 358, 513, 340, 757, 652, 755, 248, 609, 271, 74, 476, 94, 544, 100, 584, 502, 432, 370, 760, 461,
        658, 647, 738, 172, 429, 560, 44, 536, 626, 354, 210, 638, 642, 406, 317, 641, 127, 644, 478, 431, 740, 117,
        147, 236, 51, 115, 415, 463, 726, 34, 668, 333, 458, 535, 139, 85, 92, 199, 621, 285, 357, 355, 14, 131, 222,
        116, 91, 464, 1, 443, 585, 727, 169, 766, 256, 594, 80, 428, 310, 177, 198, 612, 234, 208, 209, 514, 264, 327,
        231, 167, 179, 322, 45, 28, 157, 701, 183, 84, 426, 695, 388, 557, 528, 556, 97, 616, 166, 539, 206, 269, 146,
        237, 324, 750, 284, 367, 75, 29, 277, 533, 696, 548, 272, 296, 229, 2, 699, 312, 39, 178, 705, 62, 618, 401,
        709, 732, 165, 95, 54, 325, 72, 453, 389, 149, 125, 87, 171, 108, 703, 214, 672, 690, 633, 221, 185, 180, 507,
        220, 124, 24, 348, 746, 553, 226, 437, 10, 545, 132, 71, 261, 275, 61, 608, 258, 232, 238, 440, 390, 713, 564,
        383, 764, 242, 197, 307, 282, 516, 554, 102, 142, 181, 43, 767, 276, 290, 529, 281, 30, 98, 466, 32, 270, 691,
        660, 255, 622, 241, 651, 527, 240, 20, 495, 407, 155, 722, 613, 368, 718, 510, 182, 424, 735, 579, 409, 607,
        689, 46, 18, 118, 158, 203, 747, 603, 25, 762, 113, 295, 342, 736, 715, 77, 542, 294, 278, 329, 629, 590, 13,
        252, 233, 305, 543, 9, 12, 76, 360, 81, 162, 349, 193, 217, 366, 47, 205, 369, 49, 649, 558, 56, 351, 73, 186,
        104, 378, 129, 680, 386, 16, 287, 632, 101, 133, 338, 724, 257, 170, 330, 176, 530, 391, 21, 473, 141, 228, 211,
        286, 639, 380, 673, 83, 640, 207, 345, 114, 103, 410, 656, 589, 17, 577, 268, 344, 413, 283, 743, 123, 299, 452,
        377, 280, 4, 326, 90, 493, 475, 430, 244, 328, 230, 434, 67, 540, 436, 416, 439, 215, 109, 304, 444, 105, 212,
        78, 190, 154, 587, 661, 619, 292, 408, 379, 455, 756, 697, 353, 442, 460, 708, 692, 33, 752, 465, 433, 251, 341,
        137, 50, 227, 467, 483, 262, 654, 303, 265, 739, 417, 522, 320, 291, 481, 427, 218, 331, 487, 200, 352, 631,
        225, 89, 515, 521, 122, 423, 645, 243, 519, 128, 488, 670, 561, 457, 375, 706, 403, 40, 363, 498, 482, 196, 474,
        38, 418, 148, 27, 445, 7, 11, 486, 512, 491, 381, 168, 525, 402, 422, 112, 219, 143, 69, 192, 730, 259, 531,
        546, 22, 627, 336, 534, 164, 332, 297, 505, 253, 41, 518, 334, 504, 19, 119, 490, 419, 570, 420, 362, 398, 298,
        323, 456, 551, 506, 156, 160, 509, 385, 597, 86, 96, 550, 70, 194, 318, 163, 552, 274, 538, 394, 704, 565, 79,
        321, 568, 582, 343, 184, 134, 588, 573, 382, 729, 449, 373, 679, 723, 5, 485, 761, 311, 462, 425, 511, 508, 737,
        152, 235, 591, 569, 574, 600, 602, 53, 55, 615, 3, 624, 617, 541, 435, 572, 667, 566, 759, 714, 604, 371, 733,
        0, 630, 450, 130, 501, 712, 480, 532, 239, 614, 195, 575, 392, 188, 643, 249, 26, 479, 57, 562, 397, 477, 451,
        126, 313, 567, 728, 571, 496, 107, 448, 266, 399, 315, 628, 751, 650, 605, 636, 140, 620, 161, 547, 520, 151,
        653, 346, 684, 365, 662, 374, 611, 173, 350, 59, 648, 635, 36, 526, 23, 454, 637, 314, 306, 686, 694, 6, 499,
        665, 106, 524, 655, 610, 68, 189, 702, 625, 676, 666, 720, 682, 267, 578, 683, 634, 669, 308, 136, 361, 711,
        719, 110, 421, 492, 159, 438, 664, 144, 301, 576, 396, 580, 111, 663, 593, 31, 376, 400, 302, 472, 414, 685,
        675, 753, 246, 646, 748, 254, 749, 744, 293, 537, 42, 503, 52, 700, 174, 35, 245, 745, 563, 204, 596, 15, 497,
        721, 678, 583, 758,
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