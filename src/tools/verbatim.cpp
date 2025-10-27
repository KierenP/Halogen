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
    constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 259, 362, 468, 78, 397, 52, 709, 515, 382, 279, 645,
        127, 83, 289, 194, 621, 351, 313, 653, 274, 385, 506, 450, 325, 53, 710, 647, 293, 567, 263, 160, 603, 723, 84,
        758, 577, 581, 539, 17, 360, 417, 622, 467, 485, 605, 594, 529, 734, 250, 738, 197, 327, 77, 425, 230, 701, 658,
        144, 625, 55, 489, 651, 334, 442, 283, 461, 260, 644, 536, 218, 174, 648, 69, 487, 665, 424, 229, 93, 730, 208,
        687, 764, 656, 534, 680, 261, 754, 469, 15, 109, 57, 14, 119, 466, 735, 649, 460, 711, 686, 641, 597, 736, 24,
        186, 322, 677, 717, 583, 244, 484, 75, 371, 674, 373, 584, 392, 372, 226, 242, 573, 694, 188, 576, 49, 354, 690,
        149, 700, 550, 475, 432, 0, 374, 378, 331, 294, 572, 123, 136, 342, 184, 72, 714, 537, 561, 147, 459, 171, 298,
        480, 97, 76, 402, 135, 317, 654, 46, 26, 390, 715, 429, 157, 32, 2, 634, 609, 724, 185, 152, 394, 266, 508, 199,
        138, 54, 175, 618, 422, 118, 312, 707, 124, 482, 269, 228, 162, 527, 516, 565, 280, 108, 201, 462, 222, 37, 332,
        189, 737, 255, 657, 483, 588, 554, 449, 65, 217, 726, 365, 423, 256, 712, 598, 44, 102, 268, 271, 137, 575, 154,
        501, 431, 702, 92, 214, 41, 106, 642, 210, 614, 438, 40, 220, 231, 749, 742, 190, 745, 499, 96, 333, 407, 23,
        284, 464, 383, 166, 246, 107, 1, 639, 16, 120, 112, 746, 195, 56, 302, 176, 513, 415, 224, 299, 81, 158, 3, 514,
        659, 219, 521, 128, 338, 509, 215, 273, 94, 275, 465, 323, 131, 587, 86, 556, 336, 477, 18, 615, 133, 733, 488,
        140, 290, 377, 620, 183, 213, 367, 763, 433, 205, 100, 297, 291, 519, 249, 379, 139, 316, 212, 491, 43, 105,
        126, 748, 452, 82, 492, 363, 728, 612, 580, 446, 388, 756, 704, 314, 148, 193, 412, 278, 85, 227, 369, 48, 59,
        305, 563, 414, 187, 29, 247, 163, 88, 241, 91, 427, 345, 181, 51, 761, 165, 202, 45, 167, 233, 308, 339, 95,
        396, 543, 716, 270, 276, 42, 232, 341, 182, 287, 225, 741, 533, 752, 688, 413, 495, 448, 375, 486, 115, 524,
        352, 348, 326, 689, 426, 267, 310, 593, 631, 358, 683, 122, 387, 590, 240, 254, 359, 663, 340, 713, 398, 239,
        435, 300, 318, 454, 613, 22, 235, 692, 301, 36, 337, 507, 281, 703, 169, 732, 180, 366, 168, 145, 530, 25, 406,
        303, 216, 557, 604, 525, 540, 221, 349, 497, 329, 636, 637, 541, 661, 155, 437, 629, 628, 8, 335, 463, 436, 272,
        411, 315, 203, 765, 389, 428, 453, 296, 89, 211, 403, 522, 251, 132, 599, 68, 706, 547, 62, 386, 63, 376, 110,
        743, 410, 420, 103, 638, 177, 473, 380, 750, 265, 99, 611, 517, 243, 395, 721, 553, 684, 304, 146, 490, 60, 532,
        209, 350, 623, 512, 496, 447, 150, 238, 471, 494, 71, 725, 472, 198, 381, 481, 502, 404, 253, 12, 493, 161, 153,
        627, 731, 321, 440, 101, 727, 421, 9, 676, 478, 526, 179, 277, 248, 762, 760, 479, 400, 223, 178, 393, 607, 64,
        306, 61, 90, 114, 19, 500, 38, 27, 578, 708, 292, 330, 113, 98, 564, 616, 111, 7, 408, 370, 455, 498, 368, 264,
        586, 520, 458, 552, 558, 568, 439, 20, 523, 295, 159, 347, 617, 258, 418, 511, 559, 668, 759, 582, 544, 722,
        585, 80, 549, 503, 401, 4, 571, 441, 28, 346, 419, 405, 164, 518, 504, 319, 172, 364, 535, 70, 311, 751, 11,
        555, 34, 660, 608, 669, 236, 344, 234, 6, 151, 50, 443, 635, 705, 531, 474, 320, 156, 626, 409, 31, 600, 630,
        510, 592, 288, 141, 451, 545, 767, 619, 74, 470, 601, 538, 456, 560, 13, 430, 21, 570, 391, 282, 650, 142, 361,
        173, 655, 207, 39, 595, 444, 47, 652, 134, 262, 356, 695, 666, 596, 640, 457, 624, 667, 196, 200, 589, 691, 121,
        307, 675, 664, 286, 672, 610, 130, 357, 384, 681, 237, 646, 671, 753, 632, 662, 548, 670, 693, 696, 309, 192,
        399, 602, 252, 757, 698, 129, 566, 739, 678, 204, 685, 10, 245, 416, 434, 643, 562, 58, 673, 718, 719, 33, 476,
        67, 5, 574, 720, 285, 206, 542, 355, 697, 591, 551, 633, 125, 104, 191, 328, 528, 606, 569, 353, 505, 740, 324,
        445, 343, 747, 73, 79, 170, 679, 729, 35, 699, 116, 682, 257, 546, 117, 744, 143, 579, 87, 30, 66, 766, 755,
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