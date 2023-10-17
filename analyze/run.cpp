#include <iostream>
#include <fstream>
#include <string.h>
#include <cassert>
using namespace std;

void write_text(const char *txt, const char *filename){
  FILE *fp = fopen(filename, "w");
  assert(fp != NULL);
  fwrite(txt, 1, strlen(txt), fp);
  fclose(fp);
}
void copy_file(const char *from_file_name, const char *to_file_name){
  ifstream is(from_file_name, ios::in | ios::binary);
  ofstream os(to_file_name, ios::out | ios::binary);
  os << is.rdbuf();
}

char buf[128];
constexpr int half_n = 44;
int indices[half_n];
int speech_num;

void output_result(ifstream &result, bool is_eof, bool is_out){
  std::string s; result >> s; // Output
  // trial
  if(!is_eof){
    int res[half_n];
    for(int i = 0; i < speech_num; i++) result >> res[i];
    int audio_diff_num = 0;
    int karuta_diff_num = 0;
    for(int i = 0; i < speech_num; i++){
      bool ok = false, ok2 = false;
      for(int j = 0; j < speech_num; j++){
        if(res[i] == indices[j]){
          ok = ok2 = true;
          break;
        }else if(res[i] % half_n == indices[j] % half_n){
          ok2 = true;
        }
      }
      if(!ok) audio_diff_num++;
      if(!ok2) karuta_diff_num++;
    }
    cerr << "\nAudio Diff: " << audio_diff_num << "/" << speech_num << "\n";
    cerr << "Karuta Diff: " << karuta_diff_num << "/" << speech_num << "\n";
    int score; result >> score;
    if(is_out){
      cout << audio_diff_num << " " << karuta_diff_num << "\n";
      cout << score << "\n";
    }
  }
  // production
  else{
    if(is_out){
      for(int i = 0; i < speech_num; i++){
        int a; result >> a;
        cout << a << " \n"[i == speech_num - 1];
      }
    }
  }
}

int main(){
  ifstream info("test/information.txt");
  info >> speech_num;
  cerr << "Run TIme: " << (180.0/17*(speech_num-3) + 60) + 15 << "[s]\n";
  sprintf(buf, "constexpr int m = %d;\n", speech_num);
  write_text(buf, "select_num.hpp"); // update speech num
  for(int i = 0; i < speech_num; i++) info >> indices[i];
  write_text("Input\n", "in.txt");
#if defined(_WIN32) || defined(_WIN64)
  system("g++ yakinamashi.cpp -Ofast -std=c++17 -fopenmp -lgomp");
  system("a.exe < in.txt > out.txt");
  ifstream pre_result("out.txt");
  output_result(pre_result, info.eof(), 0);
  pre_result.close();
  system("g++ yakinamashi_thread.cpp -Ofast -fopenmp -lgomp");
  copy_file("out.txt", "in.txt");
  system("a.exe < in.txt > out.txt");
#else
  system("g++ yakinamashi.cpp -Ofast");
  system("./a.out < in.txt > out.txt");
  ifstream pre_result("out.txt");
  output_result(pre_result, info.eof(), 0);
  pre_result.close();
  system("g++ yakinamashi_thread.cpp -Ofast -fopenmp -lgomp");
  copy_file("out.txt", "in.txt");
  system("./a.out < in.txt > out.txt");
#endif
  ifstream result("out.txt");
  output_result(result, info.eof(), 1);
  result.close();
}