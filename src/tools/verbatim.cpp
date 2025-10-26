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
    constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 554, 499, 519, 578, 39, 732, 337, 738, 254, 296, 766,
        471, 731, 582, 712, 36, 250, 625, 7, 198, 223, 535, 276, 426, 118, 407, 224, 695, 338, 586, 334, 642, 734, 387,
        687, 714, 649, 596, 708, 532, 150, 205, 260, 585, 246, 93, 671, 191, 190, 92, 700, 621, 660, 703, 709, 686, 317,
        681, 490, 388, 673, 652, 333, 466, 654, 608, 715, 11, 1, 569, 688, 594, 33, 72, 420, 564, 28, 504, 512, 665,
        754, 513, 527, 29, 303, 399, 154, 8, 287, 220, 717, 656, 751, 339, 322, 99, 597, 461, 329, 592, 274, 176, 386,
        196, 187, 727, 767, 108, 32, 312, 488, 77, 275, 570, 359, 371, 212, 430, 107, 54, 630, 45, 563, 305, 146, 193,
        556, 272, 180, 702, 725, 211, 375, 636, 497, 67, 607, 428, 185, 406, 494, 70, 722, 252, 711, 378, 101, 80, 601,
        658, 716, 60, 324, 680, 69, 736, 482, 352, 155, 351, 134, 325, 396, 330, 56, 616, 311, 241, 632, 546, 323, 664,
        423, 704, 139, 121, 645, 76, 689, 613, 200, 429, 348, 85, 749, 744, 151, 486, 341, 271, 245, 414, 277, 167, 202,
        391, 381, 289, 194, 655, 268, 746, 675, 713, 619, 168, 206, 103, 480, 411, 84, 623, 10, 404, 201, 752, 292, 248,
        403, 131, 281, 485, 627, 142, 581, 760, 225, 443, 336, 579, 551, 743, 385, 332, 256, 184, 503, 20, 739, 214,
        600, 507, 438, 293, 548, 662, 105, 62, 231, 74, 726, 251, 362, 136, 583, 3, 517, 344, 449, 259, 384, 595, 458,
        149, 236, 321, 68, 239, 453, 419, 182, 683, 95, 75, 455, 448, 48, 400, 591, 379, 575, 170, 129, 97, 728, 315,
        742, 286, 651, 127, 238, 290, 35, 218, 679, 508, 183, 476, 227, 498, 17, 110, 240, 705, 678, 264, 366, 637, 255,
        192, 374, 140, 304, 66, 314, 685, 53, 472, 376, 737, 620, 144, 327, 684, 294, 541, 232, 90, 295, 152, 735, 64,
        647, 335, 57, 243, 137, 234, 61, 285, 459, 502, 755, 81, 410, 650, 284, 44, 350, 357, 158, 710, 265, 197, 5,
        355, 130, 733, 543, 96, 360, 719, 464, 22, 230, 365, 415, 367, 31, 456, 181, 37, 750, 452, 501, 475, 618, 545,
        297, 102, 186, 559, 9, 86, 316, 661, 691, 538, 94, 278, 747, 162, 4, 674, 394, 552, 135, 148, 465, 758, 638,
        648, 15, 328, 484, 401, 402, 540, 301, 364, 748, 319, 73, 398, 178, 47, 133, 417, 418, 635, 493, 13, 43, 634,
        249, 663, 370, 424, 343, 643, 439, 279, 431, 244, 156, 435, 718, 177, 666, 41, 369, 161, 442, 740, 444, 462,
        395, 372, 515, 320, 495, 233, 128, 253, 441, 446, 567, 457, 560, 119, 141, 88, 210, 422, 505, 573, 310, 460,
        247, 216, 536, 262, 539, 393, 550, 235, 126, 511, 282, 172, 478, 356, 55, 208, 27, 89, 380, 481, 358, 509, 514,
        112, 762, 433, 487, 65, 163, 308, 153, 174, 171, 470, 195, 450, 753, 115, 506, 730, 542, 0, 143, 26, 544, 302,
        593, 724, 91, 549, 18, 273, 445, 347, 124, 326, 598, 24, 349, 492, 528, 496, 425, 531, 189, 588, 390, 534, 510,
        606, 46, 132, 533, 547, 698, 525, 179, 697, 346, 270, 690, 175, 283, 377, 516, 633, 759, 373, 557, 555, 479,
        291, 58, 761, 427, 40, 474, 164, 383, 71, 405, 723, 209, 219, 640, 562, 436, 306, 447, 463, 694, 659, 229, 421,
        617, 615, 561, 413, 529, 342, 147, 412, 526, 204, 389, 34, 353, 518, 228, 267, 288, 558, 21, 221, 299, 113, 604,
        605, 408, 409, 392, 257, 300, 576, 467, 14, 741, 603, 188, 568, 87, 207, 566, 226, 104, 580, 701, 50, 138, 500,
        440, 657, 2, 345, 98, 203, 553, 473, 106, 756, 721, 639, 537, 628, 624, 49, 644, 454, 363, 646, 6, 720, 354,
        641, 523, 610, 469, 199, 109, 258, 693, 242, 602, 577, 477, 298, 611, 125, 169, 82, 489, 263, 669, 59, 763, 672,
        524, 522, 629, 25, 159, 116, 706, 78, 682, 340, 309, 79, 587, 165, 677, 574, 120, 166, 589, 692, 530, 434, 696,
        382, 83, 572, 145, 676, 437, 432, 222, 584, 670, 631, 707, 699, 23, 280, 237, 261, 667, 266, 416, 42, 215, 361,
        12, 397, 483, 599, 622, 122, 521, 157, 63, 19, 451, 117, 590, 269, 100, 653, 16, 213, 123, 626, 52, 491, 612,
        318, 307, 745, 51, 160, 38, 609, 217, 729, 571, 313, 668, 331, 468, 173, 520, 30, 565, 765, 757, 614, 764, 368,
        114, 111,
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