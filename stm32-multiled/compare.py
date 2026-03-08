import sys
import pdb

print("Total arguments:", len(sys.argv))
print("Script name:", sys.argv[0])
a=[]

for arg in sys.argv[1:]:
    a.append(str(arg))

pos = 0
choice = 0
try:
    sample = a[choice][pos]
    while sample:
        for match in a:
            # print(match[pos], sample)
            if match[pos] != sample:
                print("Fails at :", pos, "Match", match[pos], sample)
        pos += 1
        sample = a[choice][pos]   # update for next iteration

except:
    print("End of String")
