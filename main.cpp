/**
 * A program to find all solutions to the Diopthane equation a^N + b^N = c^N +
 * d^N where 1 <= b <= a <= B and 1 <= d <= c <= B. You can set the values of N
 * and B by editing the constants that appear immediately after the includes.
 *
 * The program has no external dependencies, so it can be compiled by just
 * invoking make or calling a C++ compiler of your choice.
 *
 * This project is distributed under the BSD 2-Clause License.
 * Copyright (c) 2025, Gideon Grinberg
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 *     2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <queue>
#include <string>
#include <vector>

// Configuration constants (see top-level comment)
constexpr int N = 4;
constexpr int B = 1001805;
constexpr int MAX_HITS = 30000;            // 0 for infinite
constexpr size_t BUFFER_SIZE = 1024 * 512; // 500kb
constexpr int UPDATE_INTERVAL =
    100; // number of new hits to update the progress bar

typedef unsigned __int128 u128;
typedef std::vector<u128> PowList;

constexpr u128 U128_MAX = (u128)(~(u128)0);

// On Mac (and I think windows), sprintf is marked as deprecated for security
// concerns We are using it in a hot path with a pre-allocated buffer and no
// external input, so I think it's fine to suppress the warning. This macro does
// so without globally disabling deprecation warnings for other functions.
#define sprintf_nowarn(buf, fmt, ...)                                          \
    ({                                                                         \
        _Pragma("clang diagnostic push") _Pragma(                              \
            "clang diagnostic ignored \"-Wdeprecated-declarations\"") int      \
            __result = sprintf(buf, fmt, __VA_ARGS__);                         \
        _Pragma("clang diagnostic pop") __result;                              \
    })

struct Node {
    int a, b;
    u128 sum;
};

struct SumCmp {
    bool operator()(const Node &x, const Node &y) const {
        return x.sum > y.sum;
    }
};

class ResultsBuffer {
    std::array<char, BUFFER_SIZE> buffer{};
    size_t pos = 0;
    std::ofstream &file;

    // Formats a u128 into a buffer and returns the size
    // adopted from https://stackoverflow.com/a/11660651
    size_t formatU128(char *buffer, u128 value) {
        if (value > UINT64_MAX) {
            u128 leading = value / 10000000000000000000ULL;
            uint64_t trailing = value % 10000000000000000000ULL;
            int pos = formatU128(buffer, leading);
            return pos + sprintf_nowarn(buffer + pos, "%019llu", trailing);

        } else {
            return sprintf_nowarn(buffer, "%llu", static_cast<uint64_t>(value));
        }
    }

  public:
    ResultsBuffer(std::ofstream &f) : file(f) {}
    ~ResultsBuffer() { this->flush(); }

    /**
     * Pushes the solution a^N + b^N = c^N + d^N = sum into the buffer.
     * Returns true if the buffer flushes and false otherwise.
     */
    bool push(int a, int b, int c, int d, u128 sum) {
        pos += sprintf_nowarn(buffer.data() + pos, "%d %d %d %d ", a, b, c, d);
        pos += formatU128(buffer.data() + pos, sum);
        buffer[pos++] = '\n';
        if (pos >= BUFFER_SIZE - 200) {
            flush();
            return true;
        }

        return false;
    }

    /**
     * Flush the buffer into the output file.
     */
    void flush() {
        if (pos > 0) {
            file.write(buffer.data(), pos);
            pos = 0;
        }
    }
};

constexpr u128 powN(u128 x) {
    u128 result = 1;
    for (int i = 0; i < N; i++) {
        result *= x;
    }

    return result;
}

// returns a^N for 0<a<B
static PowList precompute_pows() {
    PowList result(B + 1);
    for (int i = 0; i <= B; i++) {
        result[i] = powN(static_cast<u128>(i));
    }

    return result;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << "[OUTPUT PATH]" << "\n";
        std::exit(1);
    }

    std::ofstream outfile(argv[1]);
    if (!outfile) {
        std::cerr << "Could not open output file: " << argv[1] << "\n";
        std::exit(1);
    }

    ResultsBuffer results(outfile);

    std::cout << "Searching for "
              << (MAX_HITS > 0 ? ("up to " + std::to_string(MAX_HITS) + " ")
                               : "")
              << "solutions with N = " << N << " and "
              << "an upper bound of " << B << "\n";
    std::cout << "Precomputing powers of " << N << "\n";
    PowList pows = precompute_pows();
    std::priority_queue<Node, std::vector<Node>, SumCmp> pq;

    std::cout << "Initializing heap\n";
    for (int a = 1; a <= B; a++) {
        pq.emplace(Node{a, 1, pows[a] + pows[1]});
    }

    u128 prev_sum = U128_MAX; // max u128 as sentinel value
    std::vector<std::pair<int, int>> prev_pairs;
    prev_pairs.reserve(8);

    std::cout << "Beginning search loop.\n";
    std::cout << "0/" << MAX_HITS << " hits found." << std::flush;
    int hits = 0;
    while (!pq.empty()) {
        Node curr = pq.top();
        pq.pop();

        std::pair<int, int> curr_pair = {curr.a, curr.b};
        if (curr.sum == prev_sum) {
            for (auto &prev_pair : prev_pairs) {
                if (!(prev_pair.first == curr_pair.first &&
                      prev_pair.second == curr_pair.second)) {
                    results.push(prev_pair.first, prev_pair.second,
                                 curr_pair.first, curr_pair.second, curr.sum);

                    if (MAX_HITS > 0) {
                        hits += 1;
                        if (hits % UPDATE_INTERVAL == 0) {
                            std::cout << "\r" << hits << "/" << MAX_HITS
                                      << " hits found.";
                        }

                        if (hits >= MAX_HITS) {
                            std::cout << std::endl;
                            return 0;
                        }
                    }
                }
            }

            prev_pairs.push_back(curr_pair);
        } else {
            // reset accumulator for new sum
            prev_pairs.clear();
            prev_pairs.push_back(curr_pair);
            prev_sum = curr.sum;
        }

        if (curr.b + 1 <= curr.a) {
            int nb = curr.b + 1;
            pq.emplace(Node{curr.a, nb, pows[curr.a] + pows[nb]});
        }
    }
}