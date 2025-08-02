
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <memory>
#include <cstring>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

class FastFileReader {
public:
    // Read file in large chunks and process lines
    std::vector<std::vector<std::string>> readAndSplit(const std::string& filepath, char delimiter) {
        std::vector<std::vector<std::string>> result;
       
        // Open file in binary mode for fastest reading
        std::ifstream file(filepath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open file: " + filepath);
        }

        // Get file size and allocate buffer
        const size_t file_size = file.tellg();
        file.seekg(0);
       
        // Use large buffer (4MB) for reading
        const size_t buffer_size = 4 * 1024 * 1024;
        std::unique_ptr<char[]> buffer(new char[buffer_size]);
       
        size_t remaining = file_size;
        std::string leftover;
       
        while (remaining > 0) {
            // Read next chunk
            const size_t read_size = std::min(buffer_size, remaining);
            file.read(buffer.get(), read_size);
            remaining -= read_size;
           
            // Process buffer
            processChunk(buffer.get(), read_size, delimiter, leftover, result);
        }
       
        // Process any remaining data
        if (!leftover.empty()) {
            processLine(leftover, delimiter, result);
        }
       
        return result;
    }

private:
    void processChunk(char* buffer, size_t length, char delimiter,
                     std::string& leftover, std::vector<std::vector<std::string>>& result) {
        char* start = buffer;
        char* end = buffer + length;
        char* current = start;
       
        while (current < end) {
            // Find next newline
            char* newline = std::find(current, end, ' ');
           
            // Combine with leftover if needed
            std::string line;
            if (!leftover.empty()) {
                line = leftover + std::string(current, newline);
                leftover.clear();
            } else {
                line.assign(current, newline);
            }
           
            // If we didn't reach the end, process the complete line
            if (newline != end) {
                processLine(line, delimiter, result);
                current = newline + 1;
            } else {
                // Save leftover for next chunk
                leftover.assign(current, newline);
                break;
            }
        }
    }
   
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
       
        // Add the last token
        tokens.emplace_back(line.substr(start));
        result.emplace_back(std::move(tokens));
    }
};

PYBIND11_MODULE(fastfilereader, m) {
    py::class_<FastFileReader>(m, "FastFileReader")
        .def(py::init<>())
        .def("read_and_split", &FastFileReader::readAndSplit,
             "Read a large file and split each line by delimiter",
             py::arg("filepath"), py::arg("delimiter")=',');
}




Sent from my phone
