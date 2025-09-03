#! /usr/bin/env python3
# This program validates the results of main.cpp for N = 4, B = 1_001_805, MAX_HITS = 30_000 against known results
import urllib.request


RED = "\033[0;31m"
GREEN = "\033[0;32m"
BOLD = "\033[1m"
END = "\033[0m"

n = 3
num_valid = 0
total = 0
with open("results.txt") as f:
    found = [[int(i) for i in line.split(" ")] for line in f.readlines()]

print(f"{BOLD}Validating solutions{END}")
for solution in found:
    a, b, c, d, s = solution
    valid = (a**n + b**n == s and c ** n + d**n == s)
    if valid:
        num_valid += 1
    else:
        print(f"{RED}INVALID SOLUTION: {a}^{n} + {b}^{n} = {c}^{n} + {d}^{n} = {s}{END}")
    total += 1

color = GREEN if total == num_valid else RED
print(f"{color}Checked {total} total solutions, found {num_valid} valid solutions.{END}")

print(f"{BOLD}Checking against known solutions{END}")
known_url = "https://oeis.org/A018786/b018786.txt"
with urllib.request.urlopen(known_url) as response:
    known_txt = response.read().decode('utf-8')

known_solns = set(int(i.split(" ")[1]) for i in known_txt.splitlines() if len(i.split(" ")) >= 2)
found_solns = set(s for (_, _, _, _, s) in found)

missing = known_solns - found_solns
extra = found_solns - known_solns

print(f"Known solutions: {len(known_solns)}")
print(f"Found solutions: {len(found_solns)}")

if missing:
    print(f"{len(missing)} solutions in known but not found:")
    for sol in sorted(missing):
        print(f"  Missing: {sol}")

if extra:
    print(f"{len(extra)} solutions in found but not in known:")
    for sol in sorted(extra):
        print(f"  Extra: {sol}")

