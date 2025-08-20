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
    constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 222, 80, 137, 451, 713, 49, 27, 532, 320, 530, 593,
        108, 708, 67, 345, 153, 192, 87, 640, 694, 639, 395, 739, 410, 296, 154, 21, 599, 446, 231, 457, 566, 747, 581,
        177, 660, 710, 391, 38, 32, 641, 327, 385, 175, 260, 394, 170, 240, 219, 437, 360, 425, 452, 513, 282, 631, 765,
        241, 601, 655, 33, 436, 763, 489, 262, 271, 533, 53, 564, 630, 463, 366, 623, 389, 335, 57, 372, 473, 200, 419,
        359, 682, 118, 386, 476, 331, 594, 172, 142, 89, 750, 77, 580, 225, 735, 695, 523, 728, 62, 228, 703, 617, 544,
        332, 521, 539, 572, 636, 496, 709, 211, 60, 424, 550, 224, 656, 168, 444, 692, 447, 157, 266, 498, 128, 292,
        665, 130, 68, 648, 284, 178, 101, 441, 338, 104, 214, 50, 442, 434, 29, 75, 93, 76, 26, 659, 484, 281, 662, 380,
        388, 676, 565, 518, 412, 368, 9, 688, 98, 105, 458, 113, 161, 705, 408, 749, 628, 460, 714, 16, 70, 392, 561,
        413, 369, 406, 25, 274, 227, 103, 173, 318, 148, 373, 288, 182, 184, 679, 30, 28, 340, 595, 303, 576, 646, 761,
        589, 357, 190, 497, 176, 100, 83, 78, 616, 645, 718, 430, 117, 365, 209, 598, 51, 317, 95, 536, 131, 527, 245,
        102, 363, 361, 737, 247, 624, 13, 474, 36, 86, 58, 99, 112, 689, 20, 201, 711, 121, 106, 124, 495, 268, 305,
        462, 44, 706, 244, 353, 677, 619, 621, 744, 666, 35, 700, 333, 467, 348, 286, 255, 5, 693, 17, 491, 764, 252,
        512, 729, 529, 699, 207, 334, 88, 326, 85, 596, 610, 324, 134, 47, 440, 606, 563, 354, 635, 574, 257, 698, 1,
        526, 120, 23, 722, 583, 291, 236, 34, 294, 755, 632, 295, 235, 208, 301, 586, 42, 81, 503, 256, 289, 212, 481,
        328, 620, 422, 71, 55, 678, 534, 733, 321, 24, 448, 745, 116, 751, 370, 650, 743, 754, 229, 56, 329, 487, 553,
        181, 633, 350, 741, 478, 468, 337, 575, 193, 342, 323, 652, 653, 194, 671, 508, 622, 734, 431, 347, 724, 762,
        355, 766, 158, 509, 343, 570, 614, 362, 661, 759, 514, 276, 477, 470, 571, 152, 483, 254, 156, 374, 618, 377,
        400, 352, 285, 346, 381, 753, 97, 315, 520, 18, 760, 738, 349, 664, 250, 206, 393, 376, 644, 90, 500, 330, 511,
        471, 585, 308, 663, 433, 401, 383, 2, 371, 341, 160, 402, 531, 0, 12, 396, 582, 767, 418, 261, 459, 757, 297,
        399, 249, 494, 403, 4, 259, 275, 557, 398, 426, 290, 501, 119, 364, 314, 141, 584, 149, 122, 472, 382, 387, 670,
        548, 48, 405, 82, 155, 65, 587, 94, 165, 423, 39, 456, 221, 213, 732, 310, 239, 439, 420, 126, 432, 202, 185,
        716, 507, 351, 707, 74, 278, 654, 647, 464, 31, 242, 651, 109, 480, 603, 461, 686, 143, 197, 450, 145, 712, 15,
        568, 22, 307, 246, 272, 485, 183, 270, 740, 72, 547, 293, 438, 91, 311, 475, 204, 159, 230, 414, 123, 715, 562,
        180, 313, 502, 188, 704, 300, 602, 3, 415, 515, 416, 129, 524, 449, 195, 493, 339, 66, 237, 264, 243, 258, 140,
        215, 455, 673, 629, 517, 719, 59, 545, 546, 166, 248, 226, 40, 528, 133, 543, 411, 218, 443, 162, 41, 138, 519,
        560, 549, 579, 96, 316, 279, 306, 429, 554, 578, 84, 535, 309, 115, 216, 187, 132, 189, 135, 643, 378, 730, 466,
        139, 490, 482, 492, 217, 691, 590, 555, 592, 358, 638, 407, 573, 263, 167, 186, 169, 758, 253, 199, 273, 597,
        191, 668, 725, 609, 542, 658, 421, 611, 683, 73, 588, 384, 63, 559, 604, 7, 265, 14, 577, 203, 726, 613, 428,
        19, 469, 110, 723, 701, 634, 731, 637, 537, 657, 605, 283, 525, 390, 625, 298, 626, 136, 150, 615, 510, 404,
        499, 163, 174, 612, 667, 54, 375, 179, 277, 6, 721, 541, 43, 234, 649, 552, 299, 251, 669, 607, 10, 322, 454,
        627, 267, 238, 522, 379, 144, 680, 681, 558, 210, 538, 685, 367, 591, 61, 674, 79, 325, 504, 69, 233, 435, 356,
        319, 569, 198, 687, 479, 702, 114, 164, 516, 171, 46, 196, 445, 427, 672, 147, 551, 45, 344, 64, 232, 280, 642,
        127, 146, 151, 37, 717, 409, 223, 727, 488, 11, 417, 684, 269, 506, 675, 720, 608, 304, 736, 111, 600, 8, 742,
        302, 312, 567, 746, 540, 453, 556, 752, 465, 748, 696, 205, 505, 756, 92, 697, 52, 107, 397, 486, 125, 690, 220,
        336, 287,
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