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
    constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 639, 621, 348, 721, 67, 650, 93, 407, 207, 666, 435,
        571, 484, 558, 352, 660, 335, 751, 658, 339, 720, 695, 23, 604, 439, 662, 652, 233, 661, 640, 256, 712, 314,
        516, 726, 271, 303, 648, 616, 766, 606, 682, 738, 531, 107, 541, 226, 239, 579, 73, 108, 11, 419, 39, 437, 683,
        88, 9, 52, 130, 160, 270, 644, 60, 733, 603, 92, 50, 288, 592, 6, 529, 533, 2, 34, 131, 168, 79, 685, 578, 739,
        667, 634, 412, 655, 269, 182, 729, 193, 623, 392, 133, 763, 166, 480, 462, 35, 127, 569, 574, 401, 475, 218,
        196, 716, 744, 58, 377, 625, 600, 749, 691, 710, 527, 45, 632, 362, 170, 481, 510, 506, 601, 134, 673, 236, 28,
        585, 192, 3, 759, 698, 121, 84, 724, 169, 514, 554, 319, 562, 68, 508, 171, 473, 503, 588, 289, 86, 1, 342, 268,
        331, 248, 725, 51, 119, 765, 430, 275, 148, 244, 293, 176, 659, 157, 373, 560, 379, 338, 518, 187, 704, 610,
        495, 748, 20, 396, 677, 247, 178, 364, 380, 708, 651, 209, 522, 635, 487, 584, 330, 208, 440, 296, 453, 10, 700,
        228, 649, 18, 593, 443, 359, 341, 198, 116, 99, 204, 614, 688, 711, 122, 44, 482, 460, 161, 386, 40, 507, 365,
        241, 89, 637, 678, 260, 750, 382, 498, 63, 638, 153, 590, 70, 22, 512, 645, 232, 197, 434, 66, 707, 59, 542,
        410, 334, 13, 642, 37, 432, 152, 17, 548, 643, 421, 94, 137, 321, 158, 253, 291, 580, 191, 450, 371, 258, 64,
        264, 38, 581, 754, 71, 147, 565, 492, 418, 87, 752, 741, 32, 344, 267, 399, 329, 308, 452, 263, 426, 8, 456,
        753, 280, 676, 513, 101, 618, 702, 294, 447, 586, 251, 151, 324, 387, 491, 611, 486, 80, 669, 300, 746, 173,
        566, 459, 727, 696, 177, 730, 138, 266, 164, 307, 767, 150, 190, 657, 398, 219, 33, 203, 298, 479, 46, 731, 693,
        120, 206, 144, 322, 100, 224, 598, 464, 411, 340, 740, 83, 57, 674, 259, 559, 328, 257, 12, 205, 85, 128, 325,
        142, 349, 15, 416, 415, 117, 406, 246, 367, 114, 72, 220, 4, 286, 353, 351, 189, 474, 555, 227, 345, 520, 354,
        576, 570, 567, 357, 278, 397, 65, 320, 423, 355, 179, 624, 607, 496, 124, 210, 212, 526, 140, 653, 718, 299,
        488, 656, 41, 327, 361, 404, 563, 254, 557, 143, 350, 274, 317, 125, 368, 75, 185, 139, 400, 461, 476, 403, 755,
        214, 455, 167, 337, 628, 549, 304, 0, 97, 409, 433, 538, 249, 358, 745, 706, 27, 428, 468, 442, 175, 429, 602,
        297, 734, 229, 159, 284, 279, 188, 394, 420, 735, 323, 448, 5, 502, 670, 91, 155, 194, 756, 310, 346, 465, 202,
        255, 180, 664, 463, 723, 561, 631, 78, 312, 546, 102, 172, 265, 181, 283, 145, 21, 375, 305, 287, 49, 454, 19,
        467, 599, 494, 245, 105, 668, 234, 451, 295, 282, 493, 14, 589, 500, 681, 309, 511, 665, 490, 273, 417, 276,
        438, 515, 225, 82, 343, 646, 699, 272, 489, 446, 237, 427, 509, 672, 195, 537, 154, 762, 390, 449, 540, 535,
        536, 222, 424, 619, 499, 115, 95, 483, 505, 7, 347, 532, 383, 686, 550, 547, 539, 545, 363, 376, 74, 61, 497,
        466, 184, 736, 758, 425, 445, 281, 521, 216, 501, 408, 471, 609, 687, 388, 109, 575, 697, 262, 56, 552, 199, 36,
        374, 223, 332, 90, 372, 250, 551, 252, 663, 457, 544, 405, 594, 211, 69, 235, 477, 597, 48, 582, 135, 165, 556,
        523, 215, 302, 422, 47, 77, 301, 113, 587, 689, 612, 627, 104, 240, 732, 395, 326, 622, 336, 106, 129, 591, 313,
        393, 81, 54, 553, 573, 633, 444, 141, 402, 311, 519, 221, 384, 641, 231, 360, 472, 369, 243, 543, 760, 24, 629,
        692, 764, 630, 261, 378, 26, 620, 504, 595, 132, 146, 431, 111, 306, 615, 470, 564, 149, 156, 722, 315, 333,
        201, 217, 389, 42, 485, 118, 605, 385, 469, 316, 110, 292, 96, 16, 761, 617, 123, 680, 290, 719, 626, 737, 103,
        183, 163, 705, 534, 436, 413, 694, 703, 31, 43, 126, 356, 596, 76, 30, 200, 671, 713, 714, 647, 98, 675, 717,
        136, 458, 62, 186, 613, 391, 715, 277, 162, 525, 366, 53, 112, 370, 583, 757, 709, 381, 636, 572, 55, 25, 684,
        742, 701, 690, 524, 743, 213, 747, 679, 528, 530, 318, 238, 608, 29, 230, 577, 568, 242, 285, 441, 414, 728,
        517, 654, 174, 478,
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