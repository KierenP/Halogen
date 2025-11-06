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
    constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 749, 133, 501, 676, 488, 321, 567, 601, 78, 637, 717,
        47, 528, 272, 674, 304, 174, 585, 293, 120, 355, 537, 39, 365, 22, 686, 573, 264, 521, 636, 754, 208, 9, 746,
        446, 701, 405, 451, 13, 95, 705, 60, 654, 404, 583, 631, 680, 702, 347, 55, 738, 599, 378, 295, 600, 706, 7,
        634, 553, 307, 733, 556, 667, 376, 473, 525, 143, 348, 357, 188, 670, 353, 32, 756, 407, 18, 492, 410, 234, 252,
        587, 395, 758, 196, 236, 209, 93, 458, 181, 24, 17, 463, 255, 707, 56, 303, 71, 653, 352, 649, 191, 703, 572,
        354, 561, 28, 720, 507, 135, 106, 356, 23, 610, 747, 247, 725, 399, 138, 630, 100, 566, 166, 122, 594, 482, 81,
        67, 146, 598, 578, 290, 704, 223, 673, 104, 619, 375, 73, 685, 562, 761, 325, 10, 593, 767, 72, 222, 310, 464,
        137, 381, 710, 232, 607, 109, 267, 281, 287, 90, 85, 124, 341, 312, 396, 691, 144, 161, 734, 765, 343, 205, 314,
        709, 542, 300, 559, 638, 742, 469, 156, 546, 298, 16, 602, 548, 557, 171, 319, 693, 121, 711, 233, 157, 699, 61,
        294, 291, 626, 438, 129, 80, 434, 741, 97, 187, 195, 678, 54, 444, 437, 79, 211, 268, 263, 125, 155, 204, 622,
        130, 35, 745, 592, 448, 27, 151, 590, 226, 322, 51, 285, 335, 692, 92, 31, 379, 186, 470, 203, 216, 635, 165,
        530, 554, 728, 440, 688, 183, 215, 323, 197, 579, 111, 447, 739, 665, 493, 346, 762, 419, 249, 277, 597, 344,
        254, 508, 64, 420, 159, 514, 176, 620, 461, 266, 286, 88, 642, 748, 40, 91, 68, 595, 604, 45, 640, 744, 339, 29,
        37, 421, 289, 87, 50, 764, 436, 371, 552, 184, 740, 564, 659, 369, 173, 269, 15, 517, 372, 256, 306, 639, 280,
        574, 46, 107, 262, 8, 435, 657, 621, 766, 220, 48, 265, 697, 102, 309, 696, 224, 320, 4, 278, 330, 38, 326, 752,
        49, 149, 162, 329, 112, 700, 394, 524, 423, 105, 430, 311, 551, 77, 485, 243, 248, 41, 230, 99, 411, 164, 651,
        336, 392, 66, 661, 98, 432, 655, 349, 74, 308, 340, 456, 540, 136, 516, 374, 512, 532, 363, 370, 315, 377, 668,
        172, 684, 229, 650, 633, 147, 193, 427, 231, 413, 108, 390, 11, 213, 539, 318, 301, 206, 385, 491, 288, 603,
        160, 334, 504, 402, 550, 588, 616, 63, 605, 175, 260, 712, 132, 201, 69, 383, 210, 459, 5, 408, 422, 664, 536,
        279, 250, 324, 484, 417, 506, 366, 481, 535, 168, 681, 433, 154, 526, 59, 570, 645, 442, 283, 689, 246, 117,
        646, 618, 439, 581, 462, 70, 629, 453, 170, 228, 388, 547, 1, 119, 389, 412, 198, 235, 199, 128, 219, 398, 534,
        83, 471, 719, 33, 474, 328, 480, 666, 261, 628, 200, 698, 0, 192, 452, 218, 245, 167, 582, 238, 414, 690, 586,
        131, 476, 731, 415, 495, 496, 244, 500, 327, 126, 275, 497, 177, 609, 360, 722, 736, 139, 169, 297, 431, 687,
        751, 331, 472, 669, 760, 271, 212, 544, 683, 382, 292, 510, 240, 158, 103, 613, 543, 520, 755, 34, 549, 367,
        750, 62, 441, 515, 708, 84, 529, 558, 150, 545, 477, 86, 400, 82, 362, 202, 531, 115, 217, 337, 227, 282, 43,
        418, 624, 284, 386, 591, 565, 483, 101, 273, 276, 257, 153, 2, 270, 663, 608, 519, 445, 185, 25, 296, 6, 52,
        373, 127, 577, 145, 57, 576, 503, 123, 457, 397, 393, 555, 455, 221, 258, 141, 299, 274, 313, 114, 14, 368, 380,
        391, 606, 253, 575, 568, 409, 142, 75, 116, 596, 359, 527, 571, 89, 36, 30, 625, 623, 118, 140, 358, 148, 468,
        467, 207, 499, 658, 538, 487, 611, 338, 518, 302, 401, 541, 454, 694, 317, 682, 589, 251, 647, 305, 180, 387,
        490, 110, 726, 466, 641, 214, 656, 94, 643, 190, 644, 20, 424, 42, 425, 259, 652, 194, 426, 522, 671, 478, 428,
        509, 612, 632, 763, 584, 450, 241, 163, 182, 479, 563, 237, 715, 523, 152, 675, 345, 96, 21, 660, 498, 580, 460,
        416, 615, 53, 511, 76, 677, 351, 429, 494, 19, 65, 614, 627, 569, 662, 178, 713, 648, 730, 716, 735, 714, 333,
        727, 718, 486, 679, 724, 58, 3, 475, 316, 443, 695, 403, 732, 361, 513, 225, 737, 502, 533, 560, 729, 44, 342,
        743, 26, 723, 465, 12, 113, 350, 505, 406, 757, 189, 672, 179, 753, 332, 721, 759, 449, 239, 617, 384, 489, 364,
        134, 242,
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