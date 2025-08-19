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
    constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 404, 383, 704, 281, 707, 511, 251, 693, 136, 730, 416,
        457, 754, 117, 645, 629, 159, 635, 459, 527, 558, 436, 748, 741, 409, 186, 622, 344, 22, 153, 137, 719, 230,
        641, 594, 482, 699, 760, 708, 520, 549, 488, 464, 326, 726, 166, 76, 521, 357, 413, 762, 125, 221, 746, 75, 339,
        274, 66, 716, 87, 628, 447, 454, 302, 631, 212, 362, 124, 688, 696, 576, 615, 524, 220, 465, 440, 644, 33, 689,
        21, 492, 54, 260, 41, 296, 584, 205, 389, 53, 259, 751, 745, 572, 466, 43, 600, 670, 98, 7, 350, 652, 250, 747,
        545, 567, 700, 682, 547, 540, 753, 307, 387, 582, 20, 169, 364, 637, 690, 24, 662, 217, 613, 12, 531, 417, 740,
        471, 360, 214, 254, 376, 277, 337, 218, 120, 40, 723, 210, 77, 25, 160, 486, 535, 15, 19, 528, 370, 49, 47, 709,
        633, 727, 539, 35, 91, 69, 554, 462, 11, 617, 650, 429, 108, 392, 368, 1, 550, 177, 46, 290, 152, 420, 173, 590,
        541, 155, 425, 548, 126, 592, 203, 632, 563, 461, 512, 348, 473, 127, 72, 607, 403, 697, 202, 763, 185, 378,
        279, 455, 345, 199, 352, 349, 263, 200, 62, 188, 289, 529, 207, 638, 248, 764, 514, 518, 239, 449, 100, 3, 562,
        737, 343, 553, 103, 479, 715, 694, 579, 625, 215, 570, 583, 476, 566, 31, 228, 233, 306, 406, 96, 236, 564, 163,
        225, 375, 18, 168, 162, 395, 213, 130, 710, 450, 439, 252, 744, 13, 640, 275, 211, 292, 442, 121, 232, 721, 634,
        463, 78, 458, 63, 446, 501, 509, 578, 552, 367, 265, 242, 493, 351, 86, 728, 161, 284, 393, 81, 489, 766, 668,
        734, 280, 197, 359, 107, 264, 183, 286, 324, 285, 73, 245, 374, 705, 88, 288, 27, 135, 533, 593, 297, 676, 678,
        311, 209, 95, 695, 243, 342, 504, 341, 170, 261, 67, 448, 145, 490, 65, 544, 334, 485, 619, 412, 123, 598, 706,
        669, 468, 391, 5, 94, 316, 648, 729, 181, 0, 45, 336, 314, 190, 29, 443, 508, 555, 571, 55, 522, 79, 581, 309,
        519, 249, 506, 642, 133, 229, 247, 390, 523, 742, 328, 621, 507, 304, 630, 586, 171, 322, 667, 713, 610, 142,
        725, 759, 283, 101, 559, 758, 305, 256, 114, 146, 388, 216, 301, 353, 720, 675, 157, 99, 609, 399, 227, 382,
        599, 365, 451, 561, 596, 90, 118, 542, 408, 165, 44, 74, 266, 415, 691, 497, 414, 396, 226, 122, 702, 530, 665,
        606, 57, 427, 340, 517, 422, 574, 131, 434, 623, 237, 313, 37, 437, 113, 672, 656, 141, 310, 104, 401, 115, 134,
        484, 140, 603, 180, 500, 418, 276, 453, 71, 386, 335, 58, 683, 299, 51, 8, 244, 293, 430, 208, 4, 315, 84, 626,
        222, 167, 460, 585, 475, 110, 565, 223, 660, 294, 102, 330, 575, 347, 338, 757, 643, 397, 661, 671, 556, 483,
        191, 151, 495, 423, 377, 532, 761, 496, 444, 128, 732, 755, 536, 172, 491, 421, 358, 494, 196, 147, 17, 398,
        201, 756, 743, 269, 381, 42, 28, 273, 174, 26, 319, 366, 410, 97, 636, 474, 379, 445, 257, 111, 722, 505, 627,
        60, 300, 438, 534, 411, 369, 537, 546, 14, 480, 262, 52, 503, 291, 64, 478, 193, 591, 543, 231, 295, 318, 481,
        371, 144, 499, 477, 698, 267, 331, 56, 516, 246, 405, 731, 194, 724, 703, 154, 325, 182, 10, 573, 686, 195, 601,
        526, 327, 587, 749, 602, 16, 424, 234, 268, 431, 106, 36, 32, 156, 674, 85, 580, 83, 59, 317, 456, 752, 235,
        308, 139, 187, 608, 680, 538, 238, 510, 361, 551, 435, 178, 569, 129, 258, 467, 373, 620, 189, 502, 270, 614,
        82, 241, 663, 646, 589, 354, 717, 112, 655, 192, 604, 30, 224, 472, 616, 272, 255, 647, 525, 68, 560, 739, 148,
        653, 432, 651, 50, 611, 105, 498, 428, 372, 711, 332, 441, 198, 666, 588, 684, 639, 577, 48, 9, 659, 597, 278,
        714, 452, 487, 240, 346, 34, 132, 595, 321, 92, 329, 687, 149, 253, 612, 116, 767, 426, 323, 649, 685, 673, 654,
        419, 312, 657, 271, 109, 679, 624, 6, 384, 282, 150, 355, 303, 750, 204, 363, 738, 333, 664, 718, 356, 681, 23,
        184, 470, 568, 143, 2, 80, 400, 733, 735, 175, 433, 93, 176, 515, 658, 736, 70, 39, 119, 701, 677, 402, 380,
        385, 513, 407, 298, 164, 605, 712, 692, 138, 179, 287, 469, 394, 206, 320, 89, 219, 61, 618, 557, 38, 158, 765,
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

