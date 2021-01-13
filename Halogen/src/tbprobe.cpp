/*
Copyright (c) 2015 basil00
Modifications Copyright (c) 2016-2019 by Jon Dart

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <assert.h>
#ifdef __cplusplus
#include <atomic>
#else
#include <stdatomic.h>
#endif
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef TB_NO_STDBOOL
#typedef uint8 bool
#else
#include <stdbool.h>
#endif
#include "tbprobe.h"

#define TB_PIECES 7
#define TB_HASHBITS  (TB_PIECES < 7 ?  11 : 12)
#define TB_MAX_PIECE (TB_PIECES < 7 ? 254 : 650)
#define TB_MAX_PAWN  (TB_PIECES < 7 ? 256 : 861)
#define TB_MAX_SYMS  4096

#ifndef _WIN32
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#define SEP_CHAR ':'
#define FD int
#define FD_ERR -1
typedef size_t map_t;
#else
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#define SEP_CHAR ';'
#define FD HANDLE
#define FD_ERR INVALID_HANDLE_VALUE
typedef HANDLE map_t;
#endif

#ifdef __cplusplus
using namespace std;
#endif

#define DECOMP64

// Threading support
#ifndef TB_NO_THREADS
#if defined(__cplusplus) && (__cplusplus >= 201103L)

#include <mutex>
#define LOCK_T std::mutex
#define LOCK_INIT(x)
#define LOCK_DESTROY(x)
#define LOCK(x) x.lock()
#define UNLOCK(x) x.unlock()

#else
#ifndef _WIN32
#define LOCK_T pthread_mutex_t
#define LOCK_INIT(x) pthread_mutex_init(&(x), NULL)
#define LOCK_DESTROY(x) pthread_mutex_destroy(&(x))
#define LOCK(x) pthread_mutex_lock(&(x))
#define UNLOCK(x) pthread_mutex_unlock(&(x))
#else
#define LOCK_T HANDLE
#define LOCK_INIT(x) do { x = CreateMutex(NULL, FALSE, NULL); } while (0)
#define LOCK_DESTROY(x) CloseHandle(x)
#define LOCK(x) WaitForSingleObject(x, INFINITE)
#define UNLOCK(x) ReleaseMutex(x)
#endif

#endif
#else /* TB_NO_THREADS */
#define LOCK_T          int
#define LOCK_INIT(x)    /* NOP */
#define LOCK_DESTROY(x) /* NOP */
#define LOCK(x)         /* NOP */
#define UNLOCK(x)       /* NOP */
#endif

// population count implementation
#undef TB_SOFTWARE_POP_COUNT

#if defined(TB_CUSTOM_POP_COUNT)
#define popcount(x) TB_CUSTOM_POP_COUNT(x)
#elif defined(TB_NO_HW_POP_COUNT)
#define TB_SOFTWARE_POP_COUNT
#elif defined (__GNUC__) && defined(__x86_64__) && defined(__SSE4_2__)
#include <popcntintrin.h>
#define popcount(x)             (int)_mm_popcnt_u64((x))
#elif defined(_MSC_VER) && (_MSC_VER >= 1500) && defined(_M_AMD64)
#include <nmmintrin.h>
#define popcount(x)             (int)_mm_popcnt_u64((x))
#else
#define TB_SOFTWARE_POP_COUNT
#endif

#ifdef TB_SOFTWARE_POP_COUNT
// Not a recognized compiler/architecture that has popcount:
// fall back to a software popcount. This one is still reasonably
// fast.
static inline unsigned tb_software_popcount(uint64_t x)
{
    x = x - ((x >> 1) & 0x5555555555555555ull);
    x = (x & 0x3333333333333333ull) + ((x >> 2) & 0x3333333333333333ull);
    x = (x + (x >> 4)) & 0x0f0f0f0f0f0f0f0full;
    return (x * 0x0101010101010101ull) >> 56;
}

#define popcount(x) tb_software_popcount(x)
#endif

// LSB (least-significant bit) implementation
#ifdef TB_CUSTOM_LSB
#define lsb(b) TB_CUSTOM_LSB(b)
#else
#if defined(__GNUC__)
static inline unsigned lsb(uint64_t b) {
    assert(b != 0);
    return __builtin_ffsll(b)-1;
}
#elif defined(_MSC_VER)
static inline unsigned lsb(uint64_t b) {
    assert(b != 0);
    DWORD index;
#ifdef _WIN64
    _BitScanForward64(&index,b);
    return (unsigned)index;
#else
    if (b & 0xffffffffULL) {
      _BitScanForward(&index,(unsigned long)(b & 0xffffffffULL));
      return (unsigned)index;
    }
    else {
      _BitScanForward(&index,(unsigned long)(b >> 32));
      return 32 + (unsigned)index;
    }
#endif
}
#else
/* not a compiler/architecture with recognized builtins */
static uint32_t get_bit32(uint64_t x) {
  return (uint32_t)(((int32_t)(x))&-((int32_t)(x)));
}
static const unsigned MAGIC32 = 0xe89b2be;
static const uint32_t MagicTable32[32] = {31,0,9,1,10,20,13,2,7,11,21,23,17,14,3,25,30,8,19,12,6,22,16,24,29,18,5,15,28,4,27,26};
static unsigned lsb(uint64_t b) {
  if (b & 0xffffffffULL)
    return MagicTable32[(get_bit32(b & 0xffffffffULL)*MAGIC32)>>27];
  else
    return MagicTable32[(get_bit32(b >> 32)*MAGIC32)>>27]+32;
}
#endif
#endif

#define max(a,b) a > b ? a : b
#define min(a,b) a < b ? a : b

#include "stdendian.h"

#if _BYTE_ORDER == _BIG_ENDIAN

/* (unused)
static uint64_t from_le_u64(uint64_t input) {
  return bswap64(input);
}
*/

static uint32_t from_le_u32(uint32_t input) {
  return bswap32(input);
}

static uint16_t from_le_u16(uint16_t input) {
  return bswap16(input);
}

static uint64_t from_be_u64(uint64_t x) {
  return x;
}

static uint32_t from_be_u32(uint32_t x) {
  return x;
}

/* (unused)
  static uint16_t from_be_u16(uint16_t x) {
  return x;
 }*/

#else

/* (unused)
static uint64_t from_le_u64(uint64_t x) {
  return x;
}
*/

static uint32_t from_le_u32(uint32_t x) {
  return x;
}

static uint16_t from_le_u16(uint16_t x) {
  return x;
}

static uint64_t from_be_u64(uint64_t input) {
  return bswap64(input);
}

static uint32_t from_be_u32(uint32_t input) {
  return bswap32(input);
}

/* (unused)
static uint16_t from_be_u16(const uint16_t input) {
  return bswap16(input);
}
*/

#endif

inline static uint32_t read_le_u32(void *p)
{
  return from_le_u32(*(uint32_t *)p);
}

inline static uint16_t read_le_u16(void *p)
{
  return from_le_u16(*(uint16_t *)p);
}

static size_t file_size(FD fd) {
#ifdef _WIN32
  LARGE_INTEGER fileSize;
  if (GetFileSizeEx(fd, &fileSize)==0) {
    return 0;
  }
  return (size_t)fileSize.QuadPart;
#else
  struct stat buf;
  if (fstat(fd,&buf)) {
    return 0;
  } else {
    return buf.st_size;
  }
#endif
}

#ifndef TB_NO_THREADS
static LOCK_T tbMutex;
#endif
static int initialized = 0;
static int numPaths = 0;
static char *pathString = NULL;
static char **paths = NULL;

