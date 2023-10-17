#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <cassert>

constexpr int short_max = (1 << 15) - 1;
constexpr int default_sampling_hz = 48000;
constexpr int analyze_change_prop = 3;
#ifdef USE_MULTI_THREAD
constexpr int analyze_sampling_hz = 6000;
#else
constexpr int analyze_sampling_hz = 6000/analyze_change_prop;
#endif
constexpr int default_audio_max_length = 394606;
constexpr int analyze_audio_max_length = default_audio_max_length / (default_sampling_hz / analyze_sampling_hz);
static_assert(default_sampling_hz % analyze_sampling_hz == 0);

struct Wave {
  int fs; //サンプリング周波数
  int bits; //量子化bit数
  int L = 0; //データ長
  std::vector<int> data;
  int &operator[](const int i) noexcept{
    assert(0 <= i && i < L);
    return data[i];
  }
  int operator[](const int i) const noexcept{
    assert(0 <= i && i < L);
    return data[i];
  }
};

struct FileReader {
  FILE *fp;
  const int len;
  char *buf;
  const char *p;
  FileReader(const char *file_name) : fp(fopen(file_name, "rb")), len(get_file_size(fp)), buf(new char[len]), p(buf){
    if(fp == NULL){
      fprintf(stderr, "file(%s) was not found", file_name);
      exit(1);
    }
    const int size = (int)fread(buf, 1, len, fp);
    assert(size == len);
  }
  ~FileReader(){
    fclose(fp);
    delete[] buf;
  }
  int get_file_size(FILE *fp){
    if(fp == NULL) return 1;
    fseek(fp, 0, SEEK_END);
    const int pos = (int)ftell(fp);
    fseek(fp, 0, SEEK_SET);
    return pos;
  }
  constexpr void through(const int steps) noexcept{
    p += steps;
    assert(p < buf + len);
  }
  template <class T>
  constexpr T read() noexcept{
    assert(p < buf + len);
    const T val = *(T*)p;
    p += sizeof(T);
    return val;
  }
};

struct FileWriter {
  FILE *fp;
  FileWriter(const char *file_name) : fp(fopen(file_name, "wb")){
    if(fp == NULL){
      fprintf(stderr, "file(%s) was not found", file_name);
      exit(1);
    }
  }
  ~FileWriter(){
    fclose(fp);
  }
  template <class T>
  void write(const T val) noexcept{
    fwrite(&val, sizeof(T), 1, fp);
  }
  template <size_t size>
  void write(const char (&str)[size]) noexcept{
    fwrite(str, 1, size-1, fp); //except null-moji
  }
};

void read_audio(Wave &prm, const char *filename){
  FileReader reader(filename);
  reader.through(24);
  prm.fs = reader.read<int>();
  reader.through(6);
  prm.bits = (int)reader.read<short>();
  reader.through(4);
  prm.L = reader.read<int>() / 2;
  prm.data.resize(prm.L);
  for(int i = 0; i < prm.L; i++){
    prm[i] = (int)reader.read<short>();
  }
}

void write_audio(const Wave &prm, const char *filename){
  FileWriter writer(filename);
  writer.write("RIFF");
  writer.write(36 + prm.L * 2);
  writer.write("WAVEfmt ");
  writer.write(16);
  writer.write((short)1);
  writer.write((short)1);
  writer.write(prm.fs);
  writer.write(prm.fs * prm.bits / 8);
  writer.write((short)(prm.bits / 8));
  writer.write((short)prm.bits);
  writer.write("data");
  writer.write(prm.L * 2);
  for(int i = 0; i < prm.L; i++){
    short data_data;
    if(prm[i] > short_max) data_data = short_max;
    else if(prm[i] < -short_max) data_data = -short_max;
    else data_data = prm[i];
    writer.write(data_data);
  }
}


//prm.fs[Hz] -> fs[Hz]
void change_sampling_hz(Wave &prm, const int fs){
  assert(prm.fs % fs == 0);
  const int p = prm.fs / fs;
  prm.fs = fs;
  std::vector<int> new_data(prm.L / p);
  for(int i = 0; i < prm.L / p; i++){
    new_data[i] = prm[i * p];
  }
  prm.L /= p;
  prm.data = new_data;
}

// st[i]: waves[i]の開始位置(1sample単位)
[[nodiscard]]
Wave merge_audio(const std::vector<Wave> &waves, const std::vector<int> &st){
  assert(waves.size() == st.size());
  Wave res;
  res.bits = waves[0].bits;
  res.fs = waves[0].fs;
  res.L = 0;
  const int n = (int)waves.size();
  for(int i = 0; i < n; i++){
    assert(res.bits == waves[i].bits && res.fs == waves[i].fs);
    res.L = std::max(res.L, waves[i].L + st[i]);
  }
  res.data.assign(res.L, 0);
  for(int i = 0; i < n; i++){
    for(int j = 0; j < waves[i].L; j++){
      res[j + st[i]] += waves[i][j];
    }
  }
  for(int i = 0; i < res.L; i++){
    if(res[i] > short_max) res[i] = short_max;
    else if(res[i] < -short_max) res[i] = -short_max;
  }
  return res;
}

//sep[i]: 分割する位置。sep.front() = 0, sep.back() = wave.L
[[nodiscard]]
std::vector<Wave> separate_audio(const Wave &wave, const std::vector<int> &sep){
  const int n = (int)sep.size();
  assert(sep[0] == 0 && sep[n - 1] == wave.L);
  for(int i = 1; i < n - 1; i++){
    assert(sep[i] < sep[i + 1]);
    assert(0 < sep[i] && sep[i] < wave.L);
  }
  std::vector<Wave> res(n - 1);
  for(int i = 0; i < n - 1; i++){
    res[i].bits = wave.bits;
    res[i].fs = wave.fs;
    res[i].L = sep[i + 1] - sep[i];
    for(int j = sep[i]; j < sep[i + 1]; j++){
      res[i].data.emplace_back(wave[j]);
    }
    assert((int)res[i].data.size() == sep[i + 1] - sep[i]);
  }
  return res;
}