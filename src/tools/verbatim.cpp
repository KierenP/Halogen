// Load the network file, do some preprocessing, and then save it to the build directory for inclusion in the final
// binary

#include "network/arch.hpp"
#include "network/simd/define.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <tuple>
#include <utility>

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
    constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 375, 12, 732, 764, 169, 458, 676, 378, 179, 65, 362,
        128, 450, 229, 755, 384, 690, 505, 42, 412, 713, 17, 315, 259, 511, 415, 435, 484, 358, 352, 225, 553, 727, 545,
        246, 372, 456, 227, 239, 679, 622, 51, 73, 40, 357, 611, 277, 413, 385, 650, 519, 118, 753, 33, 563, 318, 443,
        410, 63, 745, 411, 238, 345, 188, 720, 759, 356, 723, 736, 455, 81, 78, 691, 164, 7, 678, 503, 67, 364, 199, 52,
        301, 31, 593, 8, 668, 330, 626, 9, 444, 621, 595, 438, 739, 556, 562, 208, 268, 24, 541, 396, 582, 701, 767,
        334, 473, 111, 760, 703, 460, 0, 404, 737, 287, 203, 99, 637, 365, 733, 74, 237, 326, 765, 694, 202, 640, 628,
        752, 402, 536, 465, 62, 449, 485, 84, 117, 132, 409, 212, 343, 223, 420, 122, 508, 300, 568, 80, 722, 83, 688,
        735, 516, 659, 462, 651, 457, 243, 77, 693, 15, 252, 636, 200, 704, 414, 174, 373, 70, 543, 349, 145, 107, 454,
        90, 156, 514, 147, 613, 376, 249, 325, 741, 182, 486, 135, 191, 266, 260, 618, 754, 312, 395, 283, 307, 647, 21,
        526, 400, 323, 214, 419, 317, 535, 716, 278, 453, 762, 575, 645, 746, 281, 700, 152, 213, 34, 38, 608, 683, 499,
        403, 596, 221, 570, 98, 734, 89, 319, 247, 696, 442, 721, 94, 22, 371, 531, 643, 527, 46, 295, 507, 44, 421, 26,
        534, 617, 37, 313, 491, 424, 45, 116, 236, 387, 638, 446, 244, 483, 91, 14, 263, 597, 743, 441, 194, 433, 521,
        154, 127, 258, 131, 257, 271, 159, 272, 380, 388, 724, 430, 542, 303, 56, 16, 280, 170, 1, 189, 342, 110, 264,
        133, 537, 590, 279, 656, 340, 551, 682, 744, 218, 436, 569, 619, 632, 134, 299, 48, 20, 329, 467, 276, 567, 311,
        610, 114, 641, 210, 580, 242, 310, 13, 468, 728, 524, 321, 124, 168, 6, 327, 354, 718, 190, 322, 255, 339, 292,
        59, 3, 399, 274, 50, 139, 341, 284, 87, 18, 719, 463, 115, 294, 55, 298, 648, 654, 706, 649, 355, 550, 730, 143,
        344, 220, 666, 248, 738, 363, 267, 405, 574, 368, 490, 370, 408, 86, 577, 360, 401, 506, 215, 92, 296, 308, 205,
        366, 177, 423, 104, 426, 432, 103, 173, 148, 391, 601, 146, 250, 594, 504, 517, 286, 756, 439, 270, 766, 53, 11,
        554, 673, 589, 478, 241, 564, 291, 497, 49, 674, 757, 748, 451, 331, 195, 338, 351, 417, 138, 603, 359, 176,
        129, 232, 157, 297, 121, 112, 275, 434, 306, 125, 141, 219, 93, 302, 684, 136, 102, 192, 353, 211, 664, 592,
        245, 374, 35, 675, 571, 428, 379, 183, 184, 464, 47, 502, 488, 95, 185, 461, 256, 198, 494, 687, 686, 60, 437,
        591, 515, 620, 429, 285, 476, 10, 119, 474, 481, 565, 41, 120, 158, 470, 106, 305, 361, 105, 445, 25, 459, 289,
        394, 282, 162, 216, 689, 418, 320, 100, 578, 587, 253, 126, 529, 166, 710, 393, 469, 669, 498, 646, 109, 549,
        557, 97, 149, 230, 500, 522, 475, 88, 71, 79, 350, 347, 602, 142, 447, 625, 533, 382, 171, 406, 599, 709, 96,
        386, 101, 422, 231, 544, 653, 758, 4, 224, 113, 64, 440, 262, 163, 367, 572, 528, 197, 167, 175, 615, 714, 407,
        540, 761, 54, 548, 155, 130, 204, 699, 392, 222, 240, 573, 153, 482, 576, 512, 566, 151, 600, 547, 193, 558,
        234, 532, 585, 471, 377, 560, 740, 586, 493, 172, 581, 501, 265, 27, 583, 369, 658, 144, 705, 288, 751, 546,
        427, 742, 254, 58, 180, 559, 452, 324, 672, 667, 61, 82, 32, 346, 187, 631, 66, 606, 598, 68, 181, 623, 520,
        612, 416, 165, 633, 671, 630, 681, 677, 251, 639, 29, 390, 348, 76, 635, 316, 381, 480, 178, 644, 614, 383, 652,
        261, 642, 552, 466, 555, 489, 23, 217, 634, 335, 663, 150, 665, 657, 609, 496, 579, 660, 448, 670, 561, 30, 233,
        477, 290, 509, 510, 627, 729, 747, 269, 655, 28, 57, 530, 123, 5, 750, 43, 314, 624, 697, 680, 36, 85, 72, 201,
        523, 661, 337, 137, 293, 707, 692, 588, 39, 431, 495, 69, 518, 196, 328, 333, 715, 708, 479, 304, 702, 492, 629,
        662, 525, 538, 207, 228, 513, 604, 711, 487, 206, 397, 309, 332, 209, 717, 616, 186, 235, 398, 425, 140, 160,
        712, 685, 108, 539, 695, 161, 607, 584, 273, 726, 731, 749, 226, 2, 75, 698, 389, 605, 763, 725, 19, 336, 472,
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