static FD open_tb(const char *str, const char *suffix)
{
  int i;
  FD fd;
  char *file;

  for (i = 0; i < numPaths; i++) {
    file = (char*)malloc(strlen(paths[i]) + strlen(str) +
                         strlen(suffix) + 2);
    strcpy(file, paths[i]);
#ifdef _WIN32
    strcat(file,"\\");
#else
    strcat(file,"/");
#endif
    strcat(file, str);
    strcat(file, suffix);
#ifndef _WIN32
    fd = open(file, O_RDONLY);
#else
#ifdef _UNICODE
    wchar_t ucode_name[4096];
    size_t len;
    mbstowcs_s(&len, ucode_name, 4096, file, _TRUNCATE);
    fd = CreateFile(ucode_name, GENERIC_READ, FILE_SHARE_READ, NULL,
			  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
#else
    fd = CreateFile(file, GENERIC_READ, FILE_SHARE_READ, NULL,
			  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
#endif
#endif
    free(file);
    if (fd != FD_ERR) {
      return fd;
    }
  }
  return FD_ERR;
}

static void close_tb(FD fd)
{
#ifndef _WIN32
  close(fd);
#else
  CloseHandle(fd);
#endif
}

static void *map_file(FD fd, map_t *mapping)
{
#ifndef _WIN32
  struct stat statbuf;
  if (fstat(fd, &statbuf)) {
    perror("fstat");
    close_tb(fd);
    return NULL;
  }
  *mapping = statbuf.st_size;
  void *data = mmap(NULL, statbuf.st_size, PROT_READ,
			      MAP_SHARED, fd, 0);
  if (data == MAP_FAILED) {
    perror("mmap");
    return NULL;
  }
#else
  DWORD size_low, size_high;
  size_low = GetFileSize(fd, &size_high);
  HANDLE map = CreateFileMapping(fd, NULL, PAGE_READONLY, size_high, size_low,
				  NULL);
  if (map == NULL) {
    fprintf(stderr,"CreateFileMapping() failed, error = %lu.\n", GetLastError());
    return NULL;
  }
  *mapping = (map_t)map;
  void *data = (void *)MapViewOfFile(map, FILE_MAP_READ, 0, 0, 0);
  if (data == NULL) {
    fprintf(stderr,"MapViewOfFile() failed, error = %lu.\n", GetLastError());
  }
#endif
  return data;
}

#ifndef _WIN32
static void unmap_file(void *data, map_t size)
{
  if (!data) return;
  if (!munmap(data, size)) {
	  perror("munmap");
  }
}
#else
static void unmap_file(void *data, map_t mapping)
{
  if (!data) return;
  if (!UnmapViewOfFile(data)) {
	  fprintf(stderr, "unmap failed, error code %lu\n", GetLastError());
  }
  if (!CloseHandle((HANDLE)mapping)) {
	  fprintf(stderr, "CloseHandle failed, error code %lu\n", GetLastError());
  }
}
#endif

#define poplsb(x)               ((x) & ((x) - 1))

int TB_MaxCardinality = 0, TB_MaxCardinalityDTM = 0;
unsigned TB_LARGEST = 0;
//extern int TB_CardinalityDTM;

static const char *tbSuffix[] = { ".rtbw", ".rtbm", ".rtbz" };
static uint32_t tbMagic[] = { 0x5d23e871, 0x88ac504b, 0xa50c66d7 };

enum { WDL, DTM, DTZ };
enum { PIECE_ENC, FILE_ENC, RANK_ENC };

// Attack and move generation code
#define TB_PAWN 1
#define TB_KNIGHT 2
#define TB_BISHOP 3
#define TB_ROOK 4
#define TB_QUEEN 5
#define TB_KING 6

#define TB_WPAWN TB_PAWN
#define TB_BPAWN (TB_PAWN | 8)

#define WHITE_KING              (TB_WPAWN + 5)
#define WHITE_QUEEN             (TB_WPAWN + 4)
#define WHITE_ROOK              (TB_WPAWN + 3)
#define WHITE_BISHOP            (TB_WPAWN + 2)
#define WHITE_KNIGHT            (TB_WPAWN + 1)
#define WHITE_PAWN              TB_WPAWN
#define BLACK_KING              (TB_BPAWN + 5)
#define BLACK_QUEEN             (TB_BPAWN + 4)
#define BLACK_ROOK              (TB_BPAWN + 3)
#define BLACK_BISHOP            (TB_BPAWN + 2)
#define BLACK_KNIGHT            (TB_BPAWN + 1)
#define BLACK_PAWN              TB_BPAWN

#define PRIME_WHITE_QUEEN       11811845319353239651ull
#define PRIME_WHITE_ROOK        10979190538029446137ull
#define PRIME_WHITE_BISHOP      12311744257139811149ull
#define PRIME_WHITE_KNIGHT      15202887380319082783ull
#define PRIME_WHITE_PAWN        17008651141875982339ull
#define PRIME_BLACK_QUEEN       15484752644942473553ull
#define PRIME_BLACK_ROOK        18264461213049635989ull
#define PRIME_BLACK_BISHOP      15394650811035483107ull
#define PRIME_BLACK_KNIGHT      13469005675588064321ull
#define PRIME_BLACK_PAWN        11695583624105689831ull

#define BOARD_RANK_EDGE         0x8181818181818181ull
#define BOARD_FILE_EDGE         0xFF000000000000FFull
#define BOARD_EDGE              (BOARD_RANK_EDGE | BOARD_FILE_EDGE)
#define BOARD_RANK_1            0x00000000000000FFull
#define BOARD_FILE_A            0x8080808080808080ull

#define KEY_KvK                 0

#define BEST_NONE               0xFFFF
#define SCORE_ILLEGAL           0x7FFF

// Note: WHITE, BLACK values are reverse of Stockfish
#ifdef __cplusplus
namespace {
    enum Color { TB_BLACK, TB_WHITE };
    enum PieceType { TB_ENUM_PAWN = 1, TB_ENUM_KNIGHT, TB_ENUM_BISHOP, TB_ENUM_ROOK, TB_ENUM_QUEEN, TB_ENUM_KING };
    enum Piece {
        W_PAWN = 1, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
        B_PAWN = 9, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING
    };
#else
typedef enum Color { BLACK, WHITE } Color;
typedef enum PieceType { PAWN = 1, KNIGHT, BISHOP, ROOK, QUEEN, KING } PieceType;
typedef enum Piece {
    W_PAWN = 1, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
    B_PAWN = 9, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING
} Piece;
#endif

static inline Color ColorOfPiece(int piece) {
    return (Color)(!(piece >> 3));
}

static inline PieceType TypeOfPiece(int piece) {
    return (PieceType)(piece & 7);
}

typedef int32_t Value;

typedef struct Pos
{
    uint64_t white;
    uint64_t black;
    uint64_t kings;
    uint64_t queens;
    uint64_t rooks;
    uint64_t bishops;
    uint64_t knights;
    uint64_t pawns;
    uint8_t rule50;
    uint8_t ep;
    bool turn;
} Pos;

static inline uint64_t pieces_by_type(const Pos* pos, Color c, PieceType p) {
    uint64_t mask = (c == TB_WHITE) ? pos->white : pos->black;
    switch (p) {
    case TB_ENUM_PAWN:
        return pos->pawns & mask;
    case TB_ENUM_KNIGHT:
        return pos->knights & mask;
    case TB_ENUM_BISHOP:
        return pos->bishops & mask;
    case TB_ENUM_ROOK:
        return pos->rooks & mask;
    case TB_ENUM_QUEEN:
        return pos->queens & mask;
    case TB_ENUM_KING:
        return pos->kings & mask;
    default:
        assert(0);
        return 0;
    }
}

static const char piece_to_char[] = " PNBRQK  pnbrqk";

// map upper-case characters to piece types
static PieceType char_to_piece_type(char c) {
    for (int i = TB_ENUM_PAWN; i <= TB_ENUM_KING; i++)
        if (c == piece_to_char[i]) {
            return (PieceType)i;
        }
    return (PieceType)0;
}

#define rank(s)                 ((s) >> 3)
#define file(s)                 ((s) & 0x07)
#define board(s)                ((uint64_t)1 << (s))
#define square(r, f)            (8 * (r) + (f))

#ifdef TB_KING_ATTACKS
#define king_attacks(s)         TB_KING_ATTACKS(s)
#define king_attacks_init()     /* NOP */
#else       /* TB_KING_ATTACKS */

static uint64_t king_attacks_table[64];

#define king_attacks(s)         king_attacks_table[(s)]

static void king_attacks_init(void)
{
    for (unsigned s = 0; s < 64; s++)
    {
        unsigned r = rank(s);
        unsigned f = file(s);
        uint64_t b = 0;
        if (r != 0 && f != 0)
            b |= board(square(r - 1, f - 1));
        if (r != 0)
            b |= board(square(r - 1, f));
        if (r != 0 && f != 7)
            b |= board(square(r - 1, f + 1));
        if (f != 7)
            b |= board(square(r, f + 1));
        if (r != 7 && f != 7)
            b |= board(square(r + 1, f + 1));
        if (r != 7)
            b |= board(square(r + 1, f));
        if (r != 7 && f != 0)
            b |= board(square(r + 1, f - 1));
        if (f != 0)
            b |= board(square(r, f - 1));
        king_attacks_table[s] = b;
    }
}

#endif      /* TB_KING_ATTACKS */

#ifdef TB_KNIGHT_ATTACKS
#define knight_attacks(s)       TB_KNIGHT_ATTACKS(s)
#define knight_attacks_init()   /* NOP */
#else       /* TB_KNIGHT_ATTACKS */

static uint64_t knight_attacks_table[64];

#define knight_attacks(s)       knight_attacks_table[(s)]

static void knight_attacks_init(void)
{
    for (unsigned s = 0; s < 64; s++)
    {
        int r1, r = rank(s);
        int f1, f = file(s);
        uint64_t b = 0;
        r1 = r - 1; f1 = f - 2;
        if (r1 >= 0 && f1 >= 0)
            b |= board(square(r1, f1));
        r1 = r - 1; f1 = f + 2;
        if (r1 >= 0 && f1 <= 7)
            b |= board(square(r1, f1));
        r1 = r - 2; f1 = f - 1;
        if (r1 >= 0 && f1 >= 0)
            b |= board(square(r1, f1));
        r1 = r - 2; f1 = f + 1;
        if (r1 >= 0 && f1 <= 7)
            b |= board(square(r1, f1));
        r1 = r + 1; f1 = f - 2;
        if (r1 <= 7 && f1 >= 0)
            b |= board(square(r1, f1));
        r1 = r + 1; f1 = f + 2;
        if (r1 <= 7 && f1 <= 7)
            b |= board(square(r1, f1));
        r1 = r + 2; f1 = f - 1;
        if (r1 <= 7 && f1 >= 0)
            b |= board(square(r1, f1));
        r1 = r + 2; f1 = f + 1;
        if (r1 <= 7 && f1 <= 7)
            b |= board(square(r1, f1));
        knight_attacks_table[s] = b;
    }
}

#endif      /* TB_KNIGHT_ATTACKS */

#ifdef TB_BISHOP_ATTACKS
#define bishop_attacks(s, occ)  TB_BISHOP_ATTACKS(s, occ)
#define bishop_attacks_init()   /* NOP */
#else       /* TB_BISHOP_ATTACKS */

static uint64_t diag_attacks_table[64][64];
static uint64_t anti_attacks_table[64][64];

static const unsigned square2diag_table[64] =
{
    0,  1,  2,  3,  4,  5,  6,  7,
    14, 0,  1,  2,  3,  4,  5,  6,
    13, 14, 0,  1,  2,  3,  4,  5,
    12, 13, 14, 0,  1,  2,  3,  4,
    11, 12, 13, 14, 0,  1,  2,  3,
    10, 11, 12, 13, 14, 0,  1,  2,
    9,  10, 11, 12, 13, 14, 0,  1,
    8,  9,  10, 11, 12, 13, 14, 0
};

static const unsigned square2anti_table[64] =
{
    8,  9,  10, 11, 12, 13, 14, 0,
    9,  10, 11, 12, 13, 14, 0,  1,
    10, 11, 12, 13, 14, 0,  1,  2,
    11, 12, 13, 14, 0,  1,  2,  3,
    12, 13, 14, 0,  1,  2,  3,  4,
    13, 14, 0,  1,  2,  3,  4,  5,
    14, 0,  1,  2,  3,  4,  5,  6,
    0,  1,  2,  3,  4,  5,  6,  7
};

static const uint64_t diag2board_table[15] =
{
    0x8040201008040201ull,
    0x0080402010080402ull,
    0x0000804020100804ull,
    0x0000008040201008ull,
    0x0000000080402010ull,
    0x0000000000804020ull,
    0x0000000000008040ull,
    0x0000000000000080ull,
    0x0100000000000000ull,
    0x0201000000000000ull,
    0x0402010000000000ull,
    0x0804020100000000ull,
    0x1008040201000000ull,
    0x2010080402010000ull,
    0x4020100804020100ull,
};

static const uint64_t anti2board_table[15] =
{
    0x0102040810204080ull,
    0x0204081020408000ull,
    0x0408102040800000ull,
    0x0810204080000000ull,
    0x1020408000000000ull,
    0x2040800000000000ull,
    0x4080000000000000ull,
    0x8000000000000000ull,
    0x0000000000000001ull,
    0x0000000000000102ull,
    0x0000000000010204ull,
    0x0000000001020408ull,
    0x0000000102040810ull,
    0x0000010204081020ull,
    0x0001020408102040ull,
};

static inline size_t diag2index(uint64_t b)
{
    b *= 0x0101010101010101ull;
    b >>= 56;
    b >>= 1;
    return (size_t)b;
}

static inline size_t anti2index(uint64_t b)
{
    return diag2index(b);
}

#define diag(s)                 square2diag_table[(s)]
#define anti(s)                 square2anti_table[(s)]
#define diag2board(d)           diag2board_table[(d)]
#define anti2board(a)           anti2board_table[(a)]

static uint64_t bishop_attacks(unsigned sq, uint64_t occ)
{
    occ &= ~board(sq);
    unsigned d = diag(sq), a = anti(sq);
    uint64_t d_occ = occ & (diag2board(d) & ~BOARD_EDGE);
    uint64_t a_occ = occ & (anti2board(a) & ~BOARD_EDGE);
    size_t d_idx = diag2index(d_occ);
    size_t a_idx = anti2index(a_occ);
    uint64_t d_attacks = diag_attacks_table[sq][d_idx];
    uint64_t a_attacks = anti_attacks_table[sq][a_idx];
    return d_attacks | a_attacks;
}

static void bishop_attacks_init(void)
{
    for (unsigned idx = 0; idx < 64; idx++)
    {
        unsigned idx1 = idx << 1;
        for (unsigned s = 0; s < 64; s++)
        {
            int r = rank(s);
            int f = file(s);
            uint64_t b = 0;
            for (int i = -1; f + i >= 0 && r + i >= 0; i--)
            {
                unsigned occ = (1 << (f + i));
                b |= board(square(r + i, f + i));
                if (idx1 & occ)
                    break;
            }
            for (int i = 1; f + i <= 7 && r + i <= 7; i++)
            {
                unsigned occ = (1 << (f + i));
                b |= board(square(r + i, f + i));
                if (idx1 & occ)
                    break;
            }
            diag_attacks_table[s][idx] = b;
        }
    }

    for (unsigned idx = 0; idx < 64; idx++)
    {
        unsigned idx1 = idx << 1;
        for (unsigned s = 0; s < 64; s++)
        {
            int r = rank(s);
            int f = file(s);
            uint64_t b = 0;
            for (int i = -1; f + i >= 0 && r - i <= 7; i--)
            {
                unsigned occ = (1 << (f + i));
                b |= board(square(r - i, f + i));
                if (idx1 & occ)
                    break;
            }
            for (int i = 1; f + i <= 7 && r - i >= 0; i++)
            {
                unsigned occ = (1 << (f + i));
                b |= board(square(r - i, f + i));
                if (idx1 & occ)
                    break;
            }
            anti_attacks_table[s][idx] = b;
        }
    }
}

#endif      /* TB_BISHOP_ATTACKS */

#ifdef TB_ROOK_ATTACKS
#define rook_attacks(s, occ)    TB_ROOK_ATTACKS(s, occ)
#define rook_attacks_init()     /* NOP */
#else       /* TB_ROOK_ATTACKS */

static uint64_t rank_attacks_table[64][64];
static uint64_t file_attacks_table[64][64];

static inline size_t rank2index(uint64_t b, unsigned r)
{
    b >>= (8 * r);
    b >>= 1;
    return (size_t)b;
}

static inline size_t file2index(uint64_t b, unsigned f)
{
    b >>= f;
    b *= 0x0102040810204080ull;
    b >>= 56;
    b >>= 1;
    return (size_t)b;
}

#define rank2board(r)           (0xFFull << (8 * (r)))
#define file2board(f)           (0x0101010101010101ull << (f))

static uint64_t rook_attacks(unsigned sq, uint64_t occ)
{
    occ &= ~board(sq);
    unsigned r = rank(sq), f = file(sq);
    uint64_t r_occ = occ & (rank2board(r) & ~BOARD_RANK_EDGE);
    uint64_t f_occ = occ & (file2board(f) & ~BOARD_FILE_EDGE);
    size_t r_idx = rank2index(r_occ, r);
    size_t f_idx = file2index(f_occ, f);
    uint64_t r_attacks = rank_attacks_table[sq][r_idx];
    uint64_t f_attacks = file_attacks_table[sq][f_idx];
    return r_attacks | f_attacks;
}

static void rook_attacks_init(void)
{
    for (unsigned idx = 0; idx < 64; idx++)
    {
        unsigned idx1 = idx << 1, occ;
        for (int f = 0; f <= 7; f++)
        {
            uint64_t b = 0;
            if (f > 0)
            {
                int i = f - 1;
                do
                {
                    occ = (1 << i);
                    b |= board(square(0, i));
                    i--;
                } while (!(idx1 & occ) && i >= 0);
            }
            if (f < 7)
            {
                int i = f + 1;
                do
                {
                    occ = (1 << i);
                    b |= board(square(0, i));
                    i++;
                } while (!(idx1 & occ) && i <= 7);
            }
            for (int r = 0; r <= 7; r++)
            {
                rank_attacks_table[square(r, f)][idx] = b;
                b <<= 8;
            }
        }
    }
    for (unsigned idx = 0; idx < 64; idx++)
    {
        unsigned idx1 = idx << 1, occ;
        for (int r = 0; r <= 7; r++)
        {
            uint64_t b = 0;
            if (r > 0)
            {
                int i = r - 1;
                do
                {
                    occ = (1 << i);
                    b |= board(square(i, 0));
                    i--;
                } while (!(idx1 & occ) && i >= 0);
            }
            if (r < 7)
            {
                int i = r + 1;
                do
                {
                    occ = (1 << i);
                    b |= board(square(i, 0));
                    i++;
                } while (!(idx1 & occ) && i <= 7);
            }
            for (int f = 0; f <= 7; f++)
            {
                file_attacks_table[square(r, f)][idx] = b;
                b <<= 1;
            }
        }
    }
}

#endif      /* TB_ROOK_ATTACKS */

#ifdef TB_QUEEN_ATTACKS
#define queen_attacks(s, occ)   TB_QUEEN_ATTACKS(s, occ)
#else       /* TB_QUEEN_ATTACKS */
#define queen_attacks(s, occ)   \
    (rook_attacks((s), (occ)) | bishop_attacks((s), (occ)))
#endif      /* TB_QUEEN_ATTACKS */

#ifdef TB_PAWN_ATTACKS
#define pawn_attacks(s, c)      TB_PAWN_ATTACKS(s, c)
#define pawn_attacks_init()     /* NOP */
#else       /* TB_PAWN_ATTACKS */

static uint64_t pawn_attacks_table[2][64];

#define pawn_attacks(s, c)      pawn_attacks_table[(c)][(s)]

static void pawn_attacks_init(void)
{
    for (unsigned s = 0; s < 64; s++)
    {
        int r = rank(s);
        int f = file(s);

        uint64_t b = 0;
        if (r != 7)
        {
            if (f != 0)
                b |= board(square(r + 1, f - 1));
            if (f != 7)
                b |= board(square(r + 1, f + 1));
        }
        pawn_attacks_table[1][s] = b;

        b = 0;
        if (r != 0)
        {
            if (f != 0)
                b |= board(square(r - 1, f - 1));
            if (f != 7)
                b |= board(square(r - 1, f + 1));
        }
        pawn_attacks_table[0][s] = b;
    }
}

#endif      /* TB_PAWN_ATTACKS */

/*
 * Given a position, produce a 64-bit material signature key.
 */
static uint64_t calc_key(const Pos* pos, bool mirror)
{
    uint64_t white = pos->white, black = pos->black;
    if (mirror)
    {
        uint64_t tmp = white;
        white = black;
        black = tmp;
    }
    return popcount(white & pos->queens) * PRIME_WHITE_QUEEN +
        popcount(white & pos->rooks) * PRIME_WHITE_ROOK +
        popcount(white & pos->bishops) * PRIME_WHITE_BISHOP +
        popcount(white & pos->knights) * PRIME_WHITE_KNIGHT +
        popcount(white & pos->pawns) * PRIME_WHITE_PAWN +
        popcount(black & pos->queens) * PRIME_BLACK_QUEEN +
        popcount(black & pos->rooks) * PRIME_BLACK_ROOK +
        popcount(black & pos->bishops) * PRIME_BLACK_BISHOP +
        popcount(black & pos->knights) * PRIME_BLACK_KNIGHT +
        popcount(black & pos->pawns) * PRIME_BLACK_PAWN;
}

// Produce a 64-bit material key corresponding to the material combination
// defined by pcs[16], where pcs[1], ..., pcs[6] are the number of white
// pawns, ..., kings and pcs[9], ..., pcs[14] are the number of black
// pawns, ..., kings.
static uint64_t calc_key_from_pcs(int* pcs, int mirror)
{
    mirror = (mirror ? 8 : 0);
    return pcs[WHITE_QUEEN ^ mirror] * PRIME_WHITE_QUEEN +
        pcs[WHITE_ROOK ^ mirror] * PRIME_WHITE_ROOK +
        pcs[WHITE_BISHOP ^ mirror] * PRIME_WHITE_BISHOP +
        pcs[WHITE_KNIGHT ^ mirror] * PRIME_WHITE_KNIGHT +
        pcs[WHITE_PAWN ^ mirror] * PRIME_WHITE_PAWN +
        pcs[BLACK_QUEEN ^ mirror] * PRIME_BLACK_QUEEN +
        pcs[BLACK_ROOK ^ mirror] * PRIME_BLACK_ROOK +
        pcs[BLACK_BISHOP ^ mirror] * PRIME_BLACK_BISHOP +
        pcs[BLACK_KNIGHT ^ mirror] * PRIME_BLACK_KNIGHT +
        pcs[BLACK_PAWN ^ mirror] * PRIME_BLACK_PAWN;
}

// Produce a 64-bit material key corresponding to the material combination
// piece[0], ..., piece[num - 1], where each value corresponds to a piece
// (1-6 for white pawn-king, 9-14 for black pawn-king).
static uint64_t calc_key_from_pieces(uint8_t* piece, int num)
{
    uint64_t key = 0;
    static const uint64_t keys[16] = { 0,PRIME_WHITE_PAWN,PRIME_WHITE_KNIGHT,
                                      PRIME_WHITE_BISHOP,PRIME_WHITE_ROOK,
                                      PRIME_WHITE_QUEEN,0,0,PRIME_BLACK_PAWN,
                                      PRIME_BLACK_KNIGHT,PRIME_BLACK_BISHOP,
                                      PRIME_BLACK_ROOK,PRIME_BLACK_QUEEN,0 };
    for (int i = 0; i < num; i++) {
        assert(piece[i] < 16);
        key += keys[piece[i]];
    }
    return key;
}

#define make_move(promote, from, to)                                    \
    ((((promote) & 0x7) << 12) | (((from) & 0x3F) << 6) | ((to) & 0x3F))
#define move_from(move)                                                 \
    (((move) >> 6) & 0x3F)
#define move_to(move)                                                   \
    ((move) & 0x3F)
#define move_promotes(move)                                             \
    (((move) >> 12) & 0x7)

static inline int type_of_piece_moved(Pos* pos, TbMove move) {
    for (int i = TB_ENUM_PAWN; i <= TB_ENUM_KING; i++) {
        if ((pieces_by_type(pos, (Color)(pos->turn == TB_WHITE), (PieceType)i) & board(move_from(move))) != 0) {
            return i;
        }
    }
    assert(0);
    return 0;
}

#define MAX_MOVES               TB_MAX_MOVES
#define MOVE_STALEMATE          0xFFFF
#define MOVE_CHECKMATE          0xFFFE

static TbMove* add_move(TbMove* moves, bool promotes, unsigned from,
    unsigned to)
{
    if (!promotes)
        *moves++ = make_move(TB_PROMOTES_NONE, from, to);
    else
    {
        *moves++ = make_move(TB_PROMOTES_QUEEN, from, to);
        *moves++ = make_move(TB_PROMOTES_KNIGHT, from, to);
        *moves++ = make_move(TB_PROMOTES_ROOK, from, to);
        *moves++ = make_move(TB_PROMOTES_BISHOP, from, to);
    }
    return moves;
}

/*
 * Generate all captures, including all underpomotions
 */
static TbMove* gen_captures(const Pos* pos, TbMove* moves)
{
    uint64_t occ = pos->white | pos->black;
    uint64_t us = (pos->turn ? pos->white : pos->black),
        them = (pos->turn ? pos->black : pos->white);
    uint64_t b, att;
    {
        unsigned from = lsb(pos->kings & us);
        assert(from < 64);
        for (att = king_attacks(from) & them; att; att = poplsb(att))
        {
            unsigned to = lsb(att);
            moves = add_move(moves, false, from, to);
        }
    }
    for (b = us & pos->queens; b; b = poplsb(b))
    {
        unsigned from = lsb(b);
        for (att = queen_attacks(from, occ) & them; att; att = poplsb(att))
        {
            unsigned to = lsb(att);
            moves = add_move(moves, false, from, to);
        }
    }
    for (b = us & pos->rooks; b; b = poplsb(b))
    {
        unsigned from = lsb(b);
        for (att = rook_attacks(from, occ) & them; att; att = poplsb(att))
        {
            unsigned to = lsb(att);
            moves = add_move(moves, false, from, to);
        }
    }
    for (b = us & pos->bishops; b; b = poplsb(b))
    {
        unsigned from = lsb(b);
        for (att = bishop_attacks(from, occ) & them; att; att = poplsb(att))
        {
            unsigned to = lsb(att);
            moves = add_move(moves, false, from, to);
        }
    }
    for (b = us & pos->knights; b; b = poplsb(b))
    {
        unsigned from = lsb(b);
        for (att = knight_attacks(from) & them; att; att = poplsb(att))
        {
            unsigned to = lsb(att);
            moves = add_move(moves, false, from, to);
        }
    }
    for (b = us & pos->pawns; b; b = poplsb(b))
    {
        unsigned from = lsb(b);
        att = pawn_attacks(from, pos->turn);
        if (pos->ep != 0 && ((att & board(pos->ep)) != 0))
        {
            unsigned to = pos->ep;
            moves = add_move(moves, false, from, to);
        }
        for (att = att & them; att; att = poplsb(att))
        {
            unsigned to = lsb(att);
            moves = add_move(moves, (rank(to) == 7 || rank(to) == 0), from,
                to);
        }
    }
    return moves;
}

/*
 * Generate all moves.
 */
static TbMove* gen_moves(const Pos* pos, TbMove* moves)
{
    uint64_t occ = pos->white | pos->black;
    uint64_t us = (pos->turn ? pos->white : pos->black),
        them = (pos->turn ? pos->black : pos->white);
    uint64_t b, att;

    {
        unsigned from = lsb(pos->kings & us);
        for (att = king_attacks(from) & ~us; att; att = poplsb(att))
        {
            unsigned to = lsb(att);
            moves = add_move(moves, false, from, to);
        }
    }
    for (b = us & pos->queens; b; b = poplsb(b))
    {
        unsigned from = lsb(b);
        for (att = queen_attacks(from, occ) & ~us; att; att = poplsb(att))
        {
            unsigned to = lsb(att);
            moves = add_move(moves, false, from, to);
        }
    }
    for (b = us & pos->rooks; b; b = poplsb(b))
    {
        unsigned from = lsb(b);
        for (att = rook_attacks(from, occ) & ~us; att; att = poplsb(att))
        {
            unsigned to = lsb(att);
            moves = add_move(moves, false, from, to);
        }
    }
    for (b = us & pos->bishops; b; b = poplsb(b))
    {
        unsigned from = lsb(b);
        for (att = bishop_attacks(from, occ) & ~us; att; att = poplsb(att))
        {
            unsigned to = lsb(att);
            moves = add_move(moves, false, from, to);
        }
    }
    for (b = us & pos->knights; b; b = poplsb(b))
    {
        unsigned from = lsb(b);
        for (att = knight_attacks(from) & ~us; att; att = poplsb(att))
        {
            unsigned to = lsb(att);
            moves = add_move(moves, false, from, to);
        }
    }
    for (b = us & pos->pawns; b; b = poplsb(b))
    {
        unsigned from = lsb(b);
        unsigned next = from + (pos->turn ? 8 : -8);
        att = pawn_attacks(from, pos->turn);
        if (pos->ep != 0 && ((att & board(pos->ep)) != 0))
        {
            unsigned to = pos->ep;
            moves = add_move(moves, false, from, to);
        }
        att &= them;
        if ((board(next) & occ) == 0)
        {
            att |= board(next);
            unsigned next2 = from + (pos->turn ? 16 : -16);
            if ((pos->turn ? rank(from) == 1 : rank(from) == 6) &&
                ((board(next2) & occ) == 0))
                att |= board(next2);
        }
        for (; att; att = poplsb(att))
        {
            unsigned to = lsb(att);
            moves = add_move(moves, (rank(to) == 7 || rank(to) == 0), from,
                to);
        }
    }
    return moves;
}

/*
 * Test if the given move is an en passant capture.
 */
static bool is_en_passant(const Pos* pos, TbMove move)
{
    uint16_t from = move_from(move);
    uint16_t to = move_to(move);
    uint64_t us = (pos->turn ? pos->white : pos->black);
    if (pos->ep == 0)
        return false;
    if (to != pos->ep)
        return false;
    if ((board(from) & us & pos->pawns) == 0)
        return false;
    return true;
}


/*
 * Test if the given move is a capture.
 */
static bool is_capture(const Pos* pos, TbMove move)
{
    uint16_t to = move_to(move);
    uint64_t them = (pos->turn ? pos->black : pos->white);
    return (them & board(to)) != 0 || is_en_passant(pos, move);
}


/*
 * Test if the given position is legal.
 * (Pawns on backrank? Can the king be captured?)
 */
static bool is_legal(const Pos* pos)
{
    uint64_t occ = pos->white | pos->black;
    uint64_t us = (pos->turn ? pos->black : pos->white),
        them = (pos->turn ? pos->white : pos->black);
    uint64_t king = pos->kings & us;
    if (!king)
        return false;
    unsigned sq = lsb(king);
    if (king_attacks(sq) & (pos->kings & them))
        return false;
    uint64_t ratt = rook_attacks(sq, occ);
    uint64_t batt = bishop_attacks(sq, occ);
    if (ratt & (pos->rooks & them))
        return false;
    if (batt & (pos->bishops & them))
        return false;
    if ((ratt | batt) & (pos->queens & them))
        return false;
    if (knight_attacks(sq) & (pos->knights & them))
        return false;
    if (pawn_attacks(sq, !pos->turn) & (pos->pawns & them))
        return false;
    return true;
}

/*
 * Test if the king is in check.
 */
static bool is_check(const Pos* pos)
{
    uint64_t occ = pos->white | pos->black;
    uint64_t us = (pos->turn ? pos->white : pos->black),
        them = (pos->turn ? pos->black : pos->white);
    uint64_t king = pos->kings & us;
    assert(king != 0);
    unsigned sq = lsb(king);
    uint64_t ratt = rook_attacks(sq, occ);
    uint64_t batt = bishop_attacks(sq, occ);
    if (ratt & (pos->rooks & them))
        return true;
    if (batt & (pos->bishops & them))
        return true;
    if ((ratt | batt) & (pos->queens & them))
        return true;
    if (knight_attacks(sq) & (pos->knights & them))
        return true;
    if (pawn_attacks(sq, pos->turn) & (pos->pawns & them))
        return true;
    return false;
}

/*
 * Test if the position is valid.
 */
static bool is_valid(const Pos* pos)
{
    if (popcount(pos->kings) != 2)
        return false;
    if (popcount(pos->kings & pos->white) != 1)
        return false;
    if (popcount(pos->kings & pos->black) != 1)
        return false;
    if ((pos->white & pos->black) != 0)
        return false;
    if ((pos->kings & pos->queens) != 0)
        return false;
    if ((pos->kings & pos->rooks) != 0)
        return false;
    if ((pos->kings & pos->bishops) != 0)
        return false;
    if ((pos->kings & pos->knights) != 0)
        return false;
    if ((pos->kings & pos->pawns) != 0)
        return false;
    if ((pos->queens & pos->rooks) != 0)
        return false;
    if ((pos->queens & pos->bishops) != 0)
        return false;
    if ((pos->queens & pos->knights) != 0)
        return false;
    if ((pos->queens & pos->pawns) != 0)
        return false;
    if ((pos->rooks & pos->bishops) != 0)
        return false;
    if ((pos->rooks & pos->knights) != 0)
        return false;
    if ((pos->rooks & pos->pawns) != 0)
        return false;
    if ((pos->bishops & pos->knights) != 0)
        return false;
    if ((pos->bishops & pos->pawns) != 0)
        return false;
    if ((pos->knights & pos->pawns) != 0)
        return false;
    if (pos->pawns & BOARD_FILE_EDGE)
        return false;
    if ((pos->white | pos->black) !=
        (pos->kings | pos->queens | pos->rooks | pos->bishops | pos->knights |
            pos->pawns))
        return false;
    return is_legal(pos);
}

#define do_bb_move(b, from, to)                                         \
    (((b) & (~board(to)) & (~board(from))) |                            \
        ((((b) >> (from)) & 0x1) << (to)))

static bool do_move(Pos* pos, const Pos* pos0, TbMove move)
{
    unsigned from = move_from(move);
    unsigned to = move_to(move);
    unsigned promotes = move_promotes(move);
    pos->turn = !pos0->turn;
    pos->white = do_bb_move(pos0->white, from, to);
    pos->black = do_bb_move(pos0->black, from, to);
    pos->kings = do_bb_move(pos0->kings, from, to);
    pos->queens = do_bb_move(pos0->queens, from, to);
    pos->rooks = do_bb_move(pos0->rooks, from, to);
    pos->bishops = do_bb_move(pos0->bishops, from, to);
    pos->knights = do_bb_move(pos0->knights, from, to);
    pos->pawns = do_bb_move(pos0->pawns, from, to);
    pos->ep = 0;
    if (promotes != TB_PROMOTES_NONE)
    {
        pos->pawns &= ~board(to);       // Promotion
        switch (promotes)
        {
        case TB_PROMOTES_QUEEN:
            pos->queens |= board(to); break;
        case TB_PROMOTES_ROOK:
            pos->rooks |= board(to); break;
        case TB_PROMOTES_BISHOP:
            pos->bishops |= board(to); break;
        case TB_PROMOTES_KNIGHT:
            pos->knights |= board(to); break;
        }
        pos->rule50 = 0;
    }
    else if ((board(from) & pos0->pawns) != 0)
    {
        pos->rule50 = 0;                // Pawn move
        if (rank(from) == 1 && rank(to) == 3 &&
            (pawn_attacks(from + 8, true) & pos0->pawns & pos0->black) != 0)
            pos->ep = from + 8;
        else if (rank(from) == 6 && rank(to) == 4 &&
            (pawn_attacks(from - 8, false) & pos0->pawns & pos0->white) != 0)
            pos->ep = from - 8;
        else if (to == pos0->ep)
        {
            unsigned ep_to = (pos0->turn ? to - 8 : to + 8);
            uint64_t ep_mask = ~board(ep_to);
            pos->white &= ep_mask;
            pos->black &= ep_mask;
            pos->pawns &= ep_mask;
        }
    }
    else if ((board(to) & (pos0->white | pos0->black)) != 0)
        pos->rule50 = 0;                // Capture
    else
        pos->rule50 = pos0->rule50 + 1; // Normal move
    if (!is_legal(pos))
        return false;
    return true;
}

static bool legal_move(const Pos* pos, TbMove move) {
    struct Pos pos1;
    return do_move(&pos1, pos, move);
}

/*
 * Test if the king is in checkmate.
 */
static bool is_mate(const Pos* pos)
{
    if (!is_check(pos))
        return false;
    uint16_t moves0[MAX_MOVES];
    uint16_t* moves = moves0;
    uint16_t* end = gen_moves(pos, moves);
    for (; moves < end; moves++)
    {
        Pos pos1;
        if (do_move(&pos1, pos, *moves))
            return false;
    }
    return true;
}

/*
 * Generate all legal moves.
 */
static TbMove* gen_legal(const Pos* pos, TbMove* moves)
{
    TbMove pl_moves[TB_MAX_MOVES];
    TbMove* end = gen_moves(pos, pl_moves);
    TbMove* results = moves;
    for (TbMove* m = pl_moves; m < end; m++) {
        if (legal_move(pos, *m)) {
            *results++ = *m;
        }
    }
    return results;
}

#ifdef __cplusplus
};
#endif

struct PairsData {
  uint8_t *indexTable;
  uint16_t *sizeTable;
  uint8_t *data;
  uint16_t *offset;
  uint8_t *symLen;
  uint8_t *symPat;
  uint8_t blockSize;
  uint8_t idxBits;
  uint8_t minLen;
  uint8_t constValue[2];
  uint64_t base[1];
};

struct EncInfo {
  struct PairsData *precomp;
  size_t factor[TB_PIECES];
  uint8_t pieces[TB_PIECES];
  uint8_t norm[TB_PIECES];
};

struct BaseEntry {
  uint64_t key;
  uint8_t *data[3];
  map_t mapping[3];
#ifdef __cplusplus
  atomic<bool> ready[3];
#else
  atomic_bool ready[3];
#endif
  uint8_t num;
  bool symmetric, hasPawns, hasDtm, hasDtz;
  union {
    bool kk_enc;
    uint8_t pawns[2];
  };
  bool dtmLossOnly;
};

struct PieceEntry {
  struct BaseEntry be;
  struct EncInfo ei[5]; // 2 + 2 + 1
  uint16_t *dtmMap;
  uint16_t dtmMapIdx[2][2];
  void *dtzMap;
  uint16_t dtzMapIdx[4];
  uint8_t dtzFlags;
};

struct PawnEntry {
  struct BaseEntry be;
  struct EncInfo ei[24]; // 4 * 2 + 6 * 2 + 4
  uint16_t *dtmMap;
  uint16_t dtmMapIdx[6][2][2];
  void *dtzMap;
  uint16_t dtzMapIdx[4][4];
  uint8_t dtzFlags[4];
  bool dtmSwitched;
};

struct TbHashEntry {
  uint64_t key;
  struct BaseEntry *ptr;
};

static int tbNumPiece, tbNumPawn;
static int numWdl, numDtm, numDtz;

static struct PieceEntry *pieceEntry;
static struct PawnEntry *pawnEntry;
static struct TbHashEntry tbHash[1 << TB_HASHBITS];

static void init_indices(void);

// Forward declarations. These functions without the tb_
// prefix take a pos structure as input.
static int probe_wdl(Pos *pos, int *success);
static int probe_dtz(Pos *pos, int *success);
static int root_probe_wdl(const Pos *pos, bool useRule50, struct TbRootMoves *rm);
static int root_probe_dtz(const Pos *pos, bool hasRepeated, bool useRule50, struct TbRootMoves *rm);
static uint16_t probe_root(Pos *pos, int *score, unsigned *results);

unsigned tb_probe_wdl_impl(
    uint64_t white,
    uint64_t black,
    uint64_t kings,
    uint64_t queens,
    uint64_t rooks,
    uint64_t bishops,
    uint64_t knights,
    uint64_t pawns,
    unsigned ep,
    bool turn)
{
    Pos pos =
    {
        white,
        black,
        kings,
        queens,
        rooks,
        bishops,
        knights,
        pawns,
        0,
        (uint8_t)ep,
        turn
    };
    int success;
    int v = probe_wdl(&pos, &success);
    if (success == 0)
        return TB_RESULT_FAILED;
    return (unsigned)(v + 2);
}

static unsigned dtz_to_wdl(int cnt50, int dtz)
{
    int wdl = 0;
    if (dtz > 0)
        wdl = (dtz + cnt50 <= 100? 2: 1);
    else if (dtz < 0)
        wdl = (-dtz + cnt50 <= 100? -2: -1);
    return wdl + 2;
}

unsigned tb_probe_root_impl(
    uint64_t white,
    uint64_t black,
    uint64_t kings,
    uint64_t queens,
    uint64_t rooks,
    uint64_t bishops,
    uint64_t knights,
    uint64_t pawns,
    unsigned rule50,
    unsigned ep,
    bool turn,
    unsigned *results)
{
    Pos pos =
    {
        white,
        black,
        kings,
        queens,
        rooks,
        bishops,
        knights,
        pawns,
        (uint8_t)rule50,
        (uint8_t)ep,
        turn
    };
    int dtz;
    if (!is_valid(&pos))
        return TB_RESULT_FAILED;
    TbMove move = probe_root(&pos, &dtz, results);
    if (move == 0)
        return TB_RESULT_FAILED;
    if (move == MOVE_CHECKMATE)
        return TB_RESULT_CHECKMATE;
    if (move == MOVE_STALEMATE)
        return TB_RESULT_STALEMATE;
    unsigned res = 0;
    res = TB_SET_WDL(res, dtz_to_wdl(rule50, dtz));
    res = TB_SET_DTZ(res, (dtz < 0? -dtz: dtz));
    res = TB_SET_FROM(res, move_from(move));
    res = TB_SET_TO(res, move_to(move));
    res = TB_SET_PROMOTES(res, move_promotes(move));
    res = TB_SET_EP(res, is_en_passant(&pos, move));
    return res;
}

int tb_probe_root_dtz(
    uint64_t white,
    uint64_t black,
    uint64_t kings,
    uint64_t queens,
    uint64_t rooks,
    uint64_t bishops,
    uint64_t knights,
    uint64_t pawns,
    unsigned rule50,
    unsigned castling,
    unsigned ep,
    bool     turn,
    bool     hasRepeated,
    bool     useRule50,
    struct TbRootMoves *results) {
    Pos pos =
    {
        white,
        black,
        kings,
        queens,
        rooks,
        bishops,
        knights,
        pawns,
        (uint8_t)rule50,
        (uint8_t)ep,
        turn
    };
    if (castling != 0) return 0;
    return root_probe_dtz(&pos, hasRepeated, useRule50, results);
}

int tb_probe_root_wdl(
    uint64_t white,
    uint64_t black,
    uint64_t kings,
    uint64_t queens,
    uint64_t rooks,
    uint64_t bishops,
    uint64_t knights,
    uint64_t pawns,
    unsigned rule50,
    unsigned castling,
    unsigned ep,
    bool     turn,
    bool     useRule50,
    struct TbRootMoves *results) {
    Pos pos =
    {
        white,
        black,
        kings,
        queens,
        rooks,
        bishops,
        knights,
        pawns,
        (uint8_t)rule50,
        (uint8_t)ep,
        turn
    };
    if (castling != 0) return 0;
    return root_probe_wdl(&pos, useRule50, results);
}

// Given a position, produce a text string of the form KQPvKRP, where
// "KQP" represents the white pieces if flip == false and the black pieces
// if flip == true.
static void prt_str(const Pos *pos, char *str, bool flip)
{
  int color = flip ? TB_BLACK : TB_WHITE;

  for (int pt = TB_ENUM_KING; pt >= TB_ENUM_PAWN; pt--)
    for (int i = popcount(pieces_by_type(pos, (Color)color, (PieceType)pt)); i > 0; i--)
      *str++ = piece_to_char[pt];
  *str++ = 'v';
  color ^= 1;
  for (int pt = TB_ENUM_KING; pt >= TB_ENUM_PAWN; pt--)
    for (int i = popcount(pieces_by_type(pos, (Color)color, (PieceType)pt)); i > 0; i--)
      *str++ = piece_to_char[pt];
  *str++ = 0;
}

static bool test_tb(const char *str, const char *suffix)
{
  FD fd = open_tb(str, suffix);
  if (fd != FD_ERR) {
    size_t size = file_size(fd);
    close_tb(fd);
    if ((size & 63) != 16) {
      fprintf(stderr, "Incomplete tablebase file %s.%s\n", str, suffix);
      printf("info string Incomplete tablebase file %s.%s\n", str, suffix);
      fd = FD_ERR;
    }
  }
  return fd != FD_ERR;
}

static void *map_tb(const char *name, const char *suffix, map_t *mapping)
{
  FD fd = open_tb(name, suffix);
  if (fd == FD_ERR)
    return NULL;

  void *data = map_file(fd, mapping);
  if (data == NULL) {
    fprintf(stderr, "Could not map %s%s into memory.\n", name, suffix);
    exit(EXIT_FAILURE);
  }

  close_tb(fd);

  return data;
}

static void add_to_hash(struct BaseEntry *ptr, uint64_t key)
{
  int idx;

  idx = key >> (64 - TB_HASHBITS);
  while (tbHash[idx].ptr)
    idx = (idx + 1) & ((1 << TB_HASHBITS) - 1);

  tbHash[idx].key = key;
  tbHash[idx].ptr = ptr;
}

#define pchr(i) piece_to_char[TB_ENUM_QUEEN - (i)]
#define Swap(a,b) {int tmp=a;a=b;b=tmp;}

static void init_tb(char *str)
{
  if (!test_tb(str, tbSuffix[WDL]))
    return;

  int pcs[16];
  for (int i = 0; i < 16; i++)
    pcs[i] = 0;
  int color = 0;
  for (char *s = str; *s; s++)
    if (*s == 'v')
      color = 8;
    else {
      int piece_type = char_to_piece_type(*s);
      if (piece_type) {
        assert((piece_type | color) < 16);
        pcs[piece_type | color]++;
      }
    }

  uint64_t key = calc_key_from_pcs(pcs, false);
  uint64_t key2 = calc_key_from_pcs(pcs, true);

  bool hasPawns = pcs[W_PAWN] || pcs[B_PAWN];

  struct BaseEntry *be = hasPawns ? &pawnEntry[tbNumPawn++].be
                                  : &pieceEntry[tbNumPiece++].be;
  be->hasPawns = hasPawns;
  be->key = key;
  be->symmetric = key == key2;
  be->num = 0;
  for (int i = 0; i < 16; i++)
    be->num += pcs[i];

  numWdl++;
  numDtm += be->hasDtm = test_tb(str, tbSuffix[DTM]);
  numDtz += be->hasDtz = test_tb(str, tbSuffix[DTZ]);

  if (be->num > TB_MaxCardinality) {
    TB_MaxCardinality = be->num;
  }
  if (be->hasDtm)
    if (be->num > TB_MaxCardinalityDTM) {
      TB_MaxCardinalityDTM = be->num;
    }

  for (int type = 0; type < 3; type++)
    atomic_init(&be->ready[type], false);

  if (!be->hasPawns) {
    int j = 0;
    for (int i = 0; i < 16; i++)
      if (pcs[i] == 1) j++;
    be->kk_enc = j == 2;
  } else {
    be->pawns[0] = pcs[W_PAWN];
    be->pawns[1] = pcs[B_PAWN];
    if (pcs[B_PAWN] && (!pcs[W_PAWN] || pcs[W_PAWN] > pcs[B_PAWN]))
      Swap(be->pawns[0], be->pawns[1]);
  }

  add_to_hash(be, key);
  if (key != key2)
    add_to_hash(be, key2);
}

#define PIECE(x) ((struct PieceEntry *)(x))
#define PAWN(x) ((struct PawnEntry *)(x))

int num_tables(struct BaseEntry *be, const int type)
{
  return be->hasPawns ? type == DTM ? 6 : 4 : 1;
}

struct EncInfo *first_ei(struct BaseEntry *be, const int type)
{
  return  be->hasPawns
        ? &PAWN(be)->ei[type == WDL ? 0 : type == DTM ? 8 : 20]
        : &PIECE(be)->ei[type == WDL ? 0 : type == DTM ? 2 : 4];
}

static void free_tb_entry(struct BaseEntry *be)
{
  for (int type = 0; type < 3; type++) {
    if (atomic_load_explicit(&be->ready[type], memory_order_relaxed)) {
      unmap_file((void*)(be->data[type]), be->mapping[type]);
      int num = num_tables(be, type);
      struct EncInfo *ei = first_ei(be, type);
      for (int t = 0; t < num; t++) {
        free(ei[t].precomp);
        if (type != DTZ)
          free(ei[num + t].precomp);
      }
      atomic_store_explicit(&be->ready[type], false, memory_order_relaxed);
    }
  }
}

bool tb_init(const char *path)
{
  if (!initialized) {
    init_indices();
    king_attacks_init();
    knight_attacks_init();
    bishop_attacks_init();
    rook_attacks_init();
    pawn_attacks_init();
    initialized = 1;
  }

  // if pathString is set, we need to clean up first.
  if (pathString) {
    free(pathString);
    free(paths);

    for (int i = 0; i < tbNumPiece; i++)
      free_tb_entry((struct BaseEntry *)&pieceEntry[i]);
    for (int i = 0; i < tbNumPawn; i++)
      free_tb_entry((struct BaseEntry *)&pawnEntry[i]);

    LOCK_DESTROY(tbMutex);

    pathString = NULL;
    numWdl = numDtm = numDtz = 0;
  }

  // if path is an empty string or equals "<empty>", we are done.
  const char *p = path;
  if (strlen(p) == 0 || !strcmp(p, "<empty>")) return true;

  pathString = (char*)malloc(strlen(p) + 1);
  strcpy(pathString, p);
  numPaths = 0;
  for (int i = 0;; i++) {
    if (pathString[i] != SEP_CHAR)
      numPaths++;
    while (pathString[i] && pathString[i] != SEP_CHAR)
      i++;
    if (!pathString[i]) break;
    pathString[i] = 0;
  }
  paths = (char**)malloc(numPaths * sizeof(*paths));
  for (int i = 0, j = 0; i < numPaths; i++) {
    while (!pathString[j]) j++;
    paths[i] = &pathString[j];
    while (pathString[j]) j++;
  }

  LOCK_INIT(tbMutex);

  tbNumPiece = tbNumPawn = 0;
  TB_MaxCardinality = TB_MaxCardinalityDTM = 0;
  TB_LARGEST = 0;

  if (!pieceEntry) {
    pieceEntry = (struct PieceEntry*)malloc(TB_MAX_PIECE * sizeof(*pieceEntry));
    pawnEntry = (struct PawnEntry*)malloc(TB_MAX_PAWN * sizeof(*pawnEntry));
    if (!pieceEntry || !pawnEntry) {
      fprintf(stderr, "Out of memory.\n");
      exit(EXIT_FAILURE);
    }
  }

  for (int i = 0; i < (1 << TB_HASHBITS); i++) {
    tbHash[i].key = 0;
    tbHash[i].ptr = NULL;
  }

  char str[16];
  int i, j, k, l, m;

  for (i = 0; i < 5; i++) {
    sprintf(str, "K%cvK", pchr(i));
    init_tb(str);
  }

  for (i = 0; i < 5; i++)
    for (j = i; j < 5; j++) {
      sprintf(str, "K%cvK%c", pchr(i), pchr(j));
      init_tb(str);
    }

  for (i = 0; i < 5; i++)
    for (j = i; j < 5; j++) {
      sprintf(str, "K%c%cvK", pchr(i), pchr(j));
      init_tb(str);
    }

  for (i = 0; i < 5; i++)
    for (j = i; j < 5; j++)
      for (k = 0; k < 5; k++) {
        sprintf(str, "K%c%cvK%c", pchr(i), pchr(j), pchr(k));
        init_tb(str);
      }

  for (i = 0; i < 5; i++)
    for (j = i; j < 5; j++)
      for (k = j; k < 5; k++) {
        sprintf(str, "K%c%c%cvK", pchr(i), pchr(j), pchr(k));
        init_tb(str);
      }

  // 6- and 7-piece TBs make sense only with a 64-bit address space
  if (sizeof(size_t) < 8 || TB_PIECES < 6)
    goto finished;

  for (i = 0; i < 5; i++)
    for (j = i; j < 5; j++)
      for (k = i; k < 5; k++)
        for (l = (i == k) ? j : k; l < 5; l++) {
          sprintf(str, "K%c%cvK%c%c", pchr(i), pchr(j), pchr(k), pchr(l));
          init_tb(str);
        }

  for (i = 0; i < 5; i++)
    for (j = i; j < 5; j++)
      for (k = j; k < 5; k++)
        for (l = 0; l < 5; l++) {
          sprintf(str, "K%c%c%cvK%c", pchr(i), pchr(j), pchr(k), pchr(l));
          init_tb(str);
        }

  for (i = 0; i < 5; i++)
    for (j = i; j < 5; j++)
      for (k = j; k < 5; k++)
        for (l = k; l < 5; l++) {
          sprintf(str, "K%c%c%c%cvK", pchr(i), pchr(j), pchr(k), pchr(l));
          init_tb(str);
        }

  if (TB_PIECES < 7)
    goto finished;

  for (i = 0; i < 5; i++)
    for (j = i; j < 5; j++)
      for (k = j; k < 5; k++)
        for (l = k; l < 5; l++)
          for (m = l; m < 5; m++) {
            sprintf(str, "K%c%c%c%c%cvK", pchr(i), pchr(j), pchr(k), pchr(l), pchr(m));
            init_tb(str);
          }

  for (i = 0; i < 5; i++)
    for (j = i; j < 5; j++)
      for (k = j; k < 5; k++)
        for (l = k; l < 5; l++)
          for (m = 0; m < 5; m++) {
            sprintf(str, "K%c%c%c%cvK%c", pchr(i), pchr(j), pchr(k), pchr(l), pchr(m));
            init_tb(str);
          }

  for (i = 0; i < 5; i++)
    for (j = i; j < 5; j++)
      for (k = j; k < 5; k++)
        for (l = 0; l < 5; l++)
          for (m = l; m < 5; m++) {
            sprintf(str, "K%c%c%cvK%c%c", pchr(i), pchr(j), pchr(k), pchr(l), pchr(m));
            init_tb(str);
          }

finished:
  printf("info string Found %d WDL, %d DTM and %d DTZ tablebase files.\n",
      numWdl, numDtm, numDtz);
  fflush(stdout);
  
  // Set TB_LARGEST, for backward compatibility with pre-7-man Fathom
  TB_LARGEST = (unsigned)TB_MaxCardinality;
  if ((unsigned)TB_MaxCardinalityDTM > TB_LARGEST) {
    TB_LARGEST = TB_MaxCardinalityDTM;
  }
  return true;
}

void tb_free(void)
{
  tb_init("");
  free(pieceEntry);
  free(pawnEntry);
}

static const int8_t OffDiag[] = {
  0,-1,-1,-1,-1,-1,-1,-1,
  1, 0,-1,-1,-1,-1,-1,-1,
  1, 1, 0,-1,-1,-1,-1,-1,
  1, 1, 1, 0,-1,-1,-1,-1,
  1, 1, 1, 1, 0,-1,-1,-1,
  1, 1, 1, 1, 1, 0,-1,-1,
  1, 1, 1, 1, 1, 1, 0,-1,
  1, 1, 1, 1, 1, 1, 1, 0
};

static const uint8_t Triangle[] = {
  6, 0, 1, 2, 2, 1, 0, 6,
  0, 7, 3, 4, 4, 3, 7, 0,
  1, 3, 8, 5, 5, 8, 3, 1,
  2, 4, 5, 9, 9, 5, 4, 2,
  2, 4, 5, 9, 9, 5, 4, 2,
  1, 3, 8, 5, 5, 8, 3, 1,
  0, 7, 3, 4, 4, 3, 7, 0,
  6, 0, 1, 2, 2, 1, 0, 6
};

static const uint8_t FlipDiag[] = {
   0,  8, 16, 24, 32, 40, 48, 56,
   1,  9, 17, 25, 33, 41, 49, 57,
   2, 10, 18, 26, 34, 42, 50, 58,
   3, 11, 19, 27, 35, 43, 51, 59,
   4, 12, 20, 28, 36, 44, 52, 60,
   5, 13, 21, 29, 37, 45, 53, 61,
   6, 14, 22, 30, 38, 46, 54, 62,
   7, 15, 23, 31, 39, 47, 55, 63
};

static const uint8_t Lower[] = {
  28,  0,  1,  2,  3,  4,  5,  6,
   0, 29,  7,  8,  9, 10, 11, 12,
   1,  7, 30, 13, 14, 15, 16, 17,
   2,  8, 13, 31, 18, 19, 20, 21,
   3,  9, 14, 18, 32, 22, 23, 24,
   4, 10, 15, 19, 22, 33, 25, 26,
   5, 11, 16, 20, 23, 25, 34, 27,
   6, 12, 17, 21, 24, 26, 27, 35
};

static const uint8_t Diag[] = {
   0,  0,  0,  0,  0,  0,  0,  8,
   0,  1,  0,  0,  0,  0,  9,  0,
   0,  0,  2,  0,  0, 10,  0,  0,
   0,  0,  0,  3, 11,  0,  0,  0,
   0,  0,  0, 12,  4,  0,  0,  0,
   0,  0, 13,  0,  0,  5,  0,  0,
   0, 14,  0,  0,  0,  0,  6,  0,
  15,  0,  0,  0,  0,  0,  0,  7
};

static const uint8_t Flap[2][64] = {
  {  0,  0,  0,  0,  0,  0,  0,  0,
     0,  6, 12, 18, 18, 12,  6,  0,
     1,  7, 13, 19, 19, 13,  7,  1,
     2,  8, 14, 20, 20, 14,  8,  2,
     3,  9, 15, 21, 21, 15,  9,  3,
     4, 10, 16, 22, 22, 16, 10,  4,
     5, 11, 17, 23, 23, 17, 11,  5,
     0,  0,  0,  0,  0,  0,  0,  0  },
  {  0,  0,  0,  0,  0,  0,  0,  0,
     0,  1,  2,  3,  3,  2,  1,  0,
     4,  5,  6,  7,  7,  6,  5,  4,
     8,  9, 10, 11, 11, 10,  9,  8,
    12, 13, 14, 15, 15, 14, 13, 12,
    16, 17, 18, 19, 19, 18, 17, 16,
    20, 21, 22, 23, 23, 22, 21, 20,
     0,  0,  0,  0,  0,  0,  0,  0  }
};

static const uint8_t PawnTwist[2][64] = {
  {  0,  0,  0,  0,  0,  0,  0,  0,
    47, 35, 23, 11, 10, 22, 34, 46,
    45, 33, 21,  9,  8, 20, 32, 44,
    43, 31, 19,  7,  6, 18, 30, 42,
    41, 29, 17,  5,  4, 16, 28, 40,
    39, 27, 15,  3,  2, 14, 26, 38,
    37, 25, 13,  1,  0, 12, 24, 36,
     0,  0,  0,  0,  0,  0,  0,  0 },
  {  0,  0,  0,  0,  0,  0,  0,  0,
    47, 45, 43, 41, 40, 42, 44, 46,
    39, 37, 35, 33, 32, 34, 36, 38,
    31, 29, 27, 25, 24, 26, 28, 30,
    23, 21, 19, 17, 16, 18, 20, 22,
    15, 13, 11,  9,  8, 10, 12, 14,
     7,  5,  3,  1,  0,  2,  4,  6,
     0,  0,  0,  0,  0,  0,  0,  0 }
};

static const int16_t KKIdx[10][64] = {
  { -1, -1, -1,  0,  1,  2,  3,  4,
    -1, -1, -1,  5,  6,  7,  8,  9,
    10, 11, 12, 13, 14, 15, 16, 17,
    18, 19, 20, 21, 22, 23, 24, 25,
    26, 27, 28, 29, 30, 31, 32, 33,
    34, 35, 36, 37, 38, 39, 40, 41,
    42, 43, 44, 45, 46, 47, 48, 49,
    50, 51, 52, 53, 54, 55, 56, 57 },
  { 58, -1, -1, -1, 59, 60, 61, 62,
    63, -1, -1, -1, 64, 65, 66, 67,
    68, 69, 70, 71, 72, 73, 74, 75,
    76, 77, 78, 79, 80, 81, 82, 83,
    84, 85, 86, 87, 88, 89, 90, 91,
    92, 93, 94, 95, 96, 97, 98, 99,
   100,101,102,103,104,105,106,107,
   108,109,110,111,112,113,114,115},
  {116,117, -1, -1, -1,118,119,120,
   121,122, -1, -1, -1,123,124,125,
   126,127,128,129,130,131,132,133,
   134,135,136,137,138,139,140,141,
   142,143,144,145,146,147,148,149,
   150,151,152,153,154,155,156,157,
   158,159,160,161,162,163,164,165,
   166,167,168,169,170,171,172,173 },
  {174, -1, -1, -1,175,176,177,178,
   179, -1, -1, -1,180,181,182,183,
   184, -1, -1, -1,185,186,187,188,
   189,190,191,192,193,194,195,196,
   197,198,199,200,201,202,203,204,
   205,206,207,208,209,210,211,212,
   213,214,215,216,217,218,219,220,
   221,222,223,224,225,226,227,228 },
  {229,230, -1, -1, -1,231,232,233,
   234,235, -1, -1, -1,236,237,238,
   239,240, -1, -1, -1,241,242,243,
   244,245,246,247,248,249,250,251,
   252,253,254,255,256,257,258,259,
   260,261,262,263,264,265,266,267,
   268,269,270,271,272,273,274,275,
   276,277,278,279,280,281,282,283 },
  {284,285,286,287,288,289,290,291,
   292,293, -1, -1, -1,294,295,296,
   297,298, -1, -1, -1,299,300,301,
   302,303, -1, -1, -1,304,305,306,
   307,308,309,310,311,312,313,314,
   315,316,317,318,319,320,321,322,
   323,324,325,326,327,328,329,330,
   331,332,333,334,335,336,337,338 },
  { -1, -1,339,340,341,342,343,344,
    -1, -1,345,346,347,348,349,350,
    -1, -1,441,351,352,353,354,355,
    -1, -1, -1,442,356,357,358,359,
    -1, -1, -1, -1,443,360,361,362,
    -1, -1, -1, -1, -1,444,363,364,
    -1, -1, -1, -1, -1, -1,445,365,
    -1, -1, -1, -1, -1, -1, -1,446 },
  { -1, -1, -1,366,367,368,369,370,
    -1, -1, -1,371,372,373,374,375,
    -1, -1, -1,376,377,378,379,380,
    -1, -1, -1,447,381,382,383,384,
    -1, -1, -1, -1,448,385,386,387,
    -1, -1, -1, -1, -1,449,388,389,
    -1, -1, -1, -1, -1, -1,450,390,
    -1, -1, -1, -1, -1, -1, -1,451 },
  {452,391,392,393,394,395,396,397,
    -1, -1, -1, -1,398,399,400,401,
    -1, -1, -1, -1,402,403,404,405,
    -1, -1, -1, -1,406,407,408,409,
    -1, -1, -1, -1,453,410,411,412,
    -1, -1, -1, -1, -1,454,413,414,
    -1, -1, -1, -1, -1, -1,455,415,
    -1, -1, -1, -1, -1, -1, -1,456 },
  {457,416,417,418,419,420,421,422,
    -1,458,423,424,425,426,427,428,
    -1, -1, -1, -1, -1,429,430,431,
    -1, -1, -1, -1, -1,432,433,434,
    -1, -1, -1, -1, -1,435,436,437,
    -1, -1, -1, -1, -1,459,438,439,
    -1, -1, -1, -1, -1, -1,460,440,
    -1, -1, -1, -1, -1, -1, -1,461 }
};

static const uint8_t FileToFile[] = { 0, 1, 2, 3, 3, 2, 1, 0 };
static const int WdlToMap[5] = { 1, 3, 0, 2, 0 };
static const uint8_t PAFlags[5] = { 8, 0, 0, 0, 4 };

static size_t Binomial[7][64];
static size_t PawnIdx[2][6][24];
static size_t PawnFactorFile[6][4];
static size_t PawnFactorRank[6][6];

static void init_indices(void)
{
  int i, j, k;

  // Binomial[k][n] = Bin(n, k)
  for (i = 0; i < 7; i++)
    for (j = 0; j < 64; j++) {
      size_t f = 1;
      size_t l = 1;
      for (k = 0; k < i; k++) {
        f *= (j - k);
        l *= (k + 1);
      }
      Binomial[i][j] = f / l;
    }

  for (i = 0; i < 6; i++) {
    size_t s = 0;
    for (j = 0; j < 24; j++) {
      PawnIdx[0][i][j] = s;
      s += Binomial[i][PawnTwist[0][(1 + (j % 6)) * 8 + (j / 6)]];
      if ((j + 1) % 6 == 0) {
        PawnFactorFile[i][j / 6] = s;
        s = 0;
      }
    }
  }

  for (i = 0; i < 6; i++) {
    size_t s = 0;
    for (j = 0; j < 24; j++) {
      PawnIdx[1][i][j] = s;
      s += Binomial[i][PawnTwist[1][(1 + (j / 4)) * 8 + (j % 4)]];
      if ((j + 1) % 4 == 0) {
        PawnFactorRank[i][j / 4] = s;
        s = 0;
      }
    }
  }
}

int leading_pawn(int *p, struct BaseEntry *be, const int enc)
{
  for (int i = 1; i < be->pawns[0]; i++)
    if (Flap[enc-1][p[0]] > Flap[enc-1][p[i]])
      Swap(p[0], p[i]);

  return enc == FILE_ENC ? FileToFile[p[0] & 7] : (p[0] - 8) >> 3;
}

size_t encode(int *p, struct EncInfo *ei, struct BaseEntry *be,
    const int enc)
{
  int n = be->num;
  size_t idx;
  int k;

  if (p[0] & 0x04)
    for (int i = 0; i < n; i++)
      p[i] ^= 0x07;

  if (enc == PIECE_ENC) {
    if (p[0] & 0x20)
      for (int i = 0; i < n; i++)
        p[i] ^= 0x38;

    for (int i = 0; i < n; i++)
      if (OffDiag[p[i]]) {
        if (OffDiag[p[i]] > 0 && i < (be->kk_enc ? 2 : 3))
          for (int j = 0; j < n; j++)
            p[j] = FlipDiag[p[j]];
        break;
      }

    if (be->kk_enc) {
      idx = KKIdx[Triangle[p[0]]][p[1]];
      k = 2;
    } else {
      int s1 = (p[1] > p[0]);
      int s2 = (p[2] > p[0]) + (p[2] > p[1]);

      if (OffDiag[p[0]])
        idx = Triangle[p[0]] * 63*62 + (p[1] - s1) * 62 + (p[2] - s2);
      else if (OffDiag[p[1]])
        idx = 6*63*62 + Diag[p[0]] * 28*62 + Lower[p[1]] * 62 + p[2] - s2;
      else if (OffDiag[p[2]])
        idx = 6*63*62 + 4*28*62 + Diag[p[0]] * 7*28 + (Diag[p[1]] - s1) * 28 + Lower[p[2]];
      else
        idx = 6*63*62 + 4*28*62 + 4*7*28 + Diag[p[0]] * 7*6 + (Diag[p[1]] - s1) * 6 + (Diag[p[2]] - s2);
      k = 3;
    }
    idx *= ei->factor[0];
  } else {
    for (int i = 1; i < be->pawns[0]; i++)
      for (int j = i + 1; j < be->pawns[0]; j++)
        if (PawnTwist[enc-1][p[i]] < PawnTwist[enc-1][p[j]])
          Swap(p[i], p[j]);

    k = be->pawns[0];
    idx = PawnIdx[enc-1][k-1][Flap[enc-1][p[0]]];
    for (int i = 1; i < k; i++)
      idx += Binomial[k-i][PawnTwist[enc-1][p[i]]];
    idx *= ei->factor[0];

    // Pawns of other color
    if (be->pawns[1]) {
      int t = k + be->pawns[1];
      for (int i = k; i < t; i++)
        for (int j = i + 1; j < t; j++)
          if (p[i] > p[j]) Swap(p[i], p[j]);
      size_t s = 0;
      for (int i = k; i < t; i++) {
        int sq = p[i];
        int skips = 0;
        for (int j = 0; j < k; j++)
          skips += (sq > p[j]);
        s += Binomial[i - k + 1][sq - skips - 8];
      }
      idx += s * ei->factor[k];
      k = t;
    }
  }

  for (; k < n;) {
    int t = k + ei->norm[k];
    for (int i = k; i < t; i++)
      for (int j = i + 1; j < t; j++)
        if (p[i] > p[j]) Swap(p[i], p[j]);
    size_t s = 0;
    for (int i = k; i < t; i++) {
      int sq = p[i];
      int skips = 0;
      for (int j = 0; j < k; j++)
        skips += (sq > p[j]);
      s += Binomial[i - k + 1][sq - skips];
    }
    idx += s * ei->factor[k];
    k = t;
  }

  return idx;
}

static size_t encode_piece(int *p, struct EncInfo *ei, struct BaseEntry *be)
{
  return encode(p, ei, be, PIECE_ENC);
}

static size_t encode_pawn_f(int *p, struct EncInfo *ei, struct BaseEntry *be)
{
  return encode(p, ei, be, FILE_ENC);
}

static size_t encode_pawn_r(int *p, struct EncInfo *ei, struct BaseEntry *be)
{
  return encode(p, ei, be, RANK_ENC);
}

// Count number of placements of k like pieces on n squares
static size_t subfactor(size_t k, size_t n)
{
  size_t f = n;
  size_t l = 1;
  for (size_t i = 1; i < k; i++) {
    f *= n - i;
    l *= i + 1;
  }

  return f / l;
}

static size_t init_enc_info(struct EncInfo *ei, struct BaseEntry *be,
    uint8_t *tb, int shift, int t, const int enc)
{
  bool morePawns = enc != PIECE_ENC && be->pawns[1] > 0;

  for (int i = 0; i < be->num; i++) {
    ei->pieces[i] = (tb[i + 1 + morePawns] >> shift) & 0x0f;
    ei->norm[i] = 0;
  }

  int order = (tb[0] >> shift) & 0x0f;
  int order2 = morePawns ? (tb[1] >> shift) & 0x0f : 0x0f;

  int k = ei->norm[0] =  enc != PIECE_ENC ? be->pawns[0]
                       : be->kk_enc ? 2 : 3;

  if (morePawns) {
    ei->norm[k] = be->pawns[1];
    k += ei->norm[k];
  }

  for (int i = k; i < be->num; i += ei->norm[i])
    for (int j = i; j < be->num && ei->pieces[j] == ei->pieces[i]; j++)
      ei->norm[i]++;

  int n = 64 - k;
  size_t f = 1;

  for (int i = 0; k < be->num || i == order || i == order2; i++) {
    if (i == order) {
      ei->factor[0] = f;
      f *=  enc == FILE_ENC ? PawnFactorFile[ei->norm[0] - 1][t]
          : enc == RANK_ENC ? PawnFactorRank[ei->norm[0] - 1][t]
          : be->kk_enc ? 462 : 31332;
    } else if (i == order2) {
      ei->factor[ei->norm[0]] = f;
      f *= subfactor(ei->norm[ei->norm[0]], 48 - ei->norm[0]);
    } else {
      ei->factor[k] = f;
      f *= subfactor(ei->norm[k], n);
      n -= ei->norm[k];
      k += ei->norm[k];
    }
  }

  return f;
}

static void calc_symLen(struct PairsData *d, uint32_t s, char *tmp)
{
  uint8_t *w = d->symPat + 3 * s;
  uint32_t s2 = (w[2] << 4) | (w[1] >> 4);
  if (s2 == 0x0fff)
    d->symLen[s] = 0;
  else {
    uint32_t s1 = ((w[1] & 0xf) << 8) | w[0];
    if (!tmp[s1]) calc_symLen(d, s1, tmp);
    if (!tmp[s2]) calc_symLen(d, s2, tmp);
    d->symLen[s] = d->symLen[s1] + d->symLen[s2] + 1;
  }
  tmp[s] = 1;
}

static struct PairsData *setup_pairs(uint8_t **ptr, size_t tb_size,
    size_t *size, uint8_t *flags, int type)
{
  struct PairsData *d;
  uint8_t *data = *ptr;

  *flags = data[0];
  if (data[0] & 0x80) {
    d = (struct PairsData*)malloc(sizeof(struct PairsData));
    d->idxBits = 0;
    d->constValue[0] = type == WDL ? data[1] : 0;
    d->constValue[1] = 0;
    *ptr = data + 2;
    size[0] = size[1] = size[2] = 0;
    return d;
  }

  uint8_t blockSize = data[1];
  uint8_t idxBits = data[2];
  uint32_t realNumBlocks = read_le_u32(data+4);
  uint32_t numBlocks = realNumBlocks + data[3];
  int maxLen = data[8];
  int minLen = data[9];
  int h = maxLen - minLen + 1;
  uint32_t numSyms = (uint32_t)read_le_u16(data + 10 + 2 * h);
  d = (struct PairsData*)malloc(sizeof(struct PairsData) + h * sizeof(uint64_t) + numSyms);
  d->blockSize = blockSize;
  d->idxBits = idxBits;
  d->offset = (uint16_t *)(&data[10]);
  d->symLen = (uint8_t *)d + sizeof(struct PairsData) + h * sizeof(uint64_t);
  d->symPat = &data[12 + 2 * h];
  d->minLen = minLen;
  *ptr = &data[12 + 2 * h + 3 * numSyms + (numSyms & 1)];

  size_t num_indices = (tb_size + (1ULL << idxBits) - 1) >> idxBits;
  size[0] = 6ULL * num_indices;
  size[1] = 2ULL * numBlocks;
  size[2] = (size_t)realNumBlocks << blockSize;

  assert(numSyms < TB_MAX_SYMS);
  char tmp[TB_MAX_SYMS];
  memset(tmp, 0, numSyms);
  for (uint32_t s = 0; s < numSyms; s++)
    if (!tmp[s])
      calc_symLen(d, s, tmp);

  d->base[h - 1] = 0;
  for (int i = h - 2; i >= 0; i--)
    d->base[i] = (d->base[i + 1] + read_le_u16((uint8_t *)(d->offset + i)) - read_le_u16((uint8_t *)(d->offset + i + 1))) / 2;
#ifdef DECOMP64
  for (int i = 0; i < h; i++)
    d->base[i] <<= 64 - (minLen + i);
#else
  for (int i = 0; i < h; i++)
    d->base[i] <<= 32 - (minLen + i);
#endif
  d->offset -= d->minLen;

  return d;
}

static bool init_table(struct BaseEntry *be, const char *str, int type)
{
  uint8_t *data = (uint8_t*)map_tb(str, tbSuffix[type], &be->mapping[type]);
  if (!data) return false;

  if (read_le_u32(data) != tbMagic[type]) {
    fprintf(stderr, "Corrupted table.\n");
    unmap_file((void*)data, be->mapping[type]);
    return false;
  }

  be->data[type] = data;

  bool split = type != DTZ && (data[4] & 0x01);
  if (type == DTM)
    be->dtmLossOnly = data[4] & 0x04;

  data += 5;

  size_t tb_size[6][2];
  int num = num_tables(be, type);
  struct EncInfo *ei = first_ei(be, type);
  int enc = !be->hasPawns ? PIECE_ENC : type != DTM ? FILE_ENC : RANK_ENC;

  for (int t = 0; t < num; t++) {
    tb_size[t][0] = init_enc_info(&ei[t], be, data, 0, t, enc);
    if (split)
      tb_size[t][1] = init_enc_info(&ei[num + t], be, data, 4, t, enc);
    data += be->num + 1 + (be->hasPawns && be->pawns[1]);
  }
  data += (uintptr_t)data & 1;

  size_t size[6][2][3];
  for (int t = 0; t < num; t++) {
    uint8_t flags;
    ei[t].precomp = setup_pairs(&data, tb_size[t][0], size[t][0], &flags, type);
    if (type == DTZ) {
      if (!be->hasPawns)
        PIECE(be)->dtzFlags = flags;
      else
        PAWN(be)->dtzFlags[t] = flags;
    }
    if (split)
      ei[num + t].precomp = setup_pairs(&data, tb_size[t][1], size[t][1], &flags, type);
    else if (type != DTZ)
      ei[num + t].precomp = NULL;
  }

  if (type == DTM && !be->dtmLossOnly) {
    uint16_t *map = (uint16_t *)data;
    *(be->hasPawns ? &PAWN(be)->dtmMap : &PIECE(be)->dtmMap) = map;
    uint16_t (*mapIdx)[2][2] = be->hasPawns ? &PAWN(be)->dtmMapIdx[0]
                                             : &PIECE(be)->dtmMapIdx;
    for (int t = 0; t < num; t++) {
      for (int i = 0; i < 2; i++) {
        mapIdx[t][0][i] = (uint16_t)(data + 1 - (uint8_t*)map);
        data += 2 + 2 * read_le_u16(data);
      }
      if (split) {
        for (int i = 0; i < 2; i++) {
          mapIdx[t][1][i] = (uint16_t)(data + 1 - (uint8_t*)map);
          data += 2 + 2 * read_le_u16(data);
        }
      }
    }
  }

  if (type == DTZ) {
    void *map = data;
    *(be->hasPawns ? &PAWN(be)->dtzMap : &PIECE(be)->dtzMap) = map;
    uint16_t (*mapIdx)[4] = be->hasPawns ? &PAWN(be)->dtzMapIdx[0]
                                          : &PIECE(be)->dtzMapIdx;
    uint8_t *flags = be->hasPawns ? &PAWN(be)->dtzFlags[0]
                                  : &PIECE(be)->dtzFlags;
    for (int t = 0; t < num; t++) {
      if (flags[t] & 2) {
        if (!(flags[t] & 16)) {
          for (int i = 0; i < 4; i++) {
            mapIdx[t][i] = (uint16_t)(data + 1 - (uint8_t *)map);
            data += 1 + data[0];
          }
        } else {
          data += (uintptr_t)data & 0x01;
          for (int i = 0; i < 4; i++) {
            mapIdx[t][i] = (uint16_t)(data + 1 - (uint8_t *)map);
            data += 2 + 2 * read_le_u16(data);
          }
        }
      }
    }
    data += (uintptr_t)data & 0x01;
  }

  for (int t = 0; t < num; t++) {
    ei[t].precomp->indexTable = data;
    data += size[t][0][0];
    if (split) {
      ei[num + t].precomp->indexTable = data;
      data += size[t][1][0];
    }
  }

  for (int t = 0; t < num; t++) {
    ei[t].precomp->sizeTable = (uint16_t *)data;
    data += size[t][0][1];
    if (split) {
      ei[num + t].precomp->sizeTable = (uint16_t *)data;
      data += size[t][1][1];
    }
  }

  for (int t = 0; t < num; t++) {
    data = (uint8_t *)(((uintptr_t)data + 0x3f) & ~0x3f);
    ei[t].precomp->data = data;
    data += size[t][0][2];
    if (split) {
      data = (uint8_t *)(((uintptr_t)data + 0x3f) & ~0x3f);
      ei[num + t].precomp->data = data;
      data += size[t][1][2];
    }
  }

  if (type == DTM && be->hasPawns)
    PAWN(be)->dtmSwitched =
      calc_key_from_pieces(ei[0].pieces, be->num) != be->key;

  return true;
}

static uint8_t *decompress_pairs(struct PairsData *d, size_t idx)
{
  if (!d->idxBits)
    return d->constValue;

  uint32_t mainIdx = (uint32_t)(idx >> d->idxBits);
  int litIdx = (idx & (((size_t)1 << d->idxBits) - 1)) - ((size_t)1 << (d->idxBits - 1));
  uint32_t block;
  memcpy(&block, d->indexTable + 6 * mainIdx, sizeof(block));
  block = from_le_u32(block);

  uint16_t idxOffset = *(uint16_t *)(d->indexTable + 6 * mainIdx + 4);
  litIdx += from_le_u16(idxOffset);

  if (litIdx < 0)
    while (litIdx < 0)
      litIdx += d->sizeTable[--block] + 1;
  else
    while (litIdx > d->sizeTable[block])
      litIdx -= d->sizeTable[block++] + 1;

  uint32_t *ptr = (uint32_t *)(d->data + ((size_t)block << d->blockSize));

  int m = d->minLen;
  uint16_t *offset = d->offset;
  uint64_t *base = d->base - m;
  uint8_t *symLen = d->symLen;
  uint32_t sym, bitCnt;

#ifdef DECOMP64
  uint64_t code = from_be_u64(*(uint64_t *)ptr);

  ptr += 2;
  bitCnt = 0; // number of "empty bits" in code
  for (;;) {
    int l = m;
    while (code < base[l]) l++;
    sym = from_le_u16(offset[l]);
    sym += (uint32_t)((code - base[l]) >> (64 - l));
    if (litIdx < (int)symLen[sym] + 1) break;
    litIdx -= (int)symLen[sym] + 1;
    code <<= l;
    bitCnt += l;
    if (bitCnt >= 32) {
      bitCnt -= 32;
      uint32_t tmp = from_be_u32(*ptr++);
      code |= (uint64_t)tmp << bitCnt;
    }
  }
#else
  uint32_t next = 0;
  uint32_t data = *ptr++;
  uint32_t code = from_be_u32(data);
  bitCnt = 0; // number of bits in next
  for (;;) {
    int l = m;
    while (code < base[l]) l++;
    sym = offset[l] + ((code - base[l]) >> (32 - l));
    if (litIdx < (int)symLen[sym] + 1) break;
    litIdx -= (int)symLen[sym] + 1;
    code <<= l;
    if (bitCnt < l) {
      if (bitCnt) {
	code |= (next >> (32 - l));
	l -= bitCnt;
      }
      data = *ptr++;
      next = from_be_u32(data);
      bitCnt = 32;
    }
    code |= (next >> (32 - l));
    next <<= l;
    bitCnt -= l;
  }
#endif
  uint8_t *symPat = d->symPat;
  while (symLen[sym] != 0) {
    uint8_t *w = symPat + (3 * sym);
    int s1 = ((w[1] & 0xf) << 8) | w[0];
    if (litIdx < (int)symLen[s1] + 1)
      sym = s1;
    else {
      litIdx -= (int)symLen[s1] + 1;
      sym = (w[2] << 4) | (w[1] >> 4);
    }
  }

  return &symPat[3 * sym];
}

// p[i] is to contain the square 0-63 (A1-H8) for a piece of type
// pc[i] ^ flip, where 1 = white pawn, ..., 14 = black king and pc ^ flip
// flips between white and black if flip == true.
// Pieces of the same type are guaranteed to be consecutive.
inline static int fill_squares(const Pos *pos, uint8_t *pc, bool flip, int mirror, int *p,
    int i)
{
  Color color = ColorOfPiece(pc[i]);
  if (flip) color = (Color)(!(int)color);
  uint64_t bb = pieces_by_type(pos, color, TypeOfPiece(pc[i]));
  unsigned sq;
  do {
    sq = lsb(bb);
    p[i++] = sq ^ mirror;
    bb = poplsb(bb);
  } while (bb);
  return i;
}

int probe_table(const Pos *pos, int s, int *success, const int type)
{
  // Obtain the position's material-signature key
  uint64_t key = calc_key(pos,false);

  // Test for KvK
  // Note: Cfish has key == 2ULL for KvK but we have 0
  if (type == WDL && key == 0ULL)
    return 0;

  int hashIdx = key >> (64 - TB_HASHBITS);
  while (tbHash[hashIdx].key && tbHash[hashIdx].key != key)
    hashIdx = (hashIdx + 1) & ((1 << TB_HASHBITS) - 1);
  if (!tbHash[hashIdx].ptr) {
    *success = 0;
    return 0;
  }

  struct BaseEntry *be = tbHash[hashIdx].ptr;
  if ((type == DTM && !be->hasDtm) || (type == DTZ && !be->hasDtz)) {
    *success = 0;
    return 0;
  }

  // Use double-checked locking to reduce locking overhead
  if (!atomic_load_explicit(&be->ready[type], memory_order_acquire)) {
    LOCK(tbMutex);
    if (!atomic_load_explicit(&be->ready[type], memory_order_relaxed)) {
      char str[16];
      prt_str(pos, str, be->key != key);
      if (!init_table(be, str, type)) {
        tbHash[hashIdx].ptr = NULL; // mark as deleted
        *success = 0;
        UNLOCK(tbMutex);
        return 0;
      }
      atomic_store_explicit(&be->ready[type], true, memory_order_release);
    }
    UNLOCK(tbMutex);
  }

  bool bside, flip;
  if (!be->symmetric) {
    flip = key != be->key;
    bside = (pos->turn == TB_WHITE) == flip;
    if (type == DTM && be->hasPawns && PAWN(be)->dtmSwitched) {
      flip = !flip;
      bside = !bside;
    }
  } else {
    flip = pos->turn != TB_WHITE;
    bside = false;
  }

  struct EncInfo *ei = first_ei(be, type);
  int p[TB_PIECES];
  size_t idx;
  int t = 0;
  uint8_t flags = 0; // initialize to fix GCC warning

  if (!be->hasPawns) {
    if (type == DTZ) {
      flags = PIECE(be)->dtzFlags;
      if ((flags & 1) != bside && !be->symmetric) {
        *success = -1;
        return 0;
      }
    }
    ei = type != DTZ ? &ei[bside] : ei;
    for (int i = 0; i < be->num;)
      i = fill_squares(pos, ei->pieces, flip, 0, p, i);
    idx = encode_piece(p, ei, be);
  } else {
    int i = fill_squares(pos, ei->pieces, flip, flip ? 0x38 : 0, p, 0);
    t = leading_pawn(p, be, type != DTM ? FILE_ENC : RANK_ENC);
    if (type == DTZ) {
      flags = PAWN(be)->dtzFlags[t];
      if ((flags & 1) != bside && !be->symmetric) {
        *success = -1;
        return 0;
      }
    }
    ei =  type == WDL ? &ei[t + 4 * bside]
        : type == DTM ? &ei[t + 6 * bside] : &ei[t];
    while (i < be->num)
      i = fill_squares(pos, ei->pieces, flip, flip ? 0x38 : 0, p, i);
    idx = type != DTM ? encode_pawn_f(p, ei, be) : encode_pawn_r(p, ei, be);
  }

  uint8_t *w = decompress_pairs(ei->precomp, idx);

  if (type == WDL)
    return (int)w[0] - 2;

  int v = w[0] + ((w[1] & 0x0f) << 8);

  if (type == DTM) {
    if (!be->dtmLossOnly)
      v = (int)from_le_u16(be->hasPawns
                       ? PAWN(be)->dtmMap[PAWN(be)->dtmMapIdx[t][bside][s] + v]
                        : PIECE(be)->dtmMap[PIECE(be)->dtmMapIdx[bside][s] + v]);
  } else {
    if (flags & 2) {
      int m = WdlToMap[s + 2];
      if (!(flags & 16))
        v =  be->hasPawns
           ? ((uint8_t *)PAWN(be)->dtzMap)[PAWN(be)->dtzMapIdx[t][m] + v]
           : ((uint8_t *)PIECE(be)->dtzMap)[PIECE(be)->dtzMapIdx[m] + v];
      else
        v = (int)from_le_u16(be->hasPawns
                         ? ((uint16_t *)PAWN(be)->dtzMap)[PAWN(be)->dtzMapIdx[t][m] + v]
                          : ((uint16_t *)PIECE(be)->dtzMap)[PIECE(be)->dtzMapIdx[m] + v]);
    }
    if (!(flags & PAFlags[s + 2]) || (s & 1))
      v *= 2;
  }

  return v;
}

static int probe_wdl_table(const Pos *pos, int *success)
{
  return probe_table(pos, 0, success, WDL);
}

static int probe_dtm_table(const Pos *pos, int won, int *success)
{
  return probe_table(pos, won, success, DTM);
}

static int probe_dtz_table(const Pos *pos, int wdl, int *success)
{
  return probe_table(pos, wdl, success, DTZ);
}

// probe_ab() is not called for positions with en passant captures.
static int probe_ab(const Pos *pos, int alpha, int beta, int *success)
{
  assert(pos->ep == 0);

  TbMove moves0[TB_MAX_CAPTURES];
  TbMove *m = moves0;
  // Generate (at least) all legal captures including (under)promotions.
  // It is OK to generate more, as long as they are filtered out below.
  TbMove *end = gen_captures(pos, m);
  for (; m < end; m++) {
    Pos pos1;
    TbMove move = *m;
    if (!is_capture(pos, move))
      continue;
    if (!do_move(&pos1, pos, move))
      continue; // illegal move
    int v = -probe_ab(&pos1, -beta, -alpha, success);
    if (*success == 0) return 0;
    if (v > alpha) {
      if (v >= beta)
        return v;
      alpha = v;
    }
  }

  int v = probe_wdl_table(pos, success);

  return alpha >= v ? alpha : v;
}

// Probe the WDL table for a particular position.
//
// If *success != 0, the probe was successful.
//
// If *success == 2, the position has a winning capture, or the position
// is a cursed win and has a cursed winning capture, or the position
// has an ep capture as only best move.
// This is used in probe_dtz().
//
// The return value is from the point of view of the side to move:
// -2 : loss
// -1 : loss, but draw under 50-move rule
//  0 : draw
//  1 : win, but draw under 50-move rule
//  2 : win
int probe_wdl(Pos *pos, int *success)
{
  *success = 1;

  // Generate (at least) all legal captures including (under)promotions.
  TbMove moves0[TB_MAX_CAPTURES];
  TbMove *m = moves0;
  TbMove *end = gen_captures(pos, m);
  int bestCap = -3, bestEp = -3;

  // We do capture resolution, letting bestCap keep track of the best
  // capture without ep rights and letting bestEp keep track of still
  // better ep captures if they exist.

  for (; m < end; m++) {
    Pos pos1;
    TbMove move = *m;
    if (!is_capture(pos, move))
      continue;
    if (!do_move(&pos1, pos, move))
      continue; // illegal move
    int v = -probe_ab(&pos1, -2, -bestCap, success);
    if (*success == 0) return 0;
    if (v > bestCap) {
      if (v == 2) {
        *success = 2;
        return 2;
      }
      if (!is_en_passant(pos,move))
        bestCap = v;
      else if (v > bestEp)
        bestEp = v;
    }
  }

  int v = probe_wdl_table(pos, success);
  if (*success == 0) return 0;

  // Now max(v, bestCap) is the WDL value of the position without ep rights.
  // If the position without ep rights is not stalemate or no ep captures
  // exist, then the value of the position is max(v, bestCap, bestEp).
  // If the position without ep rights is stalemate and bestEp > -3,
  // then the value of the position is bestEp (and we will have v == 0).

  if (bestEp > bestCap) {
    if (bestEp > v) { // ep capture (possibly cursed losing) is best.
      *success = 2;
      return bestEp;
    }
    bestCap = bestEp;
  }

  // Now max(v, bestCap) is the WDL value of the position unless
  // the position without ep rights is stalemate and bestEp > -3.

  if (bestCap >= v) {
    // No need to test for the stalemate case here: either there are
    // non-ep captures, or bestCap == bestEp >= v anyway.
    *success = 1 + (bestCap > 0);
    return bestCap;
  }

  // Now handle the stalemate case.
  if (bestEp > -3 && v == 0) {
    TbMove moves[TB_MAX_MOVES];
    end = gen_moves(pos, moves);
    // Check for stalemate in the position with ep captures.
    for (m = moves; m < end; m++) {
      if (!is_en_passant(pos,*m) && legal_move(pos, *m)) break;
    }
    if (m == end && !is_check(pos)) {
      // stalemate score from tb (w/o e.p.), but an en-passant capture
      // is possible.
      *success = 2;
      return bestEp;
    }
  }
  // Stalemate / en passant not an issue, so v is the correct value.

  return v;
}

#if 0
// This will not be called for positions with en passant captures
static Value probe_dtm_dc(const Pos *pos, int won, int *success)
{
  assert(ep_square() == 0);

  Value v, bestCap = -TB_VALUE_INFINITE;

  TbMove moves0[TB_MAX_CAPTURES];
  TbMove *end, *m = moves0;

  // Generate at least all legal captures including (under)promotions
  end = gen_captures(pos, m);
  Pos pos1;
  for (; m < end; m++) {
    TbMove move = m->move;
    if (!is_capture(pos, move))
      continue;
    if (!do_move(&pos1, pos, move))
      continue;
    if (!won)
      v = -probe_dtm_dc(&pos1, 1, success) + 1;
    else if (probe_ab(&pos1, -1, 0, success) < 0 && *success)
      v = -probe_dtm_dc(&pos1, 0, success) - 1;
    else
      v = -TB_VALUE_INFINITE;
    bestCap = max(bestCap,v);
    if (*success == 0) return 0;
  }

  int dtm = probe_dtm_table(pos, won, success);
  v = won ? TB_VALUE_MATE - 2 * dtm + 1 : -TB_VALUE_MATE + 2 * dtm;

  return max(bestCap,v);
}
#endif

static Value probe_dtm_win(const Pos *pos, int *success);

// Probe a position known to lose by probing the DTM table and looking
// at captures.
static Value probe_dtm_loss(const Pos *pos, int *success)
{
  Value v, best = -TB_VALUE_INFINITE, numEp = 0;

  TbMove moves0[TB_MAX_CAPTURES];
  // Generate at least all legal captures including (under)promotions
  TbMove *end, *m = moves0;
  end = gen_captures(pos, m);

  Pos pos1;
  for (; m < end; m++) {
    TbMove move = *m;
    if (!is_capture(pos, move) || !legal_move(pos, move))
      continue;
    if (is_en_passant(pos, move))
      numEp++;
    do_move(&pos1, pos, move);
    v = -probe_dtm_win(&pos1, success) + 1;
    if (v > best) {
      best = v;
    }
    if (*success == 0)
      return 0;
  }

  // If there are en passant captures, the position without ep rights
  // may be a stalemate. If it is, we must avoid probing the DTM table.
  if (numEp != 0 && gen_legal(pos, m) == m + numEp)
    return best;

  v = -TB_VALUE_MATE + 2 * probe_dtm_table(pos, 0, success);
  return best > v ? best : v;
}

static Value probe_dtm_win(const Pos *pos, int *success)
{
  Value v, best = -TB_VALUE_INFINITE;

  // Generate all moves
  TbMove moves0[TB_MAX_CAPTURES];
  TbMove *m = moves0;
  TbMove *end = gen_moves(pos, m);
  // Perform a 1-ply search
  Pos pos1;
  for (; m < end; m++) {
    TbMove move = *m;
    if (do_move(&pos1, pos, move)) {
      // not legal
      continue;
    }
    if ((pos1.ep > 0  ? probe_wdl(&pos1, success)
         : probe_ab(&pos1, -1, 0, success)) < 0
        && *success)
      v = -probe_dtm_loss(&pos1, success) - 1;
    else
      v = -TB_VALUE_INFINITE;
    if (v > best) {
      best = v;
    }
    if (*success == 0) return 0;
  }

  return best;
}

#if 0
// To be called only for non-drawn positions.
Value TB_probe_dtm2(const Pos *pos, int wdl, int *success)
{
  assert(wdl != 0);

  *success = 1;
  Value v, bestCap = -TB_VALUE_INFINITE, bestEp = -TB_VALUE_INFINITE;

  TbMove moves0[TB_MAX_CAPTURES];
  TbMove *end, *m = moves0;

  // Generate at least all legal captures including (under)promotions
  end = gen_captures(pos, m);
  Pos pos0 = *pos;

  // Resolve captures, letting bestCap keep track of the best non-ep
  // capture and letting bestEp keep track of the best ep capture.
  Pos pos1;
  for (; m < end; m++) {
    TbMove move = *m;
    if (!is_capture(pos, move))
      continue;
    if (!do_move(&pos1, pos, move))
      continue;
    if (wdl < 0)
      v = -probe_dtm_dc(&pos1, 1, success) + 1;
    else if (probe_ab(&pos1, -1, 0, success) < 0 && *success)
      v = -probe_dtm_dc(&pos1, 0, success) - 1;
    else
      v = -TB_VALUE_MATE;
    if (is_en_passant(&pos1, move))
      bestEp = max(bestEp,v);
    else
      bestCap = max(bestCap,v);
    if (*success == 0)
      return 0;
  }

  // If there are en passant captures, we have to determine the WDL value
  // for the position without ep rights if it might be different.
  if (bestEp > -TB_VALUE_INFINITE && (bestEp < 0 || bestCap < 0)) {
    assert(ep_square() != 0);
    uint8_t s = pos->st->epSquare;
    pos->st->epSquare = 0;
    wdl = probe_ab(pos, -2, 2, success);
    pos->st->epSquare = s;
    if (*success == 0)
      return 0;
    if (wdl == 0)
      return bestEp;
  }

  bestCap = max(bestCap,v);
  int dtm = probe_dtm_table(pos, wdl > 0, success);
  v = wdl > 0 ? TB_VALUE_MATE - 2 * dtm + 1 : -TB_VALUE_MATE + 2 * dtm;
  return max(bestCap,v);
}
#endif

static int WdlToDtz[] = { -1, -101, 0, 101, 1 };

// Probe the DTZ table for a particular position.
// If *success != 0, the probe was successful.
// The return value is from the point of view of the side to move:
//         n < -100 : loss, but draw under 50-move rule
// -100 <= n < -1   : loss in n ply (assuming 50-move counter == 0)
//         0        : draw
//     1 < n <= 100 : win in n ply (assuming 50-move counter == 0)
//   100 < n        : win, but draw under 50-move rule
//
// If the position mate, -1 is returned instead of 0.
//
// The return value n can be off by 1: a return value -n can mean a loss
// in n+1 ply and a return value +n can mean a win in n+1 ply. This
// cannot happen for tables with positions exactly on the "edge" of
// the 50-move rule.
//
// This means that if dtz > 0 is returned, the position is certainly
// a win if dtz + 50-move-counter <= 99. Care must be taken that the engine
// picks moves that preserve dtz + 50-move-counter <= 99.
//
// If n = 100 immediately after a capture or pawn move, then the position
// is also certainly a win, and during the whole phase until the next
// capture or pawn move, the inequality to be preserved is
// dtz + 50-movecounter <= 100.
//
// In short, if a move is available resulting in dtz + 50-move-counter <= 99,
// then do not accept moves leading to dtz + 50-move-counter == 100.
//
int probe_dtz(Pos *pos, int *success)
{
  int wdl = probe_wdl(pos, success);
  if (*success == 0) return 0;

  // If draw, then dtz = 0.
  if (wdl == 0) return 0;

  // Check for winning capture or en passant capture as only best move.
  if (*success == 2)
    return WdlToDtz[wdl + 2];

  TbMove moves[TB_MAX_MOVES];
  TbMove *m = moves, *end = NULL;
  Pos pos1;

  // If winning, check for a winning pawn move.
  if (wdl > 0) {
    // Generate at least all legal non-capturing pawn moves
    // including non-capturing promotions.
    // (The following call in fact generates all moves.)
    end = gen_legal(pos, moves);

    for (m = moves; m < end; m++) {
      TbMove move = *m;
      if (type_of_piece_moved(pos,move) != TB_ENUM_PAWN || is_capture(pos, move))
         continue;
      if (!do_move(&pos1, pos, move))
         continue; // not legal
      int v = -probe_wdl(&pos1, success);
      if (*success == 0) return 0;
      if (v == wdl) {
        assert(wdl < 3);
        return WdlToDtz[wdl + 2];
      }
    }
  }

  // If we are here, we know that the best move is not an ep capture.
  // In other words, the value of wdl corresponds to the WDL value of
  // the position without ep rights. It is therefore safe to probe the
  // DTZ table with the current value of wdl.

  int dtz = probe_dtz_table(pos, wdl, success);
  if (*success >= 0)
    return WdlToDtz[wdl + 2] + ((wdl > 0) ? dtz : -dtz);

  // *success < 0 means we need to probe DTZ for the other side to move.
  int best;
  if (wdl > 0) {
    best = INT32_MAX;
  } else {
    // If (cursed) loss, the worst case is a losing capture or pawn move
    // as the "best" move, leading to dtz of -1 or -101.
    // In case of mate, this will cause -1 to be returned.
    best = WdlToDtz[wdl + 2];
    // If wdl < 0, we still have to generate all moves.
    end = gen_moves(pos, m);
  }
  assert(end != NULL);

  for (m = moves; m < end; m++) {
    TbMove move = *m;
    // We can skip pawn moves and captures.
    // If wdl > 0, we already caught them. If wdl < 0, the initial value
    // of best already takes account of them.
    if (is_capture(pos, move) || type_of_piece_moved(pos, move) == TB_ENUM_PAWN)
      continue;
    if (!do_move(&pos1, pos, move)) {
      // move was not legal
      continue;
    }
    int v = -probe_dtz(&pos1, success);
    // Check for the case of mate in 1
    if (v == 1 && is_mate(&pos1))
      best = 1;
    else if (wdl > 0) {
      if (v > 0 && v + 1 < best)
        best = v + 1;
    } else {
      if (v - 1 < best)
        best = v - 1;
    }
    if (*success == 0) return 0;
  }
  return best;
}

// Use the DTZ tables to rank and score all root moves in the list.
// A return value of 0 means that not all probes were successful.
static int root_probe_dtz(const Pos *pos, bool hasRepeated, bool useRule50, struct TbRootMoves *rm)
{
  int v, success;

  // Obtain 50-move counter for the root position.
  int cnt50 = pos->rule50;

  // The border between draw and win lies at rank 1 or rank 900, depending
  // on whether the 50-move rule is used.
  int bound = useRule50 ? 900 : 1;

  // Probe, rank and score each move.
  TbMove rootMoves[TB_MAX_MOVES];
  TbMove * end = gen_legal(pos,rootMoves);
  rm->size = (unsigned)(end-rootMoves);
  Pos pos1;
  for (unsigned i = 0; i < rm->size; i++) {
    struct TbRootMove *m = &(rm->moves[i]);
    m->move = rootMoves[i];
    do_move(&pos1, pos, m->move);

    // Calculate dtz for the current move counting from the root position.
    if (pos1.rule50 == 0) {
      // If the move resets the 50-move counter, dtz is -101/-1/0/1/101.
      v = -probe_wdl(&pos1, &success);
      assert(v < 3);
      v = WdlToDtz[v + 2];
    } else {
      // Otherwise, take dtz for the new position and correct by 1 ply.
      v = -probe_dtz(&pos1, &success);
      if (v > 0) v++;
      else if (v < 0) v--;
    }
    // Make sure that a mating move gets value 1.
    if (v == 2 && is_mate(&pos1)) {
      v = 1;
    }

    if (!success) return 0;

    // Better moves are ranked higher. Guaranteed wins are ranked equally.
    // Losing moves are ranked equally unless a 50-move draw is in sight.
    // Note that moves ranked 900 have dtz + cnt50 == 100, which in rare
    // cases may be insufficient to win as dtz may be one off (see the
    // comments before TB_probe_dtz()).
    int r =  v > 0 ? (v + cnt50 <= 99 && !hasRepeated ? 1000 : 1000 - (v + cnt50))
           : v < 0 ? (-v * 2 + cnt50 < 100 ? -1000 : -1000 + (-v + cnt50))
           : 0;
    m->tbRank = r;

    // Determine the score to be displayed for this move. Assign at least
    // 1 cp to cursed wins and let it grow to 49 cp as the position gets
    // closer to a real win.
    m->tbScore =  r >= bound ? TB_VALUE_MATE - TB_MAX_MATE_PLY - 1
                : r >  0     ? max( 3, r - 800) * TB_VALUE_PAWN / 200
                : r == 0     ? TB_VALUE_DRAW
                : r > -bound ? min(-3, r + 800) * TB_VALUE_PAWN / 200
                :             -TB_VALUE_MATE + TB_MAX_MATE_PLY + 1;
  }
  return 1;
}

// Use the WDL tables to rank all root moves in the list.
// This is a fallback for the case that some or all DTZ tables are missing.
// A return value of 0 means that not all probes were successful.
int root_probe_wdl(const Pos *pos, bool useRule50, struct TbRootMoves *rm)
{
  static int WdlToRank[] = { -1000, -899, 0, 899, 1000 };
  static Value WdlToValue[] = {
    -TB_VALUE_MATE + TB_MAX_MATE_PLY + 1,
    TB_VALUE_DRAW - 2,
    TB_VALUE_DRAW,
    TB_VALUE_DRAW + 2,
    TB_VALUE_MATE - TB_MAX_MATE_PLY - 1
  };

  int v, success;

  // Probe, rank and score each move.
  TbMove moves[TB_MAX_MOVES];
  TbMove *end = gen_legal(pos,moves);
  rm->size = (unsigned)(end-moves);
  Pos pos1;
  for (unsigned i = 0; i < rm->size; i++) {
    struct TbRootMove *m = &rm->moves[i];
    m->move = moves[i];
    do_move(&pos1, pos, m->move);
    v = -probe_wdl(&pos1, &success);
    if (!success) return 0;
    if (!useRule50)
      v = v > 0 ? 2 : v < 0 ? -2 : 0;
    m->tbRank = WdlToRank[v + 2];
    m->tbScore = WdlToValue[v + 2];
  }

  return 1;
}

static const int wdl_to_dtz[] =
{
    -1, -101, 0, 101, 1
};

// This supports the original Fathom root probe API
static uint16_t probe_root(Pos *pos, int *score, unsigned *results)
{
    int success;
    int dtz = probe_dtz(pos, &success);
    if (!success)
        return 0;

    int16_t scores[MAX_MOVES];
    uint16_t moves0[MAX_MOVES];
    uint16_t *moves = moves0;
    uint16_t *end = gen_moves(pos, moves);
    size_t len = end - moves;
    size_t num_draw = 0;
    unsigned j = 0;
    for (unsigned i = 0; i < len; i++)
    {
        Pos pos1;
        if (!do_move(&pos1, pos, moves[i]))
        {
            scores[i] = SCORE_ILLEGAL;
            continue;
        }
        int v = 0;
        //        print_move(pos,moves[i]);
        if (dtz > 0 && is_mate(&pos1))
            v = 1;
        else
        {
            if (pos1.rule50 != 0)
            {
                v = -probe_dtz(&pos1, &success);
                if (v > 0)
                    v++;
                else if (v < 0)
                    v--;
            }
            else
            {
                v = -probe_wdl(&pos1, &success);
                v = wdl_to_dtz[v + 2];
            }
        }
        num_draw += (v == 0);
        if (!success)
            return 0;
        scores[i] = v;
        if (results != NULL)
        {
            unsigned res = 0;
            res = TB_SET_WDL(res, dtz_to_wdl(pos->rule50, v));
            res = TB_SET_FROM(res, move_from(moves[i]));
            res = TB_SET_TO(res, move_to(moves[i]));
            res = TB_SET_PROMOTES(res, move_promotes(moves[i]));
            res = TB_SET_EP(res, is_en_passant(pos, moves[i]));
            res = TB_SET_DTZ(res, (v < 0? -v: v));
            results[j++] = res;
        }
    }
    if (results != NULL)
        results[j++] = TB_RESULT_FAILED;
    if (score != NULL)
        *score = dtz;

    // Now be a bit smart about filtering out moves.
    if (dtz > 0)        // winning (or 50-move rule draw)
    {
        int best = BEST_NONE;
        uint16_t best_move = 0;
        for (unsigned i = 0; i < len; i++)
        {
            int v = scores[i];
            if (v == SCORE_ILLEGAL)
                continue;
            if (v > 0 && v < best)
            {
                best = v;
                best_move = moves[i];
            }
        }
        return (best == BEST_NONE? 0: best_move);
    }
    else if (dtz < 0)   // losing (or 50-move rule draw)
    {
        int best = 0;
        uint16_t best_move = 0;
        for (unsigned i = 0; i < len; i++)
        {
            int v = scores[i];
            if (v == SCORE_ILLEGAL)
                continue;
            if (v < best)
            {
                best = v;
                best_move = moves[i];
            }
        }
        return (best == 0? MOVE_CHECKMATE: best_move);
    }
    else                // drawing
    {
        // Check for stalemate:
        if (num_draw == 0)
            return MOVE_STALEMATE;

        // Select a "random" move that preserves the draw.
        // Uses calc_key as the PRNG.
        size_t count = calc_key(pos, !pos->turn) % num_draw;
        for (unsigned i = 0; i < len; i++)
        {
            int v = scores[i];
            if (v == SCORE_ILLEGAL)
                continue;
            if (v == 0)
            {
                if (count == 0)
                    return moves[i];
                count--;
            }
        }
        return 0;
    }
}

#ifndef TB_NO_HELPER_API

unsigned tb_pop_count(uint64_t bb)
{
    return popcount(bb);
}

unsigned tb_lsb(uint64_t bb)
{
    return lsb(bb);
}

uint64_t tb_pop_lsb(uint64_t bb)
{
    return poplsb(bb);
}

uint64_t tb_king_attacks(unsigned sq)
{
    return king_attacks(sq);
}

uint64_t tb_queen_attacks(unsigned sq, uint64_t occ)
{
    return queen_attacks(sq, occ);
}

uint64_t tb_rook_attacks(unsigned sq, uint64_t occ)
{
    return rook_attacks(sq, occ);
}

uint64_t tb_bishop_attacks(unsigned sq, uint64_t occ)
{
    return bishop_attacks(sq, occ);
}

uint64_t tb_knight_attacks(unsigned sq)
{
    return knight_attacks(sq);
}

uint64_t tb_pawn_attacks(unsigned sq, bool color)
{
    return pawn_attacks(sq, color);
}

#endif      /* TB_NO_HELPER_API */
