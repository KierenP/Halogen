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
    constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 428, 714, 601, 638, 664, 222, 713, 421, 68, 555, 625,
        704, 706, 381, 44, 678, 199, 750, 334, 759, 157, 581, 548, 400, 180, 422, 697, 424, 368, 409, 554, 248, 383,
        109, 22, 329, 169, 513, 503, 270, 449, 587, 60, 355, 584, 415, 146, 416, 758, 61, 93, 659, 186, 359, 49, 251,
        123, 721, 731, 724, 479, 121, 72, 747, 135, 66, 542, 130, 703, 698, 330, 268, 570, 210, 693, 36, 134, 423, 127,
        481, 459, 410, 715, 13, 723, 500, 429, 155, 51, 324, 232, 298, 161, 39, 594, 412, 684, 604, 765, 580, 578, 265,
        398, 598, 487, 4, 246, 252, 497, 766, 545, 705, 518, 405, 676, 569, 81, 681, 254, 622, 76, 566, 361, 213, 188,
        290, 689, 97, 767, 483, 690, 444, 280, 754, 454, 291, 185, 236, 635, 114, 276, 762, 585, 628, 168, 70, 55, 528,
        540, 567, 749, 763, 253, 617, 323, 553, 666, 445, 408, 149, 534, 476, 50, 145, 137, 547, 396, 675, 414, 595,
        599, 752, 437, 28, 139, 600, 551, 511, 636, 96, 502, 37, 156, 183, 426, 179, 5, 57, 273, 89, 737, 26, 465, 113,
        102, 700, 23, 669, 111, 466, 539, 436, 652, 160, 512, 402, 136, 25, 370, 643, 653, 154, 301, 176, 69, 141, 31,
        204, 322, 639, 406, 711, 307, 722, 308, 477, 493, 175, 743, 343, 753, 605, 339, 80, 642, 99, 46, 223, 205, 371,
        221, 509, 651, 299, 582, 313, 202, 492, 663, 238, 250, 374, 287, 105, 38, 209, 241, 271, 53, 100, 40, 90, 19,
        306, 679, 224, 458, 261, 430, 122, 125, 245, 563, 65, 562, 91, 120, 641, 219, 417, 460, 631, 473, 340, 447, 378,
        11, 108, 294, 603, 101, 321, 239, 362, 615, 10, 262, 289, 18, 655, 207, 29, 249, 144, 351, 198, 386, 275, 189,
        335, 602, 508, 33, 656, 314, 384, 316, 440, 256, 438, 212, 201, 8, 556, 665, 16, 543, 187, 391, 317, 331, 673,
        716, 269, 35, 558, 744, 550, 182, 288, 84, 433, 363, 45, 272, 77, 761, 151, 660, 34, 350, 395, 88, 71, 725, 568,
        411, 718, 474, 328, 341, 456, 217, 110, 292, 231, 529, 462, 177, 353, 285, 338, 274, 9, 379, 305, 694, 48, 608,
        171, 92, 535, 612, 342, 297, 116, 240, 304, 295, 616, 375, 337, 124, 627, 494, 195, 495, 740, 106, 613, 131,
        225, 541, 365, 646, 533, 592, 407, 237, 607, 486, 319, 346, 119, 501, 382, 147, 531, 333, 576, 300, 227, 650,
        132, 358, 142, 634, 727, 133, 387, 15, 82, 206, 228, 347, 435, 596, 425, 557, 173, 451, 432, 56, 736, 312, 203,
        78, 196, 692, 264, 448, 218, 719, 453, 709, 6, 47, 41, 190, 730, 325, 118, 611, 431, 63, 720, 64, 372, 178, 193,
        385, 571, 637, 7, 164, 191, 399, 52, 573, 158, 401, 478, 388, 397, 468, 633, 413, 380, 220, 278, 94, 233, 661,
        734, 485, 87, 364, 194, 348, 498, 58, 32, 662, 1, 560, 181, 446, 434, 117, 281, 751, 491, 708, 588, 418, 506,
        520, 519, 488, 389, 373, 369, 345, 523, 293, 649, 517, 126, 525, 310, 214, 332, 618, 559, 344, 629, 536, 739,
        170, 138, 282, 277, 717, 443, 392, 107, 701, 657, 687, 682, 3, 452, 115, 577, 336, 670, 166, 255, 623, 733, 685,
        267, 746, 546, 480, 565, 75, 590, 128, 258, 211, 597, 95, 86, 574, 296, 242, 357, 59, 489, 696, 442, 376, 549,
        561, 43, 349, 686, 235, 17, 257, 530, 496, 352, 510, 216, 165, 192, 200, 143, 79, 283, 475, 24, 315, 427, 140,
        526, 326, 609, 610, 499, 356, 671, 390, 544, 62, 226, 73, 583, 162, 620, 12, 302, 103, 311, 243, 244, 455, 85,
        575, 538, 507, 172, 420, 572, 367, 104, 0, 439, 471, 677, 591, 320, 259, 303, 674, 564, 215, 152, 112, 472, 27,
        614, 260, 632, 702, 163, 648, 393, 404, 30, 153, 403, 658, 129, 279, 263, 521, 366, 668, 467, 450, 621, 482,
        640, 532, 464, 579, 505, 745, 461, 626, 14, 74, 630, 230, 318, 98, 645, 647, 726, 586, 688, 360, 738, 695, 377,
        174, 699, 184, 167, 552, 470, 266, 735, 710, 707, 741, 394, 504, 514, 463, 150, 83, 537, 234, 654, 672, 755,
        619, 624, 21, 515, 148, 680, 2, 589, 728, 729, 522, 490, 457, 159, 284, 484, 667, 524, 20, 606, 469, 441, 742,
        286, 712, 54, 309, 197, 732, 748, 42, 644, 67, 229, 691, 354, 756, 757, 516, 208, 593, 247, 683, 527, 760, 419,
        764, 327,
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