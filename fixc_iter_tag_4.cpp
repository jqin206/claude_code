
#include <iostream> 
#include <fstream> 
#include <string> 
#include <chrono> 
#include <sstream>
#include <iomanip>
#include <vector>
#include <immintrin.h> 
#include <cstdint>

using namespace std;

string ts()
{
  chrono::time_point curr_time_tp = chrono::system_clock::now();
  time_t curr_time_t = chrono::system_clock::to_time_t(curr_time_tp);
  struct tm* curr_time_tm = gmtime(&curr_time_t);
  
  int ms = chrono::duration_cast<chrono::milliseconds> (curr_time_tp.time_since_epoch()).count() % 1000;
  
  stringstream ss;
  ss << put_time(curr_time_tm, "%Y%m%d-%H:%M:%S.") << setfill('0') << setw(3) << ms;
  string time_str = ss.str();
  return time_str;
}

inline string calc_checksum(const string& msg)
{
  const char* ptr = msg.data();
  size_t size = msg.size();
  
  __m256i sum = _mm256_setzero_si256();
  size_t i = 0;
  // Process 32 bytes at a time
  for (; i + 32 <= size; i += 32)
  {
    __m256i chunk = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(ptr + i));
    sum = _mm256_add_epi8(sum, chunk);
  }
  // Horizontal sum of 32 bytes
  uint8_t simd_sum = 0;
  alignas (32) uint8_t temp[32];
  _mm256_store_si256(reinterpret_cast<__m256i*>(temp), sum);
  
  for (size_t j = 0; j < 32; ++j)
  {
  simd_sum += temp[j];
  }

  // Process remaining bytes
  uint8_t remainder_sum = 0;
  for (; i < size; ++i)
  {
    remainder_sum += ptr[i];
  }
  
  uint8_t checksum = (simd_sum + remainder_sum) % 256 ;
  char checksum_str[4];
  snprintf(checksum_str, sizeof(checksum_str), "%03d", checksum); 
  return checksum_str;
}

int main(int argc, char* argv[])
{
  vector<string> args(argv+1, argv+argc);
  string VER, HOST, PORT, SEND, TARG, FILE, HB;
  for (auto it = args.begin(); it != args.end()-1; it++)
  {
    string flag = *it;
    if (flag == "-v" || flag == "--version")
      VER = *(it+1);
    else if (flag == "-h" || flag == "--host")
      HOST = *(it+1);
    else if (flag == "-p" || flag == = "--port")
      PORT = *(it+1);
    else if (flag == "-s" || flag == "--SenderCompID")
      SEND = *(it+1);
    else if (flag == "-t" || flag == "--TargetCompID")
      TARG = *(it+1);
    else if (flag == "-f" || flag == "--Inputfile")
      FILE = *(it+1);
    else if (flag == "-b"
      HB = *(it+1);
  }
  
ifstream f(FILE);
string line;
char SOH = 1;
  
stringstream ss_tag49;
ss_tag49 << "49=" << SEND;
string tag49 = ss_tag49.str();
  
stringstream ss_tag56;
ss_tag56 << "56=" << TARG;
string tag56 = ss_tag56.str();
  
stringstream ss_tag8; 
ss_tag8 << "8=FIX." << VER;
string tag8 = ss_tag8.str();

int seq = 0;
while (getline(f, line))
{
  string tag_val;
  stringstream ss_line(line);
  stringstream ss_body;
  string msg_type;
  
  if (line.substr(0,5) == "8=FIX")
  {
    seq++;
    while (getline(ss_line, tag_val, '|'))
    {
      int equal_sign = tag_val.find("=");
      string tag = tag_val.substr(0, equal_sign);
      if (tag == "8" || tag == "9" || tag == "10" || tag == "34" || tag == "49" || tag == "52" || tag == "56" )
        continue;
      else if (tag == "35")
        msg_type = tag_val.substr(equal_sign+1, tag_val.size());
      else if (tag == "108")
        ss_body << tag << "=" << HB << SOH;
      else
      {
        string val = tag_val.substr(equal_sign+1, tag_val.size()); 
        ss_body << tag << "=" << val << SOH;
      }
    }
  }
}
f.close();
}