auto pad_l1_weight_int32(const decltype(raw_network::l1_weight)& input)
{
    auto output = std::make_unique<std::array<std::array<std::array<int32_t, L1_SIZE>, FT_SIZE>, OUTPUT_BUCKETS>>();

    for (size_t i = 0; i < OUTPUT_BUCKETS; i++)
    {
        for (size_t j = 0; j < FT_SIZE; j++)
        {
            for (size_t k = 0; k < L1_SIZE; k++)
            {
                int8_t padded[4] = { static_cast<int8_t>(input[i][k][j]), 0, 0, 0 };
                (*output)[i][j][k] = *reinterpret_cast<int32_t*>(&padded);
            }
        }
    }

    return output;
}
}

#if defined(USE_AVX512_VNNI)

struct network
{
    alignas(64) std::array<std::array<int16_t, FT_SIZE>, INPUT_SIZE * KING_BUCKET_COUNT> ft_weight = {};
    alignas(64) std::array<int16_t, FT_SIZE> ft_bias = {};
    alignas(64) std::array<std::array<std::array<int32_t, L1_SIZE>, FT_SIZE>, OUTPUT_BUCKETS> l1_weight = {};
    alignas(64) std::array<std::array<int32_t, L1_SIZE>, OUTPUT_BUCKETS> l1_bias = {};
    alignas(64) std::array<std::array<std::array<float, L2_SIZE>, L1_SIZE * 2>, OUTPUT_BUCKETS> l2_weight = {};
    alignas(64) std::array<std::array<float, L2_SIZE>, OUTPUT_BUCKETS> l2_bias = {};
    alignas(64) std::array<std::array<float, L2_SIZE>, OUTPUT_BUCKETS> l3_weight = {};
    alignas(64) std::array<float, OUTPUT_BUCKETS> l3_bias = {};
};

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

    auto [ft_weight1, ft_bias1] = adjust_for_packus(raw_net->ft_weight, raw_net->ft_bias);
    auto final_net = std::make_unique<network>();
    final_net->ft_weight = *ft_weight1;
    final_net->ft_bias = *ft_bias1;
    final_net->l1_weight = *pad_l1_weight_int32(raw_net->l1_weight);
    final_net->l1_bias = *cast_l1_bias_int32(*rescale_l1_bias(raw_net->l1_bias));
    final_net->l2_weight = *transpose_l2_weights(raw_net->l2_weight);
    final_net->l2_bias = raw_net->l2_bias;
    final_net->l3_weight = raw_net->l3_weight;
    final_net->l3_bias = raw_net->l3_bias;

    std::ofstream out(argv[2], std::ios::binary);
    out.write(reinterpret_cast<const char*>(final_net.get()), sizeof(network));

    std::cout << "Created embedded network" << std::endl;
}
#else
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
#endif