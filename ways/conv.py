#!/usr/bin/env python

if __name__ == "__main__":
    f = open("ways.txt", "r")
    f2 = open("ways2.txt", "w")
    if(f == None):
        print "Could not open file"
        exit

    for line in f:
        if(line[0].isdigit() == False):
            continue

        # Split the string
        arr = line.split(":")
        mins = int(arr[0])
        secs = int(arr[1])

        # Convert that to seconds
        timestamp = mins*60 + secs
        
        # Now into USB mass storage blocks
        # Assume 44100 samples/sec, joint stereo
        sampleNum = timestamp*44100*2
        blockNum = sampleNum/512

        f2.write(str(blockNum) + ",\n")
