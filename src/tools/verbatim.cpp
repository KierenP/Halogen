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
    constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 513, 66, 471, 398, 65, 658, 764, 610, 144, 142, 176,
        757, 73, 481, 442, 744, 489, 687, 430, 519, 751, 195, 25, 64, 302, 277, 84, 180, 733, 432, 574, 503, 523, 275,
        101, 168, 729, 674, 427, 468, 336, 108, 351, 232, 710, 387, 147, 648, 357, 459, 235, 730, 662, 53, 330, 312,
        399, 329, 569, 583, 549, 736, 428, 496, 691, 482, 460, 364, 83, 606, 56, 365, 640, 587, 568, 632, 491, 721, 62,
        78, 259, 216, 126, 515, 221, 657, 414, 709, 634, 283, 539, 88, 324, 30, 39, 649, 393, 82, 80, 603, 51, 38, 560,
        103, 589, 670, 397, 366, 475, 590, 171, 263, 373, 392, 307, 339, 124, 585, 611, 104, 58, 106, 695, 314, 173,
        231, 621, 287, 623, 385, 325, 233, 132, 601, 588, 703, 499, 367, 680, 440, 388, 642, 429, 117, 421, 417, 316,
        591, 96, 749, 525, 219, 476, 379, 288, 36, 685, 663, 450, 708, 317, 161, 331, 157, 300, 163, 49, 252, 439, 0,
        739, 342, 258, 42, 579, 119, 577, 766, 502, 350, 400, 181, 402, 71, 369, 160, 122, 334, 94, 262, 380, 554, 203,
        193, 172, 11, 358, 326, 609, 718, 354, 247, 224, 659, 204, 597, 110, 5, 747, 435, 767, 234, 311, 63, 689, 129,
        197, 630, 218, 449, 701, 743, 212, 643, 726, 225, 470, 639, 2, 490, 170, 27, 598, 448, 653, 538, 19, 446, 666,
        238, 3, 413, 105, 452, 698, 93, 651, 571, 677, 699, 102, 465, 616, 290, 254, 383, 148, 138, 613, 52, 401, 457,
        57, 189, 86, 550, 266, 1, 40, 120, 85, 114, 700, 158, 323, 304, 750, 697, 425, 599, 248, 321, 535, 91, 271, 575,
        497, 165, 761, 115, 415, 285, 133, 693, 431, 347, 274, 162, 755, 299, 281, 143, 127, 188, 113, 675, 363, 174,
        376, 615, 405, 152, 443, 472, 423, 719, 92, 68, 669, 229, 544, 130, 141, 483, 716, 723, 600, 87, 567, 527, 389,
        55, 89, 495, 289, 566, 618, 265, 403, 166, 593, 732, 192, 348, 727, 724, 327, 335, 17, 390, 501, 318, 146, 596,
        604, 306, 240, 139, 270, 545, 395, 510, 61, 207, 8, 309, 509, 187, 77, 291, 276, 352, 222, 72, 223, 24, 370, 79,
        627, 386, 13, 303, 128, 199, 293, 182, 74, 239, 748, 257, 486, 355, 43, 511, 362, 228, 458, 745, 612, 562, 255,
        605, 581, 260, 665, 712, 353, 765, 607, 45, 343, 561, 762, 504, 679, 391, 372, 18, 416, 692, 660, 728, 137, 754,
        541, 167, 280, 243, 436, 396, 763, 411, 547, 118, 426, 419, 194, 690, 313, 629, 264, 756, 297, 500, 420, 211,
        279, 445, 681, 107, 740, 149, 537, 447, 454, 60, 438, 190, 456, 381, 54, 715, 626, 16, 617, 37, 214, 125, 322,
        624, 32, 121, 109, 241, 508, 474, 684, 473, 356, 441, 668, 261, 480, 650, 295, 404, 95, 145, 220, 349, 488, 209,
        200, 111, 558, 686, 477, 267, 406, 90, 206, 26, 455, 31, 215, 556, 34, 507, 696, 169, 408, 676, 46, 654, 682,
        655, 407, 6, 517, 208, 178, 268, 28, 487, 526, 33, 528, 305, 521, 123, 81, 4, 41, 466, 47, 533, 580, 198, 532,
        48, 286, 540, 213, 15, 333, 100, 7, 217, 529, 418, 308, 14, 201, 320, 553, 555, 530, 463, 625, 424, 338, 273,
        559, 714, 564, 565, 678, 179, 516, 112, 479, 753, 226, 237, 337, 572, 278, 563, 378, 573, 646, 359, 620, 493,
        368, 518, 20, 462, 116, 296, 631, 720, 551, 340, 594, 734, 637, 494, 694, 136, 186, 50, 140, 282, 244, 622, 292,
        534, 546, 134, 578, 328, 737, 382, 245, 453, 531, 444, 191, 98, 251, 633, 12, 249, 156, 608, 412, 552, 377, 467,
        584, 67, 641, 230, 371, 246, 638, 557, 464, 175, 451, 344, 619, 437, 644, 645, 570, 159, 586, 131, 298, 150,
        735, 164, 332, 543, 151, 10, 22, 522, 75, 524, 536, 688, 498, 155, 236, 667, 759, 758, 154, 506, 256, 760, 153,
        614, 59, 242, 360, 707, 664, 9, 661, 582, 548, 520, 177, 410, 683, 652, 69, 434, 319, 595, 514, 469, 301, 253,
        183, 512, 636, 628, 702, 742, 205, 394, 384, 673, 635, 711, 272, 185, 196, 741, 227, 704, 97, 485, 70, 21, 505,
        433, 250, 374, 99, 722, 202, 294, 269, 361, 602, 713, 671, 310, 29, 647, 135, 672, 738, 592, 409, 35, 542, 461,
        725, 345, 746, 484, 706, 731, 717, 492, 478, 76, 752, 284, 576, 705, 23, 184, 346, 375, 422, 315, 656, 44, 210,
        341,
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