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
    constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 727, 639, 257, 435, 69, 118, 173, 127, 352, 437, 617,
        501, 607, 343, 738, 284, 274, 723, 78, 156, 589, 268, 334, 449, 648, 298, 545, 171, 155, 180, 580, 163, 661,
        480, 494, 627, 279, 5, 699, 335, 328, 1, 267, 650, 211, 447, 76, 757, 534, 172, 766, 478, 603, 207, 181, 711,
        24, 259, 29, 301, 57, 605, 726, 586, 249, 441, 254, 686, 707, 531, 260, 43, 194, 628, 70, 2, 406, 191, 17, 653,
        124, 22, 234, 307, 455, 85, 86, 659, 485, 745, 663, 632, 349, 549, 414, 94, 593, 93, 521, 360, 678, 202, 507,
        276, 694, 416, 47, 0, 237, 108, 8, 733, 13, 27, 137, 390, 518, 375, 141, 179, 767, 197, 213, 223, 6, 670, 729,
        459, 303, 63, 306, 752, 287, 598, 182, 135, 508, 665, 631, 396, 209, 378, 647, 150, 562, 165, 218, 728, 643,
        151, 432, 380, 221, 46, 715, 110, 681, 395, 159, 52, 96, 133, 713, 635, 525, 190, 512, 232, 256, 672, 407, 41,
        250, 667, 749, 736, 402, 597, 717, 601, 557, 241, 169, 675, 112, 555, 535, 186, 215, 224, 741, 128, 389, 161,
        590, 113, 278, 330, 440, 443, 462, 103, 196, 37, 431, 204, 104, 592, 321, 262, 622, 743, 175, 442, 170, 144,
        210, 51, 428, 411, 164, 479, 520, 177, 16, 77, 140, 273, 222, 379, 377, 11, 492, 625, 19, 235, 28, 162, 115,
        472, 264, 201, 751, 266, 513, 193, 645, 554, 393, 688, 434, 56, 121, 174, 228, 587, 542, 138, 231, 649, 329,
        515, 184, 132, 320, 448, 244, 505, 178, 114, 658, 546, 744, 265, 153, 33, 382, 277, 39, 239, 157, 293, 40, 618,
        119, 684, 286, 219, 299, 220, 575, 48, 91, 570, 122, 290, 371, 305, 20, 342, 300, 372, 295, 292, 355, 676, 200,
        243, 614, 195, 208, 285, 660, 404, 50, 168, 246, 316, 149, 66, 59, 310, 761, 160, 696, 624, 602, 755, 99, 641,
        394, 288, 427, 579, 247, 32, 145, 566, 533, 476, 682, 596, 496, 720, 270, 326, 236, 333, 309, 255, 120, 397,
        101, 272, 621, 547, 252, 269, 130, 552, 322, 709, 362, 697, 318, 251, 248, 693, 504, 294, 370, 367, 376, 705,
        314, 563, 12, 89, 55, 313, 748, 44, 381, 577, 564, 199, 685, 214, 185, 340, 550, 359, 350, 556, 167, 198, 553,
        571, 704, 656, 732, 493, 528, 539, 701, 401, 638, 203, 474, 409, 509, 483, 100, 423, 753, 608, 258, 537, 373,
        433, 446, 490, 666, 280, 403, 176, 296, 725, 7, 358, 430, 139, 425, 4, 38, 34, 763, 634, 529, 400, 14, 187, 467,
        74, 336, 541, 87, 391, 111, 719, 363, 412, 325, 454, 560, 383, 188, 21, 419, 451, 444, 461, 737, 216, 73, 420,
        626, 80, 107, 642, 271, 489, 758, 58, 424, 463, 687, 9, 106, 387, 674, 10, 84, 718, 240, 45, 486, 487, 606, 439,
        583, 502, 475, 426, 484, 495, 283, 282, 242, 585, 500, 95, 146, 36, 81, 368, 386, 464, 481, 327, 98, 53, 68,
        361, 503, 71, 654, 54, 514, 126, 436, 527, 522, 339, 418, 762, 510, 722, 523, 558, 543, 456, 227, 712, 698, 253,
        524, 445, 532, 559, 417, 102, 506, 595, 365, 469, 438, 26, 604, 346, 42, 354, 369, 612, 217, 206, 548, 695, 374,
        82, 116, 297, 147, 357, 408, 516, 457, 567, 109, 281, 212, 450, 619, 573, 700, 337, 421, 25, 125, 460, 351, 3,
        582, 609, 158, 289, 526, 415, 134, 105, 429, 356, 538, 584, 540, 18, 338, 79, 611, 345, 323, 398, 97, 600, 651,
        324, 599, 30, 565, 143, 332, 568, 117, 302, 49, 75, 348, 569, 148, 716, 405, 61, 482, 620, 166, 680, 304, 23,
        399, 123, 72, 453, 466, 317, 633, 517, 473, 637, 341, 623, 129, 636, 742, 226, 702, 683, 233, 498, 388, 229,
        610, 511, 616, 759, 551, 646, 136, 83, 530, 344, 477, 311, 312, 664, 308, 62, 35, 319, 205, 629, 189, 315, 588,
        263, 465, 615, 756, 452, 677, 679, 384, 764, 640, 668, 230, 92, 657, 364, 644, 468, 671, 662, 245, 581, 65, 630,
        60, 392, 471, 225, 652, 154, 655, 703, 366, 152, 706, 613, 578, 754, 710, 669, 497, 689, 714, 192, 691, 275, 67,
        88, 740, 422, 734, 15, 760, 491, 692, 261, 142, 331, 90, 730, 347, 488, 690, 724, 765, 544, 561, 291, 458, 238,
        385, 746, 594, 572, 721, 591, 64, 183, 470, 747, 410, 31, 574, 673, 731, 131, 499, 576, 735, 353, 519, 708, 413,
        536, 739, 750,
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