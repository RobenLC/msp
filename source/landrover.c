#include <stdint.h> 
#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
//#include <getopt.h> 
#include <fcntl.h> 
#include <sys/ioctl.h> 
#include <sys/mman.h> 
#include <linux/types.h> 
#include <linux/spi/spidev.h> 
//#include <sys/times.h> 
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
//main()

#define ENABLE_SPI1        (1)
#define SPI_CPHA  0x01          /* clock phase */
#define SPI_CPOL  0x02          /* clock polarity */
#define SPI_MODE_0		(0|0)
#define SPI_MODE_1		(0|SPI_CPHA)
#define SPI_MODE_2		(SPI_CPOL|0)
#define SPI_MODE_3		(SPI_CPOL|SPI_CPHA)
#define SPI_SPEED    20000000

#define OP_MAX 0xff
#define OP_NONE 0x00

/* flow operation */
#define OP_PON          0x1                
#define OP_QRY          0x2
#define OP_RDY          0x3                
#define OP_SINGLE       0x4
#define OP_DOUBLE       0x5
#define OP_ACTION       0x6
#define OP_FIH             0x7
#define OP_RAW           0x8
#define OP_MSINGLE    0x9
#define OP_MDOUBLE    0xa
#define OP_HANDSCAN  0xb
#define OP_NOTESCAN  0xc
#define OP_POLL           0xd

/* SD read write operation */               
#define OP_SDRD          0x20
#define OP_SDWT          0x21
#define OP_STSEC_00     0x22
#define OP_STSEC_01     0x23
#define OP_STSEC_02     0x24
#define OP_STSEC_03     0x25
#define OP_STLEN_00     0x26
#define OP_STLEN_01     0x27
#define OP_STLEN_02     0x28
#define OP_STLEN_03     0x29
#define OP_SDAT         0x2a
#define OP_FREESEC      0x2b
#define OP_USEDSEC      0x2c
#define OP_SDINIT          0x2d
#define OP_SDSTATS       0x2e
#define OP_EG_DECT       0x2f
/* scanner parameters */
#define OP_MSG          0x30       
#define OP_FFORMAT      0x31
#define OP_COLRMOD      0x32
#define OP_COMPRAT      0x33
#define OP_RESOLTN      0x34
#define OP_SCANGAV      0x35
#define OP_MAXWIDH      0x36
#define OP_WIDTHAD_H    0x37
#define OP_WIDTHAD_L    0x38
#define OP_SCANLEN_H    0x39
#define OP_SCANLEN_L    0x3a
#define OP_INTERIMG     0x3b
#define OP_AFEIC        0x3c
#define OP_EXTPULSE     0x3d
#define OP_SUPBACK      0x3e
#define OP_LOG          0x3f
#define OP_RGRD          0x40
#define OP_RGWT          0x41
#define OP_RGDAT        0x42
#define OP_RGADD_H      0x43
#define OP_RGADD_L      0x44

#define OP_CROP_01        0x45
#define OP_CROP_02        0x46
#define OP_CROP_03        0x47
#define OP_CROP_04        0x48
#define OP_CROP_05        0x49
#define OP_CROP_06        0x4a

#define OP_IMG_LEN        0x4b

#define OP_META_DAT     0x4c

#define OP_AP_MODEN     0x50
/*
#define OP_DAT 0x08
#define OP_SCM 0x09
#define OP_DUL 0x0a
*/
#define OP_FUNCTEST_00              0x70
#define OP_FUNCTEST_01              0x71
#define OP_FUNCTEST_02              0x72
#define OP_FUNCTEST_03              0x73
#define OP_FUNCTEST_04              0x74
#define OP_FUNCTEST_05              0x75
#define OP_FUNCTEST_06              0x76
#define OP_FUNCTEST_07              0x77
#define OP_FUNCTEST_08              0x78
#define OP_FUNCTEST_09              0x79
#define OP_FUNCTEST_10              0x7A
#define OP_FUNCTEST_11              0x7B
#define OP_FUNCTEST_12              0x7C
#define OP_FUNCTEST_13              0x7D
#define OP_FUNCTEST_14              0x7E
#define OP_FUNCTEST_15              0x7F

#define OP_BLEEDTHROU_ADJUST 0x81
#define OP_BLACKWHITE_THSHLD 0x82

#define DATA_END_PULL_LOW_DELAY 30000

/* debug */
#define OP_SAVE         0x80
#define OP_ERROR        0xe0

#define SEC_LEN 512
#define SPI_TRUNK_SZ   (32768)
#define DIRECT_WT_DISK    (0)

#define RANDOM_CROP_COORD (0)
#define CROP_NUMBER   (18)

#define sprintf_f  sprintf

#if (CROP_NUMBER == 18)
#define CROP_SAMPLE_SIZE (35)

struct cropSample_s {
    int cropx;
    int cropy;
};

struct cropPoints_s{
    struct cropSample_s samples[CROP_SAMPLE_SIZE]; 
};
#endif

#if RANDOM_CROP_COORD
#else
#if 0
static int crop_01[2] = {517, 72};
static int crop_02[2] = {545, 1304};
static int crop_03[2] = {965, 1304};
static int crop_04[2] = {2696, 1264};
static int crop_05[2] = {2677, 72};
static int crop_06[2] = {517, 72};
#elif 0
static int crop_01[2] = {1316, 1080};
static int crop_02[2] = {2319, 1288};
static int crop_03[2] = {3412, 1288};
static int crop_04[2] = {3482, 56};
static int crop_05[2] = {3466, 0};
static int crop_06[2] = {1347, 0};
#elif 0
static int crop_01[2] = {517*2, 580*2};
static int crop_02[2] = {1544*2, 688*2};
static int crop_03[2] = {1586*2, 688*2};
static int crop_04[2] = {1611*2, 60*2};
static int crop_05[2] = {1607*2, 32*2};
static int crop_06[2] = {537*2, 32*2};
#elif (CROP_NUMBER == 18)
#if (CROP_SAMPLE_SIZE == 35)
static struct cropPoints_s crop_01 = {{{738,  146}  , {982,  69}   , {1468, 1257 }, {1154,  290} , {1062,  1894}, {1263,  2267}, {918,  1909} , {1003,  56}  , {714,  362}  , {881,  541}  , {1803,  195} , {671,  1565} , {1423,  48}  , {877,  2641} , {0,  2729}   , {1169,  1841}, {1591,  586} ,{365,  1224} , {491,  20}   , {575,  1107} , {314,  1314} , {421,  823}  , {623,  546} , {575,  544} , {656,  888}  , {427,  831} , {496,  730}, {839,  94}   , {701,  887} , {830,  980} , {2507, 1871}, {0,    1753}, {689,  1895}, {674,  1895}, {472,  355}}};
static struct cropPoints_s crop_02 = {{{824,  4030} , {1018,  1482}, {1868, 13824}, {1219,  2176}, {2316,  2375}, {2397,  2679}, {3102,  2477}, {1099,  2164}, {780,  2492} , {3010,  2781}, {1933,  3130}, {3644,  1923}, {1496,  2191}, {3169,  2992}, {2499,  4578}, {2794,  2126}, {1700,  2619},{1514,  1298}, {516,  1969} , {975,  1516} , {1124,  2033}, {1576,  1197}, {1476,  944}, {726,  923} , {1200, 1106} , {944,  1165}, {1545, 858}, {1193, 1073}, {1192,  1099}, {1424, 1088}, {2535, 1983}, {4,    1980}, {720,  2013}, {707,  1967}, {2168, 1960}}};
static struct cropPoints_s crop_03 = {{{865,  4030} , {1266,  1482}, {9352, 13824}, {1256,  2176}, {2351,  2375}, {2467,  2679}, {3325,  2477}, {1256,  2164}, {854,  2492} , {3096,  2781}, {1996,  3130}, {3697,  1923}, {1585,  2191}, {3196,  2992}, {2525,  4578}, {2997,  2126}, {1743,  2619},{1552,  1298}, {725,  1969} , {1769,  1516}, {1332,  2033}, {1600,  1197}, {1531,  944}, {770,  923} , {1248, 1106} , {990,  1165}, {1599, 858}, {1251, 1073}, {1290,  1099}, {1484, 1088}, {2560, 1983}, {44,   1980}, {765,  2013}, {750,  1967}, {2204, 1960}}};
static struct cropPoints_s crop_04 = {{{3731,  3797}, {3138,  1325}, {9392, 13778}, {2629,  1879}, {2653,  305} , {2552,  102} , {3373,  73}  , {2859,  375} , {3715,  2194}, {3139,  352} , {3134,  2796}, {3761,  318} , {2923,  2026}, {3587,  373} , {3488,  788} , {3037,  1754}, {3538,  2426},{1584,  188} , {1640,  1640}, {1788,  30}  , {1451,  728} , {1834,  374} , {1763,  364}, {1674,  260}, {1442, 180}  , {1383, 296} , {1667, 115}, {1508, 700} , {1397,  345} , {1581, 91}  , {4106, 249} , {1574, 265} , {2173, 206} , {2354, 299} , {2245, 1851}}};
static struct cropPoints_s crop_05 = {{{3629,  3}   , {3105,  1}   , {8696, 6    }, {2591,  6}   , {1610,  7}   , {1688,  2}   , {1394,  4}   , {2793,  8}   , {3671,  6}   , {3104,  5}   , {3006,  5}   , {1132,  3}   , {2860,  6}   , {1542,  7}   , {736,  2}    , {2978,  1}   , {3434,  2}   ,{1552,  1}   , {1615,  2}   , {1342,  8}   , {1429,  2}   , {754,  2}    , {942,  7}   , {818,  1}   , {912,  2}    , {881,  7}   , {633,  6}  , {1419, 6}   , {1166,  2}   , {1072, 7}   , {3038, 2}   , {500,  1}   , {1122, 1}   , {1300, 5}   , {1590, 8}}};
static struct cropPoints_s crop_06 = {{{3375,  3}   , {2867,  1}   , {8580, 6    }, {1168,  6}   , {1384,  7}   , {1324,  2}   , {1021,  4}   , {2343,  8}   , {3306,  6}   , {893,  5}    , {2872,  5}   , {734,  3}    , {2534,  6}   , {1167,  7}   , {678,  2}    , {1210,  1}   , {2970,  2}   ,{1225,  1}   , {919,  2}    , {595,  8}    , {319,  2}    , {685,  2}    , {851,  7}   , {741,  1}   , {833,  2}    , {799,  7}   , {562,  6}  , {1337, 6}   , {1052,  2}   , {912,  7}   , {2960, 2}   , {431,  1}   , {1010, 1}   , {1217, 5}   , {1529, 8}}};
static struct cropPoints_s crop_07 = {{{3174,  11}  , {2563,  9}   , {8614, 14   }, {1163,  14}  , {1364,  15}  , {1323,  10}  , {1021,  12}  , {2138,  16}  , {2902,  14}  , {891,  13}   , {2672,  13}  , {740,  11}   , {2107,  14}  , {1158,  15}  , {676,  10}   , {1210,  9}   , {2870,  10}  ,{772,  9}    , {514,  10}   , {588,  16}   , {319,  10}   , {675,  10}   , {831,  15}  , {721,  9}   , {812,  10}   , {778,  15}  , {562,  14} , {1258, 14}  , {1020,  10}  , {899,  15}  , {2953, 10}  , {415,  9}   , {999,  9}   , {1204, 13}  , {1496, 16}}};
static struct cropPoints_s crop_08 = {{{3635,  11}  , {3104,  9}   , {8698, 14   }, {2593,  14}  , {1633,  15}  , {1867,  10}  , {1977,  12}  , {2824,  16}  , {3667,  14}  , {3106,  13}  , {2997,  13}  , {1263,  11}  , {2856,  14}  , {1848,  15}  , {756,  10}   , {2982,  9}   , {3435,  10}  ,{1570,  9}   , {1615,  10}  , {1785,  16}  , {1443,  10}  , {779,  10}   , {962,  15}  , {839,  9}   , {933,  10}   , {902,  15}  , {710,  14} , {1433, 14}  , {1195,  10}  , {1255, 15}  , {3089, 10}  , {552,  9}   , {1191, 9}   , {1343, 13}  , {1603, 16}}};
static struct cropPoints_s crop_09 = {{{2879,  19}  , {2179,  17}  , {8606, 22   }, {1163,  22}  , {1259,  23}  , {1325,  18}  , {1016,  20}  , {1244,  24}  , {2324,  22}  , {891,  21}   , {2499,  21}  , {737,  19}   , {1696,  22}  , {1155,  23}  , {674,  18}   , {1212,  17}  , {2867,  18}  ,{396,  17}   , {495,  18}   , {589,  24}   , {322,  18}   , {667,  18}   , {818,  23}  , {705,  17}  , {796,  18}   , {765,  23}  , {561,  22} , {1173, 22}  , {997,  18}   , {889,  23}  , {2950, 18}  , {408,  17}  , {998,  17}  , {1199, 21}  , {1464, 24}}};
static struct cropPoints_s crop_10 = {{{3639,  19}  , {3104,  17}  , {8706, 22   }, {2591,  22}  , {1646,  23}  , {2060,  18}  , {2681,  20}  , {2825,  24}  , {3669,  22}  , {3105,  21}  , {3000,  21}  , {1402,  19}  , {2857,  22}  , {2269,  23}  , {787,  18}   , {2979,  17}  , {3452,  18}  ,{1572,  17}  , {1616,  18}  , {1787,  24}  , {1446,  18}  , {805,  18}   , {976,  23}  , {854,  17}  , {1115,  18}  , {917,  23}  , {789,  22} , {1440, 22}  , {1215,  18}  , {1429, 23}  , {3134, 18}  , {597,  17}  , {1254, 17}  , {1386, 21}  , {1607, 24}}};
static struct cropPoints_s crop_11 = {{{818,  4002} , {1016,  1456}, {1902, 13797}, {1201,  2149}, {2155,  2350}, {1275,  2649}, {988,  2451} , {1066,  2135}, {771,  2461} , {899,  2756} , {1927,  3100}, {3002,  1898}, {1494,  2165}, {2842,  2966}, {2410,  4553}, {1190,  2096}, {1679,  2593},{855,  1272} , {516,  1937} , {576,  1487} , {315,  2001} , {1533,  1169}, {1423,  918}, {662,  896} , {991,  1081} , {882,  1134}, {1188, 829}, {1093, 1045}, {738,  1073} , {873,  1062}, {2524, 1953}, {1,    1952}, {701,  1984}, {683,  1940}, {2105, 1935}}};
static struct cropPoints_s crop_12 = {{{2182,  4002}, {2297,  1456}, {9336, 13797}, {2581,  2149}, {2370,  2350}, {2474,  2649}, {3337,  2451}, {2832,  2135}, {3081,  2461}, {3100,  2756}, {2446,  3100}, {3701,  1898}, {2284,  2165}, {3227,  2966}, {2535,  4553}, {3004,  2096}, {1996,  2593},{1558,  1272}, {1539,  1937}, {1775,  1487}, {1342,  2001}, {1603,  1169}, {1585,  918}, {984,  896} , {1302,  1081}, {1053, 1134}, {1603, 829}, {1340, 1045}, {1320,  1073}, {1523, 1062}, {2643, 1953}, {97,   1952}, {811,  1984}, {821,  1940}, {2220, 1935}}};
static struct cropPoints_s crop_13 = {{{820,  4010} , {1016,  1464}, {1914, 13805}, {1205,  2157}, {2169,  2358}, {1581,  2657}, {1127,  2459}, {1064,  2143}, {772,  2469} , {1051,  2764}, {1927,  3108}, {3223,  1906}, {1494,  2173}, {2941,  2974}, {2444,  4561}, {1195,  2104}, {1678,  2601},{1072,  1280}, {516,  1945} , {575,  1495} , {315,  2009} , {1543,  1177}, {1434,  926}, {674,  904} , {1158,  1089}, {892,  1142}, {1290, 837}, {1110, 1053}, {745,  1081} , {912,  1070}, {2535, 1961}, {0,    1960}, {715,  1992}, {696,  1948}, {2122, 1943}}};
static struct cropPoints_s crop_14 = {{{1849,  4010}, {1996,  1464}, {9360, 13805}, {2184,  2157}, {2366,  2358}, {2473,  2657}, {3314,  2459}, {2829,  2143}, {2295,  2469}, {3102,  2764}, {2309,  3108}, {3701,  1906}, {2062,  2173}, {3223,  2974}, {2539,  4561}, {3003,  2104}, {1900,  2601},{1560,  1280}, {1536,  1945}, {1774,  1495}, {1341,  2009}, {1607,  1177}, {1577,  926}, {961,  904} , {1291,  1089}, {1041, 1142}, {1603, 837}, {1325, 1053}, {1316,  1081}, {1515, 1070}, {2620, 1961}, {90,   1960}, {792,  1992}, {801,  1948}, {2223, 1943}}};
static struct cropPoints_s crop_15 = {{{820,  4018} , {1017,  1472}, {1926, 13813}, {1199,  2165}, {2292,  2366}, {1905,  2665}, {1824,  2467}, {1066,  2151}, {773,  2477} , {2054,  2772}, {1948,  3116}, {3474,  1914}, {1495,  2181}, {3051,  2982}, {2484,  4569}, {1197,  2112}, {1689,  2609},{1291,  1288}, {515,  1953} , {576,  1503} , {316,  2017} , {1558,  1185}, {1447,  934}, {689,  912} , {1173,  1097}, {904,  1150}, {1397, 845}, {1138, 1061}, {756,  1089} , {1138, 1078}, {2533, 1969}, {0,    1968}, {702,  2000}, {693,  1956}, {2159, 1951}}};
static struct cropPoints_s crop_16 = {{{1468,  4018}, {1572,  1472}, {9322, 13813}, {1757,  2165}, {2363,  2366}, {2471,  2665}, {3321,  2467}, {2191,  2151}, {1710,  2477}, {3104,  2772}, {2187,  3116}, {3701,  1914}, {1824,  2181}, {3197,  2982}, {2529,  4569}, {3003,  2112}, {1831,  2609},{1559,  1288}, {1532,  1953}, {1773,  1503}, {1336,  2017}, {1607,  1185}, {1559,  934}, {800,  912} , {1274,  1097}, {1030, 1150}, {1601, 845}, {1300, 1061}, {1308,  1089}, {1505, 1078}, {2601, 1969}, {72,   1968}, {780,  2000}, {754,  1956}, {2215, 1951}}};
static struct cropPoints_s crop_17 = {{{824,  4026} , {1018,  1480}, {1902, 13821}, {1198,  2173}, {2316,  2374}, {2223,  2673}, {3023,  2475}, {1075,  2159}, {777,  2485} , {3010,  2780}, {1931,  3124}, {3644,  1922}, {1496,  2189}, {3163,  2990}, {2499,  4577}, {1946,  2120}, {1697,  2617},{1491,  1296}, {516,  1961} , {585,  1511} , {620,  2025} , {1573,  1193}, {1473,  942}, {719,  920} , {1200,  1105}, {922,  1158}, {1507, 853}, {1171, 1069}, {1160,  1097}, {1376, 1086}, {2537, 1977}, {2,    1976}, {713,  2008}, {704,  1964}, {2168, 1959}}};
static struct cropPoints_s crop_18 = {{{1145,  4026}, {1290,  1480}, {9356, 13821}, {1295,  2173}, {2351,  2374}, {2472,  2673}, {3330,  2475}, {1516,  2159}, {1300,  2485}, {3096,  2780}, {2056,  3124}, {3697,  1922}, {1624,  2189}, {3197,  2990}, {2525,  4577}, {3000,  2120}, {1744,  2617},{1553,  1296}, {1515,  1961}, {1762,  1511}, {1333,  2025}, {1602,  1193}, {1535,  942}, {778,  920} , {1248,  1105}, {1007, 1158}, {1601, 853}, {1272, 1069}, {1294,  1097}, {1489, 1086}, {2571, 1977}, {54,   1976}, {757,  2008}, {755,  1964}, {2204, 1959}}};
#elif (CROP_SAMPLE_SIZE == 4)
static struct cropPoints_s crop_01 = {{{74  , 1410}, { 297,  869}, { 251, 1152}, { 235, 1150}}};
static struct cropPoints_s crop_02 = {{{1102, 1917}, { 855, 1222}, {1491, 1183}, {1767, 1284}}};
static struct cropPoints_s crop_03 = {{{1119, 1917}, { 877, 1222}, {1821, 1183}, {1799, 1284}}};
static struct cropPoints_s crop_04 = {{{1793, 487 }, {1395, 362 }, {1838, 57  }, {1894, 126 }}};
static struct cropPoints_s crop_05 = {{{ 792, 2   }, { 845, 3   }, {1135, 3   }, { 479, 2   }}};
static struct cropPoints_s crop_06 = {{{ 742, 2   }, { 807, 3   }, { 268, 3   }, { 329, 2   }}};
static struct cropPoints_s crop_07 = {{{ 739, 6   }, { 804, 7   }, { 268, 7   }, { 328, 6   }}};
static struct cropPoints_s crop_08 = {{{ 800, 6   }, { 853, 7   }, {1472, 7   }, { 532, 6   }}};
static struct cropPoints_s crop_09 = {{{ 738, 10  }, { 803, 11  }, { 267, 11  }, { 328, 10  }}};
static struct cropPoints_s crop_10 = {{{ 808, 10  }, { 859, 11  }, {1818, 11  }, { 590, 10  }}};
static struct cropPoints_s crop_11 = {{{1069, 1901}, { 814, 1206}, { 534, 1167}, {1578, 1268}}};
static struct cropPoints_s crop_12 = {{{1126, 1901}, { 887, 1206}, {1821, 1167}, {1800, 1268}}};
static struct cropPoints_s crop_13 = {{{1077, 1905}, { 831, 1210}, { 823, 1171}, {1621, 1272}}};
static struct cropPoints_s crop_14 = {{{1124, 1905}, { 885, 1210}, {1821, 1171}, {1800, 1272}}};
static struct cropPoints_s crop_15 = {{{1086, 1909}, { 840, 1214}, {1056, 1175}, {1663, 1276}}};
static struct cropPoints_s crop_16 = {{{1123, 1909}, { 884, 1214}, {1821, 1175}, {1800, 1276}}};
static struct cropPoints_s crop_17 = {{{1094, 1913}, { 847, 1218}, {1254, 1179}, {1716, 1280}}};
static struct cropPoints_s crop_18 = {{{1121, 1913}, { 881, 1218}, {1821, 1179}, {1799, 1280}}};
#elif (CROP_SAMPLE_SIZE == 5)
#if 1 /* 0614 */
static struct cropPoints_s crop_01 = {{{1699, 21  }, {870, 1318 }, {1602, 1878}, {1478, 1894}, {1483, 1889}}};
static struct cropPoints_s crop_02 = {{{1727, 2143}, {1720, 2374}, {2777, 2127}, {2654, 2161}, {2657, 2188}}};
static struct cropPoints_s crop_03 = {{{1995, 2143}, {1746, 2374}, {2831, 2127}, {2702, 2161}, {2706, 2188}}};
static struct cropPoints_s crop_04 = {{{2909, 2015}, {3267, 1040}, {3117, 251 }, {2958, 390 }, {3012, 254 }}};
static struct cropPoints_s crop_05 = {{{2890, 5   }, {2399, 4   }, {2096, 2   }, {1937, 7   }, {1984, 5   }}};
static struct cropPoints_s crop_06 = {{{2486, 5   }, {2350, 4   }, {1868, 2   }, {1712, 7   }, {1772, 5   }}};
static struct cropPoints_s crop_07 = {{{1699, 21  }, {2339, 20  }, {1740, 18  }, {1663, 23  }, {1646, 21  }}};
static struct cropPoints_s crop_08 = {{{2888, 21  }, {2413, 20  }, {2131, 18  }, {2205, 23  }, {2030, 21  }}};
static struct cropPoints_s crop_09 = {{{1701, 37  }, {2327, 36  }, {1729, 34  }, {1570, 39  }, {1632, 37  }}};
static struct cropPoints_s crop_10 = {{{2889, 37  }, {2424, 36  }, {2544, 34  }, {2786, 39  }, {2411, 37  }}};
static struct cropPoints_s crop_11 = {{{1721, 2020}, {1605, 2275}, {1622, 2017}, {1511, 2054}, {1630, 2084}}};
static struct cropPoints_s crop_12 = {{{2909, 2020}, {1897, 2275}, {2996, 2017}, {2859, 2054}, {2860, 2084}}};
static struct cropPoints_s crop_13 = {{{1720, 2052}, {1657, 2307}, {1777, 2049}, {1615, 2086}, {1818, 2116}}};
static struct cropPoints_s crop_14 = {{{2906, 2052}, {1863, 2307}, {2979, 2049}, {2859, 2086}, {2857, 2116}}};
static struct cropPoints_s crop_15 = {{{1720, 2084}, {1688, 2339}, {2192, 2081}, {1828, 2118}, {2213, 2148}}};
static struct cropPoints_s crop_16 = {{{2907, 2084}, {1791, 2339}, {2978, 2081}, {2855, 2118}, {2853, 2148}}};
static struct cropPoints_s crop_17 = {{{1721, 2116}, {1715, 2371}, {2615, 2113}, {2497, 2150}, {2631, 2180}}};
static struct cropPoints_s crop_18 = {{{2906, 2116}, {1752, 2371}, {2957, 2113}, {2734, 2150}, {2730, 2180}}};
#else /* 0616 */
static struct cropPoints_s crop_01 = {{{1365, 2068 }, {1284, 12   }, {1205, 2091 }, {1167, 137  }, {1174, 2103 }}};
static struct cropPoints_s crop_02 = {{{2495, 2197 }, {1303, 2134 }, {2338, 2215 }, {1409, 2229 }, {2303, 2152 }}};
static struct cropPoints_s crop_03 = {{{2545, 2197 }, {1723, 2134 }, {2383, 2215 }, {1456, 2229 }, {2355, 2152 }}};
static struct cropPoints_s crop_04 = {{{2711, 89   }, {2486, 1910 }, {2572, 106  }, {2587, 2091 }, {2410, 28   }}};
static struct cropPoints_s crop_05 = {{{1613, 7    }, {2431, 4    }, {1469, 5    }, {2324, 1    }, {1507, 6    }}};
static struct cropPoints_s crop_06 = {{{1543, 7    }, {1873, 4    }, {1408, 5    }, {2278, 1    }, {1290, 6    }}};
static struct cropPoints_s crop_07 = {{{1531, 11   }, {1527, 8    }, {1394, 9    }, {2249, 5    }, {1228, 10   }}};
static struct cropPoints_s crop_08 = {{{1668, 11   }, {2466, 8    }, {1507, 9    }, {2342, 5    }, {1687, 10   }}};
static struct cropPoints_s crop_09 = {{{1531, 15   }, {1284, 12   }, {1392, 13   }, {2215, 9    }, {1228, 14   }}};
static struct cropPoints_s crop_10 = {{{1729, 15   }, {2466, 12   }, {1553, 13   }, {2342, 9    }, {1871, 14   }}};
static struct cropPoints_s crop_11 = {{{2314, 2182 }, {1298, 2119 }, {2189, 2200 }, {1408, 2216 }, {1785, 2137 }}};
static struct cropPoints_s crop_12 = {{{2548, 2182 }, {2483, 2119 }, {2387, 2200 }, {1559, 2216 }, {2359, 2137 }}};
static struct cropPoints_s crop_13 = {{{2364, 2186 }, {1297, 2123 }, {2237, 2204 }, {1408, 2220 }, {1941, 2141 }}};
static struct cropPoints_s crop_14 = {{{2548, 2186 }, {2482, 2123 }, {2385, 2204 }, {1529, 2220 }, {2358, 2141 }}};
static struct cropPoints_s crop_15 = {{{2409, 2190 }, {1300, 2127 }, {2284, 2208 }, {1410, 2224 }, {2059, 2145 }}};
static struct cropPoints_s crop_16 = {{{2548, 2190 }, {2479, 2127 }, {2385, 2208 }, {1497, 2224 }, {2358, 2145 }}};
static struct cropPoints_s crop_17 = {{{2468, 2194 }, {1299, 2131 }, {2320, 2212 }, {1409, 2228 }, {2211, 2149 }}};
static struct cropPoints_s crop_18 = {{{2546, 2194 }, {2477, 2131 }, {2384, 2212 }, {1456, 2228 }, {2357, 2149 }}};
#endif
#elif (CROP_SAMPLE_SIZE == 6)
static struct cropPoints_s crop_01 = {{{1271, 141}, {1351, 28 }, {1163, 93 }, {520, 510 }, {877, 465 }, {924, 407 }}};
static struct cropPoints_s crop_02 = {{{1529, 3129}, {1451, 2315}, {1309, 3282}, {1240, 3629}, {1529, 3593}, {1577, 3330}}};
static struct cropPoints_s crop_03 = {{{1563, 3129}, {1527, 2315}, {1364, 3282}, {1260, 3629}, {1553, 3593}, {1601, 3330}}};
static struct cropPoints_s crop_04 = {{{3317, 2937}, {2849, 2109}, {3623, 3173}, {3481, 3074}, {3788, 3104}, {3323, 2933}}};
static struct cropPoints_s crop_05 = {{{3043, 2}, {2780, 7}, {3477, 6}, {2798, 5}, {3147, 2}, {2678, 7}}};
static struct cropPoints_s crop_06 = {{{2934, 2}, {2431, 7}, {3271, 6}, {2738, 5}, {3088, 2}, {2637, 7}}};
static struct cropPoints_s crop_07 = {{{2847, 10}, {1952, 15}, {3051, 14}, {2692, 13}, {3047, 10}, {2604, 15}}};
static struct cropPoints_s crop_08 = {{{3054, 10}, {2781, 15}, {3454, 14}, {2810, 13}, {3157, 10}, {2679, 15}}};
static struct cropPoints_s crop_09 = {{{2739, 18}, {1481, 23}, {2816, 22}, {2651, 21}, {3006, 18}, {2567, 23}}};
static struct cropPoints_s crop_10 = {{{3064, 18}, {2780, 23}, {3481, 22}, {2811, 21}, {3158, 18}, {2681, 23}}};
static struct cropPoints_s crop_11 = {{{1525, 3097}, {1444, 2278}, {1308, 3253}, {1234, 3604}, {1522, 3561}, {1562, 3302}}};
static struct cropPoints_s crop_12 = {{{1863, 3097}, {1630, 2278}, {2098, 3253}, {1355, 3604}, {1690, 3561}, {1716, 3302}}};
static struct cropPoints_s crop_13 = {{{1526, 3105}, {1444, 2286}, {1310, 3261}, {1236, 3612}, {1524, 3569}, {1564, 3310}}};
static struct cropPoints_s crop_14 = {{{1779, 3105}, {1630, 2286}, {1865, 3261}, {1327, 3612}, {1662, 3569}, {1681, 3310}}};
static struct cropPoints_s crop_15 = {{{1526, 3113}, {1459, 2294}, {1308, 3269}, {1239, 3620}, {1526, 3577}, {1569, 3318}}};
static struct cropPoints_s crop_16 = {{{1706, 3113}, {1549, 2294}, {1676, 3269}, {1289, 3620}, {1621, 3577}, {1650, 3318}}};
static struct cropPoints_s crop_17 = {{{1528, 3121}, {1451, 2310}, {1308, 3277}, {1240, 3628}, {1528, 3585}, {1575, 3326}}};
static struct cropPoints_s crop_18 = {{{1627, 3121}, {1527, 2310}, {1471, 3277}, {1260, 3628}, {1583, 3585}, {1618, 3326}}};
#elif (CROP_SAMPLE_SIZE == 7)
static struct cropPoints_s crop_01 = {{{967, 105  }, {492, 3190 }, {1437, 12  }, {1135, 2976}, {1475, 150 }, {903, 2999 }, {946, 124  }}};
static struct cropPoints_s crop_02 = {{{1100, 3290  }, {2738, 3299  }, {1491, 3038  }, {2877, 3171  }, {1761, 3154  }, {2628, 3097  }, {1161, 3125  }}};
static struct cropPoints_s crop_03 = {{{1146, 3290}, {2792, 3299}, {1551, 3038}, {2914, 3171}, {1801, 3154}, {2690, 3097}, {1197, 3125}}};
static struct cropPoints_s crop_04 = {{{3415, 3152}, {2881, 67  }, {3287, 2958}, {3202, 187 }, {3548, 2954}, {2826, 88  }, {2953, 2978}}};
static struct cropPoints_s crop_05 = {{{3280, 3}, {768, 3 }, {3231, 1}, {1482, 7}, {3248, 2}, {1174, 6}, {2707, 2}}};
static struct cropPoints_s crop_06 = {{{3086, 3}, {569, 3 }, {2462, 1}, {1413, 7}, {3152, 2}, {1028, 6}, {2604, 2}}};
static struct cropPoints_s crop_07 = {{{2859, 11}, {569, 11 }, {1521, 9 }, {1413, 15}, {3067, 10}, {1028, 14}, {2507, 10}}};
static struct cropPoints_s crop_08 = {{{3268, 11}, {1059, 11}, {3237, 9 }, {1555, 15}, {3257, 10}, {1363, 14}, {2723, 10}}};
static struct cropPoints_s crop_09 = {{{2678, 19}, {569, 19 }, {1439, 17}, {1412, 23}, {2981, 18}, {1028, 22}, {2413, 18}}};
static struct cropPoints_s crop_10 = {{{3280, 19}, {1328, 19}, {3238, 17}, {1625, 23}, {3268, 18}, {1536, 22}, {2737, 18}}};
static struct cropPoints_s crop_11 = {{{1110, 3258}, {2188, 3274}, {1490, 3008}, {2616, 3142}, {1760, 3129}, {2029, 3069}, {1159, 3097}}};
static struct cropPoints_s crop_12 = {{{1822, 3258}, {2811, 3274}, {2274, 3008}, {2924, 3142}, {2015, 3129}, {2698, 3069}, {1573, 3097}}};
static struct cropPoints_s crop_13 = {{{1097, 3266}, {2356, 3282}, {1490, 3016}, {2692, 3150}, {1760, 3137}, {2215, 3077}, {1160, 3105}}};
static struct cropPoints_s crop_14 = {{{1643, 3266}, {2804, 3282}, {2080, 3016}, {2922, 3150}, {1948, 3137}, {2697, 3077}, {1465, 3105}}};
static struct cropPoints_s crop_15 = {{{1098, 3274}, {2546, 3290}, {1490, 3024}, {2774, 3158}, {1761, 3145}, {2384, 3085}, {1161, 3113}}};
static struct cropPoints_s crop_16 = {{{1471, 3274}, {2802, 3290}, {1889, 3024}, {2918, 3158}, {1873, 3145}, {2693, 3085}, {1355, 3113}}};
static struct cropPoints_s crop_17 = {{{1102, 3282}, {2738, 3298}, {1490, 3032}, {2854, 3166}, {1761, 3153}, {2572, 3093}, {1160, 3121}}};
static struct cropPoints_s crop_18 = {{{1293, 3282}, {2792, 3298}, {1680, 3032}, {2913, 3166}, {1801, 3153}, {2689, 3093}, {1231, 3121}}};
#else
static int crop_01[2] = {399, 45};
static int crop_02[2] = {432, 4392};
static int crop_03[2] = {583, 4392};
static int crop_04[2] = {3850, 4228};
static int crop_05[2] = {3799, 1};
static int crop_06[2] = {3410, 1};
static int crop_07[2] = {2547, 10};
static int crop_08[2] = {3825, 10};
static int crop_09[2] = {1538, 20};
static int crop_10[2] = {3823, 20};
static int crop_11[2] = {431, 4301};
static int crop_12[2] = {3850, 4301};
static int crop_13[2] = {431, 4324};
static int crop_14[2] = {3847, 4324};
static int crop_15[2] = {433, 4347};
static int crop_16[2] = {3838, 4347};
static int crop_17[2] = {433, 4370};
static int crop_18[2] = {2238, 4370};
#endif
#else
static int crop_01[2] = {731, 31};
static int crop_02[2] = {797, 1129};
static int crop_03[2] = {829, 1129};
static int crop_04[2] = {1498, 1042};
static int crop_05[2] = {1426, 1};
static int crop_06[2] = {1278, 1};
#endif
#endif

#define DOT_8 0x01
#define DOT_7 0x02
#define DOT_6 0x04
#define DOT_5 0x08
#define DOT_4 0x10
#define DOT_3 0x20
#define DOT_2 0x40
#define DOT_1 0x80

#define OPT_SIZE (OP_EXTPULSE - OP_FFORMAT + 1)
#define TIFF_RAW (0)
static FILE *mlog = 0;
static struct logPool_s *mlogPool;
static char *infpath;

typedef int (*func)(int argc, char *argv[]);

typedef enum {
    STINIT = 0,
    WAIT,
    NEXT,
    BREAK,
    EVTMAX,
}event_e;

typedef enum {
    SPY = 0,
    BULLET, // 1
    LASER,  // 2
    AUTO_A,  // 3
    AUTO_B,  // 4
    AUTO_C,  // 5
    AUTO_D,  // 6
    AUTO_E,  // 7
    AUTO_F,  // 8
    AUTO_G,  // 9
    AUTO_H,  // 10
    AUTO_I,  // 11
    SMAX,   // 12
}state_e;

typedef enum {
    PSSET = 0,
    PSACT,
    PSWT,
    PSRLT,
    PSTSM,
    PSMAX,
}status_e;

typedef enum {
    SINSCAN_NONE=0,
    SINSCAN_WIFI_ONLY,
    SINSCAN_SD_ONLY,
    SINSCAN_WIFI_SD,
    SINSCAN_WHIT_BLNC,
    SINSCAN_USB,
    SINSCAN_DUAL_STRM,
    SINSCAN_DUAL_SD,
} singleScan_e;

typedef enum {
    DOUSCAN_NONE=0,
    DOUSCAN_WIFI_ONLY,
    DOUSCAN_SD_ONLY,
    DOUSCAN_WIFI_SD,
} doubleScan_e;

typedef enum {
    NOTESCAN_NONE=0,
    NOTESCAN_OPTION_01,
    NOTESCAN_OPTION_02,
    NOTESCAN_OPTION_03,
    NOTESCAN_OPTION_04,
    NOTESCAN_OPTION_05,
    NOTESCAN_OPTION_06,
    NOTESCAN_OPTION_07,
} noteScan_e;

typedef enum {
    FILE_FORMAT_NONE=0,
    FILE_FORMAT_JPG,
    FILE_FORMAT_PDF,
    FILE_FORMAT_RAW,
    FILE_FORMAT_TIFF_I,
    FILE_FORMAT_TIFF_M,
} fileFormat_e;

typedef enum {
    COLOR_MODE_NONE=0,
    COLOR_MODE_COLOR,
    COLOR_MODE_GRAY,
    COLOR_MODE_GRAY_DETAIL,
    COLOR_MODE_BLACKWHITE,
} colorMode_e;

#if 0
typedef enum {
    ASPMETA_FUNC_NONE = 0,
    ASPMETA_FUNC_CONF = 0x1,       /* 0b00000001 */
    ASPMETA_FUNC_CROP = 0x2,       /* 0b00000010 */
    ASPMETA_FUNC_IMGLEN = 0x4,   /* 0b00000100 */
    ASPMETA_FUNC_SDFREE = 0x8,   /* 0b00001000 */
    ASPMETA_FUNC_SDUSED = 0x10, /* 0b00010000 */
    ASPMETA_FUNC_SDRD = 0x20,     /* 0b00100000 */
    ASPMETA_FUNC_SDWT = 0x40,    /* 0b01000000 */
} aspMetaFuncbit_e;
#endif

typedef enum {
    ASPMETA_FUNC_NONE = 0,
    ASPMETA_FUNC_CONF = 0b00000001,
    ASPMETA_FUNC_CROP = 0b00000010,
    ASPMETA_FUNC_IMGLEN = 0b00000100,
    ASPMETA_FUNC_SDFREE = 0b00001000,
    ASPMETA_FUNC_SDUSED = 0b00010000,
    ASPMETA_FUNC_SDRD = 0b00100000,
    ASPMETA_FUNC_SDWT = 0b01000000,
} aspMetaFuncbit_e;

typedef enum {
    ASPMETA_POWON_INIT = 0,
    ASPMETA_SCAN_GO = 1,
    ASPMETA_SCAN_COMPLETE = 2,
    ASPMETA_CROP_300DPI = 3,
    ASPMETA_CROP_600DPI = 4,
    ASPMETA_SCAN_COMPLE_DUO = 5,
    ASPMETA_CROP_300DPI_DUO = 6,
    ASPMETA_CROP_600DPI_DUO = 7,
    ASPMETA_SD = 8,
    ASPMETA_OCR = 9,
} aspMetaParam_e;

typedef enum {
    ASPOP_STA_NONE = 0x0,
    ASPOP_STA_WR = 0x01,
    ASPOP_STA_UPD = 0x02,
    ASPOP_STA_APP = 0x04,
    ASPOP_STA_CON = 0x08,
    ASPOP_STA_SCAN = 0x10,
} aspOpSt_e;

typedef enum {
    ASPOP_CODE_NONE = 0,
    ASPOP_FILE_FORMAT,   /* 01 */
    ASPOP_COLOR_MODE,
    ASPOP_COMPRES_RATE,
    ASPOP_SCAN_SINGLE,
    ASPOP_SCAN_DOUBLE,
    ASPOP_ACTION,
    ASPOP_RESOLUTION,
    ASPOP_SCAN_GRAVITY,
    ASPOP_MAX_WIDTH,
    ASPOP_WIDTH_ADJ_H, /* 10 */
    ASPOP_WIDTH_ADJ_L,
    ASPOP_SCAN_LENS_H,
    ASPOP_SCAN_LENS_L,
    ASPOP_INTER_IMG,     
    ASPOP_AFEIC_SEL,     
    ASPOP_EXT_PULSE,     
    ASPOP_SDFAT_RD,      
    ASPOP_SDFAT_WT,
    ASPOP_SDFAT_STR01,
    ASPOP_SDFAT_STR02,  /* 20 */
    ASPOP_SDFAT_STR03,
    ASPOP_SDFAT_STR04,
    ASPOP_SDFAT_LEN01,
    ASPOP_SDFAT_LEN02,
    ASPOP_SDFAT_LEN03,
    ASPOP_SDFAT_LEN04,
    ASPOP_SDFAT_SDAT,
    ASPOP_REG_RD,
    ASPOP_REG_WT,
    ASPOP_REG_ADDRH, /* 30 */
    ASPOP_REG_ADDRL,
    ASPOP_REG_DAT,
    ASPOP_SUP_SAVE,
    ASPOP_SDFREE_FREESEC,
    ASPOP_SDFREE_STR01,
    ASPOP_SDFREE_STR02,
    ASPOP_SDFREE_STR03,
    ASPOP_SDFREE_STR04,
    ASPOP_SDFREE_LEN01,
    ASPOP_SDFREE_LEN02, /* 40 */
    ASPOP_SDFREE_LEN03,
    ASPOP_SDFREE_LEN04,
    ASPOP_SDUSED_USEDSEC,
    ASPOP_SDUSED_STR01,
    ASPOP_SDUSED_STR02,
    ASPOP_SDUSED_STR03,
    ASPOP_SDUSED_STR04,
    ASPOP_SDUSED_LEN01,
    ASPOP_SDUSED_LEN02,
    ASPOP_SDUSED_LEN03, /* 50 */
    ASPOP_SDUSED_LEN04,
    ASPOP_FUNTEST_00,
    ASPOP_FUNTEST_01,
    ASPOP_FUNTEST_02,
    ASPOP_FUNTEST_03,
    ASPOP_FUNTEST_04,
    ASPOP_FUNTEST_05,
    ASPOP_FUNTEST_06,
    ASPOP_FUNTEST_07,
    ASPOP_FUNTEST_08, /* 60 */
    ASPOP_FUNTEST_09,
    ASPOP_FUNTEST_10,
    ASPOP_FUNTEST_11,
    ASPOP_FUNTEST_12,
    ASPOP_FUNTEST_13,
    ASPOP_FUNTEST_14,
    ASPOP_FUNTEST_15,
    ASPOP_BLEEDTHROU_ADJUST,
    ASPOP_BLACKWHITE_THSHLD,
    ASPOP_CROP_01,
    ASPOP_CROP_02, /* 70 */
    ASPOP_CROP_03,
    ASPOP_CROP_04,
    ASPOP_CROP_05,
    ASPOP_CROP_06,
    ASPOP_IMG_LEN, /* must be here for old design */
    ASPOP_CROP_07,
    ASPOP_CROP_08,
    ASPOP_CROP_09,
    ASPOP_CROP_10,
    ASPOP_CROP_11, /* 80 */
    ASPOP_CROP_12,
    ASPOP_CROP_13,
    ASPOP_CROP_14,
    ASPOP_CROP_15,
    ASPOP_CROP_16,
    ASPOP_CROP_17,
    ASPOP_CROP_18,
    ASPOP_CROP_COOR_XH,
    ASPOP_CROP_COOR_XL,
    ASPOP_CROP_COOR_YH, /* 90 */
    ASPOP_CROP_COOR_YL,
    ASPOP_EG_DECT,
    ASPOP_AP_MODE,
    ASPOP_XCROP_GAT,
    ASPOP_XCROP_LINSTR,
    ASPOP_XCROP_LINREC,
    ASPOP_CROP_01_DUO,
    ASPOP_CROP_02_DUO, 
    ASPOP_CROP_03_DUO,
    ASPOP_CROP_04_DUO,   /* 100 */
    ASPOP_CROP_05_DUO,
    ASPOP_CROP_06_DUO,
    ASPOP_IMG_LEN_DUO, /* must be here for old design */
    ASPOP_CROP_07_DUO,
    ASPOP_CROP_08_DUO,
    ASPOP_CROP_09_DUO,
    ASPOP_CROP_10_DUO,
    ASPOP_CROP_11_DUO, 
    ASPOP_CROP_12_DUO,
    ASPOP_CROP_13_DUO,  /* 110 */
    ASPOP_CROP_14_DUO,
    ASPOP_CROP_15_DUO,
    ASPOP_CROP_16_DUO,
    ASPOP_CROP_17_DUO,
    ASPOP_CROP_18_DUO,
    ASPOP_XCROP_GAT_DUO,
    ASPOP_XCROP_LINSTR_DUO,
    ASPOP_XCROP_LINREC_DUO,
    ASPOP_CODE_MAX, /* 119 */
} aspOpCode_e;

typedef enum {
    ASPOP_TYPE_NONE = 0,
    ASPOP_TYPE_SINGLE,
    ASPOP_TYPE_MULTI,
    ASPOP_TYPE_VALUE,
} aspOpType_e;

typedef enum {
    ASPOP_MASK_0 = 0x0,
    ASPOP_MASK_1 = 0x1,
    ASPOP_MASK_2 = 0x3,
    ASPOP_MASK_3 = 0x7,
    ASPOP_MASK_4 = 0xf,
    ASPOP_MASK_5 = 0x1f,
    ASPOP_MASK_6 = 0x3f,
    ASPOP_MASK_7 = 0x7f,
    ASPOP_MASK_8 = 0xff,
    ASPOP_MASK_16 = 0xffff,
    ASPOP_MASK_32 = 0xffffffff,
} aspOpMask_e;

typedef enum {
    APM_NONE=0,
    APM_AP,
    APM_DIRECT,
} APMode_e;

struct aspConfig_s{
    uint32_t opStatus;
    uint32_t opCode;
    uint32_t opValue;
    uint32_t opMask;
    uint32_t opType;
    uint32_t opBitlen;
};

struct psdata_s {
    uint32_t result;
    uint32_t ansp0;
    struct procRes_s *rs;
};

typedef int (*stfunc)(struct psdata_s *data);

struct logPool_s{
    char *pool;
    char *cur;
    int max;
    int len;
};

struct cmd_s{
    int  id;
    char str[16];
    func pfunc;
};

struct pipe_s{
    int rt[2];
};

struct ring_s{
    int run;
    int seq;
};

struct ring_p{
    struct ring_s lead;
    struct ring_s dual;
    struct ring_s prelead;
    struct ring_s predual;
    struct ring_s folw;
};

struct socket_s{
    int listenfd;
    int connfd;
    struct sockaddr_in serv_addr; 
};

struct shmem_s{
    int totsz;
    int chksz;
    int slotn;
    char **pp;
    int svdist;
    struct ring_p *r;
    int lastflg;
    int lastsz;
    int dualsz;	
};

struct modersp_s{
    int m;
    int r;
    int d;
    int v;	
};

struct info16Bit_s{
    uint8_t     inout;
    uint8_t     seqnum;
    uint8_t     opcode;
    uint8_t     data;
    uint32_t   info;
};

struct SdAddrs_s{
    uint32_t f;
    uint32_t sdc;
    union {
        uint32_t n;
        uint8_t d[4];
    };
    uint32_t sda;
};

struct DiskFile_s{
    int  rtops;
    int  rtlen;
    FILE *vsd;
    char *sdt;
    int rtMax;
};

struct RegisterRW_s{
    uint32_t rga16;
    int rgidx;
    int rgact;
};

struct machineCtrl_s{
    uint32_t seqcnt;
    struct info16Bit_s tmp;
    struct info16Bit_s cur;
    struct info16Bit_s get;
    struct SdAddrs_s sdst;
    struct SdAddrs_s sdln;
    struct DiskFile_s fdsk;
    struct RegisterRW_s regp;
};

struct virtualReg_s {
    uint32_t vrAddr;
    uint32_t vrValue;
};

struct cropCoord_s {
    unsigned int CROP_COOD_01[2];// = {818, 557};
    unsigned int CROP_COOD_02[2];// = {980, 1557};
    unsigned int CROP_COOD_03[2];// = {1168, 1557};
    unsigned int CROP_COOD_04[2];// = {1586, 1484};
    unsigned int CROP_COOD_05[2];// = {1415, 487};
    unsigned int CROP_COOD_06[2];// = {1253, 487};
    unsigned int CROP_COOD_07[2];// = {0, 0};
    unsigned int CROP_COOD_08[2];// = {0, 0};
    unsigned int CROP_COOD_09[2];// = {0, 0};
    unsigned int CROP_COOD_10[2];// = {0, 0};
    unsigned int CROP_COOD_11[2];// = {0, 0};
    unsigned int CROP_COOD_12[2];// = {0, 0};
    unsigned int CROP_COOD_13[2];// = {0, 0};
    unsigned int CROP_COOD_14[2];// = {0, 0};
    unsigned int CROP_COOD_15[2];// = {0, 0};
    unsigned int CROP_COOD_16[2];// = {0, 0};
    unsigned int CROP_COOD_17[2];// = {0, 0};
    unsigned int CROP_COOD_18[2];// = {0, 0};
};

struct intMbs_s{
    union {
        uint32_t n;
        uint8_t d[4];
    };
};

struct aspMetaData_s{
  struct intMbs_s     FUNC_BITS;             // byte[4] 
  unsigned char  ASP_MAGIC[2];            //byte[6] "0x20 0x14"
  
  /* ASPMETA_FUNC_CONF = 0x1 */       /* 0b00000001 */
  unsigned char  FILE_FORMAT;             //0x31
  unsigned char  COLOR_MODE;              //0x32
  unsigned char  COMPRESSION_RATE;        //0x33
  unsigned char  RESOLUTION;              //0x34
  unsigned char  SCAN_GRAVITY;            //0x35
  unsigned char  CIS_MAX_WIDTH;           //0x36
  unsigned char  WIDTH_ADJUST_H;          //0x37
  unsigned char  WIDTH_ADJUST_L;          //0x38
  unsigned char  SCAN_LENGTH_H;           //0x39
  unsigned char  SCAN_LENGTH_L;           //0x3a
  unsigned char  INTERNAL_IMG;            //0x3b
  unsigned char  AFE_IC_SELEC;            //0x3c
  unsigned char  EXTNAL_PULSE;            //0x3d
  unsigned char  SUP_WRITEBK;             //0x3e
  unsigned char  OP_FUNC_00;              //0x70
  unsigned char  OP_FUNC_01;              //0x71
  unsigned char  OP_FUNC_02;              //0x72
  unsigned char  OP_FUNC_03;              //0x73
  unsigned char  OP_FUNC_04;              //0x74
  unsigned char  OP_FUNC_05;              //0x75
  unsigned char  OP_FUNC_06;              //0x76
  unsigned char  OP_FUNC_07;              //0x77
  unsigned char  OP_FUNC_08;              //0x78
  unsigned char  OP_FUNC_09;              //0x79
  unsigned char  OP_FUNC_10;              //0x7A
  unsigned char  OP_FUNC_11;              //0x7B
  unsigned char  OP_FUNC_12;              //0x7C
  unsigned char  OP_FUNC_13;              //0x7D
  unsigned char  OP_FUNC_14;              //0x7E
  unsigned char  OP_FUNC_15;              //0x7F  
  unsigned char  BLEEDTHROU_ADJUST; //0x81
  unsigned char  BLACKWHITE_THSHLD; //0x82  
  unsigned char  OP_RESERVE[26];          // byte[64]

  /* ASPMETA_FUNC_CROP = 0x2 */       /* 0b00000010 */
  struct intMbs_s CROP_POS_1;        //byte[68]
  struct intMbs_s CROP_POS_2;        //byte[72]
  struct intMbs_s CROP_POS_3;        //byte[76]
  struct intMbs_s CROP_POS_4;        //byte[80]
  struct intMbs_s CROP_POS_5;        //byte[84]
  struct intMbs_s CROP_POS_6;        //byte[88]
  struct intMbs_s CROP_POS_7;        //byte[92]
  struct intMbs_s CROP_POS_8;        //byte[96]
  struct intMbs_s CROP_POS_9;        //byte[100]
  struct intMbs_s CROP_POS_10;        //byte[104]
  struct intMbs_s CROP_POS_11;        //byte[108]
  struct intMbs_s CROP_POS_12;        //byte[112]
  struct intMbs_s CROP_POS_13;        //byte[116]
  struct intMbs_s CROP_POS_14;        //byte[120]
  struct intMbs_s CROP_POS_15;        //byte[124]
  struct intMbs_s CROP_POS_16;        //byte[128]
  struct intMbs_s CROP_POS_17;        //byte[132]
  struct intMbs_s CROP_POS_18;        //byte[136]
  unsigned char  Start_Pos_1st;         //byte[137]
  unsigned char  Start_Pos_2nd;        //byte[138]
  unsigned char  End_Pos_All;            //byte[139]
  unsigned char  Start_Pos_RSV;        //byte[140], not using for now
  unsigned char  YLine_Gap;               //byte[141]
  unsigned char  Start_YLine_No;       //byte[142]
  unsigned short YLines_Recorded;     //byte[144] 16bits
  unsigned char CROP_RESERVE[16]; //byte[160]

  /* ASPMETA_FUNC_IMGLEN = 0x4 */     /* 0b00000100 */
  struct intMbs_s SCAN_IMAGE_LEN;     //byte[164]
  
  /* ASPMETA_FUNC_SDFREE = 0x8 */     /* 0b00001000 */
  struct intMbs_s  FREE_SECTOR_ADD;   //byte[168]
  struct intMbs_s  FREE_SECTOR_LEN;   //byte[172]
  
  /* ASPMETA_FUNC_SDUSED = 0x16 */    /* 0b00010000 */
  struct intMbs_s  USED_SECTOR_ADD;   //byte[176]
  struct intMbs_s  USED_SECTOR_LEN;   //byte[180]
  
  /* ASPMETA_FUNC_SDRD = 0x32 */      /* 0b00100000 */
  /* ASPMETA_FUNC_SDWT = 0x64 */      /* 0b01000000 */
  struct intMbs_s  SD_RW_SECTOR_ADD;  //byte[184]
  struct intMbs_s  SD_RW_SECTOR_LEN;  //byte[188]
  
  unsigned char available[324];
};

struct aspMetaMass_s{
    int massIdx;
    int massUsed;
    int massMax;
    int massGap;
    int massRecd;
    int massStart;
    char *masspt;
};

struct mainRes_s{
    int sid[6];
    int sfm[2];
    int smode;
    uint32_t opTable[OPT_SIZE];
    struct virtualReg_s regTable[128];
    struct machineCtrl_s mchine;
    // 3 pipe
    struct pipe_s pipedn[7];
    struct pipe_s pipeup[7];
    // data mode share memory
    struct shmem_s dataRx;
    struct shmem_s dataTx; /* we don't have data mode Tx, so use it as cmdRx for spi1 */
    // command mode share memory
    struct shmem_s cmdRx; /* cmdRx for spi0 */
    struct shmem_s cmdTx;
    
    struct aspMetaData_s *metaout;
    struct aspMetaData_s *metain;
    struct aspMetaMass_s metaMass;

    struct aspMetaData_s *metaoutDuo;
    struct aspMetaData_s *metainDuo;
    struct aspMetaMass_s metaMassDuo;

    struct aspConfig_s configTable[ASPOP_CODE_MAX];
    
    int sd_init;
    // file save
    FILE *fptn;
    FILE *fs;
    // file log
    FILE *flog;
    FILE *fdat;
    // time measurement
    struct timespec time[2];
    // log buffer
    char log[256];
    struct socket_s socket_r;
    struct socket_s socket_t;
    struct logPool_s plog;
    char filein[128];
    uint32_t scan_length;
    struct cropCoord_s cropCoord;
    struct cropCoord_s cropCoordDuo;
};

typedef int (*fselec)(struct mainRes_s *mrs, struct modersp_s *modersp);

struct fselec_s{
    int  id;
    fselec pfunc;
};

struct procRes_s{
    // pipe
    int spifd;
    int *poptable;
    struct virtualReg_s *pregtb;
    struct pipe_s *ppipedn;
    struct pipe_s *ppipeup;
    struct shmem_s *pdataRx;
    struct shmem_s *pdataTx;
    struct shmem_s *pcmdRx;
    struct shmem_s *pcmdTx;
    struct machineCtrl_s *pmch;
    struct aspMetaData_s *pmetaout;
    struct aspMetaData_s *pmetain;
    struct aspMetaMass_s *pmetaMass;
    struct aspMetaData_s *pmetaoutduo;
    struct aspMetaData_s *pmetainduo;
    struct aspMetaMass_s *pmetaMassduo;
    struct aspConfig_s *pcfgTable;
    
    int *psd_init;
    // data mode share memory
    int cdsz_s;
    int mdsz_s;
    char **dmp_s;
    // command mode share memory
    int ccsz_s;
    int mcsz_s;
    char **cmp_s;
    // save file
    FILE *fs_s;
    // save log file
    FILE *flog_s;
    FILE *fdat_s;
    // time measurement
    struct timespec *tm[2];
    char logs[256];
    struct socket_s *psocket_r;
    struct socket_s *psocket_t;
    struct logPool_s *plogs;
    uint32_t *pscnlen;
    struct cropCoord_s *pcropCoord;
    struct cropCoord_s *pcropCoordDuo;
};


//memory alloc. put in/put out
static char **memory_init(int *sz, int tsize, int csize);
//debug printf
static int print_f(struct logPool_s *plog, char *head, char *str);
static int printf_flush(struct logPool_s *plog, FILE *f);
//time measurement, start /stop
static int time_diff(struct timespec *s, struct timespec *e, int unit);
//file rw open, save to file for debug
static int file_save_get(FILE **fp, char *path1);
//res put in
static int res_put_in(struct procRes_s *rs, struct mainRes_s *mrs, int idx);
//p0: control, monitor, and debug
static int p0(struct mainRes_s *mrs);
static int p0_init(struct mainRes_s *mrs);
static int p0_end(struct mainRes_s *mrs);
//p1: spi0 send
static int p1(struct procRes_s *rs, struct procRes_s *rcmd);
static int p1_init(struct procRes_s *rs);
static int p1_end(struct procRes_s *rs);
//p2: spi0 recv
static int p2(struct procRes_s *rs);
static int p2_init(struct procRes_s *rs);
static int p2_end(struct procRes_s *rs);
//p3: spi1 recv
static int p3(struct procRes_s *rs);
static int p3_init(struct procRes_s *rs);
static int p3_end(struct procRes_s *rs);
//p4: socket send
static int p4(struct procRes_s *rs);
static int p4_init(struct procRes_s *rs);
static int p4_end(struct procRes_s *rs);
//p5: socket recv
static int p5(struct procRes_s *rs, struct procRes_s *rcmd);
static int p5_init(struct procRes_s *rs);
static int p5_end(struct procRes_s *rs);

static int pn_init(struct procRes_s *rs);
static int pn_end(struct procRes_s *rs);
//IPC wrap
static int rs_ipc_put(struct procRes_s *rs, char *str, int size);
static int rs_ipc_get(struct procRes_s *rs, char *str, int size);
static int mrs_ipc_put(struct mainRes_s *mrs, char *str, int size, int idx);
static int mrs_ipc_get(struct mainRes_s *mrs, char *str, int size, int idx);
static int mtx_data(int fd, uint8_t *rx_buff, uint8_t *tx_buff, int num, int pksz, int maxsz);

static int ring_buf_init(struct shmem_s *pp);
static int ring_buf_get_dual(struct shmem_s *pp, char **addr, int sel);
static int ring_buf_set_last_dual(struct shmem_s *pp, int size, int sel);
static int ring_buf_prod_dual(struct shmem_s *pp, int sel);
static int ring_buf_cons_dual(struct shmem_s *pp, char **addr, int sel);
static int ring_buf_get(struct shmem_s *pp, char **addr);
static int ring_buf_set_last(struct shmem_s *pp, int size);
static int ring_buf_prod(struct shmem_s *pp);
static int ring_buf_cons(struct shmem_s *pp, char **addr, int *len);
static int ring_buf_info_len(struct shmem_s *pp);
static int shmem_from_str(char **addr, char *dst, char *sz);
static int shmem_dump(char *src, int size);
static int shmem_pop_send(struct mainRes_s *mrs, char **addr, int seq, int p);
static int shmem_rlt_get(struct mainRes_s *mrs, int seq, int p);
static int stspy_01(struct psdata_s *data);
static int stspy_02(struct psdata_s *data);
static int stspy_03(struct psdata_s *data);
static int stspy_04(struct psdata_s *data);
static int stspy_05(struct psdata_s *data);
static int stbullet_01(struct psdata_s *data);
static int stbullet_02(struct psdata_s *data);
static int stbullet_03(struct psdata_s *data);
static int stbullet_04(struct psdata_s *data);
static int stbullet_05(struct psdata_s *data);
static int stlaser_01(struct psdata_s *data);
static int stlaser_02(struct psdata_s *data);
static int stlaser_03(struct psdata_s *data);
static int stlaser_04(struct psdata_s *data);
static int stlaser_05(struct psdata_s *data);
static int stauto_01(struct psdata_s *data);
static int stauto_02(struct psdata_s *data);
static int stauto_03(struct psdata_s *data);
static int stauto_04(struct psdata_s *data);
static int stauto_05(struct psdata_s *data);
static int stauto_06(struct psdata_s *data);
static int stauto_07(struct psdata_s *data);
static int stauto_08(struct psdata_s *data);
static int stauto_09(struct psdata_s *data);
static int stauto_10(struct psdata_s *data);
static int stauto_11(struct psdata_s *data);
static int stauto_12(struct psdata_s *data);
static int stauto_13(struct psdata_s *data);
static int stauto_14(struct psdata_s *data);
static int stauto_15(struct psdata_s *data);
static int stauto_16(struct psdata_s *data);
static int stauto_17(struct psdata_s *data);
static int stauto_18(struct psdata_s *data);
static int stauto_19(struct psdata_s *data);
static int stauto_20(struct psdata_s *data);
static uint32_t lsb2Msb(struct intMbs_s *msb, uint32_t lsb);
static uint32_t msb2lsb(struct intMbs_s *msb);

static int aspMetaMassSample(struct aspMetaMass_s *mass)
{
    char *pbuf;
    char sample[128];
#if (CROP_SAMPLE_SIZE == 35)
    char spox[] = "/mnt/mmc2/crop13/xpos_%.2d.bin";
#elif (CROP_SAMPLE_SIZE == 4)
    char spox[] = "/mnt/mmc2/crop4/xpos_%.2d.bin";
#else
    char spox[] = "/mnt/mmc2/crop4/xpos_%.2d.bin";
#endif
    int idx=0, ret=0, fileLen=0, maxSize=0;
    int readLen=0;
    FILE *fp;
#if CROP_SAMPLE_SIZE
    idx = mass->massIdx % CROP_SAMPLE_SIZE;
#else
    idx = 0;
#endif

    pbuf = mass->masspt;
    maxSize = mass->massMax;

    sprintf(sample, spox, idx);

    fp = fopen(sample, "r");

    ret = fseek(fp, 0, SEEK_END);
    if (ret) {
        printf("[mass] file seek failed!! ret:%d \n", ret);
        return -1;        
    } 

    fileLen = ftell(fp);
    printf("[mass] file [%s] size: %d, idx: %d \n", sample, fileLen, idx);

    ret = fseek(fp, 0, SEEK_SET);
    if (ret) {
        printf("[mass] file seek failed!! ret:%d \n", ret);
        return -2;        
    }

    if (fileLen > maxSize) {
        readLen = maxSize;
    } else {
        readLen = fileLen;
    }

    ret = fread(pbuf, 1, readLen, fp);
    if (ret != readLen) {
        printf("[mass] WARNING!!! read file size:%d, read:%d, total:%d", ret, readLen, fileLen);  
    }

    msync(pbuf, readLen, MS_SYNC);
    
    mass->massUsed = readLen;
    
    return readLen;
}

static int aspMetaMassCons(struct aspMetaMass_s *mass, int start, int end)
{
#define RAW_W 4320
#define RAW_H  6992
    unsigned int *daddr;
    char *laddr;
    int max=0, ret=0, buffUsed=0, buffMax=0;
    int rcord[4];
    int sx=0, sy=0;
    unsigned short edlf=0, edrt;
    unsigned int edtot=0;
    int msStart=0, msGap;
    
    if (!mass) return -1;
    
    daddr = (unsigned int*)mass->masspt;
    if (!daddr) return -2;

    buffMax = mass->massMax;
    msStart = mass->massStart;
    msGap = mass->massGap;
    if ((msStart == 0) || (msStart > 1000)) {
        msStart = 12;
    }

    if ((msGap == 0) || (msGap > 32)) {
        msGap = 8;
    }

    max = (RAW_W * RAW_H) / 8;
    laddr = 0;
    laddr = malloc(max);
    if (!laddr) {
        printf("[mass] allocate memory size %d falied!!! ret:%d \n", max, laddr);
        return -1;
    }

    memset(laddr, 0, max);

    rcord[0] = 100;
    rcord[1] = 100;
    rcord[2] = 900;
    rcord[3] = 100;

    ret = tiffDrawLine(laddr, rcord, RAW_W, RAW_H, max);                    

    rcord[0] = 900;
    rcord[1] = 100;
    rcord[2] = 900;
    rcord[3] = 900;

    ret = tiffDrawLine(laddr, rcord, RAW_W, RAW_H, max);                    

    rcord[0] = 900;
    rcord[1] = 900;
    rcord[2] = 100;
    rcord[3] = 900;

    ret = tiffDrawLine(laddr, rcord, RAW_W, RAW_H, max);                    

    rcord[0] = 100;
    rcord[1] = 900;
    rcord[2] = 100;
    rcord[3] = 100;

    ret = tiffDrawLine(laddr, rcord, RAW_W, RAW_H, max);                    

    rcord[0] = 1200;
    rcord[1] = 100;
    rcord[2] = 1200;
    rcord[3] = 900;

    ret = tiffDrawLine(laddr, rcord, RAW_W, RAW_H, max);                    

    msync(laddr, max, MS_SYNC);

    for (sy = 0; sy < RAW_H; sy+=msGap) {
        edlf=0; edrt=0; edtot=0;
        for (sx = msStart; sx < (RAW_W - 8); sx++) {
            ret = tiffGetDot(laddr, sx, sy, RAW_W, RAW_H, max);
            if (ret != 0) {
                edlf = randomGen(sx - 8, sx + 8);
                printf("[mass] get dot (%d, %d) = %d !!! \n", sx, sy, edlf);

                break;
            }
        }

        sx++;
        
        for (; sx < (RAW_W - 8); sx++) {
            ret = tiffGetDot(laddr, sx, sy, RAW_W, RAW_H, max);
            if (ret != 0) {
                edrt = randomGen(sx - 8, sx + 8);
                printf("[mass] get dot (%d, %d) = %d !!! \n", sx, sy, edrt);

                break;
            }
        }

        edtot = edlf;
        edtot = (edtot << 16) | edrt;
        
        //*daddr = edtot;
        lsb2Msb((struct intMbs_s *)daddr, edtot);
        daddr++;
        buffUsed += 4;

        if (buffUsed > buffMax) break;
    }

    mass->massUsed = buffUsed;

    free(laddr);
    return buffUsed;
}

static uint32_t lsb2Msb(struct intMbs_s *msb, uint32_t lsb)
{
    uint32_t org=0;
    int i=4;

    org = lsb;

    while (i) {
        i--;
        msb->d[i] = lsb & 0xff;

        //printf("[%d] :0x%.2x -> 0x%.2x \n", i, lsb & 0xff, msb->d[i]);
        
        lsb = lsb >> 8;
    }

    //printf("lsb2Msb() lsb:0x%.8x -> msb:0x%.8x \n", org, msb->n);

    return msb->n;
}

static uint32_t msb2lsb(struct intMbs_s *msb)
{
    uint32_t lsb=0;
    int i=0;

    while (i < 4) {
        lsb = lsb << 8;
        
        lsb |= msb->d[i];
        
        //printf("[%d] :0x%.2x <- 0x%.2x \n", i, lsb & 0xff, msb->d[i]);
        
        i++;
    }

    //printf("msb2lsb() msb:0x%.8x -> lsb:0x%.8x \n", msb->n, lsb);
    
    return lsb;
}

static int aspMetaRelease(unsigned int funcbits, struct mainRes_s *mrs, struct procRes_s *rs) 
{
    int i=0, act=0;
    struct intMbs_s *pt=0;
    struct aspMetaData_s *pmeta;
    struct aspMetaMass_s *pmass;
    struct SdAddrs_s *sds, *sdn;
    int opSt=0, opEd=0;
    int istr=0, iend=0, idx=0, ret=0;
    char *pvdst=0, *pvend=0;
    unsigned char linGap, linStart;
    unsigned short linRec;
    unsigned int val;
    uint32_t secStr=0, secLen=0;
    struct aspConfig_s *pct=0, *pdt=0;
    
    if ((!mrs) && (!rs)) return -1;
    
    if (mrs) {
        sds = &mrs->mchine.sdst;
        sdn = &mrs->mchine.sdln;

        pmeta = mrs->metain;
        pmass = &mrs->metaMass;

        pct = mrs->configTable;
    } else {
        sds = &rs->pmch->sdst;
        sdn = &rs->pmch->sdln;

        pmeta = rs->pmetain;
        pmass = rs->pmetaMass;

        pct = rs->pcfgTable;
    }
    
    msync(pmeta, sizeof(struct aspMetaData_s), MS_SYNC);
    msync(pct, sizeof(struct aspConfig_s) * ASPOP_CODE_MAX, MS_SYNC);
        
    if ((pmeta->ASP_MAGIC[0] != 0x20) || (pmeta->ASP_MAGIC[1] != 0x14)) {
        printf("Error!!! magic number: 0x%.2x 0x%.2x \n", pmeta->ASP_MAGIC[0], pmeta->ASP_MAGIC[1]);
        return -2;
    } else {
        printf("[meta] release, magic number: 0x%.2x 0x%.2x \n", pmeta->ASP_MAGIC[0], pmeta->ASP_MAGIC[1]);
    }
    
    if (funcbits == ASPMETA_FUNC_NONE) return -3;

    if (funcbits & ASPMETA_FUNC_CONF) {

        opSt = OP_FFORMAT;
        opEd = OP_SUPBACK;
        
        istr = ASPOP_FILE_FORMAT;
        iend = ASPOP_SUP_SAVE;
        
        pvdst = &pmeta->FILE_FORMAT;
        pvend = &pmeta->SUP_WRITEBK;

        for (idx = istr; idx <= iend; idx++) {
            if (pct[idx].opCode == opSt) {
            
                //*pvdst = pct[idx].opValue & 0xff;
                pct[idx].opValue = *pvdst & 0xff;
                pct[idx].opStatus = ASPOP_STA_CON;
                printf("[meta] 0x%.2x = 0x%.2x (0x%.2x)\n", pct[idx].opCode, pct[idx].opValue, pct[idx].opStatus);

                pvdst++;
                opSt++;
            }

            if (pvend < pvdst) {
                 break;
            }
            
            if (opEd < opSt) {
                 break;
            }
        }

        opSt = OP_FUNCTEST_00;
        opEd = OP_FUNCTEST_15;

        istr = ASPOP_FUNTEST_00;
        iend = ASPOP_FUNTEST_15;
        
        pvdst = &pmeta->OP_FUNC_00;
        pvend = &pmeta->OP_FUNC_15;

        for (idx = istr; idx <= iend; idx++) {
            if (pct[idx].opCode == opSt) {
            
                //*pvdst = pct[idx].opValue & 0xff;
                pct[idx].opValue = *pvdst & 0xff;
                pct[idx].opStatus = ASPOP_STA_CON;
                printf("[meta] 0x%.2x = 0x%.2x (0x%.2x)\n", pct[idx].opCode, pct[idx].opValue, pct[idx].opStatus);

                pvdst++;
                opSt++;
            }

            if (pvend < pvdst) {
                 break;
            }
            if (opEd < opSt) {
                 break;
            }
        }

        opSt = OP_BLEEDTHROU_ADJUST;
        opEd = OP_BLACKWHITE_THSHLD;

        istr = ASPOP_BLEEDTHROU_ADJUST;
        iend = ASPOP_BLACKWHITE_THSHLD;
        
        pvdst = &pmeta->BLEEDTHROU_ADJUST;
        pvend = &pmeta->BLACKWHITE_THSHLD;

        for (idx = istr; idx <= iend; idx++) {
            if (pct[idx].opCode == opSt) {
            
                //*pvdst = pct[idx].opValue & 0xff;
                pct[idx].opValue = *pvdst & 0xff;
                pct[idx].opStatus = ASPOP_STA_CON;
                printf("[meta] 0x%.2x = 0x%.2x (0x%.2x)\n", pct[idx].opCode, pct[idx].opValue, pct[idx].opStatus);

                pvdst++;
                opSt++;
            }

            if (pvend < pvdst) {
                 break;
            }
            if (opEd < opSt) {
                 break;
            }
        }
        
    }
    
    if (funcbits & ASPMETA_FUNC_CROP) {
    }

    if (funcbits & ASPMETA_FUNC_IMGLEN) {
    }

    if (funcbits & ASPMETA_FUNC_SDFREE) {
    
    }

    if (funcbits & ASPMETA_FUNC_SDUSED) {
    
    }

    if (funcbits & ASPMETA_FUNC_SDRD) {
        pt = &pmeta->SD_RW_SECTOR_ADD;
        secStr = msb2lsb(pt);
        pt = &pmeta->SD_RW_SECTOR_LEN;
        secLen = msb2lsb(pt);

        sds->n = secStr;
        sdn->n = secLen;

        printf("read secStr: %d, secLen: %d \n", secStr, secLen);
        act |= ASPMETA_FUNC_SDRD;
    }

    if (funcbits & ASPMETA_FUNC_SDWT) {
        pt = &pmeta->SD_RW_SECTOR_ADD;
        secStr = msb2lsb(pt);
        pt = &pmeta->SD_RW_SECTOR_LEN;
        secLen = msb2lsb(pt);

        sds->n = secStr;
        sdn->n = secLen;

        printf("read secStr: %d, secLen: %d \n", secStr, secLen);

        act |= ASPMETA_FUNC_SDWT;
    }

    return act;
}

static int aspMetaClear(struct mainRes_s *mrs, struct procRes_s *rs, int out) 
{
    struct aspMetaData_s *pmeta;
    
    if ((!mrs) && (!rs)) return -1;
    
    if (mrs) {
        if (out) {
            pmeta = mrs->metaout;
        } else {
            pmeta = mrs->metain;        
        }
    } else {
        if (out) {
            pmeta = rs->pmetaout;
        } else {
            pmeta = rs->pmetain;
        }
    }

    memset(pmeta, 0, sizeof(struct aspMetaData_s));
    msync(pmeta, sizeof(struct aspMetaData_s), MS_SYNC);
    
    return 0;
}

static int aspMetaBuild(unsigned int funcbits, struct mainRes_s *mrs, struct procRes_s *rs) 
{
    int xposIdx=0, xparm=0;
#if (CROP_SAMPLE_SIZE == 35)
    uint32_t xposParm[CROP_SAMPLE_SIZE] = 
                                           {0x080700ce, 0x080700b9, 0x080700ce, 0x080700ce, 0x080700ce, 
                                            0x080700ce, 0x080700ce, 0x080f00ce, 0x080700ce, 0x080700ce, 
                                            0x080700ce, 0x080700ce, 0x080700ce, 0x080700ce, 0x080700ce, 
                                            0x080700ce, 0x080700ce, 0x080700a2, 0x080700f6, 0x080f00bc, 
                                            0x080700fe, 0x1007004a, 0x1007003b, 0x10070039, 0x100f0045, 
                                            0x10070048, 0x10070035, 0x10070043, 0x10070044, 0x100f0043,
                                            0x2008003d, 0x2008003d, 0x2008003e, 0x2008003d, 0x2010003c};
#elif (CROP_SAMPLE_SIZE == 4)
    uint32_t xposParm[4] = {0x080300ef, 0x080b0098, 0x08070094, 0x080700a0};
#else 
    uint32_t xposParm[4] = {0,0,0,0};
#endif
    uint32_t tbits=0;
    unsigned int *psrc;
    struct intMbs_s *pdst;
    uint32_t val=0, scan_len=0;
    int opSt=0, opEd=0, ret=0;
    int istr=0, iend=0, idx=0;
    struct aspMetaData_s *pmeta;
    struct cropCoord_s *pCrop;
    struct aspMetaMass_s *mass;
    char *pvdst=0, *pvend=0, *pch=0;
    struct aspConfig_s *pct=0, *pdt=0;

    if ((!mrs) && (!rs)) return -1;
    
    if (mrs) {
        pmeta = mrs->metaout;
        pCrop = &(mrs->cropCoord);
        scan_len = mrs->scan_length;
        mass = &(mrs->metaMass);
        pct = mrs->configTable;
    } else {
        pmeta = rs->pmetaout;
        pCrop = rs->pcropCoord;
        scan_len = *(rs->pscnlen);
        mass = rs->pmetaMass;
        pct = rs->pcfgTable;
    }

    msync(pCrop, 18 * sizeof(struct cropCoord_s), MS_SYNC);

    if (funcbits == ASPMETA_FUNC_NONE) {
        /*append mass points*/
#if RANDOM_CROP_COORD
        ret = aspMetaMassCons(mass, 0, 0);        
#else
        ret = aspMetaMassSample(mass);
#endif

        if (ret > 0) {
            printf("dump meta mass: \n");
            msync(mass->masspt, ret, MS_SYNC);
            shmem_dump(mass->masspt, ret);
        } else {
            printf("Error!!! build meta mass failed!!! \n");
        }  
    }

    if (funcbits & ASPMETA_FUNC_CONF) {

    }
    
    if (funcbits & ASPMETA_FUNC_CROP) {
        psrc = pCrop->CROP_COOD_01;
        pdst = &(pmeta->CROP_POS_1);
        for (idx = 0; idx < 18; idx++) {
            val = (psrc[0] & 0xffff) << 16;
            val |= psrc[1] & 0xffff;
            
            //*pdst = val;
            lsb2Msb(pdst, val);
            
            pdst ++;
            psrc += 2;
        }

        xposIdx = mass->massIdx;
#if CROP_SAMPLE_SIZE
        xparm = xposParm[xposIdx % CROP_SAMPLE_SIZE];
#else
        xparm = xposParm[0];
#endif
        pmeta->YLine_Gap = (xparm >> 24) & 0xff;
        pmeta->Start_YLine_No = (xparm >> 16) & 0xff;

        pch = (char *)&(pmeta->YLines_Recorded);

        val = xparm & 0xffff;

/*        
        if (((mass->massIdx + 1) % 7) != 0) {
            val = (xparm & 0xffff) << 16;
        } else {
            val = 0;
        }
*/
        
        if (((mass->massIdx + 1) % 4) == 0) {
            val = 1;
        }
            
        printf("######### index: [%d] val: %d !!! #########\n", mass->massIdx, val);
        
        ret = val;

        pch[0] = val >> 8;
        pch[1] = val & 0xff;
        
        //lsb2Msb(pdst, val);        
    }

    if (funcbits & ASPMETA_FUNC_IMGLEN) {
        pdst = &(pmeta->SCAN_IMAGE_LEN);
        scan_len = pct[ASPOP_IMG_LEN].opValue;
        
        //lsb2Msb(pdst, 0);
        lsb2Msb(pdst, scan_len);
        //lsb2Msb(pdst, 3496); //5400 //3496

        ret = scan_len;
    }

    if (funcbits & ASPMETA_FUNC_SDFREE) {
    
    }

    if (funcbits & ASPMETA_FUNC_SDUSED) {
    
    }

    if (funcbits & ASPMETA_FUNC_SDRD) {
    
    }

    if (funcbits & ASPMETA_FUNC_SDWT) {
    
    }

    pmeta->ASP_MAGIC[0] = 0x20;
    pmeta->ASP_MAGIC[1] = 0x14;

    //pmeta->ASP_MAGIC[0] = 0xab;
    //pmeta->ASP_MAGIC[1] = 0xcd;
    
    //pmeta->FUNC_BITS |= funcbits;
    tbits = msb2lsb(&pmeta->FUNC_BITS);
    tbits |= funcbits;
    lsb2Msb(&pmeta->FUNC_BITS, tbits);
    
    msync(pmeta, sizeof(struct aspMetaData_s), MS_SYNC);
    
    return ret;
}

static int aspMetaBuildDuo(unsigned int funcbits, struct mainRes_s *mrs, struct procRes_s *rs) 
{
    int xposIdx=0, xparm=0;
#if (CROP_SAMPLE_SIZE == 35)
    uint32_t xposParm[CROP_SAMPLE_SIZE] = 
                                           {0x080700ce, 0x080700b9, 0x080700ce, 0x080700ce, 0x080700ce, 
                                            0x080700ce, 0x080700ce, 0x080f00ce, 0x080700ce, 0x080700ce, 
                                            0x080700ce, 0x080700ce, 0x080700ce, 0x080700ce, 0x080700ce, 
                                            0x080700ce, 0x080700ce, 0x080700a2, 0x080700f6, 0x080f00bc, 
                                            0x080700fe, 0x1007004a, 0x1007003b, 0x10070039, 0x100f0045, 
                                            0x10070048, 0x10070035, 0x10070043, 0x10070044, 0x100f0043,
                                            0x2008003d, 0x2008003d, 0x2008003e, 0x2008003d, 0x2010003c};
#elif (CROP_SAMPLE_SIZE == 4)
    uint32_t xposParm[4] = {0x080300ef, 0x080b0098, 0x08070094, 0x080700a0};
#else 
    uint32_t xposParm[4] = {0,0,0,0};
#endif
    uint32_t tbits=0;
    unsigned int *psrc;
    struct intMbs_s *pdst;
    uint32_t val=0, scan_len=0;
    int opSt=0, opEd=0, ret=0;
    int istr=0, iend=0, idx=0;
    struct aspMetaData_s *pmetaduo;
    struct cropCoord_s *pCropDuo;
    struct aspMetaMass_s *mass, *massduo;
    char *pvdst=0, *pvend=0, *pch=0;
    struct aspConfig_s *pct=0, *pdt=0;
    
    if ((!mrs) && (!rs)) return -1;
    
    if (mrs) {
        pmetaduo = mrs->metaoutDuo;
        pCropDuo= &(mrs->cropCoordDuo);        
        scan_len = mrs->scan_length;
        mass = &(mrs->metaMass);
        massduo = &(mrs->metaMassDuo);
        pct = mrs->configTable;
    } else {
        pmetaduo = rs->pmetaoutduo;
        pCropDuo = rs->pcropCoordDuo;
        scan_len = *(rs->pscnlen);
        mass = rs->pmetaMass;
        massduo = rs->pmetaMassduo;
        pct = rs->pcfgTable;
    }

    msync(pCropDuo, 18 * sizeof(struct cropCoord_s), MS_SYNC);

    if (funcbits == ASPMETA_FUNC_NONE) {
        /*append mass points*/
#if RANDOM_CROP_COORD
        ret = aspMetaMassCons(massduo, 0, 0);        
#else
        ret = aspMetaMassSample(massduo);
#endif

        if (ret > 0) {
            printf("dump meta mass: \n");
            msync(massduo->masspt, ret, MS_SYNC);
            shmem_dump(massduo->masspt, ret);
        } else {
            printf("Error!!! build meta mass failed!!! \n");
        }  
    }

    if (funcbits & ASPMETA_FUNC_CONF) {

    }
    
    if (funcbits & ASPMETA_FUNC_CROP) {
        psrc = pCropDuo->CROP_COOD_01;
        pdst = &(pmetaduo->CROP_POS_1);
        for (idx = 0; idx < 18; idx++) {
            val = (psrc[0] & 0xffff) << 16;
            val |= psrc[1] & 0xffff;
            
            //*pdst = val;
            lsb2Msb(pdst, val);
            
            pdst ++;
            psrc += 2;
        }

        xposIdx = massduo->massIdx;
#if CROP_SAMPLE_SIZE
        xparm = xposParm[xposIdx % CROP_SAMPLE_SIZE];
#else
        xparm = xposParm[0];
#endif
        pmetaduo->YLine_Gap = (xparm >> 24) & 0xff;
        pmetaduo->Start_YLine_No = (xparm >> 16) & 0xff;

        pch = (char *)&(pmetaduo->YLines_Recorded);

        val = xparm & 0xffff;

        if (((mass->massIdx + 1) % 4) == 0) {
            val = 1;
        }

        ret = val;
        
        pch[0] = val >> 8;
        pch[1] = val & 0xff;        
        
        //lsb2Msb(pdst, val);        
    }

    if (funcbits & ASPMETA_FUNC_IMGLEN) {
        pdst = &(pmetaduo->SCAN_IMAGE_LEN);
        scan_len = pct[ASPOP_IMG_LEN_DUO].opValue;
        
        //lsb2Msb(pdst, 0);
        lsb2Msb(pdst, scan_len);
        //lsb2Msb(pdst, 4800);
    }

    if (funcbits & ASPMETA_FUNC_SDFREE) {
    
    }

    if (funcbits & ASPMETA_FUNC_SDUSED) {
    
    }

    if (funcbits & ASPMETA_FUNC_SDRD) {
    
    }

    if (funcbits & ASPMETA_FUNC_SDWT) {
    
    }

    pmetaduo->ASP_MAGIC[0] = 0x20;
    pmetaduo->ASP_MAGIC[1] = 0x14;

    //pmeta->ASP_MAGIC[0] = 0xab;
    //pmeta->ASP_MAGIC[1] = 0xcd;
    
    //pmeta->FUNC_BITS |= funcbits;
    tbits = msb2lsb(&pmetaduo->FUNC_BITS);
    tbits |= funcbits;
    lsb2Msb(&pmetaduo->FUNC_BITS, tbits);
    
    msync(pmetaduo, sizeof(struct aspMetaData_s), MS_SYNC);
    
    return 0;
}
static int dbgMeta(unsigned int funcbits, struct aspMetaData_s *pmeta) 
{
    msync(pmeta, sizeof(struct aspMetaData_s), MS_SYNC);
    printf("********************************************\n");
    printf("[meta] debug print , funcBits: 0x%.8x, magic[0]: 0x%.2x magic[1]: 0x%.2x \n", funcbits, pmeta->ASP_MAGIC[0], pmeta->ASP_MAGIC[1]);

    if ((pmeta->ASP_MAGIC[0] != 0x20) || (pmeta->ASP_MAGIC[1] != 0x14)) {
        printf("[meta] Error!!! magic[0]: 0x%.2x magic[1]: 0x%.2x \n", pmeta->ASP_MAGIC[0], pmeta->ASP_MAGIC[1]);
        return -2;
    }
    
    if (funcbits == ASPMETA_FUNC_NONE) return -3;

    if (funcbits & ASPMETA_FUNC_CONF) {
        printf("[meta]__ASPMETA_FUNC_CONF__(0x%x & 0x%x = 0x%x)\n", funcbits, ASPMETA_FUNC_CONF, (funcbits & ASPMETA_FUNC_CONF));
        printf("[meta]FILE_FORMAT: 0x%.2x    \n",pmeta->FILE_FORMAT     );          //0x31
        printf("[meta]COLOR_MODE: 0x%.2x      \n",pmeta->COLOR_MODE      );        //0x32
        printf("[meta]COMPRESSION_RATE: 0x%.2x\n",pmeta->COMPRESSION_RATE);   //0x33
        printf("[meta]RESOLUTION: 0x%.2x      \n",pmeta->RESOLUTION      );         //0x34
        printf("[meta]SCAN_GRAVITY: 0x%.2x    \n",pmeta->SCAN_GRAVITY    );       //0x35
        printf("[meta]CIS_MAX_Width: 0x%.2x   \n",pmeta->CIS_MAX_WIDTH   );        //0x36
        printf("[meta]WIDTH_ADJUST_H: 0x%.2x  \n",pmeta->WIDTH_ADJUST_H  );     //0x37
        printf("[meta]WIDTH_ADJUST_L: 0x%.2x  \n",pmeta->WIDTH_ADJUST_L  );      //0x38
        printf("[meta]SCAN_LENGTH_H: 0x%.2x   \n",pmeta->SCAN_LENGTH_H   );      //0x39
        printf("[meta]SCAN_LENGTH_L: 0x%.2x   \n",pmeta->SCAN_LENGTH_L   );       //0x3a
        printf("[meta]INTERNAL_IMG: 0x%.2x    \n",pmeta->INTERNAL_IMG    );         //0x3b
        printf("[meta]AFE_IC_SELEC: 0x%.2x    \n",pmeta->AFE_IC_SELEC    );         //0x3c
        printf("[meta]EXTNAL_PULSE: 0x%.2x    \n",pmeta->EXTNAL_PULSE    );         //0x3d
        printf("[meta]SUP_WRITEBK: 0x%.2x     \n",pmeta->SUP_WRITEBK     );       //0x3e
        printf("[meta]OP_FUNC_00: 0x%.2x      \n",pmeta->OP_FUNC_00      );     //0x70
        printf("[meta]OP_FUNC_01: 0x%.2x      \n",pmeta->OP_FUNC_01      );     //0x71
        printf("[meta]OP_FUNC_02: 0x%.2x      \n",pmeta->OP_FUNC_02      );     //0x72
        printf("[meta]OP_FUNC_03: 0x%.2x      \n",pmeta->OP_FUNC_03      );     //0x73
        printf("[meta]OP_FUNC_04: 0x%.2x      \n",pmeta->OP_FUNC_04      );     //0x74
        printf("[meta]OP_FUNC_05: 0x%.2x      \n",pmeta->OP_FUNC_05      );     //0x75
        printf("[meta]OP_FUNC_06: 0x%.2x      \n",pmeta->OP_FUNC_06      );     //0x76
        printf("[meta]OP_FUNC_07: 0x%.2x      \n",pmeta->OP_FUNC_07      );     //0x77
        printf("[meta]OP_FUNC_08: 0x%.2x      \n",pmeta->OP_FUNC_08      );     //0x78
        printf("[meta]OP_FUNC_09: 0x%.2x      \n",pmeta->OP_FUNC_09      );     //0x79
        printf("[meta]OP_FUNC_10: 0x%.2x      \n",pmeta->OP_FUNC_10      );     //0x7A
        printf("[meta]OP_FUNC_11: 0x%.2x      \n",pmeta->OP_FUNC_11      );     //0x7B
        printf("[meta]OP_FUNC_12: 0x%.2x      \n",pmeta->OP_FUNC_12      );     //0x7C
        printf("[meta]OP_FUNC_13: 0x%.2x      \n",pmeta->OP_FUNC_13      );     //0x7D
        printf("[meta]OP_FUNC_14: 0x%.2x      \n",pmeta->OP_FUNC_14      );     //0x7E
        printf("[meta]OP_FUNC_15: 0x%.2x      \n",pmeta->OP_FUNC_15      );     //0x7F  
        printf("[meta]BLEEDTHROU_ADJUST: 0x%.2x      \n",pmeta->BLEEDTHROU_ADJUST);
        printf("[meta]BLACKWHITE_THSHLD: 0x%.2x      \n",pmeta->BLACKWHITE_THSHLD);
    }
    
    if (funcbits & ASPMETA_FUNC_CROP) {
        printf("[meta]__ASPMETA_FUNC_CROP__(0x%x & 0x%x = 0x%x)\n", funcbits, ASPMETA_FUNC_CROP, (funcbits & ASPMETA_FUNC_CROP));
        printf("[meta]CROP_POSX_01: %d, %d\n", msb2lsb(&pmeta->CROP_POS_1) >> 16, msb2lsb(&pmeta->CROP_POS_1) & 0xffff);                      //byte[68]
        printf("[meta]CROP_POSX_02: %d, %d\n", msb2lsb(&pmeta->CROP_POS_2) >> 16, msb2lsb(&pmeta->CROP_POS_2) & 0xffff);                      //byte[72]
        printf("[meta]CROP_POSX_03: %d, %d\n", msb2lsb(&pmeta->CROP_POS_3) >> 16, msb2lsb(&pmeta->CROP_POS_3) & 0xffff);                      //byte[76]
        printf("[meta]CROP_POSX_04: %d, %d\n", msb2lsb(&pmeta->CROP_POS_4) >> 16, msb2lsb(&pmeta->CROP_POS_4) & 0xffff);                      //byte[80]
        printf("[meta]CROP_POSX_05: %d, %d\n", msb2lsb(&pmeta->CROP_POS_5) >> 16, msb2lsb(&pmeta->CROP_POS_5) & 0xffff);                      //byte[84]
        printf("[meta]CROP_POSX_06: %d, %d\n", msb2lsb(&pmeta->CROP_POS_6) >> 16, msb2lsb(&pmeta->CROP_POS_6) & 0xffff);                      //byte[88]
        printf("[meta]CROP_POSX_07: %d, %d\n", msb2lsb(&pmeta->CROP_POS_7) >> 16, msb2lsb(&pmeta->CROP_POS_7) & 0xffff);                      //byte[92]
        printf("[meta]CROP_POSX_08: %d, %d\n", msb2lsb(&pmeta->CROP_POS_8) >> 16, msb2lsb(&pmeta->CROP_POS_8) & 0xffff);                      //byte[96]
        printf("[meta]CROP_POSX_09: %d, %d\n", msb2lsb(&pmeta->CROP_POS_9) >> 16, msb2lsb(&pmeta->CROP_POS_9) & 0xffff);                      //byte[100]
        printf("[meta]CROP_POSX_10: %d, %d\n", msb2lsb(&pmeta->CROP_POS_10) >> 16, msb2lsb(&pmeta->CROP_POS_10) & 0xffff);                      //byte[104]
        printf("[meta]CROP_POSX_11: %d, %d\n", msb2lsb(&pmeta->CROP_POS_11) >> 16, msb2lsb(&pmeta->CROP_POS_11) & 0xffff);                      //byte[108]
        printf("[meta]CROP_POSX_12: %d, %d\n", msb2lsb(&pmeta->CROP_POS_12) >> 16, msb2lsb(&pmeta->CROP_POS_12) & 0xffff);                      //byte[112]
        printf("[meta]CROP_POSX_13: %d, %d\n", msb2lsb(&pmeta->CROP_POS_13) >> 16, msb2lsb(&pmeta->CROP_POS_13) & 0xffff);                      //byte[116]
        printf("[meta]CROP_POSX_14: %d, %d\n", msb2lsb(&pmeta->CROP_POS_14) >> 16, msb2lsb(&pmeta->CROP_POS_14) & 0xffff);                      //byte[120]
        printf("[meta]CROP_POSX_15: %d, %d\n", msb2lsb(&pmeta->CROP_POS_15) >> 16, msb2lsb(&pmeta->CROP_POS_15) & 0xffff);                      //byte[124]
        printf("[meta]CROP_POSX_16: %d, %d\n", msb2lsb(&pmeta->CROP_POS_16) >> 16, msb2lsb(&pmeta->CROP_POS_16) & 0xffff);                      //byte[128]
        printf("[meta]CROP_POSX_17: %d, %d\n", msb2lsb(&pmeta->CROP_POS_17) >> 16, msb2lsb(&pmeta->CROP_POS_17) & 0xffff);                      //byte[132]
        printf("[meta]CROP_POSX_18: %d, %d\n", msb2lsb(&pmeta->CROP_POS_18) >> 16, msb2lsb(&pmeta->CROP_POS_18) & 0xffff);                      //byte[136]

    }

    if (funcbits & ASPMETA_FUNC_IMGLEN) {
        printf("[meta]__ASPMETA_FUNC_IMGLEN__(0x%x & 0x%x = 0x%x)\n", funcbits, ASPMETA_FUNC_IMGLEN, (funcbits & ASPMETA_FUNC_IMGLEN));
        printf("[meta]SCAN_IMAGE_LEN: %d\n", msb2lsb(&pmeta->SCAN_IMAGE_LEN));                      //byte[124]        
    }

    if (funcbits & ASPMETA_FUNC_SDFREE) {      
        printf("[meta]__ASPMETA_FUNC_SDFREE__(0x%x & 0x%x = 0x%x)\n", funcbits, ASPMETA_FUNC_SDFREE, (funcbits & ASPMETA_FUNC_SDFREE));
        printf("[meta]FREE_SECTOR_ADD: %d\n", msb2lsb(&pmeta->FREE_SECTOR_ADD));                      //byte[128]            
        printf("[meta]FREE_SECTOR_LEN: %d\n", msb2lsb(&pmeta->FREE_SECTOR_LEN));                      //byte[132]        
    }

    if (funcbits & ASPMETA_FUNC_SDUSED) {
        printf("[meta]__ASPMETA_FUNC_SDUSED__(0x%x & 0x%x = 0x%x)\n", funcbits, ASPMETA_FUNC_SDUSED, (funcbits & ASPMETA_FUNC_SDUSED));
        printf("[meta]USED_SECTOR_ADD: %d\n", msb2lsb(&pmeta->USED_SECTOR_ADD));                      //byte[136]            
        printf("[meta]USED_SECTOR_LEN: %d\n", msb2lsb(&pmeta->USED_SECTOR_LEN));                      //byte[140]        
    }

    if (funcbits & ASPMETA_FUNC_SDRD) {
        printf("[meta]__ASPMETA_FUNC_SDRD__(0x%x & 0x%x = 0x%x)\n", funcbits, ASPMETA_FUNC_SDRD, (funcbits & ASPMETA_FUNC_SDRD));
        printf("[meta]SD_RW_SECTOR_ADD: %d\n", msb2lsb(&pmeta->SD_RW_SECTOR_ADD));                      //byte[144]            
        printf("[meta]SD_RW_SECTOR_LEN: %d\n", msb2lsb(&pmeta->SD_RW_SECTOR_LEN));                      //byte[148]        
    }

    if (funcbits & ASPMETA_FUNC_SDWT) {
        printf("[meta]__ASPMETA_FUNC_SDWT__(0x%x & 0x%x = 0x%x)\n", funcbits, ASPMETA_FUNC_SDWT, (funcbits & ASPMETA_FUNC_SDWT));
        printf("[meta]SD_RW_SECTOR_ADD: %d\n", msb2lsb(&pmeta->SD_RW_SECTOR_ADD));                      //byte[136]            
        printf("[meta]SD_RW_SECTOR_LEN: %d\n", msb2lsb(&pmeta->SD_RW_SECTOR_LEN));                      //byte[140]        
    }

    printf("********************************************\n");
    return 0;
}

static void* aspSalloc(int slen)
{
    char *p=0;
    
    p = mmap(NULL, slen, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    return p;
}

static int tiffClearDot(char *img, int dotx, int doty, int width, int length, int max)
{
    uint8_t *dst, val=0, org=0;
    uint8_t bitSet[8] = {DOT_1, DOT_2, DOT_3, DOT_4, DOT_5, DOT_6, DOT_7, DOT_8};
    int estMax, estWid=0;
    int offsetX=0, offsetY=0, resX=0;
    
    //if (!img) return -1;
    //if (!width) return -3;
    //if (!length) return -4;

    estMax = (width * length) / 8; 

    //if (estMax > max) return -5;

    if (dotx >= width) {
        dotx = width -1;
    }
    
    if (doty >= length) {
        doty = length -1;
    }

    if ((width % 8) == 0) {
        estWid = width / 8;
    } else {
        estWid = (width / 8) + 1;
    }

    resX = dotx % 8;

    offsetY = doty;
    offsetX = dotx / 8;

    dst = img + (offsetX + offsetY*estWid);

    org = *dst;
    
    val = org & (~(bitSet[resX]));

    //printf("(%d, %d)clear dot, org: 0x%.2x, val: 0x%.2x - x:%d, y:%d, res:%d (%d)\n", dotx, doty, org, val, offsetX, offsetY, resX, (offsetX + offsetY*estWid));
    
    *dst = val;

    return val;
}

inline int tiffGetDot(char *img, int dotx, int doty, int width, int length, int max)
{
    uint8_t *dst, val=0, org=0;
    uint8_t bitSet[8] = {DOT_1, DOT_2, DOT_3, DOT_4, DOT_5, DOT_6, DOT_7, DOT_8};
    int estMax, estWid=0;
    int offsetX=0, offsetY=0, resX=0;
    //if (!img) return -1;
    //if (!width) return -3;
    //if (!length) return -4;

    estMax = (width * length) / 8; 

    //if (estMax > max) return -5;

    if (dotx >= width) {
        dotx = width -1;
    }
    
    if (doty >= length) {
        doty = length -1;
    }

    if ((width % 8) == 0) {
        estWid = width / 8;
    } else {
        estWid = (width / 8) + 1;
    }

    resX = dotx % 8;

    offsetY = doty;
    offsetX = dotx / 8;

    dst = img + (offsetX + offsetY*estWid);

    org = *dst;

    val = 0;
    val = org & (bitSet[resX]);

    //printf("(%d, %d)get dot, org: 0x%.2x, val: 0x%.2x - x:%d, y:%d, res:%d (%d)\n", dotx, doty, org, val, offsetX, offsetY, resX, (offsetX + offsetY*estWid));

    return val;
}

int tiffDrawDot(char *img, int dotx, int doty, int width, int length, int max)
{
    uint8_t *dst, val=0, org=0;
    uint8_t bitSet[8] = {DOT_1, DOT_2, DOT_3, DOT_4, DOT_5, DOT_6, DOT_7, DOT_8};
    int estMax, estWid=0;
    int offsetX=0, offsetY=0, resX=0;
    //if (!img) return -1;
    //if (!width) return -3;
    //if (!length) return -4;

    estMax = (width * length) / 8; 

    //if (estMax > max) return -5;

    if (dotx >= width) {
        dotx = width -1;
    }
    
    if (doty >= length) {
        doty = length -1;
    }

    if ((width % 8) == 0) {
        estWid = width / 8;
    } else {
        estWid = (width / 8) + 1;
    }

    resX = dotx % 8;

    offsetY = doty;
    offsetX = dotx / 8;

    dst = img + (offsetX + offsetY*estWid);

    org = *dst;
    
    val = org & (~(bitSet[resX]));
    val |= bitSet[resX];

    //printf("(%d, %d)draw dot, org: 0x%.2x, val: 0x%.2x - x:%d, y:%d, res:%d (%d)\n", dotx, doty, org, val, offsetX, offsetY, resX, (offsetX + offsetY*estWid));
    
    *dst = val;

    return val;
}

int tiffClearLine(char *img, int *cord, int width, int length, int max)
{
    int ret=0, cnt=0;
    int estMax, srcx, srcy, dstx, dsty;
    double stx=0, sty=0, edx=0, edy=0;
    double offx=0, offy=0;
    if (!img) return -1;
    if (!cord) return -2;
    if (!width) return -3;
    if (!length) return -4;

    estMax = (width * length) / 8; 

    if (estMax > max) return -5;

    srcx = cord[0]; 
    srcy = cord[1]; 
    dstx = cord[2];
    dsty = cord[3];
    
    stx = cord[0];
    sty = cord[1];
    edx = cord[2];
    edy = cord[3];

    offx = edx - stx;
    offy = edy - sty;

    if (abs(offx) < 0.0001) {
        offx = 0;
    } else if (abs(offy) < 0.0001) {
        offy = 0;
    } 

    if (abs(offx) > abs(offy)) {
        offy = offy / abs(offx);
        offx = offx / abs(offx);
    } else if (abs(offy) > abs(offx)) {
        offx = offx / abs(offy);
        offy = offy / abs(offy);
    } else {
        if (abs(offy) > 0) {
            offx = offx / abs(offx);
            offy = offy / abs(offy);
        }
    }

    //printf("clear line  (%d, %d) -> (%d, %d)\n", srcx, srcy, dstx, dsty);
    
    while ((srcx != dstx) || (srcy != dsty)) {
         ret = tiffClearDot(img, srcx, srcy, width, length, max);
         cnt++;
     
         stx += offx;
         sty += offy;
         srcx = (int)stx;
         srcy = (int)sty;

         //printf("clear %d, %d - %d\n", srcx, srcy,cnt);
     }
     ret = tiffClearDot(img, srcx, srcy, width, length, max);
     cnt++;
     
    return cnt;
}


int tiffDrawLine(char *img, int *cord, int width, int length, int max)
{
    int ret=0, cnt=0;
    int estMax, srcx, srcy, dstx, dsty;
    double stx=0, sty=0, edx=0, edy=0;
    double offx=0, offy=0;
    if (!img) return -1;
    if (!cord) return -2;
    if (!width) return -3;
    if (!length) return -4;

    estMax = (width * length) / 8; 

    if (estMax > max) return -5;

    srcx = cord[0]; 
    srcy = cord[1]; 
    dstx = cord[2];
    dsty = cord[3];
    
    stx = cord[0];
    sty = cord[1];
    edx = cord[2];
    edy = cord[3];

    offx = edx - stx;
    offy = edy - sty;
    
    if (abs(offx) < 0.0001) {
        offx = 0;
    } else if (abs(offy) < 0.0001) {
        offy = 0;
    } 

    if (abs(offx) > abs(offy)) {
        offy = offy / abs(offx);
        offx = offx / abs(offx);
    } else if (abs(offy) > abs(offx)) {
        offx = offx / abs(offy);
        offy = offy / abs(offy);
    } else {
        if (abs(offy) > 0) {
            offx = offx / abs(offx);
            offy = offy / abs(offy);
        }
    }

    //printf("draw line  (%d, %d) -> (%d, %d) (%f, %f)\n", srcx, srcy, dstx, dsty, offx, offy);
    
    while ((srcx != dstx) || (srcy != dsty)) {
         ret = tiffDrawDot(img, srcx, srcy, width, length, max);
         cnt++;
     
         stx += offx;
         sty += offy;
         srcx = (int)stx;
         srcy = (int)sty;
         //printf("draw %d, %d - %d\n", srcx, srcy,cnt);
     }
     ret = tiffDrawDot(img, srcx, srcy, width, length, max);
     cnt++;
     
    return cnt;
}

int tiffDrawBox(char *img, int *cord, int width, int length, int max)
{
    int estMax, i;
    int stx=0, sty=0, edx=0, edy=0;
    int rcord[4];
    if (!img) return -1;
    if (!cord) return -2;
    if (!width) return -3;
    if (!length) return -4;

    estMax = (width * length) / 8; 

    if (estMax > max) return -5;

    stx = cord[0];
    sty = cord[1];
    edx = cord[2];
    edy = cord[3];

    rcord[0] = stx;
    rcord[1] = sty;
    rcord[2] = edx;
    rcord[3] = edy;

    for (i = stx; i <= edx; i++) {
        rcord[0] = i;
        rcord[2] = i;
        tiffDrawLine(img, rcord, width, length, max);
    }

    return 0;
}

int tiffClearBox(char *img, int *cord, int width, int length, int max)
{
    int estMax, i=0;
    int stx=0, sty=0, edx=0, edy=0;
    int rcord[4];
    if (!img) return -1;
    if (!cord) return -2;
    if (!width) return -3;
    if (!length) return -4;

    estMax = (width * length) / 8; 

    if (estMax > max) return -5;

    stx = cord[0];
    sty = cord[1];
    edx = cord[2];
    edy = cord[3];

    rcord[0] = stx;
    rcord[1] = sty;
    rcord[2] = edx;
    rcord[3] = edy;

    for (i = stx; i <= edx; i++) {
        rcord[0] = i;
        rcord[2] = i;
        tiffClearLine(img, rcord, width, length, max);
    }

    return 0;
}

FILE *find_save(char *dst, char *tmple)
{
    int i;
    FILE *f=0;
    for (i =0; i < 1000; i++) {
        f = 0;
        sprintf(dst, tmple, i);
        f = fopen(dst, "r");
        if (!f) {
            printf("open file [%s]\n", dst);
            break;
        } else {
            //printf("open file [%s] succeed \n", dst);
            fclose(f);
        }
    }
    
    f = fopen(dst, "w");
    return f;
}

inline uint16_t abs_info(struct info16Bit_s *p, uint16_t info)
{
    p->data = info & 0xff;
    p->opcode = (info >> 8) & 0xff;
    //p->seqnum = (info >> 12) & 0x7;
    //p->inout = (info >> 15) & 0x1;

    return info;
}

inline uint16_t pkg_info(struct info16Bit_s *p)
{
    uint16_t info = 0;
    info |= p->data & 0xff;
    info |= (p->opcode & 0xff) << 8;
    //info |= (p->seqnum & 0x7) << 12;
    //info |= (p->inout & 0x1) << 15;

    return info;
}

inline uint32_t abs_result(uint32_t result)
{
    result = result >> 16;
    return (result & 0xff);
}
inline uint32_t emb_result(uint32_t result, uint32_t flag) 
{
    result &= ~0xff0000;
    result |= (flag << 16) & 0xff0000;
    return result;
}

inline uint32_t emb_stanPro(uint32_t result, uint32_t rlt, uint32_t sta, uint32_t pro) 
{
    result &= ~0xffffff;
    result |= ((rlt & 0xff) << 16) | ((sta & 0xff) << 8) | (pro & 0xff);
    return result;
}

inline uint32_t emb_event(uint32_t result, uint32_t flag) 
{
    result &= ~0xff000000;
    result |= (flag & 0xff) << 24;
    return result;
}

inline uint32_t emb_state(uint32_t result, uint32_t flag) 
{
    char str[32];
    static int pre=0;
    pre = result;
    result &= ~0xff00;
    result |= (flag & 0xff) << 8;

    if (pre != result) {
        sprintf(str, "0x%.8x -> 0x%.8x\n", pre, result); 
        print_f(mlogPool, "state", str); 
    }

    return result;
}

inline uint32_t emb_process(uint32_t result, uint32_t flag) 
{
    result &= ~0xff;
    result |= flag & 0xff;
    return result;
}

static int next_spy(struct psdata_s *data)
{
    int pro, rlt, next = -1;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    struct info16Bit_s *t;
    t = &data->rs->pmch->tmp;

    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;
    evt = (data->result >> 8) & 0xff;
	
    //sprintf(str, "next %d-%d %d [0x%x]\n", pro, rlt, evt, data->result); 
    //print_f(mlogPool, "spy", str); 

    tmpRlt = data->result;
    if (rlt == WAIT) {
        next = pro;
    } else if (rlt == NEXT) {
        /* reset pro */  
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);
        switch (pro) {
            case PSSET: /* load file, prepare data and spi */
                //sprintf(str, "PSSET\n"); 
                //print_f(mlogPool, "spy", str); 
                next = PSACT;
                break;
            case PSACT: /* check RDY, and start transmitting */
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "spy", str); 
                //next = PSWT;
                next = PSTSM; /* jump to next stage */
                evt = SPY;
                break;
            case PSWT: /* end the transmitting and back to polling OP_QRY */
                //sprintf(str, "PSWT\n"); 
                //print_f(mlogPool, "spy", str); 
                next = PSRLT;
                break;
            case PSRLT:
                //sprintf(str, "PSRLT\n"); 
                //print_f(mlogPool, "spy", str); 
                next = PSTSM;
                break;
            case PSTSM:
                sprintf(str, "PSTSM - ansp: %d\n", tmpAns); 
                print_f(mlogPool, "spy", str); 
                //next = PSMAX;
                t->opcode = tmpAns & 0xff;
                t->data = (tmpAns >> 8) & 0xff;
                switch (tmpAns & 0xff) {
                case OP_POLL:
                    //t->opcode = tmpAns & 0xff;
                    //t->data = (tmpAns >> 8) & 0xff;
                    next = PSSET; /* get and repeat value */
                    evt = AUTO_A;
                    break;
                case OP_NOTESCAN:
                    switch ((tmpAns >> 8) & 0xff) {
                        case NOTESCAN_OPTION_01:
                        case NOTESCAN_OPTION_02:
                        case NOTESCAN_OPTION_03:
                        case NOTESCAN_OPTION_04:
                        case NOTESCAN_OPTION_05:
                        case NOTESCAN_OPTION_06:
                        case NOTESCAN_OPTION_07:
                            t->opcode = tmpAns & 0xff;
                            t->data = (tmpAns >> 8) & 0xff;
                            next = PSTSM;
                            evt = AUTO_C;
                            break;
                        default:
                            next = PSSET; /* get and repeat value */
                            evt = AUTO_A;
                            break;
                    }
                    break;
                case OP_SINGLE: /* currently support */         
                case OP_MSINGLE:
                case OP_HANDSCAN:
                    switch ((tmpAns >> 8) & 0xff) {
                        case SINSCAN_DUAL_STRM:
                        case SINSCAN_DUAL_SD:
                            t->opcode = tmpAns & 0xff;
                            t->data = (tmpAns >> 8) & 0xff;
                            #if 1
                            next = PSSET; 
                            evt = BULLET;
                            #else
                            //next = PSRLT; /* for OP_SUPBACK */
                            //evt = AUTO_B;
                            #endif
                            break;
                        case SINSCAN_WIFI_ONLY:
                            t->opcode = tmpAns & 0xff;
                            t->data = SINSCAN_WIFI_ONLY;
                            next = PSTSM;
                            evt = AUTO_C;
                            break;
                        case SINSCAN_SD_ONLY:
                            t->opcode = tmpAns & 0xff;
                            t->data = SINSCAN_SD_ONLY;
                            next = PSTSM;
                            evt = AUTO_C;
                            break;
                        case SINSCAN_WIFI_SD:
                            t->opcode = tmpAns & 0xff;
                            t->data = SINSCAN_WIFI_SD;
                            next = PSTSM;
                            evt = AUTO_C;
                            break;
                         default:
                            break;
                    }
                    break;
                case OP_DOUBLE: 
                case OP_MDOUBLE:
                    switch ((tmpAns >> 8) & 0xff) {
                        case DOUSCAN_WIFI_ONLY:
                        case DOUSCAN_WIFI_SD:
                            next = PSACT;
                            evt = AUTO_D;
                            break;
                        default:
                            break;
                    }
                    break;
                case OP_SDRD:                                       
                case OP_SDWT:                                       
                    next = PSACT; /* get and repeat value */
                    evt = AUTO_A;
                    break;
                case OP_STSEC_00:                                  
                case OP_STSEC_01:                                  
                case OP_STSEC_02:                                  
                case OP_STSEC_03:                                  
                    if ((tmpAns >> 16) & 0xff) {
                        next = PSACT; 
                        evt = AUTO_E;
                    
                    } else {
                        next = PSWT; 
                        evt = AUTO_A;
                    }
                    break;
                case OP_STLEN_00:                                  
                case OP_STLEN_01:                                  
                case OP_STLEN_02:                                  
                case OP_STLEN_03:                                  
                    if ((tmpAns >> 16) & 0xff) {
                        next = PSWT; 
                        evt = AUTO_E;
                    } else {
                        next = PSRLT; /* get and repeat value */
                        evt = AUTO_A;
                    }
                    break;
                case OP_FFORMAT:                                  
                case OP_COLRMOD:                                  
                case OP_COMPRAT:                                  
                case OP_RESOLTN:                                  
                case OP_SCANGAV:                                  
                case OP_MAXWIDH:                                  
                case OP_WIDTHAD_H:                                
                case OP_WIDTHAD_L:                                
                case OP_INTERIMG:
                case OP_AFEIC:
                case OP_EXTPULSE:
                    next = PSSET; /* save value */
                    evt = AUTO_H;
                    break;
                case OP_ACTION:
                case OP_SUPBACK:
                case OP_FUNCTEST_00:
                case OP_FUNCTEST_01:
                case OP_FUNCTEST_02:
                case OP_FUNCTEST_03:
                case OP_FUNCTEST_04:
                case OP_FUNCTEST_05:
                case OP_FUNCTEST_06:
                case OP_FUNCTEST_07:
                case OP_FUNCTEST_08:
                case OP_FUNCTEST_09:
                case OP_FUNCTEST_10:
                case OP_FUNCTEST_11:
                case OP_FUNCTEST_12:
                case OP_FUNCTEST_13:
                case OP_FUNCTEST_14:
                case OP_FUNCTEST_15:
                    next = PSSET; /* get and repeat value */
                    evt = AUTO_A;
                    break;
                case OP_SCANLEN_H:                                
                    next = PSWT; 
                    evt = AUTO_G;
                    break;
                case OP_SCANLEN_L:
                    next = PSRLT; 
                    evt = AUTO_G;
                    break;
                case OP_SAVE:
                    next = PSRLT; 
                    evt = AUTO_D;
                    break;
                case OP_SDAT:                                     
                    next = PSSET;
                    evt = AUTO_B;
                    break;
                case OP_FREESEC:
                    next = PSTSM;
                    evt = AUTO_D;
                    break;
                case OP_USEDSEC:
                    next = PSSET;
                    evt = AUTO_E;
                    break;
                case OP_SDINIT:
                    next = PSRLT;
                    evt = AUTO_E;
                    break;
                case OP_SDSTATS:
                    next = PSTSM;
                    evt = AUTO_E;
                    break;
                case OP_RGRD:
                case OP_RGWT:
                case OP_RGADD_H:
                case OP_RGADD_L:
                    next = PSACT;
                    evt = AUTO_B;
                    break;
                case OP_RGDAT:
                    next = PSWT;
                    evt = AUTO_B;
                    break;
                case OP_CROP_01:
                    next = PSSET;
                    evt = AUTO_F;
                    break;
                case OP_CROP_02:
                    next = PSACT;
                    evt = AUTO_F;
                    break;
                case OP_CROP_03:
                    next = PSWT;
                    evt = AUTO_F;
                    break;
                case OP_CROP_04:
                    next = PSRLT;
                    evt = AUTO_F;
                    break;
                case OP_CROP_05:
                    next = PSTSM;
                    evt = AUTO_F;
                    break;
                case OP_CROP_06:
                    next = PSSET;
                    evt = AUTO_G;
                    break;
                case OP_IMG_LEN:
                    next = PSACT;
                    evt = AUTO_G;
                    break;
                case OP_META_DAT:
                    t->opcode = OP_META_DAT;
                    t->data = (tmpAns >> 8) & 0xff;

                    next = PSACT;
                    evt = AUTO_H;
                    break;

                case OP_RAW:
                    next = PSRLT;
                    evt = AUTO_H;
                    break;

/*
                case OP_SUPBACK:
                    next = PSTSM;
                    evt = AUTO_B;
                    break;
*/

                default:
                    break;
                }
                break;
            default:
                //sprintf(str, "default\n"); 
                //print_f(mlogPool, "spy", str); 
                next = PSSET;
                break;
        }
    }

    if (next < 0) {
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);

        next = PSTSM; /* break */
        evt = SPY;
    }

    tmpRlt = emb_event(tmpRlt, evt);
    return emb_process(tmpRlt, next);
}

static uint32_t next_bullet(struct psdata_s *data)
{
    int pro, rlt, next = -1;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;
    evt = (data->result >> 8) & 0xff;

    //sprintf(str, "%d-%d\n", pro, rlt); 
    //print_f(mlogPool, "bullet", str); 

    tmpRlt = data->result;
    if (rlt == WAIT) {
        next = pro;
    } else if (rlt == NEXT) {
        /* reset pro */  
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);
        switch (pro) {
            case PSSET: /* load file, prepare data and spi */
                //sprintf(str, "PSSET\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSACT;
                break;
            case PSACT: /* check RDY, and start transmitting */
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSWT;
                break;
            case PSWT: /* end the transmitting and back to polling OP_QRY */
                //sprintf(str, "PSWT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSRLT;
                break;
            case PSRLT:
                //sprintf(str, "PSRLT\n"); 
                //print_f(mlogPool, "bullet", str); 
                next = PSTSM;
                break;
            case PSTSM:
                //sprintf(str, "PSTSM\n"); 
                //print_f(mlogPool, "bullet", str); 
                //next = PSMAX;
                next = PSSET;
                /* jump to next stage */
                evt = LASER;
                break;
            default:
                sprintf(str, "default\n"); 
                print_f(mlogPool, "bullet", str); 
                next = PSSET;
                break;
        }
    }

    if (next < 0) {
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);

        next = PSTSM; /* break */
        evt = SPY;
    }

    tmpRlt = emb_event(tmpRlt, evt);
    return emb_process(tmpRlt, next);
}

static int next_laser(struct psdata_s *data)
{
    int pro, rlt, next = -1;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;
    evt = (data->result >> 8) & 0xff;

    //sprintf(str, "%d-%d\n", pro, rlt); 
    //print_f(mlogPool, "laser", str); 

    tmpRlt = data->result;
    if (rlt == WAIT) {
        next = pro;
    } else if (rlt == NEXT) {
        /* reset pro */  
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);
        switch (pro) {
            case PSSET: /* load file, prepare data and spi */
                //sprintf(str, "PSSET\n"); 
                //print_f(mlogPool, "laser", str); 
                next = PSACT;
                break;
            case PSACT: /* check RDY, and start transmitting */
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "laser", str); 
                next = PSWT;
                break;
            case PSWT: /* end the transmitting and back to polling OP_QRY */
                //sprintf(str, "PSWT\n"); 
                //print_f(mlogPool, "laser", str); 
                next = PSRLT;
                break;
            case PSRLT:
                //sprintf(str, "PSRLT\n"); 
                //print_f(mlogPool, "laser", str); 
                next = PSTSM;
                break;
            case PSTSM:
                //sprintf(str, "PSTSM\n"); 
                //print_f(mlogPool, "laser", str); 
                //next = PSMAX;
                next = PSTSM;
                /* jump to next stage */
                evt = SPY;
                break;
            default:
                sprintf(str, "default\n"); 
                print_f(mlogPool, "laser", str); 
                next = PSSET;
                break;
        }
    }

    if (next < 0) {
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);

        next = PSTSM; /* break */
        evt = SPY;
    }

    tmpRlt = emb_event(tmpRlt, evt);
    return emb_process(tmpRlt, next);
}

static int next_auto_A(struct psdata_s *data)
{
    int pro, rlt, next = -1;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;
    evt = (data->result >> 8) & 0xff;

    //sprintf(str, "%d-%d, ans:0x%x\n", pro, rlt, data->ansp0); 
    //print_f(mlogPool, "auto_A", str); 

    tmpRlt = data->result;
    if (rlt == WAIT) {
        next = pro;
    } else if (rlt == NEXT) {
        /* reset pro */  
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);
        switch (pro) {
            case PSSET: /* load file, prepare data and spi */
                //sprintf(str, "PSSET\n"); 
                //print_f(mlogPool, "auto_A", str); 
                next = PSTSM; /* jump to next stage */
                evt = SPY;
                break;
            case PSACT: /* check RDY, and start transmitting */
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "auto_A", str); 
                next = PSTSM;
                evt = SPY;
                break;
            case PSWT: /* end the transmitting and back to polling OP_QRY */
                //sprintf(str, "PSWT\n"); 
                //print_f(mlogPool, "auto_A", str); 
                next = PSTSM;
                evt = SPY;
                break;
            case PSRLT:
                //sprintf(str, "PSRLT - ans: 0x%x\n", tmpAns); 
                //print_f(mlogPool, "auto_A", str); 
                next = PSTSM;
                evt = SPY;
                break;
            case PSTSM:
                //sprintf(str, "PSTSM\n"); 
                //print_f(mlogPool, "auto_A", str); 
                next = PSSET;
                evt = AUTO_B;
                break;
            default:
                sprintf(str, "default\n"); 
                print_f(mlogPool, "laser", str); 
                next = PSSET;
                break;
        }
    }

    if (next < 0) {
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);
        next = PSTSM; /* break */
        evt = SPY;
    }

    tmpRlt = emb_event(tmpRlt, evt);
    return emb_process(tmpRlt, next);
}

static int next_auto_B(struct psdata_s *data)
{
    int pro, rlt, next = -1;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;
    evt = (data->result >> 8) & 0xff;

    //sprintf(str, "%d-%d\n", pro, rlt); 
    //print_f(mlogPool, "auto_A", str); 

    tmpRlt = data->result;
    if (rlt == WAIT) {
        next = pro;
    } else if (rlt == NEXT) {
        /* reset pro */  
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);
        switch (pro) {
            case PSSET: /* load file, prepare data and spi */
                //sprintf(str, "PSSET\n"); 
                //print_f(mlogPool, "auto_B", str); 
                break;
            case PSACT: /* check RDY, and start transmitting */
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "auto_B", str); 
                next = PSTSM;
                evt = SPY;
                break;
            case PSWT: /* end the transmitting and back to polling OP_QRY */
                //sprintf(str, "PSWT\n"); 
                //print_f(mlogPool, "auto_B", str); 
                next = PSTSM;
                evt = SPY;
                break;
            case PSRLT:
                //sprintf(str, "PSRLT\n"); 
                //print_f(mlogPool, "auto_B", str); 
                next = PSACT;
                evt = BULLET;
                break;
            case PSTSM:
                //sprintf(str, "PSTSM\n"); 
                //print_f(mlogPool, "auto_B", str); 
                next = PSSET;
                evt = AUTO_C;
                break;
            default:
                sprintf(str, "default\n"); 
                print_f(mlogPool, "laser", str); 
                next = PSSET;
                break;
        }
    }

    if (next < 0) {
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);

        next = PSTSM; /* break */
        evt = SPY;
    }

    tmpRlt = emb_event(tmpRlt, evt);
    return emb_process(tmpRlt, next);
}

static int next_auto_C(struct psdata_s *data)
{
    int pro, rlt, next = -1;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;
    evt = (data->result >> 8) & 0xff;

    //sprintf(str, "%d-%d\n", pro, rlt); 
    //print_f(mlogPool, "auto_A", str); 

    tmpRlt = data->result;
    if (rlt == WAIT) {
        next = pro;
    } else if (rlt == NEXT) {
        /* reset pro */  
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);
        switch (pro) {
            case PSSET: /* load file, prepare data and spi */
                //sprintf(str, "PSSET\n"); 
                //print_f(mlogPool, "auto_C", str); 
                next = PSACT;
                break;
            case PSACT: /* check RDY, and start transmitting */
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "auto_C", str); 
                next = PSWT;
                break;
            case PSWT: /* end the transmitting and back to polling OP_QRY */
                //sprintf(str, "PSWT\n"); 
                //print_f(mlogPool, "auto_C", str); 
                next = PSRLT;
                break;
            case PSRLT: /* LAST */
                //sprintf(str, "PSRLT\n"); 
                //print_f(mlogPool, "auto_C", str); 
                break;
            case PSTSM:
                //sprintf(str, "PSTSM\n"); 
                //print_f(mlogPool, "auto_C", str); 
                next = PSSET; /* break */
                evt = AUTO_D;
                break;
            default:
                sprintf(str, "default\n"); 
                print_f(mlogPool, "laser", str); 
                next = PSSET;
                break;
        }
    }

    if (next < 0) {
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);

        next = PSTSM; /* break */
        evt = SPY;
    }

    tmpRlt = emb_event(tmpRlt, evt);
    return emb_process(tmpRlt, next);
}

static int next_auto_D(struct psdata_s *data)
{
    int pro, rlt, next = -1;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;
    evt = (data->result >> 8) & 0xff;

    //sprintf(str, "%d-%d\n", pro, rlt); 
    //print_f(mlogPool, "auto_A", str); 

    tmpRlt = data->result;
    if (rlt == WAIT) {
        next = pro;
    } else if (rlt == NEXT) {
        /* reset pro */  
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);
        switch (pro) {
            case PSSET: /* load file, prepare data and spi */
                //sprintf(str, "PSSET\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSWT;
                evt = AUTO_C;
                break;
            case PSACT: /* check RDY, and start transmitting */
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSWT;
                break;
            case PSWT: /* end the transmitting and back to polling OP_QRY */
                //sprintf(str, "PSWT\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSWT;
                evt = AUTO_C;
                break;
            case PSRLT:
                //sprintf(str, "PSRLT\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSTSM; 
                evt = SPY;
                break;
            case PSTSM:
                //sprintf(str, "PSTSM\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSTSM; 
                evt = SPY;
                break;
            default:
                sprintf(str, "default\n"); 
                print_f(mlogPool, "laser", str); 
                next = PSSET;
                break;
        }
    }

    if (next < 0) {
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);

        next = PSTSM; /* break */
        evt = SPY;
    }

    tmpRlt = emb_event(tmpRlt, evt);
    return emb_process(tmpRlt, next);
}

static int next_auto_E(struct psdata_s *data)
{
    int pro, rlt, next = -1;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;
    evt = (data->result >> 8) & 0xff;

    //sprintf(str, "%d-%d\n", pro, rlt); 
    //print_f(mlogPool, "auto_A", str); 

    tmpRlt = data->result;
    if (rlt == WAIT) {
        next = pro;
    } else if (rlt == NEXT) {
        /* reset pro */  
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);
        switch (pro) {
            case PSSET: 
                //sprintf(str, "PSSET\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSTSM; 
                evt = SPY;
                break;
            case PSACT: 
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSTSM; 
                evt = SPY;
                break;
            case PSWT: 
                //sprintf(str, "PSWT\n"); 
                //print_f(mlogPool, "auto_D", str); 
                switch ((tmpAns >> 8) & 0xff) {
                    case 0x01:
                        next = PSTSM; 
                        evt = AUTO_D;
                        break;
                    case 0x02:
                        next = PSSET; 
                        evt = AUTO_E;
                        break;
                    default:
                        next = PSTSM; 
                        evt = SPY;
                        break;
                }
                break;
            case PSRLT:
                //sprintf(str, "PSRLT\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSTSM; 
                evt = SPY;
                break;
            case PSTSM:
                //sprintf(str, "PSTSM\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSTSM; 
                evt = SPY;
                break;
            default:
                sprintf(str, "default\n"); 
                print_f(mlogPool, "laser", str); 
                next = PSSET;
                break;
        }
    }

    if (next < 0) {
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);

        next = PSTSM; /* break */
        evt = SPY;
    }

    tmpRlt = emb_event(tmpRlt, evt);
    return emb_process(tmpRlt, next);
}

static int next_auto_F(struct psdata_s *data)
{
    int pro, rlt, next = -1;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;
    evt = (data->result >> 8) & 0xff;

    //sprintf(str, "%d-%d\n", pro, rlt); 
    //print_f(mlogPool, "auto_A", str); 

    tmpRlt = data->result;
    if (rlt == WAIT) {
        next = pro;
    } else if (rlt == NEXT) {
        /* reset pro */  
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);
        switch (pro) {
            case PSSET: 
                //sprintf(str, "PSSET\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSTSM; 
                evt = SPY;
                break;
            case PSACT: 
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSTSM; 
                evt = SPY;
                break;
            case PSWT: 
                //sprintf(str, "PSWT\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSTSM; 
                evt = SPY;
                break;
            case PSRLT:
                //sprintf(str, "PSRLT\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSTSM; 
                evt = SPY;
                break;
            case PSTSM:
                //sprintf(str, "PSTSM\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSTSM; 
                evt = SPY;
                break;
            default:
                sprintf(str, "default\n"); 
                print_f(mlogPool, "laser", str); 
                next = PSSET;
                break;
        }
    }

    if (next < 0) {
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);

        next = PSTSM; /* break */
        evt = SPY;
    }

    tmpRlt = emb_event(tmpRlt, evt);
    return emb_process(tmpRlt, next);
}

static int next_auto_G(struct psdata_s *data)
{
    int pro, rlt, next = -1;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;
    evt = (data->result >> 8) & 0xff;

    //sprintf(str, "%d-%d\n", pro, rlt); 
    //print_f(mlogPool, "auto_A", str); 

    tmpRlt = data->result;
    if (rlt == WAIT) {
        next = pro;
    } else if (rlt == NEXT) {
        /* reset pro */  
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);
        switch (pro) {
            case PSSET: 
                //sprintf(str, "PSSET\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSTSM; 
                evt = SPY;
                break;
            case PSACT: 
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSTSM; 
                evt = SPY;
                break;
            case PSWT: 
                //sprintf(str, "PSWT\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSTSM; 
                evt = SPY;
                break;
            case PSRLT:
                //sprintf(str, "PSRLT\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSTSM; 
                evt = SPY;
                break;
            case PSTSM:
                //sprintf(str, "PSTSM\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSTSM; 
                evt = SPY;
                break;
            default:
                sprintf(str, "default\n"); 
                print_f(mlogPool, "laser", str); 
                next = PSSET;
                break;
        }
    }

    if (next < 0) {
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);

        next = PSTSM; /* break */
        evt = SPY;
    }

    tmpRlt = emb_event(tmpRlt, evt);
    return emb_process(tmpRlt, next);
}

static int next_auto_H(struct psdata_s *data)
{
    int pro, rlt, next = -1;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;
    evt = (data->result >> 8) & 0xff;

    //sprintf(str, "%d-%d\n", pro, rlt); 
    //print_f(mlogPool, "auto_H", str); 

    tmpRlt = data->result;
    if (rlt == WAIT) {
        next = pro;
    } else if (rlt == NEXT) {
        /* reset pro */  
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);
        switch (pro) {
            case PSSET: 
                //sprintf(str, "PSSET\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSTSM; 
                evt = SPY;
                break;
            case PSACT: 
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSWT; 
                break;
            case PSWT: 
                //sprintf(str, "PSWT\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSTSM; 
                evt = SPY;
                break;
            case PSRLT:
                //sprintf(str, "PSRLT\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSTSM; 
                break;
            case PSTSM:
                //sprintf(str, "PSTSM\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSTSM; 
                evt = SPY;
                break;
            default:
                sprintf(str, "default\n"); 
                print_f(mlogPool, "laser", str); 
                next = PSSET;
                break;
        }
    }

    if (next < 0) {
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);

        next = PSTSM; /* break */
        evt = SPY;
    }

    tmpRlt = emb_event(tmpRlt, evt);
    return emb_process(tmpRlt, next);
}

static int next_auto_I(struct psdata_s *data)
{
    int pro, rlt, next = -1;
    uint32_t tmpAns = 0, evt = 0, tmpRlt = 0;
    char str[256];
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;
    evt = (data->result >> 8) & 0xff;

    //sprintf(str, "%d-%d\n", pro, rlt); 
    //print_f(mlogPool, "auto_I", str); 

    tmpRlt = data->result;
    if (rlt == WAIT) {
        next = pro;
    } else if (rlt == NEXT) {
        /* reset pro */  
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);
        switch (pro) {
            case PSSET: 
                //sprintf(str, "PSSET\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSTSM; 
                evt = SPY;
                break;
            case PSACT: 
                //sprintf(str, "PSACT\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSTSM; 
                evt = SPY;
                break;
            case PSWT: 
                //sprintf(str, "PSWT\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSTSM; 
                evt = SPY;
                break;
            case PSRLT:
                //sprintf(str, "PSRLT\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSTSM; 
                evt = SPY;
                break;
            case PSTSM:
                //sprintf(str, "PSTSM\n"); 
                //print_f(mlogPool, "auto_D", str); 
                next = PSTSM; 
                evt = SPY;
                break;
            default:
                sprintf(str, "default\n"); 
                print_f(mlogPool, "laser", str); 
                next = PSSET;
                break;
        }
    }

    if (next < 0) {
        tmpAns = data->ansp0;
        data->ansp0 = 0;
        tmpRlt = emb_result(tmpRlt, STINIT);

        next = PSTSM; /* break */
        evt = SPY;
    }

    tmpRlt = emb_event(tmpRlt, evt);
    return emb_process(tmpRlt, next);
}

static int next_error(struct psdata_s *data)
{
    int pro, rlt, next;
    char str[256];
    rlt = (data->result >> 16) & 0xff;
    pro = data->result & 0xff;

    //sprintf(str, "%d-%d\n", pro, rlt); 
    //print_f(mlogPool, "error", str); 
                    
    if (rlt == NEXT) {
        switch (pro) {
            case PSSET:
                sprintf(str, "PSSET\n"); 
                print_f(mlogPool, "error", str); 
                next = PSACT;
                break;
            case PSACT:
                sprintf(str, "PSACT\n"); 
                print_f(mlogPool, "error", str); 
                next = PSWT;
                break;
            case PSWT:
                sprintf(str, "PSWT\n"); 
                print_f(mlogPool, "error", str); 
                next = PSRLT;
                break;
            case PSRLT:
                sprintf(str, "PSRLT\n"); 
                print_f(mlogPool, "error", str); 
                next = PSTSM;
                break;
            case PSTSM:
                sprintf(str, "PSTSM\n"); 
                print_f(mlogPool, "error", str); 
                next = PSSET;
                break;
            default:
                sprintf(str, "default\n"); 
                print_f(mlogPool, "error", str); 
                next = PSSET;
                break;
        }
    }
    next = 0; /* error handle, return to 0 */

    return emb_process(data->result, next);
}
static int ps_next(struct psdata_s *data)
{
    int sta, ret, evt, nxtst = -1, nxtrlt = 0;
    char str[256];

    sta = (data->result >> 8) & 0xff;
    nxtst = sta;

    //sprintf(str, "sta: 0x%x\n", sta); 
    //print_f(mlogPool, "psnext", str); 

    switch (sta) {
        case SPY:
            ret = next_spy(data);
            evt = (ret >> 24) & 0xff;
            nxtst = evt; /* state change */

            break;
        case BULLET:
            ret = next_bullet(data);
            evt = (ret >> 24) & 0xff;
            nxtst = evt; /* state change */

            break;
        case LASER:
            ret = next_laser(data);
            evt = (ret >> 24) & 0xff;
            nxtst = evt; /* end the test loop */

            break;
        case AUTO_A:
            ret = next_auto_A(data);
            evt = (ret >> 24) & 0xff;
            nxtst = evt; /* end the test loop */

            break;
        case AUTO_B:
            ret = next_auto_B(data);
            evt = (ret >> 24) & 0xff;
            nxtst = evt; /* end the test loop */

            break;
        case AUTO_C:
            ret = next_auto_C(data);
            evt = (ret >> 24) & 0xff;
            nxtst = evt; /* end the test loop */

            break;
        case AUTO_D:
            ret = next_auto_D(data);
            evt = (ret >> 24) & 0xff;
            nxtst = evt; /* end the test loop */

            break;
        case AUTO_E:
            ret = next_auto_E(data);
            evt = (ret >> 24) & 0xff;
            nxtst = evt; /* end the test loop */

            break;
        case AUTO_F:
            ret = next_auto_F(data);
            evt = (ret >> 24) & 0xff;
            nxtst = evt; /* end the test loop */

            break;
        case AUTO_G:
            ret = next_auto_G(data);
            evt = (ret >> 24) & 0xff;
            nxtst = evt; /* end the test loop */

            break;
        case AUTO_H:
            ret = next_auto_H(data);
            evt = (ret >> 24) & 0xff;
            nxtst = evt; /* end the test loop */

            break;
        case AUTO_I:
            ret = next_auto_I(data);
            evt = (ret >> 24) & 0xff;
            nxtst = evt; /* end the test loop */

            break;
        default:
            ret = next_error(data);
            evt = (ret >> 24) & 0xff;
            //if (evt == 0x1) nxtst = SPY;

            break;
    }

    nxtrlt = emb_state(ret, nxtst);

    //sprintf(str, "ret: 0x%.4x nxtst: 0x%x nxtrlt: 0x%.4x\n", ret, nxtst, nxtrlt); 
    //print_f(mlogPool, "ps_next", str); 
    
#if 0
    data->result += 1;
    if ((data->result & 0xf) == PSMAX) {
        data->result = (data->result & 0xf0) + 0x10;
    }

    if (((data->result & 0xf0) >> 4)== SMAX) {
        data->result = -1;
    }
#endif

    return nxtrlt;

}
static int stspy_01(struct psdata_s *data)
{ 
    // keep polling, kind of idle mode
    // jump to next status if receive any op code

    char str[128], ch = 0; 
    uint32_t rlt;
    rlt = abs_result(data->result);	

    //sprintf(str, "op_01 - rlt:0x%x \n", rlt); 
    //print_f(mlogPool, "spy", str); 

    switch (rlt) {
        case STINIT:
            ch = 1; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            //sprintf(str, "op_01: result: %x\n", data->result); 
            //print_f(mlogPool, "spy", str);  
            break;
        case WAIT:
            if (data->ansp0 == 1)
                data->result = emb_result(data->result, NEXT);
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }

    return ps_next(data);
}
static int stspy_02(struct psdata_s *data) 
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    struct info16Bit_s *p;
    struct procRes_s *rs;
    rs = data->rs;
    p = &rs->pmch->get;

    rlt = abs_result(data->result);	

    //sprintf(str, "op_02 - rlt:0x%.8x \n", rlt); 
    //print_f(mlogPool, "spy", str); 

    switch (rlt) {
        case STINIT:
            ch = 3; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            //printf(str, "op_02: result: %x\n", data->result); 
            //print_f(mlogPool, "spy", str);  
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                sprintf(str, "op_02: %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
                print_f(mlogPool, "spy", str);  
            }            
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}
static int stspy_03(struct psdata_s *data) 
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    rlt = abs_result(data->result);	

    //sprintf(str, "op_03 - rlt:0x%x \n", rlt); 
    //print_f(mlogPool, "spy", str); 

    switch (rlt) {
        case STINIT:
            ch = 5; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            //sprintf(str, "op_03: result: %x\n", data->result); 
            //print_f(mlogPool, "spy", str);  
            break;
        case WAIT:
            if (data->ansp0 == 1)
                data->result = emb_result(data->result, NEXT);
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }

    return ps_next(data);
}
static int stspy_04(struct psdata_s *data) 
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    rlt = abs_result(data->result);	

    //sprintf(str, "op_04 - rlt:0x%.8x \n", rlt); 
    //print_f(mlogPool, "spy", str); 

    switch (rlt) {
        case STINIT:
            ch = 7; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            //sprintf(str, "op_04: result: %x\n", data->result); 
            //print_f(mlogPool, "spy", str);  
            break;
        case WAIT:
            if (data->ansp0 == 1)
                data->result = emb_result(data->result, NEXT);
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}
static int stspy_05(struct psdata_s *data) 
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    struct info16Bit_s *p;
    struct SdAddrs_s *m, *s;
    
    rlt = abs_result(data->result);	

    s = &data->rs->pmch->sdst;
    m = &data->rs->pmch->sdln;
    
    //sprintf(str, "op_05 - rlt:0x%.8x \n", rlt); 
    //print_f(mlogPool, "spy", str); 

    switch (rlt) {
        case STINIT:
            p = &data->rs->pmch->get;
            memset(p, 0, sizeof(struct info16Bit_s));

            ch = 9; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            //sprintf(str, "op_05: result: %x\n", data->result); 
            //print_f(mlogPool, "spy", str);  
            break;
        case WAIT:
            p = &data->rs->pmch->get;
            sprintf(str, "05: asnp: 0x%x, get pkt: 0x%x 0x%x\n", data->ansp0, p->opcode, p->data); 
            print_f(mlogPool, "spy", str);  

            if ((data->ansp0 & 0xff) == p->opcode) {
                data->ansp0 |= (p->data << 8);
                data->ansp0 |= ((m->f & 0xff)<< 16);
                
                switch (data->ansp0 & 0xff) {
                    case OP_SINGLE: /* currently support */
                    case OP_MSINGLE:
                    case OP_HANDSCAN:
                    case OP_NOTESCAN:
                    case OP_POLL:
                    case OP_SDRD:
                    case OP_SDWT:
                    case OP_STSEC_00:
                    case OP_STSEC_01:
                    case OP_STSEC_02:
                    case OP_STSEC_03:
                    case OP_STLEN_00:
                    case OP_STLEN_01:
                    case OP_STLEN_02:
                    case OP_STLEN_03:
                    case OP_FFORMAT:
                    case OP_COLRMOD:
                    case OP_COMPRAT:
                    case OP_RESOLTN:
                    case OP_SCANGAV:
                    case OP_MAXWIDH:
                    case OP_WIDTHAD_H:
                    case OP_WIDTHAD_L:
                    case OP_SCANLEN_H:
                    case OP_SCANLEN_L:
                    case OP_INTERIMG:
                    case OP_AFEIC:
                    case OP_EXTPULSE:
                    case OP_SDAT:
                    case OP_FREESEC:
                    case OP_USEDSEC:
                    case OP_SDINIT:
                    case OP_SDSTATS:
                    case OP_ACTION:
                    case OP_RAW:
                    case OP_RGRD:
                    case OP_RGWT:
                    case OP_RGDAT:
                    case OP_RGADD_H:
                    case OP_RGADD_L:

                    case OP_CROP_01:
                    case OP_CROP_02:
                    case OP_CROP_03:
                    case OP_CROP_04:
                    case OP_CROP_05:
                    case OP_CROP_06:
                    case OP_IMG_LEN:
                    case OP_META_DAT:

                    case OP_SUPBACK:
                    case OP_DOUBLE:
                    case OP_MDOUBLE:
                    case OP_SAVE:
                    case OP_FUNCTEST_00:
                    case OP_FUNCTEST_01:
                    case OP_FUNCTEST_02:
                    case OP_FUNCTEST_03:
                    case OP_FUNCTEST_04:
                    case OP_FUNCTEST_05:
                    case OP_FUNCTEST_06:
                    case OP_FUNCTEST_07:
                    case OP_FUNCTEST_08:
                    case OP_FUNCTEST_09:
                    case OP_FUNCTEST_10:
                    case OP_FUNCTEST_11:
                    case OP_FUNCTEST_12:
                    case OP_FUNCTEST_13:
                    case OP_FUNCTEST_14:
                    case OP_FUNCTEST_15:

                        sprintf(str, "go to next \n"); 
                        print_f(mlogPool, "spy", str);  
                        data->result = emb_result(data->result, NEXT);
                        break;
                    default:
                        break;
                }
            }
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}
static int stbullet_01(struct psdata_s *data) 
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    struct info16Bit_s *p;
    struct procRes_s *rs;
    rs = data->rs;
    p = &rs->pmch->get;
    rlt = abs_result(data->result);	
    //sprintf(str, "op_01: rlt: %.8x result: %.8x ans:%d\n", rlt, data->result, data->ansp0); 
    //print_f(mlogPool, "bullet", str);  

    switch (rlt) {
        case STINIT:
            ch = 11; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            //sprintf(str, "op_01: result: %x\n", data->result); 
            //print_f(mlogPool, "bullet", str);  

            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                sprintf(str, "op_01: %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
                print_f(mlogPool, "bullet", str);  
            }

            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }

    return ps_next(data);
}
static int stbullet_02(struct psdata_s *data) 
{ 
    uint32_t rlt;
    char str[128], ch = 0; 
    rlt = abs_result(data->result);	
    //sprintf(str, "op_02: rlt: %.8x result: %.8x ans:%d\n", rlt, data->result, data->ansp0); 
    //print_f(mlogPool, "bullet", str);  

    switch (rlt) {
        case STINIT:
            ch = 13; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            break;
        case WAIT:
            if (data->ansp0 == 1)
                data->result = emb_result(data->result, NEXT);
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}
static int stbullet_03(struct psdata_s *data) 
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
	
    rlt = abs_result(data->result);	
    //sprintf(str, "op_03: rlt: %.8x result: %.8x ans:%d\n", rlt, data->result, data->ansp0); 
    //print_f(mlogPool, "bullet", str);  

    switch (rlt) {
        case STINIT:
            ch = 14; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            break;
        case WAIT:
            if (data->ansp0 == 1)
                data->result = emb_result(data->result, NEXT);
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}
static int stbullet_04(struct psdata_s *data) 
{ 
    char str[128], ch = 0; 
    uint32_t rlt;
    rlt = abs_result(data->result);	
    //sprintf(str, "op_04: rlt: %x result: %x ans:%d\n", rlt, data->result, data->ansp0); 
    //print_f(mlogPool, "bullet", str);  

    switch (rlt) {
        case STINIT:
            ch = 5; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            break;
        case WAIT:
            if (data->ansp0 == 1)
                data->result = emb_result(data->result, NEXT);
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}
static int stbullet_05(struct psdata_s *data) 
{ 
    char str[128], ch = 0;
    uint32_t rlt;
    rlt = abs_result(data->result);	
    //sprintf(str, "op_05: rlt: %x result: %x ans:%d\n", rlt, data->result, data->ansp0);  
    //print_f(mlogPool, "bullet", str);  

    switch (rlt) {
        case STINIT:
            ch = 20; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}
static int stlaser_01(struct psdata_s *data) 
{ 
    char str[128], ch = 0;
    uint32_t rlt;
    rlt = abs_result(data->result);	
    //sprintf(str, "op_01: rlt: %x result: %x ans:%d\n", rlt, data->result, data->ansp0);  
    //print_f(mlogPool, "laser", str);  

    switch (rlt) {
        case STINIT:
            ch = 5; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}
static int stlaser_02(struct psdata_s *data) 
{ 
    char str[128], ch = 0;
    uint32_t rlt;
    rlt = abs_result(data->result);	
    //sprintf(str, "op_02: rlt: %.8x result: %.8x ans:%d\n", rlt, data->result, data->ansp0);  
    //print_f(mlogPool, "laser", str);  

    switch (rlt) {
        case STINIT:
            ch = 19; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}
static int stlaser_03(struct psdata_s *data) 
{ 
    char str[128], ch = 0;
    uint32_t rlt;
    rlt = abs_result(data->result);	
    //sprintf(str, "op_03: rlt: %x result: %x ans:%d\n", rlt, data->result, data->ansp0);  
    //print_f(mlogPool, "laser", str);  

    switch (rlt) {
        case STINIT:
            ch = 17; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}
static int stlaser_04(struct psdata_s *data) 
{ 
    char str[128], ch = 0;
    uint32_t rlt;
    rlt = abs_result(data->result);	
    //sprintf(str, "op_04: rlt: %x result: %x ans:%d\n", rlt, data->result, data->ansp0);  
    //print_f(mlogPool, "laser", str);  

    switch (rlt) {
        case STINIT:
            ch = 5; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);

}
static int stlaser_05(struct psdata_s *data) 
{ 
    char str[128], ch = 0;
    uint32_t rlt;
    rlt = abs_result(data->result);	
    //sprintf(str, "op_05: rlt: %.8x result: %.8x ans:%d\n", rlt, data->result, data->ansp0);  
    //print_f(mlogPool, "laser", str);  

    switch (rlt) {
        case STINIT:
            ch = 7; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);

}

static int stauto_01(struct psdata_s *data)
{ 
    char str[128], ch = 0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;

    rlt = abs_result(data->result);	
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_01", str);  

    switch (rlt) {
        case STINIT:
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            memset(p, 0, sizeof(struct info16Bit_s));
            p->opcode = g->opcode;
            #if 1 /* test opcode transmitting */
            p->data = g->data;
            #else
            p->data = 0xaf;
            #endif
            ch = 24; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);

            memset(g, 0, sizeof(struct info16Bit_s));
            break;
        case WAIT:
            g = &data->rs->pmch->get;

            if ((data->ansp0 & 0xff) == g->opcode) {
                sprintf(str, "ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data);  
                print_f(mlogPool, "auto_01", str);  
                data->result = emb_result(data->result, NEXT);
            }			
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);

}

static int stauto_02(struct psdata_s *data)
{ 
    char str[128], ch = 0;
    uint32_t rlt;
    uint8_t op;
    struct info16Bit_s *p, *g;
    struct SdAddrs_s *s, *m;

    FILE *fp=0;

    rlt = abs_result(data->result);	
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_02", str);  

    switch (rlt) {
        case STINIT:
            s = &data->rs->pmch->sdst;
            m = &data->rs->pmch->sdln;
            //memset(s, 0, sizeof(struct SdAddrs_s));
            //memset(m, 0, sizeof(struct SdAddrs_s));
            s->f = 0;
            m->f = 0;

            ch = 36; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);

            break;
        case WAIT:
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;

            if ((data->ansp0 & 0xff) == g->opcode) {
                if ((data->ansp0 & 0xff) == p->opcode) {
                    sprintf(str, "ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data);  
                    print_f(mlogPool, "auto_02", str);  
                    data->result = emb_result(data->result, NEXT);
                } else {
                    sprintf(str, "ERROR!!! ansp:0x%.2x, g:0x%.2x s:0x%.2x, break!!!\n", data->ansp0, g->opcode, p->opcode);  
                    print_f(mlogPool, "auto_02", str);  
                    data->result = emb_result(data->result, BREAK);
                }		
            } else {
                    sprintf(str, "WAIT!!! ansp:0x%.2x, g:0x%.2x s:0x%.2x!!!\n", data->ansp0, g->opcode, p->opcode);  
                    print_f(mlogPool, "auto_02", str);  
            }
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);

}

static int stauto_03(struct psdata_s *data)
{ 
    char str[128], ch=0, ix=0;
    uint8_t op=0, dt=0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;
    struct SdAddrs_s *s, *m;
	
    rlt = abs_result(data->result);	
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_03", str);  

    switch (rlt) {
        case STINIT:
            s = &data->rs->pmch->sdst;
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            memset(p, 0, sizeof(struct info16Bit_s));

            op = g->opcode;

            switch (op) {
            case OP_STSEC_00:
            case OP_STSEC_01:
            case OP_STSEC_02:
            case OP_STSEC_03:
                ix = (OP_STSEC_03 - op) & 0xf;
                s->d[ix] = g->data;
                s->f &= ~(0x1 << ix);

                sprintf(str, "!! flag: 0x%.2x data: 0x%.2x !!- 1\n", s->f, s->d[ix]);  
                print_f(mlogPool, "auto_03", str);  

                p->opcode = g->opcode;
                p->data = g->data;

                sprintf(str, "!! flag: 0x%.2x data: 0x%.2x !!- 2\n", s->f, s->d[ix]);  
                print_f(mlogPool, "auto_03", str);  

                ch = 24; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);

                memset(g, 0, sizeof(struct info16Bit_s));			
                break;
            default:
                sprintf(str, "ERROR!!! get pkt: 0x%.2x 0x%.2x break!!\n", g->opcode, g->data);  
                print_f(mlogPool, "auto_03", str);  
                data->result = emb_result(data->result, NEXT);
                break;
            }
            break;
        case WAIT:
            g = &data->rs->pmch->get;
            op = g->opcode;
            switch (op) {
            case OP_STSEC_00:
            case OP_STSEC_01:
            case OP_STSEC_02:
            case OP_STSEC_03:
                s = &data->rs->pmch->sdst;

                if ((data->ansp0 & 0xff) == op) {
                    ix = (OP_STSEC_03 - op) & 0xf;

                    if (s->d[ix] == g->data) {
                        s->f |= 0x1 << ix;
                    }

                    sprintf(str, "ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data);  
                    print_f(mlogPool, "auto_03", str);  
                    data->result = emb_result(data->result, NEXT);
                }
                break;
            case 0: /* may add counter to prevent dead lock */
            default:
                break;
            }

            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);

}
static int stauto_04(struct psdata_s *data)
{ 
    char str[128], ch=0, ix=0;
    uint8_t op=0, dt=0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;
    struct SdAddrs_s *m;

    rlt = abs_result(data->result);
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_04", str);  

    switch (rlt) {
        case STINIT:
            m = &data->rs->pmch->sdln;
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            memset(p, 0, sizeof(struct info16Bit_s));

            op = g->opcode;

            switch (op) {
            case OP_STLEN_00:
            case OP_STLEN_01:
            case OP_STLEN_02:
            case OP_STLEN_03:
                ix = (OP_STLEN_03 - op) & 0xf;
                m->d[ix] = g->data;
                m->f &= ~(0x1 << ix);

                sprintf(str, "!! flag: 0x%.2x data: 0x%.2x !! - 1\n", m->f, p->data);  
                print_f(mlogPool, "auto_04", str);  


                p->opcode = g->opcode;
                p->data = g->data;


                sprintf(str, "!! flag: 0x%.2x data: 0x%.2x !! - 2\n", m->f, p->data);  
                print_f(mlogPool, "auto_04", str);  

                ch = 24; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);

                memset(g, 0, sizeof(struct info16Bit_s));			
                break;
            default:
                sprintf(str, "ERROR!!! get pkt: 0x%.2x 0x%.2x break!!\n", g->opcode, g->data);  
                print_f(mlogPool, "auto_04", str);  
                data->result = emb_result(data->result, NEXT);
                break;
            }
            break;
        case WAIT:
            g = &data->rs->pmch->get;
            op = g->opcode;
            switch (op) {
            case OP_STLEN_00:
            case OP_STLEN_01:
            case OP_STLEN_02:
            case OP_STLEN_03:
                m = &data->rs->pmch->sdln;

                if ((data->ansp0 & 0xff) == op) {
                    ix = (OP_STLEN_03 - op) & 0xf;

                    if (m->d[ix] == g->data) {
                        m->f |= 0x1 << ix;
                    }

                    sprintf(str, "m->f: 0x%x, ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to next!!\n", m->f, data->ansp0, g->opcode, g->data);  
                    print_f(mlogPool, "auto_04", str);  
                    data->result = emb_result(data->result, NEXT);

                }
                break;
            case 0: /* may add counter to prevent dead lock */
            default:
                break;
            }

            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);

}
static int stauto_05(struct psdata_s *data)
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;
    struct SdAddrs_s *s, *m;
    FILE *fp;

    s = &data->rs->pmch->sdst;
    m = &data->rs->pmch->sdln;
    g = &data->rs->pmch->get;
    p = &data->rs->pmch->cur;

    rlt = abs_result(data->result);
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_05", str);  

    switch (rlt) {
        case STINIT:
            //fp = data->rs->pmch->fdsk.vsd;

            memset(p, 0, sizeof(struct info16Bit_s));

            if ((s->f == 0xf) && (m->f == 0xf)) {
                p->opcode = g->opcode;
                p->data = g->data;

                sprintf(str, "get start sector: %d length: %d , send OP_SDAT confirm !!!\n", s->n, m->n);  
                print_f(mlogPool, "auto_05", str);  

                ch = 24; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);

            } else {
                sprintf(str, "ERROR!!! get pkt: 0x%.2x 0x%.2x, go to next!!\n", g->opcode, g->data);  
                print_f(mlogPool, "auto_05", str);  
                data->result = emb_result(data->result, NEXT);
            }

            memset(g, 0, sizeof(struct info16Bit_s));
            break;
        case WAIT:
            g = &data->rs->pmch->get;

            if ((g->opcode) && ((data->ansp0 & 0xff) == g->opcode)) {
                s->f = 0;
                m->f = 0;
                sprintf(str, "ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data);  
                print_f(mlogPool, "auto_05", str);  
                data->result = emb_result(data->result, NEXT);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_06(struct psdata_s *data)
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct DiskFile_s *pf;
    rlt = abs_result(data->result);	
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_06", str);  

    switch (rlt) {
        case STINIT:
            ch = 26; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            break;
        case WAIT:
            
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
                pf = &data->rs->pmch->fdsk;
                pf->rtlen = 0;
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, BREAK);
                pf = &data->rs->pmch->fdsk;
                pf->rtlen = 0;
            }
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_07(struct psdata_s *data)
{
    char ch = 0;
    uint32_t rlt;
    struct DiskFile_s *pf;
    struct procRes_s *rs;
    struct info16Bit_s *c, *g;
    struct RegisterRW_s *preg;

    rs = data->rs;
    rlt = abs_result(data->result);	

    g = &rs->pmch->get;
    c = &rs->pmch->cur;
    preg = &rs->pmch->regp;

    //sprintf(rs->logs, "op07 result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(rs->plogs, "reg", rs->logs);  

    switch (rlt) {
        case STINIT:
            memset(c, 0, sizeof(struct info16Bit_s));
            c->opcode = g->opcode;
            c->data = g->data;

            ch = 38; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);

            memset(g, 0, sizeof(struct info16Bit_s));
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                if (g->opcode == OP_RGRD) {
                    rs->pmch->regp.rgact = g->opcode;
                    sprintf(rs->logs, "OP_RGRD 16bit addr:0x%.8x, go to next!!\n", preg->rga16);  
                    print_f(rs->plogs, "reg", rs->logs);  
                    data->result = emb_result(data->result, NEXT);
                } else if (g->opcode == OP_RGWT) {
                    rs->pmch->regp.rgact = g->opcode;
                    sprintf(rs->logs, "OP_RGWT 16bit addr:0x%.8x, go to next!!\n", preg->rga16);  
                    print_f(rs->plogs, "reg", rs->logs);  
                    data->result = emb_result(data->result, NEXT);
                } else if (g->opcode == OP_RGADD_H) {
                    preg->rga16 &= 0xffff00ff;
                    preg->rga16 |= g->data << 8;
                    sprintf(rs->logs, "OP_RGADD_H 16bit addr:0x%.8x, go to next!!\n", preg->rga16);  
                    print_f(rs->plogs, "reg", rs->logs);  
                    data->result = emb_result(data->result, NEXT);
                } else if (g->opcode == OP_RGADD_L) {
                    preg->rga16 &= 0xffffff00;
                    preg->rga16 |= g->data;
                    sprintf(rs->logs, "OP_RGADD_L 16bit addr:0x%.8x, go to next!!\n", preg->rga16);  
                    print_f(rs->plogs, "reg", rs->logs);  
                    data->result = emb_result(data->result, NEXT);
                } else {
                    sprintf(rs->logs, "ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data);  
                    print_f(rs->plogs, "reg", rs->logs);  
                    data->result = emb_result(data->result, BREAK);
                }
            } else if (data->ansp0 == 2) { 
                sprintf(rs->logs, "ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, break!!\n", data->ansp0, g->opcode, g->data);  
                print_f(rs->plogs, "reg", rs->logs);  
                data->result = emb_result(data->result, BREAK);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_08(struct psdata_s *data)
{
    int i=0, idx=-1, avl=0;
    char ch = 0;
    uint32_t rlt;
    struct DiskFile_s *pf;
    struct procRes_s *rs;
    struct info16Bit_s *c, *g;
    struct RegisterRW_s *preg;
    struct virtualReg_s *ptb;

    rs = data->rs;
    rlt = abs_result(data->result);	

    g = &rs->pmch->get;
    c = &rs->pmch->cur;
    preg = &rs->pmch->regp;
    ptb = rs->pregtb;

    //sprintf(rs->logs, "op08 result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(rs->plogs, "reg", rs->logs);  

    switch (rlt) {
        case STINIT:
           memset(c, 0, sizeof(struct info16Bit_s));
           c->opcode = g->opcode;

           idx = -1; avl = -1;
           if (preg->rgact == OP_RGRD) {
                for (i = 0; i < 128; i++) {
                    if (avl < 0) {
                        if (ptb[i].vrAddr == 0) {
                            avl = i;
                        }
                    }
                    if (ptb[i].vrAddr == preg->rga16) {
                        idx = i; 
                        break;
                    }
                }

                if (idx < 0) {
                    preg->rgidx = avl;
                    c->data = 0;
                } else {
                    preg->rgidx = idx;
                    c->data = ptb[idx].vrValue;
                }
                sprintf(rs->logs, "OP_RGRD rgact 0x%x, get reg data: 0x%x, idx:%d, avl:%d !!\n", preg->rgact, c->data, idx, avl);  
                print_f(rs->plogs, "reg", rs->logs);  

                ch = 38; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);

                memset(g, 0, sizeof(struct info16Bit_s));
            } else if (preg->rgact == OP_RGWT) {
                c->data = g->data;
                ch = 38; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);

                memset(g, 0, sizeof(struct info16Bit_s));
            } else {
                sprintf(rs->logs, "unknown rgact 0x%x, get pkt: 0x%.2x 0x%.2x, break!!\n", preg->rgact, g->opcode, g->data);  
                print_f(rs->plogs, "reg", rs->logs);  

                data->result = emb_result(data->result, BREAK);
            }
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                if (preg->rgact == OP_RGWT) {
                    idx = -1; avl = -1;
                    for (i = 0; i < 128; i++) {
                        if (avl < 0) {
                            if (ptb[i].vrAddr == 0) {
                                avl = i;
                            }
                        }

                        if (ptb[i].vrAddr == preg->rga16) {
                            idx = i; 
                            break;
                        }
                    }

                    if (idx < 0) {
                        preg->rgidx = avl;
                        ptb[avl].vrValue = g->data;
                        ptb[avl].vrAddr = preg->rga16;
                    } else {
                        preg->rgidx = idx;
                        ptb[idx].vrValue = g->data;
                    }
                }

                sprintf(rs->logs, "get op: 0x%.2x/0x%.2x, idx:%d, avl:%d, go to next!!\n", g->opcode, g->data, idx, avl);  
                print_f(rs->plogs, "reg", rs->logs);  
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) { 
                sprintf(rs->logs, "ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, break!!\n", data->ansp0, g->opcode, g->data);  
                print_f(rs->plogs, "reg", rs->logs);  
                data->result = emb_result(data->result, BREAK);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_09(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct procRes_s *rs;
    struct info16Bit_s *c, *g;    

    rs = data->rs;
    g = &rs->pmch->get;
    c = &rs->pmch->cur;

    rlt = abs_result(data->result);	
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_09", str);  

    switch (rlt) {
        case STINIT:
            ch = 40; 

            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, BREAK);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}
static int stauto_10(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct procRes_s *rs;
    struct info16Bit_s *c, *g;    

    rs = data->rs;
    g = &rs->pmch->get;
    c = &rs->pmch->cur;

    rlt = abs_result(data->result);	
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_10", str);  

    switch (rlt) {
        case STINIT:
            if (c->info) {
                ch = 42; 
                g->info = 0;
            } else {
                ch = 17; 
            }

            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, BREAK);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}
static int stauto_11(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    
    rlt = abs_result(data->result);	
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_11", str);  

    switch (rlt) {
        case STINIT:
            ch = 44; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, BREAK);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_12(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct procRes_s *rs;
    struct info16Bit_s *c, *g;    

    rs = data->rs;
    g = &rs->pmch->get;
    c = &rs->pmch->cur;

    rlt = abs_result(data->result);	
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_12", str);  

    switch (rlt) {
        case STINIT:
            if (g->info) {
                if (c->info == g->info) {
                    c->info = 0;
                    g->info = 0;
                }
            }
            ch = 20; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, BREAK);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_13(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    
    rlt = abs_result(data->result);	
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_13", str);  

    switch (rlt) {
        case STINIT:
            ch = 17; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, BREAK);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_14(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    
    rlt = abs_result(data->result);	
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_14", str);  

    switch (rlt) {
        case STINIT:
            ch = 7; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, BREAK);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_15(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct info16Bit_s *t;
    t = &data->rs->pmch->tmp;

    rlt = abs_result(data->result);	
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_15", str);  

    switch (rlt) {
        case STINIT:
            ch = 48; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, BREAK);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}
static int stauto_16(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    
    rlt = abs_result(data->result);	
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_16", str);  

    switch (rlt) {
        case STINIT:
            ch = 50; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, BREAK);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_17(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;
    
    rlt = abs_result(data->result);	
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_17", str);  

    switch (rlt) {
        case STINIT:
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;

            p->opcode = g->opcode;
            p->data = g->data;

            sprintf(str, "g(op:0x%x, arg:0x%x) t(op:0x%x, arg:0x%x) \n", g->opcode, g->data, p->opcode, p->data);  
            print_f(mlogPool, "auto_17", str);  
            
            ch = 53; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, BREAK);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_18(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    
    rlt = abs_result(data->result);	
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_18", str);  

    switch (rlt) {
        case STINIT:
            ch = 55; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, BREAK);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_19(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;

    rlt = abs_result(data->result);	
    sprintf(str, "op_19 result: %.8x ansp:%d\n", data->result, data->ansp0);  
    print_f(mlogPool, "save", str);  

    switch (rlt) {
        case STINIT:
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            memset(p, 0, sizeof(struct info16Bit_s));
            p->opcode = g->opcode;
            p->data = g->data;

            ch = 58; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);

            memset(g, 0, sizeof(struct info16Bit_s));
            break;
        case WAIT:
            g = &data->rs->pmch->get;

            if (data->ansp0 == g->opcode) {
                sprintf(str, "ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data);  
                print_f(mlogPool, "auto_01", str);  
                data->result = emb_result(data->result, NEXT);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);

}

static int stauto_20(struct psdata_s *data)
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;
    struct SdAddrs_s *s, *m;
    FILE *fp;

    s = &data->rs->pmch->sdst;
    m = &data->rs->pmch->sdln;
    g = &data->rs->pmch->get;
    p = &data->rs->pmch->cur;

    rlt = abs_result(data->result);
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_20", str);  

    switch (rlt) {
        case STINIT:
            //fp = data->rs->pmch->fdsk.vsd;
            memset(p, 0, sizeof(struct info16Bit_s));

            if (s->f & 0x100) {
                s->sda = s->n;
                s->f &= ~(0x100);
            } else {
                s->f = 0x100;
            }

            if (m->f & 0x100) {
                m->sda = m->n;
                m->f &= ~(0x100);
            } else {
                m->f = 0x100;
            }

            if ((m->f & 0x100) || (s->f & 0x100)) {
                p->opcode = g->opcode;
                p->data = g->data;           

                ch = 24; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
            } else {
                data->result = emb_result(data->result, BREAK);

                sprintf(str, "get start sector: %d length: %d DONE !!!\n", s->sda, m->sda);  
                print_f(mlogPool, "auto_20", str);  
            }
                
            memset(g, 0, sizeof(struct info16Bit_s));
            break;
        case WAIT:

            if ((data->ansp0 & 0xff) == g->opcode) {
                sprintf(str, "ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data);  
                print_f(mlogPool, "auto_20", str);  
                data->result = emb_result(data->result, NEXT);
            }
            
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_21(struct psdata_s *data)
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;
    struct SdAddrs_s *s, *m;
    FILE *fp;

    rlt = abs_result(data->result);
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_05", str);  

    switch (rlt) {
        case STINIT:
            //fp = data->rs->pmch->fdsk.vsd;
            s = &data->rs->pmch->sdst;
            m = &data->rs->pmch->sdln;
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            //memset(p, 0, sizeof(struct info16Bit_s));

            if (m->f & 0x200) {
                s->f = 0;
                m->f = 0;
                
                sprintf(str, "send start sector: %d/%d, data length: %d/%d confirm !!!\n", s->n, s->sda, m->n, m->sda);  
                print_f(mlogPool, "auto_21", str);  
                data->result = emb_result(data->result, BREAK);
            
            } else {
                if ((s->sda) && (m->sda)) {
                    p->opcode = g->opcode;
                    p->data = g->data;

                    sprintf(str, "get start sector: %d, total length: %d , secPrClst: %d, data length: %d confirm !!!\n", s->n, m->sda, g->data, m->n);  
                    print_f(mlogPool, "auto_21", str);  

                    ch = 59; 
                    rs_ipc_put(data->rs, &ch, 1);
                    data->result = emb_result(data->result, WAIT);

                } else {
                    s->f = 0;
                    m->f = 0;

                    sprintf(str, "ERROR!!! get start sector: %d, total length: %d , flag: 0x%x, 0x%x !!!\n", s->sda, m->sda, s->f, m->f);  
                    print_f(mlogPool, "auto_21", str);  
                    data->result = emb_result(data->result, BREAK);
                }
            }

            memset(g, 0, sizeof(struct info16Bit_s));
            break;
        case WAIT:
            g = &data->rs->pmch->get;

            if ((data->ansp0 & 0xff) == g->opcode) {
                sprintf(str, "ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data);  
                print_f(mlogPool, "auto_05", str);  
                data->result = emb_result(data->result, NEXT);
            }
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_22(struct psdata_s *data)
{ 
    char str[128], ch=0, ix=0;
    uint8_t op=0, dt=0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;
    struct SdAddrs_s *s, *m;
	
    rlt = abs_result(data->result);	
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_22", str);  

    switch (rlt) {
        case STINIT:
            s = &data->rs->pmch->sdst;
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            memset(p, 0, sizeof(struct info16Bit_s));

            op = g->opcode;

            switch (op) {
            case OP_STSEC_00:
            case OP_STSEC_01:
            case OP_STSEC_02:
            case OP_STSEC_03:
                ix = (OP_STSEC_03 - op) & 0xf;
                //s->d[ix] = g->data;
                //s->f &= ~(0x1 << ix);

                sprintf(str, "!! flag: 0x%.2x data: 0x%.2x !!- 1\n", s->f, s->d[ix]);  
                print_f(mlogPool, "auto_22", str);  

                if (s->f & (0x1 << ix)) {
                    p->opcode = g->opcode;
                    p->data = s->d[ix];
                } else {
                    p->opcode = g->opcode;
                    p->data = g->data;
                    s->d[ix] = g->data;
                }
                
                sprintf(str, "!! flag: 0x%.2x data: 0x%.2x !!- 2\n", s->f, s->d[ix]);  
                print_f(mlogPool, "auto_22", str);  

                ch = 24; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);

                memset(g, 0, sizeof(struct info16Bit_s));			
                break;
            default:
                sprintf(str, "ERROR!!! get pkt: 0x%.2x 0x%.2x break!!\n", g->opcode, g->data);  
                print_f(mlogPool, "auto_22", str);  
                data->result = emb_result(data->result, NEXT);
                break;
            }
            break;
        case WAIT:
            g = &data->rs->pmch->get;
            op = g->opcode;
            switch (op) {
            case OP_STSEC_00:
            case OP_STSEC_01:
            case OP_STSEC_02:
            case OP_STSEC_03:
                s = &data->rs->pmch->sdst;

                if ((data->ansp0 & 0xff) == op) {
                    ix = (OP_STSEC_03 - op) & 0xf;
                    
                    if (s->f & (0x1 << ix)) {
                        s->f &= ~(0x1 << ix);                    
                    } 

                    sprintf(str, "ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data);  
                    print_f(mlogPool, "auto_22", str);  
                    data->result = emb_result(data->result, NEXT);
                }
                break;
            case 0: /* may add counter to prevent dead lock */
            default:
                break;
            }

            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);

}
static int stauto_23(struct psdata_s *data)
{ 
    char str[128], ch=0, ix=0;
    uint8_t op=0, dt=0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;
    struct SdAddrs_s *m;

    rlt = abs_result(data->result);
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_23", str);  

    switch (rlt) {
        case STINIT:
            m = &data->rs->pmch->sdln;
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            memset(p, 0, sizeof(struct info16Bit_s));

            op = g->opcode;

            switch (op) {
            case OP_STLEN_00:
            case OP_STLEN_01:
            case OP_STLEN_02:
            case OP_STLEN_03:
                ix = (OP_STLEN_03 - op) & 0xf;
                //m->d[ix] = g->data;
                //m->f &= ~(0x1 << ix);

                sprintf(str, "!! flag: 0x%.2x opcode: 0x%.2x data: 0x%.2x !! - 1\n", m->f, g->opcode, g->data);  
                print_f(mlogPool, "auto_23", str);  

                if (m->f & (0x1 << ix)) {
                    p->opcode = g->opcode;
                    p->data = m->d[ix];
                } else {
                    p->opcode = g->opcode;
                    p->data = g->data;
                    m->d[ix] = g->data;
                }

                sprintf(str, "!! flag: 0x%.2x opcode: 0x%.2x, data: 0x%.2x !! - 2\n", m->f, p->opcode, p->data);  
                print_f(mlogPool, "auto_23", str);  

                ch = 24; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);

                memset(g, 0, sizeof(struct info16Bit_s));			
                break;
            default:
                sprintf(str, "ERROR!!! get pkt: 0x%.2x 0x%.2x break!!\n", g->opcode, g->data);  
                print_f(mlogPool, "auto_23", str);  
                data->result = emb_result(data->result, NEXT);
                break;
            }
            break;
        case WAIT:
            g = &data->rs->pmch->get;
            op = g->opcode;
            switch (op) {
            case OP_STLEN_00:
            case OP_STLEN_01:
            case OP_STLEN_02:
            case OP_STLEN_03:
                m = &data->rs->pmch->sdln;

                if ((data->ansp0 & 0xff) == op) {
                    ix = (OP_STLEN_03 - op) & 0xf;

                    if (m->f & (0x1 << ix)) {
                        m->f &= ~(0x1 << ix);
                    }

                    if (op == OP_STLEN_03) {
                        data->ansp0 |= ((m->f >> 8) << 8);
                    }

                    sprintf(str, "m->f: 0x%x, ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to next!!\n", m->f, data->ansp0, g->opcode, g->data);  
                    print_f(mlogPool, "auto_23", str);  
                    data->result = emb_result(data->result, NEXT);

                }
                break;
            case 0: /* may add counter to prevent dead lock */
            default:
                break;
            }

            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);

}

static int stauto_24(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct info16Bit_s *p, *g, *t;
    int *pSdInit = 0;

    rlt = abs_result(data->result);	
    sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    print_f(mlogPool, "auto_24", str);  

    switch (rlt) {
        case STINIT:
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            t = &data->rs->pmch->tmp;
            memset(p, 0, sizeof(struct info16Bit_s));
            p->opcode = g->opcode;
            p->data = g->data;

            ch = 24; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);

            memset(g, 0, sizeof(struct info16Bit_s));
            break;
        case WAIT:
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            
            if (g->opcode == p->opcode) {
                sprintf(str, "ansp:0x%.2x, recv: 0x%.2x 0x%.2x, send: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data, p->opcode, p->data);  
                print_f(mlogPool, "auto_24", str);  

                sleep(2);
                pSdInit = data->rs->psd_init;
                *pSdInit = 1;
                sprintf(str, "Set SD status: 0x%.2x \n", *pSdInit);  
                print_f(mlogPool, "auto_24", str);  

                data->result = emb_result(data->result, NEXT);
            }	
            
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);


}

static int stauto_25(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;
    int *pSdInit = 0, SDinit = 0;
    
    rlt = abs_result(data->result);	
    sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    print_f(mlogPool, "auto_25", str);  

    switch (rlt) {
        case STINIT:
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            memset(p, 0, sizeof(struct info16Bit_s));

            pSdInit = data->rs->psd_init;
            SDinit = *pSdInit;
            msync(pSdInit, sizeof(int), MS_SYNC);
            
            p->opcode = OP_SDSTATS;
            p->data = SDinit & 0xff;

            sprintf(str, "SD status: 0x%.2x \n", p->data);  
            print_f(mlogPool, "auto_25", str);  

            ch = 24; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);

            memset(g, 0, sizeof(struct info16Bit_s));
            break;
        case WAIT:
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            
            if (g->opcode == p->opcode) {
                sprintf(str, "ansp:0x%.2x, recv: 0x%.2x 0x%.2x, send: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data, p->opcode, p->data);  
                print_f(mlogPool, "auto_25", str);  
                data->result = emb_result(data->result, NEXT);
            }	
            
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_26(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;
    struct SdAddrs_s *s, *m;

    rlt = abs_result(data->result);
    
    switch (rlt) {
        case STINIT:
            s = &data->rs->pmch->sdst;
            m = &data->rs->pmch->sdln;
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            //memset(p, 0, sizeof(struct info16Bit_s));

            p->opcode = OP_CROP_01;
            p->data = 0xff;
            
            ch = 60; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            sprintf(str, "go %d\n", ch);  
            print_f(mlogPool, "auto_26", str);  

            memset(g, 0, sizeof(struct info16Bit_s));
            break;
        case WAIT:
            g = &data->rs->pmch->get;

            if ((data->ansp0 & 0xff) == g->opcode) {
                sprintf(str, "ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data);  
                print_f(mlogPool, "auto_26", str);  
                data->result = emb_result(data->result, NEXT);
            }
            
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_27(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;
    struct SdAddrs_s *s, *m;

    rlt = abs_result(data->result);
    
    switch (rlt) {
        case STINIT:
            s = &data->rs->pmch->sdst;
            m = &data->rs->pmch->sdln;
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            //memset(p, 0, sizeof(struct info16Bit_s));

            p->opcode = OP_CROP_02;
            p->data = 0xff;

            ch = 61; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            sprintf(str, "go %d\n", ch);  
            print_f(mlogPool, "auto_27", str);  

            memset(g, 0, sizeof(struct info16Bit_s));
            break;
        case WAIT:
            g = &data->rs->pmch->get;

            if ((data->ansp0 & 0xff) == g->opcode) {
                sprintf(str, "ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data);  
                print_f(mlogPool, "auto_27", str);  
                data->result = emb_result(data->result, NEXT);
            }
            
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_28(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;
    struct SdAddrs_s *s, *m;

    rlt = abs_result(data->result);
    
    switch (rlt) {
        case STINIT:
            s = &data->rs->pmch->sdst;
            m = &data->rs->pmch->sdln;
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            //memset(p, 0, sizeof(struct info16Bit_s));

            p->opcode = OP_CROP_03;
            p->data = 0xff;

            ch = 62; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            sprintf(str, "go %d\n", ch);  
            print_f(mlogPool, "auto_28", str);  

            memset(g, 0, sizeof(struct info16Bit_s));
            break;
        case WAIT:
            g = &data->rs->pmch->get;

            if ((data->ansp0 & 0xff) == g->opcode) {
                sprintf(str, "ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data);  
                print_f(mlogPool, "auto_28", str);  
                data->result = emb_result(data->result, NEXT);
            }
            
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_29(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;
    struct SdAddrs_s *s, *m;

    rlt = abs_result(data->result);
    
    switch (rlt) {
        case STINIT:
            s = &data->rs->pmch->sdst;
            m = &data->rs->pmch->sdln;
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            //memset(p, 0, sizeof(struct info16Bit_s));

            p->opcode = OP_CROP_04;
            p->data = 0xff;

            ch = 63; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            sprintf(str, "go %d\n", ch);  
            print_f(mlogPool, "auto_29", str);  

            memset(g, 0, sizeof(struct info16Bit_s));
            break;
        case WAIT:
            g = &data->rs->pmch->get;

            if ((data->ansp0 & 0xff) == g->opcode) {
                sprintf(str, "ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data);  
                print_f(mlogPool, "auto_29", str);  
                data->result = emb_result(data->result, NEXT);
            }
            
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_30(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;
    struct SdAddrs_s *s, *m;

    rlt = abs_result(data->result);
    
    switch (rlt) {
        case STINIT:
            s = &data->rs->pmch->sdst;
            m = &data->rs->pmch->sdln;
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            //memset(p, 0, sizeof(struct info16Bit_s));

            p->opcode = OP_CROP_05;
            p->data = 0xff;

            ch = 64; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            sprintf(str, "go %d\n", ch);  
            print_f(mlogPool, "auto_30", str);  

            memset(g, 0, sizeof(struct info16Bit_s));
            break;
        case WAIT:
            g = &data->rs->pmch->get;

            if ((data->ansp0 & 0xff) == g->opcode) {
                sprintf(str, "ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data);  
                print_f(mlogPool, "auto_30", str);  
                data->result = emb_result(data->result, NEXT);
            }
            
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_31(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;
    struct SdAddrs_s *s, *m;

    rlt = abs_result(data->result);
    
    switch (rlt) {
        case STINIT:
            s = &data->rs->pmch->sdst;
            m = &data->rs->pmch->sdln;
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            //memset(p, 0, sizeof(struct info16Bit_s));

            p->opcode = OP_CROP_06;
            p->data = 0xff;

            ch = 65; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            sprintf(str, "go %d\n", ch);  
            print_f(mlogPool, "auto_31", str);  

            memset(g, 0, sizeof(struct info16Bit_s));
            break;
        case WAIT:
            g = &data->rs->pmch->get;

            if ((data->ansp0 & 0xff) == g->opcode) {
                sprintf(str, "ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data);  
                print_f(mlogPool, "auto_31", str);  
                data->result = emb_result(data->result, NEXT);
            }
            
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_32(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;
    struct SdAddrs_s *s, *m;

    rlt = abs_result(data->result);
    
    switch (rlt) {
        case STINIT:
            s = &data->rs->pmch->sdst;
            m = &data->rs->pmch->sdln;
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            //memset(p, 0, sizeof(struct info16Bit_s));

            p->opcode = OP_IMG_LEN;
            p->data = 0xff;

            ch = 66; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            sprintf(str, "go %d\n", ch);  
            print_f(mlogPool, "auto_32", str);  

            memset(g, 0, sizeof(struct info16Bit_s));
            break;
        case WAIT:
            g = &data->rs->pmch->get;

            if ((data->ansp0 & 0xff) == g->opcode) {
                sprintf(str, "ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data);  
                print_f(mlogPool, "auto_32", str);  
                data->result = emb_result(data->result, NEXT);
            }
            
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_33(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt, *pslen=0, val=0;
    struct info16Bit_s *p, *g;
    struct procRes_s *rs;

    rs = data->rs;
    rlt = abs_result(data->result);	
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_33", str);  

    switch (rlt) {
        case STINIT:
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            memset(p, 0, sizeof(struct info16Bit_s));
            p->opcode = g->opcode;
            p->data = g->data;
            pslen = rs->pscnlen;

            val = *pslen;
            val &= ~0xffffff00;
            val |= g->data << 8;

            *pslen = val;

            ch = 24; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);

            memset(g, 0, sizeof(struct info16Bit_s));
            break;
        case WAIT:
            g = &data->rs->pmch->get;

            if ((data->ansp0 & 0xff) == g->opcode) {
                sprintf(str, "ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data);  
                print_f(mlogPool, "auto_33", str);  
                data->result = emb_result(data->result, NEXT);
            }			
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_34(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt, *pslen=0, val=0;
    struct info16Bit_s *p, *g;
    struct procRes_s *rs;

    rs = data->rs;
    rlt = abs_result(data->result);	
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_34", str);  

    switch (rlt) {
        case STINIT:
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            memset(p, 0, sizeof(struct info16Bit_s));
            p->opcode = g->opcode;
            p->data = g->data;
            pslen = rs->pscnlen;

            val = *pslen;
            val &= ~0xffff00ff;
            val |= g->data;

            *pslen = val;

            ch = 24; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);

            memset(g, 0, sizeof(struct info16Bit_s));
            break;
        case WAIT:
            g = &data->rs->pmch->get;

            if ((data->ansp0 & 0xff) == g->opcode) {
                sprintf(str, "ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data);  
                print_f(mlogPool, "auto_34", str);  
                data->result = emb_result(data->result, NEXT);
            }			
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_35(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;
    int *pSdInit = 0, SDinit = 0;
    
    rlt = abs_result(data->result);	
    sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    print_f(mlogPool, "auto_35", str);  

    switch (rlt) {
        case STINIT:
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            memset(p, 0, sizeof(struct info16Bit_s));

            pSdInit = data->rs->psd_init;
            SDinit = *pSdInit;
            msync(pSdInit, sizeof(int), MS_SYNC);
            
            p->opcode = 0;
            p->data = 0;

            ch = 24; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);

            memset(g, 0, sizeof(struct info16Bit_s));
            break;
        case WAIT:
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            
            if (g->opcode == p->opcode) {
                sprintf(str, "ansp:0x%.2x, recv: 0x%.2x 0x%.2x, send: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data, p->opcode, p->data);  
                print_f(mlogPool, "auto_35", str);  
                data->result = emb_result(data->result, NEXT);
            }	
            
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_36(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt, *popt, val=0;
    int tid=0;
    struct info16Bit_s *p, *g;
    struct procRes_s *rs;
    
    rs = data->rs;
    popt = rs->poptable;

    rlt = abs_result(data->result);	
    sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    print_f(mlogPool, "auto_36", str);  

    switch (rlt) {
        case STINIT:
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            memset(p, 0, sizeof(struct info16Bit_s));

            p->opcode = g->opcode;
            p->data = g->data;
            tid = g->opcode - OP_FFORMAT;

            if (tid < OPT_SIZE) {
                ch = 24; 
                rs_ipc_put(data->rs, &ch, 1);
                data->result = emb_result(data->result, WAIT);
                memset(g, 0, sizeof(struct info16Bit_s));
            } else {
                sprintf(str, "ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to BREAK!!!!\n", data->ansp0, g->opcode, g->data);  
                print_f(mlogPool, "auto_36", str);  
                data->result = emb_result(data->result, BREAK);
            }

            break;
        case WAIT:
            g = &data->rs->pmch->get;
            if ((data->ansp0 & 0xff) == g->opcode) {           
                tid = g->opcode - OP_FFORMAT;
                if (tid < OPT_SIZE) {
                    val = popt[tid];
                    popt[tid] = g->data;
                    sprintf(str, "tid:%d, original data: 0x%.8x, ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to next!!\n", tid, val, data->ansp0, g->opcode, g->data);  
                    print_f(mlogPool, "auto_36", str);  
                    data->result = emb_result(data->result, NEXT);
                } else {
                    sprintf(str, "tid: %d, out of range, ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to BREAK!!!!\n", tid, data->ansp0, g->opcode, g->data);  
                    print_f(mlogPool, "auto_36", str);  
                    data->result = emb_result(data->result, BREAK);  
                }
            }			
            
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_37(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct info16Bit_s *p, *g, *t;
    
    rlt = abs_result(data->result);	

    g = &data->rs->pmch->get;
    p = &data->rs->pmch->cur;
    t = &data->rs->pmch->tmp;
    sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    print_f(mlogPool, "auto_37", str);  

    switch (rlt) {
        case STINIT:
            
            memset(p, 0, sizeof(struct info16Bit_s));

            t->opcode = g->opcode;
            t->data = g->data;
            
            ch = 48; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);

            memset(g, 0, sizeof(struct info16Bit_s));
            break;
        case WAIT:

            if (data->ansp0 == 1) {
                sprintf(str, "ansp:0x%.2x, go to next!!\n", data->ansp0);  
                print_f(mlogPool, "auto_37", str);  

                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, BREAK);
            }
            
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_38(struct psdata_s *data) 
{
    #define TEST_LEN (30)
    //int testSeq[TEST_LEN] = {9, 13, 14, 15, 16};
    int testSeq[TEST_LEN] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15, 16, 17,18,19,20,21,22, 23,24,25, 26, 27, 28, 29};
    char str[128], ch = 0;
    uint32_t rlt;
    int ret = 0;
    struct procRes_s *rs;
    struct aspMetaData_s *pmetaIn, *pmetaOut;
    struct aspMetaMass_s *pmass;
    struct info16Bit_s *p, *g, *t;
    struct aspMetaData_s *pmetaInDuo, *pmetaOutDuo;
    struct aspMetaMass_s *pmassDuo;
    uint32_t funcbits=0;
    struct DiskFile_s *fd;
    struct aspConfig_s *pct=0, *pdt=0;
    
    rs = data->rs;
    fd = &rs->pmch->fdsk;
    pmetaIn = rs->pmetain;
    pmetaOut = rs->pmetaout;
    pmass = rs->pmetaMass;
    pmetaInDuo = rs->pmetainduo;
    pmetaOutDuo = rs->pmetaoutduo;
    pmassDuo = rs->pmetaMassduo;

    g = &rs->pmch->get;
    p = &rs->pmch->cur;
    t = &rs->pmch->tmp;

    pct = rs->pcfgTable;
    rlt = abs_result(data->result);	
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_38", str);  

    switch (rlt) {
        case STINIT:
            if (t->opcode == OP_META_DAT) {
                pdt = &pct[ASPOP_FILE_FORMAT];
                aspMetaClear(0, rs, 1);
                aspMetaClear(0, rs, 0);

                switch (t->data) {
                    case ASPMETA_POWON_INIT:
                        aspMetaBuild(ASPMETA_FUNC_CONF, 0, rs);
                        break;
                    case ASPMETA_SCAN_GO: /* todo: meta before scan  */
                        //aspMetaBuild(ASPMETA_FUNC_CROP, 0, rs);
                        break;
                    case ASPMETA_SCAN_COMPLETE:
                        pmass->massStart = 12;
                        pmass->massGap = 8;
                        aspMetaBuild(ASPMETA_FUNC_NONE, 0, rs);

                        pmass->massRecd = pmass->massUsed / 4;
                        ret = aspMetaBuild(ASPMETA_FUNC_CROP, 0, rs);
                        if ((ret == 0) || (ret == 1)) {
                            pmass->massIdx += 1;
                        }

                        aspMetaBuild(ASPMETA_FUNC_IMGLEN, 0, rs);
                        break;
                    case ASPMETA_CROP_300DPI:
                    case ASPMETA_OCR:
                        pmass->massStart = 12;
                        pmass->massGap = 8;
                        //pmass->massIdx = 9;
                        //t->info += 1;
                        //pmass->massIdx = testSeq[t->info % TEST_LEN];
                        pmass->massIdx += 1;
                        
                        //aspMetaBuild(ASPMETA_FUNC_NONE, 0, rs);
                        break;
                    case ASPMETA_CROP_600DPI:
                        pmass->massStart = 12;
                        pmass->massGap = 8;
                        //pmass->massIdx = 9;
                        //t->info += 1;
                        //pmass->massIdx = testSeq[t->info % TEST_LEN];
                        pmass->massIdx += 1;
                        //aspMetaBuild(ASPMETA_FUNC_NONE, 0, rs);
                        break;
                     /*todo: double side scan S*/
                    case ASPMETA_SCAN_COMPLE_DUO:
                        pmass->massStart = 12;
                        pmass->massGap = 8;
                        aspMetaBuild(ASPMETA_FUNC_NONE, 0, rs);
                        
                        pmass->massRecd = pmass->massUsed / 4;
                        ret = aspMetaBuild(ASPMETA_FUNC_CROP, 0, rs);
 
                        aspMetaBuild(ASPMETA_FUNC_IMGLEN, 0, rs);

                        pmassDuo->massIdx = pmass->massIdx + 1;
                        pmassDuo->massStart = 12;
                        pmassDuo->massGap = 8;
                        aspMetaBuildDuo(ASPMETA_FUNC_NONE, 0, rs);
                        pmassDuo->massRecd = pmassDuo->massUsed / 4;
                        aspMetaBuildDuo(ASPMETA_FUNC_CROP, 0, rs);

                        aspMetaBuildDuo(ASPMETA_FUNC_IMGLEN, 0, rs);

                        if ((ret == 0) ||(ret == 1)) {
                            pmass->massIdx += 1;
                        }

                        break;
                    case ASPMETA_CROP_300DPI_DUO:
                        pmass->massStart = 12;
                        pmass->massGap = 8;
                        //t->info += 1;
                        //pmass->massIdx = testSeq[t->info % TEST_LEN];
                        pmass->massIdx += 1;
                        
                        pmassDuo->massStart = 12;
                        pmassDuo->massGap = 8;
                        break;
                    case ASPMETA_CROP_600DPI_DUO:
                        pmass->massStart = 12;
                        pmass->massGap = 8;
                        //t->info += 1;
                        //pmass->massIdx = testSeq[t->info % TEST_LEN];
                        pmass->massIdx += 1;

                        pmassDuo->massStart = 12;
                        pmassDuo->massGap = 8;
                        break;
                    case ASPMETA_SD:
                        aspMetaBuild(ASPMETA_FUNC_SDRD, 0, rs);
                        break;
                     /*todo: double side scan E*/
                    default:
                        sprintf(str, "Warnning!!!meta get parameter: 0x%.2x, wrong !!! \n", t->data);  
                        print_f(mlogPool, "auto_38", str);  
                        break;
                }

                sprintf(str, "\n\ncheck index, org: %d, duo: %d info: %d\n\n\n", pmass->massIdx, pmassDuo->massIdx, t->info);  
                print_f(mlogPool, "auto_38", str);  

                switch (t->data) {
                    case ASPMETA_POWON_INIT:
                    case ASPMETA_SCAN_GO: /* todo: meta before scan  */
                    case ASPMETA_SCAN_COMPLETE:
                        sprintf(str, "meta func bits: 0x%.2x, go wait !!! \n", pmetaOut->FUNC_BITS);  
                        print_f(mlogPool, "auto_38", str);  

                        ch = 67; 
                        rs_ipc_put(data->rs, &ch, 1);
                        data->result = emb_result(data->result, WAIT); 
                        break;
                    case ASPMETA_CROP_300DPI:
                    case ASPMETA_CROP_600DPI:
                    case ASPMETA_OCR:
                        sprintf(str, "meta func bits: 0x%.2x, go wait !!! \n", pmetaOut->FUNC_BITS);  
                        print_f(mlogPool, "auto_38", str);  

                        ch = 69; 
                        rs_ipc_put(data->rs, &ch, 1);
                        data->result = emb_result(data->result, WAIT); 
                        break;
                     /*todo: double side scan S*/
                    case ASPMETA_SCAN_COMPLE_DUO:
                        sprintf(str, "meta func bits: 0x%.2x, go wait !!! \n", pmetaOut->FUNC_BITS);  
                        print_f(mlogPool, "auto_38", str);  

                        ch = 74; 
                        rs_ipc_put(data->rs, &ch, 1);
                        data->result = emb_result(data->result, WAIT); 
                        break;
                    case ASPMETA_CROP_300DPI_DUO:
                    case ASPMETA_CROP_600DPI_DUO:
                        sprintf(str, "meta func bits: 0x%.2x, go wait !!! \n", pmetaOut->FUNC_BITS);  
                        print_f(mlogPool, "auto_38", str);  

                        ch = 76; 
                        rs_ipc_put(data->rs, &ch, 1);
                        data->result = emb_result(data->result, WAIT); 
                        break;
                    case ASPMETA_SD:
                        sprintf(str, "meta func bits: 0x%.2x, go wait !!! \n", pmetaOut->FUNC_BITS);  
                        print_f(mlogPool, "auto_38", str);  

                        ch = 67; 
                        rs_ipc_put(data->rs, &ch, 1);
                        data->result = emb_result(data->result, WAIT); 
                        break;
                     /*todo: double side scan E*/
                    default:
                        sprintf(str, "Error!! get opcode data 0x%.2x wrong !!! \n", t->data);  
                        print_f(mlogPool, "auto_38", str);  

                        data->result = emb_result(data->result, BREAK);  
                        break;
                }
   
            } else {
                sprintf(str, "Error!! get opcode 0x%.2x wrong !!! \n", t->opcode);  
                print_f(mlogPool, "auto_38", str);  
                
                data->result = emb_result(data->result, BREAK);            
            }
            break;
        case WAIT:

            if (data->ansp0 == 1) {
                funcbits = msb2lsb(&pmetaIn->FUNC_BITS);
                ret = aspMetaRelease(funcbits, 0, rs);
                shmem_dump((char *) rs->pmetain, sizeof(struct aspMetaData_s));
                dbgMeta(funcbits,  pmetaIn);

                if (funcbits & ASPMETA_FUNC_SDRD) {
                    fd->rtops =  OP_SDRD;
                } 
                else if (funcbits & ASPMETA_FUNC_SDWT) {
                    fd->rtops =  OP_SDWT;
                } 
                
                sprintf(str, "ansp:0x%.2x, go to next!!(rtops:0x%x,funcbits:0x%x)\n", data->ansp0, fd->rtops, funcbits);  
                print_f(mlogPool, "auto_38", str);  
                
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, BREAK);
            }
            
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_39(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;
    
    rlt = abs_result(data->result);	
    sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    print_f(mlogPool, "auto_39", str);  

    switch (rlt) {
        case STINIT:
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            memset(p, 0, sizeof(struct info16Bit_s));
            
            ch = 48; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);

            memset(g, 0, sizeof(struct info16Bit_s));
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                sprintf(str, "ansp:0x%.2x, go to next!!\n", data->ansp0);  
                print_f(mlogPool, "auto_39", str);  

                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, BREAK);
            }
            
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_40(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;
    
    rlt = abs_result(data->result);	
    //sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    //print_f(mlogPool, "auto_40", str);  

    switch (rlt) {
        case STINIT:
            ch = 71; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);
            break;
        case WAIT:
            if (data->ansp0 == 1) {
                data->result = emb_result(data->result, NEXT);
            } else if (data->ansp0 == 2) {
                data->result = emb_result(data->result, BREAK);
            }            
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_41(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;
    
    rlt = abs_result(data->result);	
    sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    print_f(mlogPool, "auto_00", str);  

    switch (rlt) {
        case STINIT:
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            memset(p, 0, sizeof(struct info16Bit_s));
            
            ch = 24; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);

            memset(g, 0, sizeof(struct info16Bit_s));
            break;
        case WAIT:
            g = &data->rs->pmch->get;

            if ((data->ansp0 & 0xff) == g->opcode) {
                sprintf(str, "ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data);  
                print_f(mlogPool, "auto_00", str);  
                data->result = emb_result(data->result, NEXT);
            }			
            
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_42(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;
    
    rlt = abs_result(data->result);	
    sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    print_f(mlogPool, "auto_00", str);  

    switch (rlt) {
        case STINIT:
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            memset(p, 0, sizeof(struct info16Bit_s));
            
            ch = 24; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);

            memset(g, 0, sizeof(struct info16Bit_s));
            break;
        case WAIT:
            g = &data->rs->pmch->get;

            if ((data->ansp0 & 0xff) == g->opcode) {
                sprintf(str, "ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data);  
                print_f(mlogPool, "auto_00", str);  
                data->result = emb_result(data->result, NEXT);
            }			
            
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_43(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;
    
    rlt = abs_result(data->result);	
    sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    print_f(mlogPool, "auto_00", str);  

    switch (rlt) {
        case STINIT:
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            memset(p, 0, sizeof(struct info16Bit_s));
            
            ch = 24; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);

            memset(g, 0, sizeof(struct info16Bit_s));
            break;
        case WAIT:
            g = &data->rs->pmch->get;

            if ((data->ansp0 & 0xff) == g->opcode) {
                sprintf(str, "ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data);  
                print_f(mlogPool, "auto_00", str);  
                data->result = emb_result(data->result, NEXT);
            }			
            
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_44(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;
    
    rlt = abs_result(data->result);	
    sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    print_f(mlogPool, "auto_00", str);  

    switch (rlt) {
        case STINIT:
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            memset(p, 0, sizeof(struct info16Bit_s));
            
            ch = 24; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);

            memset(g, 0, sizeof(struct info16Bit_s));
            break;
        case WAIT:
            g = &data->rs->pmch->get;

            if ((data->ansp0 & 0xff) == g->opcode) {
                sprintf(str, "ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data);  
                print_f(mlogPool, "auto_00", str);  
                data->result = emb_result(data->result, NEXT);
            }			
            
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int stauto_45(struct psdata_s *data) 
{
    char str[128], ch = 0;
    uint32_t rlt;
    struct info16Bit_s *p, *g;
    
    rlt = abs_result(data->result);	
    sprintf(str, "result: %.8x ansp:%d\n", data->result, data->ansp0);  
    print_f(mlogPool, "auto_00", str);  

    switch (rlt) {
        case STINIT:
            g = &data->rs->pmch->get;
            p = &data->rs->pmch->cur;
            memset(p, 0, sizeof(struct info16Bit_s));
            
            ch = 24; 
            rs_ipc_put(data->rs, &ch, 1);
            data->result = emb_result(data->result, WAIT);

            memset(g, 0, sizeof(struct info16Bit_s));
            break;
        case WAIT:
            g = &data->rs->pmch->get;

            if ((data->ansp0 & 0xff) == g->opcode) {
                sprintf(str, "ansp:0x%.2x, get pkt: 0x%.2x 0x%.2x, go to next!!\n", data->ansp0, g->opcode, g->data);  
                print_f(mlogPool, "auto_00", str);  
                data->result = emb_result(data->result, NEXT);
            }			
            
            break;
        case NEXT:
            break;
        case BREAK:
            break;
        default:
            break;
    }
    return ps_next(data);
}

static int shmem_rlt_get(struct mainRes_s *mrs, int seq, int p)
{
    int ret, sz, len;
    char ch, *stop_at, dst[16];;

    ret = mrs_ipc_get(mrs, &ch, 1, p);
    if (ret > 0) {
        len = mrs_ipc_get(mrs, &ch, 1, p);
        if ((len > 0) && (ch == 'l')) {
            len = mrs_ipc_get(mrs, dst, 8, p);
            if (len == 8) {
                sz = strtoul(dst, &stop_at, 10);
            }
        }
    
        if ((ch == 'l') && (sz == 0)) {
        } else {
            ring_buf_prod_dual(&mrs->dataRx, seq);
        }
        //printf("[dback] ch:%c rt:%d idx:%d \n", ch,  len, seq);
        //shmem_dump(addr[0], sz);
        if (ch == 'l') {
            ring_buf_set_last_dual(&mrs->dataRx, sz, seq);
        }

    }

    return ret;
}

static int shmem_pop_send(struct mainRes_s *mrs, char **addr, int seq, int p)
{
    char str[128];
    int sz = 0;
    sz = ring_buf_get_dual(&mrs->dataRx, addr, seq);
    //printf("shmem pop:0x%.8x, seq:%d sz:%d\n", *addr, seq, sz);
    if (sz < 0) return (-1);
    sprintf(str, "d%.8xl%.8d\n", *addr, sz);
    print_f(&mrs->plog, "pop", str);
    //printf("[%s]\n", str);
    mrs_ipc_put(mrs, str, 18, p);

    return sz;
}

static int shmem_dump(char *src, int size)
{
    char str[128];
    int inc;
    if (!src) return -1;

    inc = 0;
    sprintf(str, "memdump[0x%.8x] sz%d: \n", src, size);
    print_f(mlogPool, "dump", str);
    while (inc < size) {
        sprintf(str, "%.2x ", *src);
        print_f(NULL, NULL, str);

        if (!((inc+1) % 16)) {
            sprintf(str, "\n");
            print_f(NULL, NULL, str);
        }
        inc++;
        src++;
    }

    return inc;
}
static int shmem_from_str(char **addr, char *dst, char *sz)
{
    char *stop_at;
    int size;
    if ((!addr) || (!dst) || (!sz)) return -1;

    *addr = (char *)strtoul(dst, &stop_at, 16);
    size = strtoul(sz, &stop_at, 10);

    return size;
}

static int ring_buf_info_len(struct shmem_s *pp)
{
    int dualn = 0;
    int leadn = 0;
    int folwn = 0;
    int dist;

    folwn = pp->r->folw.run * pp->slotn + pp->r->folw.seq;
    dualn = pp->r->dual.run * pp->slotn + pp->r->dual.seq;
    leadn = pp->r->lead.run * pp->slotn + pp->r->lead.seq;

    if (dualn > leadn) {
        dist = dualn - folwn;
    } else {
        dist = leadn - folwn;
    }

    return dist;
}

static int ring_buf_init(struct shmem_s *pp)
{
    pp->r->lead.run = 0;
    pp->r->lead.seq = 0;
    pp->r->dual.run = 0;
    pp->r->dual.seq = 1;
    pp->r->prelead.run = 0;
    pp->r->prelead.seq = 0;
    pp->r->predual.run = 0;
    pp->r->predual.seq = 1;
    pp->r->folw.run = 0;
    pp->r->folw.seq = 0;
    pp->lastflg = 0;
    pp->lastsz = 0;
    pp->dualsz = 0;

    msync(pp, sizeof(struct shmem_s), MS_SYNC);
    return 0;
}

static int ring_buf_get_dual(struct shmem_s *pp, char **addr, int sel)
{
    char str[128];
    int dualn = 0;
    int folwn = 0;
    int dist;
    int tmps;

    sel = sel % 2;

    folwn = pp->r->folw.run * pp->slotn + pp->r->folw.seq;

    if (sel) {
        dualn = pp->r->predual.run * pp->slotn + pp->r->predual.seq;
    } else {
        dualn = pp->r->prelead.run * pp->slotn + pp->r->prelead.seq;
    }

    dist = dualn - folwn;
    //sprintf(str, "get d:%d, %d /%d \n", dist, dualn, folwn);
    //print_f(mlogPool, "ring", str);

    if (dist > (pp->slotn - 3))  return -1;

    if (sel) {
        if ((pp->r->predual.seq + 2) < pp->slotn) {
            *addr = pp->pp[pp->r->predual.seq+2];
        } else {
            tmps = (pp->r->predual.seq+2) % pp->slotn;
            *addr = pp->pp[tmps];
        }
    } else {
        if ((pp->r->prelead.seq + 2) < pp->slotn) {
            *addr = pp->pp[pp->r->prelead.seq+2];
        } else {
            tmps = (pp->r->prelead.seq+2) % pp->slotn;
            *addr = pp->pp[tmps];
        }
    }

    if (sel) {
        if ((pp->r->predual.seq + 2) < pp->slotn) {
            pp->r->predual.seq += 2;
        } else {
            pp->r->predual.seq = 1;
            pp->r->predual.run += 1;
        }
    } else {
        if ((pp->r->prelead.seq + 2) < pp->slotn) {
            pp->r->prelead.seq += 2;
        } else {
            pp->r->prelead.seq = 0;
            pp->r->prelead.run += 1;
        }
    }

    return pp->chksz;	
}

static int ring_buf_get(struct shmem_s *pp, char **addr)
{
    char str[128];
    int leadn = 0;
    int folwn = 0;
    int dist;

    folwn = pp->r->folw.run * pp->slotn + pp->r->folw.seq;
    leadn = pp->r->lead.run * pp->slotn + pp->r->lead.seq;

    dist = leadn - folwn;
    //sprintf(str, "get d:%d, %d \n", dist, leadn, folwn);
    //print_f(mlogPool, "ring", str);

    if (dist > (pp->slotn - 2))  return -1;

    if ((pp->r->lead.seq + 1) < pp->slotn) {
        *addr = pp->pp[pp->r->lead.seq+1];
    } else {
        *addr = pp->pp[0];
    }

    return pp->chksz;	
}

static int ring_buf_set_last_dual(struct shmem_s *pp, int size, int sel)
{
    char str[128];
    sel = sel % 2;

    if (sel) {
        pp->dualsz = size;
    } else {
        pp->lastsz = size;
    }
    pp->lastflg += 1;

    //sprintf(str, "[last] d:%d l:%d flg:%d \n", pp->dualsz, pp->lastsz, pp->lastflg);
    //print_f(mlogPool, "ring", str);

    msync(pp, sizeof(struct shmem_s), MS_SYNC);
    return pp->lastflg;
}

static int ring_buf_set_last(struct shmem_s *pp, int size)
{
    pp->lastsz = size;
    pp->lastflg = 1;

    msync(pp, sizeof(struct shmem_s), MS_SYNC);
    return pp->lastflg;
}
static int ring_buf_prod_dual(struct shmem_s *pp, int sel)
{
    sel = sel % 2;
    if (sel) {
        if ((pp->r->dual.seq + 2) < pp->slotn) {
            pp->r->dual.seq += 2;
        } else {
            pp->r->dual.seq = 1;
            pp->r->dual.run += 1;
        }
    } else {
        if ((pp->r->lead.seq + 2) < pp->slotn) {
            pp->r->lead.seq += 2;
        } else {
            pp->r->lead.seq = 0;
            pp->r->lead.run += 1;
        }
    }
    msync(pp, sizeof(struct shmem_s), MS_SYNC);
    return 0;
}

static int ring_buf_prod(struct shmem_s *pp)
{
    if ((pp->r->lead.seq + 1) < pp->slotn) {
        pp->r->lead.seq += 1;
    } else {
        pp->r->lead.seq = 0;
        pp->r->lead.run += 1;
    }
    msync(pp, sizeof(struct shmem_s), MS_SYNC);
    return 0;
}

static int ring_buf_cons_dual(struct shmem_s *pp, char **addr, int sel)
{
    int ret=-1;
    char str[128];
    int dualn = 0;
    int leadn = 0;
    int folwn = 0;
    int dist;

    folwn = pp->r->folw.run * pp->slotn + pp->r->folw.seq;
    dualn = pp->r->dual.run * pp->slotn + pp->r->dual.seq;
    leadn = pp->r->lead.run * pp->slotn + pp->r->lead.seq;
    if (dualn > leadn) {
        dist = dualn - folwn;
    } else {
        dist = leadn - folwn;
    }

    //sprintf(str, "cons d: %d %d/%d/%d \n", dist, leadn, dualn, folwn);
    //print_f(mlogPool, "ring", str);

    if ((pp->lastflg) && (dist < 1)) return (-1);
    if (dist < 1)  return (-2);

    if ((pp->r->folw.seq + 1) < pp->slotn) {
        *addr = pp->pp[pp->r->folw.seq + 1];
        pp->r->folw.seq += 1;
    } else {
        *addr = pp->pp[0];
        pp->r->folw.seq = 0;
        pp->r->folw.run += 1;
    }

    if ((pp->lastflg) && (dist == 1)) {
        sprintf(str, "[clast] f:%d %d, d:%d %d l: %d %d \n", pp->r->folw.run, pp->r->folw.seq, 
            pp->r->dual.run, pp->r->dual.seq, pp->r->lead.run, pp->r->lead.seq);
        print_f(mlogPool, "ring", str);
        if (dualn > leadn) {
            if ((pp->r->folw.run == pp->r->dual.run) &&
             (pp->r->folw.seq == pp->r->dual.seq)) {
                //return pp->dualsz;
                ret = pp->dualsz;
            }
        } else {
            if ((pp->r->folw.run == pp->r->lead.run) &&
             (pp->r->folw.seq == pp->r->lead.seq)) {
                //return pp->lastsz;
                ret = pp->lastsz;
            }
        }
    }

    if (ret < 0) {
        ret = pp->chksz; 
    }

    //sprintf(str, "cons dual len:%d \n", ret);
    //print_f(mlogPool, "ring", str);

    msync(pp, sizeof(struct shmem_s), MS_SYNC);
    return ret;
}

static int ring_buf_cons(struct shmem_s *pp, char **addr, int *len)
{
    char str[128];
    int leadn = 0;
    int folwn = 0;
    int dist;

    folwn = pp->r->folw.run * pp->slotn + pp->r->folw.seq;
    leadn = pp->r->lead.run * pp->slotn + pp->r->lead.seq;
    dist = leadn - folwn;

    //sprintf(str, "cons, d: %d %d/%d \n", dist, leadn, folwn);
    //print_f(mlogPool, "ring", str);
    *len = 0;

    if (dist < 1)  return -1;

    if ((pp->r->folw.seq + 1) < pp->slotn) {
        *addr = pp->pp[pp->r->folw.seq + 1];
        pp->r->folw.seq += 1;
    } else {
        *addr = pp->pp[0];
        pp->r->folw.seq = 0;
        pp->r->folw.run += 1;
    }

    if ((pp->lastflg) && (dist == 1)) {
        if ((pp->r->folw.run == pp->r->lead.run) &&
            (pp->r->folw.seq == pp->r->lead.seq)) {

            *len = pp->lastsz;
            return -2;
        }
    }

    msync(pp, sizeof(struct shmem_s), MS_SYNC);

    *len = pp->chksz;
    return 0;
}

static int mtx_data(int fd, uint8_t *rx_buff, uint8_t *tx_buff, int num, int pksz, int maxsz)
{
    int pkt_size;
    int ret, i, errcnt; 
    int remain;

    struct spi_ioc_transfer *tr = malloc(sizeof(struct spi_ioc_transfer) * num);
    memset(tr, 0, sizeof(struct spi_ioc_transfer));
    
    uint8_t tg;
    uint8_t *tx = tx_buff;
    uint8_t *rx = rx_buff;
    pkt_size = pksz;
    remain = maxsz;

    for (i = 0; i < num; i++) {
        remain -= pkt_size;
        if (remain < 0) break;

        tr[i].tx_buf = (unsigned long)tx;
        tr[i].rx_buf = (unsigned long)rx;
        tr[i].len = pkt_size;
        tr[i].delay_usecs = 0;
        tr[i].speed_hz = SPI_SPEED;
        tr[i].bits_per_word = 8;
        
        tx += pkt_size;
        rx += pkt_size;
    }
    
    ret = ioctl(fd, SPI_IOC_MESSAGE(i), tr);
    if (ret < 0) {
        printf("can't send spi message, ret: %d\n", ret);
    }
    //printf("tx/rx len: %d\n", ret);
    
    free(tr);
    return ret;
}

static int mtx_data_16(int fd, uint16_t *rx_buff, uint16_t *tx_buff, int num, int pksz, int maxsz)
{
    int pkt_size;
    int ret, i, errcnt; 
    int remain;

    struct spi_ioc_transfer *tr = malloc(sizeof(struct spi_ioc_transfer) * num);
    memset(tr, 0, sizeof(struct spi_ioc_transfer));
    
    uint8_t tg;
    uint16_t *tx = tx_buff;
    uint16_t *rx = rx_buff;
    pkt_size = pksz;
    remain = maxsz;

    for (i = 0; i < num; i++) {
        remain -= pkt_size;
        if (remain < 0) break;

        tr[i].tx_buf = (unsigned long)tx;
        tr[i].rx_buf = (unsigned long)rx;
        tr[i].len = pkt_size;
        tr[i].delay_usecs = 0;
        tr[i].speed_hz = SPI_SPEED;
        tr[i].bits_per_word = 16;
        
        tx += pkt_size;
        rx += pkt_size;
    }
    
    ret = ioctl(fd, SPI_IOC_MESSAGE(i), tr);
    if (ret < 0)
        printf("can't send spi message\n");
    
    //printf("tx/rx len: %d\n", ret);
    
    free(tr);
    return ret;
}

static int mrs_ipc_get(struct mainRes_s *mrs, char *str, int size, int idx)
{
    int ret;
    ret = read(mrs->pipeup[idx].rt[0], str, size);
    return ret;
}

static int mrs_ipc_put(struct mainRes_s *mrs, char *str, int size, int idx)
{
    int ret;
    ret = write(mrs->pipedn[idx].rt[1], str, size);
    return ret;
}

static int rs_ipc_put(struct procRes_s *rs, char *str, int size)
{
    int ret;
    ret = write(rs->ppipeup->rt[1], str, size);
    return ret;
}

static int rs_ipc_get(struct procRes_s *rs, char *str, int size)
{
    int ret;
    ret = read(rs->ppipedn->rt[0], str, size);
    return ret;
}

static int pn_init(struct procRes_s *rs)
{
    close(rs->ppipedn->rt[1]);
    close(rs->ppipeup->rt[0]);
    return 0;
}

static int pn_end(struct procRes_s *rs)
{
    close(rs->ppipedn->rt[0]); 
    close(rs->ppipeup->rt[1]);
    return 0;
}

static int p5_init(struct procRes_s *rs)
{
    int ret;
    ret = pn_init(rs);
    return ret;
}

static int p5_end(struct procRes_s *rs)
{
    int ret;
    ret = pn_end(rs);
    return ret;
}

static int p4_init(struct procRes_s *rs)
{
    int ret;
    ret = pn_init(rs);
    return ret;
}

static int p4_end(struct procRes_s *rs)
{
    int ret;
    ret = pn_end(rs);
    return ret;
}

static int p3_init(struct procRes_s *rs)
{
    int ret;
    ret = pn_init(rs);
    return ret;
}

static int p3_end(struct procRes_s *rs)
{
    int ret;
    ret = pn_end(rs);
    return ret;
}

static int p2_init(struct procRes_s *rs)
{
    int ret;
    ret = pn_init(rs);
    return ret;
}

static int p2_end(struct procRes_s *rs)
{
    int ret;
    ret = pn_end(rs);
    return ret;
}

static int p1_init(struct procRes_s *rs)
{
    int ret;
    ret = pn_init(rs);
    return ret;
}

static int p1_end(struct procRes_s *rs)
{
    int ret;
    ret = pn_end(rs);
    return ret;
}

static int p0_init(struct mainRes_s *mrs) 
{
    close(mrs->pipedn[0].rt[0]);
    close(mrs->pipedn[1].rt[0]);
    close(mrs->pipedn[2].rt[0]);
    close(mrs->pipedn[3].rt[0]);
    close(mrs->pipedn[4].rt[0]);
	
    close(mrs->pipeup[0].rt[1]);
    close(mrs->pipeup[1].rt[1]);
    close(mrs->pipeup[2].rt[1]);
    close(mrs->pipeup[3].rt[1]);
    close(mrs->pipeup[4].rt[1]);

    return 0;
}

static int p0_end(struct mainRes_s *mrs)
{
    close(mrs->pipeup[0].rt[0]);
    close(mrs->pipeup[1].rt[0]);
    close(mrs->pipeup[2].rt[0]);
    close(mrs->pipeup[3].rt[0]);
    close(mrs->pipeup[4].rt[0]);

    close(mrs->pipedn[0].rt[1]);
    close(mrs->pipedn[1].rt[1]);
    close(mrs->pipedn[2].rt[1]);
    close(mrs->pipedn[3].rt[1]);
    close(mrs->pipedn[4].rt[1]);

    kill(mrs->sid[0]);
    kill(mrs->sid[1]);
    kill(mrs->sid[2]);
    kill(mrs->sid[3]);
    kill(mrs->sid[4]);

    fclose(mrs->fs);
    munmap(mrs->dataRx.pp[0], 1024*SPI_TRUNK_SZ);
    munmap(mrs->dataTx.pp[0], 256*SPI_TRUNK_SZ);
    munmap(mrs->cmdRx.pp[0], 256*SPI_TRUNK_SZ);
    munmap(mrs->cmdTx.pp[0], 512*SPI_TRUNK_SZ);
    free(mrs->dataRx.pp);
    free(mrs->cmdRx.pp);
    free(mrs->dataTx.pp);
    free(mrs->cmdTx.pp);
    return 0;
}

static int cmdfunc_01(int argc, char *argv[])
{
    struct mainRes_s *mrs;
    char str[256], ch;
    if (!argv) return -1;

    mrs = (struct mainRes_s *)argv[0];

    ch = '\0';

    if (argc == 0) {
        ch = 'p';
    }

    if (argc == 2) {
        ch = '3';
    }

    if (argc == 1) {
        ch = '4';
    }

    if (argc == 5) {
        ch = 's';
    }

    if (argc == 7) {
        ch = 'b';
    }

    sprintf(str, "cmdfunc_01 argc:%d ch:%c\n", argc, ch); 
    print_f(mlogPool, "cmdfunc", str);

    mrs_ipc_put(mrs, &ch, 1, 6);
    return 1;
}

static int dbg(struct mainRes_s *mrs)
{
    int ci, pi, ret, idle = 0;
    char cmd[256], *addr[3];
    char poll[32] = "poll";

    struct cmd_s cmdtab[8] = {{0, "poll", cmdfunc_01}, {1, "command", cmdfunc_01}, {2, "data", cmdfunc_01}, {3, "run", cmdfunc_01}, 
                                {4, "aspect", cmdfunc_01}, {5, "leo", cmdfunc_01}, {6, "mothership", cmdfunc_01}, {7, "lanch", cmdfunc_01}};

    p0_init(mrs);
	
    while (1) {
        /* command parsing */
        ci = 0;    
        ci = mrs_ipc_get(mrs, cmd, 256, 5);

        if (ci > 0) {
            cmd[ci] = '\0';
            //sprintf(mrs->log, "get [%s] size:%d \n", cmd, ci);
            //print_f(&mrs->plog, "DBG", mrs->log);
        } else {
            if (idle > 5) {
                idle = 0;
                //strcpy(cmd, poll);
            } else {
                idle ++;
                printf_flush(&mrs->plog, mrs->flog);
                usleep(1000);
                continue;
            }
        }

        pi = 0;
        while (pi < 8) {
            if ((strlen(cmd) == strlen(cmdtab[pi].str))) {
                if (!strncmp(cmd, cmdtab[pi].str, strlen(cmdtab[pi].str))) {
                    break;
                }
            }
            pi++;
        }

        /* command execution */
        if (pi < 8) {
            addr[0] = (char *)mrs;
            sprintf(mrs->log, "input [%d]%s\n", pi, cmdtab[pi].str, cmdtab[pi].id, cmd);
            print_f(&mrs->plog, "DBG", mrs->log);
            ret = cmdtab[pi].pfunc(cmdtab[pi].id, addr);

            //mrs_ipc_put(mrs, "t", 1, 6);
        }

        cmd[0] = '\0';

        printf_flush(&mrs->plog, mrs->flog);
    }

    p0_end(mrs);
}

static int fs00(struct mainRes_s *mrs, struct modersp_s *modersp){ return 0; }
static int fs01(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    //sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
    //print_f(&mrs->plog, "fs01", mrs->log);
    /* wait until control pin become 0 */

    mrs_ipc_put(mrs, "g", 1, 3);
    modersp->m = modersp->m + 1;
    return 0; 
}
static int fs02(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int len=0;
    char ch=0;
    //sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
    //print_f(&mrs->plog, "fs02", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'G')){
        if (modersp->d) {
            modersp->m = modersp->d;
            modersp->d = 0;
            return 2;
        } else {
            modersp->r = 1;
            return 1;
        }
    }
    return 0; 
}
static int fs03(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int bitset = 0;
    struct info16Bit_s *p;

    //sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
    //print_f(&mrs->plog, "fs03", mrs->log);

    bitset = 1;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
    sprintf(mrs->log, "Set spi 0 slave ready: %d\n", bitset);
    print_f(&mrs->plog, "fs03", mrs->log);
    bitset = 1;
    ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
    sprintf(mrs->log, "Set spi 1 slave ready: %d\n", bitset);
    print_f(&mrs->plog, "fs03", mrs->log);

    modersp->r = 1;
    return 1; 
}

static int fs04(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0, bitset=0;
    char ch=0;
    struct info16Bit_s *p;

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        p = &mrs->mchine.get;
        //sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
        //print_f(&mrs->plog, "fs04", mrs->log);

        if (p->opcode == OP_PON) {
            bitset = 1;
            ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
            sprintf(mrs->log, "Set spi 0 slave ready: %d\n", bitset);
            print_f(&mrs->plog, "fs04", mrs->log);
            bitset = 1;
            ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
            sprintf(mrs->log, "Set spi 1 slave ready: %d\n", bitset);
            print_f(&mrs->plog, "fs04", mrs->log);

            modersp->r = 1;
            return 1;
        } else {
            modersp->m = modersp->m - 1;        
            return 2;
        }
    }
    return 0; 
}

static int fs05(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    //sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
    //print_f(&mrs->plog, "fs05", mrs->log);
    /* wait until control pin become 1 */
	
    mrs_ipc_put(mrs, "b", 1, 3);
    modersp->m = modersp->m + 1;
    return 0; 
}
static int fs06(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int len=0;
    char ch=0;
    //sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
    //print_f(&mrs->plog, "fs06", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'B')){
        if (modersp->d) {
            modersp->m = modersp->d;
            modersp->d = 0;
            return 2;
        } else {
            modersp->r = 1;
            return 1;
        }
    }
    return 0; 
}

static int fs07(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    struct info16Bit_s *p;

    p = &mrs->mchine.cur;
    sprintf(mrs->log, "set 0x%.2x 0x%.2x \n", p->opcode, p->data);
    print_f(&mrs->plog, "fs07", mrs->log);

    mrs->mchine.seqcnt += 1;
    if (mrs->mchine.seqcnt >= 0x8) {
        mrs->mchine.seqcnt = 0;
    }

    p->opcode = OP_RDY;
    p->inout = 0;
    p->seqnum = mrs->mchine.seqcnt;
	
    mrs_ipc_put(mrs, "c", 1, 3);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs08(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;
    struct info16Bit_s *p;

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        p = &mrs->mchine.get;
        sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
        print_f(&mrs->plog, "fs08", mrs->log);

        if (p->opcode == OP_RDY) {
            modersp->r = 1;
            return 1;
        } else {
            modersp->m = modersp->m - 1;        
            modersp->r = 2;
            return 2;
        }
    }
    return 0; 
}

static int fs09(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    struct info16Bit_s *p;


    p = &mrs->mchine.cur;


    p->opcode = OP_QRY;
    p->data = 0;
	
    sprintf(mrs->log, "put op:0x%.2x data:0x%.2x \n", p->opcode, p->data);
    print_f(&mrs->plog, "fs09", mrs->log);

    mrs_ipc_put(mrs, "c", 1, 3);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs10(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;
    struct info16Bit_s *p;

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);
        p = &mrs->mchine.get;
        sprintf(mrs->log, "get op:0x%.2x data:0x%.2x \n", p->opcode, p->data);
        print_f(&mrs->plog, "fs10", mrs->log);

        switch (p->opcode) {
        case OP_SINGLE: /* currently support */       
        case OP_MSINGLE:
        case OP_HANDSCAN:
        case OP_NOTESCAN:
        case OP_POLL:
        case OP_DOUBLE: 
        case OP_MDOUBLE:
        case OP_SDRD:                                       
        case OP_SDWT:                                       
        case OP_SDAT:                                     
        case OP_FREESEC:
        case OP_USEDSEC:
        case OP_SDINIT:
        case OP_SDSTATS:
        case OP_STSEC_00:                                  
        case OP_STSEC_01:                                  
        case OP_STSEC_02:                                  
        case OP_STSEC_03:                                  
        case OP_STLEN_00:                                  
        case OP_STLEN_01:                                  
        case OP_STLEN_02:                                  
        case OP_STLEN_03:                                  
        case OP_FFORMAT:                                  
        case OP_COLRMOD:                                  
        case OP_COMPRAT:                                  
        case OP_RESOLTN:                                  
        case OP_SCANGAV:                                  
        case OP_MAXWIDH:                                  
        case OP_WIDTHAD_H:                                
        case OP_WIDTHAD_L:                                
        case OP_SCANLEN_H:                                
        case OP_SCANLEN_L:                              
        case OP_INTERIMG:
        case OP_AFEIC:
        case OP_EXTPULSE:
        case OP_ACTION:
        case OP_RAW:
        case OP_RGRD:
        case OP_RGWT:
        case OP_RGDAT:
        case OP_RGADD_H:
        case OP_RGADD_L:

        case OP_CROP_01:
        case OP_CROP_02:
        case OP_CROP_03:
        case OP_CROP_04:
        case OP_CROP_05:
        case OP_CROP_06:
        case OP_IMG_LEN:
        case OP_META_DAT:

        case OP_SUPBACK:
        case OP_SAVE:
        case OP_FUNCTEST_00:
        case OP_FUNCTEST_01:
        case OP_FUNCTEST_02:
        case OP_FUNCTEST_03:
        case OP_FUNCTEST_04:
        case OP_FUNCTEST_05:
        case OP_FUNCTEST_06:
        case OP_FUNCTEST_07:
        case OP_FUNCTEST_08:
        case OP_FUNCTEST_09:
        case OP_FUNCTEST_10:
        case OP_FUNCTEST_11:
        case OP_FUNCTEST_12:
        case OP_FUNCTEST_13:
        case OP_FUNCTEST_14:
        case OP_FUNCTEST_15:

            modersp->r = p->opcode | (p->data << 8);
            return 1;
            break;                                       
        default:
            modersp->m = modersp->m - 1;        
            return 2;
            break;
        }
    }

    return 0; 
}

static int fs11(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    struct info16Bit_s *p, *t;
    //sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
    //print_f(&mrs->plog, "fs11", mrs->log);

    p = &mrs->mchine.cur;
    t = &mrs->mchine.tmp;
    
    mrs->mchine.seqcnt += 1;
    if (mrs->mchine.seqcnt >= 0x8) {
        mrs->mchine.seqcnt = 0;
    }

    p->opcode = t->opcode;
    p->data = t->data;

    mrs_ipc_put(mrs, "c", 1, 3);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs12(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;
    struct info16Bit_s *p, *c;

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        p = &mrs->mchine.get;
        c = &mrs->mchine.cur;
        //sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
        //print_f(&mrs->plog, "fs12", mrs->log);

        if ((p->opcode == c->opcode) && (p->data == c->data)) {
            modersp->r = 1;
            return 1;
        } else {
            modersp->m = modersp->m - 1;        
            modersp->r = 2;
            return 2;
        }
    }
    return 0; 
}

static int fs13(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int bitset=0, ret;
    bitset = 0;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
    sprintf(mrs->log, "spi0 Set data mode: %d\n", bitset);
    print_f(&mrs->plog, "fs13", mrs->log);
    bitset = 0;
    ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
    sprintf(mrs->log, "spi1 Set data mode: %d\n", bitset);
    print_f(&mrs->plog, "fs13", mrs->log);

    int bits = 8;
    ret = ioctl(mrs->sfm[0], SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (ret == -1) {
        sprintf(mrs->log, "can't set bits per word"); 
        print_f(&mrs->plog, "fs13", mrs->log);
    }
    ret = ioctl(mrs->sfm[0], SPI_IOC_RD_BITS_PER_WORD, &bits); 
    if (ret == -1) {
        sprintf(mrs->log, "can't get bits per word"); 
        print_f(&mrs->plog, "fs13", mrs->log);
    }

    ret = ioctl(mrs->sfm[1], SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (ret == -1) {
        sprintf(mrs->log, "can't set bits per word"); 
        print_f(&mrs->plog, "fs13", mrs->log);
    }
    ret = ioctl(mrs->sfm[1], SPI_IOC_RD_BITS_PER_WORD, &bits); 
    if (ret == -1) {
        sprintf(mrs->log, "can't get bits per word"); 
        print_f(&mrs->plog, "fs13", mrs->log);
    }

    ring_buf_init(&mrs->dataRx);
    mrs->dataRx.r->folw.seq = 1;

    modersp->r = 1;
    return 1;
}
static int fs14(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    sprintf(mrs->log, "read patern file\n");
    print_f(&mrs->plog, "fs14", mrs->log);

    mrs_ipc_put(mrs, "r", 1, 1);
    modersp->m = modersp->m + 1;
    modersp->v = 0;

    mrs_ipc_put(mrs, "b", 1, 3);
    mrs_ipc_put(mrs, "b", 1, 4);
    modersp->m = 21;
    modersp->d = 15;

    return 0;
}

static int fs15(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;

    len = mrs_ipc_get(mrs, &ch, 1, 1);
    if (len > 0) {
        sprintf(mrs->log, "ch:%c, v:%d \n", ch, modersp->v);
        print_f(&mrs->plog, "fs15", mrs->log);

        if ((ch == 'r') || (ch == 'e')) {
            if (modersp->v % 2) {
                mrs_ipc_put(mrs, &ch, 1, 4);
            } else {
                mrs_ipc_put(mrs, &ch, 1, 3);
            }
            modersp->m = modersp->m + 1;
        }
    }

    return 0; 
}

static int fs16(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0, bitset;
    char ch=0;

    //sprintf(mrs->log, "fs16 \n");
    //print_f(&mrs->plog, "fs16", mrs->log);

    if (modersp->v % 2) {
        len = mrs_ipc_get(mrs, &ch, 1, 4);
    } else {
        len = mrs_ipc_get(mrs, &ch, 1, 3);
    }

    if (len > 0) {    
        //sprintf(mrs->log, "get ch:%c \n", ch);
        //print_f(&mrs->plog, "fs16", mrs->log);

        if (ch == 'r') {
            modersp->m = modersp->m - 1;
            modersp->v += 1;
            modersp->r = 2;
            return 2;
        }
        if (ch == 'e') {
            sprintf(mrs->log, "get ch:%c \n", ch);
            print_f(&mrs->plog, "fs16", mrs->log);
            modersp->r = 1;
            return 1;
        }
    }

    return 0; 
}

static int fs17(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    struct info16Bit_s *p;

    sprintf(mrs->log, "get OP_FIH \n");
    print_f(&mrs->plog, "fs17", mrs->log);

    p = &mrs->mchine.cur;

    mrs->mchine.seqcnt += 1;
    if (mrs->mchine.seqcnt >= 0x8) {
        mrs->mchine.seqcnt = 0;
    }

    p->opcode = OP_FIH;
    p->inout = 0;
    p->seqnum = mrs->mchine.seqcnt;
	
    mrs_ipc_put(mrs, "c", 1, 3);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs18(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int len=0;
    char ch=0;
    struct info16Bit_s *p;

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        p = &mrs->mchine.get;
        //sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
        //print_f(&mrs->plog, "fs18", mrs->log);

        if (p->opcode == OP_FIH) {
            modersp->m = 23;
        } else {
            modersp->m = modersp->m - 1; 
            modersp->r = 2;
            return 2;
        }
    }
    return 0; 
}

static int fs19(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int bitset;
    bitset = 1;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
    //printf("Set spi 0 slave ready: %d\n", bitset);
    sprintf(mrs->log, "Set spi 0 slave ready: %d\n", bitset);
    print_f(&mrs->plog, "fs19", mrs->log);

    bitset = 1;
    ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
    //printf("Set spi 1 slave ready: %d\n", bitset);
    sprintf(mrs->log, "Set spi 1 slave ready: %d\n", bitset);
    print_f(&mrs->plog, "fs19", mrs->log);

    modersp->r = 1;
    return 1;
}

static int fs20(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int bitset;
    usleep(1);

    bitset = 1;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
    sprintf(mrs->log, "[%d]Set RDY pin %d, cnt:%d\n",0, bitset, modersp->d);
    print_f(&mrs->plog, "fs20", mrs->log);

    bitset = 1;
    ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
    sprintf(mrs->log, "[%d]Set RDY pin %d, cnt:%d\n",1, bitset, modersp->d);
    print_f(&mrs->plog, "fs20", mrs->log);

    usleep(1000);

    bitset = 0;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
    sprintf(mrs->log, "[%d]Set RDY pin %d, cnt:%d\n",0, bitset, modersp->d);
    print_f(&mrs->plog, "fs20", mrs->log);

    bitset = 0;
    ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
    sprintf(mrs->log, "[%d]Set RDY pin %d, cnt:%d\n",1, bitset, modersp->d);
    print_f(&mrs->plog, "fs20", mrs->log);

    usleep(DATA_END_PULL_LOW_DELAY);

    bitset = 1;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
    sprintf(mrs->log, "[%d]Set RDY pin %d, cnt:%d\n",0, bitset, modersp->d);
    print_f(&mrs->plog, "fs20", mrs->log);

    bitset = 1;
    ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
    sprintf(mrs->log, "[%d]Set RDY pin %d, cnt:%d\n",1, bitset, modersp->d);
    print_f(&mrs->plog, "fs20", mrs->log);

    usleep(1000);

    bitset = 0;
    ioctl(mrs->sfm[0], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
    sprintf(mrs->log, "[%d]Get RDY pin %d, cnt:%d\n",0, bitset, modersp->d);
    print_f(&mrs->plog, "fs20", mrs->log);

    bitset = 0;
    ioctl(mrs->sfm[1], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
    sprintf(mrs->log, "[%d]Get RDY pin %d, cnt:%d\n",1, bitset, modersp->d);
    print_f(&mrs->plog, "fs20", mrs->log);

    //usleep(60000);

    bitset = 0;
    ioctl(mrs->sfm[0], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
    sprintf(mrs->log, "[%d]Get RDY pin %d, cnt:%d\n",0, bitset, modersp->d);
    print_f(&mrs->plog, "fs20", mrs->log);

    bitset = 0;
    ioctl(mrs->sfm[1], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
    sprintf(mrs->log, "[%d]Get RDY pin %d, cnt:%d\n",1, bitset, modersp->d);
    print_f(&mrs->plog, "fs20", mrs->log);

/*    
    bitset = 1;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
    sprintf(mrs->log, "Set spi 0 slave ready: %d\n", bitset);
    print_f(&mrs->plog, "fs20", mrs->log);

    bitset = 1;
    ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
    sprintf(mrs->log, "Set spi 1 slave ready: %d\n", bitset);
    print_f(&mrs->plog, "fs20", mrs->log);
*/
    modersp->r = 1;
    return 1;
}

static int fs21(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int len=0;
    char ch=0;

    //sprintf(mrs->log, "fs21 \n");
    //print_f(&mrs->plog, "fs21", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'B')){
        modersp->m = modersp->m + 1;
    }
    return 0; 

}

static int fs22(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int len=0;
    char ch=0;

    //sprintf(mrs->log, "fs21 \n");
    //print_f(&mrs->plog, "fs22", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 4);
    if ((len > 0) && (ch == 'B')){
        modersp->m = modersp->d;
        modersp->d = 0;
    }
    return 0; 

}

static int fs23(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int bitset;
    bitset = 1;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY

    sprintf(mrs->log, "Set spi 0 slave ready: %d\n", bitset);
    print_f(&mrs->plog, "fs23", mrs->log);

    bitset = 1;
    ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY

    sprintf(mrs->log, "Set spi 1 slave ready: %d\n", bitset);
    print_f(&mrs->plog, "fs23", mrs->log);

    modersp->r = 1;
    return 1;
}

static int fs24(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    struct info16Bit_s *p;

    p = &mrs->mchine.cur;
    sprintf(mrs->log, "put  0x%.2x 0x%.2x \n",p->opcode, p->data);
    print_f(&mrs->plog, "fs24", mrs->log);

    msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);
	
    mrs_ipc_put(mrs, "c", 1, 3);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs25(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int ret = 0;
    int len=0;
    char ch=0;
    struct info16Bit_s *g;

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        g = &mrs->mchine.get;
        //sprintf(mrs->log, "pull 0x%.2x 0x%.2x \n", g->opcode, g->data);
        //print_f(&mrs->plog, "fs25", mrs->log);

        if (g->opcode) {
            if (!modersp->r) {
                modersp->r = g->opcode;
            }
            if (modersp->d) {
                modersp->m = modersp->d;
                modersp->d = 0;
                ret = 2;
            } else {
                ret = 1;
            }
        } else {
            modersp->m = modersp->m - 1;        
            ret = 2;
        }
        sprintf(mrs->log, "r:%d, d:%d m:%d, op:0x%x,0x%x\n",modersp->r, modersp->d, modersp->m, g->opcode, g->data);
        print_f(&mrs->plog, "fs25", mrs->log);

        return ret;
    }
    return 0; 
}

static int fs26(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    char ch=0;
    uint8_t *uch;
    uint32_t startSec = 0, secNum = 0;
    uint32_t startAddr = 0, bLength = 0, maxLen=0;
    int ret = 0, len=0;
    struct info16Bit_s *c, *p;
    struct DiskFile_s *pf;

    sprintf(mrs->log, "read disk file\n");
    print_f(&mrs->plog, "fs26", mrs->log);

    msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);
    pf = &mrs->mchine.fdsk;

    uch = mrs->mchine.sdst.d;
    startSec = mrs->mchine.sdst.n;
    secNum = mrs->mchine.sdln.n;
    mrs->mchine.sdst.f = 0;
    mrs->mchine.sdln.f = 0;

    maxLen = pf->rtMax;

    sprintf(mrs->log, "secStr: %d (%x,%x,%x,%x), secLen: %d\n", startSec, uch[0], uch[1], uch[2], uch[3], secNum);
    print_f(&mrs->plog, "fs26", mrs->log);


    startAddr = startSec * SEC_LEN;
    bLength = secNum * SEC_LEN;

    if (bLength > maxLen) {
        ret = -1;
        goto end;
    }
#if 0 /* pre load in main, so don't have to do it again */
    if (!pf->vsd) {
        ret = -2;
        goto end;
    }


    ret = fseek(pf->vsd, startAddr, SEEK_SET);
    if (ret) {
        sprintf(mrs->log, "seek to %d failed!!! \n", startAddr);
        print_f(&mrs->plog, "fs26", mrs->log);
        goto end;
    }

    char *buff=pf->sdt; 
    int totsz = bLength;

    while (totsz) {
        if (totsz < 32768) {
            len = totsz;
            totsz = 0;
        } else {
            len = 32768;
            totsz -= 32768;
        }
    
        len = fread(buff, 1, len, pf->vsd);
        sprintf(mrs->log, "read file size: %d/%d \n", len, totsz);
        print_f(&mrs->plog, "fs26", mrs->log);
        buff += len;
    }
//#else
    len = fread(pf->sdt, 1, bLength, pf->vsd);
    sprintf(mrs->log, "read file size: %d/%d \n", len, bLength);
    print_f(&mrs->plog, "fs26", mrs->log);
    msync(pf->sdt, bLength, MS_SYNC);
#endif

    if (!ret) {
        pf->rtlen = bLength;

        c = &mrs->mchine.cur;
        p = &mrs->mchine.get;
        c->opcode = p->opcode;
        c->data = p->data;
        modersp->m = 24;
        modersp->d = 27;
        return 2;
    } else {
        pf->rtlen = 0;
    }

end:
/*
    fclose(pf->vsd);
    pf->vsd = 0;
*/
    c = &mrs->mchine.cur;

    c->opcode = 0xe0;
    c->data = 0x5f;
    modersp->m = 24;
    modersp->d = 0;
    modersp->r = 2;
    return 2;
}

static int fs27(struct mainRes_s *mrs, struct modersp_s *modersp) 
{
    int bitset, ret;
    bitset = 0;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
    sprintf(mrs->log, "spi0 set data mode: %d\n", bitset);
    print_f(&mrs->plog, "fs27", mrs->log);

    bitset = 1;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
    sprintf(mrs->log, "spi0 set ready: %d\n", bitset);
    print_f(&mrs->plog, "fs27", mrs->log);


    int bits = 8;
    ret = ioctl(mrs->sfm[0], SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (ret == -1) {
        sprintf(mrs->log, "can't set bits per word"); 
        print_f(&mrs->plog, "fs27", mrs->log);
    }
    ret = ioctl(mrs->sfm[0], SPI_IOC_RD_BITS_PER_WORD, &bits); 
    if (ret == -1) {
        sprintf(mrs->log, "can't get bits per word"); 
        print_f(&mrs->plog, "fs27", mrs->log);
    }

    ret = ioctl(mrs->sfm[1], SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (ret == -1) {
        sprintf(mrs->log, "can't set bits per word"); 
        print_f(&mrs->plog, "fs27", mrs->log);
    }
    ret = ioctl(mrs->sfm[1], SPI_IOC_RD_BITS_PER_WORD, &bits); 
    if (ret == -1) {
        sprintf(mrs->log, "can't get bits per word"); 
        print_f(&mrs->plog, "fs27", mrs->log);
    }

    modersp->m = modersp->m + 1;

    return 0;
}

static int fs28(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    char ch;
	
    sprintf(mrs->log, "start to do  OP_SDAT !!!\n");
    print_f(&mrs->plog, "fs28", mrs->log);

    ch = 's';
    mrs_ipc_put(mrs, &ch, 1, 3);

    modersp->m = modersp->m+1;
    return 0; 
}

static int fs29(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;

    //sprintf(mrs->log, "wait!!!\n");
    //print_f(&mrs->plog, "fs29", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'S')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        sprintf(mrs->log, "get S OP_SDAT DONE!!!\n");
        print_f(&mrs->plog, "fs29", mrs->log);

        modersp->m = modersp->m + 1;
        return 0;
    } else if ((len > 0) && (ch == 's')) {
        sprintf(mrs->log, "get S OP_SDAT FAIL!!!\n");
        print_f(&mrs->plog, "fs29", mrs->log);

        modersp->r = 2;
        return 1;
    }

    return 0; 
}

static int fs30(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int bitset;
    //usleep(60000);

    bitset = 0;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
    sprintf(mrs->log, "Set spi%d RDY pin: %d, finished!! \n", 0, bitset);
    print_f(&mrs->plog, "fs30", mrs->log);

    //bitset = 0;
    //ioctl(mrs->sfm[0], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
    //sprintf(mrs->log, "Get spi%d RDY pin: %d \n", 0, bitset);
    //print_f(&mrs->plog, "fs30", mrs->log);

    //usleep(60000);

    usleep(DATA_END_PULL_LOW_DELAY);

    bitset = 1;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
    sprintf(mrs->log, "Set spi%d RDY pin: %d, finished!! \n", 0, bitset);
    print_f(&mrs->plog, "fs30", mrs->log);

    bitset = 0;
    ioctl(mrs->sfm[0], _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
    sprintf(mrs->log, "Get spi%d RDY pin: %d \n", 0, bitset);
    print_f(&mrs->plog, "fs30", mrs->log);

    bitset = 1;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
    sprintf(mrs->log, "spi0 set ready: %d\n", bitset);
    print_f(&mrs->plog, "fs30", mrs->log);

    modersp->d = modersp->m + 1;
    modersp->m = 5;
    return 2;

}

static int fs31(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    struct info16Bit_s *p;

    sprintf(mrs->log, "send OP_FIH \n");
    print_f(&mrs->plog, "fs31", mrs->log);

    p = &mrs->mchine.cur;

    mrs->mchine.seqcnt += 1;
    if (mrs->mchine.seqcnt >= 0x8) {
        mrs->mchine.seqcnt = 0;
    }

    p->opcode = OP_FIH;
    p->inout = 0;
    p->seqnum = mrs->mchine.seqcnt;
	
    mrs_ipc_put(mrs, "c", 1, 3);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs32(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int len=0;
    char ch=0;
    struct info16Bit_s *p;

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        p = &mrs->mchine.get;
        sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
        print_f(&mrs->plog, "fs32", mrs->log);

        if (p->opcode == OP_FIH) {
            if (modersp->d) {
                modersp->m = modersp->d;
                modersp->d = 0;
            } else {
                modersp->m = modersp->m + 1;
            }
            modersp->v = 1;
        } else {
            modersp->m = modersp->m - 1; 
            modersp->r = 2;
            return 2;
        }
    }
    return 0; 
}

static int fs33(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int bitset;

    bitset = modersp->v;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
    sprintf(mrs->log, "spi0 set ready: %d\n", bitset);
    print_f(&mrs->plog, "fs33", mrs->log);

    if (modersp->d) {
        modersp->m = modersp->d;
        modersp->d = 0;
    } else {
        modersp->m = modersp->m + 1;
    }

    return 0;
}

static int fs34(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    struct info16Bit_s *p;

    p = &mrs->mchine.cur;
    sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
    print_f(&mrs->plog, "fs34", mrs->log);

    mrs->mchine.seqcnt += 1;
    if (mrs->mchine.seqcnt >= 0x8) {
        mrs->mchine.seqcnt = 0;
    }

    p->opcode = OP_RDY;
    p->inout = 0;
    p->seqnum = mrs->mchine.seqcnt;
	
    mrs_ipc_put(mrs, "c", 1, 3);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs35(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int ret=0;
    int len=0;
    char ch=0;
    struct info16Bit_s *p;

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        p = &mrs->mchine.get;
        sprintf(mrs->log, "get %d 0x%.1x 0x%.1x 0x%.2x \n", p->inout, p->seqnum, p->opcode, p->data);
        print_f(&mrs->plog, "fs35", mrs->log);

        if (p->opcode == OP_RDY) {

            if (mrs->mchine.fdsk.rtops == OP_SDWT) {
                modersp->m = 37;
                return 2;
            } else {
                modersp->r = 1;
                return 1;
            }
        } else {
            modersp->r = 2;
            return 1;
        }
    }
    return 0; 
}

static int fs36(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int ret = -1;
    struct info16Bit_s *p, *c;
    //char diskname[128] = "/mnt/mmc2/disk_rmImg.bin";
    //char diskname[128] = "/mnt/mmc2/disk_rx127_255log.bin";
    //char diskname[128] = "/mnt/mmc2/disk_golden.bin";
    //char diskname[128] = "/mnt/mmc2/debug_fat.bin";
    //char diskname[128] = "/mnt/mmc2/disk_05.bin";
    //char diskname[128] = "/dev/mmcblk0p1";
    //char diskname[128] = "/dev/mmcblk0";
    //char diskname[128] = "/mnt/mmc2/empty_256.dsk";
    //char diskname[128] = "/mnt/mmc2/folder_256.dsk";
    //char diskname[128] = "/mnt/mmc2/disk_LFN_64.bin";
    //char diskname[128] = "/mnt/mmc2/disk_onefolder.bin";
    //char diskname[128] = "/mnt/mmc2/mingssd.bin";
    //char diskname[128] = "/mnt/mmc2/16g_ghost.bin";
    //char diskname[128] = "/mnt/mmc2/32g_beformat_rmmore.bin";
    //char diskname[128] = "/mnt/mmc2/32g_32k_format.bin";
    //char diskname[128] = "/mnt/mmc2/32g_64k_format.bin";
    //char diskname[128] = "/mnt/mmc2/32g_64k_format_add_01_rm.bin";
    //char diskname[128] = "/mnt/mmc2/32g_64k_format_add_01.bin";
    //char diskname[128] = "/mnt/mmc2/64G_2pics.bin";
    //char diskname[128] = "/mnt/mmc2/64G_empty.bin";
    //char diskname[128] = "/mnt/mmc2/32g_ios_format.bin";
    //char diskname[128] = "/mnt/mmc2/128g_ios_format.bin";
    //char diskname[128] = "/mnt/mmc2/32g_and.bin";
    //char diskname[128] = "/mnt/mmc2/32g_win.bin";
    char diskname[128] = "/mnt/mmc2/32g_mass.bin";
    //char diskname[128] = "/mnt/mmc2/phy_32g_empty_diff.bin";
    //char diskname[128] = "/mnt/mmc2/phy_folder.bin";
    struct DiskFile_s *fd;
    FILE *fp=0;
    struct aspMetaData_s *pmeta;
    uint32_t funcbits=0;
    
    pmeta = mrs->metain;
    fd = &mrs->mchine.fdsk;
    p = &mrs->mchine.get;
    c = &mrs->mchine.cur;
/*
    fd->vsd = fopen(diskname, "r");
    if (fd->vsd == NULL) {
        sprintf(mrs->log, "disk file [%s] open failed!!! \n", diskname);
        print_f(&mrs->plog, "fs36", mrs->log);
        goto err;
    }

    ret = fseek(fd->vsd, 0, SEEK_END);
    if (ret) {
        sprintf(mrs->log, "seek file [%s] failed!!! \n", diskname);
        print_f(&mrs->plog, "fs36", mrs->log);
        goto err;
    }
*/
    funcbits = msb2lsb(&pmeta->FUNC_BITS);
    sprintf(mrs->log, "open disk file [%s], op:0x%x, func: 0x%x \n", diskname, p->opcode, funcbits);
    print_f(&mrs->plog, "fs36", mrs->log);

    if (p->opcode == OP_SDRD) {
        fd->rtops =  OP_SDRD;
    } else if (p->opcode == OP_SDWT) {
        fd->rtops =  OP_SDWT;
#if DIRECT_WT_DISK 
        fp = fopen(diskname, "w+");
#endif
    } else {
        if (funcbits & ASPMETA_FUNC_SDRD) {
            fd->rtops =  OP_SDRD;
            modersp->m = 67;
            modersp->d = 0;
            return 2;
        } 
        else if (funcbits & ASPMETA_FUNC_SDWT) {
            fd->rtops =  OP_SDWT;
            modersp->m = 67;
            modersp->d = 0;
            return 2;
        } 
        else {
            goto err;
        }
    }

    if (!fd->sdt) {
        sprintf(mrs->log, "ERROR!!! open file [%s] failed, opcodet: 0x%.2x/0x%.2x !!!\n", diskname, p->opcode, p->data);  
        print_f(&mrs->plog, "fs36", mrs->log);
        goto err;
    } else {
        if (fp) {
            fd->vsd = fp;
        }
        c->opcode = p->opcode;
        c->data = p->data;
        ret = 0;
    }

err:
    if (ret) {
        c->opcode = 0xE0;
        c->data = 0x5f;
    }

    p->opcode = 0;
    p->data = 0;

    modersp->m = 24;
    modersp->d = 0;
    modersp->r = 0;
    return 2;
}

static int fs37(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int acusz = 0;
    int ret=0;
    int len=0;
    char ch=0;
    struct DiskFile_s *pf;
    //acusz = rs->pmch->sdln.n * SEC_LEN;
    acusz = mrs->mchine.sdln.n * SEC_LEN;
            
    pf = &mrs->mchine.fdsk;
    sprintf(mrs->log, "save to file size: %d\n", pf->rtMax);
    print_f(&mrs->plog, "fs37", mrs->log);

    msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);
    msync(pf->sdt, pf->rtMax, MS_SYNC);
    
/*
    ret = fseek(pf->vsd, 0, SEEK_SET);
    if (ret) {
        sprintf(mrs->log, "seek file to zero failed!!! \n");
        print_f(&mrs->plog, "fs37", mrs->log);
        modersp->r = 2;
        return 1;
    }

    if (pf->vsd) {
#if 0
        msync(pf->sdt, acusz, MS_SYNC);            
        ret = fwrite(pf->sdt, 1, acusz, mrs->fs);
        fflush(mrs->fs);
        sync();
//#else 
        ret = fwrite(pf->sdt, 1, pf->rtMax, pf->vsd);
#endif
        sprintf(mrs->log, "write file size: %d/%d \n", ret, pf->rtMax);
        print_f(&mrs->plog, "fs37", mrs->log);
        fflush(pf->vsd);
        fsync((int)pf->vsd);
        
    } else {
        ret = -1;
        sprintf(mrs->log, "write file size: %d/%d failed!!!, fp == 0\n", ret, pf->rtMax);
        print_f(&mrs->plog, "fs37", mrs->log);
    }


    //sync();

    fclose(pf->vsd);
    pf->vsd = 0;
*/
    pf->rtops = 0 ;

    if (ret > 0) {
        modersp->r = 1;
    } else {
        modersp->r = 2;
    }

    return 1; 
}

static int fs38(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    struct info16Bit_s *p;

    p = &mrs->mchine.cur;
    sprintf(mrs->log, "put  0x%.2x 0x%.2x \n",p->opcode, p->data);
    print_f(&mrs->plog, "fs38", mrs->log);

    //msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);
	
    mrs_ipc_put(mrs, "c", 1, 3);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs39(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int ret = 0;
    int len=0;
    char ch=0;
    struct info16Bit_s *g, *c;

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        c = &mrs->mchine.cur;
        g = &mrs->mchine.get;
        sprintf(mrs->log, "pull 0x%.2x 0x%.2x \n", g->opcode, g->data);
        print_f(&mrs->plog, "fs39", mrs->log);

        if (g->opcode) {
            if (g->opcode == c->opcode) {
                modersp->r = 1;
            } else {
                modersp->r = 2;
            }

            if (modersp->d) {
                modersp->m = modersp->d;
                modersp->d = 0;
                ret = 2;
            } else {
                ret = 1;
            }
        } else {
            modersp->r = 2;
            ret = 1;
        }
        sprintf(mrs->log, "r:%d, d:%d m:%d, op:0x%x,0x%x\n",modersp->r, modersp->d, modersp->m, g->opcode, g->data);
        print_f(&mrs->plog, "fs39", mrs->log);

        return ret;
    }
    return 0; 
}

static int fs40(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    struct info16Bit_s *p;

    p = &mrs->mchine.cur;

    sprintf(mrs->log, "set 0x%.2x 0x%.2x \n", p->opcode, p->data);
    print_f(&mrs->plog, "fs40", mrs->log);

    mrs->mchine.seqcnt += 1;
    if (mrs->mchine.seqcnt >= 0x8) {
        mrs->mchine.seqcnt = 0;
    }

    p->opcode = OP_SUPBACK;
    p->data = 0;

    mrs_ipc_put(mrs, "c", 1, 3);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs41(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;
    struct info16Bit_s *p;

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        p = &mrs->mchine.get;

        sprintf(mrs->log, "get 0x%.2x 0x%.2x \n", p->opcode, p->data);
        print_f(&mrs->plog, "fs41", mrs->log);

        if ((p->opcode == OP_SINGLE) && (p->data == SINSCAN_DUAL_STRM)) {
            modersp->m = modersp->m + 1;
            return 2;
        } else {
            modersp->m = modersp->m - 1;        
            return 2;
        }
    }
    return 0; 
}

static int fs42(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    struct info16Bit_s *p;
    p = &mrs->mchine.cur;

    sprintf(mrs->log, "set 0x%.2x 0x%.2x \n", p->opcode, p->data);
    print_f(&mrs->plog, "fs2", mrs->log);

    mrs->mchine.seqcnt += 1;
    if (mrs->mchine.seqcnt >= 0x8) {
        mrs->mchine.seqcnt = 0;
    }

    p->opcode = OP_SUPBACK;
    p->data = 0;
    
    mrs_ipc_put(mrs, "c", 1, 3);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs43(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;
    struct info16Bit_s *p;

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        p = &mrs->mchine.get;

        sprintf(mrs->log, "get 0x%.2x 0x%.2x \n", p->opcode, p->data);
        print_f(&mrs->plog, "fs43", mrs->log);

        if (p->opcode == OP_SUPBACK) {
            if (modersp->d) {
                modersp->m = modersp->d;
                modersp->d = 0;
                return 2;
            } else {
                modersp->r = 1;
                return 1;
            }
        } else {
            modersp->m = modersp->m - 1;        
            modersp->r = 2;
            return 2;
        }
    }
    return 0; 
}

static int fs44(struct mainRes_s *mrs, struct modersp_s *modersp) 
{
    int bitset=0, ret;

    sprintf(mrs->log, "set up\n");
    print_f(&mrs->plog, "fs44", mrs->log);
    

    bitset = 0;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
    sprintf(mrs->log, "spi0 Set data mode: %d\n", bitset);
    print_f(&mrs->plog, "fs13", mrs->log);
    bitset = 0;
    ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
    sprintf(mrs->log, "spi1 Set data mode: %d\n", bitset);
    print_f(&mrs->plog, "fs13", mrs->log);

    int bits = 8;
    ret = ioctl(mrs->sfm[0], SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (ret == -1) {
        sprintf(mrs->log, "can't set bits per word"); 
        print_f(&mrs->plog, "fs13", mrs->log);
    }
    ret = ioctl(mrs->sfm[0], SPI_IOC_RD_BITS_PER_WORD, &bits); 
    if (ret == -1) {
        sprintf(mrs->log, "can't get bits per word"); 
        print_f(&mrs->plog, "fs13", mrs->log);
    }

    ret = ioctl(mrs->sfm[1], SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (ret == -1) {
        sprintf(mrs->log, "can't set bits per word"); 
        print_f(&mrs->plog, "fs13", mrs->log);
    }
    ret = ioctl(mrs->sfm[1], SPI_IOC_RD_BITS_PER_WORD, &bits); 
    if (ret == -1) {
        sprintf(mrs->log, "can't get bits per word"); 
        print_f(&mrs->plog, "fs13", mrs->log);
    }

    ring_buf_init(&mrs->dataTx);
    
    //mrs_ipc_put(mrs, "k", 1, 1);
    mrs_ipc_put(mrs, "b", 1, 3);
    
    modersp->m = modersp->m + 1;
    return 2;
}

static int fs45(struct mainRes_s *mrs, struct modersp_s *modersp) 
{
    int len=0;
    char ch=0;

    //sprintf(mrs->log, "wait for CTL pin to be high \n");
    //print_f(&mrs->plog, "fs45", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'B')){
        modersp->v = 0;
        mrs_ipc_put(mrs, "k", 1, 3);
        modersp->m = modersp->m + 1;
        return 2;
    }

    return 0; 
}

static int fs46(struct mainRes_s *mrs, struct modersp_s *modersp) 
{
    int len=0;
    char ch=0;

    //sprintf(mrs->log, "cnt: %d\n", modersp->v);
    //print_f(&mrs->plog, "fs46", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    while (len) {
        if (len > 0) {
            modersp->v += 1;

            sprintf(mrs->log, "cnt: %d ch:%c\n", modersp->v, ch);
            print_f(&mrs->plog, "fs46", mrs->log);

            if (ch == 'k') {
                mrs_ipc_put(mrs, "k", 1, 1);
            } else if (ch == 'K') {
                mrs_ipc_put(mrs, "K", 1, 1);
                modersp->m = modersp->m + 1;
                return 2;
            }
        }
        len = mrs_ipc_get(mrs, &ch, 1, 3);
    }
    return 0; 

}
static int fs47(struct mainRes_s *mrs, struct modersp_s *modersp) 
{
    int len=0;
    char ch=0;

    //sprintf(mrs->log, "save file done \n");
    //print_f(&mrs->plog, "fs47", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 1);
    if ((len > 0) && (ch == 'K')){
        modersp->r = 1;
        return 1;
    }

    return 0; 
}

static int fs48(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    struct info16Bit_s *p, *t;
    p = &mrs->mchine.cur;
    t = &mrs->mchine.tmp;
    
    sprintf(mrs->log, "set 0x%.2x 0x%.2x \n", p->opcode, p->data);
    print_f(&mrs->plog, "fs48", mrs->log);

    mrs->mchine.seqcnt += 1;
    if (mrs->mchine.seqcnt >= 0x8) {
        mrs->mchine.seqcnt = 0;
    }

    p->opcode = t->opcode;
    p->data = t->data;
    
    mrs_ipc_put(mrs, "c", 1, 3);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs49(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;
    struct info16Bit_s *p, *c;

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        p = &mrs->mchine.get;
        c = &mrs->mchine.cur;

        sprintf(mrs->log, "get 0x%.2x 0x%.2x \n", p->opcode, p->data);
        print_f(&mrs->plog, "fs49", mrs->log);

        if ((p->opcode == c->opcode) && (p->data == c->data)) {
            if (modersp->d) {
                modersp->m = modersp->d;
                modersp->d = 0;
                return 2;
            } else {
                modersp->r = 1;
                return 1;
            }
        } else {
            modersp->m = modersp->m - 1;        
            modersp->r = 2;
            return 2;
        }
    }
    return 0; 
}

static int fs50(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int bitset, ret;
    sprintf(mrs->log, "read patern file\n");
    print_f(&mrs->plog, "fs50", mrs->log);
    
    bitset = 0;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
    sprintf(mrs->log, "spi0 Set data mode: %d\n", bitset);
    print_f(&mrs->plog, "fs50", mrs->log);

    int bits = 8;
    ret = ioctl(mrs->sfm[0], SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (ret == -1) {
        sprintf(mrs->log, "spi0 can't set bits per word"); 
        print_f(&mrs->plog, "fs50", mrs->log);
    }
    ret = ioctl(mrs->sfm[0], SPI_IOC_RD_BITS_PER_WORD, &bits); 
    if (ret == -1) {
        sprintf(mrs->log, "spi0 can't get bits per word"); 
        print_f(&mrs->plog, "fs50", mrs->log);
    }

    ring_buf_init(&mrs->dataTx);
    mrs_ipc_put(mrs, "s", 1, 1);

    modersp->m = modersp->m + 1;
    modersp->v = 0;

    return 0;
}

static int fs51(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;

    //sprintf(mrs->log, "cnt: %d\n", modersp->v);
    //print_f(&mrs->plog, "fs52", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 1);
    while (len) {
        if (len > 0) {
            modersp->v += 1;

            //sprintf(mrs->log, "cnt: %d ch:%c\n", modersp->v, ch);
            //print_f(&mrs->plog, "fs51", mrs->log);

            if (ch == 's') {
                mrs_ipc_put(mrs, "i", 1, 3);
            } else if (ch == 'S') {
                mrs_ipc_put(mrs, "I", 1, 3);
                modersp->m = modersp->m + 1;
                return 2;
            }
        }
        len = mrs_ipc_get(mrs, &ch, 1, 1);
    }
    return 0; 
}

static int fs52(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;

    //sprintf(mrs->log, "spi send done \n");
    //print_f(&mrs->plog, "fs52", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'I')){
        modersp->m = 20;
        modersp->d = 0;
        return 2;
    }

    return 0; 
}

static int fs53(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    struct info16Bit_s *p;
    p = &mrs->mchine.cur;

    sprintf(mrs->log, "set 0x%.2x 0x%.2x \n", p->opcode, p->data);
    print_f(&mrs->plog, "fs53", mrs->log);

    mrs->mchine.seqcnt += 1;
    if (mrs->mchine.seqcnt >= 0x8) {
        mrs->mchine.seqcnt = 0;
    }

    //p->opcode = OP_DOUBLE;
    //p->data = DOUSCAN_WIFI_ONLY;
    
    mrs_ipc_put(mrs, "c", 1, 3);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs54(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;
    struct info16Bit_s *p;

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'C')) {
        msync(&mrs->mchine, sizeof(struct machineCtrl_s), MS_SYNC);

        p = &mrs->mchine.get;

        sprintf(mrs->log, "get 0x%.2x 0x%.2x \n", p->opcode, p->data);
        print_f(&mrs->plog, "fs54", mrs->log);

        if (((p->opcode == OP_DOUBLE) || (p->opcode == OP_MDOUBLE)) && 
            ((p->data == DOUSCAN_WIFI_ONLY) || (p->data == DOUSCAN_WIFI_SD))) {
            modersp->r = 1;
            return 1;
        } else {
            modersp->m = modersp->m - 1;        
            modersp->r = 2;
            return 2;
        }
    }
    return 0; 
}

static int fs55(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int bitset, ret;
    sprintf(mrs->log, "read patern file\n");
    print_f(&mrs->plog, "fs55", mrs->log);
    
    bitset = 0;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
    sprintf(mrs->log, "spi0 Set data mode: %d\n", bitset);
    print_f(&mrs->plog, "fs55", mrs->log);

    bitset = 1;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
    sprintf(mrs->log, "Set spi 0 slave ready: %d\n", bitset);
    print_f(&mrs->plog, "fs55", mrs->log);

    int bits = 8;
    ret = ioctl(mrs->sfm[0], SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (ret == -1) {
        sprintf(mrs->log, "spi0 can't set bits per word"); 
        print_f(&mrs->plog, "fs55", mrs->log);
    }
    ret = ioctl(mrs->sfm[0], SPI_IOC_RD_BITS_PER_WORD, &bits); 
    if (ret == -1) {
        sprintf(mrs->log, "spi0 can't get bits per word"); 
        print_f(&mrs->plog, "fs55", mrs->log);
    }

    bitset = 0;
    ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
    sprintf(mrs->log, "spi1 Set data mode: %d\n", bitset);
    print_f(&mrs->plog, "fs55", mrs->log);
    
    bitset = 1;
    ioctl(mrs->sfm[1], _IOW(SPI_IOC_MAGIC, 11, __u32), &bitset);   //SPI_IOC_WR_SLVE_READY
    sprintf(mrs->log, "Set spi 1 slave ready: %d\n", bitset);
    print_f(&mrs->plog, "fs55", mrs->log);

    bits = 8;
    ret = ioctl(mrs->sfm[1], SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (ret == -1) {
        sprintf(mrs->log, "spi1 can't set bits per word"); 
        print_f(&mrs->plog, "fs55", mrs->log);
    }
    ret = ioctl(mrs->sfm[1], SPI_IOC_RD_BITS_PER_WORD, &bits); 
    if (ret == -1) {
        sprintf(mrs->log, "spi1 can't get bits per word"); 
        print_f(&mrs->plog, "fs55", mrs->log);
    }


    ring_buf_init(&mrs->dataTx);
    ring_buf_init(&mrs->cmdTx);
    mrs_ipc_put(mrs, "d", 1, 1);

    modersp->m = modersp->m + 1;
    modersp->v = 0;

    return 0;
}

static int fs56(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;

    sprintf(mrs->log, "v: 0x%.2x\n", modersp->v);
    print_f(&mrs->plog, "fs56", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 1);
    while (len > 0) {
        //if (len > 0) {
            //sprintf(mrs->log, "cnt: %d ch:%c\n", modersp->v, ch);
            //print_f(&mrs->plog, "fs56", mrs->log);

            if (ch == 's') {
                mrs_ipc_put(mrs, "i", 1, 3);
            } else if (ch == 'd') {
                mrs_ipc_put(mrs, "i", 1, 4);
            } else if (ch == 'S') {
                modersp->v |= 0x01;
                mrs_ipc_put(mrs, "I", 1, 3);
            } else if (ch == 'D') {
                modersp->v |= 0x10;
                mrs_ipc_put(mrs, "I", 1, 4);
            } else {
                sprintf(mrs->log, "WARNING!! unknown ch:%c\n", ch);
                print_f(&mrs->plog, "fs56", mrs->log);
            }            
        //}
        len = mrs_ipc_get(mrs, &ch, 1, 1);
    }

    if (modersp->v & 0x11) {
        modersp->m = modersp->m + 1;
        modersp->v = 0;
        sprintf(mrs->log, "go to next fs, v: 0x%.2x\n", modersp->v);
        print_f(&mrs->plog, "fs56", mrs->log);

        return 2;
    }

    return 0; 
    
}
static int fs57(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch0=0, ch1=0;

    //sprintf(mrs->log, "spi send done \n");
    //print_f(&mrs->plog, "fs57", mrs->log);

    len=0;
    len = mrs_ipc_get(mrs, &ch0, 1, 3);
    if ((len > 0) && (ch0 == 'I')){
        modersp->v |= 0x01;
    }

    len=0;
    len = mrs_ipc_get(mrs, &ch1, 1, 4);
    if ((len > 0) && (ch1 == 'I')){
        modersp->v |= 0x10;
    }

    if (modersp->v == 0x11) {
        modersp->m = 20;
        modersp->d = 0;

        sprintf(mrs->log, "go to next fs, v: 0x%.2x\n", modersp->v);
        print_f(&mrs->plog, "fs57", mrs->log);
        
        modersp->v = 0;
        return 2;
    }
    
    return 0; 
}

static int fs58(struct mainRes_s *mrs, struct modersp_s *modersp) 
{
    int ret = -1;
    struct info16Bit_s *p, *c;
    //char diskname[128] = "/mnt/mmc2/disk_rmImg.bin";
    //char diskname[128] = "/mnt/mmc2/disk_rx127_255log.bin";
    //char diskname[128] = "/mnt/mmc2/disk_golden.bin";
    //char diskname[128] = "/mnt/mmc2/debug_fat.bin";
    //char diskname[128] = "/mnt/mmc2/disk_05.bin";
    //char diskname[128] = "/dev/mmcblk0p1";
    //char diskname[128] = "/dev/mmcblk0";
    //char diskname[128] = "/mnt/mmc2/empty_256.dsk";
    //char diskname[128] = "/mnt/mmc2/folder_256.dsk";
    //char diskname[128] = "/mnt/mmc2/disk_LFN_64.bin";
    //char diskname[128] = "/mnt/mmc2/disk_onefolder.bin";
    //char diskname[128] = "/mnt/mmc2/mingssd.bin";
    //char diskname[128] = "/mnt/mmc2/16g_ghost.bin";
    //char diskname[128] = "/mnt/mmc2/32g_beformat_rmmore.bin";
    //char diskname[128] = "/mnt/mmc2/32g_32k_format.bin";
    //char diskname[128] = "/mnt/mmc2/32g_64k_format.bin";
    //char diskname[128] = "/mnt/mmc2/32g_64k_format_add_01_rm.bin";
    //char diskname[128] = "/mnt/mmc2/32g_64k_format_add_01.bin";
    //char diskname[128] = "/mnt/mmc2/64G_2pics.bin";
    //char diskname[128] = "/mnt/mmc2/64G_empty.bin";
    //char diskname[128] = "/mnt/mmc2/32g_ios_format.bin";
    //char diskname[128] = "/mnt/mmc2/128g_ios_format.bin";
    //char diskname[128] = "/mnt/mmc2/32g_and.bin";
    //char diskname[128] = "/mnt/mmc2/32g_win.bin";
    char diskname[128] = "/mnt/mmc2/32g_mass.bin";
    //char diskname[128] = "/mnt/mmc2/phy_32g_empty_diff.bin";
    //char diskname[128] = "/mnt/mmc2/phy_folder.bin";
    struct DiskFile_s *fd;
    FILE *fp=0;

    fd = &mrs->mchine.fdsk;
    p = &mrs->mchine.get;
    c = &mrs->mchine.cur;

    fp = fopen(diskname, "w+");
    if (fp == NULL) {
        sprintf(mrs->log, "save disk file [%s] open failed!!! \n", diskname);
        print_f(&mrs->plog, "fs58", mrs->log);
        modersp->r = 2;
        return 1;
    }

    ret = fseek(fp, 0, SEEK_SET);
    if (ret) {
        sprintf(mrs->log, "seek file [%s] failed!!! \n", diskname);
        print_f(&mrs->plog, "fs58", mrs->log);
        modersp->r = 2;
        return 1;
    }

    sprintf(mrs->log, "start to write disk file [%s], op:0x%x\n", diskname, p->opcode);
    print_f(&mrs->plog, "fs58", mrs->log);

    if (!fd->sdt) {
        sprintf(mrs->log, "ERROR!!! memory is not existed, opcodet: 0x%.2x/0x%.2x !!!\n", p->opcode, p->data);  
        print_f(&mrs->plog, "fs58", mrs->log);
        modersp->r = 2;
        return 1;
    } else {
        msync(fd->sdt, fd->rtMax, MS_SYNC);

        ret = fwrite(fd->sdt, 1, fd->rtMax, fp);

        sprintf(mrs->log, "write file size: %d/%d \n", ret, fd->rtMax);
        print_f(&mrs->plog, "fs58", mrs->log);
        fflush(fd->vsd);
        fsync((int)fd->vsd);        
    }

    fclose(fp);
    
    sync();
    
    modersp->d = 0;    
    modersp->m = 24;
    return 2;
}
static int fs59(struct mainRes_s *mrs, struct modersp_s *modersp) 
{
#define SAMPLE_MAX 16
    int ret = -1, idx = 0, fileLen = 0;
    struct info16Bit_s *p, *c;
    char samplefile[128] = "/mnt/mmc2/sample/greenhill_%.2d.jpg";
    char sampledst[128];
    struct DiskFile_s *fd;
    uint32_t strSec, secLen, strflag, lenflag;
    struct SdAddrs_s *psstr, *pslen;
    FILE *fp = 0;
    char *saveDst=0;

    
    psstr = &mrs->mchine.sdst;
    pslen = &mrs->mchine.sdln;

    fd = &mrs->mchine.fdsk;
    p = &mrs->mchine.get;
    c = &mrs->mchine.cur;

    idx = psstr->sdc % SAMPLE_MAX;
    psstr->sdc++;

    sprintf(sampledst, samplefile, idx);
    fp = fopen(sampledst, "r");
    if (!fp) {
        sprintf(mrs->log, "ERROR!!!file read [%s] failed \n", sampledst);
        print_f(&mrs->plog, "fs59", mrs->log);
        modersp->r = 2;
        return 1;        
    } else {
        sprintf(mrs->log, "file read [%s] ok \n", sampledst);
        print_f(&mrs->plog, "fs59", mrs->log);
    }
    
    ret = fseek(fp, 0, SEEK_END);
    if (ret) {
        sprintf(mrs->log, " file seek failed!! ret:%d \n", ret);
        print_f(&mrs->plog, "fs59", mrs->log);
        modersp->r = 2;
        return 1;        
    } 

    fileLen = ftell(fp);
    sprintf(mrs->log, " file [%s] size: %d \n", sampledst, fileLen);
    print_f(&mrs->plog, "fs59", mrs->log);

    ret = fseek(fp, 0, SEEK_SET);
    if (ret) {
        sprintf(mrs->log, " file seek failed!! ret:%d \n", ret);
        print_f(&mrs->plog, "fs59", mrs->log);
        modersp->r = 2;
        return 1;        
    }

    strSec = psstr->sda;
    secLen = fileLen/SEC_LEN  + (((fileLen%SEC_LEN) == 0) ?0:1); // should be length of file

    sprintf(mrs->log, "start to write [%s] to SD strSec: %d, secLen: %d, size: %d  \n", sampledst, strSec, secLen, fileLen);
    print_f(&mrs->plog, "fs59", mrs->log);

    if (!fd->sdt) {
        sprintf(mrs->log, "ERROR!!! memory is not existed");  
        print_f(&mrs->plog, "fs59", mrs->log);
        modersp->r = 2;
        return 1;
    } 
    
    msync(fd->sdt, fd->rtMax, MS_SYNC);

    saveDst = fd->sdt + (strSec * SEC_LEN);
    ret = fread(saveDst, 1, fileLen, fp);
    if (ret != fileLen) {
        sprintf(mrs->log, "WARNING!!! read file size:%d/%d", ret, fileLen);  
        print_f(&mrs->plog, "fs59", mrs->log);
    }

    psstr->n = strSec;
    pslen->n = secLen;

    psstr->f = 0xf;
    pslen->f = 0xf;
    pslen->f |= 0x200;

    modersp->m = 24;
    return 0;
}
#if 1
#elif 0
#define CROP_COOD_01 {818, 557}
#define CROP_COOD_02 {980, 1557}
#define CROP_COOD_03 {1168, 1557}
#define CROP_COOD_04 {1586, 1484}
#define CROP_COOD_05 {1415, 487}
#define CROP_COOD_06 {1253, 487}
#elif 0
#define CROP_COOD_01 {123, 15 }
#define CROP_COOD_02 {52 , 83 }
#define CROP_COOD_03 {121, 149}
#define CROP_COOD_04 {145, 149}
#define CROP_COOD_05 {213, 83 }
#define CROP_COOD_06 {143, 15 }
#elif 0 /* 00 */
#define CROP_COOD_01 {158, 2808 }
#define CROP_COOD_02 {2453 , 3672 }
#define CROP_COOD_03 {2487,  3672 }
#define CROP_COOD_04 {3250,  472}
#define CROP_COOD_05 {841, 48 }
#define CROP_COOD_06 {758, 48 }
#elif 0 /* 01 */
#define CROP_COOD_01 {309, 712 }
#define CROP_COOD_02 {949 , 3368 }
#define CROP_COOD_03 {988,  3368 }
#define CROP_COOD_04 {3392,  2992}
#define CROP_COOD_05 {2657, 32 }
#define CROP_COOD_06 {2559, 32 }
#elif 0 /* 02 */
#define CROP_COOD_01 {666, 1184 }
#define CROP_COOD_02 {1567, 4016 }
#define CROP_COOD_03 {1604, 4016 }
#define CROP_COOD_04 {4000, 3048 }
#define CROP_COOD_05 {2945, 32 }
#define CROP_COOD_06 {2820, 32 }
#elif 0 /* 03 */
#define CROP_COOD_01 {252, 608 }
#define CROP_COOD_02 {572, 3288 }
#define CROP_COOD_03 {634, 3288 }
#define CROP_COOD_04 {3072, 3104 }
#define CROP_COOD_05 {2641, 32 }
#define CROP_COOD_06 {2578, 32 }
#elif 0 /* 04 */
#define CROP_COOD_01 {1721, 887 }
#define CROP_COOD_02 {2376, 1874 }
#define CROP_COOD_03 {2514, 1874 }
#define CROP_COOD_04 {3169, 887 }
#define CROP_COOD_05 {2609, 45 }
#define CROP_COOD_06 {2281, 45 }
#elif 0 /* 05 */
#define CROP_COOD_01 {1321, 887 }
#define CROP_COOD_02 {2376, 1874 }
#define CROP_COOD_03 {2514, 1874 }
#define CROP_COOD_04 {3269, 1017 }
#define CROP_COOD_05 {2609, 325 }
#define CROP_COOD_06 {2281, 325 }
#elif 0 /* 06 */
#define CROP_COOD_01 {358, 357}
#define CROP_COOD_02 {630, 640}
#define CROP_COOD_03 {678, 640}
#define CROP_COOD_04 {951, 381}
#define CROP_COOD_05 {685, 104}
#define CROP_COOD_06 {624, 104}
#else
#define CROP_COOD_01 {0, 0}
#define CROP_COOD_02 {0, 0}
#define CROP_COOD_03 {0, 0}
#define CROP_COOD_04 {0, 0}
#define CROP_COOD_05 {0, 0}
#define CROP_COOD_06 {0, 0}
#endif

#define CROP_SCALE 1
static int fs60(struct mainRes_s *mrs, struct modersp_s *modersp)  
{
    //int axy[2] = CROP_COOD_01;
    int *axy = mrs->cropCoord.CROP_COOD_01;
    int id=0;
    uint32_t tmp32=0;
    uint16_t  x=0, y=0;
    struct SdAddrs_s *psstr, *pslen;
    
    pslen = &mrs->mchine.sdln;

    x = axy[0] * CROP_SCALE;
    y = axy[1] * CROP_SCALE;;

    tmp32 = (x << 16) | y;

    pslen->n = 0;
    for (id = 0; id < 4; id++) {
        pslen->d[id] = (tmp32 >> (8 * (3 - id))) & 0xff;
    }

    sprintf(mrs->log, "x, y = (%d, %d)[0x%.8x]\n", x, y, pslen->n);
    print_f(&mrs->plog, "fs60", mrs->log);

    pslen->f = 0xf;
    pslen->f |= 0x200;
    
    modersp->m = 24;
    //modersp->r = 1;
    return 0;
}

static int fs61(struct mainRes_s *mrs, struct modersp_s *modersp)  
{
    //int axy[2] = CROP_COOD_02;
    int *axy = mrs->cropCoord.CROP_COOD_02;
    int id=0;
    uint32_t tmp32=0;

    uint16_t  x=0, y=0;
    struct SdAddrs_s *psstr, *pslen;
    
    pslen = &mrs->mchine.sdln;

    x = axy[0] * CROP_SCALE;
    y = axy[1] * CROP_SCALE;;
    
    tmp32 = (x << 16) | y;

    pslen->n = 0;
    for (id = 0; id < 4; id++) {
        pslen->d[id] = (tmp32 >> (8 * (3 - id))) & 0xff;
    }

    sprintf(mrs->log, "x, y = (%d, %d)[0x%.8x]\n", x, y, pslen->n);
    print_f(&mrs->plog, "fs61", mrs->log);

    pslen->f = 0xf;
    pslen->f |= 0x200;
    
   modersp->m = 24;
   // modersp->r = 1;
    return 0;
}

static int fs62(struct mainRes_s *mrs, struct modersp_s *modersp)  
{
    //int axy[2] = CROP_COOD_03;
    int *axy = mrs->cropCoord.CROP_COOD_03;
    int id=0;
    uint32_t tmp32=0;

    uint16_t  x=0, y=0;
    struct SdAddrs_s *psstr, *pslen;
    
    pslen = &mrs->mchine.sdln;

    x = axy[0] * CROP_SCALE;
    y = axy[1] * CROP_SCALE;;

    tmp32 = (x << 16) | y;

    pslen->n = 0;
    for (id = 0; id < 4; id++) {
        pslen->d[id] = (tmp32 >> (8 * (3 - id))) & 0xff;
    }

    sprintf(mrs->log, "x, y = (%d, %d)[0x%.8x]\n", x, y, pslen->n);
    print_f(&mrs->plog, "fs62", mrs->log);

    pslen->f = 0xf;
    pslen->f |= 0x200;
    
    modersp->m = 24;
    //modersp->r = 1;
    return 0;
}

static int fs63(struct mainRes_s *mrs, struct modersp_s *modersp)  
{
    //int axy[2] = CROP_COOD_04;
    int *axy = mrs->cropCoord.CROP_COOD_04;
    int id=0;
    uint32_t tmp32=0;

    uint16_t  x=0, y=0;
    struct SdAddrs_s *psstr, *pslen;
    
    pslen = &mrs->mchine.sdln;

    x = axy[0] * CROP_SCALE;
    y = axy[1] * CROP_SCALE;;

    tmp32 = (x << 16) | y;

    pslen->n = 0;
    for (id = 0; id < 4; id++) {
        pslen->d[id] = (tmp32 >> (8 * (3 - id))) & 0xff;
    }

    sprintf(mrs->log, "x, y = (%d, %d)[0x%.8x]\n", x, y, pslen->n);
    print_f(&mrs->plog, "fs63", mrs->log);

    pslen->f = 0xf;
    pslen->f |= 0x200;
    
    modersp->m = 24;
    //modersp->r = 1;
    return 0;
}

static int fs64(struct mainRes_s *mrs, struct modersp_s *modersp)  
{
    //int axy[2] = CROP_COOD_05;
    int *axy = mrs->cropCoord.CROP_COOD_05;
    int id=0;
    uint32_t tmp32=0;

    uint16_t  x=0, y=0;
    struct SdAddrs_s *psstr, *pslen;
    
    pslen = &mrs->mchine.sdln;

    x = axy[0] * CROP_SCALE;
    y = axy[1] * CROP_SCALE;;

    tmp32 = (x << 16) | y;

    pslen->n = 0;
    for (id = 0; id < 4; id++) {
        pslen->d[id] = (tmp32 >> (8 * (3 - id))) & 0xff;
    }

    sprintf(mrs->log, "x, y = (%d, %d)[0x%.8x]\n", x, y, pslen->n);
    print_f(&mrs->plog, "fs64", mrs->log);

    pslen->f = 0xf;
    pslen->f |= 0x200;
    
    modersp->m = 24;
    //modersp->r = 1;
    return 0;
}

static int fs65(struct mainRes_s *mrs, struct modersp_s *modersp)  
{
    //int axy[2] = CROP_COOD_06;
    int *axy = mrs->cropCoord.CROP_COOD_06;
    int id=0;
    uint32_t tmp32=0;

    uint16_t  x=0, y=0;
    struct SdAddrs_s *psstr, *pslen;
    
    pslen = &mrs->mchine.sdln;

    x = axy[0] * CROP_SCALE;
    y = axy[1] * CROP_SCALE;;

    tmp32 = (x << 16) | y;

    pslen->n = 0;
    for (id = 0; id < 4; id++) {
        pslen->d[id] = (tmp32 >> (8 * (3 - id))) & 0xff;
    }

    sprintf(mrs->log, "x, y = (%d, %d)[0x%.8x]\n", x, y, pslen->n);
    print_f(&mrs->plog, "fs65", mrs->log);

    pslen->f = 0xf;
    pslen->f |= 0x200;
    
    modersp->m = 24;
    //modersp->r = 1;
    return 0;
}

static int fs66(struct mainRes_s *mrs, struct modersp_s *modersp)  
{
    int id=0;
    uint32_t tmp32=0;

    struct SdAddrs_s *psstr, *pslen;
    
    pslen = &mrs->mchine.sdln;

    //tmp32 = 16843832; //0x010107a8 //0x01010438
    //tmp32 = mrs->scan_length;
    tmp32 = 0;
    
    pslen->n = 0;
    for (id = 0; id < 4; id++) {
        pslen->d[id] = (tmp32 >> (8 * (3 - id))) & 0xff;
    }

    sprintf(mrs->log, "scan length = (%d)[0x%.8x]\n", tmp32, pslen->n);
    print_f(&mrs->plog, "fs66", mrs->log);

    pslen->f = 0xf;
    pslen->f |= 0x200;
    
    modersp->m = 24;
    //modersp->r = 1;
    return 0;
}

static int fs67(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    sprintf(mrs->log, "trigger metaout transfer \n");
    print_f(&mrs->plog, "fs67", mrs->log);

    mrs_ipc_put(mrs, "j", 1, 3);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs68(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'J')) {
        msync(&mrs->metaout, sizeof(struct aspMetaData_s), MS_SYNC);

        sprintf(mrs->log, "get ch = %c\n", ch);
        print_f(&mrs->plog, "fs68", mrs->log);

        modersp->m = 30;

        return 2;
    }
    return 0; 
}

static int fs69(struct mainRes_s *mrs, struct modersp_s *modersp)  
{
    sprintf(mrs->log, "trigger metaout transfer \n");
    print_f(&mrs->plog, "fs69", mrs->log);

    mrs_ipc_put(mrs, "m", 1, 3);
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs70(struct mainRes_s *mrs, struct modersp_s *modersp)  
{
    int len=0;
    char ch=0;

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'M')) {
        msync(&mrs->metaout, sizeof(struct aspMetaData_s), MS_SYNC);

        sprintf(mrs->log, "get ch = %c\n", ch);
        print_f(&mrs->plog, "fs70", mrs->log);

        modersp->m = 30;

        return 2;
    }
    return 0; 
}

static int fs71(struct mainRes_s *mrs, struct modersp_s *modersp)
{
    int bitset, ret;
    sprintf(mrs->log, "read patern file\n");
    print_f(&mrs->plog, "fs71", mrs->log);
    
    bitset = 0;
    ioctl(mrs->sfm[0], _IOW(SPI_IOC_MAGIC, 8, __u32), &bitset);   //SPI_IOC_WR_DATA_MODE
    sprintf(mrs->log, "spi0 Set data mode: %d\n", bitset);
    print_f(&mrs->plog, "fs71", mrs->log);

    int bits = 8;
    ret = ioctl(mrs->sfm[0], SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (ret == -1) {
        sprintf(mrs->log, "spi0 can't set bits per word"); 
        print_f(&mrs->plog, "fs71", mrs->log);
    }
    ret = ioctl(mrs->sfm[0], SPI_IOC_RD_BITS_PER_WORD, &bits); 
    if (ret == -1) {
        sprintf(mrs->log, "spi0 can't get bits per word"); 
        print_f(&mrs->plog, "fs71", mrs->log);
    }

    ring_buf_init(&mrs->dataTx);
    mrs_ipc_put(mrs, "b", 1, 1);

    modersp->m = modersp->m + 1;
    modersp->v = 0;

    return 0;
}

static int fs72(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;

    //sprintf(mrs->log, "cnt: %d\n", modersp->v);
    //print_f(&mrs->plog, "fs72", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 1);
    while (len) {
        if (len > 0) {
            modersp->v += 1;

            //sprintf(mrs->log, "cnt: %d ch:%c\n", modersp->v, ch);
            //print_f(&mrs->plog, "fs72", mrs->log);

            if (ch == 'b') {
                mrs_ipc_put(mrs, "i", 1, 3);
            } else if (ch == 'B') {
                mrs_ipc_put(mrs, "I", 1, 3);
                modersp->m = modersp->m + 1;
                return 2;
            }
        }
        len = mrs_ipc_get(mrs, &ch, 1, 1);
    }
    return 0; 
}

static int fs73(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;

    //sprintf(mrs->log, "spi send done \n");
    //print_f(&mrs->plog, "fs73", mrs->log);

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'I')){
        modersp->m = 30;
        modersp->d = 0;
        return 2;
    }

    return 0; 
}

static int fs74(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    sprintf(mrs->log, "trigger metaout transfer \n");
    print_f(&mrs->plog, "fs74", mrs->log);

    mrs_ipc_put(mrs, "j", 1, 3);
    mrs_ipc_put(mrs, "j", 1, 4);
    modersp->v = 0;
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs75(struct mainRes_s *mrs, struct modersp_s *modersp)
{ 
    int len=0;
    char ch=0;

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'J')) {
        msync(&mrs->metaout, sizeof(struct aspMetaData_s), MS_SYNC);

        sprintf(mrs->log, "get ch = %c from p4\n", ch);
        print_f(&mrs->plog, "fs75", mrs->log);

        modersp->v |= 0x01;
    }

    len = mrs_ipc_get(mrs, &ch, 1, 4);
    if ((len > 0) && (ch == 'J')) {
        msync(&mrs->metaoutDuo, sizeof(struct aspMetaData_s), MS_SYNC);

        sprintf(mrs->log, "get ch = %c from p5\n", ch);
        print_f(&mrs->plog, "fs75", mrs->log);

        modersp->v |= 0x10;
    }

    if (modersp->v == 0x11) {
        modersp->m = 30;
        return 2;
    }

    return 0; 
}

static int fs76(struct mainRes_s *mrs, struct modersp_s *modersp)  
{
    sprintf(mrs->log, "trigger metaout transfer \n");
    print_f(&mrs->plog, "fs76", mrs->log);

    mrs_ipc_put(mrs, "m", 1, 3);
    mrs_ipc_put(mrs, "m", 1, 4);
    modersp->v = 0;
    modersp->m = modersp->m + 1;
    return 0; 
}

static int fs77(struct mainRes_s *mrs, struct modersp_s *modersp)  
{
    int len=0;
    char ch=0;

    len = mrs_ipc_get(mrs, &ch, 1, 3);
    if ((len > 0) && (ch == 'M')) {
        msync(&mrs->metaout, sizeof(struct aspMetaData_s), MS_SYNC);

        sprintf(mrs->log, "get ch = %c from p4\n", ch);
        print_f(&mrs->plog, "fs77", mrs->log);

        modersp->v |= 0x01;
    }

    len = mrs_ipc_get(mrs, &ch, 1, 4);
    if ((len > 0) && (ch == 'M')) {
        msync(&mrs->metaout, sizeof(struct aspMetaData_s), MS_SYNC);

        sprintf(mrs->log, "get ch = %c from p5\n", ch);
        print_f(&mrs->plog, "fs77", mrs->log);

        modersp->v |= 0x10;
    }

    if (modersp->v == 0x11) {
        modersp->m = 30;
        return 2;
    }

    return 0; 
}

static int fs78(struct mainRes_s *mrs, struct modersp_s *modersp)  
{
    modersp->r = 1;
    return 1;
}
static int fs79(struct mainRes_s *mrs, struct modersp_s *modersp)  
{
    modersp->r = 1;
    return 1;
}
static int p0(struct mainRes_s *mrs)
{
#define PS_NUM 80
    int len, tmp, ret;
    char str[128], ch;

    struct modersp_s modesw;
    struct fselec_s afselec[PS_NUM] = {{ 0, fs00},{ 1, fs01},{ 2, fs02},{ 3, fs03},{ 4, fs04},
                                 { 5, fs05},{ 6, fs06},{ 7, fs07},{ 8, fs08},{ 9, fs09},
                                 {10, fs10},{11, fs11},{12, fs12},{13, fs13},{14, fs14},
                                 {15, fs15},{16, fs16},{17, fs17},{18, fs18},{19, fs19},
                                 {20, fs20},{21, fs21},{22, fs22},{23, fs23},{24, fs24},
                                 {25, fs25},{26, fs26},{27, fs27},{28, fs28},{29, fs29},
                                 {30, fs30},{31, fs31},{32, fs32},{33, fs33},{34, fs34},
                                 {35, fs35},{36, fs36},{37, fs37},{38, fs38},{39, fs39},
                                 {40, fs40},{41, fs41},{42, fs42},{43, fs43},{44, fs44},
                                 {45, fs45},{46, fs46},{47, fs47},{48, fs48},{49, fs49},
                                 {50, fs50},{51, fs51},{52, fs52},{53, fs53},{54, fs54},
                                 {55, fs55},{56, fs56},{57, fs57},{58, fs58},{59, fs59},
                                 {60, fs60},{61, fs61},{62, fs62},{63, fs63},{64, fs64},
                                 {65, fs65},{66, fs66},{67, fs67},{68, fs68},{69, fs69},
                                 {70, fs70},{71, fs71},{72, fs72},{73, fs73},{74, fs74},
                                 {75, fs75},{76, fs76},{77, fs77},{78, fs78},{79, fs79}};

    p0_init(mrs);

    // [todo]initial value
    modesw.m = -1;
    modesw.r = 0;
    modesw.d = 0;
    while (1) {
        //sprintf(mrs->log, ".");
        //print_f(&mrs->plog, "P0", mrs->log);

        // p2 data mode spi0 rx
        len = mrs_ipc_get(mrs, &ch, 1, 0);
        if (len > 0) {
            //sprintf(mrs->log, "ret:%d ch:%c\n", ret, ch);
            //print_f(&mrs->plog, "P0", mrs->log);

            if (modesw.m == -1) {
                if ((ch > 0) && (ch < PS_NUM)) {
                    modesw.m = ch;
                }
            } else {
                /* todo: interrupt state machine here */
            }
        }

        if ((modesw.m > 0) && (modesw.m < PS_NUM)) {
            ret = (*afselec[modesw.m].pfunc)(mrs, &modesw);
            //sprintf(mrs->log, "pmode:%d rsp:%d\n", modesw.m, modesw.r);
            //print_f(&mrs->plog, "P0", mrs->log);
            if (ret == 1) {
                tmp = modesw.m;
                modesw.m = 0;
            }
        }

        if (modesw.m == 0) {
            sprintf(mrs->log, "pmode:%d rsp:%d - end\n", tmp, modesw.r);
            print_f(&mrs->plog, "P0", mrs->log);

            ch = modesw.r; /* response */
            modesw.m = -1;
            modesw.r = 0;
            modesw.d = 0;

            mrs_ipc_put(mrs, &ch, 1, 0);
        } else {
            mrs_ipc_put(mrs, "$", 1, 0);
        }

        usleep(100);
        //sleep(2);
    }

    // save to file for debug
    //if (pmode == 1) {
    //   msync(mrs->dataRx.pp[0], 1024*SPI_TRUNK_SZ, MS_SYNC);
    //    ret = fwrite(mrs->dataRx.pp[0], 1, 1024*SPI_TRUNK_SZ, mrs->fs);
    //    printf("\np0 write file %d size %d/%d \n", mrs->fs, 1024*SPI_TRUNK_SZ, ret);
    //}

    p0_end(mrs);
    return 0;
}

static int p1(struct procRes_s *rs, struct procRes_s *rcmd)
{
    int px, pi, ret = 0, ci;
    char ch, cmd, cmdt;
    char *addr;
    uint32_t evt;

    sprintf(rs->logs, "p1\n");
    print_f(rs->plogs, "P1", rs->logs);
    struct psdata_s stdata;
    stfunc pf[SMAX][PSMAX] = {{stspy_01, stspy_02, stspy_03, stspy_04, stspy_05}, // SPY
                            {stbullet_01, stbullet_02, stbullet_03, stbullet_04, stbullet_05}, // BULLET
                            {stlaser_01, stlaser_02, stlaser_03, stlaser_04, stlaser_05},  // LASER
                            {stauto_01, stauto_02, stauto_03, stauto_04, stauto_05}, // AUTO_A
                            {stauto_06, stauto_07, stauto_08, stauto_09, stauto_10}, // AUTO_B
                            {stauto_11, stauto_12, stauto_13, stauto_14, stauto_15}, // AUTO_C
                            {stauto_16, stauto_17, stauto_18, stauto_19, stauto_20}, // AUTO_D
                            {stauto_21, stauto_22, stauto_23, stauto_24, stauto_25}, // AUTO_E
                            {stauto_26, stauto_27, stauto_28, stauto_29, stauto_30}, // AUTO_F
                            {stauto_31, stauto_32, stauto_33, stauto_34, stauto_35}, // AUTO_G
                            {stauto_36, stauto_37, stauto_38, stauto_39, stauto_40}, // AUTO_H
                            {stauto_41, stauto_42, stauto_43, stauto_44, stauto_45}}; // AUTO_I

    /* A.1 ~ A.4 for start sector 01 - 04*/
    /* A.5 = A.8 for sector len   01 - 04*/
    /* A.9 for sd sector transmitting using command mode */
    p1_init(rs);
    // wait for ch from p0
    // state machine control
    stdata.rs = rs;
    pi = 0;    stdata.result = 0;    cmd = '\0';   cmdt = '\0';
    while (1) {
        //sprintf(rs->logs, "+\n");
        //print_f(rs->plogs, "P1", rs->logs);

        ci = 0; 
        ci = rs_ipc_get(rcmd, &cmd, 1);
        while (ci > 0) {
            sprintf(rs->logs, "%c\n", cmd);
            print_f(rs->plogs, "P1CMD", rs->logs);

            if (cmdt == '\0') {
                if (cmd == 's') {
                    stdata.result = 0x0;
                    cmdt = cmd;
                } else if (cmd == 'b') {
                    stdata.result = emb_stanPro(0, STINIT, BULLET, PSSET);
                    cmdt = cmd;
                } else if (cmd == 'p') {
                    stdata.result = emb_stanPro(0, STINIT, SPY, PSSET);
                    cmdt = cmd;
                }
            } else { /* command to interrupt state machine here */

            }
            ci = 0;
            ci = rs_ipc_get(rcmd, &cmd, 1);            
        }

        ret = 0; ch = '\0';
        ret = rs_ipc_get(rs, &ch, 1);
        //if (ret > 0) {
        //    sprintf(rs->logs, "ret:%d ch:%c\n", ret, ch);
        //    print_f(rs->plogs, "P1", rs->logs);
        //}

        if (((ret > 0) && (ch != '$')) ||
            (cmdt != '\0')) {
            stdata.ansp0 = ch;
            evt = stdata.result;
            pi = (evt >> 8) & 0xff;
            px = (evt & 0xff);
            if ((pi >= SMAX) || (px >= PSMAX)) {
                cmdt = '\0';
                continue;
            }

            stdata.result = (*pf[pi][px])(&stdata);

            //sprintf(rs->logs, "ret:%d ch:%d evt:0x%.4x\n", ret, ch, evt);
            //print_f(rs->plogs, "P1", rs->logs);
        }

        //rs_ipc_put(rs, &ch, 1);
        //usleep(60000);
    }

    p1_end(rs);
    return 0;
}

inline int randomGen(int min, int max)
{
    uint32_t seed[16] = {1, 3, 5, 11, 13, 17, 23, 29, 31, 37, 41, 47, 53, 57, 61, 67};
    struct timespec ctm;
    uint32_t ns=0;
    int range;
    int r;
    int v;

    range = max - min;
    if (range < 0) range = 0;

    clock_gettime(CLOCK_REALTIME, &ctm);
    
    //printf("randomGen() - %d ~ %d\n", min, max);
    ns = ctm.tv_nsec;
    srandom(seed[ns%16]);
    r = random();

    v = min + (r % range);
    //printf("clock() - %d - v:%d r:%d\n", ctm.tv_nsec, v, r);
    
    return v;
}

static int randCrop(int *p, int minX, int maxX, int minY, int maxY)
{
    int x=0, y=0;
    if (!p) return -1;

    x = randomGen(minX, maxX);
    y = randomGen(minY, maxY);

    printf("randCrop get xy = (%d, %d) \n", x, y);

    p[0] = x;
    p[1] = y;

    return 0;
}

static int p2(struct procRes_s *rs)
{
#define SAVE_OUT (0)
#define RAW_W 4320
#define RAW_H  6992

    /* spi0 */
    char ch;
    int totsz=0, fsize=0, pi=0, len, opsz=0, ret=0, max=0, tlen=0, idx=0, ix=0;
    int totduo, iduo=0, maxduo=0;
    char *addr, *laddr=0, *taddr;
    char filedst[128];
    uint32_t popt_fformat=0, popt_colorMode=0;
    int rcord[4], rawoffset=0;
    //char filename[128] = "/mnt/mmc2/sample1.mp4";
    //char filename[128] = "/mnt/mmc2/handmade.jpg";
    //char filebmpraw[128] = "/mnt/mmc2/bmp/cut.bmp";
    char filebmpraw[128] = "/mnt/mmc2/bmp/crop_20_cut.bmp";
    char fileGaryRaw[128] = "/mnt/mmc2/bmp/raw_gray.bmp";
    char fileColorRaw[128] = "/mnt/mmc2/bmp/300dpi_raw_color.bmp";
    //char filebmpraw[128] = "/mnt/mmc2/bmp/crop_20_cut_24bits.bmp";
    //char filebmpraw[128] = "/mnt/mmc2/bmp/crop_20_24bits.bmp";
    char filename[128] = "/mnt/mmc2/scan_pro.jpg";
    char filenameDuo[128] = "/mnt/mmc2/scan_pro.jpg";
    char filetiffraw[128] = "/mnt/mmc2/tiff_raw.bin";
    char samplefile[128] = "/mnt/mmc2/sample/greenhill_%.2d.jpg";
#if (CROP_SAMPLE_SIZE == 35)
    #define TEST_TRG_SIZE 2
    int testTarget[] = {27, 28};
    int curTarget = 0;
    char cropfile[128] = "/mnt/mmc2/crop13/crop_%.2d.jpg";
    char cropfileRaw[128] = "/mnt/mmc2/crop13/crop_%.2d.bmp";
#elif (CROP_SAMPLE_SIZE == 4)
    char cropfile[128] = "/mnt/mmc2/crop4/crop_%.2d.jpg";
#elif (CROP_SAMPLE_SIZE == 5)
#if 1 /* 0614 */
    char cropfile[128] = "/mnt/mmc2/crop5_0614/crop_%.2d.jpg";
#else /* 0616 */
    char cropfile[128] = "/mnt/mmc2/crop5/crop_%.2d.jpg";
#endif
#elif (CROP_SAMPLE_SIZE == 6)
    char cropfile[128] = "/mnt/mmc2/crop/crop_%.2d.jpg";
#elif (CROP_SAMPLE_SIZE == 7)
    char cropfile[128] = "/mnt/mmc2/crop3/crop_%.2d.jpg";
#else
#endif
    //char filename[128] = "/mnt/mmc2/textfile_02.bin";
    char fileback[128] = "/mnt/mmc2/tx/recv_%d.bin";
#if SAVE_OUT
    char fileout[128] = "/mnt/mmc2/tx/sample_%d.jpg";
#endif
    //char filename[128] = "/mnt/mmc2/hilldesert.jpg";
    //char filename[128] = "/mnt/mmc2/sample1.mp4";
    //char filename[128] = "/mnt/mmc2/pattern2.txt";
    FILE *fp = NULL, *fout=NULL, *fduo=NULL;
    
    struct cropCoord_s *pCrop;
    struct cropCoord_s *pCropDuo;

    struct aspConfig_s *pct=0, *pdt=0;
    
    unsigned int *pCROP_COOD_01;
    unsigned int *pCROP_COOD_02;
    unsigned int *pCROP_COOD_03;
    unsigned int *pCROP_COOD_04;
    unsigned int *pCROP_COOD_05;
    unsigned int *pCROP_COOD_06;
    unsigned int *pCROP_COOD_07;
    unsigned int *pCROP_COOD_08;
    unsigned int *pCROP_COOD_09;
    unsigned int *pCROP_COOD_10;
    unsigned int *pCROP_COOD_11;
    unsigned int *pCROP_COOD_12;
    unsigned int *pCROP_COOD_13;
    unsigned int *pCROP_COOD_14;
    unsigned int *pCROP_COOD_15;
    unsigned int *pCROP_COOD_16;
    unsigned int *pCROP_COOD_17;
    unsigned int *pCROP_COOD_18;

    unsigned int *pCROP_COOD_01_duo;
    unsigned int *pCROP_COOD_02_duo;
    unsigned int *pCROP_COOD_03_duo;
    unsigned int *pCROP_COOD_04_duo;
    unsigned int *pCROP_COOD_05_duo;
    unsigned int *pCROP_COOD_06_duo;
    unsigned int *pCROP_COOD_07_duo;
    unsigned int *pCROP_COOD_08_duo;
    unsigned int *pCROP_COOD_09_duo;
    unsigned int *pCROP_COOD_10_duo;
    unsigned int *pCROP_COOD_11_duo;
    unsigned int *pCROP_COOD_12_duo;
    unsigned int *pCROP_COOD_13_duo;
    unsigned int *pCROP_COOD_14_duo;
    unsigned int *pCROP_COOD_15_duo;
    unsigned int *pCROP_COOD_16_duo;
    unsigned int *pCROP_COOD_17_duo;
    unsigned int *pCROP_COOD_18_duo;

    pct = rs->pcfgTable;
    msync(pct, sizeof(struct aspConfig_s) * ASPOP_CODE_MAX, MS_SYNC);

    for (ix = 0; ix < ASPOP_CODE_MAX; ix++) {
        pdt = &pct[ix];
        printf("ctb[%d] 0x%.2x opcode:0x%.2x 0x%.2x val:0x%.2x mask:0x%.2x len:%d \n", ix,
        pdt->opStatus,
        pdt->opCode,
        pdt->opType,
        pdt->opValue,
        pdt->opMask,
        pdt->opBitlen);
    }
    
    popt_fformat = pct[ASPOP_FILE_FORMAT].opValue;
    popt_colorMode = pct[ASPOP_COLOR_MODE].opValue;

    pCrop = rs->pcropCoord;
    pCropDuo = rs->pcropCoordDuo;

    pCROP_COOD_01 = pCrop->CROP_COOD_01;
    pCROP_COOD_02 = pCrop->CROP_COOD_02;
    pCROP_COOD_03 = pCrop->CROP_COOD_03;
    pCROP_COOD_04 = pCrop->CROP_COOD_04;
    pCROP_COOD_05 = pCrop->CROP_COOD_05;
    pCROP_COOD_06 = pCrop->CROP_COOD_06;
    pCROP_COOD_07 = pCrop->CROP_COOD_07;
    pCROP_COOD_08 = pCrop->CROP_COOD_08;
    pCROP_COOD_09 = pCrop->CROP_COOD_09;
    pCROP_COOD_10 = pCrop->CROP_COOD_10;
    pCROP_COOD_11 = pCrop->CROP_COOD_11;
    pCROP_COOD_12 = pCrop->CROP_COOD_12;
    pCROP_COOD_13 = pCrop->CROP_COOD_13;
    pCROP_COOD_14 = pCrop->CROP_COOD_14;
    pCROP_COOD_15 = pCrop->CROP_COOD_15;
    pCROP_COOD_16 = pCrop->CROP_COOD_16;
    pCROP_COOD_17 = pCrop->CROP_COOD_17;
    pCROP_COOD_18 = pCrop->CROP_COOD_18;

    pCROP_COOD_01_duo = pCropDuo->CROP_COOD_01;
    pCROP_COOD_02_duo = pCropDuo->CROP_COOD_02;
    pCROP_COOD_03_duo = pCropDuo->CROP_COOD_03;
    pCROP_COOD_04_duo = pCropDuo->CROP_COOD_04;
    pCROP_COOD_05_duo = pCropDuo->CROP_COOD_05;
    pCROP_COOD_06_duo = pCropDuo->CROP_COOD_06;
    pCROP_COOD_07_duo = pCropDuo->CROP_COOD_07;
    pCROP_COOD_08_duo = pCropDuo->CROP_COOD_08;
    pCROP_COOD_09_duo = pCropDuo->CROP_COOD_09;
    pCROP_COOD_10_duo = pCropDuo->CROP_COOD_10;
    pCROP_COOD_11_duo = pCropDuo->CROP_COOD_11;
    pCROP_COOD_12_duo = pCropDuo->CROP_COOD_12;
    pCROP_COOD_13_duo = pCropDuo->CROP_COOD_13;
    pCROP_COOD_14_duo = pCropDuo->CROP_COOD_14;
    pCROP_COOD_15_duo = pCropDuo->CROP_COOD_15;
    pCROP_COOD_16_duo = pCropDuo->CROP_COOD_16;
    pCROP_COOD_17_duo = pCropDuo->CROP_COOD_17;
    pCROP_COOD_18_duo = pCropDuo->CROP_COOD_18;

    if (infpath[0] != '\0') {
        strcpy(filename, infpath);
    } else {
#if CROP_SAMPLE_SIZE
        sprintf(filename, cropfile, (idx%CROP_SAMPLE_SIZE));
        sprintf(filenameDuo, cropfile, ((idx+1)%CROP_SAMPLE_SIZE));
#else
        sprintf(filename, samplefile, (idx%36));
        sprintf(filenameDuo, samplefile, ((idx+1)%36));
#endif
    }

    fp = fopen(filename, "r");
    if (!fp) {
        sprintf(rs->logs, "file read [%s] failed \n", filename);
        print_f(rs->plogs, "P2", rs->logs);
        while(1);
    } else {
        sprintf(rs->logs, "file read [%s] ok \n", filename);
        print_f(rs->plogs, "P2", rs->logs);
    }
    fclose(fp);

    fduo = fopen(filenameDuo, "r");
    if (!fduo) {
        sprintf(rs->logs, "duo file read [%s] failed \n", filenameDuo);
        print_f(rs->plogs, "P2", rs->logs);
        while(1);
    } else {
        sprintf(rs->logs, "duo file read [%s] ok \n", filenameDuo);
        print_f(rs->plogs, "P2", rs->logs);
    }
    fclose(fduo);

    sprintf(rs->logs, "p2\n");
    print_f(rs->plogs, "P2", rs->logs);

    p2_init(rs);
    while (1) {
        //sprintf(rs->logs, "!\n");
        //print_f(rs->plogs, "P2", rs->logs);

        len = rs_ipc_get(rs, &ch, 1);
        if (len > 0) {

            if (pct[ASPOP_FILE_FORMAT].opStatus == ASPOP_STA_CON) {
                popt_fformat = pct[ASPOP_FILE_FORMAT].opValue;
                popt_colorMode = pct[ASPOP_COLOR_MODE].opValue;
            } else {
                popt_fformat = FILE_FORMAT_JPG;
            }

            sprintf(rs->logs, "%c format:0x%.2x, color: 0x%.2x\n", ch, popt_fformat, popt_colorMode);
            print_f(rs->plogs, "P2", rs->logs);
            
            if (((idx%36) == 13) || ((idx%36) == 14)) {
                idx = 15;
            }
            
            if ((idx%36) == 8)  {
                idx = 9;
            }

            if (infpath[0] != '\0') {
                strcpy(filename, infpath);
            } else if (popt_fformat == FILE_FORMAT_TIFF_I) {
                sprintf(filename, filetiffraw);
            } else if (popt_fformat == FILE_FORMAT_RAW) {
                if (popt_colorMode == COLOR_MODE_COLOR) {
#if CROP_SAMPLE_SIZE                    
                    if (rs->pmetaMass->massIdx > 18) {
                        rs->pmetaMass->massIdx -= 18;
                    }

                    rs->pmetaMass->massIdx = 18+(rs->pmetaMass->massIdx % 12); // for debug

                    idx = rs->pmetaMass->massIdx;

                    if (((idx+1) % 7) == 0) {
                        pct[ASPOP_IMG_LEN].opValue = 0;
                        pct[ASPOP_IMG_LEN_DUO].opValue = 0;
                    } else {
                        pct[ASPOP_IMG_LEN].opValue = 3496;
                        pct[ASPOP_IMG_LEN_DUO].opValue = 3496;
                    }
                    
                    sprintf(filename, cropfileRaw, (idx%CROP_SAMPLE_SIZE));
                    sprintf(filenameDuo, cropfileRaw, ((idx+1)%CROP_SAMPLE_SIZE));
#else
                    strcpy(filename, fileColorRaw);                
#endif
                } else {
                    strcpy(filename, fileGaryRaw);                                
                }
            } else {
#if CROP_SAMPLE_SIZE
                #define SAMPLE_MIN 4
                #define SAMPLE_CROP_MAX CROP_SAMPLE_SIZE // less than CROP_SAMPLE_SIZE
                if (rs->pmetaMass->massIdx > SAMPLE_MIN) {
                    rs->pmetaMass->massIdx -= SAMPLE_MIN;
                }
                rs->pmetaMass->massIdx = SAMPLE_MIN + (rs->pmetaMass->massIdx % (SAMPLE_CROP_MAX - SAMPLE_MIN)); // for debug
                
                idx = rs->pmetaMass->massIdx;

                if (((idx+1) % 7) == 0) {
                    pct[ASPOP_IMG_LEN].opValue = 0;
                    pct[ASPOP_IMG_LEN_DUO].opValue = 0;
                } else {
                    pct[ASPOP_IMG_LEN].opValue = 3496;
                    pct[ASPOP_IMG_LEN_DUO].opValue = 3496;
                }
                
                //idx = testTarget[curTarget % TEST_TRG_SIZE];
                rs->pmetaMass->massIdx = idx;
                curTarget += 1;
                
                sprintf(filename, cropfile, (idx%CROP_SAMPLE_SIZE));
                sprintf(filenameDuo, cropfile, ((idx+1)%CROP_SAMPLE_SIZE));
#else
                sprintf(filename, samplefile, (idx%36));
                sprintf(filenameDuo, samplefile, ((idx+1)%36));
#endif
            }

            sprintf(rs->logs, "get sample file: [%s] \n", filename);
            print_f(rs->plogs, "P2", rs->logs);
            
            sprintf(rs->logs, "get duo sample file: [%s] \n", filenameDuo);
            print_f(rs->plogs, "P2", rs->logs);

#if 0 /* simulate the delay before transmitting */
            int countD = 0;
            while (countD < 10) {
                sleep(1);
                countD++;
                sprintf(rs->logs, " %d s \n", countD);
                print_f(rs->plogs, "P2", rs->logs);
            }
#endif

#if RANDOM_CROP_COORD /* random crop coordinates */
            //randCrop(pCROP_COOD_01, 900, 1000, 1000, 2000);
            pCROP_COOD_01[0] = randomGen(900, 1000);
            pCROP_COOD_01[1] = randomGen(1300, 1800);
            pCROP_COOD_02[0] = randomGen(1400, 1500);
            pCROP_COOD_02[1] = randomGen(2001, 2100);
            pCROP_COOD_03[0] = randomGen(1501, 1600);
            pCROP_COOD_03[1] = pCROP_COOD_02[1];
            //randCrop(pCROP_COOD_04, 2000, 2100, 1000, 2000);
            pCROP_COOD_04[0] = randomGen(2001, 2100);
            pCROP_COOD_04[1] = randomGen(1300, 1800);
            pCROP_COOD_05[0] = randomGen(1501, 1600);
            pCROP_COOD_05[1] = randomGen(900, 999);
            pCROP_COOD_06[0] = randomGen(1400, 1500);
            pCROP_COOD_06[1] = pCROP_COOD_05[1];
            pCROP_COOD_07[0] = pCROP_COOD_06[0] - 5;
            pCROP_COOD_07[1] = pCROP_COOD_06[1] + 5;
            pCROP_COOD_08[0] = pCROP_COOD_05[0] + 5;
            pCROP_COOD_08[1] = pCROP_COOD_05[1] + 5;
            pCROP_COOD_09[0] = pCROP_COOD_07[0] - 5;
            pCROP_COOD_09[1] = pCROP_COOD_07[1] + 5;
            pCROP_COOD_10[0] = pCROP_COOD_08[0] + 5;
            pCROP_COOD_10[1] = pCROP_COOD_08[1] + 5;

            pCROP_COOD_17[0] = pCROP_COOD_02[0] - 5;
            pCROP_COOD_17[1] = pCROP_COOD_02[1] - 5;
            pCROP_COOD_18[0] = pCROP_COOD_03[0] + 5;
            pCROP_COOD_18[1] = pCROP_COOD_03[1] - 5;

            pCROP_COOD_15[0] = pCROP_COOD_17[0] - 5;
            pCROP_COOD_15[1] = pCROP_COOD_17[1] - 5;
            pCROP_COOD_16[0] = pCROP_COOD_18[0] + 5;
            pCROP_COOD_16[1] = pCROP_COOD_18[1] - 5;
            
            pCROP_COOD_13[0] = pCROP_COOD_15[0] - 5;
            pCROP_COOD_13[1] = pCROP_COOD_15[1] - 5;
            pCROP_COOD_14[0] = pCROP_COOD_16[0] + 5;
            pCROP_COOD_14[1] = pCROP_COOD_16[1] - 5;

            pCROP_COOD_11[0] = pCROP_COOD_13[0] - 5;
            pCROP_COOD_11[1] = pCROP_COOD_13[1] - 5;
            pCROP_COOD_12[0] = pCROP_COOD_14[0] + 5;
            pCROP_COOD_12[1] = pCROP_COOD_14[1] - 5;
#else
#if (CROP_NUMBER == 18)
#if CROP_SAMPLE_SIZE
            pCROP_COOD_01[0] = crop_01.samples[idx%CROP_SAMPLE_SIZE].cropx;
            pCROP_COOD_01[1] = crop_01.samples[idx%CROP_SAMPLE_SIZE].cropy;
            pCROP_COOD_02[0] = crop_02.samples[idx%CROP_SAMPLE_SIZE].cropx;
            pCROP_COOD_02[1] = crop_02.samples[idx%CROP_SAMPLE_SIZE].cropy;
            pCROP_COOD_03[0] = crop_03.samples[idx%CROP_SAMPLE_SIZE].cropx;
            pCROP_COOD_03[1] = crop_03.samples[idx%CROP_SAMPLE_SIZE].cropy;
            pCROP_COOD_04[0] = crop_04.samples[idx%CROP_SAMPLE_SIZE].cropx;
            pCROP_COOD_04[1] = crop_04.samples[idx%CROP_SAMPLE_SIZE].cropy;
            pCROP_COOD_05[0] = crop_05.samples[idx%CROP_SAMPLE_SIZE].cropx;
            pCROP_COOD_05[1] = crop_05.samples[idx%CROP_SAMPLE_SIZE].cropy;
            pCROP_COOD_06[0] = crop_06.samples[idx%CROP_SAMPLE_SIZE].cropx;
            pCROP_COOD_06[1] = crop_06.samples[idx%CROP_SAMPLE_SIZE].cropy;
            pCROP_COOD_07[0] = crop_07.samples[idx%CROP_SAMPLE_SIZE].cropx;
            pCROP_COOD_07[1] = crop_07.samples[idx%CROP_SAMPLE_SIZE].cropy;
            pCROP_COOD_08[0] = crop_08.samples[idx%CROP_SAMPLE_SIZE].cropx;
            pCROP_COOD_08[1] = crop_08.samples[idx%CROP_SAMPLE_SIZE].cropy;
            pCROP_COOD_09[0] = crop_09.samples[idx%CROP_SAMPLE_SIZE].cropx;
            pCROP_COOD_09[1] = crop_09.samples[idx%CROP_SAMPLE_SIZE].cropy;
            pCROP_COOD_10[0] = crop_10.samples[idx%CROP_SAMPLE_SIZE].cropx;
            pCROP_COOD_10[1] = crop_10.samples[idx%CROP_SAMPLE_SIZE].cropy;            
            pCROP_COOD_17[0] = crop_17.samples[idx%CROP_SAMPLE_SIZE].cropx;
            pCROP_COOD_17[1] = crop_17.samples[idx%CROP_SAMPLE_SIZE].cropy;
            pCROP_COOD_18[0] = crop_18.samples[idx%CROP_SAMPLE_SIZE].cropx;
            pCROP_COOD_18[1] = crop_18.samples[idx%CROP_SAMPLE_SIZE].cropy;
            pCROP_COOD_15[0] = crop_15.samples[idx%CROP_SAMPLE_SIZE].cropx;
            pCROP_COOD_15[1] = crop_15.samples[idx%CROP_SAMPLE_SIZE].cropy;
            pCROP_COOD_16[0] = crop_16.samples[idx%CROP_SAMPLE_SIZE].cropx;
            pCROP_COOD_16[1] = crop_16.samples[idx%CROP_SAMPLE_SIZE].cropy;
            pCROP_COOD_13[0] = crop_13.samples[idx%CROP_SAMPLE_SIZE].cropx;
            pCROP_COOD_13[1] = crop_13.samples[idx%CROP_SAMPLE_SIZE].cropy;
            pCROP_COOD_14[0] = crop_14.samples[idx%CROP_SAMPLE_SIZE].cropx;
            pCROP_COOD_14[1] = crop_14.samples[idx%CROP_SAMPLE_SIZE].cropy;
            pCROP_COOD_11[0] = crop_11.samples[idx%CROP_SAMPLE_SIZE].cropx;
            pCROP_COOD_11[1] = crop_11.samples[idx%CROP_SAMPLE_SIZE].cropy;
            pCROP_COOD_12[0] = crop_12.samples[idx%CROP_SAMPLE_SIZE].cropx;
            pCROP_COOD_12[1] = crop_12.samples[idx%CROP_SAMPLE_SIZE].cropy;

            pCROP_COOD_01_duo[0] = crop_01.samples[(idx+1)%CROP_SAMPLE_SIZE].cropx;
            pCROP_COOD_01_duo[1] = crop_01.samples[(idx+1)%CROP_SAMPLE_SIZE].cropy;
            pCROP_COOD_02_duo[0] = crop_02.samples[(idx+1)%CROP_SAMPLE_SIZE].cropx;
            pCROP_COOD_02_duo[1] = crop_02.samples[(idx+1)%CROP_SAMPLE_SIZE].cropy;
            pCROP_COOD_03_duo[0] = crop_03.samples[(idx+1)%CROP_SAMPLE_SIZE].cropx;
            pCROP_COOD_03_duo[1] = crop_03.samples[(idx+1)%CROP_SAMPLE_SIZE].cropy;
            pCROP_COOD_04_duo[0] = crop_04.samples[(idx+1)%CROP_SAMPLE_SIZE].cropx;
            pCROP_COOD_04_duo[1] = crop_04.samples[(idx+1)%CROP_SAMPLE_SIZE].cropy;
            pCROP_COOD_05_duo[0] = crop_05.samples[(idx+1)%CROP_SAMPLE_SIZE].cropx;
            pCROP_COOD_05_duo[1] = crop_05.samples[(idx+1)%CROP_SAMPLE_SIZE].cropy;
            pCROP_COOD_06_duo[0] = crop_06.samples[(idx+1)%CROP_SAMPLE_SIZE].cropx;
            pCROP_COOD_06_duo[1] = crop_06.samples[(idx+1)%CROP_SAMPLE_SIZE].cropy;
            pCROP_COOD_07_duo[0] = crop_07.samples[(idx+1)%CROP_SAMPLE_SIZE].cropx;
            pCROP_COOD_07_duo[1] = crop_07.samples[(idx+1)%CROP_SAMPLE_SIZE].cropy;
            pCROP_COOD_08_duo[0] = crop_08.samples[(idx+1)%CROP_SAMPLE_SIZE].cropx;
            pCROP_COOD_08_duo[1] = crop_08.samples[(idx+1)%CROP_SAMPLE_SIZE].cropy;
            pCROP_COOD_09_duo[0] = crop_09.samples[(idx+1)%CROP_SAMPLE_SIZE].cropx;
            pCROP_COOD_09_duo[1] = crop_09.samples[(idx+1)%CROP_SAMPLE_SIZE].cropy;
            pCROP_COOD_10_duo[0] = crop_10.samples[(idx+1)%CROP_SAMPLE_SIZE].cropx;
            pCROP_COOD_10_duo[1] = crop_10.samples[(idx+1)%CROP_SAMPLE_SIZE].cropy;            
            pCROP_COOD_17_duo[0] = crop_17.samples[(idx+1)%CROP_SAMPLE_SIZE].cropx;
            pCROP_COOD_17_duo[1] = crop_17.samples[(idx+1)%CROP_SAMPLE_SIZE].cropy;
            pCROP_COOD_18_duo[0] = crop_18.samples[(idx+1)%CROP_SAMPLE_SIZE].cropx;
            pCROP_COOD_18_duo[1] = crop_18.samples[(idx+1)%CROP_SAMPLE_SIZE].cropy;
            pCROP_COOD_15_duo[0] = crop_15.samples[(idx+1)%CROP_SAMPLE_SIZE].cropx;
            pCROP_COOD_15_duo[1] = crop_15.samples[(idx+1)%CROP_SAMPLE_SIZE].cropy;
            pCROP_COOD_16_duo[0] = crop_16.samples[(idx+1)%CROP_SAMPLE_SIZE].cropx;
            pCROP_COOD_16_duo[1] = crop_16.samples[(idx+1)%CROP_SAMPLE_SIZE].cropy;
            pCROP_COOD_13_duo[0] = crop_13.samples[(idx+1)%CROP_SAMPLE_SIZE].cropx;
            pCROP_COOD_13_duo[1] = crop_13.samples[(idx+1)%CROP_SAMPLE_SIZE].cropy;
            pCROP_COOD_14_duo[0] = crop_14.samples[(idx+1)%CROP_SAMPLE_SIZE].cropx;
            pCROP_COOD_14_duo[1] = crop_14.samples[(idx+1)%CROP_SAMPLE_SIZE].cropy;
            pCROP_COOD_11_duo[0] = crop_11.samples[(idx+1)%CROP_SAMPLE_SIZE].cropx;
            pCROP_COOD_11_duo[1] = crop_11.samples[(idx+1)%CROP_SAMPLE_SIZE].cropy;
            pCROP_COOD_12_duo[0] = crop_12.samples[(idx+1)%CROP_SAMPLE_SIZE].cropx;
            pCROP_COOD_12_duo[1] = crop_12.samples[(idx+1)%CROP_SAMPLE_SIZE].cropy;
#endif
#else
            pCROP_COOD_07[0] = pCROP_COOD_06[0];
            pCROP_COOD_07[1] = pCROP_COOD_06[1];
            pCROP_COOD_08[0] = pCROP_COOD_05[0];
            pCROP_COOD_08[1] = pCROP_COOD_05[1];
            pCROP_COOD_09[0] = pCROP_COOD_07[0];
            pCROP_COOD_09[1] = pCROP_COOD_07[1];
            pCROP_COOD_10[0] = pCROP_COOD_08[0];
            pCROP_COOD_10[1] = pCROP_COOD_08[1];
                                                
            pCROP_COOD_17[0] = pCROP_COOD_02[0];
            pCROP_COOD_17[1] = pCROP_COOD_02[1];
            pCROP_COOD_18[0] = pCROP_COOD_03[0];
            pCROP_COOD_18[1] = pCROP_COOD_03[1];
                                                
            pCROP_COOD_15[0] = pCROP_COOD_17[0];
            pCROP_COOD_15[1] = pCROP_COOD_17[1];
            pCROP_COOD_16[0] = pCROP_COOD_18[0];
            pCROP_COOD_16[1] = pCROP_COOD_18[1];
                                                
            pCROP_COOD_13[0] = pCROP_COOD_15[0];
            pCROP_COOD_13[1] = pCROP_COOD_15[1];
            pCROP_COOD_14[0] = pCROP_COOD_16[0];
            pCROP_COOD_14[1] = pCROP_COOD_16[1];
                                                
            pCROP_COOD_11[0] = pCROP_COOD_13[0];
            pCROP_COOD_11[1] = pCROP_COOD_13[1];
            pCROP_COOD_12[0] = pCROP_COOD_14[0];
            pCROP_COOD_12[1] = pCROP_COOD_14[1];
#endif
#endif
            idx++;
            if (ch == 'r') {

#if SAVE_OUT
                fout = find_save(filedst, fileout);
                if (fout) {
                    sprintf(rs->logs, "file save back to [%s]\n",filedst);
                    print_f(rs->plogs, "P2", rs->logs); 
                } else {
                    sprintf(rs->logs, "FAIL to find file [%s]\n",fileback);
                    print_f(rs->plogs, "P2", rs->logs); 
                }
#endif
                fp = fopen(filename, "r");
                if (!fp) {
                    sprintf(rs->logs, "file read [%s] failed \n", filename);
                    print_f(rs->plogs, "P2", rs->logs);
                    continue;
                } else {
                    sprintf(rs->logs, "file read [%s] ok \n", filename);
                    print_f(rs->plogs, "P2", rs->logs);
                }

                totsz = 0;
                pi = 0;
                
                ret = fseek(fp, 0, SEEK_END);
                if (ret) {
                    sprintf(rs->logs, " file seek failed!! ret:%d \n", ret);
                    print_f(rs->plogs, "P2", rs->logs);
                } 
                
                max = ftell(fp);
                sprintf(rs->logs, " file [%s] size: %d \n", filename, max);
                print_f(rs->plogs, "P2", rs->logs);

                ret = fseek(fp, 0, SEEK_SET);
                if (ret) {
                    sprintf(rs->logs, " file seek failed!! ret:%d \n", ret);
                    print_f(rs->plogs, "P2", rs->logs);
                }

                while (1) {
                    len = ring_buf_get_dual(rs->pdataRx, &addr, pi);      
                    memset(addr, 0xff, len);
                    if (max < len) {
                        len = max;
                    }

                    ret = fseek(fp, totsz, SEEK_SET);
                    if (ret) {
                        sprintf(rs->logs, " file seek failed!! ret:%d \n", ret);
                        print_f(rs->plogs, "P2", rs->logs);
                    }

                    msync(addr, len, MS_SYNC);
                    
                    fsize = fread(addr, 1, len, fp);

                    
                    totsz += fsize;
                    max -= fsize;

                    ring_buf_prod_dual(rs->pdataRx, pi);
#if SAVE_OUT
                    tlen = fwrite(addr, 1, fsize, fout);
#else
                    tlen = fsize;
#endif
                    sprintf(rs->logs, " %d %d/%d/%d - %d/%d\n", pi, fsize, tlen, len, totsz, max);
                    print_f(rs->plogs, "P2", rs->logs);
                    
                    if (!max) break;
                    pi++;
                    rs_ipc_put(rs, "r", 1);
                    
                }
#if SAVE_OUT
                /* align to SPI_TRUNK_SZ */
                tlen = fsize % 1024;
                sprintf(rs->logs, "1.r %d sz %d \n", tlen, fsize);
                print_f(rs->plogs, "P2", rs->logs);

                if (tlen) {
                    fsize = fsize + 1024 - tlen;
                }

                tlen = totsz % 1024;
                sprintf(rs->logs, "2.r %d sz %d \n", tlen, totsz);
                print_f(rs->plogs, "P2", rs->logs);

                if (tlen) {
                    totsz = totsz + 1024 - tlen;
                }
#endif
                ring_buf_set_last_dual(rs->pdataRx, fsize, pi);
                rs_ipc_put(rs, "r", 1);
                rs_ipc_put(rs, "e", 1);

                rs->pmch->cur.info = totsz;

                sprintf(rs->logs, "file [%s] read size: %d \n",filename, totsz);
                print_f(rs->plogs, "P2", rs->logs);
#if SAVE_OUT
                fflush(fout);
                fclose(fout);
#endif
                fclose(fp);
                fp = NULL;
            }

            if (ch == 'g') {
                int totsz=0, fsize=0, pi=0, len, ret;
                char *addr, ch, *laddr, *saddr, *pool;
                char filename[128] = "/mnt/mmc2/pattern2.txt";
                FILE *fp;

                laddr = malloc(64);
                saddr = malloc(64);
                pool = malloc(4096);

                ch = 0x30;
                addr = laddr;
                for (pi = 0; pi < 31; pi++) {
                    addr[pi] = ch;
                    ch++;
                }

                ch = 0x30;
                addr = laddr+31;
                for (pi = 0; pi < 31; pi++) {
                    addr[pi] = ch;
                    ch++;
                }

                fp = fopen(filename, "wr");
                if (!fp) {
                    sprintf(rs->logs, "file read [%s] failed \n", filename);
                    print_f(rs->plogs, "P2", rs->logs);
                }
/*
                addr = pool;
                for (pi = 0; pi < 128; pi++) {
                    memcpy(addr, laddr + (pi % 32), 31);
                    addr+=31;
                    *addr = '\n';
                    addr+=1;
                }
*/
                for (pi = 0; pi < 2097151; pi++) {
                    memcpy(saddr, laddr + (pi % 31), 31);
                    saddr[31] = '\n';
                    ret = fwrite(saddr, 1, 32, fp);
                    //sprintf(rs->logs, "g ret:%d - %d\n", ret, pi);
                    //print_f(rs->plogs, "P2", rs->logs);
                }
                sprintf(rs->logs, "g ret:%d - %d\n", ret, pi);
                print_f(rs->plogs, "P2", rs->logs);

                fclose(fp);

                fp = fopen(filename, "r");
                if (!fp) {
                    sprintf(rs->logs, "file read [%s] failed \n", filename);
                    print_f(rs->plogs, "P2", rs->logs);
                }

                len = ring_buf_get_dual(rs->pdataRx, &addr, pi);
                fsize = fread(addr, 1, len, fp);
                totsz += fsize;
                while (fsize == len) {
                    ring_buf_prod_dual(rs->pdataRx, pi);
                    pi++;
                    rs_ipc_put(rs, "r", 1);
                    len = ring_buf_get_dual(rs->pdataRx, &addr, pi);
                    fsize = fread(addr, 1, len, fp);
                    totsz += fsize;
                }

                ring_buf_prod_dual(rs->pdataRx, pi);
                ring_buf_set_last_dual(rs->pdataRx, fsize, pi);
                rs_ipc_put(rs, "r", 1);
                rs_ipc_put(rs, "e", 1);

                sprintf(rs->logs, "file [%s] read size: %d \n",filename, totsz);
                print_f(rs->plogs, "P2", rs->logs);

                fclose(fp);
            }

            if (ch == 'k') { 
                fp = find_save(filedst, fileback);
                if (fp) {
                    sprintf(rs->logs, "file save back to [%s]\n",filedst);
                    print_f(rs->plogs, "P2", rs->logs); 
                } else {
                    sprintf(rs->logs, "FAIL to find file [%s]\n",fileback);
                    print_f(rs->plogs, "P2", rs->logs); 
                }
                
                totsz = rs->pmch->cur.info;
                sprintf(rs->logs, "total size: %d\n", totsz);
                print_f(rs->plogs, "P2", rs->logs); 

                totsz = 0;
                pi = 0;
                while (1) {
                    ret = ring_buf_cons(rs->pdataTx, &addr, &len);
                    if ((ret == 0) ||(ret == -2)) {

                        if (len > 0) {
                            pi++;                    
                            msync(addr, len, MS_SYNC);
                            #if 1 /*debug*/
                            opsz = fwrite(addr, 1, len, fp);
                            totsz += opsz;
                            #else
                            opsz = len;
                            #endif
                            sprintf(rs->logs, "w %d/%d \n", opsz, totsz);
                            print_f(rs->plogs, "P2", rs->logs);        

                            memset(addr, 0x95, len);
                        }

                        if (ret == -2) {
                            sprintf(rs->logs, "save len:%d cnt:%d total:%d -loop end\n", opsz, pi, totsz);
                            print_f(rs->plogs, "P2", rs->logs);         
                            break;
                        }
                    } else {
                        sprintf(rs->logs, "wait for buffer, ret:%d\n", ret);
                        print_f(rs->plogs, "P2", rs->logs);         
                    }

                    if (ch != 'K') {
                        ch = 0;
                        rs_ipc_get(rs, &ch, 1);
                    }
                }

                while (ch != 'K') {
                    sprintf(rs->logs, "%c clr\n", ch);
                    print_f(rs->plogs, "P2", rs->logs);         
                    ch = 0;
                    rs_ipc_get(rs, &ch, 1);
                }

                sync();
                fflush(fp);
                fclose(fp);

                rs_ipc_put(rs, "K", 1);
                sprintf(rs->logs, "file save cnt:%d total:%d- end\n", pi, totsz);
                print_f(rs->plogs, "P2", rs->logs);      
                
            }

            if (ch == 's') { /*single*/
#if SAVE_OUT
                fout = find_save(filedst, fileout);
                if (fout) {
                    sprintf(rs->logs, "file save back to [%s]\n",filedst);
                    print_f(rs->plogs, "P2", rs->logs); 
                } else {
                    sprintf(rs->logs, "FAIL to find file [%s]\n",fileback);
                    print_f(rs->plogs, "P2", rs->logs); 
                }
#endif

                fp = fopen(filename, "r");
                if (!fp) {
                    sprintf(rs->logs, "file read [%s] failed \n", filename);
                    print_f(rs->plogs, "P2", rs->logs);
                    continue;
                } else {
                    sprintf(rs->logs, "file read [%s] ok \n", filename);
                    print_f(rs->plogs, "P2", rs->logs);
                }

                totsz = 0;
                pi = 0;
                
                ret = fseek(fp, 0, SEEK_END);
                if (ret) {
                    sprintf(rs->logs, " file seek failed!! ret:%d \n", ret);
                    print_f(rs->plogs, "P2", rs->logs);
                } 
                
                max = ftell(fp);
                sprintf(rs->logs, " file [%s] size: %d \n", filename, max);
                print_f(rs->plogs, "P2", rs->logs);

                ret = fseek(fp, 0, SEEK_SET);
                if (ret) {
                    sprintf(rs->logs, " file seek failed!! ret:%d \n", ret);
                    print_f(rs->plogs, "P2", rs->logs);
                }

#if TIFF_RAW 
                if (popt_fformat == FILE_FORMAT_TIFF_I) {
                    laddr = 0;
                    laddr = malloc(max);
                    if (!laddr) {
                        sprintf(rs->logs, " allocate memory size %d falied!!! ret:%d \n", max, laddr);
                        print_f(rs->plogs, "P2", rs->logs);
                        continue;
                    }
                
                    fsize = fread(laddr, 1, max, fp);
                
                    sprintf(rs->logs, " read file size %d, result: %d!!! ret:%d \n", max, fsize);
                    print_f(rs->plogs, "P2", rs->logs);
      
                    rcord[0] = 2+(rawoffset%RAW_W);
                    rcord[1] = 2+(rawoffset%RAW_W);
                    rcord[2] = 1000+(rawoffset%RAW_H);
                    rcord[3] = 1000+(rawoffset%RAW_H);

                    ret = tiffClearBox(laddr, rcord, RAW_W, RAW_H, max);

                    rcord[0] = 4+(rawoffset%RAW_W);
                    rcord[1] = 4+(rawoffset%RAW_W);
                    rcord[2] = 998+(rawoffset%RAW_H);
                    rcord[3] = 998+(rawoffset%RAW_H);

                    ret = tiffDrawLine(laddr, rcord, RAW_W, RAW_H, max);                    

                    rcord[0] = 100+(rawoffset%RAW_W);
                    rcord[1] = 100+(rawoffset%RAW_W);
                    rcord[2] = 900+(rawoffset%RAW_H);
                    rcord[3] = 900+(rawoffset%RAW_H);

                    ret = tiffDrawBox(laddr, rcord, RAW_W, RAW_H, max);                    

                    rcord[0] = 200+(rawoffset%RAW_W);
                    rcord[1] = 200+(rawoffset%RAW_W);
                    rcord[2] = 800+(rawoffset%RAW_H);
                    rcord[3] = 800+(rawoffset%RAW_H);

                    ret = tiffClearBox(laddr, rcord, RAW_W, RAW_H, max);                    

                    rcord[0] = 100+(rawoffset%RAW_W);
                    rcord[1] = 100+(rawoffset%RAW_W);
                    rcord[2] = 900+(rawoffset%RAW_H);
                    rcord[3] = 900+(rawoffset%RAW_H);

                    ret = tiffClearLine(laddr, rcord, RAW_W, RAW_H, max);                    
                
                    sprintf(rs->logs, " tiff draw line to raw, ret:%d \n", ret);
                    print_f(rs->plogs, "P2", rs->logs);

                    rawoffset += 119;
                    while ((((rawoffset%RAW_W) + 1000) > RAW_W) ||(((rawoffset%RAW_H) + 1000) > RAW_H)) {
                        rawoffset += 119;
                    }
                
                    fsize = 0;
                }
#endif

                while (1) {
                    len = 0;
                    len = ring_buf_get(rs->pdataTx, &addr);
                    while (len <= 0) {
                        usleep(150000);
                        len = ring_buf_get(rs->pdataTx, &addr);
                    }

                    memset(addr, 0x15, len);
                    if (max < len) {
                        len = max;
                    }
#if TIFF_RAW 
                    if (popt_fformat == FILE_FORMAT_TIFF_I) {
                        taddr = laddr + totsz;
                        memcpy(addr, taddr, len);
                        msync(addr, len, MS_SYNC);
                        fsize = len;
                    }
#else
                    ret = fseek(fp, totsz, SEEK_SET);
                    if (ret) {
                        sprintf(rs->logs, " file seek failed!! ret:%d \n", ret);
                        print_f(rs->plogs, "P2", rs->logs);
                    }

                    
                    fsize = fread(addr, 1, len, fp);
                    msync(addr, len, MS_SYNC);
#endif

                    totsz += fsize;
                    max -= len;

                    ring_buf_prod(rs->pdataTx);
#if SAVE_OUT
                    tlen = fwrite(addr, 1, fsize, fout);
#else
                    tlen = len;
#endif

                    sprintf(rs->logs, " %d - %d/%d\n", pi, totsz, max);
                    print_f(rs->plogs, "P2s", rs->logs);
                    
                    if (!max) break;
                    pi++;
                    rs_ipc_put(rs, "s", 1);
                    
                }
                

                /* align to SPI_TRUNK_SZ */
                tlen = fsize % 1024;
                sprintf(rs->logs, "1.r %d sz %d \n", tlen, fsize);
                print_f(rs->plogs, "P2", rs->logs);

                if (tlen) {
                    fsize = fsize + 1024 - tlen;
                }

                tlen = totsz % 1024;
                sprintf(rs->logs, "2.r %d sz %d \n", tlen, totsz);
                print_f(rs->plogs, "P2", rs->logs);

                if (tlen) {
                    totsz = totsz + 1024 - tlen;
                }
#if 0 /* debug */
                fsize = SPI_TRUNK_SZ;
#endif
                ring_buf_set_last(rs->pdataTx, fsize);
                rs_ipc_put(rs, "s", 1);
                rs_ipc_put(rs, "S", 1);
                
#if SAVE_OUT
                fflush(fout);
                fclose(fout);
#endif
                rs->pmch->cur.info = totsz;

                sprintf(rs->logs, "file [%s] read size: %d \n",filename, totsz);
                print_f(rs->plogs, "P2", rs->logs);
#if TIFF_RAW 
                if (laddr) {
                    free(laddr);
                    laddr = 0;
                }
#endif
                fclose(fp);
                fp = NULL;
            }

            if (ch == 'd') { /*double*/
                fp = fopen(filename, "r");
                if (!fp) {
                    sprintf(rs->logs, "file read [%s] failed \n", filename);
                    print_f(rs->plogs, "P2d", rs->logs);
                    continue;
                } else {
                    sprintf(rs->logs, "file read [%s] ok \n", filename);
                    print_f(rs->plogs, "P2d", rs->logs);
                }

                ret = fseek(fp, 0, SEEK_END);
                if (ret) {
                    sprintf(rs->logs, " file seek failed!! ret:%d \n", ret);
                    print_f(rs->plogs, "P2", rs->logs);
                } 
                
                max = ftell(fp);
                sprintf(rs->logs, " file [%s] size: %d \n", filename, max);
                print_f(rs->plogs, "P2", rs->logs);

                ret = fseek(fp, 0, SEEK_SET);
                if (ret) {
                    sprintf(rs->logs, " file seek failed!! ret:%d \n", ret);
                    print_f(rs->plogs, "P2", rs->logs);
                }

                fduo = fopen(filenameDuo, "r");
                if (!fduo) {
                    sprintf(rs->logs, "duo file read [%s] failed \n", filenameDuo);
                    print_f(rs->plogs, "P2d", rs->logs);
                    continue;
                } else {
                    sprintf(rs->logs, "duo file read [%s] ok \n", filenameDuo);
                    print_f(rs->plogs, "P2d", rs->logs);
                }
                
                ret = fseek(fduo, 0, SEEK_END);
                if (ret) {
                    sprintf(rs->logs, " duo file seek failed!! ret:%d \n", ret);
                    print_f(rs->plogs, "P2", rs->logs);
                } 
                
                maxduo = ftell(fduo);
                sprintf(rs->logs, " duo file [%s] size: %d \n", filenameDuo, maxduo);
                print_f(rs->plogs, "P2", rs->logs);

                ret = fseek(fduo, 0, SEEK_SET);
                if (ret) {
                    sprintf(rs->logs, " duo file seek failed!! ret:%d \n", ret);
                    print_f(rs->plogs, "P2", rs->logs);
                }

                totsz = 0;
                pi = 0;

                totduo = 0;
                iduo = 0;

                while ((max > 0) || (maxduo > 0)) {
#if 1
                    if (max > 0) {
                        ret = fseek(fp, totsz, SEEK_SET);
                        if (ret) {
                            sprintf(rs->logs, " file seek failed!! ret:%d - 1\n", ret);
                            print_f(rs->plogs, "P2", rs->logs);
                            break;
                        }
                        len = 0;
                        len = ring_buf_get(rs->pdataTx, &addr);
                        while (len <= 0) {
                            usleep(10000);
                            len = ring_buf_get(rs->pdataTx, &addr);
                        }
                        memset(addr, 0xff, len);
                        if (max < len) {
                            len = max;
                            max = 0;
                        } else {
                            max -= len;
                        }
                        
                        //msync(addr, len, MS_SYNC);
                        fsize = fread(addr, 1, len, fp);
                        msync(addr, len, MS_SYNC);
                        ring_buf_prod(rs->pdataTx);
                        tlen = len;
                        sprintf(rs->logs, " %d - %d,%d - 1\n", pi, totsz, fsize);
                        print_f(rs->plogs, "P2d", rs->logs);
                        rs_ipc_put(rs, "s", 1);
                        
                        totsz += fsize;
                        pi++;

                        if (max == 0) {
                            /* align to SPI_TRUNK_SZ */
                            tlen = fsize % 1024;
                            sprintf(rs->logs, "1.r %d sz %d \n", tlen, fsize);
                            print_f(rs->plogs, "P2", rs->logs);

                            if (tlen) {
                                fsize = fsize + 1024 - tlen;
                            }

                            tlen = totsz % 1024;
                            sprintf(rs->logs, "2.r %d sz %d \n", tlen, totsz);
                            print_f(rs->plogs, "P2", rs->logs);

                            if (tlen) {
                                totsz = totsz + 1024 - tlen;
                            }
                            ring_buf_set_last(rs->pdataTx, fsize);
                        }
                    }
#endif
#if 1
                    if (maxduo > 0) {
                        ret = fseek(fduo, totduo, SEEK_SET);
                        if (ret) {
                            sprintf(rs->logs, " file seek failed!! ret:%d - 2\n", ret);
                            print_f(rs->plogs, "P2", rs->logs);
                            break;
                        }
                        len = 0;
                        len = ring_buf_get(rs->pcmdTx, &addr);
                        while (len <= 0) {
                            usleep(10000);
                            len = ring_buf_get(rs->pcmdTx, &addr);
                        }
                        memset(addr, 0xff, len);
                        if (maxduo < len) {
                            len = maxduo;
                            maxduo = 0;
                        } else {
                            maxduo -= len;
                        }
                        
                        //msync(addr, len, MS_SYNC);
                        fsize = fread(addr, 1, len, fduo);
                        msync(addr, len, MS_SYNC);
                        
                        ring_buf_prod(rs->pcmdTx);
                        tlen = len;
                        sprintf(rs->logs, " %d - %d,%d - 2\n", iduo, totduo, fsize);
                        print_f(rs->plogs, "P2d", rs->logs);
                        rs_ipc_put(rs, "d", 1);
                        totduo += fsize;
                        iduo++;

                        if (maxduo == 0) {
                            /* align to SPI_TRUNK_SZ */
                            tlen = fsize % 1024;
                            sprintf(rs->logs, "1. duo r %d sz %d \n", tlen, fsize);
                            print_f(rs->plogs, "P2", rs->logs);

                            if (tlen) {
                                fsize = fsize + 1024 - tlen;
                            }

                            tlen = totduo % 1024;
                            sprintf(rs->logs, "2. duo r %d sz %d \n", tlen, totduo);
                            print_f(rs->plogs, "P2", rs->logs);

                            if (tlen) {
                                totduo = totduo + 1024 - tlen;
                            }

                            ring_buf_set_last(rs->pcmdTx, fsize);
                        }
                    }
#endif

                }
                

                rs_ipc_put(rs, "S", 1);
                rs_ipc_put(rs, "D", 1);

                rs->pmch->cur.info = totsz;
                rs->pmch->tmp.info = totduo;
                
                sprintf(rs->logs, "file [%s] read size: %d \n",filename, totsz);
                print_f(rs->plogs, "P2d", rs->logs);
                
                sprintf(rs->logs, "duo file [%s] read size: %d \n",filenameDuo, totduo);
                print_f(rs->plogs, "P2d", rs->logs);

                fclose(fp);
                fclose(fduo);
                fp = NULL;
                fduo = NULL;
            }

            if (ch == 'b') { /*raw*/
#if SAVE_OUT
                fout = find_save(filedst, fileout);
                if (fout) {
                    sprintf(rs->logs, "file save back to [%s]\n",filedst);
                    print_f(rs->plogs, "P2", rs->logs); 
                } else {
                    sprintf(rs->logs, "FAIL to find file [%s]\n",fileback);
                    print_f(rs->plogs, "P2", rs->logs); 
                }
#endif

                fp = fopen(filebmpraw, "r");
                if (!fp) {
                    sprintf(rs->logs, "file read [%s] failed \n", filebmpraw);
                    print_f(rs->plogs, "P2", rs->logs);
                    continue;
                } else {
                    sprintf(rs->logs, "file read [%s] ok \n", filebmpraw);
                    print_f(rs->plogs, "P2", rs->logs);
                }

                totsz = 0;
                pi = 0;
                
                ret = fseek(fp, 0, SEEK_END);
                if (ret) {
                    sprintf(rs->logs, " file seek failed!! ret:%d \n", ret);
                    print_f(rs->plogs, "P2", rs->logs);
                } 
                
                max = ftell(fp);
                sprintf(rs->logs, " file [%s] size: %d \n", filebmpraw, max);
                print_f(rs->plogs, "P2", rs->logs);

                ret = fseek(fp, 0, SEEK_SET);
                if (ret) {
                    sprintf(rs->logs, " file seek failed!! ret:%d \n", ret);
                    print_f(rs->plogs, "P2", rs->logs);
                }

                while (1) {
                    len = 0;
                    len = ring_buf_get(rs->pdataTx, &addr);
                    while (len <= 0) {
                        usleep(150000);
                        len = ring_buf_get(rs->pdataTx, &addr);
                    }

                    memset(addr, 0xbb, len);
                    if (max < len) {
                        len = max;
                    }

                    ret = fseek(fp, totsz, SEEK_SET);
                    if (ret) {
                        sprintf(rs->logs, " file seek failed!! ret:%d \n", ret);
                        print_f(rs->plogs, "P2", rs->logs);
                    }

                    fsize = fread(addr, 1, len, fp);
                    msync(addr, len, MS_SYNC);

                    totsz += fsize;
                    max -= len;

                    ring_buf_prod(rs->pdataTx);
#if SAVE_OUT
                    tlen = fwrite(addr, 1, fsize, fout);
#else
                    tlen = len;
#endif

                    sprintf(rs->logs, " %d - %d/%d\n", pi, totsz, max);
                    print_f(rs->plogs, "P2s", rs->logs);
                    
                    if (!max) break;
                    pi++;
                    rs_ipc_put(rs, "b", 1);
                    
                }
                

                /* align to SPI_TRUNK_SZ */
                tlen = fsize % 1024;
                sprintf(rs->logs, "1.r %d sz %d \n", tlen, fsize);
                print_f(rs->plogs, "P2", rs->logs);

                if (tlen) {
                    fsize = fsize + 1024 - tlen;
                }

                tlen = totsz % 1024;
                sprintf(rs->logs, "2.r %d sz %d \n", tlen, totsz);
                print_f(rs->plogs, "P2", rs->logs);

                if (tlen) {
                    totsz = totsz + 1024 - tlen;
                }
#if 0 /* debug */
                fsize = SPI_TRUNK_SZ;
#endif
                ring_buf_set_last(rs->pdataTx, fsize);
                rs_ipc_put(rs, "b", 1);
                rs_ipc_put(rs, "B", 1);
                
#if SAVE_OUT
                fflush(fout);
                fclose(fout);
#endif
                rs->pmch->cur.info = totsz;

                sprintf(rs->logs, "file [%s] read size: %d \n",filebmpraw, totsz);
                print_f(rs->plogs, "P2", rs->logs);

                fclose(fp);
                fp = NULL;
            }

        }
    }

    p2_end(rs);
    return 0;
}

static int p3(struct procRes_s *rs)
{
    /* spi1 */
    char ch;
    int len;

    sprintf(rs->logs, "p3\n");
    print_f(rs->plogs, "P3", rs->logs);

    p3_init(rs);
    while (1) {
        //sprintf(rs->logs, "/\n");
        //print_f(rs->plogs, "P3", rs->logs);

        len = rs_ipc_get(rs, &ch, 1);
        if (len > 0) {
            sprintf(rs->logs, "%c \n", ch);
            print_f(rs->plogs, "P3", rs->logs);
        }
    }

    p3_end(rs);
    return 0;
}

#define MSP_P4_SAVE_DAT (0)
static int p4(struct procRes_s *rs)
{
#define OUT_SAVE (0)

#if OUT_SAVE
    char fileout[128] = "/mnt/mmc2/tx/fat_lov.bin";
    FILE *fout = NULL;
#endif
    float flsize, fltime;
    char ch, *tx, *rx, *tx_buff, *rx_buff, *addr=0;
    uint16_t *tx16, *rx16, in16;
    char *tx8, *rx8;
    int len = 0, cmode = 0, ret, pi=0, acusz=0, starts=0, opsz=0, totsz, tdiff;
    int slen=0;
    uint32_t bitset;
    struct DiskFile_s *pf;
    struct aspMetaData_s *pmeta;

    sprintf(rs->logs, "p4\n");
    print_f(rs->plogs, "P4", rs->logs);

    p4_init(rs);

    pmeta = rs->pmetain;
    
    pf = &rs->pmch->fdsk;
    tx = malloc(64);
    rx = malloc(64);
    rx_buff = malloc(SPI_TRUNK_SZ);

    int i;
    for (i = 0; i < 64; i+=4) {
        tx[i] = 0xaa;
        tx[i+1] = 0x55; 
        tx[i+2] = 0xff; 
        tx[i+3] = 0xee; 
    }

    tx16 = (uint16_t *)tx;
    rx16 = (uint16_t *)rx;	

    while (1) {
        //printf("^");
        //sprintf(rs->logs, "^\n");
        //print_f(rs->plogs, "P4", rs->logs);

        len = 0;
        len = rs_ipc_get(rs, &ch, 1);
        if (len > 0) {
            sprintf(rs->logs, "%c \n", ch);
            print_f(rs->plogs, "P4", rs->logs);

            switch (ch) {
                case 'g':
                    cmode = 1;
                    break;
                case 'c':
                    cmode = 2;
                    break;
                case 'e':
                    pi = 0;
                case 'r':
                    cmode = 3;
                    break;
                case 'b':
                    cmode = 4;
                    break;
                case 's':
                    cmode = 5;
                    break;
                case 'k':
                    cmode = 6;
                    break;
                case 'i':
                    cmode = 7;
                    break;
                case 'j':
                    cmode = 8;
                    break;
                case 'm':
                    cmode = 9;
                    break;
                default:
                    break;
            }
        }

        if (cmode == 1) {
            bitset = 1;
            ioctl(rs->spifd, _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
            sprintf(rs->logs, "Check if RDY pin == 0 (pin:%d)\n", bitset);
            print_f(rs->plogs, "P4", rs->logs);

            while (1) {
                bitset = 1;
                ioctl(rs->spifd, _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
                //sprintf(rs->logs, "Get RDY pin: %d\n", bitset);
                //print_f(rs->plogs, "P4", rs->logs);

                if (bitset == 0) break;
            }
            if (!bitset) rs_ipc_put(rs, "G", 1);
        } else if (cmode == 2) {
            int bits = 16;
            ret = ioctl(rs->spifd, SPI_IOC_WR_BITS_PER_WORD, &bits);
            if (ret == -1) {
                sprintf(rs->logs, "can't set bits per word"); 
                print_f(rs->plogs, "P4", rs->logs);
            }
            ret = ioctl(rs->spifd, SPI_IOC_RD_BITS_PER_WORD, &bits); 
            if (ret == -1) {
                sprintf(rs->logs, "can't get bits per word"); 
                print_f(rs->plogs, "P4", rs->logs);
            }

            len = 0;

            msync(rs->pmch, sizeof(struct machineCtrl_s), MS_SYNC);            
            *tx16 = pkg_info(&rs->pmch->cur);
            len = mtx_data_16(rs->spifd, rx16, tx16, 1, 2, 1024);
            if (len > 0) {
                in16 = rx16[0];

                //sprintf(rs->logs, "16Bits send: 0x%.4x, get: 0x%.4x\n", tx16[0], in16);
                //print_f(rs->plogs, "P4", rs->logs);

                abs_info(&rs->pmch->get, in16);
                rs_ipc_put(rs, "C", 1);
            }
            sprintf(rs->logs, "16Bits send: 0x%.4x, get: 0x%.4x, len: %d\n", *tx16, in16, len);
            print_f(rs->plogs, "P4", rs->logs);
        } else if (cmode == 3) {
            int size=0, opsz;
            char *addr;
            pi = 0;
            size = ring_buf_cons_dual(rs->pdataRx, &addr, pi);
            if (size >= 0) {
                //sprintf(rs->logs, "cons 0x%x %d %d \n", addr, size, pi);
                //print_f(rs->plogs, "P4", rs->logs);
                 
                msync(addr, size, MS_SYNC);

#if OUT_SAVE
                fout = fopen(fileout, "a+");
                if (!fout) {
                    sprintf(rs->logs, "file read [%s] failed \n", fileout);
                    print_f(rs->plogs, "P4", rs->logs);
                } else {
                    sprintf(rs->logs, "file read [%s] ok \n", fileout);
                    print_f(rs->plogs, "P4", rs->logs);

                    slen = fwrite(addr, 1, size, fout);

                    fflush(fout);
                    fclose(fout);
                    fout = NULL;

                }
#endif

                /* send data to wifi socket */
                //opsz = write(rs->psocket_t->connfd, addr, size);
                opsz = mtx_data(rs->spifd, NULL, addr, 1, size, 1024*1024);
                //printf("socket tx %d %d\n", rs->psocket_r->connfd, opsz);

                sprintf(rs->logs, "%d/%d, %d\n", opsz, size, slen);
                print_f(rs->plogs, "P4", rs->logs);

                pi+=2;

                rs_ipc_put(rs, "r", 1);
            } else {
                sprintf(rs->logs, "socket tx %d %d - end\n", opsz, size);
                print_f(rs->plogs, "P4", rs->logs);
                rs_ipc_put(rs, "e", 1);
            }
        } else if (cmode == 4) {
            bitset = 0;
            ioctl(rs->spifd, _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
            sprintf(rs->logs, "Check if RDY pin == 1 (pin:%d)\n", bitset);
            print_f(rs->plogs, "P4", rs->logs);

            while (1) {
                bitset = 0;
                ioctl(rs->spifd, _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
                //sprintf(rs->logs, "Get RDY pin: %d\n", bitset);
                //print_f(rs->plogs, "P4", rs->logs);

                if (bitset == 1) break;
            }
            if (bitset) rs_ipc_put(rs, "B", 1);
        } else if (cmode == 5) {
#if MSP_P4_SAVE_DAT
                rs->fdat_s = 0;
                if (pf->rtops ==  OP_SDWT) {
                    ret = file_save_get(&rs->fdat_s, "/mnt/mmc2/tx/%d.dat");
                    /*
                    if (ret) {
                        sprintf(rs->logs, "get tx log data file error - %d, hold here\n", ret);
                        print_f(rs->plogs, "P4", rs->logs);         
                        while(1);
                    } else {
                        sprintf(rs->logs, "get tx log data file ok - %d, f: %x\n", ret, rs->fdat_s);
                        print_f(rs->plogs, "P4", rs->logs);         
                    }
                    */
                }
#endif
            starts = rs->pmch->sdst.n * SEC_LEN;
            acusz = rs->pmch->sdln.n * SEC_LEN;

            sprintf(rs->logs, "offset:%d, len: %d (rtop: 0x%x)\n", starts, acusz, pf->rtops);
            print_f(rs->plogs, "P4", rs->logs);

            rs->pmch->sdln.f = 0;
            rs->pmch->sdst.f = 0;

            if (acusz > rs->pmch->fdsk.rtMax) {
                sprintf(rs->logs, "OP_SDAT failed due to size > max, len:%d,mx:%d\n", acusz, rs->pmch->fdsk.rtMax);
                print_f(rs->plogs, "P4", rs->logs);

                rs_ipc_put(rs, "s", 1);
                continue;
            }


            if (!rs->pmch->fdsk.sdt) {
                sprintf(rs->logs, "OP_SDAT failed due to input buff == null\n");
                print_f(rs->plogs, "P4", rs->logs);

                rs_ipc_put(rs, "s", 1);
                continue;
            }

            tx_buff = rs->pmch->fdsk.sdt + starts;
            msync(tx_buff, acusz, MS_SYNC);
            //shmem_dump(tx_buff, acusz);

            pi = 0;
            while (acusz>0) {
                if (acusz > SPI_TRUNK_SZ) {
                    slen = SPI_TRUNK_SZ;
                    acusz -= slen;
                } else {
                    slen = acusz;
                    acusz = 0;
                }

                if (pf->rtops ==  OP_SDWT) {
                    len = mtx_data(rs->spifd, tx_buff, rx_buff, 1, slen, 1024*1024);
                    //shmem_dump(tx_buff, len);
#if OUT_SAVE
                fout = fopen(fileout, "a+");
                if (!fout) {
                    sprintf(rs->logs, "file read [%s] failed \n", fileout);
                    print_f(rs->plogs, "P4", rs->logs);
                } else {
                    sprintf(rs->logs, "file read [%s] ok \n", fileout);
                    print_f(rs->plogs, "P4", rs->logs);

                    slen = fwrite(tx_buff, 1, slen, fout);

                    fflush(fout);
                    fclose(fout);
                    fout = NULL;

                }
#endif
                }
                else if (pf->rtops ==  OP_SDRD) {
                    len = mtx_data(rs->spifd, rx_buff, tx_buff, 1, slen, 1024*1024);                
                }
                else {
                    len = 0;
                }

                sprintf(rs->logs, "%d.Send %d/%d bytes!!(rtop: 0x%x)\n", pi, len, slen, pf->rtops);
                print_f(rs->plogs, "P4", rs->logs);

#if MSP_P4_SAVE_DAT
                if (rs->fdat_s) {
                    msync(tx_buff, slen, MS_SYNC);
                    fwrite(tx_buff, 1, slen, rs->fdat_s);
                }
#endif
                pi++;
                tx_buff += len;
            }

            rs_ipc_put(rs, "S", 1);

#if MSP_P4_SAVE_DAT
            if (rs->fdat_s) {
                fflush(rs->fdat_s);
                fclose(rs->fdat_s);
            }
#endif
            rs->fdat_s = 0;
        }
        else if (cmode == 6) {
        
            totsz = rs->pmch->cur.info;
            sprintf(rs->logs, "get total size:%d\n", totsz);
            print_f(rs->plogs, "P4", rs->logs);

            acusz = 0;

            while (1) {
                len = ring_buf_get(rs->pdataTx, &addr);

                if (len > 0) {
                    memset(addr, 0xaa, len);
                
                    if (totsz < len) {
                        len = totsz;
                    }

                    msync(addr, len, MS_SYNC);
                    opsz = mtx_data(rs->spifd, addr, addr, 1, len, 1024*1024);  


                    if (opsz <= 0) {
                        sprintf(rs->logs, "ERROR!!! tx %d/%d acusz:%d\n", opsz, len, acusz);
                        print_f(rs->plogs, "P4", rs->logs);
                        break;
                    }
                    
                    acusz += opsz;
                    totsz -= opsz;
                    
                    sprintf(rs->logs, "t %d / %d\n", opsz, totsz);
                    print_f(rs->plogs, "P4", rs->logs);

                    if (!totsz) break;

                    ring_buf_prod(rs->pdataTx);
                
                    rs_ipc_put(rs, "k", 1);          
                }
            }

            totsz = rs->pmch->cur.info;
            rs->pmch->get.info = acusz;

            ring_buf_prod(rs->pdataTx);
            ring_buf_set_last(rs->pdataTx, opsz);
            rs_ipc_put(rs, "k", 1);          
            rs_ipc_put(rs, "K", 1);                      
            sprintf(rs->logs, "tx acusz:%d totsz:%d - end\n", acusz, totsz);
            print_f(rs->plogs, "P4", rs->logs);

        }
        else if (cmode == 7) {
            clock_gettime(CLOCK_REALTIME, rs->tm[0]);
            totsz = 0;
            pi = 0;
            while (1) {
                ret = ring_buf_cons(rs->pdataTx, &addr, &len);
                if ((ret == 0) ||(ret == -2)) {
                    if (len > 0) {
                        pi++;                    
                        msync(addr, len, MS_SYNC);
                        #if 1 /*debug*/
                        opsz = mtx_data(rs->spifd, addr, addr, 1, len, 1024*1024);  
                        totsz += opsz;
                        #else
                        opsz = len;
                        #endif
                        sprintf(rs->logs, "t %d/%d \n", opsz, totsz);
                        print_f(rs->plogs, "P4", rs->logs);        
                        memset(addr, 0x95, len);
                    }

                    if (ret == -2) {
                        sprintf(rs->logs, "send len:%d cnt:%d total:%d -loop end\n", opsz, pi, totsz);
                        print_f(rs->plogs, "P4", rs->logs);         
                        break;
                    }
                } else {
                    sprintf(rs->logs, "wait for buffer, ret:%d\n", ret);
                    print_f(rs->plogs, "P4", rs->logs);         
                }

                if (ch != 'I') {
                    ch = 0;
                    rs_ipc_get(rs, &ch, 1);
                }
            }

            clock_gettime(CLOCK_REALTIME, rs->tm[1]);

            tdiff = time_diff(rs->tm[0], rs->tm[1], 1000);

            rs->pmch->get.info = totsz;
            
            flsize = totsz;
            fltime = tdiff;
            sprintf(rs->logs, "time:%d us, totsz:%d bytes, thoutghput: %f MBits\n", tdiff, totsz, (flsize*8)/fltime);
            print_f(rs->plogs, "P4", rs->logs);

            while (ch != 'I') {
                ch = 0;
                rs_ipc_get(rs, &ch, 1);
                sprintf(rs->logs, "%c clr\n", ch);
                print_f(rs->plogs, "P4", rs->logs);         
            }

            rs_ipc_put(rs, "I", 1);
            sprintf(rs->logs, "spi send cnt:%d total:%d- end\n", pi, totsz);
            print_f(rs->plogs, "P4", rs->logs);      
        }
        else if (cmode == 8) {
            int bits = 8;
            ret = ioctl(rs->spifd, SPI_IOC_WR_BITS_PER_WORD, &bits);
            if (ret == -1) {
                sprintf(rs->logs, "can't set bits per word"); 
                print_f(rs->plogs, "P4", rs->logs);
            }
            ret = ioctl(rs->spifd, SPI_IOC_RD_BITS_PER_WORD, &bits); 
            if (ret == -1) {
                sprintf(rs->logs, "can't get bits per word"); 
                print_f(rs->plogs, "P4", rs->logs);
            }
            
            totsz = 0;
            len = 0;
            pi = 0;  

            len = 512;
            tx8 = (char *)rs->pmetaout;
            rx8 = (char *)rs->pmetain;
            
            msync(tx8, 512, MS_SYNC);
            opsz = 0;

            opsz = mtx_data(rs->spifd, rx8, tx8, 1, len, 1024*1024);  

            sprintf(rs->logs, "spi0 recv %d\n", opsz);
            print_f(rs->plogs, "P4", rs->logs);

            //shmem_dump(rx8, opsz);

            msync(pmeta, 512, MS_SYNC);
            
            sprintf(rs->logs, "meta get magic number: 0x%.2x 0x%.2x \n", pmeta->ASP_MAGIC[0], pmeta->ASP_MAGIC[1]);
            print_f(rs->plogs, "P4", rs->logs);

            if (opsz < 0) {
                sprintf(rs->logs, "opsz:%d ERROR!!!\n", opsz);
                print_f(rs->plogs, "P4", rs->logs);    
            }

            rs_ipc_put(rs, "J", 1);
            pi += 1;

            totsz += opsz;

            sprintf(rs->logs, "totsz: %d, len:%d opsz:%d break!\n", totsz, len, opsz);
            print_f(rs->plogs, "P4", rs->logs);
        }
        else if (cmode == 9) {
            int bits = 8;
            ret = ioctl(rs->spifd, SPI_IOC_WR_BITS_PER_WORD, &bits);
            if (ret == -1) {
                sprintf(rs->logs, "can't set bits per word"); 
                print_f(rs->plogs, "P4", rs->logs);
            }
            ret = ioctl(rs->spifd, SPI_IOC_RD_BITS_PER_WORD, &bits); 
            if (ret == -1) {
                sprintf(rs->logs, "can't get bits per word"); 
                print_f(rs->plogs, "P4", rs->logs);
            }
            
            totsz = 0;
            len = 0;
            pi = 0;  

            len = rs->pmetaMass->massUsed;
            tx8 = (char *)rs->pmetaMass->masspt;
            rx8 = (char *)rx_buff;
            
            msync(tx8, len, MS_SYNC);
            opsz = 0;

            opsz = mtx_data(rs->spifd, rx8, tx8, 1, len, 1024*1024);  

            sprintf(rs->logs, "spi0 recv %d\n", opsz);
            print_f(rs->plogs, "P4", rs->logs);

            //shmem_dump(rx8, opsz);

            //msync(pmeta, 512, MS_SYNC);
            
            //sprintf(rs->logs, "meta get magic number: 0x%.2x 0x%.2x \n", pmeta->ASP_MAGIC[0], pmeta->ASP_MAGIC[1]);
            //print_f(rs->plogs, "P4", rs->logs);

            if (opsz < 0) {
                sprintf(rs->logs, "opsz:%d ERROR!!!\n", opsz);
                print_f(rs->plogs, "P4", rs->logs);    
            }

            rs_ipc_put(rs, "M", 1);
            pi += 1;

            totsz += opsz;

            sprintf(rs->logs, "totsz: %d, len:%d opsz:%d break!\n", totsz, len, opsz);
            print_f(rs->plogs, "P4", rs->logs);
        }
    }

    p4_end(rs);
    return 0;
}

static int p5(struct procRes_s *rs, struct procRes_s *rcmd)
{
    char ch, *tx, *rx, *addr=0;
    char *tx8, *rx8;
    uint16_t *tx16, *rx16;
    int len, cmode, ret, pi=1;
    uint32_t bitset;
    int totsz=0, opsz=0;
    struct aspMetaData_s *pmetaduo;

    pmetaduo = rs->pmetainduo;

    sprintf(rs->logs, "p5\n");
    print_f(rs->plogs, "P5", rs->logs);

    p5_init(rs);

    rs_ipc_put(rcmd, "poll", 4);

    tx = malloc(SPI_TRUNK_SZ);
    rx = malloc(SPI_TRUNK_SZ);

    int i;
    for (i = 0; i < 64; i+=4) {
        tx[i] = 0xaa;
        tx[i+1] = 0x55; 
        tx[i+2] = 0xff; 
        tx[i+3] = 0xee; 
    }

    tx16 = (uint16_t *)tx;
    rx16 = (uint16_t *)rx;

    while (1) {
        //sprintf(rs->logs, "#\n");
       //print_f(rs->plogs, "P5", rs->logs);

        len = rs_ipc_get(rs, &ch, 1);
        if (len > 0) {
            sprintf(rs->logs, "%c \n", ch);
            print_f(rs->plogs, "P5", rs->logs);

            switch (ch) {
                case 'g':
                    cmode = 1;
                    break;
                case 'c':
                    cmode = 2;
                case 'e':
                    pi = 1;
                case 'r':
                    cmode = 3;
                    break;
                case 'b':
                    cmode = 4;
                    break;
                case 'i':
                    cmode = 5;
                    break;
                case 'j':
                    cmode = 6;
                    break;
                case 'm':
                    cmode = 7;
                    break;
                default:
                    break;
            }
        }

        if (cmode == 1) {
            while (1) {
                bitset = 1;
                ioctl(rs->spifd, _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
                sprintf(rs->logs, "Get RDY pin: %d\n", bitset);
                print_f(rs->plogs, "P5", rs->logs);
                if (bitset == 0) break;
            }
            if (!bitset) rs_ipc_put(rs, "G", 1);
        } else if (cmode == 2) {
            int bits = 16;
            ret = ioctl(rs->spifd, SPI_IOC_WR_BITS_PER_WORD, &bits);
            if (ret == -1) {
                sprintf(rs->logs, "can't set bits per word"); 
                print_f(rs->plogs, "P5", rs->logs);
            }
            ret = ioctl(rs->spifd, SPI_IOC_RD_BITS_PER_WORD, &bits); 
            if (ret == -1) {
                sprintf(rs->logs, "can't get bits per word"); 
                print_f(rs->plogs, "P5", rs->logs);
            }

            len = 0;
            len = mtx_data_16(rs->spifd, rx16, tx16, 1, 1, 64);
            if (len > 0) rs_ipc_put(rs, "C", 1);
            sprintf(rs->logs, "16Bits get: %.8x\n", *rx16);
            print_f(rs->plogs, "P5", rs->logs);
        } else if (cmode == 3) {
            int size=0, opsz;
            char *addr;
            size = ring_buf_cons_dual(rs->pdataRx, &addr, pi);
            if (size >= 0) {
                //sprintf(rs->logs, "cons 0x%x %d %d \n", addr, size, pi);
                //print_f(rs->plogs, "P5", rs->logs);
                 
                msync(addr, size, MS_SYNC);
                /* send data to wifi socket */
                //opsz = write(rs->psocket_t->connfd, addr, size);
                opsz = mtx_data(rs->spifd, NULL, addr, 1, size, 1024*1024);
                //printf("socket tx %d %d\n", rs->psocket_r->connfd, opsz);
                sprintf(rs->logs, "%d/%d\n", opsz, size);
                print_f(rs->plogs, "P5", rs->logs);

                pi+=2;
                rs_ipc_put(rs, "r", 1);
            } else {
                sprintf(rs->logs, "socket tx %d %d - end\n", opsz, size);
                print_f(rs->plogs, "P5", rs->logs);
                rs_ipc_put(rs, "e", 1);
            }
        } else if (cmode == 4) {
            bitset = 0;
            ioctl(rs->spifd, _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
            sprintf(rs->logs, "Check if RDY pin == 1 (pin:%d)\n", bitset);
            print_f(rs->plogs, "P5", rs->logs);

            while (1) {
                bitset = 0;
                ioctl(rs->spifd, _IOR(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_RD_CTL_PIN
                //sprintf(rs->logs, "Get RDY pin: %d\n", bitset);
                //print_f(rs->plogs, "P4", rs->logs);

                if (bitset == 1) break;
            }
            if (bitset) rs_ipc_put(rs, "B", 1);
        }
        else if (cmode == 5) {
            totsz = 0;
            pi = 0;
            while (1) {
                ret = ring_buf_cons(rs->pcmdTx, &addr, &len);
                if ((ret == 0) ||(ret == -2)) {
                    if (len > 0) {
                        pi++;                    
                        msync(addr, len, MS_SYNC);
                        #if ENABLE_SPI1 /*debug*/
                        opsz = mtx_data(rs->spifd, addr, addr, 1, len, 1024*1024);  
                        totsz += opsz;
                        #else
                        opsz = len;
                        #endif
                        sprintf(rs->logs, "t %d/%d \n", opsz, totsz);
                        print_f(rs->plogs, "P5", rs->logs);        
                        memset(addr, 0x95, len);
                    }

                    if (ret == -2) {
                        sprintf(rs->logs, "send len:%d cnt:%d total:%d -loop end\n", opsz, pi, totsz);
                        print_f(rs->plogs, "P5", rs->logs);         
                        break;
                    }
                } else {
                    sprintf(rs->logs, "wait for buffer, ret:%d\n", ret);
                    print_f(rs->plogs, "P5", rs->logs);         
                }

                if (ch != 'I') {
                    ch = 0;
                    rs_ipc_get(rs, &ch, 1);
                }
            }

            while (ch != 'I') {
                ch = 0;
                rs_ipc_get(rs, &ch, 1);
                sprintf(rs->logs, "%c clr\n", ch);
                print_f(rs->plogs, "P5", rs->logs);         
            }

            rs_ipc_put(rs, "I", 1);
            sprintf(rs->logs, "spi send cnt:%d total:%d- end\n", pi, totsz);
            print_f(rs->plogs, "P5", rs->logs);      
        }
        else if (cmode == 6) {
            int bits = 8;
            ret = ioctl(rs->spifd, SPI_IOC_WR_BITS_PER_WORD, &bits);
            if (ret == -1) {
                sprintf(rs->logs, "can't set bits per word"); 
                print_f(rs->plogs, "P5", rs->logs);
            }
            ret = ioctl(rs->spifd, SPI_IOC_RD_BITS_PER_WORD, &bits); 
            if (ret == -1) {
                sprintf(rs->logs, "can't get bits per word"); 
                print_f(rs->plogs, "P5", rs->logs);
            }
            
            totsz = 0;
            len = 0;
            pi = 0;  

            len = 512;
            tx8 = (char *)rs->pmetaoutduo;
            rx8 = (char *)rs->pmetainduo;
            
            msync(tx8, 512, MS_SYNC);
            opsz = 0;

            opsz = mtx_data(rs->spifd, rx8, tx8, 1, len, 1024*1024);  

            sprintf(rs->logs, "spi0 recv %d\n", opsz);
            print_f(rs->plogs, "P5", rs->logs);

            //shmem_dump(rx8, opsz);

            msync(pmetaduo, 512, MS_SYNC);
            
            sprintf(rs->logs, "meta get magic number: 0x%.2x 0x%.2x \n", pmetaduo->ASP_MAGIC[0], pmetaduo->ASP_MAGIC[1]);
            print_f(rs->plogs, "P5", rs->logs);

            if (opsz < 0) {
                sprintf(rs->logs, "opsz:%d ERROR!!!\n", opsz);
                print_f(rs->plogs, "P5", rs->logs);    
            }

            rs_ipc_put(rs, "J", 1);
            pi += 1;

            totsz += opsz;

            sprintf(rs->logs, "totsz: %d, len:%d opsz:%d break!\n", totsz, len, opsz);
            print_f(rs->plogs, "P5", rs->logs);
        }
        else if (cmode == 7) {
            int bits = 8;
            ret = ioctl(rs->spifd, SPI_IOC_WR_BITS_PER_WORD, &bits);
            if (ret == -1) {
                sprintf(rs->logs, "can't set bits per word"); 
                print_f(rs->plogs, "P5", rs->logs);
            }
            ret = ioctl(rs->spifd, SPI_IOC_RD_BITS_PER_WORD, &bits); 
            if (ret == -1) {
                sprintf(rs->logs, "can't get bits per word"); 
                print_f(rs->plogs, "P5", rs->logs);
            }
            
            totsz = 0;
            len = 0;
            pi = 0;  

            len = rs->pmetaMassduo->massUsed;
            tx8 = (char *)rs->pmetaMassduo->masspt;
            rx8 = (char *)rx;
            
            msync(tx8, len, MS_SYNC);
            opsz = 0;

            opsz = mtx_data(rs->spifd, rx8, tx8, 1, len, 1024*1024);  

            sprintf(rs->logs, "spi0 recv %d\n", opsz);
            print_f(rs->plogs, "P5", rs->logs);

            //shmem_dump(rx8, opsz);

            //msync(pmeta, 512, MS_SYNC);
            
            //sprintf(rs->logs, "meta get magic number: 0x%.2x 0x%.2x \n", pmeta->ASP_MAGIC[0], pmeta->ASP_MAGIC[1]);
            //print_f(rs->plogs, "P4", rs->logs);

            if (opsz < 0) {
                sprintf(rs->logs, "opsz:%d ERROR!!!\n", opsz);
                print_f(rs->plogs, "P5", rs->logs);    
            }

            rs_ipc_put(rs, "M", 1);
            pi += 1;

            totsz += opsz;

            sprintf(rs->logs, "totsz: %d, len:%d opsz:%d break!\n", totsz, len, opsz);
            print_f(rs->plogs, "P5", rs->logs);
        }
    }

    p5_end(rs);
    return 0;
}

int main(int argc, char *argv[])
{
//char diskname[128] = "/mnt/mmc2/disk_rmImg.bin";
//char diskname[128] = "/mnt/mmc2/disk_rx127_255log.bin";
//char diskname[128] = "/mnt/mmc2/disk_golden.bin";
//char diskname[128] = "/mnt/mmc2/debug_fat.bin";
//char diskname[128] = "/mnt/mmc2/disk_05.bin";
//char diskname[128] = "/dev/mmcblk0p1";
//char diskname[128] = "/dev/mmcblk0";
//char diskname[128] = "/mnt/mmc2/empty_256.dsk";
//char diskname[128] = "/mnt/mmc2/folder_256.dsk";
//char diskname[128] = "/mnt/mmc2/disk_LFN_64.bin";
//char diskname[128] = "/mnt/mmc2/disk_onefolder.bin";
//char diskname[128] = "/mnt/mmc2/mingssd.bin";
//char diskname[128] = "/mnt/mmc2/16g_ghost.bin";
//char diskname[128] = "/mnt/mmc2/32g_beformat_rmmore.bin";
//char diskname[128] = "/mnt/mmc2/32g_32k_format.bin";
//char diskname[128] = "/mnt/mmc2/32g_64k_format.bin";
//char diskname[128] = "/mnt/mmc2/32g_64k_format_add_01_rm.bin";
//char diskname[128] = "/mnt/mmc2/32g_64k_format_add_01.bin";
//char diskname[128] = "/mnt/mmc2/64G_2pics.bin";
//char diskname[128] = "/mnt/mmc2/64G_empty.bin";
//char diskname[128] = "/mnt/mmc2/32g_ios_format.bin";
//char diskname[128] = "/mnt/mmc2/128g_ios_format.bin";
//char diskname[128] = "/mnt/mmc2/32g_and.bin";
//char diskname[128] = "/mnt/mmc2/32g_win.bin";
char diskname[128] = "/mnt/mmc2/32g_mass.bin";
//char diskname[128] = "/mnt/mmc2/phy_32g_empty_diff.bin";
//char diskname[128] = "/mnt/mmc2/phy_folder.bin";
static char spi1[] = "/dev/spidev32766.0"; 
static char spi0[] = "/dev/spidev32765.0"; 

    struct mainRes_s *pmrs;
    struct procRes_s rs[7];
    int ix=0, ret=0, len=0;
    char *log;
    int tdiff, speed;
    int arg[8];
    uint32_t bitset;

    pmrs = (struct mainRes_s *)mmap(NULL, sizeof(struct mainRes_s), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);;

    if (argc > 1) {
        sprintf(pmrs->filein, "%s", argv[1]);
    } else {
        pmrs->filein[0] = '\0';
    }
    printf("Get file input: [%s] \n", pmrs->filein);

    infpath = pmrs->filein;

    pmrs->plog.max = 6*1024*1024;
    pmrs->plog.pool = mmap(NULL, pmrs->plog.max, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    if (!pmrs->plog.pool) {printf("get log pool share memory failed\n"); return 0;}
    mlogPool = &pmrs->plog;
    pmrs->plog.len = 0;
    pmrs->plog.cur = pmrs->plog.pool;

    ret = file_save_get(&pmrs->flog, "/mnt/mmc2/rx/%d.log");
    if (ret) {printf("get log file failed\n"); return 0;}
    mlog = pmrs->flog;
    ret = fwrite("test file write \n", 1, 16, pmrs->flog);
    sprintf(pmrs->log, "write file size: %d/%d\n", ret, 16);
    print_f(&pmrs->plog, "fwrite", pmrs->log);
    fflush(pmrs->flog);

    sprintf(pmrs->log, "argc:%d\n", argc);
    print_f(&pmrs->plog, "main", pmrs->log);
// show arguments
    ix = 0;
    while(argc) {
        arg[ix] = atoi(argv[ix]);
        sprintf(pmrs->log, "%d %d %s\n", ix, arg[ix], argv[ix]);
        print_f(&pmrs->plog, "arg", pmrs->log);
        ix++;
        argc--;
        if (ix > 7) break;
    }
// initial share parameter
    len = sizeof(struct aspMetaData_s);
    pmrs->metaout = aspSalloc(len);
    pmrs->metain = aspSalloc(len);
    
    len = SPI_TRUNK_SZ;
    pmrs->metaMass.masspt = aspSalloc(len);    
    pmrs->metaMass.massMax = len;
    pmrs->metaMass.massUsed = 0;

    if ((pmrs->metaout) && (pmrs->metain) && (pmrs->metaMass.masspt)) {
        sprintf(pmrs->log, "inbuff addr(0x%.8x), outbuff addr(0x%.8x), massbuff addr(0x%.8x) \n", pmrs->metain, pmrs->metaout, pmrs->metaMass.masspt);
        print_f(&pmrs->plog, "meta", pmrs->log);
    } else {
        sprintf(pmrs->log, "Error!! allocate meta memory failed!!!! \n");
        print_f(&pmrs->plog, "meta", pmrs->log);
    }

    len = sizeof(struct aspMetaData_s);
    pmrs->metaoutDuo= aspSalloc(len);
    pmrs->metainDuo= aspSalloc(len);
    
    len = SPI_TRUNK_SZ;
    pmrs->metaMassDuo.masspt = aspSalloc(len);    
    pmrs->metaMassDuo.massMax = len;
    pmrs->metaMassDuo.massUsed = 0;

    if ((pmrs->metaoutDuo) && (pmrs->metainDuo) && (pmrs->metaMassDuo.masspt)) {
        sprintf(pmrs->log, "duo inbuff addr(0x%.8x), outbuff addr(0x%.8x), massbuff addr(0x%.8x) \n", pmrs->metainDuo, pmrs->metaoutDuo, pmrs->metaMassDuo.masspt);
        print_f(&pmrs->plog, "meta", pmrs->log);
    } else {
        sprintf(pmrs->log, "Error!! allocate duo meta memory failed!!!! \n");
        print_f(&pmrs->plog, "meta", pmrs->log);
    }
    
    /* data mode rx from spi */
    clock_gettime(CLOCK_REALTIME, &pmrs->time[0]);
    pmrs->dataRx.pp = memory_init(&pmrs->dataRx.slotn, 4096*SPI_TRUNK_SZ, SPI_TRUNK_SZ);
    if (!pmrs->dataRx.pp) goto end;
    pmrs->dataRx.r = (struct ring_p *)mmap(NULL, sizeof(struct ring_p), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    pmrs->dataRx.totsz = 4096*SPI_TRUNK_SZ;
    pmrs->dataRx.chksz = SPI_TRUNK_SZ;
    pmrs->dataRx.svdist = 16;
    //sprintf(pmrs->log, "totsz:%d pp:0x%.8x\n", pmrs->dataRx.totsz, pmrs->dataRx.pp);
    //print_f(&pmrs->plog, "minit_result", pmrs->log);
    //for (ix = 0; ix < pmrs->dataRx.slotn; ix++) {
    //    sprintf(pmrs->log, "[%d] 0x%.8x\n", ix, pmrs->dataRx.pp[ix]);
    //    print_f(&pmrs->plog, "shminit_result", pmrs->log);
    //}
    clock_gettime(CLOCK_REALTIME, &pmrs->time[1]);
    tdiff = time_diff(&pmrs->time[0], &pmrs->time[1], 1000);
    sprintf(pmrs->log, "tdiff:%d \n", tdiff);
    print_f(&pmrs->plog, "time_diff", pmrs->log);

    /* data mode tx to spi */
    clock_gettime(CLOCK_REALTIME, &pmrs->time[0]);
    pmrs->dataTx.pp = memory_init(&pmrs->dataTx.slotn, 256*SPI_TRUNK_SZ, SPI_TRUNK_SZ);
    if (!pmrs->dataTx.pp) goto end;
    pmrs->dataTx.r = (struct ring_p *)mmap(NULL, sizeof(struct ring_p), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    pmrs->dataTx.totsz = 256*SPI_TRUNK_SZ;
    pmrs->dataTx.chksz = SPI_TRUNK_SZ;
    pmrs->dataTx.svdist = 8;
    //sprintf(pmrs->log, "totsz:%d pp:0x%.8x\n", pmrs->dataTx.totsz, pmrs->dataTx.pp);
    //print_f(&pmrs->plog, "minit_result", pmrs->log);
    //for (ix = 0; ix < pmrs->dataTx.slotn; ix++) {
    //    sprintf(pmrs->log, "[%d] 0x%.8x\n", ix, pmrs->dataTx.pp[ix]);
    //    print_f(&pmrs->plog, "shminit_result", pmrs->log);
    //}
    clock_gettime(CLOCK_REALTIME, &pmrs->time[1]);
    tdiff = time_diff(&pmrs->time[0], &pmrs->time[1], 1000);
    sprintf(pmrs->log, "tdiff:%d \n", tdiff);
    print_f(&pmrs->plog, "time_diff", pmrs->log);

    /* cmd mode rx from spi */
    clock_gettime(CLOCK_REALTIME, &pmrs->time[0]);
    pmrs->cmdRx.pp = memory_init(&pmrs->cmdRx.slotn, 256*SPI_TRUNK_SZ, SPI_TRUNK_SZ);
    if (!pmrs->cmdRx.pp) goto end;
    pmrs->cmdRx.r = (struct ring_p *)mmap(NULL, sizeof(struct ring_p), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    pmrs->cmdRx.totsz = 256*SPI_TRUNK_SZ;;
    pmrs->cmdRx.chksz = SPI_TRUNK_SZ;
    pmrs->cmdRx.svdist = 8;
    //sprintf(pmrs->log, "totsz:%d pp:0x%.8x\n", pmrs->cmdRx.totsz, pmrs->cmdRx.pp);
    //print_f(&pmrs->plog, "minit_result", pmrs->log);
    //for (ix = 0; ix < pmrs->cmdRx.slotn; ix++) {
    //    sprintf(pmrs->log, "[%d] 0x%.8x\n", ix, pmrs->cmdRx.pp[ix]);
    //    print_f(&pmrs->plog, "shminit_result", pmrs->log);
    //}
    clock_gettime(CLOCK_REALTIME, &pmrs->time[1]);
    tdiff = time_diff(&pmrs->time[0], &pmrs->time[1], 1000);
    sprintf(pmrs->log, "tdiff:%d \n", tdiff);
    print_f(&pmrs->plog, "time_diff", pmrs->log);
	
    /* cmd mode tx to spi */
    clock_gettime(CLOCK_REALTIME, &pmrs->time[0]);
    pmrs->cmdTx.pp = memory_init(&pmrs->cmdTx.slotn, 256*SPI_TRUNK_SZ, SPI_TRUNK_SZ);
    if (!pmrs->cmdTx.pp) goto end;
    pmrs->cmdTx.r = (struct ring_p *)mmap(NULL, sizeof(struct ring_p), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    pmrs->cmdTx.totsz = 256*SPI_TRUNK_SZ;
    pmrs->cmdTx.chksz = SPI_TRUNK_SZ;
    pmrs->cmdTx.svdist = 8;
    //sprintf(pmrs->log, "totsz:%d pp:0x%.8x\n", pmrs->cmdTx.totsz, pmrs->cmdTx.pp);
    //print_f(&pmrs->plog, "minit_result", pmrs->log);
    //for (ix = 0; ix < pmrs->cmdTx.slotn; ix++) {
    //    sprintf(pmrs->log, "[%d] 0x%.8x\n", ix, pmrs->cmdTx.pp[ix]);
    //    print_f(&pmrs->plog, "shminit_result", pmrs->log);
    //}
    clock_gettime(CLOCK_REALTIME, &pmrs->time[1]);
    tdiff = time_diff(&pmrs->time[0], &pmrs->time[1], 1000);
    sprintf(pmrs->log, "tdiff:%d \n", tdiff);
    print_f(&pmrs->plog, "time_diff", pmrs->log);

    pmrs->mchine.fdsk.vsd = fopen(diskname, "r");
    if (pmrs->mchine.fdsk.vsd == NULL) {
        sprintf(pmrs->log, "disk file [%s] open failed!!! \n", diskname);
        print_f(&pmrs->plog, "FDISK", pmrs->log);
        goto end;
    }

    ret = fseek(pmrs->mchine.fdsk.vsd, 0, SEEK_END);
    if (ret) {
        sprintf(pmrs->log, "seek file [%s] failed!!! \n", diskname);
        print_f(&pmrs->plog, "FDISK", pmrs->log);
        goto end;
    }

    pmrs->mchine.fdsk.rtMax = ftell(pmrs->mchine.fdsk.vsd);
    //pmrs->mchine.fdsk.rtMax = -1;
    if (!pmrs->mchine.fdsk.rtMax) {
        sprintf(pmrs->log, "get file [%s] size failed!!! \n", diskname);
        print_f(&pmrs->plog, "FDISK", pmrs->log);
        goto end;
    } else if (pmrs->mchine.fdsk.rtMax < 0) {
        pmrs->mchine.fdsk.rtMax = 16 * 1024 * 1024;
        sprintf(pmrs->log, "size is unknown, set file [%s] size : %d!!! \n", diskname, pmrs->mchine.fdsk.rtMax);
        print_f(&pmrs->plog, "FDISK", pmrs->log);
        goto end;
    }else {
        sprintf(pmrs->log, "get file [%s] size : %d!!! \n", diskname, pmrs->mchine.fdsk.rtMax);
        print_f(&pmrs->plog, "FDISK", pmrs->log);
    }

    pmrs->mchine.fdsk.sdt = mmap(NULL, pmrs->mchine.fdsk.rtMax, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);;
    if (!pmrs->mchine.fdsk.sdt) {
        sprintf(pmrs->log, "memory alloc for [%s] size : %d, failed!!! \n", diskname, pmrs->mchine.fdsk.rtMax);
        print_f(&pmrs->plog, "FDISK", pmrs->log);
        goto end;
    }

    ret = fseek(pmrs->mchine.fdsk.vsd, 0, SEEK_SET);
    if (ret) {
        sprintf(pmrs->log, "seek for [%s] to 0 failed!!! \n", diskname);
        print_f(&pmrs->plog, "FDISK", pmrs->log);
        goto end;
    }

    ret = fread(pmrs->mchine.fdsk.sdt, 1, pmrs->mchine.fdsk.rtMax, pmrs->mchine.fdsk.vsd);
    sprintf(pmrs->log, "read file size: %d/%d \n", ret, pmrs->mchine.fdsk.rtMax);
    print_f(&pmrs->plog, "FDISK", pmrs->log);

    fclose(pmrs->mchine.fdsk.vsd);
    pmrs->mchine.fdsk.vsd = 0;

    ret = file_save_get(&pmrs->fs, "/mnt/mmc2/rx/%d.bin");
    if (ret) {printf("get save file failed\n"); return 0;}
    //ret = fwrite("test file write \n", 1, 16, pmrs->fs);
    sprintf(pmrs->log, "write file size: %d/%d\n", ret, 16);
    print_f(&pmrs->plog, "fwrite", pmrs->log);

    // crop coordinates preset 
#if RANDOM_CROP_COORD
    pmrs->cropCoord.CROP_COOD_01[0] = 1368;
    pmrs->cropCoord.CROP_COOD_01[1] = 200;

    pmrs->cropCoord.CROP_COOD_02[0] = 1476;
    pmrs->cropCoord.CROP_COOD_02[1] = 1464;

    pmrs->cropCoord.CROP_COOD_03[0] = 1514;
    pmrs->cropCoord.CROP_COOD_03[1] = 1646;

    pmrs->cropCoord.CROP_COOD_04[0] = 2696;
    pmrs->cropCoord.CROP_COOD_04[1] = 1264;
    
    pmrs->cropCoord.CROP_COOD_05[0] = 2677;
    pmrs->cropCoord.CROP_COOD_05[1] = 72;

    pmrs->cropCoord.CROP_COOD_06[0] = 517;
    pmrs->cropCoord.CROP_COOD_06[1] = 72;
#else

#if (CROP_NUMBER == 18)
#if CROP_SAMPLE_SIZE
    pmrs->cropCoord.CROP_COOD_01[0] = crop_01.samples[0].cropx;
    pmrs->cropCoord.CROP_COOD_01[1] = crop_01.samples[0].cropy;
    pmrs->cropCoord.CROP_COOD_02[0] = crop_02.samples[0].cropx;
    pmrs->cropCoord.CROP_COOD_02[1] = crop_02.samples[0].cropy;
    pmrs->cropCoord.CROP_COOD_03[0] = crop_03.samples[0].cropx;
    pmrs->cropCoord.CROP_COOD_03[1] = crop_03.samples[0].cropy;
    pmrs->cropCoord.CROP_COOD_04[0] = crop_04.samples[0].cropx;
    pmrs->cropCoord.CROP_COOD_04[1] = crop_04.samples[0].cropy;
    pmrs->cropCoord.CROP_COOD_05[0] = crop_05.samples[0].cropx;
    pmrs->cropCoord.CROP_COOD_05[1] = crop_05.samples[0].cropy;
    pmrs->cropCoord.CROP_COOD_06[0] = crop_06.samples[0].cropx;
    pmrs->cropCoord.CROP_COOD_06[1] = crop_06.samples[0].cropy;
    pmrs->cropCoord.CROP_COOD_07[0] = crop_07.samples[0].cropx;
    pmrs->cropCoord.CROP_COOD_07[1] = crop_07.samples[0].cropy;
    pmrs->cropCoord.CROP_COOD_08[0] = crop_08.samples[0].cropx;
    pmrs->cropCoord.CROP_COOD_08[1] = crop_08.samples[0].cropy;
    pmrs->cropCoord.CROP_COOD_09[0] = crop_09.samples[0].cropx;
    pmrs->cropCoord.CROP_COOD_09[1] = crop_09.samples[0].cropy;
    pmrs->cropCoord.CROP_COOD_10[0] = crop_10.samples[0].cropx;
    pmrs->cropCoord.CROP_COOD_10[1] = crop_10.samples[0].cropy;
    pmrs->cropCoord.CROP_COOD_11[0] = crop_11.samples[0].cropx;
    pmrs->cropCoord.CROP_COOD_11[1] = crop_11.samples[0].cropy;
    pmrs->cropCoord.CROP_COOD_12[0] = crop_12.samples[0].cropx;
    pmrs->cropCoord.CROP_COOD_12[1] = crop_12.samples[0].cropy;
    pmrs->cropCoord.CROP_COOD_13[0] = crop_13.samples[0].cropx;
    pmrs->cropCoord.CROP_COOD_13[1] = crop_13.samples[0].cropy;
    pmrs->cropCoord.CROP_COOD_14[0] = crop_14.samples[0].cropx;
    pmrs->cropCoord.CROP_COOD_14[1] = crop_14.samples[0].cropy;
    pmrs->cropCoord.CROP_COOD_15[0] = crop_15.samples[0].cropx;
    pmrs->cropCoord.CROP_COOD_15[1] = crop_15.samples[0].cropy;
    pmrs->cropCoord.CROP_COOD_16[0] = crop_16.samples[0].cropx;
    pmrs->cropCoord.CROP_COOD_16[1] = crop_16.samples[0].cropy;
    pmrs->cropCoord.CROP_COOD_17[0] = crop_17.samples[0].cropx;
    pmrs->cropCoord.CROP_COOD_17[1] = crop_17.samples[0].cropy;
    pmrs->cropCoord.CROP_COOD_18[0] = crop_18.samples[0].cropx;
    pmrs->cropCoord.CROP_COOD_18[1] = crop_18.samples[0].cropy;
#else
    pmrs->cropCoord.CROP_COOD_01[0] = crop_01[0];
    pmrs->cropCoord.CROP_COOD_01[1] = crop_01[1];

    pmrs->cropCoord.CROP_COOD_02[0] = crop_02[0];
    pmrs->cropCoord.CROP_COOD_02[1] = crop_02[1];

    pmrs->cropCoord.CROP_COOD_03[0] = crop_03[0];
    pmrs->cropCoord.CROP_COOD_03[1] = crop_03[1];

    pmrs->cropCoord.CROP_COOD_04[0] = crop_04[0];
    pmrs->cropCoord.CROP_COOD_04[1] = crop_04[1];
    
    pmrs->cropCoord.CROP_COOD_05[0] = crop_05[0];
    pmrs->cropCoord.CROP_COOD_05[1] = crop_05[1];

    pmrs->cropCoord.CROP_COOD_06[0] = crop_06[0];
    pmrs->cropCoord.CROP_COOD_06[1] = crop_06[1];

    pmrs->cropCoord.CROP_COOD_07[0] = crop_07[0];
    pmrs->cropCoord.CROP_COOD_07[1] = crop_07[1];

    pmrs->cropCoord.CROP_COOD_08[0] = crop_08[0];
    pmrs->cropCoord.CROP_COOD_08[1] = crop_08[1];

    pmrs->cropCoord.CROP_COOD_09[0] = crop_09[0];
    pmrs->cropCoord.CROP_COOD_09[1] = crop_09[1];

    pmrs->cropCoord.CROP_COOD_10[0] = crop_10[0];
    pmrs->cropCoord.CROP_COOD_10[1] = crop_10[1];
    
    pmrs->cropCoord.CROP_COOD_11[0] = crop_11[0];
    pmrs->cropCoord.CROP_COOD_11[1] = crop_11[1];

    pmrs->cropCoord.CROP_COOD_12[0] = crop_12[0];
    pmrs->cropCoord.CROP_COOD_12[1] = crop_12[1];

    pmrs->cropCoord.CROP_COOD_13[0] = crop_13[0];
    pmrs->cropCoord.CROP_COOD_13[1] = crop_13[1];

    pmrs->cropCoord.CROP_COOD_14[0] = crop_14[0];
    pmrs->cropCoord.CROP_COOD_14[1] = crop_14[1];

    pmrs->cropCoord.CROP_COOD_15[0] = crop_15[0];
    pmrs->cropCoord.CROP_COOD_15[1] = crop_15[1];

    pmrs->cropCoord.CROP_COOD_16[0] = crop_16[0];
    pmrs->cropCoord.CROP_COOD_16[1] = crop_16[1];
    
    pmrs->cropCoord.CROP_COOD_17[0] = crop_17[0];
    pmrs->cropCoord.CROP_COOD_17[1] = crop_17[1];

    pmrs->cropCoord.CROP_COOD_18[0] = crop_18[0];
    pmrs->cropCoord.CROP_COOD_18[1] = crop_18[1];
#endif
#else
    pmrs->cropCoord.CROP_COOD_01[0] = crop_01[0];
    pmrs->cropCoord.CROP_COOD_01[1] = crop_01[1];

    pmrs->cropCoord.CROP_COOD_02[0] = crop_02[0];
    pmrs->cropCoord.CROP_COOD_02[1] = crop_02[1];

    pmrs->cropCoord.CROP_COOD_03[0] = crop_03[0];
    pmrs->cropCoord.CROP_COOD_03[1] = crop_03[1];

    pmrs->cropCoord.CROP_COOD_04[0] = crop_04[0];
    pmrs->cropCoord.CROP_COOD_04[1] = crop_04[1];
    
    pmrs->cropCoord.CROP_COOD_05[0] = crop_05[0];
    pmrs->cropCoord.CROP_COOD_05[1] = crop_05[1];

    pmrs->cropCoord.CROP_COOD_06[0] = crop_06[0];
    pmrs->cropCoord.CROP_COOD_06[1] = crop_06[1];
#endif
#endif

    struct aspConfig_s* ctb = 0;
    ctb = pmrs->configTable;
    sprintf_f(pmrs->log, "Reset configuration!!!");
    print_f(&pmrs->plog, "PRAM", pmrs->log);
    
    for (ix = 0; ix < ASPOP_CODE_MAX; ix++) {
        ctb = &pmrs->configTable[ix];
        switch(ix) {
        case ASPOP_CODE_NONE:   
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = 0;
            ctb->opType = ASPOP_TYPE_NONE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_0;
            ctb->opBitlen = 0;
            break;
        case ASPOP_FILE_FORMAT: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_FFORMAT;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_COLOR_MODE:  
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_COLRMOD;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_3;
            ctb->opBitlen = 8;
            break;
        case ASPOP_COMPRES_RATE:
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_COMPRAT;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_3;
            ctb->opBitlen = 8;
            break;
        case ASPOP_SCAN_SINGLE: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_SINGLE;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_SCAN_DOUBLE: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_DOUBLE;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_ACTION: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_ACTION;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_3;
            ctb->opBitlen = 8;
            break;
        case ASPOP_RESOLUTION:
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_RESOLTN;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_3;
            ctb->opBitlen = 8;
            break;
        case ASPOP_SCAN_GRAVITY:
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_SCANGAV;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_2;
            ctb->opBitlen = 8;
            break;
        case ASPOP_MAX_WIDTH:   
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_MAXWIDH;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_2;
            ctb->opBitlen = 8;
            break;
        case ASPOP_WIDTH_ADJ_H: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_WIDTHAD_H;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_WIDTH_ADJ_L: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_WIDTHAD_L;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_SCAN_LENS_H: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_SCANLEN_H;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_SCAN_LENS_L: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_SCANLEN_L;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_INTER_IMG: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_INTERIMG;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_3;
            ctb->opBitlen = 8;
            break;
        case ASPOP_AFEIC_SEL: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_AFEIC;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_EXT_PULSE: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_EXTPULSE;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_3;
            ctb->opBitlen = 8;
            break;
        case ASPOP_SDFAT_RD: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_SDRD;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_SDFAT_WT: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_SDWT;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_SDFAT_STR01: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_STSEC_00;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_SDFAT_STR02: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_STSEC_01;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_SDFAT_STR03: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_STSEC_02;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_SDFAT_STR04: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_STSEC_03;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_SDFAT_LEN01: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_STLEN_00;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_SDFAT_LEN02: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_STLEN_01;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_SDFAT_LEN03: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_STLEN_02;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_SDFAT_LEN04: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_STLEN_03;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_SDFAT_SDAT: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_SDAT;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_REG_RD: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_RGRD;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_REG_WT: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_RGWT;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_REG_ADDRH: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_RGADD_H;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_REG_ADDRL: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_RGADD_L;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_REG_DAT: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_RGDAT;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_SUP_SAVE: 
            ctb->opStatus = ASPOP_STA_NONE; // for debug, should be ASPOP_STA_NONE
            ctb->opCode = OP_SUPBACK;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff; // for debug, should be 0xff
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_SDFREE_FREESEC: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_FREESEC;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_SDFREE_STR01: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_STSEC_00;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_SDFREE_STR02: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_STSEC_01;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_SDFREE_STR03: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_STSEC_02;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_SDFREE_STR04: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_STSEC_03;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_SDFREE_LEN01: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_STLEN_00;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_SDFREE_LEN02: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_STLEN_01;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_SDFREE_LEN03: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_STLEN_02;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_SDFREE_LEN04: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_STLEN_03;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_SDUSED_USEDSEC: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_USEDSEC;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_SDUSED_STR01: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_STSEC_00;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_SDUSED_STR02: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_STSEC_01;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_SDUSED_STR03: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_STSEC_02;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_SDUSED_STR04: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_STSEC_03;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_SDUSED_LEN01: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_STLEN_00;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_SDUSED_LEN02: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_STLEN_01;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_SDUSED_LEN03: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_STLEN_02;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_SDUSED_LEN04: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_STLEN_03;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_FUNTEST_00: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_FUNCTEST_00;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_FUNTEST_01: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_FUNCTEST_01;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_FUNTEST_02: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_FUNCTEST_02;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_FUNTEST_03: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_FUNCTEST_03;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_FUNTEST_04: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_FUNCTEST_04;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_FUNTEST_05: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_FUNCTEST_05;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_FUNTEST_06: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_FUNCTEST_06;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_FUNTEST_07: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_FUNCTEST_07;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_FUNTEST_08: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_FUNCTEST_08;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_FUNTEST_09: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_FUNCTEST_09;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_FUNTEST_10: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_FUNCTEST_10;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_FUNTEST_11: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_FUNCTEST_11;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_FUNTEST_12: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_FUNCTEST_12;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_FUNTEST_13: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_FUNCTEST_13;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_FUNTEST_14: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_FUNCTEST_14;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_FUNTEST_15: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_FUNCTEST_15;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_BLEEDTHROU_ADJUST: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_BLEEDTHROU_ADJUST;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_BLACKWHITE_THSHLD: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_BLACKWHITE_THSHLD;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_CROP_01: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_CROP_01;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xffffffff;
            ctb->opMask = ASPOP_MASK_32;
            ctb->opBitlen = 32;
            break;
        case ASPOP_CROP_02: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_CROP_02;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xffffffff;
            ctb->opMask = ASPOP_MASK_32;
            ctb->opBitlen = 32;
            break;
        case ASPOP_CROP_03: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_CROP_03;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xffffffff;
            ctb->opMask = ASPOP_MASK_32;
            ctb->opBitlen = 32;
            break;
        case ASPOP_CROP_04: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_CROP_04;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xffffffff;
            ctb->opMask = ASPOP_MASK_32;
            ctb->opBitlen = 32;
            break;
        case ASPOP_CROP_05: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_CROP_05;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xffffffff;
            ctb->opMask = ASPOP_MASK_32;
            ctb->opBitlen = 32;
            break;
        case ASPOP_CROP_06: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_CROP_06;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xffffffff;
            ctb->opMask = ASPOP_MASK_32;
            ctb->opBitlen = 32;
            break;
        case ASPOP_CROP_07: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_CROP_01;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xffffffff;
            ctb->opMask = ASPOP_MASK_32;
            ctb->opBitlen = 32;
            break;
        case ASPOP_CROP_08: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_CROP_02;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xffffffff;
            ctb->opMask = ASPOP_MASK_32;
            ctb->opBitlen = 32;
            break;
        case ASPOP_CROP_09: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_CROP_03;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xffffffff;
            ctb->opMask = ASPOP_MASK_32;
            ctb->opBitlen = 32;
            break;
        case ASPOP_CROP_10: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_CROP_04;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xffffffff;
            ctb->opMask = ASPOP_MASK_32;
            ctb->opBitlen = 32;
            break;
        case ASPOP_CROP_11: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_CROP_05;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xffffffff;
            ctb->opMask = ASPOP_MASK_32;
            ctb->opBitlen = 32;
            break;
        case ASPOP_CROP_12: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_CROP_06;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xffffffff;
            ctb->opMask = ASPOP_MASK_32;
            ctb->opBitlen = 32;
            break;
        case ASPOP_CROP_13: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_CROP_01;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xffffffff;
            ctb->opMask = ASPOP_MASK_32;
            ctb->opBitlen = 32;
            break;
        case ASPOP_CROP_14: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_CROP_02;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xffffffff;
            ctb->opMask = ASPOP_MASK_32;
            ctb->opBitlen = 32;
            break;
        case ASPOP_CROP_15: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_CROP_03;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xffffffff;
            ctb->opMask = ASPOP_MASK_32;
            ctb->opBitlen = 32;
            break;
        case ASPOP_CROP_16: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_CROP_04;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xffffffff;
            ctb->opMask = ASPOP_MASK_32;
            ctb->opBitlen = 32;
            break;
        case ASPOP_CROP_17: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_CROP_05;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xffffffff;
            ctb->opMask = ASPOP_MASK_32;
            ctb->opBitlen = 32;
            break;
        case ASPOP_CROP_18: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_CROP_06;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xffffffff;
            ctb->opMask = ASPOP_MASK_32;
            ctb->opBitlen = 32;
            break;
        case ASPOP_IMG_LEN: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_IMG_LEN;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xffffffff;
            ctb->opMask = ASPOP_MASK_32;
            ctb->opBitlen = 32;
            break;
        case ASPOP_CROP_COOR_XH: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_STLEN_00;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_CROP_COOR_XL: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_STLEN_01;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_CROP_COOR_YH: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_STLEN_02;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_CROP_COOR_YL: 
            ctb->opStatus = ASPOP_STA_NONE;
            ctb->opCode = OP_STLEN_03;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0xff;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_EG_DECT: 
            ctb->opStatus = ASPOP_STA_UPD; /* default enable to test CROP */
            ctb->opCode = OP_EG_DECT;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0x1;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_AP_MODE: 
            ctb->opStatus = ASPOP_STA_APP; //default for debug ASPOP_STA_NONE;
            ctb->opCode = OP_AP_MODEN;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = APM_AP;  /* default ap mode */
            ctb->opMask = ASPOP_MASK_32;
            ctb->opBitlen = 32;
            break;
        case ASPOP_XCROP_GAT: 
            ctb->opStatus = ASPOP_STA_NONE; /* set to ASPOP_STA_SCAN from scanner*/
            ctb->opCode = OP_META_DAT;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0x0;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_XCROP_LINSTR: 
            ctb->opStatus = ASPOP_STA_NONE; /* set to ASPOP_STA_SCAN from scanner*/
            ctb->opCode = OP_META_DAT;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0x0;
            ctb->opMask = ASPOP_MASK_8;
            ctb->opBitlen = 8;
            break;
        case ASPOP_XCROP_LINREC: 
            ctb->opStatus = ASPOP_STA_NONE; /* set to ASPOP_STA_SCAN from scanner*/
            ctb->opCode = OP_META_DAT;
            ctb->opType = ASPOP_TYPE_VALUE;
            ctb->opValue = 0x0;
            ctb->opMask = ASPOP_MASK_16;
            ctb->opBitlen = 16;
            break;
        default: break;
        }
    }

// spidev id
    int fd0, fd1;
    fd0 = open(spi0, O_RDWR);
    if (fd0 < 0) 
        printf("can't open device[%s]\n", spi0); 
    else 
        printf("open device[%s]\n", spi0); 
    fd1 = open(spi1, O_RDWR);
    if (fd1 < 0) 
            printf("can't open device[%s]\n", spi1); 
    else 
        printf("open device[%s]\n", spi1); 

    pmrs->sfm[0] = fd0;
    pmrs->sfm[1] = fd1;
    pmrs->smode = 0;
    pmrs->smode |= SPI_MODE_1;

    #if 1/* set RDY pin to low before spi setup ready */
    bitset = 1;
    ret = ioctl(pmrs->sfm[0], _IOW(SPI_IOC_MAGIC, 6, __u32), &bitset);   //SPI_IOC_WR_CTL_PIN
    printf("[t]Set RDY low at beginning\n");
    #endif
    /*
     * spi speed 
     */ 

    bitset = 0;
    ioctl(pmrs->sfm[0], _IOW(SPI_IOC_MAGIC, 12, __u32), &bitset);   //SPI_IOC_WR_KBUFF_SEL

    bitset = 1;
    ioctl(pmrs->sfm[1], _IOW(SPI_IOC_MAGIC, 12, __u32), &bitset);   //SPI_IOC_WR_KBUFF_SEL

    speed = SPI_SPEED;
    ret = ioctl(pmrs->sfm[0], SPI_IOC_WR_MAX_SPEED_HZ, &speed);   //?�t�v 
    if (ret == -1) 
        printf("can't set max speed hz\n"); 
    else 
        printf("set spi0 max speed: %d hz\n", speed); 
        
    ret = ioctl(pmrs->sfm[1], SPI_IOC_WR_MAX_SPEED_HZ, &speed);   //?�t�v 
    if (ret == -1) 
        printf("can't set max speed hz\n"); 
    else 
        printf("set spi1 max speed: %d hz\n", speed); 

    /*
     * spi mode 
     */ 
    ret = ioctl(pmrs->sfm[0], SPI_IOC_WR_MODE, &pmrs->smode);
    if (ret == -1) 
        printf("can't set spi mode\n"); 
    
    ret = ioctl(pmrs->sfm[0], SPI_IOC_RD_MODE, &pmrs->smode);
    if (ret == -1) 
        printf("can't get spi mode\n"); 
    
    /*
     * spi mode 
     */ 
    ret = ioctl(pmrs->sfm[1], SPI_IOC_WR_MODE, &pmrs->smode); 
    if (ret == -1) 
        printf("can't set spi mode\n"); 
    
    ret = ioctl(pmrs->sfm[1], SPI_IOC_RD_MODE, &pmrs->smode);
    if (ret == -1) 
        printf("can't get spi mode\n"); 
    /* reset opcode table */
    memset(pmrs->opTable, 0xff, OPT_SIZE*sizeof(int));

// IPC
    pipe(pmrs->pipedn[0].rt);
    //pipe2(pmrs->pipedn[0].rt, O_NONBLOCK);
    pipe(pmrs->pipedn[1].rt);
    pipe(pmrs->pipedn[2].rt);
    pipe(pmrs->pipedn[3].rt);
    pipe(pmrs->pipedn[4].rt);
    pipe(pmrs->pipedn[5].rt);
    //pipe(pmrs->pipedn[6].rt);
    pipe2(pmrs->pipedn[6].rt, O_NONBLOCK);
	
    pipe2(pmrs->pipeup[0].rt, O_NONBLOCK);
    pipe2(pmrs->pipeup[1].rt, O_NONBLOCK);
    pipe2(pmrs->pipeup[2].rt, O_NONBLOCK);
    pipe2(pmrs->pipeup[3].rt, O_NONBLOCK);
    pipe2(pmrs->pipeup[4].rt, O_NONBLOCK);
    pipe2(pmrs->pipeup[5].rt, O_NONBLOCK);
    pipe2(pmrs->pipeup[6].rt, O_NONBLOCK);

    res_put_in(&rs[0], pmrs, 0);
    res_put_in(&rs[1], pmrs, 1);
    res_put_in(&rs[2], pmrs, 2);
    res_put_in(&rs[3], pmrs, 3);
    res_put_in(&rs[4], pmrs, 4);
    res_put_in(&rs[5], pmrs, 5);
    res_put_in(&rs[6], pmrs, 6);
	
//  Share memory init
    ring_buf_init(&pmrs->dataRx);
    pmrs->dataRx.r->folw.seq = 1;
    ring_buf_init(&pmrs->dataTx);
    ring_buf_init(&pmrs->cmdRx);
    ring_buf_init(&pmrs->cmdTx);

// fork process
    pmrs->sid[0] = fork();
    if (!pmrs->sid[0]) {
        p1(&rs[0], &rs[6]);
    } else {
        pmrs->sid[1] = fork();
        if (!pmrs->sid[1]) {
            p2(&rs[1]);
        } else {
            pmrs->sid[2] = fork();
            if (!pmrs->sid[2]) {
                p3(&rs[2]);
            } else {
                pmrs->sid[3] = fork();
                if (!pmrs->sid[3]) {
                    p4(&rs[3]);
                } else {				
                    pmrs->sid[4] = fork();
                    if (!pmrs->sid[4]) {
                        p5(&rs[4], &rs[5]);
                    } else { 
                        pmrs->sid[5] = fork();
                        if (!pmrs->sid[5]) {
                            dbg(pmrs);
                        } else {
                            p0(pmrs);
                        }
                    }
                }
            }
        }
    }
    end:

    sprintf(pmrs->log, "something wrong in mothership, break!\n");
    print_f(&pmrs->plog, "main", pmrs->log);
    printf_flush(&pmrs->plog, pmrs->flog);

    return 0;
}

static int print_f(struct logPool_s *plog, char *head, char *str)
{
    static int ptcount = 0;
    int len;
    char ch[256];
    if(!str) return (-1);

    if (head)
        sprintf(ch, "[%s] %s", head, str);
    else 
        sprintf(ch, "%s", str);

#if 0 /* remove log */
    printf(".");
    ptcount++;
    if ((ptcount % 32) == 0) {
        printf("\n");
        ptcount = 0;
    }
    return 0;
#else
    printf("%s",ch);
#endif

    if (!plog) return (-2);
	
    msync(plog, sizeof(struct logPool_s), MS_SYNC);
    len = strlen(ch);
    if ((len + plog->len) > plog->max) return (-3);
    memcpy(plog->cur, ch, strlen(ch));
    plog->cur += len;
    plog->len += len;

    //if (!mlog) return (-4);
    //fwrite(ch, 1, strlen(ch), mlog);
    //fflush(mlog);
    //fprintf(mlog, "%s", ch);
	
    return 0;
}

static int printf_flush(struct logPool_s *plog, FILE *f) 
{
    msync(plog, sizeof(struct logPool_s), MS_SYNC);
    if (plog->cur == plog->pool) return (-1);
    if (plog->len > plog->max) return (-2);

    msync(plog->pool, plog->len, MS_SYNC);
    fwrite(plog->pool, 1, plog->len, f);
    fflush(f);

    plog->cur = plog->pool;
    plog->len = 0;
    return 0;
}

static char **memory_init(int *sz, int tsize, int csize)
{
    char *mbuf, *tmpB;
    char **pma;
    int asz, idx;
    char mlog[256];
    
    if ((!tsize) || (!csize)) return (0);
    if (tsize % csize) return (0);
    if (!(tsize / csize)) return (0);
        
    asz = tsize / csize;
    pma = (char **) malloc(sizeof(char *) * asz);
    
    //sprintf(mlog, "asz:%d pma:0x%.8x\n", asz, pma);
    //print_f(mlogPool, "memory_init", mlog);
    
    mbuf = mmap(NULL, tsize, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    
    //sprintf(mlog, "mmap get 0x%.8x\n", mbuf);
    //print_f(mlogPool, "memory_init", mlog);
        
    tmpB = mbuf;
    for (idx = 0; idx < asz; idx++) {
        pma[idx] = mbuf;
        
        //sprintf(mlog, "%d 0x%.8x\n", idx, pma[idx]);
        //print_f(mlogPool, "memory_init", mlog);
        
        mbuf += csize;
    }
    *sz = asz;
    return pma;
}

static int time_diff(struct timespec *s, struct timespec *e, int unit)
{
    unsigned long long cur, tnow, lnow, past, tbef, lpast, gunit;
    int diff;

    gunit = unit;
    //clock_gettime(CLOCK_REALTIME, &curtime);
    cur = s->tv_sec;
    tnow = s->tv_nsec;
    lnow = cur * 1000000000+tnow;
    
    //clock_gettime(CLOCK_REALTIME, &curtime);
    past = e->tv_sec;
    tbef = e->tv_nsec;		
    lpast = past * 1000000000+tbef;	

    diff = (lpast - lnow)/gunit;

    return diff;
}

static int file_save_get(FILE **fp, char *path1)
{
//static char path1[] = "/mnt/mmc2/rx/%d.bin"; 

    char dst[128], temp[128], flog[256];
    FILE *f = NULL;
    int i;

    if (!path1) return (-1);

    sprintf(temp, "%s", path1);

    for (i =0; i < 1000; i++) {
        sprintf(dst, temp, i);
        f = fopen(dst, "r");
        if (!f) {
            sprintf(flog, "open file [%s]\n", dst);
            print_f(mlogPool, "save", flog);
            break;
        } else
            fclose(f);
    }
    f = fopen(dst, "w");

    *fp = f;
    return 0;
}

static int res_put_in(struct procRes_s *rs, struct mainRes_s *mrs, int idx)
{
    rs->pcmdRx = &mrs->cmdRx;
    rs->pcmdTx = &mrs->cmdTx;
    rs->pdataRx = &mrs->dataRx;
    rs->pdataTx = &mrs->dataTx;
    rs->fs_s = mrs->fs;
    rs->flog_s = mrs->flog;
    rs->fdat_s = mrs->fdat;
    rs->tm[0] = &mrs->time[0];
    rs->tm[1] = &mrs->time[1];

    rs->ppipedn = &mrs->pipedn[idx];
    rs->ppipeup = &mrs->pipeup[idx];
    rs->plogs = &mrs->plog;

    if((idx == 0) || (idx == 1) || (idx == 3)) {
        rs->spifd = mrs->sfm[0];
    } else if ((idx == 2) || (idx == 4)) {
        rs->spifd = mrs->sfm[1];
    }

    rs->psocket_r = &mrs->socket_r;
    rs->psocket_t = &mrs->socket_t;	

    rs->pmch = &mrs->mchine;
    rs->pregtb = mrs->regTable;
    rs->poptable = mrs->opTable;
    rs->psd_init = &mrs->sd_init;

    rs->pscnlen = &mrs->scan_length;
    rs->pcropCoord = &mrs->cropCoord;
    rs->pcropCoordDuo = &mrs->cropCoordDuo;

    rs->pmetaout = mrs->metaout;
    rs->pmetain = mrs->metain;    
    rs->pmetaMass = &mrs->metaMass;
    
    rs->pmetaoutduo = mrs->metaoutDuo;
    rs->pmetainduo = mrs->metainDuo;    
    rs->pmetaMassduo = &mrs->metaMassDuo;

    rs->pcfgTable = mrs->configTable;
    return 0;
}


