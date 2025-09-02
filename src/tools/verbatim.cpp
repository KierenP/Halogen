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
    constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 151, 583, 568, 695, 252, 280, 753, 569, 272, 65, 755,
        579, 159, 126, 414, 331, 173, 668, 687, 140, 556, 275, 94, 447, 650, 610, 57, 149, 591, 505, 47, 176, 448, 370,
        601, 425, 8, 715, 452, 648, 237, 535, 219, 462, 733, 438, 664, 450, 750, 759, 92, 684, 90, 630, 533, 511, 549,
        748, 320, 13, 725, 742, 380, 652, 469, 374, 522, 651, 736, 504, 628, 624, 683, 653, 670, 675, 123, 431, 263,
        390, 60, 53, 686, 160, 479, 314, 132, 722, 202, 210, 145, 45, 150, 3, 696, 422, 86, 632, 96, 379, 542, 586, 518,
        101, 747, 644, 758, 55, 408, 472, 27, 366, 466, 463, 190, 637, 344, 274, 118, 119, 43, 662, 430, 667, 264, 356,
        543, 284, 321, 30, 443, 607, 298, 735, 133, 392, 704, 110, 85, 270, 386, 434, 156, 640, 358, 208, 152, 258, 171,
        740, 226, 442, 435, 66, 682, 467, 199, 551, 271, 597, 672, 89, 281, 724, 468, 169, 80, 67, 58, 197, 757, 639,
        723, 234, 437, 127, 99, 454, 9, 128, 157, 436, 482, 546, 594, 473, 245, 187, 387, 737, 330, 649, 36, 698, 225,
        158, 268, 178, 242, 21, 161, 186, 107, 122, 11, 278, 84, 205, 418, 267, 523, 342, 148, 714, 711, 206, 446, 307,
        52, 192, 420, 221, 341, 550, 483, 317, 364, 74, 635, 666, 46, 678, 404, 39, 489, 429, 654, 63, 181, 235, 224,
        24, 493, 349, 658, 478, 503, 195, 458, 103, 622, 661, 213, 14, 729, 19, 75, 745, 292, 136, 244, 312, 120, 180,
        708, 77, 112, 333, 41, 214, 260, 265, 360, 102, 23, 751, 638, 369, 655, 720, 207, 106, 155, 325, 544, 54, 728,
        485, 166, 540, 290, 599, 441, 417, 322, 146, 595, 415, 397, 48, 500, 193, 302, 168, 1, 71, 623, 289, 319, 18,
        93, 526, 303, 293, 713, 690, 59, 459, 749, 445, 636, 73, 474, 29, 613, 10, 498, 563, 316, 283, 115, 16, 428,
        329, 665, 308, 490, 286, 593, 738, 2, 236, 627, 217, 393, 76, 250, 318, 709, 12, 571, 198, 706, 633, 347, 338,
        228, 373, 354, 476, 588, 611, 362, 657, 22, 561, 512, 141, 752, 310, 519, 336, 534, 495, 406, 565, 497, 520,
        255, 541, 734, 618, 276, 382, 371, 756, 223, 204, 184, 109, 117, 391, 273, 182, 394, 240, 305, 296, 592, 388,
        580, 529, 606, 376, 396, 144, 246, 619, 142, 377, 326, 572, 0, 32, 492, 257, 416, 285, 674, 375, 233, 291, 42,
        423, 424, 398, 480, 596, 324, 575, 545, 559, 470, 453, 487, 129, 766, 754, 697, 581, 335, 253, 566, 440, 28,
        164, 174, 381, 37, 643, 383, 701, 515, 241, 461, 486, 645, 172, 507, 746, 389, 51, 359, 491, 457, 584, 346, 220,
        282, 108, 625, 538, 97, 539, 239, 5, 626, 427, 183, 555, 230, 130, 384, 727, 395, 135, 399, 339, 15, 332, 531,
        432, 33, 355, 304, 552, 557, 301, 574, 739, 227, 764, 88, 328, 200, 506, 372, 726, 548, 137, 25, 509, 499, 730,
        385, 488, 464, 231, 400, 760, 277, 179, 352, 70, 471, 201, 717, 525, 564, 367, 315, 350, 532, 311, 439, 306,
        576, 31, 702, 378, 368, 203, 361, 139, 279, 547, 147, 621, 475, 412, 248, 455, 256, 50, 465, 449, 553, 710, 410,
        69, 456, 35, 741, 323, 295, 560, 218, 631, 481, 536, 131, 516, 38, 170, 615, 167, 62, 251, 254, 215, 357, 699,
        138, 82, 78, 196, 222, 669, 81, 407, 243, 266, 567, 72, 585, 211, 409, 153, 262, 100, 297, 348, 40, 659, 494,
        605, 598, 660, 608, 676, 582, 44, 7, 600, 603, 421, 765, 570, 259, 496, 577, 49, 124, 185, 162, 620, 694, 269,
        558, 646, 721, 692, 616, 34, 249, 413, 191, 634, 761, 163, 177, 641, 484, 340, 513, 604, 188, 744, 642, 602,
        554, 212, 229, 712, 426, 294, 517, 444, 573, 647, 703, 705, 578, 91, 693, 629, 337, 232, 527, 154, 134, 4, 530,
        673, 510, 61, 104, 656, 334, 365, 671, 111, 433, 681, 685, 700, 79, 521, 143, 587, 403, 83, 679, 125, 402, 313,
        17, 609, 477, 247, 612, 189, 113, 194, 589, 419, 20, 731, 105, 528, 121, 287, 767, 401, 688, 502, 680, 716, 718,
        299, 405, 288, 165, 689, 343, 663, 68, 26, 763, 590, 56, 114, 707, 677, 514, 617, 411, 64, 353, 175, 732, 87,
        327, 351, 261, 238, 524, 460, 508, 537, 691, 562, 719, 451, 95, 98, 309, 300, 743, 762, 209, 6, 116, 614, 216,
        501, 345, 363,
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