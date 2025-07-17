// Load the network file, do some preprocessing, and then save it to the build directory for inclusion in the final
// binary

#include "network/arch.hpp"
#include "network/simd/definitions.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>

using namespace NN;

struct raw_network
{
    alignas(64) std::array<std::array<int16_t, FT_SIZE>, INPUT_SIZE * KING_BUCKET_COUNT> ft_weight = {};
    alignas(64) std::array<int16_t, FT_SIZE> ft_bias = {};
    alignas(64) std::array<std::array<std::array<int16_t, FT_SIZE>, L1_SIZE>, OUTPUT_BUCKETS> l1_weight = {};
    alignas(64) std::array<std::array<int16_t, L1_SIZE>, OUTPUT_BUCKETS> l1_bias = {};
    alignas(64) std::array<std::array<std::array<float, L1_SIZE>, L2_SIZE>, OUTPUT_BUCKETS> l2_weight = {};
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

    // clang-format off
    constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 268, 86, 552, 758, 459, 217, 280, 743, 516, 621, 315,
        661, 473, 686, 42, 219, 596, 726, 402, 140, 342, 423, 464, 576, 150, 546, 104, 323, 674, 710, 390, 521, 684,
        413, 97, 538, 39, 391, 670, 91, 694, 401, 455, 175, 369, 718, 68, 676, 478, 41, 78, 48, 480, 348, 0, 729, 723,
        80, 56, 600, 499, 432, 579, 12, 519, 162, 534, 671, 548, 212, 506, 289, 102, 589, 6, 692, 607, 40, 653, 95, 75,
        198, 753, 555, 704, 421, 333, 759, 194, 748, 437, 360, 20, 524, 716, 201, 67, 681, 170, 206, 761, 656, 765, 101,
        530, 713, 406, 225, 71, 669, 230, 13, 4, 152, 618, 609, 539, 171, 409, 29, 184, 571, 329, 663, 216, 338, 495,
        445, 119, 732, 666, 701, 124, 292, 189, 137, 337, 378, 322, 683, 325, 185, 561, 357, 7, 207, 144, 489, 35, 257,
        387, 689, 11, 536, 254, 397, 585, 417, 460, 159, 685, 658, 5, 564, 100, 187, 353, 611, 218, 465, 73, 490, 641,
        220, 24, 764, 717, 469, 200, 583, 85, 371, 598, 575, 172, 767, 688, 158, 293, 563, 191, 134, 221, 414, 123, 447,
        537, 89, 405, 627, 572, 573, 14, 19, 258, 461, 44, 335, 129, 227, 426, 204, 211, 9, 252, 435, 163, 345, 226, 10,
        375, 442, 601, 76, 471, 168, 569, 28, 648, 757, 454, 482, 66, 381, 302, 486, 436, 501, 492, 587, 540, 518, 298,
        359, 96, 625, 568, 36, 160, 324, 232, 64, 114, 597, 145, 250, 205, 590, 256, 79, 744, 340, 639, 69, 527, 246,
        223, 449, 113, 301, 213, 408, 283, 259, 112, 46, 398, 498, 275, 27, 606, 470, 532, 249, 195, 541, 72, 617, 451,
        314, 672, 326, 147, 652, 543, 32, 631, 233, 738, 148, 222, 300, 745, 411, 514, 38, 725, 307, 276, 167, 141, 154,
        443, 664, 274, 8, 752, 570, 622, 483, 619, 118, 74, 510, 286, 90, 87, 380, 507, 463, 736, 319, 130, 18, 182,
        719, 673, 271, 34, 477, 270, 193, 253, 644, 339, 751, 741, 81, 614, 620, 479, 173, 403, 603, 400, 580, 240, 290,
        358, 121, 756, 3, 215, 415, 332, 330, 58, 183, 578, 494, 202, 425, 105, 122, 766, 544, 62, 128, 304, 379, 161,
        389, 382, 49, 749, 388, 386, 294, 260, 640, 282, 106, 632, 392, 610, 214, 103, 560, 643, 235, 429, 92, 395, 453,
        584, 450, 594, 376, 188, 662, 368, 349, 1, 467, 236, 394, 288, 199, 284, 407, 642, 366, 706, 715, 613, 724, 77,
        279, 310, 424, 47, 116, 700, 192, 151, 308, 737, 373, 372, 746, 566, 377, 331, 131, 399, 285, 760, 431, 231, 70,
        628, 410, 186, 177, 317, 513, 30, 278, 135, 448, 174, 157, 762, 535, 84, 248, 509, 420, 237, 682, 690, 695, 352,
        224, 474, 347, 362, 385, 57, 203, 343, 54, 238, 427, 355, 485, 553, 739, 457, 712, 328, 138, 266, 476, 210, 180,
        296, 567, 444, 702, 169, 493, 500, 428, 528, 136, 178, 502, 505, 412, 462, 99, 53, 481, 234, 107, 559, 595, 504,
        365, 316, 361, 497, 637, 149, 525, 109, 264, 404, 623, 281, 522, 132, 295, 646, 196, 176, 350, 557, 318, 556,
        615, 626, 419, 588, 422, 120, 554, 277, 733, 164, 551, 153, 529, 441, 263, 488, 545, 558, 668, 430, 303, 336,
        209, 438, 439, 542, 229, 15, 616, 94, 370, 416, 582, 708, 512, 255, 533, 273, 59, 517, 133, 228, 287, 320, 654,
        678, 125, 384, 356, 261, 742, 727, 487, 691, 635, 37, 239, 456, 156, 591, 241, 197, 272, 269, 31, 111, 418, 697,
        446, 26, 593, 344, 634, 291, 612, 750, 143, 354, 139, 110, 155, 508, 433, 242, 108, 52, 599, 592, 166, 604, 581,
        491, 633, 496, 247, 299, 351, 33, 434, 88, 547, 142, 267, 468, 511, 638, 624, 636, 245, 659, 699, 334, 61, 523,
        655, 243, 51, 503, 341, 127, 309, 574, 363, 25, 660, 50, 629, 602, 45, 675, 707, 472, 346, 740, 526, 714, 515,
        677, 305, 115, 297, 679, 21, 117, 650, 251, 649, 680, 244, 55, 549, 321, 374, 383, 687, 396, 484, 630, 452, 126,
        562, 265, 667, 730, 22, 364, 327, 190, 208, 657, 98, 93, 313, 763, 311, 645, 698, 647, 475, 17, 721, 720, 696,
        306, 179, 82, 531, 693, 711, 2, 731, 705, 651, 734, 735, 586, 550, 43, 65, 709, 665, 60, 63, 466, 608, 728, 605,
        312, 16, 577, 181, 165, 146, 754, 747, 755, 393, 262, 23, 722, 83, 458, 703, 565, 440, 520, 367,
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
    auto output = std::make_unique<std::array<std::array<std::array<float, L2_SIZE>, L1_SIZE>, OUTPUT_BUCKETS>>();

    for (size_t i = 0; i < OUTPUT_BUCKETS; i++)
    {
        for (size_t j = 0; j < L1_SIZE; j++)
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
    alignas(64) std::array<std::array<std::array<float, L2_SIZE>, L1_SIZE>, OUTPUT_BUCKETS> l2_weight = {};
    alignas(64) std::array<std::array<float, L2_SIZE>, OUTPUT_BUCKETS> l2_bias = {};
    alignas(64) std::array<std::array<float, L2_SIZE>, OUTPUT_BUCKETS> l3_weight = {};
    alignas(64) std::array<float, OUTPUT_BUCKETS> l3_bias = {};
};

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