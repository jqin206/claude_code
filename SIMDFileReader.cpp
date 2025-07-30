simd file reader

‪‪Sean Qin‬‬
​You​
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <memory>
#include <immintrin.h>  // SIMD intrinsics
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

class SIMDFileReader {
public:
    std::vector<std::vector<std::string>> readAndSplit(const std::string& filepath, char delimiter) {
        std::vector<std::vector<std::string>> result;
       
        // Open file
        std::ifstream file(filepath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open file: " + filepath);
        }

        // Get file size
        const size_t file_size = file.tellg();
        file.seekg(0);
       
        // Use aligned buffer for SIMD (64-byte alignment for AVX512 compatibility)
        constexpr size_t buffer_size = 4 * 1024 * 1024;
        constexpr size_t alignment = 64;
        char* buffer = static_cast<char*>(_mm_malloc(buffer_size, alignment));
       
        size_t remaining = file_size;
        std::string leftover;
       
        while (remaining > 0) {
            const size_t read_size = std::min(buffer_size, remaining);
            file.read(buffer, read_size);
            remaining -= read_size;
           
            processChunkSIMD(buffer, read_size, delimiter, leftover, result);
        }
       
        // Process any remaining data
        if (!leftover.empty()) {
            processLine(leftover, delimiter, result);
        }
       
        _mm_free(buffer);
        return result;
    }

private:
    void processChunkSIMD(char* buffer, size_t length, char delimiter,
                         std::string& leftover, std::vector<std::vector<std::string>>& result) {
        const __m256i delim_vec = _mm256_set1_epi8(delimiter);
        const __m256i newline_vec = _mm256_set1_epi8(' ');
       
        size_t processed = 0;
        const size_t simd_width = 32; // AVX2 processes 32 bytes at a time
       
        // Process 32-byte chunks with SIMD
        while (processed + simd_width <= length) {
            __m256i chunk = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(buffer + processed));
           
            // Find delimiters and newlines
            __m256i delim_mask = _mm256_cmpeq_epi8(chunk, delim_vec);
            __m256i newline_mask = _mm256_cmpeq_epi8(chunk, newline_vec);
           
            uint32_t delim_bits = _mm256_movemask_epi8(delim_mask);
            uint32_t newline_bits = _mm256_movemask_epi8(newline_mask);
           
            // Process lines
            if (newline_bits) {
                size_t line_end = processed + __builtin_ctz(newline_bits);
                std::string line = leftover + std::string(buffer + processed, buffer + line_end);
                leftover.clear();
                processLineSIMD(line, delimiter, result);
                processed = line_end + 1;
            } else {
                processed += simd_width;
            }
        }
       
        // Process remaining bytes (less than 32)
        if (processed < length) {
            leftover.append(buffer + processed, length - processed);
        }
    }
   
    void processLineSIMD(const std::string& line, char delimiter,
                        std::vector<std::vector<std::string>>& result) {
        if (line.empty()) return;
       
        std::vector<std::string> tokens;
        const char* str = line.c_str();
        const char* str_end = str + line.size();
        const char* token_start = str;
       
        // Use SIMD for the main part of the line
        const __m256i delim_vec = _mm256_set1_epi8(delimiter);
        const size_t simd_width = 32;
       
        while (str + simd_width <= str_end) {
            __m256i chunk = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(str));
            __m256i mask = _mm256_cmpeq_epi8(chunk, delim_vec);
            uint32_t mask_bits = _mm256_movemask_epi8(mask);
           
            while (mask_bits != 0) {
                uint32_t next_delim = __builtin_ctz(mask_bits);
                tokens.emplace_back(token_start, str + next_delim);
                token_start = str + next_delim + 1;
                mask_bits &= ~(1 << next_delim);
            }
           
            str += simd_width;
        }
       
        // Process remaining characters
        while (str < str_end) {
            if (*str == delimiter) {
                tokens.emplace_back(token_start, str);
                token_start = str + 1;
            }
            str++;
        }
       
        // Add last token
        tokens.emplace_back(token_start, str_end);
        result.emplace_back(std::move(tokens));
    }
   
    // Fallback for non-SIMD compatible cases
    void processLine(const std::string& line, char delimiter,
                    std::vector<std::vector<std::string>>& result) {
        if (line.empty()) return;
       
        std::vector<std::string> tokens;
        size_t start = 0;
        size_t end = line.find(delimiter);
       
        while (end != std::string::npos) {
            tokens.emplace_back(line.substr(start, end - start));
            start = end + 1;
            end = line.find(delimiter, start);
        }
       
        tokens.emplace_back(line.substr(start));
        result.emplace_back(std::move(tokens));
    }
};

PYBIND11_MODULE(simdfilereader, m) {
    py::class_<SIMDFileReader>(m, "SIMDFileReader")
        .def(py::init<>())
        .def("read_and_split", &SIMDFileReader::readAndSplit,
             "Read a large file and split each line by delimiter using SIMD",
             py::arg("filepath"), py::arg("delimiter")=',');
}




Sent from my phone
