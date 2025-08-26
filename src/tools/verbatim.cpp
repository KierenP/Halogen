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
    constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 169, 741, 406, 488, 469, 418, 72, 404, 676, 757, 100,
        699, 549, 220, 244, 697, 657, 738, 313, 605, 79, 528, 515, 21, 688, 513, 326, 733, 679, 759, 623, 232, 629, 74,
        276, 224, 734, 498, 126, 210, 669, 259, 381, 291, 662, 432, 658, 503, 511, 451, 431, 712, 52, 325, 54, 716, 555,
        417, 107, 270, 560, 587, 347, 731, 507, 695, 630, 594, 743, 301, 670, 492, 388, 335, 34, 534, 201, 141, 721, 77,
        574, 642, 538, 747, 22, 83, 628, 55, 383, 612, 238, 18, 530, 99, 61, 130, 370, 237, 332, 443, 646, 673, 151,
        150, 36, 395, 122, 725, 250, 109, 563, 229, 203, 617, 610, 361, 94, 639, 485, 24, 575, 11, 254, 620, 478, 328,
        520, 767, 308, 607, 465, 160, 655, 524, 440, 290, 473, 698, 437, 13, 728, 106, 258, 765, 508, 131, 719, 400,
        317, 666, 8, 362, 281, 653, 364, 348, 302, 683, 84, 134, 354, 702, 625, 427, 345, 525, 450, 626, 397, 25, 434,
        80, 735, 37, 182, 175, 296, 522, 391, 179, 509, 215, 63, 548, 264, 746, 133, 557, 376, 202, 704, 167, 632, 284,
        477, 419, 102, 758, 65, 225, 196, 110, 353, 186, 685, 172, 624, 1, 286, 184, 73, 48, 17, 305, 456, 60, 136, 189,
        40, 724, 568, 614, 527, 198, 385, 59, 750, 20, 249, 732, 116, 231, 192, 216, 140, 283, 155, 496, 16, 430, 650,
        71, 412, 243, 602, 223, 766, 461, 307, 342, 269, 411, 597, 566, 455, 494, 164, 50, 722, 649, 89, 315, 320, 248,
        322, 645, 173, 176, 233, 295, 598, 234, 221, 194, 299, 346, 35, 472, 274, 193, 199, 279, 425, 242, 257, 753,
        521, 287, 319, 755, 751, 289, 256, 622, 27, 82, 588, 516, 480, 423, 690, 298, 4, 458, 6, 51, 410, 644, 754, 171,
        742, 531, 330, 88, 591, 246, 407, 665, 762, 46, 218, 97, 323, 730, 275, 384, 501, 161, 672, 33, 682, 124, 148,
        374, 271, 42, 316, 713, 415, 537, 446, 19, 449, 12, 693, 10, 240, 344, 360, 378, 266, 339, 349, 129, 718, 462,
        47, 331, 760, 181, 265, 358, 327, 366, 355, 29, 490, 367, 656, 58, 62, 601, 251, 585, 23, 211, 86, 272, 671,
        205, 252, 166, 677, 123, 748, 720, 168, 569, 245, 200, 111, 543, 369, 117, 483, 39, 278, 393, 28, 539, 309, 401,
        402, 476, 505, 375, 551, 219, 156, 453, 373, 114, 380, 159, 627, 604, 546, 471, 756, 282, 178, 420, 31, 416, 66,
        581, 740, 119, 445, 154, 214, 382, 92, 190, 580, 499, 518, 207, 260, 209, 519, 180, 121, 53, 95, 0, 422, 447,
        603, 162, 678, 273, 392, 371, 532, 188, 81, 208, 439, 414, 30, 49, 428, 324, 321, 467, 648, 314, 87, 647, 517,
        146, 700, 433, 262, 357, 744, 452, 468, 390, 436, 442, 399, 481, 660, 253, 351, 608, 573, 90, 460, 613, 91, 227,
        247, 343, 592, 152, 44, 421, 454, 255, 615, 297, 482, 158, 464, 459, 356, 582, 341, 340, 368, 502, 514, 197,
        661, 69, 641, 191, 127, 523, 664, 263, 163, 536, 5, 696, 143, 235, 389, 584, 583, 466, 413, 435, 590, 372, 533,
        526, 486, 261, 405, 493, 408, 500, 318, 727, 745, 562, 684, 68, 288, 101, 78, 292, 553, 157, 558, 559, 174, 311,
        147, 561, 280, 761, 554, 764, 337, 14, 491, 708, 595, 438, 38, 352, 56, 749, 104, 96, 504, 552, 386, 108, 93,
        487, 338, 611, 9, 394, 540, 474, 572, 70, 183, 334, 139, 153, 497, 212, 510, 222, 115, 303, 565, 589, 170, 241,
        714, 379, 545, 550, 694, 293, 429, 596, 424, 409, 125, 535, 638, 145, 706, 723, 541, 654, 310, 165, 763, 98,
        132, 333, 707, 635, 457, 365, 306, 637, 350, 138, 651, 387, 739, 118, 484, 441, 663, 285, 213, 618, 448, 616,
        600, 479, 636, 633, 396, 403, 578, 570, 268, 475, 236, 506, 144, 76, 304, 667, 277, 2, 659, 128, 529, 674, 230,
        675, 57, 15, 105, 135, 187, 681, 631, 495, 609, 463, 686, 687, 489, 567, 67, 377, 571, 444, 204, 692, 329, 45,
        359, 619, 547, 112, 85, 593, 426, 75, 544, 26, 177, 726, 710, 711, 705, 41, 7, 715, 185, 737, 701, 300, 717,
        512, 470, 113, 640, 703, 691, 267, 226, 564, 680, 621, 3, 542, 709, 668, 689, 729, 752, 556, 43, 398, 32, 142,
        634, 195, 64, 736, 137, 652, 576, 579, 217, 312, 336, 294, 577, 239, 103, 606, 599, 206, 643, 120, 149, 363,
        228, 586,
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