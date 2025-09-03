// Load the network file, do some preprocessing, and then save it to the build directory for inclusion in the final
// binary

#include "network/arch.hpp"
#include "network/simd/define.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>

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
    constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 631, 169, 702, 756, 695, 750, 577, 661, 670, 513, 379,
        644, 526, 668, 16, 255, 727, 518, 610, 491, 116, 504, 419, 730, 709, 253, 617, 587, 30, 206, 248, 298, 437, 679,
        760, 85, 539, 646, 710, 164, 477, 669, 673, 740, 4, 355, 598, 75, 697, 680, 716, 99, 546, 543, 142, 52, 327, 74,
        766, 739, 107, 173, 424, 24, 383, 335, 251, 386, 89, 507, 296, 64, 692, 63, 286, 95, 514, 243, 289, 226, 703,
        557, 508, 233, 701, 752, 128, 639, 117, 696, 14, 642, 330, 97, 757, 341, 447, 137, 381, 325, 452, 353, 122, 414,
        533, 623, 329, 126, 328, 731, 480, 373, 277, 270, 663, 746, 135, 524, 26, 188, 510, 711, 553, 96, 688, 484, 356,
        167, 713, 366, 541, 578, 71, 591, 649, 240, 466, 621, 423, 139, 682, 295, 691, 348, 581, 130, 352, 620, 734,
        463, 728, 148, 211, 489, 421, 13, 156, 187, 643, 469, 683, 2, 307, 565, 652, 172, 58, 717, 304, 42, 342, 94, 33,
        194, 132, 763, 176, 445, 228, 38, 607, 694, 492, 560, 155, 127, 583, 454, 706, 294, 537, 256, 347, 505, 120,
        190, 293, 446, 198, 119, 548, 637, 45, 457, 512, 162, 604, 602, 48, 550, 310, 600, 726, 46, 145, 753, 182, 378,
        320, 473, 397, 434, 204, 376, 719, 573, 109, 418, 234, 59, 382, 346, 655, 68, 201, 326, 676, 263, 202, 152, 183,
        235, 165, 242, 613, 712, 203, 82, 522, 141, 12, 230, 478, 73, 134, 249, 761, 53, 49, 36, 375, 314, 765, 387,
        442, 66, 106, 628, 460, 403, 674, 78, 618, 638, 274, 114, 584, 459, 648, 443, 151, 389, 43, 656, 215, 285, 390,
        384, 357, 498, 56, 571, 585, 705, 743, 582, 749, 281, 297, 67, 245, 175, 394, 220, 303, 503, 269, 28, 542, 393,
        197, 50, 316, 400, 427, 163, 224, 193, 540, 273, 111, 744, 238, 339, 98, 306, 101, 323, 595, 556, 501, 115, 292,
        21, 170, 759, 372, 545, 517, 318, 69, 51, 566, 31, 653, 291, 118, 343, 349, 747, 551, 483, 767, 267, 555, 93,
        268, 205, 299, 180, 231, 70, 363, 358, 497, 312, 174, 168, 552, 9, 191, 699, 481, 741, 121, 125, 317, 367, 3,
        686, 32, 704, 624, 272, 55, 209, 17, 563, 678, 258, 474, 266, 559, 100, 413, 102, 181, 345, 641, 60, 529, 159,
        660, 402, 305, 86, 47, 364, 265, 396, 131, 283, 626, 549, 401, 61, 562, 62, 500, 516, 232, 29, 406, 745, 487,
        261, 415, 658, 428, 609, 154, 113, 229, 210, 671, 435, 425, 411, 511, 333, 35, 158, 570, 18, 280, 90, 57, 161,
        112, 616, 338, 596, 246, 241, 359, 153, 444, 416, 458, 351, 8, 461, 315, 150, 391, 309, 124, 467, 105, 521, 244,
        288, 331, 449, 308, 751, 143, 580, 677, 479, 493, 632, 462, 15, 149, 736, 486, 569, 420, 561, 178, 689, 465,
        322, 488, 369, 651, 368, 450, 499, 81, 576, 448, 257, 456, 216, 640, 506, 417, 723, 724, 407, 748, 110, 365,
        681, 108, 395, 496, 374, 667, 207, 350, 440, 271, 525, 520, 409, 451, 633, 22, 455, 690, 392, 287, 527, 700,
        519, 360, 597, 276, 217, 635, 264, 544, 398, 218, 426, 37, 177, 594, 472, 252, 20, 212, 334, 185, 7, 558, 88,
        530, 399, 313, 337, 412, 547, 625, 405, 762, 362, 495, 129, 464, 278, 44, 575, 19, 721, 614, 324, 236, 708, 758,
        485, 259, 509, 222, 718, 535, 27, 275, 354, 302, 192, 523, 321, 615, 707, 84, 103, 157, 590, 490, 742, 592, 41,
        301, 408, 729, 664, 5, 629, 136, 622, 567, 83, 606, 601, 40, 225, 77, 538, 76, 502, 319, 186, 377, 611, 388,
        475, 25, 344, 79, 534, 290, 72, 608, 160, 422, 65, 536, 138, 144, 199, 196, 0, 432, 468, 213, 195, 650, 439,
        104, 494, 630, 200, 260, 361, 657, 80, 184, 430, 482, 254, 732, 645, 654, 453, 438, 605, 336, 91, 179, 34, 227,
        568, 659, 662, 147, 574, 39, 476, 754, 647, 223, 685, 627, 687, 92, 714, 54, 87, 528, 284, 10, 146, 250, 698,
        311, 693, 441, 410, 636, 586, 340, 404, 564, 634, 1, 171, 282, 470, 133, 431, 370, 715, 737, 279, 612, 471, 23,
        593, 666, 247, 214, 725, 599, 380, 531, 189, 675, 6, 684, 433, 300, 735, 123, 764, 733, 722, 738, 720, 572, 603,
        140, 665, 219, 332, 221, 588, 554, 237, 672, 619, 11, 755, 262, 579, 166, 239, 532, 589, 436, 429, 385, 208,
        371, 515,
    };
    // clang-format on

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

struct network
{
    alignas(64) std::array<std::array<int16_t, FT_SIZE>, INPUT_SIZE * KING_BUCKET_COUNT> ft_weight = {};
    alignas(64) std::array<int16_t, FT_SIZE> ft_bias = {};
    alignas(64) std::array<std::array<int8_t, FT_SIZE * L1_SIZE>, OUTPUT_BUCKETS> l1_weight = {};
    alignas(64) std::array<std::array<int32_t, L1_SIZE>, OUTPUT_BUCKETS> l1_bias = {};
    alignas(64) std::array<std::array<std::array<float, L2_SIZE>, L1_SIZE * 2>, OUTPUT_BUCKETS> l2_weight = {};
    alignas(64) std::array<std::array<float, L2_SIZE>, OUTPUT_BUCKETS> l2_bias = {};
    alignas(64) std::array<std::array<float, L2_SIZE>, OUTPUT_BUCKETS> l3_weight = {};
    alignas(64) std::array<float, OUTPUT_BUCKETS> l3_bias = {};
};

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