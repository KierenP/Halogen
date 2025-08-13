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
    constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 388, 108, 237, 682, 399, 193, 36, 351, 478, 594, 665,
        259, 423, 600, 749, 767, 467, 442, 148, 548, 751, 402, 322, 380, 51, 171, 261, 608, 483, 565, 714, 765, 550,
        763, 431, 300, 373, 649, 477, 39, 726, 357, 633, 657, 156, 761, 458, 10, 53, 321, 748, 754, 759, 720, 16, 640,
        515, 5, 256, 415, 444, 652, 604, 410, 717, 81, 660, 335, 757, 152, 7, 623, 696, 646, 345, 637, 298, 752, 70,
        275, 425, 336, 127, 8, 725, 196, 460, 191, 629, 361, 238, 102, 279, 381, 80, 94, 472, 115, 98, 302, 194, 426,
        290, 48, 137, 323, 245, 103, 572, 52, 500, 260, 555, 247, 744, 372, 253, 233, 312, 428, 734, 687, 292, 432, 632,
        69, 89, 570, 120, 132, 217, 563, 689, 501, 134, 188, 131, 262, 360, 139, 400, 183, 142, 180, 732, 133, 113, 57,
        93, 314, 430, 106, 296, 475, 636, 230, 495, 187, 293, 370, 503, 519, 189, 118, 663, 42, 265, 151, 411, 712, 747,
        391, 455, 165, 30, 518, 758, 278, 327, 685, 699, 538, 739, 707, 62, 667, 308, 582, 418, 456, 155, 439, 283, 47,
        698, 26, 200, 220, 92, 371, 273, 186, 437, 248, 673, 332, 452, 125, 551, 644, 55, 738, 128, 522, 339, 509, 271,
        481, 523, 61, 463, 216, 158, 86, 691, 510, 68, 225, 160, 101, 722, 228, 13, 599, 299, 29, 18, 375, 479, 231,
        232, 37, 705, 210, 239, 63, 384, 369, 741, 645, 34, 251, 184, 677, 22, 695, 711, 413, 14, 246, 499, 628, 693,
        692, 727, 319, 603, 420, 90, 284, 484, 130, 655, 591, 480, 59, 269, 9, 267, 575, 264, 552, 170, 109, 621, 138,
        377, 287, 491, 742, 226, 291, 153, 338, 145, 202, 618, 224, 0, 234, 516, 553, 240, 631, 448, 4, 274, 661, 648,
        313, 352, 218, 387, 556, 412, 140, 679, 489, 615, 492, 653, 395, 427, 122, 280, 17, 326, 581, 435, 82, 282, 40,
        614, 65, 306, 671, 533, 675, 662, 43, 111, 285, 28, 337, 46, 121, 236, 329, 163, 309, 497, 688, 344, 164, 95,
        166, 438, 641, 295, 358, 577, 181, 602, 85, 634, 277, 350, 64, 272, 270, 97, 446, 566, 393, 116, 305, 307, 417,
        736, 471, 601, 490, 316, 638, 328, 382, 123, 45, 169, 24, 15, 318, 647, 320, 79, 263, 286, 172, 630, 73, 730,
        485, 743, 513, 315, 74, 199, 50, 182, 281, 454, 162, 19, 366, 362, 549, 405, 708, 356, 540, 346, 701, 721, 12,
        389, 416, 241, 469, 588, 185, 520, 190, 88, 571, 157, 433, 639, 606, 459, 397, 119, 229, 624, 443, 175, 71, 686,
        355, 451, 379, 728, 536, 197, 453, 609, 331, 288, 450, 99, 49, 112, 340, 347, 124, 424, 334, 173, 179, 392, 114,
        745, 206, 464, 524, 311, 255, 440, 96, 473, 706, 303, 268, 249, 209, 421, 535, 149, 700, 468, 401, 664, 493, 20,
        414, 385, 6, 496, 595, 737, 635, 66, 368, 136, 211, 528, 504, 135, 107, 436, 242, 129, 365, 154, 409, 41, 502,
        514, 76, 222, 310, 223, 44, 304, 75, 724, 422, 505, 250, 195, 620, 498, 457, 150, 205, 143, 659, 586, 23, 408,
        539, 177, 465, 474, 167, 650, 593, 568, 537, 403, 462, 429, 733, 713, 54, 363, 212, 545, 254, 201, 558, 561,
        554, 476, 147, 176, 643, 407, 266, 617, 546, 557, 507, 11, 146, 508, 1, 678, 77, 333, 579, 486, 574, 531, 470,
        583, 168, 543, 584, 669, 590, 755, 441, 611, 461, 750, 672, 710, 596, 324, 31, 597, 342, 517, 482, 32, 506, 576,
        390, 87, 598, 419, 203, 544, 527, 252, 512, 530, 67, 434, 619, 207, 289, 616, 740, 704, 159, 445, 626, 525, 297,
        178, 35, 449, 613, 235, 126, 670, 244, 330, 243, 612, 690, 559, 466, 542, 642, 386, 376, 58, 396, 589, 367, 398,
        348, 760, 494, 214, 341, 658, 622, 105, 141, 651, 605, 764, 756, 746, 560, 541, 580, 487, 694, 573, 674, 91, 78,
        562, 276, 564, 666, 383, 354, 27, 680, 547, 56, 192, 684, 676, 325, 3, 364, 38, 569, 488, 100, 592, 144, 534,
        104, 532, 683, 378, 703, 406, 716, 110, 174, 359, 374, 161, 317, 697, 681, 343, 709, 511, 668, 718, 258, 404,
        578, 627, 723, 33, 2, 719, 585, 60, 607, 117, 731, 753, 25, 221, 294, 735, 257, 521, 702, 353, 208, 301, 349,
        219, 394, 83, 715, 72, 625, 447, 526, 84, 656, 213, 198, 529, 654, 21, 729, 215, 227, 587, 610, 762, 204, 766,
        567,
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