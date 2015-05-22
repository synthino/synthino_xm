/*
  Synthino polyphonic synthesizer
  Copyright (C) 2014-2015 Michael Krumpus

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <avr/pgmspace.h>
#include "waveforms.h"
const int16_t sin_0001[N_WAVEFORM_SAMPLES] PROGMEM = {
  9,
  19,
  30,
  41,
  51,
  62,
  73,
  83,
  94,
  105,
  115,
  126,
  137,
  147,
  158,
  168,
  179,
  190,
  200,
  211,
  221,
  231,
  242,
  252,
  263,
  273,
  283,
  294,
  304,
  314,
  324,
  334,
  344,
  355,
  365,
  375,
  384,
  394,
  404,
  414,
  424,
  434,
  443,
  453,
  462,
  472,
  481,
  491,
  500,
  510,
  519,
  528,
  537,
  546,
  555,
  564,
  573,
  582,
  591,
  600,
  608,
  617,
  625,
  634,
  642,
  650,
  659,
  667,
  675,
  683,
  691,
  699,
  707,
  714,
  722,
  729,
  737,
  744,
  752,
  759,
  766,
  773,
  780,
  787,
  794,
  801,
  807,
  814,
  820,
  827,
  833,
  839,
  845,
  851,
  857,
  863,
  869,
  874,
  880,
  885,
  890,
  896,
  901,
  906,
  911,
  916,
  920,
  925,
  930,
  934,
  938,
  943,
  947,
  951,
  955,
  959,
  962,
  966,
  969,
  973,
  976,
  979,
  982,
  985,
  988,
  991,
  994,
  996,
  999,
  1001,
  1003,
  1005,
  1007,
  1009,
  1011,
  1012,
  1014,
  1015,
  1017,
  1018,
  1019,
  1020,
  1021,
  1022,
  1022,
  1023,
  1023,
  1023,
  1023,
  1023,
  1023,
  1023,
  1023,
  1023,
  1023,
  1022,
  1022,
  1021,
  1020,
  1019,
  1018,
  1016,
  1015,
  1014,
  1012,
  1010,
  1009,
  1007,
  1005,
  1002,
  1000,
  998,
  995,
  993,
  990,
  987,
  985,
  982,
  978,
  975,
  972,
  968,
  965,
  961,
  958,
  954,
  950,
  946,
  942,
  937,
  933,
  928,
  924,
  919,
  914,
  910,
  905,
  900,
  894,
  889,
  884,
  878,
  873,
  867,
  861,
  855,
  850,
  844,
  837,
  831,
  825,
  819,
  812,
  805,
  799,
  792,
  785,
  778,
  771,
  764,
  757,
  750,
  742,
  735,
  728,
  720,
  712,
  705,
  697,
  689,
  681,
  673,
  665,
  657,
  648,
  640,
  632,
  623,
  615,
  606,
  597,
  589,
  580,
  571,
  562,
  553,
  544,
  535,
  526,
  516,
  507,
  498,
  488,
  479,
  470,
  460,
  450,
  441,
  431,
  421,
  412,
  402,
  392,
  382,
  372,
  362,
  352,
  342,
  332,
  322,
  311,
  301,
  291,
  281,
  270,
  260,
  250,
  239,
  229,
  218,
  208,
  197,
  187,
  176,
  166,
  155,
  145,
  134,
  123,
  113,
  102,
  91,
  81,
  70,
  59,
  49,
  38,
  27,
  16,
  6,
  -5,
  -16,
  -26,
  -37,
  -48,
  -58,
  -69,
  -80,
  -90,
  -101,
  -112,
  -122,
  -133,
  -144,
  -154,
  -165,
  -175,
  -186,
  -196,
  -207,
  -217,
  -228,
  -238,
  -249,
  -259,
  -269,
  -280,
  -290,
  -300,
  -311,
  -321,
  -331,
  -341,
  -351,
  -361,
  -371,
  -381,
  -391,
  -401,
  -411,
  -421,
  -430,
  -440,
  -450,
  -459,
  -469,
  -478,
  -488,
  -497,
  -506,
  -516,
  -525,
  -534,
  -543,
  -552,
  -561,
  -570,
  -579,
  -588,
  -597,
  -605,
  -614,
  -622,
  -631,
  -639,
  -648,
  -656,
  -664,
  -672,
  -680,
  -688,
  -696,
  -704,
  -712,
  -719,
  -727,
  -734,
  -742,
  -749,
  -756,
  -764,
  -771,
  -778,
  -785,
  -791,
  -798,
  -805,
  -811,
  -818,
  -824,
  -831,
  -837,
  -843,
  -849,
  -855,
  -861,
  -867,
  -872,
  -878,
  -883,
  -889,
  -894,
  -899,
  -904,
  -909,
  -914,
  -919,
  -923,
  -928,
  -933,
  -937,
  -941,
  -945,
  -949,
  -953,
  -957,
  -961,
  -965,
  -968,
  -972,
  -975,
  -978,
  -981,
  -984,
  -987,
  -990,
  -993,
  -995,
  -998,
  -1000,
  -1002,
  -1004,
  -1006,
  -1008,
  -1010,
  -1012,
  -1013,
  -1015,
  -1016,
  -1018,
  -1019,
  -1020,
  -1021,
  -1021,
  -1022,
  -1023,
  -1023,
  -1024,
  -1024,
  -1024,
  -1024,
  -1024,
  -1024,
  -1023,
  -1023,
  -1022,
  -1022,
  -1021,
  -1020,
  -1019,
  -1018,
  -1017,
  -1016,
  -1014,
  -1013,
  -1011,
  -1009,
  -1007,
  -1005,
  -1003,
  -1001,
  -999,
  -996,
  -994,
  -991,
  -988,
  -985,
  -983,
  -979,
  -976,
  -973,
  -970,
  -966,
  -963,
  -959,
  -955,
  -951,
  -947,
  -943,
  -939,
  -934,
  -930,
  -925,
  -921,
  -916,
  -911,
  -906,
  -901,
  -896,
  -891,
  -886,
  -880,
  -875,
  -869,
  -863,
  -857,
  -852,
  -846,
  -839,
  -833,
  -827,
  -821,
  -814,
  -808,
  -801,
  -794,
  -787,
  -781,
  -774,
  -767,
  -759,
  -752,
  -745,
  -737,
  -730,
  -722,
  -715,
  -707,
  -699,
  -691,
  -684,
  -676,
  -667,
  -659,
  -651,
  -643,
  -634,
  -626,
  -617,
  -609,
  -600,
  -591,
  -583,
  -574,
  -565,
  -556,
  -547,
  -538,
  -529,
  -520,
  -510,
  -501,
  -492,
  -482,
  -473,
  -463,
  -454,
  -444,
  -434,
  -425,
  -415,
  -405,
  -395,
  -385,
  -375,
  -365,
  -355,
  -345,
  -335,
  -325,
  -315,
  -305,
  -294,
  -284,
  -274,
  -263,
  -253,
  -243,
  -232,
  -222,
  -211,
  -201,
  -190,
  -180,
  -169,
  -159,
  -148,
  -137,
  -127,
  -116,
  -106,
  -95,
  -84,
  -74,
  -63,
  -52,
  -41,
  -31,
  -20,
  -9
};
