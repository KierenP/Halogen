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
    constexpr std::array<size_t, FT_SIZE / 2> shuffle_order = { 222, 363, 652, 568, 554, 237, 221, 271, 595, 261, 450,
        466, 483, 367, 156, 314, 189, 411, 448, 30, 305, 188, 614, 274, 697, 260, 226, 464, 100, 242, 444, 718, 709,
        749, 696, 683, 121, 624, 63, 520, 310, 181, 548, 648, 379, 670, 258, 123, 360, 125, 517, 26, 317, 658, 187, 597,
        505, 102, 713, 731, 739, 337, 458, 425, 84, 740, 633, 573, 287, 646, 728, 463, 80, 599, 357, 726, 126, 76, 73,
        257, 391, 22, 51, 336, 742, 424, 752, 23, 526, 672, 475, 241, 470, 729, 585, 662, 516, 82, 159, 400, 645, 87,
        435, 131, 415, 178, 58, 306, 97, 725, 653, 301, 412, 402, 49, 703, 632, 469, 518, 540, 194, 452, 549, 433, 581,
        94, 124, 699, 535, 163, 335, 215, 705, 692, 635, 135, 410, 29, 300, 684, 487, 108, 545, 384, 754, 65, 606, 596,
        43, 113, 507, 760, 401, 604, 376, 205, 610, 407, 707, 627, 432, 762, 103, 755, 560, 390, 347, 250, 64, 622, 158,
        6, 543, 179, 592, 586, 429, 406, 20, 264, 47, 502, 388, 655, 647, 246, 761, 486, 59, 574, 199, 17, 443, 175, 18,
        21, 195, 628, 162, 134, 717, 145, 701, 240, 154, 441, 39, 753, 484, 563, 120, 16, 110, 583, 89, 209, 168, 584,
        542, 324, 457, 561, 255, 364, 292, 141, 101, 210, 636, 127, 75, 96, 618, 283, 352, 105, 723, 541, 236, 13, 495,
        0, 482, 182, 177, 173, 564, 32, 657, 118, 231, 693, 254, 312, 619, 393, 349, 620, 138, 180, 128, 71, 95, 167,
        72, 565, 341, 378, 117, 527, 679, 62, 550, 525, 605, 686, 272, 142, 225, 232, 682, 1, 38, 266, 284, 40, 513,
        234, 19, 289, 208, 4, 252, 331, 278, 382, 745, 297, 508, 216, 132, 31, 661, 228, 14, 348, 211, 626, 273, 303,
        315, 710, 52, 640, 27, 85, 494, 333, 664, 248, 386, 666, 311, 323, 766, 691, 111, 455, 204, 524, 223, 609, 330,
        279, 334, 575, 373, 759, 91, 122, 152, 191, 659, 165, 99, 434, 497, 720, 190, 359, 12, 351, 268, 473, 422, 320,
        630, 197, 354, 107, 79, 467, 259, 256, 366, 751, 667, 37, 276, 77, 370, 342, 544, 10, 509, 267, 656, 200, 198,
        695, 668, 355, 214, 381, 326, 763, 35, 387, 285, 356, 591, 25, 702, 322, 56, 93, 2, 203, 302, 298, 129, 183,
        512, 136, 244, 290, 67, 404, 8, 114, 677, 617, 368, 307, 711, 270, 362, 449, 36, 332, 243, 551, 493, 413, 492,
        282, 109, 423, 451, 409, 678, 41, 321, 296, 735, 417, 15, 427, 253, 143, 546, 438, 395, 299, 46, 421, 247, 170,
        556, 730, 11, 338, 42, 7, 74, 732, 442, 68, 414, 201, 151, 230, 716, 446, 764, 33, 734, 81, 115, 70, 459, 358,
        688, 472, 608, 207, 552, 462, 389, 396, 295, 722, 577, 280, 528, 392, 350, 418, 750, 24, 164, 405, 399, 155,
        440, 130, 318, 227, 66, 613, 90, 500, 98, 186, 224, 140, 758, 281, 157, 44, 166, 218, 501, 343, 86, 603, 196,
        265, 514, 741, 377, 747, 262, 169, 706, 489, 468, 479, 375, 522, 498, 340, 478, 529, 171, 638, 339, 235, 345,
        372, 286, 460, 69, 172, 119, 398, 419, 559, 60, 160, 309, 538, 50, 736, 313, 531, 680, 537, 521, 481, 456, 116,
        532, 553, 756, 371, 245, 206, 88, 202, 569, 251, 625, 562, 496, 28, 519, 578, 704, 176, 78, 408, 275, 765, 675,
        217, 369, 580, 146, 724, 674, 681, 558, 539, 428, 594, 480, 397, 649, 229, 249, 239, 277, 579, 319, 436, 430,
        607, 380, 600, 476, 700, 394, 611, 582, 490, 403, 192, 420, 45, 149, 642, 193, 137, 727, 34, 733, 511, 566, 557,
        504, 53, 238, 571, 55, 485, 293, 431, 634, 621, 308, 92, 491, 612, 623, 588, 533, 263, 515, 112, 534, 153, 233,
        174, 461, 416, 139, 708, 57, 288, 5, 631, 184, 598, 593, 54, 748, 219, 471, 669, 499, 148, 536, 643, 757, 719,
        687, 629, 439, 576, 83, 673, 385, 650, 665, 590, 641, 685, 639, 327, 676, 637, 9, 503, 269, 316, 651, 474, 616,
        660, 426, 587, 325, 294, 737, 602, 185, 690, 329, 746, 104, 671, 212, 147, 698, 530, 663, 344, 712, 106, 374,
        721, 644, 447, 361, 694, 437, 523, 346, 601, 291, 133, 304, 161, 465, 61, 506, 654, 567, 488, 150, 547, 213, 48,
        570, 572, 744, 365, 220, 715, 383, 445, 615, 477, 454, 3, 453, 714, 689, 510, 738, 144, 328, 353, 589, 743, 555,
        767,
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