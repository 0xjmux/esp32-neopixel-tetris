#!/usr/bin/env python

# Generate a C LUT for my 8x32 LED matrix

# 1,   2,  3,  4,  5,  6,  7, 8
# 16, 15, 14, 13, 12, 11, 10, 9
# ...

import numpy as np

totalNumLEDs = 256
displayHeightRows = 32
displayWidthCols = 8
currRow= 0

# tempArray = [[0]*displayWidthCols]*displayHeightRows
tempArray = np.zeros((displayHeightRows, displayWidthCols))


def prettyPrintArray():
    for row in tempArray:
        print(row)

# prettyPrintArray()

for i in range(totalNumLEDs):

    currRow = i // displayWidthCols
    # print("currRow is : " + str(currRow))
    # if row is even we print them backwards - first LED is on right side
    if currRow % 2 == 0:
        tempArray[currRow][displayWidthCols - (i % displayWidthCols) - 1] = i

    else:
        tempArray[currRow][i % displayWidthCols] = i


        # print(f"row {currRow} is even")
       # print(f"LED {i}", end='');



# print LUT out
# first line:
print(f"uint8_t rowcol_to_LEDNum_LUT[{ displayHeightRows }][{ displayWidthCols }] =")
print("{")
rowi = 0
for row in tempArray:
    print("  {", end='')
    for item in row:
        if not item == row[-1]:
            print(f'{int(item):3d}, ',end='')
        else:
            print(f'{int(item):3d}',end='')

    print("}, // row " + str(rowi))
    rowi += 1

print("};")



# if (i % displayWidthCols == 0): # end of a row, newline
