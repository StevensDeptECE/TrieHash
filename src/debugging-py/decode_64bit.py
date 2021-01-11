#!/usr/bin/env python3
from math import floor
# print((chr(ord('a')+5)))
# for testing
base = 27
while True:
    x = input("\t\t>>> ")
    try:
        x = int(x)
        s = ""
        while x > 0:
            if x%base != 26:
                s += (chr(ord('a')+x%base))
            else:
                s += "/"
            x = x//base
        print(s)
    except Exception:
        calc = 0
        arr = [c for c in x.strip()]
        arr = reversed(arr)
        for c in arr:
            if c == '/':
                calc *= base
                calc += 26
            else:
                calc *= base
                calc += ord(c)-ord('a')
        print(calc)
