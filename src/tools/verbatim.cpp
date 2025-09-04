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
    constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 77, 280, 649, 304, 576, 626, 220, 647, 345, 492, 59,
        722, 41, 569, 335, 557, 517, 446, 575, 364, 320, 337, 151, 349, 736, 152, 537, 397, 305, 756, 767, 731, 571,
        143, 393, 180, 681, 134, 13, 596, 661, 137, 426, 554, 111, 24, 450, 252, 553, 730, 384, 700, 244, 391, 470, 160,
        610, 120, 653, 675, 711, 26, 61, 221, 362, 583, 501, 194, 666, 674, 760, 407, 755, 526, 47, 92, 719, 352, 550,
        83, 270, 348, 38, 279, 116, 132, 738, 85, 90, 18, 705, 613, 8, 74, 750, 632, 127, 336, 63, 714, 538, 222, 720,
        703, 421, 545, 582, 389, 360, 743, 51, 206, 490, 497, 408, 45, 486, 31, 196, 558, 677, 369, 605, 658, 415, 299,
        577, 723, 273, 186, 112, 680, 732, 130, 58, 2, 81, 505, 676, 139, 701, 410, 317, 625, 88, 145, 390, 49, 203,
        669, 482, 149, 491, 496, 688, 33, 436, 593, 80, 423, 534, 376, 275, 230, 431, 466, 620, 193, 65, 380, 741, 165,
        367, 148, 636, 611, 1, 269, 668, 696, 296, 715, 591, 447, 375, 55, 418, 107, 464, 272, 101, 215, 735, 185, 35,
        574, 97, 379, 644, 316, 549, 401, 740, 465, 753, 44, 25, 598, 386, 452, 529, 651, 183, 510, 71, 708, 441, 327,
        761, 171, 455, 374, 717, 227, 292, 212, 46, 307, 562, 282, 565, 159, 16, 233, 568, 164, 428, 587, 64, 91, 213,
        238, 712, 511, 308, 245, 697, 297, 248, 609, 303, 188, 52, 329, 754, 536, 454, 181, 342, 444, 287, 114, 259,
        250, 135, 75, 57, 169, 240, 208, 136, 762, 535, 231, 618, 662, 354, 276, 504, 217, 289, 629, 144, 281, 531, 262,
        177, 513, 748, 184, 290, 333, 153, 228, 413, 419, 588, 291, 249, 473, 733, 82, 377, 765, 406, 499, 156, 32, 702,
        387, 122, 656, 242, 763, 612, 721, 318, 302, 559, 95, 500, 704, 286, 15, 543, 323, 190, 572, 746, 17, 237, 277,
        403, 502, 126, 163, 267, 162, 28, 338, 663, 580, 311, 463, 642, 604, 573, 695, 30, 532, 522, 518, 119, 110, 253,
        312, 79, 357, 319, 359, 99, 361, 123, 363, 487, 56, 133, 366, 300, 368, 606, 138, 94, 20, 544, 54, 7, 219, 402,
        718, 508, 87, 382, 530, 10, 326, 540, 373, 247, 343, 747, 224, 232, 541, 726, 67, 749, 0, 489, 246, 66, 214,
        690, 209, 561, 439, 655, 102, 468, 437, 739, 293, 602, 103, 198, 255, 640, 392, 698, 29, 552, 435, 271, 664, 11,
        590, 3, 742, 131, 383, 597, 429, 432, 652, 356, 420, 268, 442, 456, 417, 398, 353, 592, 440, 50, 752, 331, 172,
        657, 409, 48, 265, 448, 321, 37, 427, 93, 405, 474, 445, 744, 76, 168, 260, 340, 328, 399, 263, 727, 201, 344,
        471, 411, 724, 472, 404, 78, 493, 709, 451, 469, 434, 73, 314, 257, 251, 503, 105, 285, 430, 294, 483, 693, 524,
        150, 182, 725, 223, 370, 347, 615, 322, 581, 443, 108, 691, 313, 89, 226, 509, 121, 256, 355, 737, 607, 117, 23,
        438, 324, 599, 53, 129, 507, 69, 623, 109, 494, 751, 745, 306, 239, 62, 179, 479, 480, 189, 258, 166, 523, 113,
        278, 161, 195, 616, 178, 68, 197, 216, 284, 170, 729, 350, 19, 210, 241, 187, 142, 617, 40, 635, 266, 154, 207,
        385, 234, 229, 414, 547, 672, 614, 682, 551, 646, 516, 453, 525, 365, 346, 21, 274, 42, 22, 174, 372, 388, 167,
        309, 125, 124, 589, 631, 484, 381, 86, 564, 594, 378, 506, 630, 570, 600, 358, 146, 560, 84, 567, 686, 191, 478,
        462, 330, 514, 34, 601, 608, 495, 678, 548, 202, 60, 556, 595, 157, 622, 422, 192, 603, 533, 173, 283, 660, 218,
        699, 619, 628, 261, 98, 298, 4, 158, 667, 578, 555, 634, 176, 449, 728, 339, 400, 648, 528, 566, 395, 713, 118,
        539, 351, 155, 643, 659, 96, 758, 512, 104, 654, 665, 325, 412, 39, 542, 670, 72, 684, 225, 457, 416, 650, 476,
        70, 43, 175, 706, 147, 683, 764, 310, 621, 710, 637, 671, 459, 128, 235, 12, 519, 254, 461, 498, 638, 673, 424,
        334, 475, 141, 624, 694, 9, 645, 707, 520, 5, 36, 243, 332, 563, 6, 627, 685, 301, 586, 211, 579, 679, 433, 458,
        527, 639, 633, 236, 425, 295, 460, 264, 488, 27, 341, 585, 485, 521, 100, 716, 481, 734, 288, 692, 396, 759,
        315, 371, 199, 689, 467, 394, 115, 477, 200, 204, 641, 515, 140, 106, 546, 757, 687, 14, 584, 766, 205,
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