// Load the network file, do some preprocessing, and then save it to the build directory for inclusion in the final
// binary

#include "network/arch.hpp"
#include "network/simd/definitions.hpp"

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
    constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 329, 264, 216, 109, 344, 345, 693, 717, 695, 598, 426,
        274, 464, 395, 16, 62, 40, 111, 218, 164, 724, 235, 9, 183, 754, 722, 356, 606, 429, 533, 93, 283, 732, 688,
        627, 753, 257, 270, 154, 20, 405, 386, 156, 47, 589, 659, 617, 339, 391, 95, 51, 377, 31, 608, 406, 584, 579,
        163, 333, 560, 517, 112, 48, 623, 219, 343, 607, 505, 701, 250, 182, 574, 118, 648, 254, 716, 487, 503, 96, 384,
        86, 119, 734, 588, 147, 64, 570, 282, 481, 146, 421, 131, 626, 593, 316, 150, 555, 492, 103, 444, 747, 486, 682,
        765, 595, 188, 507, 591, 246, 90, 104, 37, 651, 653, 413, 680, 613, 663, 488, 91, 74, 684, 496, 578, 397, 540,
        276, 303, 745, 293, 68, 124, 469, 23, 656, 76, 667, 374, 484, 678, 624, 166, 340, 672, 748, 122, 2, 214, 614,
        73, 202, 394, 660, 573, 238, 204, 367, 637, 199, 132, 750, 38, 206, 735, 69, 364, 162, 478, 373, 674, 141, 451,
        509, 500, 685, 260, 459, 412, 200, 45, 175, 762, 161, 157, 158, 272, 597, 129, 450, 105, 71, 543, 475, 220, 4,
        29, 197, 385, 12, 180, 149, 227, 592, 203, 114, 26, 211, 14, 359, 711, 210, 600, 761, 172, 368, 208, 596, 513,
        273, 226, 280, 603, 545, 759, 243, 314, 255, 237, 448, 75, 155, 252, 28, 602, 510, 521, 514, 479, 721, 294, 700,
        428, 240, 604, 334, 372, 539, 176, 349, 437, 247, 127, 361, 719, 87, 41, 169, 493, 159, 328, 629, 253, 760, 559,
        201, 148, 36, 137, 436, 21, 107, 140, 569, 225, 702, 635, 585, 554, 755, 52, 564, 531, 301, 308, 728, 239, 1,
        287, 126, 477, 452, 691, 482, 577, 438, 305, 473, 529, 46, 34, 77, 24, 594, 22, 108, 461, 304, 712, 233, 3, 766,
        302, 522, 192, 620, 550, 381, 315, 763, 205, 101, 215, 267, 185, 313, 622, 151, 222, 449, 423, 195, 471, 292,
        98, 310, 639, 271, 15, 409, 443, 299, 50, 615, 323, 82, 249, 288, 338, 698, 567, 634, 311, 601, 631, 88, 355,
        425, 84, 751, 178, 142, 392, 632, 358, 572, 662, 387, 519, 295, 191, 63, 221, 262, 43, 556, 0, 523, 363, 152,
        506, 184, 8, 490, 325, 5, 244, 179, 281, 193, 285, 380, 628, 57, 390, 335, 66, 396, 268, 67, 399, 729, 520, 456,
        194, 401, 241, 703, 186, 633, 116, 54, 177, 346, 59, 742, 740, 351, 749, 463, 7, 128, 393, 619, 297, 515, 348,
        710, 699, 318, 690, 317, 170, 135, 11, 198, 81, 424, 587, 643, 65, 707, 551, 130, 143, 668, 300, 212, 370, 537,
        561, 145, 371, 568, 307, 697, 435, 441, 256, 453, 55, 720, 454, 462, 420, 495, 534, 171, 427, 376, 269, 113,
        341, 731, 72, 402, 472, 446, 410, 430, 27, 440, 83, 736, 407, 44, 744, 494, 232, 173, 611, 616, 60, 645, 388,
        489, 417, 664, 497, 535, 431, 501, 442, 42, 418, 375, 670, 266, 422, 229, 6, 261, 566, 512, 99, 738, 466, 56,
        230, 39, 757, 445, 416, 13, 213, 330, 483, 49, 527, 352, 248, 657, 546, 378, 258, 532, 468, 714, 134, 32, 326,
        174, 58, 730, 630, 590, 284, 19, 251, 100, 675, 526, 92, 277, 641, 125, 117, 686, 411, 121, 360, 548, 115, 563,
        434, 353, 25, 458, 565, 625, 324, 242, 382, 654, 70, 350, 139, 321, 80, 689, 414, 354, 549, 457, 583, 160, 580,
        190, 398, 245, 168, 491, 499, 562, 694, 347, 286, 480, 679, 236, 389, 357, 439, 640, 502, 752, 586, 465, 187,
        336, 525, 265, 498, 524, 618, 33, 298, 123, 612, 552, 557, 309, 10, 474, 366, 223, 97, 35, 558, 322, 544, 609,
        511, 289, 767, 536, 705, 403, 79, 709, 167, 467, 516, 575, 696, 362, 365, 538, 647, 94, 610, 231, 332, 419, 650,
        687, 102, 541, 581, 18, 518, 379, 133, 234, 432, 181, 337, 30, 455, 78, 665, 743, 671, 228, 61, 110, 153, 331,
        666, 741, 642, 263, 53, 676, 683, 644, 571, 291, 290, 460, 661, 756, 165, 692, 530, 17, 136, 85, 312, 408, 447,
        296, 638, 259, 504, 383, 542, 706, 704, 764, 508, 476, 279, 652, 275, 733, 582, 646, 713, 278, 320, 189, 553,
        636, 470, 224, 547, 658, 319, 681, 649, 677, 89, 196, 138, 725, 669, 400, 673, 758, 718, 306, 217, 726, 528,
        737, 655, 106, 144, 415, 327, 727, 708, 207, 723, 120, 576, 599, 746, 433, 485, 342, 715, 605, 739, 209, 404,
        621, 369,
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