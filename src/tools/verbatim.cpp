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
    constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 182, 249, 130, 631, 149, 524, 159, 320, 345, 475, 355,
        664, 655, 454, 471, 734, 76, 465, 18, 678, 238, 445, 729, 713, 629, 256, 212, 411, 64, 105, 179, 120, 55, 726,
        753, 648, 576, 201, 96, 264, 44, 588, 16, 22, 414, 693, 430, 290, 239, 214, 186, 665, 26, 147, 327, 521, 537,
        720, 168, 696, 108, 689, 635, 644, 642, 389, 378, 446, 660, 567, 253, 495, 596, 750, 437, 165, 354, 456, 434,
        526, 502, 483, 555, 550, 20, 764, 574, 686, 499, 208, 87, 450, 351, 359, 606, 137, 663, 287, 757, 363, 711, 148,
        85, 74, 367, 268, 296, 35, 435, 206, 78, 386, 614, 668, 477, 621, 407, 115, 649, 104, 8, 740, 252, 461, 547,
        546, 390, 602, 103, 593, 690, 440, 129, 13, 704, 506, 38, 613, 759, 172, 333, 210, 396, 564, 350, 575, 615, 497,
        416, 752, 393, 151, 444, 372, 163, 530, 177, 428, 118, 474, 43, 295, 418, 246, 12, 180, 229, 88, 388, 362, 176,
        758, 262, 174, 173, 724, 691, 570, 737, 128, 566, 332, 68, 438, 581, 131, 185, 260, 646, 630, 688, 241, 657,
        487, 468, 184, 285, 658, 549, 610, 494, 187, 202, 45, 221, 204, 478, 733, 132, 482, 562, 552, 279, 199, 171, 82,
        731, 283, 312, 218, 763, 310, 473, 761, 703, 448, 145, 745, 191, 11, 485, 67, 161, 33, 457, 97, 476, 17, 223,
        127, 674, 634, 27, 402, 377, 94, 123, 716, 637, 509, 203, 247, 40, 48, 133, 217, 86, 300, 211, 122, 638, 274,
        722, 479, 152, 358, 568, 609, 36, 723, 571, 654, 190, 237, 671, 23, 421, 587, 535, 306, 289, 19, 261, 395, 236,
        585, 220, 276, 417, 244, 627, 106, 263, 219, 284, 175, 765, 7, 719, 492, 679, 680, 561, 548, 511, 659, 336, 2,
        741, 432, 32, 278, 746, 394, 486, 314, 577, 682, 258, 727, 458, 298, 225, 579, 321, 121, 313, 167, 429, 25, 709,
        66, 301, 170, 267, 334, 544, 101, 107, 645, 70, 79, 656, 340, 315, 308, 626, 235, 501, 37, 140, 607, 650, 337,
        100, 330, 302, 265, 508, 207, 449, 500, 361, 250, 675, 347, 728, 616, 739, 119, 714, 58, 51, 766, 178, 102, 364,
        481, 72, 651, 6, 504, 717, 56, 419, 532, 338, 489, 81, 519, 28, 259, 24, 467, 569, 541, 5, 380, 183, 375, 126,
        282, 653, 426, 715, 293, 643, 357, 343, 513, 410, 63, 412, 269, 620, 447, 139, 573, 194, 150, 352, 672, 415,
        496, 370, 153, 232, 143, 110, 141, 166, 742, 157, 401, 61, 755, 198, 73, 160, 403, 356, 545, 409, 328, 34, 667,
        707, 243, 117, 209, 196, 292, 59, 111, 254, 533, 408, 452, 138, 480, 431, 146, 730, 459, 136, 91, 466, 226, 200,
        92, 404, 230, 41, 277, 9, 156, 319, 181, 708, 455, 154, 617, 692, 124, 652, 275, 738, 222, 31, 514, 470, 192,
        684, 510, 749, 164, 443, 743, 666, 353, 384, 366, 490, 98, 677, 453, 373, 113, 71, 125, 399, 233, 54, 383, 597,
        439, 515, 3, 423, 706, 406, 420, 525, 540, 507, 62, 114, 612, 424, 341, 99, 702, 600, 484, 65, 272, 376, 46,
        462, 531, 197, 29, 441, 469, 323, 505, 539, 517, 144, 498, 304, 551, 512, 538, 360, 231, 240, 95, 335, 245, 369,
        50, 605, 109, 400, 326, 687, 594, 542, 633, 518, 559, 322, 205, 365, 536, 234, 255, 557, 381, 134, 619, 344, 47,
        647, 560, 257, 565, 683, 762, 491, 591, 427, 556, 189, 493, 463, 608, 554, 598, 4, 80, 195, 464, 325, 346, 382,
        89, 611, 601, 725, 348, 216, 718, 425, 155, 224, 281, 227, 735, 553, 112, 527, 273, 193, 622, 309, 754, 628, 15,
        228, 580, 685, 732, 14, 251, 520, 188, 640, 604, 624, 42, 297, 291, 583, 767, 488, 736, 582, 294, 311, 662, 578,
        413, 625, 213, 342, 379, 543, 52, 669, 303, 1, 436, 21, 158, 324, 558, 305, 586, 756, 339, 215, 522, 135, 460,
        751, 584, 442, 391, 636, 142, 760, 286, 572, 700, 368, 673, 705, 270, 676, 698, 331, 318, 694, 307, 60, 697,
        288, 53, 701, 169, 77, 503, 392, 248, 632, 398, 57, 39, 374, 748, 712, 599, 595, 516, 405, 747, 618, 710, 589,
        266, 271, 433, 721, 0, 529, 590, 162, 472, 661, 49, 69, 84, 670, 10, 528, 93, 563, 695, 242, 116, 623, 534, 90,
        387, 349, 641, 639, 744, 523, 371, 317, 681, 316, 83, 299, 699, 451, 385, 422, 397, 603, 30, 329, 592, 280, 75,
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