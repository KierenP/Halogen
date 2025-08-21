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
    constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 502, 292, 302, 105, 681, 741, 346, 581, 473, 400, 235,
        686, 446, 47, 413, 503, 0, 43, 479, 342, 361, 690, 701, 486, 123, 116, 353, 379, 434, 506, 738, 395, 77, 33, 72,
        36, 508, 398, 90, 406, 170, 354, 74, 327, 524, 358, 760, 24, 747, 240, 262, 401, 540, 499, 268, 624, 511, 598,
        743, 542, 364, 538, 646, 56, 763, 585, 391, 253, 206, 130, 131, 696, 541, 500, 594, 273, 23, 68, 754, 710, 78,
        526, 749, 308, 734, 616, 642, 675, 19, 84, 81, 426, 98, 238, 731, 704, 117, 431, 445, 15, 429, 610, 167, 451,
        665, 44, 359, 609, 150, 456, 470, 680, 326, 725, 635, 5, 59, 86, 30, 101, 63, 278, 460, 161, 591, 423, 279, 91,
        528, 394, 490, 565, 28, 64, 110, 570, 67, 730, 245, 333, 335, 752, 748, 121, 356, 553, 176, 476, 225, 4, 521,
        227, 755, 589, 271, 241, 160, 468, 703, 159, 404, 215, 761, 550, 197, 146, 226, 443, 631, 619, 34, 417, 10, 383,
        165, 673, 357, 190, 8, 300, 689, 676, 11, 711, 707, 525, 162, 457, 708, 172, 171, 48, 636, 756, 380, 275, 714,
        604, 198, 573, 678, 566, 26, 560, 350, 3, 522, 246, 718, 362, 567, 488, 600, 102, 614, 211, 179, 666, 25, 519,
        371, 543, 369, 671, 764, 75, 158, 664, 757, 411, 742, 683, 12, 196, 182, 463, 321, 407, 288, 237, 41, 475, 740,
        661, 647, 534, 73, 120, 388, 58, 645, 304, 483, 706, 409, 118, 579, 249, 529, 595, 535, 244, 532, 744, 478, 27,
        750, 174, 375, 53, 367, 561, 76, 578, 399, 373, 252, 745, 224, 39, 54, 564, 263, 729, 719, 311, 592, 533, 277,
        21, 289, 697, 316, 65, 341, 267, 191, 42, 296, 649, 219, 115, 169, 608, 132, 9, 586, 441, 513, 669, 280, 454,
        192, 345, 6, 51, 629, 242, 435, 143, 340, 40, 254, 214, 255, 318, 568, 372, 228, 325, 295, 187, 199, 113, 332,
        256, 29, 536, 461, 599, 60, 52, 312, 18, 298, 365, 539, 140, 175, 336, 97, 545, 283, 22, 360, 194, 575, 732,
        517, 231, 149, 139, 168, 485, 633, 416, 670, 69, 694, 520, 767, 207, 474, 297, 618, 627, 154, 314, 331, 136,
        338, 376, 334, 717, 459, 71, 422, 713, 552, 625, 390, 216, 392, 247, 329, 248, 688, 396, 82, 144, 684, 587, 195,
        62, 667, 663, 726, 695, 644, 436, 147, 378, 412, 402, 339, 418, 184, 421, 603, 427, 142, 408, 265, 716, 370, 35,
        425, 153, 148, 232, 349, 384, 303, 243, 657, 124, 432, 590, 291, 439, 537, 430, 547, 181, 765, 632, 549, 261,
        66, 166, 343, 677, 266, 651, 284, 344, 108, 208, 135, 393, 13, 122, 640, 282, 424, 239, 1, 467, 403, 114, 368,
        177, 49, 88, 112, 93, 597, 347, 580, 189, 712, 307, 480, 17, 577, 157, 615, 450, 128, 672, 301, 70, 507, 722,
        493, 569, 138, 724, 487, 100, 702, 758, 687, 652, 494, 173, 281, 505, 452, 419, 496, 323, 509, 551, 389, 442, 2,
        186, 518, 104, 448, 164, 274, 83, 733, 472, 16, 440, 126, 428, 530, 447, 155, 410, 654, 449, 180, 588, 736, 751,
        514, 576, 491, 200, 299, 385, 328, 462, 504, 634, 546, 420, 455, 659, 516, 746, 415, 611, 558, 156, 555, 639,
        562, 617, 330, 203, 294, 129, 691, 87, 501, 119, 322, 387, 310, 515, 700, 55, 544, 582, 583, 693, 188, 286, 469,
        465, 727, 313, 497, 348, 217, 107, 152, 285, 453, 620, 613, 178, 45, 134, 111, 233, 287, 352, 236, 548, 127,
        601, 753, 605, 106, 584, 259, 183, 234, 612, 14, 492, 212, 484, 630, 762, 607, 220, 397, 202, 222, 623, 46, 735,
        257, 628, 495, 315, 141, 437, 596, 258, 637, 721, 260, 641, 31, 572, 32, 656, 185, 319, 193, 366, 355, 638, 554,
        643, 571, 489, 458, 648, 622, 320, 79, 660, 766, 269, 653, 137, 85, 382, 386, 563, 498, 99, 103, 306, 482, 210,
        444, 151, 679, 290, 96, 125, 309, 527, 466, 626, 305, 251, 685, 682, 80, 133, 7, 230, 662, 95, 471, 692, 557,
        351, 61, 650, 337, 218, 481, 705, 270, 201, 593, 293, 574, 709, 276, 699, 606, 324, 510, 37, 363, 38, 759, 602,
        405, 438, 414, 213, 145, 94, 205, 559, 264, 209, 728, 668, 20, 89, 204, 163, 512, 377, 723, 374, 221, 477, 737,
        317, 655, 272, 715, 50, 556, 720, 57, 92, 250, 381, 229, 531, 464, 621, 523, 739, 433, 674, 658, 109, 698, 223,
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