public fun XrayChMapping()
{
/*
LOGICO RACK VME_MODULE VME_CANALE   DIODO   AMP_TYPE  AMP  GPIB_MODULE  GPIB_CH  AMP_BOX
		   L  R  VM VC   D  AT   A  GM GC  AB                                */
  return (
		[[ 1, 0, 1, 1, 201, 22, 28, 1, 1,  6],
  	   	 [ 2, 0, 1, 2, 202, 21, 42, 1, 2,  6],
  		 [ 3, 0, 1, 3, 203, 22, 22, 1, 3,  6],
  		 [ 4, 0, 1, 4, 204, 22, 23, 1, 4,  6],
  		 [ 5, 0, 2, 1, 205, 22, 24, 2, 1,  7],
  		 [ 6, 0, 2, 2, 206, 22, 25, 2, 2,  7],
  		 [ 7, 0, 2, 3, 207, 22, 26, 2, 3,  7],
  		 [ 8, 0, 2, 4, 208, 22, 27, 2, 4,  7],
  		 [ 9, 0, 3, 1, 209, 21, 20, 3, 1,  8],
  		 [10, 0, 3, 2, 210, 22, 29, 3, 2,  8],
  		 [11, 0, 3, 3, 211, 21, 1,  3, 3,  8],
  		 [12, 0, 3, 4, 212, 21, 31, 3, 4,  8],
  		 [13, 0, 4, 1, 213, 21, 32, 4, 1,  9],
  		 [14, 0, 4, 2, 214, 21, 33, 4, 2,  9],
  		 [15, 0, 4, 3, 215, 21, 34, 4, 3,  9],
  		 [16, 0, 4, 4, 216, 21, 35, 4, 4,  9],
  		 [17, 0, 5, 1, 217, 21, 36, 5, 1, 10],
  		 [18, 0, 5, 2, 218, 21, 37, 5, 2, 10],
  		 [19, 0, 5, 3, 219, 21, 38, 5, 3, 10],

  		 [20, 1, 1, 1, 101, 21, 30, 12, 1, 1],
  		 [21, 1, 1, 2, 102, 21, 40, 12, 2, 1],
  		 [22, 1, 1, 3, 103, 21, 3,  12, 3, 1],
  		 [23, 1, 1, 4, 104, 21, 4,  12, 4, 1],
  		 [24, 1, 2, 1, 105, 21, 5,  13, 1, 2],
  		 [25, 1, 2, 2, 106, 21, 6,  13, 2, 2],
  		 [26, 1, 2, 3, 107, 21, 7,  13, 3, 2],
  		 [27, 1, 2, 4, 108, 21, 8,  13, 4, 2],
  		 [28, 1, 3, 1, 109, 22, 9,  14, 1, 3],
  		 [29, 1, 3, 2, 110, 22, 10, 14, 2, 3],
  		 [30, 1, 3, 3, 111, 22, 11, 14, 3, 3],
  		 [31, 1, 3, 4, 112, 21, 12, 14, 4, 3],
  		 [32, 1, 4, 1, 113, 21, 13, 15, 1, 4],
  		 [33, 1, 4, 2, 114, 21, 14, 15, 2, 4],
  		 [34, 1, 4, 3, 115, 21, 15, 15, 3, 4],
  		 [35, 1, 4, 4, 116, 21, 16, 15, 4, 4],
  		 [36, 1, 5, 1, 117, 21, 17, 16, 1, 5],
  		 [37, 1, 5, 2, 118, 21, 18, 16, 2, 5],
  		 [38, 1, 5, 3, 119, 21, 19, 16, 3, 5],

  		 [39, 1, 6,  1, 301, 21, 39, 17, 1, 11],
  		 [40, 1, 6,  2, 302, 21, 2,  17, 2, 11],
  		 [41, 1, 6,  3, 303, 21, 41, 17, 3, 11],
  		 [42, 1, 7,  1, 304, 22, 76, 18, 1, 12],
  		 [43, 1, 6,  4, 305, 22, 56, 17, 4, 11],
  		 [44, 1, 7,  2, 306, 21, 44, 18, 2, 12],
  		 [45, 1, 7,  3, 307, 21, 45, 18, 3, 12],
  		 [46, 1, 7,  4, 308, 22, 46, 18, 4, 12],
  		 [47, 1, 8,  1, 309, 21, 47, 19, 1, 13],
  		 [48, 1, 8,  2, 310, 22, 48, 19, 2, 13],
  		 [49, 1, 8,  3, 311, 21, 49, 19, 3, 13],
  		 [50, 1, 8,  4, 312, 22, 50, 19, 4, 13],
  		 [51, 1, 9,  1, 313, 21, 51, 20, 1, 14],
  		 [52, 1, 9,  2, 314, 22, 52, 20, 2, 14],
  		 [53, 1, 9,  3, 315, 21, 53, 20, 3, 14],
  		 [54, 1, 9,  4, 316, 21, 57, 20, 4, 14],
  		 [55, 1, 10, 1, 317, 21, 55, 21, 1, 15],
  		 [56, 1, 10, 2, 318, 21, 43, 21, 2, 15],
  		 [57, 1, 10, 3, 319, 22, 54, 21, 3, 15],

  		 [58, 0,  6, 1, 401, 22, 58,  6, 1, 16],
  		 [59, 0,  6, 2, 402, 22, 59,  6, 2, 16],
  		 [60, 0,  6, 3, 403, 22, 60,  6, 3, 16],
  		 [61, 0,  6, 4, 404, 22, 61,  6, 4, 16],
  		 [62, 0,  7, 1, 405, 22, 62,  7, 1, 17],
  		 [63, 0,  7, 2, 406, 21, 63,  7, 2, 17],
  		 [64, 0,  7, 3, 407, 21, 64,  7, 3, 17],
  		 [65, 0,  7, 4, 408, 21, 65,  7, 4, 17],
  		 [66, 0,  8, 1, 409, 21, 66,  8, 1, 18],
  		 [67, 0,  8, 2, 410, 22, 67,  8, 2, 18],
  		 [68, 0,  8, 3, 411, 22, 68,  8, 3, 18],
  		 [69, 0,  8, 4, 412, 22, 69,  8, 4, 18],
  		 [70, 0,  9, 1, 413, 21, 70,  9, 1, 19],
  		 [71, 0,  9, 2, 414, 21, 71,  9, 2, 19],
  		 [72, 0,  9, 3, 415, 21, 21,  9, 3, 19],
  		 [73, 0,  9, 4, 416, 21, 73,  9, 4, 19],
  		 [74, 0, 10, 1, 417, 22, 74, 10, 1, 20],
  		 [75, 0, 10, 2, 418, 22, 75, 10, 2, 20],
  		 [76, 0, 10, 3, 419, 21, 72, 10, 3, 20],
  		 [77, 0, 10, 4, 420, 22, 77, 10, 4, 20],
  		 [78, 0,  5, 4, 421, 22, 78, 11, 1, 21]]);
}
