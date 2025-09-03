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
    constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 548, 355, 386, 303, 1, 268, 14, 432, 463, 322, 755,
        249, 689, 625, 115, 745, 508, 520, 621, 611, 107, 250, 143, 637, 699, 367, 675, 626, 248, 176, 471, 377, 13, 82,
        228, 559, 763, 727, 469, 659, 178, 582, 555, 584, 731, 612, 751, 262, 668, 696, 748, 339, 140, 134, 255, 533,
        243, 299, 557, 479, 360, 541, 446, 144, 619, 378, 316, 610, 328, 455, 353, 22, 493, 734, 704, 157, 407, 224, 4,
        554, 406, 507, 740, 111, 449, 8, 329, 223, 47, 764, 562, 509, 156, 91, 765, 286, 131, 11, 401, 239, 618, 706,
        125, 530, 644, 568, 549, 5, 566, 61, 474, 240, 614, 575, 23, 465, 725, 80, 118, 538, 563, 760, 515, 372, 170,
        712, 280, 287, 231, 752, 154, 164, 527, 99, 44, 375, 374, 505, 204, 337, 284, 291, 715, 540, 544, 12, 68, 184,
        0, 567, 670, 21, 122, 40, 317, 66, 297, 711, 518, 666, 363, 52, 162, 142, 36, 399, 444, 686, 93, 149, 147, 411,
        221, 416, 564, 45, 56, 69, 495, 298, 133, 146, 163, 41, 38, 234, 521, 632, 453, 698, 32, 191, 430, 258, 260,
        195, 648, 336, 189, 26, 117, 167, 276, 654, 266, 54, 94, 207, 320, 208, 186, 467, 327, 697, 151, 215, 168, 332,
        656, 138, 358, 362, 211, 343, 606, 86, 498, 199, 653, 487, 29, 669, 357, 388, 212, 394, 366, 657, 736, 18, 74,
        222, 225, 305, 67, 78, 141, 739, 593, 90, 179, 565, 700, 159, 629, 738, 649, 351, 172, 615, 58, 244, 405, 102,
        334, 477, 137, 340, 607, 723, 523, 488, 59, 721, 274, 76, 55, 219, 623, 492, 450, 661, 726, 674, 77, 710, 753,
        112, 265, 110, 160, 139, 447, 31, 531, 350, 722, 685, 707, 392, 684, 546, 236, 15, 64, 247, 161, 601, 500, 308,
        585, 380, 259, 213, 46, 681, 302, 442, 631, 658, 27, 145, 435, 588, 235, 293, 489, 304, 345, 238, 709, 331, 719,
        126, 539, 226, 312, 464, 484, 83, 155, 510, 42, 285, 88, 185, 314, 553, 754, 281, 703, 95, 580, 50, 114, 716,
        595, 583, 682, 97, 767, 616, 104, 246, 759, 359, 344, 237, 335, 73, 757, 279, 702, 300, 431, 128, 194, 233, 28,
        120, 578, 205, 48, 188, 524, 354, 454, 2, 636, 634, 513, 196, 735, 511, 98, 261, 598, 413, 396, 376, 400, 609,
        230, 72, 496, 635, 628, 218, 267, 370, 713, 33, 458, 643, 428, 290, 53, 424, 412, 63, 482, 577, 379, 402, 472,
        602, 183, 198, 34, 733, 253, 241, 20, 452, 256, 84, 289, 393, 201, 749, 252, 678, 443, 254, 436, 349, 408, 486,
        315, 690, 414, 451, 550, 403, 439, 683, 462, 714, 251, 324, 391, 3, 209, 680, 313, 460, 373, 383, 468, 594, 257,
        182, 603, 415, 369, 352, 229, 718, 127, 158, 192, 409, 181, 43, 587, 175, 440, 348, 746, 203, 269, 624, 19, 441,
        277, 456, 121, 536, 613, 338, 193, 497, 547, 501, 581, 89, 729, 506, 423, 96, 381, 429, 512, 71, 766, 220, 227,
        491, 356, 705, 323, 517, 264, 294, 87, 558, 758, 605, 434, 232, 321, 190, 717, 217, 641, 387, 590, 448, 425,
        272, 480, 667, 173, 543, 398, 395, 410, 210, 301, 695, 296, 525, 270, 319, 694, 57, 390, 384, 148, 330, 81, 519,
        51, 679, 556, 307, 418, 75, 7, 417, 596, 245, 693, 561, 132, 572, 466, 499, 608, 579, 130, 570, 476, 433, 271,
        522, 39, 37, 306, 660, 571, 427, 481, 404, 165, 49, 576, 461, 105, 361, 216, 341, 62, 592, 116, 109, 108, 604,
        79, 586, 153, 169, 494, 589, 574, 180, 599, 295, 382, 676, 720, 60, 600, 617, 473, 502, 534, 152, 166, 761, 750,
        419, 310, 742, 542, 665, 275, 214, 292, 639, 371, 483, 385, 688, 347, 325, 365, 516, 630, 103, 445, 278, 747,
        206, 346, 569, 762, 9, 368, 640, 478, 470, 6, 620, 651, 197, 174, 672, 171, 741, 526, 671, 650, 673, 263, 177,
        485, 633, 459, 552, 333, 65, 622, 288, 106, 597, 16, 342, 420, 677, 326, 535, 691, 113, 30, 129, 490, 421, 318,
        24, 311, 422, 560, 101, 662, 364, 573, 119, 645, 627, 282, 426, 136, 397, 504, 242, 646, 708, 545, 85, 200, 642,
        551, 187, 728, 17, 475, 123, 309, 652, 528, 655, 743, 529, 638, 92, 724, 457, 437, 744, 663, 202, 664, 100, 25,
        730, 514, 35, 591, 701, 537, 692, 389, 756, 70, 647, 10, 687, 732, 532, 150, 273, 283, 124, 438, 737, 135, 503,
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