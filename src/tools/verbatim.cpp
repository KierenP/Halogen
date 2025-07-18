// Load the network file, do some preprocessing, and then save it to the build directory for inclusion in the final
// binary

#include "network/arch.hpp"
#include "network/simd/definitions.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>

using namespace NN;

struct raw_network
{
    alignas(64) std::array<std::array<int16_t, FT_SIZE>, INPUT_SIZE * KING_BUCKET_COUNT> ft_weight = {};
    alignas(64) std::array<int16_t, FT_SIZE> ft_bias = {};
    alignas(64) std::array<std::array<std::array<int16_t, FT_SIZE>, L1_SIZE>, OUTPUT_BUCKETS> l1_weight = {};
    alignas(64) std::array<std::array<int16_t, L1_SIZE>, OUTPUT_BUCKETS> l1_bias = {};
    alignas(64) std::array<std::array<std::array<float, L1_SIZE>, L2_SIZE>, OUTPUT_BUCKETS> l2_weight = {};
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
    constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 728, 643, 153, 260, 166, 51, 307, 467, 393, 82, 109,
        40, 697, 428, 257, 705, 362, 75, 187, 306, 547, 103, 593, 157, 290, 506, 626, 675, 169, 511, 314, 494, 763, 319,
        93, 709, 295, 132, 2, 39, 173, 484, 325, 228, 563, 516, 71, 750, 253, 130, 549, 385, 22, 107, 558, 663, 375,
        206, 30, 283, 569, 703, 744, 689, 178, 733, 65, 721, 496, 201, 218, 459, 28, 537, 386, 379, 736, 578, 532, 655,
        507, 722, 657, 243, 112, 0, 86, 463, 719, 554, 411, 381, 21, 129, 761, 630, 418, 328, 488, 324, 152, 85, 376,
        320, 289, 452, 220, 255, 108, 629, 597, 363, 529, 47, 559, 224, 291, 240, 672, 352, 460, 110, 766, 239, 334,
        679, 746, 615, 740, 18, 767, 524, 548, 738, 92, 135, 330, 66, 605, 139, 351, 197, 465, 610, 122, 403, 33, 10,
        176, 77, 634, 489, 636, 205, 195, 520, 392, 111, 592, 79, 490, 158, 170, 172, 214, 505, 143, 123, 618, 365, 658,
        556, 148, 134, 441, 635, 764, 128, 557, 525, 707, 469, 456, 159, 177, 204, 694, 76, 726, 753, 254, 25, 344, 560,
        419, 394, 542, 19, 49, 417, 677, 654, 3, 699, 604, 175, 691, 444, 356, 179, 50, 84, 354, 631, 190, 54, 238, 737,
        350, 162, 280, 535, 222, 207, 662, 497, 686, 762, 739, 6, 193, 399, 366, 667, 304, 342, 73, 145, 114, 695, 258,
        234, 274, 235, 269, 37, 32, 526, 186, 388, 154, 133, 692, 97, 409, 168, 735, 665, 7, 249, 42, 311, 237, 24, 264,
        53, 267, 31, 242, 17, 215, 682, 151, 729, 202, 620, 704, 503, 650, 191, 495, 292, 70, 94, 282, 155, 285, 760,
        619, 492, 765, 457, 447, 95, 572, 188, 296, 755, 348, 9, 644, 518, 245, 715, 607, 582, 550, 55, 273, 27, 203,
        62, 415, 232, 723, 751, 300, 706, 343, 8, 100, 335, 268, 321, 422, 248, 530, 208, 58, 136, 638, 106, 174, 690,
        332, 81, 288, 60, 256, 226, 700, 278, 590, 678, 478, 345, 56, 437, 536, 38, 632, 373, 120, 45, 121, 227, 405,
        414, 329, 416, 669, 562, 566, 361, 543, 390, 150, 156, 370, 702, 293, 115, 141, 427, 449, 44, 217, 458, 357,
        596, 315, 603, 323, 383, 468, 688, 396, 276, 262, 146, 371, 305, 473, 741, 528, 241, 546, 367, 579, 219, 485,
        96, 160, 364, 64, 622, 124, 464, 434, 266, 59, 270, 693, 263, 730, 286, 52, 127, 696, 104, 369, 229, 349, 423,
        113, 74, 611, 16, 594, 303, 250, 230, 327, 90, 432, 749, 439, 519, 683, 338, 710, 317, 430, 584, 479, 598, 142,
        426, 407, 358, 438, 628, 541, 656, 72, 309, 435, 513, 681, 639, 589, 137, 298, 646, 272, 244, 674, 680, 225,
        184, 711, 471, 48, 167, 102, 759, 302, 80, 450, 408, 246, 171, 482, 164, 745, 125, 720, 487, 470, 540, 472, 337,
        265, 13, 140, 180, 281, 713, 433, 671, 185, 499, 163, 118, 413, 299, 402, 453, 161, 440, 502, 448, 41, 138, 583,
        116, 355, 455, 333, 570, 397, 653, 99, 523, 221, 522, 510, 545, 368, 46, 391, 194, 88, 147, 534, 483, 512, 382,
        26, 474, 673, 633, 591, 725, 279, 213, 500, 284, 287, 477, 717, 404, 412, 553, 63, 552, 555, 346, 701, 297, 504,
        687, 446, 359, 651, 1, 727, 308, 565, 602, 23, 294, 377, 119, 573, 664, 216, 475, 387, 398, 189, 91, 277, 247,
        531, 748, 271, 67, 462, 339, 400, 587, 743, 410, 401, 436, 331, 316, 574, 196, 144, 601, 625, 251, 68, 732, 539,
        5, 183, 149, 431, 461, 182, 199, 714, 491, 538, 617, 613, 754, 117, 571, 581, 608, 57, 211, 514, 212, 466, 15,
        508, 595, 481, 752, 384, 223, 544, 637, 380, 252, 443, 758, 61, 476, 498, 424, 698, 616, 606, 501, 575, 649,
        341, 353, 580, 567, 645, 360, 421, 659, 389, 642, 98, 89, 275, 612, 666, 442, 600, 318, 313, 614, 670, 561, 515,
        627, 336, 209, 623, 131, 181, 425, 647, 231, 568, 20, 684, 641, 640, 11, 420, 29, 374, 517, 43, 527, 406, 521,
        340, 322, 660, 36, 533, 480, 347, 210, 451, 233, 83, 585, 586, 301, 652, 676, 685, 734, 101, 564, 445, 708, 599,
        69, 454, 326, 429, 588, 712, 493, 724, 200, 378, 624, 310, 261, 747, 236, 576, 12, 4, 648, 577, 105, 742, 192,
        716, 486, 609, 14, 35, 165, 259, 718, 551, 509, 198, 78, 621, 757, 395, 372, 756, 87, 668, 731, 126, 661, 34,
        312,
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
    auto output = std::make_unique<std::array<std::array<std::array<float, L2_SIZE>, L1_SIZE>, OUTPUT_BUCKETS>>();

    for (size_t i = 0; i < OUTPUT_BUCKETS; i++)
    {
        for (size_t j = 0; j < L1_SIZE; j++)
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
    alignas(64) std::array<std::array<std::array<float, L2_SIZE>, L1_SIZE>, OUTPUT_BUCKETS> l2_weight = {};
    alignas(64) std::array<std::array<float, L2_SIZE>, OUTPUT_BUCKETS> l2_bias = {};
    alignas(64) std::array<std::array<float, L2_SIZE>, OUTPUT_BUCKETS> l3_weight = {};
    alignas(64) std::array<float, OUTPUT_BUCKETS> l3_bias = {};
};

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