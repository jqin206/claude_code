​

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <immintrin.h> // For AVX-512
#include <fstream>
#include <sstream>

namespace py = pybind11;

// SIMD-optimized FIX message parser
std::vector<std::unordered_map<std::string, std::string>> parse_fix_messages_simd(const std::string& filepath) {
    std::vector<std::unordered_map<std::string, std::string>> messages;
    std::ifstream file(filepath);
    std::string line;
   
    // Constants for SIMD processing
    constexpr size_t SIMD_WIDTH = 64; // 512 bits = 64 bytes
    const char delimiter = '|';
    const char equals = '=';
   
    while (std::getline(file, line)) {
        std::unordered_map<std::string, std::string> message;
        size_t pos = 0;
        size_t line_len = line.size();
       
        // Process in SIMD-width chunks
        while (pos < line_len) {
            size_t remaining = line_len - pos;
            size_t chunk_size = (remaining < SIMD_WIDTH) ? remaining : SIMD_WIDTH;
           
            // Load chunk into SIMD register
            __m512i chunk = _mm512_loadu_epi8(line.data() + pos);
           
            // Find delimiters and equals signs in parallel
            __mmask64 delim_mask = _mm512_cmpeq_epi8_mask(chunk, _mm512_set1_epi8(delimiter));
            __mmask64 equals_mask = _mm512_cmpeq_epi8_mask(chunk, _mm512_set1_epi8(equals));
           
            // Process the matches
            uint64_t delim_bits = delim_mask;
            uint64_t equals_bits = equals_mask;
           
            size_t chunk_end = pos + chunk_size;
            size_t last_delim = pos;
            size_t key_start = pos;
           
            while (pos < chunk_end) {
                size_t rel_pos = pos - (chunk_end - chunk_size);
                if (delim_bits & (1ULL << rel_pos)) {
                    // Found a field delimiter
                    size_t field_end = pos;
                    size_t equals_pos = last_delim;
                   
                    // Find equals sign between last_delim and field_end
                    for (size_t i = last_delim; i < field_end; ++i) {
                        if (line[i] == equals) {
                            equals_pos = i;
                            break;
                        }
                    }
                   
                    if (equals_pos != last_delim) {
                        std::string key(line.begin() + last_delim, line.begin() + equals_pos);
                        std::string value(line.begin() + equals_pos + 1, line.begin() + field_end);
                        message[key] = value;
                    }
                   
                    last_delim = pos + 1; // Skip the delimiter
                }
                pos++;
            }
        }
       
        if (!message.empty()) {
            messages.push_back(message);
        }
    }
   
    return messages;
}

PYBIND11_MODULE(fix_parser, m) {
    m.def("parse_fix_messages_simd", &parse_fix_messages_simd,
          "Parse FIX messages from log file using SIMD optimization");
}

