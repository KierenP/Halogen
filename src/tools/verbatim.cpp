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
    constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 111, 716, 337, 342, 193, 138, 119, 237, 709, 216, 66,
        487, 241, 192, 49, 747, 376, 453, 261, 15, 706, 481, 498, 154, 586, 373, 460, 751, 658, 151, 745, 304, 757, 713,
        2, 702, 601, 648, 554, 511, 30, 496, 450, 380, 458, 395, 361, 248, 225, 480, 755, 687, 143, 53, 302, 56, 78,
        661, 639, 467, 54, 317, 551, 340, 170, 191, 300, 383, 42, 127, 442, 293, 208, 619, 670, 135, 659, 38, 647, 499,
        86, 573, 112, 396, 705, 510, 750, 724, 767, 8, 20, 91, 746, 513, 77, 505, 521, 122, 333, 690, 287, 681, 472,
        740, 131, 344, 721, 655, 378, 356, 28, 762, 620, 294, 679, 70, 479, 194, 359, 737, 83, 488, 172, 692, 16, 21,
        527, 742, 597, 576, 764, 577, 529, 615, 257, 218, 320, 298, 765, 61, 403, 364, 729, 238, 13, 24, 55, 753, 29,
        239, 414, 336, 354, 99, 605, 463, 133, 81, 60, 628, 290, 526, 162, 696, 331, 602, 346, 145, 32, 662, 446, 711,
        341, 439, 187, 415, 441, 156, 219, 178, 106, 388, 351, 626, 444, 520, 500, 365, 199, 161, 62, 163, 150, 7, 533,
        98, 455, 132, 600, 286, 319, 104, 4, 59, 64, 685, 109, 370, 153, 265, 0, 736, 634, 512, 165, 635, 89, 471, 417,
        537, 92, 173, 550, 326, 296, 731, 190, 184, 144, 570, 31, 174, 332, 588, 169, 180, 561, 189, 738, 509, 250, 37,
        232, 486, 186, 220, 598, 617, 589, 242, 48, 46, 704, 353, 760, 14, 497, 159, 107, 3, 624, 233, 6, 222, 541, 276,
        563, 445, 528, 433, 413, 58, 470, 40, 398, 642, 532, 457, 65, 328, 436, 431, 636, 282, 609, 234, 366, 197, 397,
        303, 33, 63, 676, 121, 203, 236, 256, 722, 761, 314, 228, 93, 416, 569, 493, 428, 182, 476, 23, 231, 85, 244,
        308, 157, 410, 292, 137, 269, 295, 330, 26, 321, 717, 226, 584, 47, 316, 491, 689, 113, 285, 179, 466, 564, 289,
        297, 674, 538, 215, 375, 213, 158, 733, 682, 309, 129, 88, 347, 688, 198, 260, 585, 275, 71, 212, 522, 739, 633,
        245, 490, 101, 348, 224, 139, 322, 57, 438, 540, 631, 84, 311, 535, 117, 160, 369, 372, 368, 268, 69, 100, 214,
        360, 382, 221, 579, 732, 411, 707, 278, 229, 126, 274, 271, 644, 763, 473, 324, 456, 235, 652, 374, 393, 176,
        402, 459, 677, 595, 700, 749, 409, 352, 741, 17, 525, 19, 478, 35, 10, 291, 195, 663, 543, 544, 582, 539, 494,
        385, 422, 217, 686, 313, 103, 367, 572, 401, 614, 5, 95, 714, 730, 255, 482, 389, 243, 74, 434, 259, 288, 437,
        230, 440, 451, 205, 301, 611, 384, 580, 310, 432, 147, 400, 136, 124, 608, 683, 581, 734, 279, 11, 392, 204,
        462, 429, 419, 166, 452, 406, 339, 391, 719, 464, 240, 82, 116, 484, 39, 485, 152, 318, 461, 435, 148, 299, 558,
        725, 134, 264, 549, 474, 114, 518, 146, 502, 327, 110, 67, 120, 277, 18, 613, 559, 727, 227, 45, 531, 508, 425,
        312, 175, 427, 698, 168, 516, 125, 447, 141, 338, 334, 468, 542, 246, 362, 501, 262, 534, 386, 108, 345, 477,
        667, 695, 664, 171, 621, 678, 701, 183, 96, 36, 495, 640, 253, 350, 75, 196, 325, 355, 643, 390, 281, 530, 335,
        691, 627, 22, 728, 555, 547, 251, 607, 404, 273, 249, 720, 379, 557, 503, 43, 567, 506, 329, 653, 578, 79, 280,
        645, 270, 562, 616, 97, 590, 449, 258, 548, 349, 507, 267, 594, 583, 552, 723, 177, 284, 307, 426, 649, 41, 206,
        305, 454, 610, 181, 545, 210, 51, 381, 424, 556, 123, 504, 759, 660, 80, 469, 405, 266, 637, 587, 650, 149, 363,
        591, 646, 524, 766, 90, 694, 72, 430, 560, 200, 188, 592, 604, 76, 27, 632, 553, 618, 735, 87, 475, 52, 223,
        155, 371, 657, 629, 568, 263, 625, 185, 654, 641, 50, 443, 593, 612, 394, 665, 1, 343, 34, 666, 167, 140, 412,
        712, 492, 44, 680, 357, 574, 630, 684, 102, 673, 421, 656, 483, 596, 758, 571, 651, 423, 9, 699, 418, 399, 209,
        718, 693, 669, 115, 566, 672, 748, 519, 517, 622, 546, 515, 283, 358, 603, 715, 710, 703, 73, 130, 94, 408, 708,
        252, 118, 756, 726, 306, 315, 668, 254, 377, 211, 638, 536, 743, 164, 575, 565, 387, 201, 407, 697, 675, 623,
        142, 105, 448, 465, 128, 272, 68, 752, 514, 202, 207, 523, 247, 744, 671, 489, 323, 754, 25, 599, 12, 606, 420,
    };

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