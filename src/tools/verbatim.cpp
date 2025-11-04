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
    constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 251, 95, 533, 282, 750, 677, 627, 718, 211, 311, 739,
        689, 250, 26, 337, 174, 222, 193, 327, 276, 303, 636, 661, 576, 256, 708, 183, 27, 595, 726, 640, 594, 127, 417,
        545, 394, 146, 378, 374, 645, 489, 596, 741, 592, 719, 273, 60, 45, 467, 442, 728, 513, 372, 312, 702, 526, 598,
        758, 621, 655, 709, 396, 761, 525, 59, 671, 604, 319, 52, 203, 657, 536, 43, 121, 617, 577, 138, 742, 6, 613,
        654, 405, 294, 752, 156, 486, 35, 402, 257, 745, 674, 304, 139, 445, 191, 348, 7, 238, 579, 651, 563, 490, 371,
        86, 22, 430, 638, 682, 431, 738, 109, 527, 69, 624, 195, 98, 83, 91, 289, 494, 359, 685, 625, 549, 305, 19, 38,
        580, 324, 542, 44, 82, 330, 667, 512, 133, 192, 361, 731, 700, 90, 511, 652, 460, 321, 679, 681, 432, 764, 435,
        582, 760, 283, 33, 500, 740, 659, 426, 93, 462, 684, 18, 120, 226, 461, 465, 485, 622, 686, 561, 547, 732, 328,
        710, 265, 439, 280, 185, 115, 581, 160, 373, 14, 89, 609, 80, 570, 79, 343, 189, 158, 102, 314, 65, 23, 495,
        356, 73, 347, 762, 171, 375, 611, 320, 77, 148, 268, 264, 159, 441, 31, 520, 131, 362, 340, 110, 753, 112, 516,
        353, 84, 599, 366, 123, 620, 125, 132, 67, 76, 341, 746, 182, 539, 715, 765, 349, 297, 735, 152, 224, 472, 392,
        194, 413, 186, 216, 50, 754, 231, 114, 154, 433, 99, 712, 501, 673, 204, 124, 573, 688, 647, 676, 243, 247, 295,
        683, 665, 64, 414, 241, 663, 744, 756, 644, 105, 306, 575, 11, 318, 333, 13, 390, 278, 543, 248, 335, 286, 287,
        322, 701, 168, 733, 504, 380, 487, 552, 423, 244, 597, 129, 357, 515, 302, 606, 269, 698, 518, 214, 245, 603,
        541, 81, 363, 106, 468, 88, 40, 642, 456, 217, 140, 660, 463, 117, 377, 164, 288, 704, 749, 176, 345, 648, 629,
        190, 562, 313, 162, 713, 135, 531, 706, 666, 574, 475, 246, 41, 143, 722, 181, 119, 62, 351, 274, 482, 553, 480,
        122, 354, 358, 694, 253, 233, 412, 558, 692, 145, 588, 662, 147, 331, 212, 92, 696, 370, 48, 292, 637, 298, 51,
        325, 680, 591, 232, 188, 379, 384, 488, 108, 634, 548, 208, 524, 149, 1, 568, 550, 4, 15, 368, 196, 54, 551,
        535, 397, 404, 161, 743, 400, 308, 422, 519, 418, 385, 528, 585, 406, 166, 763, 259, 32, 220, 346, 421, 332,
        409, 554, 293, 427, 118, 239, 290, 668, 693, 301, 307, 537, 416, 142, 209, 272, 136, 699, 315, 443, 107, 716,
        424, 447, 415, 72, 355, 223, 440, 444, 420, 291, 458, 184, 546, 757, 408, 590, 134, 690, 464, 36, 711, 230, 725,
        471, 505, 755, 206, 470, 258, 398, 187, 56, 254, 479, 352, 656, 96, 55, 483, 450, 669, 179, 484, 180, 529, 491,
        493, 492, 178, 237, 97, 466, 199, 678, 497, 285, 502, 137, 476, 386, 399, 496, 240, 155, 157, 111, 381, 509,
        514, 703, 584, 198, 61, 452, 720, 695, 200, 747, 116, 429, 16, 17, 559, 100, 338, 163, 271, 428, 650, 205, 53,
        12, 538, 235, 24, 571, 631, 58, 503, 227, 532, 296, 376, 279, 454, 565, 344, 255, 317, 202, 649, 150, 618, 336,
        28, 602, 70, 9, 498, 37, 566, 173, 252, 530, 310, 687, 25, 560, 481, 507, 569, 10, 177, 478, 459, 281, 653, 670,
        234, 523, 5, 309, 113, 589, 270, 643, 326, 477, 228, 103, 364, 316, 165, 534, 169, 21, 249, 567, 724, 101, 393,
        564, 522, 391, 615, 260, 434, 556, 628, 299, 262, 517, 153, 748, 87, 616, 608, 736, 388, 555, 544, 587, 473,
        436, 630, 453, 63, 75, 94, 767, 57, 410, 641, 78, 213, 167, 3, 737, 610, 510, 646, 557, 614, 261, 141, 446, 369,
        714, 350, 172, 30, 275, 508, 469, 20, 175, 675, 600, 403, 104, 207, 170, 395, 601, 329, 455, 236, 387, 197, 540,
        705, 437, 49, 284, 367, 457, 664, 639, 672, 578, 521, 215, 42, 401, 721, 323, 34, 389, 46, 586, 633, 583, 697,
        360, 29, 339, 342, 365, 229, 593, 623, 572, 210, 130, 411, 383, 218, 66, 277, 474, 0, 730, 605, 717, 68, 128,
        263, 691, 451, 619, 626, 723, 632, 729, 506, 219, 499, 382, 221, 2, 407, 39, 242, 126, 47, 438, 74, 635, 448,
        419, 449, 334, 612, 658, 8, 71, 267, 727, 734, 607, 85, 266, 225, 759, 707, 201, 300, 151, 144, 751, 766, 425,
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