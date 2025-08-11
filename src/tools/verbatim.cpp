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
    constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 455, 668, 482, 528, 356, 581, 541, 765, 696, 62, 499,
        743, 649, 640, 407, 261, 591, 722, 515, 267, 273, 727, 660, 198, 680, 736, 350, 643, 502, 483, 416, 683, 766,
        759, 575, 450, 519, 137, 481, 567, 752, 39, 162, 606, 409, 754, 192, 642, 513, 533, 196, 150, 127, 561, 542,
        650, 626, 50, 15, 59, 644, 453, 157, 449, 494, 365, 714, 28, 237, 69, 656, 563, 715, 382, 179, 10, 361, 346,
        246, 363, 415, 517, 228, 83, 755, 767, 674, 688, 331, 165, 4, 47, 592, 14, 394, 199, 249, 548, 534, 215, 622,
        355, 152, 676, 107, 570, 45, 489, 611, 103, 292, 248, 504, 682, 36, 569, 154, 605, 169, 757, 296, 684, 280, 16,
        703, 81, 550, 608, 724, 678, 238, 91, 143, 316, 43, 135, 425, 372, 375, 235, 654, 324, 641, 26, 657, 692, 65,
        712, 230, 437, 291, 33, 419, 174, 145, 92, 530, 207, 159, 5, 61, 97, 8, 505, 526, 164, 194, 40, 619, 399, 480,
        380, 53, 236, 212, 507, 559, 396, 94, 393, 646, 182, 443, 183, 512, 243, 131, 71, 599, 9, 148, 474, 607, 628,
        181, 332, 662, 124, 454, 636, 486, 84, 1, 322, 514, 713, 175, 286, 693, 557, 258, 223, 465, 51, 155, 708, 390,
        216, 764, 32, 231, 29, 620, 740, 745, 401, 686, 142, 476, 742, 19, 7, 170, 353, 588, 265, 34, 565, 252, 701,
        539, 469, 600, 242, 289, 128, 227, 444, 562, 587, 244, 655, 348, 438, 133, 49, 578, 263, 104, 601, 500, 687,
        189, 417, 20, 224, 370, 253, 343, 214, 270, 58, 531, 255, 123, 420, 571, 458, 55, 89, 120, 391, 272, 282, 406,
        204, 327, 387, 67, 271, 290, 716, 66, 637, 403, 177, 751, 63, 266, 728, 439, 240, 25, 707, 138, 146, 379, 239,
        269, 326, 167, 576, 446, 521, 268, 3, 293, 400, 761, 478, 658, 158, 60, 726, 86, 321, 171, 325, 295, 516, 490,
        106, 447, 749, 597, 85, 617, 671, 302, 202, 191, 535, 209, 73, 344, 669, 330, 250, 549, 359, 360, 594, 574, 41,
        281, 433, 233, 579, 552, 74, 691, 339, 595, 532, 577, 259, 544, 219, 675, 430, 558, 723, 746, 208, 336, 645,
        317, 78, 383, 12, 414, 616, 378, 217, 134, 624, 536, 30, 717, 389, 247, 11, 160, 408, 464, 300, 452, 98, 76,
        369, 402, 385, 139, 254, 738, 287, 95, 229, 479, 6, 184, 118, 371, 638, 347, 358, 705, 144, 90, 364, 384, 435,
        17, 487, 756, 309, 586, 496, 366, 335, 373, 151, 466, 432, 493, 117, 241, 116, 221, 412, 13, 193, 711, 186, 555,
        38, 413, 315, 623, 431, 589, 2, 166, 445, 22, 719, 303, 195, 111, 685, 653, 68, 75, 334, 197, 395, 670, 82, 222,
        277, 206, 442, 733, 173, 294, 119, 87, 472, 470, 284, 392, 677, 448, 511, 501, 0, 485, 524, 136, 739, 748, 491,
        301, 709, 283, 488, 80, 497, 741, 27, 386, 543, 220, 388, 762, 424, 598, 661, 352, 758, 351, 140, 345, 31, 314,
        537, 556, 285, 506, 187, 503, 475, 342, 381, 510, 274, 46, 583, 498, 427, 70, 226, 132, 357, 457, 288, 147, 664,
        54, 180, 463, 213, 706, 333, 341, 434, 732, 57, 100, 698, 337, 648, 218, 603, 305, 102, 428, 426, 374, 744, 462,
        156, 604, 679, 110, 429, 304, 113, 297, 178, 566, 451, 56, 404, 529, 625, 44, 72, 130, 573, 105, 473, 614, 99,
        161, 262, 232, 554, 48, 24, 639, 201, 411, 129, 627, 737, 323, 546, 125, 553, 256, 141, 602, 311, 436, 172, 647,
        122, 699, 609, 590, 35, 234, 410, 275, 615, 700, 88, 596, 525, 508, 377, 522, 93, 307, 461, 663, 126, 42, 423,
        264, 210, 509, 376, 629, 203, 560, 495, 694, 52, 697, 21, 689, 422, 456, 398, 547, 185, 631, 18, 188, 121, 580,
        593, 319, 634, 37, 418, 349, 299, 633, 64, 572, 362, 114, 340, 477, 551, 79, 397, 112, 298, 612, 368, 673, 527,
        672, 735, 635, 101, 564, 176, 318, 367, 582, 468, 632, 205, 168, 721, 690, 149, 308, 734, 245, 211, 115, 520,
        753, 750, 651, 659, 163, 421, 306, 276, 666, 190, 260, 618, 328, 695, 467, 702, 667, 109, 613, 518, 460, 545,
        77, 492, 720, 441, 630, 96, 310, 540, 153, 729, 704, 278, 710, 257, 459, 718, 251, 329, 585, 538, 471, 730, 731,
        279, 225, 725, 23, 338, 610, 523, 568, 354, 312, 584, 747, 652, 665, 200, 621, 405, 763, 313, 760, 108, 320,
        440, 484, 681,
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