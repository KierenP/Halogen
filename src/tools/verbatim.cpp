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
    constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 767, 184, 89, 121, 643, 91, 80, 36, 24, 696, 203, 590,
        674, 744, 656, 329, 760, 462, 435, 418, 394, 124, 577, 392, 58, 360, 140, 498, 761, 387, 699, 570, 741, 290,
        658, 527, 344, 117, 388, 16, 502, 640, 69, 733, 534, 484, 298, 262, 249, 52, 165, 104, 734, 720, 30, 350, 189,
        151, 226, 691, 706, 754, 102, 561, 368, 65, 18, 673, 664, 97, 408, 599, 764, 537, 251, 63, 384, 637, 442, 326,
        618, 76, 139, 321, 456, 25, 693, 758, 716, 202, 79, 8, 246, 752, 622, 64, 353, 623, 358, 274, 649, 327, 29, 260,
        371, 762, 1, 400, 286, 267, 726, 154, 465, 475, 528, 168, 756, 646, 331, 210, 652, 98, 308, 341, 169, 519, 126,
        105, 461, 273, 573, 231, 133, 690, 659, 422, 380, 463, 620, 324, 332, 663, 84, 132, 552, 487, 33, 263, 68, 77,
        455, 479, 482, 529, 303, 632, 692, 299, 100, 736, 339, 480, 37, 571, 556, 713, 166, 233, 715, 701, 651, 109,
        605, 74, 722, 106, 170, 396, 270, 338, 666, 115, 318, 101, 161, 0, 753, 514, 589, 639, 23, 395, 172, 242, 675,
        34, 596, 147, 688, 763, 153, 114, 204, 187, 118, 386, 516, 765, 499, 119, 7, 92, 206, 665, 645, 289, 252, 667,
        459, 625, 414, 399, 112, 312, 657, 183, 507, 382, 550, 346, 600, 27, 739, 490, 419, 137, 473, 212, 377, 544,
        679, 574, 234, 62, 236, 354, 283, 32, 530, 578, 14, 250, 627, 389, 702, 601, 99, 188, 163, 460, 743, 661, 592,
        2, 315, 370, 437, 721, 340, 158, 125, 248, 222, 22, 113, 108, 481, 61, 369, 51, 617, 501, 221, 216, 430, 328,
        429, 173, 343, 500, 245, 452, 567, 680, 26, 257, 138, 292, 53, 266, 55, 282, 269, 515, 553, 304, 558, 307, 217,
        301, 564, 311, 612, 474, 506, 6, 697, 157, 505, 737, 28, 193, 433, 181, 439, 376, 668, 40, 237, 333, 239, 457,
        5, 731, 405, 47, 178, 201, 179, 220, 402, 258, 67, 232, 54, 683, 740, 122, 305, 244, 56, 438, 543, 278, 410,
        468, 194, 87, 757, 276, 134, 510, 421, 265, 268, 466, 31, 146, 359, 38, 325, 164, 488, 41, 723, 45, 608, 209,
        607, 83, 467, 313, 436, 167, 159, 160, 306, 186, 199, 381, 78, 727, 535, 409, 123, 751, 129, 96, 75, 526, 504,
        477, 361, 750, 302, 288, 533, 271, 628, 694, 13, 215, 495, 367, 15, 681, 508, 591, 177, 196, 718, 416, 116, 10,
        88, 174, 413, 379, 403, 107, 254, 717, 654, 606, 211, 523, 363, 253, 375, 609, 549, 295, 738, 264, 240, 548,
        478, 272, 448, 44, 291, 441, 669, 525, 73, 420, 314, 464, 522, 317, 458, 383, 485, 128, 81, 742, 176, 348, 415,
        319, 11, 280, 155, 703, 300, 19, 476, 428, 35, 641, 103, 275, 49, 732, 469, 655, 562, 489, 424, 431, 241, 766,
        520, 171, 454, 150, 554, 391, 141, 152, 494, 143, 294, 446, 503, 689, 255, 470, 224, 728, 345, 397, 725, 648,
        580, 42, 180, 492, 521, 323, 512, 335, 483, 619, 407, 631, 444, 135, 559, 471, 604, 95, 390, 563, 357, 156, 50,
        71, 46, 145, 330, 296, 542, 597, 449, 214, 451, 82, 445, 192, 524, 531, 698, 511, 86, 131, 279, 427, 557, 545,
        277, 603, 347, 566, 93, 540, 136, 406, 551, 569, 496, 447, 365, 572, 565, 425, 443, 450, 182, 43, 660, 372, 191,
        9, 568, 148, 144, 598, 700, 404, 581, 517, 576, 729, 401, 595, 229, 710, 588, 493, 584, 205, 453, 602, 671, 235,
        70, 320, 614, 243, 374, 630, 541, 613, 587, 426, 398, 309, 594, 513, 575, 21, 336, 94, 238, 378, 207, 364, 57,
        585, 195, 223, 586, 337, 60, 434, 583, 423, 611, 676, 12, 638, 366, 316, 228, 748, 417, 593, 17, 616, 208, 432,
        719, 759, 755, 746, 3, 633, 647, 610, 532, 629, 175, 582, 110, 66, 261, 149, 230, 218, 670, 650, 310, 635, 653,
        351, 219, 672, 678, 412, 626, 256, 682, 127, 579, 539, 677, 72, 281, 227, 555, 686, 684, 284, 393, 225, 644,
        120, 39, 90, 624, 190, 695, 111, 486, 297, 705, 322, 411, 213, 560, 714, 712, 349, 634, 538, 355, 285, 711, 621,
        685, 373, 287, 334, 197, 385, 547, 130, 687, 724, 730, 745, 707, 708, 518, 472, 162, 709, 342, 362, 704, 440,
        185, 142, 509, 259, 20, 200, 636, 356, 749, 85, 48, 642, 615, 4, 497, 352, 747, 536, 491, 59, 198, 662, 247,
        735, 293, 546,
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