#include <iostream>
#include <vector>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <time.h>
#include <cstring>
#include <chrono>
#include <omp.h>
#include "audio_array.hpp"
#include "Math.hpp"
#include "hash.hpp"
#include "select_num.hpp"
#pragma GCC target("avx2")
#pragma GCC optimize("unroll-loops")
#define rep(i, n) for(int i = 0; i < (n); i++)
using uint = unsigned int;

constexpr int inf = (uint)-1 >> 1;

constexpr int n = 44*2; //candidate arrays
constexpr int half_n = n / 2;
constexpr int hz = analyze_sampling_hz; //sampling hz[48k->12k]
constexpr int tot_frame = analyze_audio_max_length; //max size of arrays[i]
constexpr int ans_length = hz * 16;

// 数列の値の型
using Val_Type = int;
using Score_Type = int;
constexpr Score_Type inf_score = (1ULL << (sizeof(Score_Type)*8-1)) - 1;

int audio_length[n] = {};
Val_Type problem[ans_length] = {};
int problem_length = ans_length;
int contains_num = 0; //確実に含まれている札の数

struct Data {
  int idx; //札の種類
  int pos; //貼り付け位置
  int st; //札の再生開始位置
  int len; //札の再生の長さ
};
Data answer[m];

// returns random [l, r)
inline int rnd(const int l, const int r) noexcept{
  static uint x = (uint)rand() | (uint)rand() << 16;
  x ^= x << 13; x ^= x >> 17;
  return (int)((x ^= x << 5) % (uint)(r - l)) + l;
}
// returns random [0, rng)
inline int rnd(const int rng) noexcept{
  static uint x = (uint)rand() | (uint)rand() << 16;
  x ^= x << 13; x ^= x >> 17;
  return (x ^= x << 5) % (uint)rng;
}

inline constexpr Val_Type Weight(const Val_Type x) noexcept{
  return x < 0 ? -x : x;
}


constexpr Score_Type calc_score(const Val_Type a[ans_length]) noexcept{
  Score_Type score = 0;
  rep(i, problem_length) score += Weight(a[i]);
  return score;
}


namespace Solver {
  void init();
  void init_values(const Data pre_result[m], const int contain);
} // namespace Solver

namespace File {

void read_values(){
  rep(i, n){
    audio_length[i] = tot_frame;
    rep(j, tot_frame){
      if(arrays[i][j] == inf){
        audio_length[i] = j;
        break;
      }
    }
  }
  Wave wave_data;
  read_audio(wave_data, "test/problem.wav");
  change_sampling_hz(wave_data, analyze_sampling_hz);
  assert(wave_data.L <= ans_length);
  for(int i = 0; i < wave_data.L; i++) problem[i] = wave_data[i];
  problem_length = wave_data.L;
  
  std::string s;
  std::cin >> s;
  rep(i, m) answer[i].idx = -1;
  if(s == "Input"){
    Solver::init();
  }else if(s == "Output"){
    Data pre_values[m];
    int tmp;
    int contain;
    rep(i, m) std::cin >> tmp; // fuda
    std::cin >> tmp; // score
    std::cin >> contain; // contains num
    // data
    rep(i, m){
      std::cin >> pre_values[i].idx >> pre_values[i].pos >> pre_values[i].st >> pre_values[i].len;
      pre_values[i].pos *= analyze_change_prop;
      pre_values[i].st *= analyze_change_prop;
      pre_values[i].len *= analyze_change_prop;
    }
    Solver::init_values(pre_values, contain);
  }else{
    std::cerr << "wrong format!!!\n";
    assert(0);
  }
}

void output_result(const Data best[m], const Score_Type final_score){
  std::cout << "Output\n";
  rep(i, m) std::cout << best[i].idx << " \n"[i == m - 1];
  std::cout << final_score << "\n";
  std::cout << "\n";
  std::cout << contains_num << "\n";
  rep(i, m){
    std::cout << best[i].idx << " " << best[i].pos << " " << best[i].st << " " << best[i].len << "\n";
  }
}

}; // namespace File

namespace Calc {

using namespace HashImpl;

[[nodiscard]]
std::vector<int> find_single_audio(const Wave &problem_data, const std::vector<Wave> &audio_waves){
  std::cerr << "start find single audio(JP)\n";
  static constexpr int range = 1 << 10;
  RollingHash<range> roliha;
  const Hash<range> problem_hash = roliha.gen(problem_data.data);
  HashSet<ull, 22> problem_sampling;
  rep(i, problem_data.L - range){
    problem_sampling.set(problem_hash.query(i, i+range));
  }
  std::vector<int> result;
  for(int i = 0; i < half_n; i++){
    const Hash<range> wave_data_hash = roliha.gen(audio_waves[i].data);
    bool ok = false;
    rep(j, audio_waves[i].L - range){
      const ull h = wave_data_hash.query(j, j+range);
      if(problem_sampling.find(h)){
        ok = true;
        break;
      }
    }
    if(ok) result.push_back(i);
  }
  std::cerr << "start find single audio(EN)\n";
  for(int i = half_n; i < n; i++){
    const Hash<range> wave_data_hash = roliha.gen(audio_waves[i].data);
    bool ok = false;
    rep(j, audio_waves[i].L - range){
      const ull h = wave_data_hash.query(j, j+range);
      if(problem_sampling.find(h)){
        ok = true;
        break;
      }
    }
    if(ok) result.push_back(i);
  }
  return result;
}

[[nodiscard]]
std::vector<int> find_double_audio(const Wave &problem_data, const std::vector<Wave> &audio_waves){
  std::cerr << "start find double audio\n";
  static constexpr int range = 1 << 10;
  RollingHash<range> roliha;
  const Hash<range> problem_hash = roliha.gen(problem_data.data);
  std::cerr << "base: " << roliha.base << "\n";
  std::vector<Hash<range>> audio_hash;
  HashMap<ull, char, 28> audio_map;
  rep(i, n){
    std::cerr << i << " ";
    audio_hash.emplace_back(roliha.gen(audio_waves[i].data));
    rep(j, audio_waves[i].L - range){
      audio_map.set(audio_hash[i].query(j), (char)i);
    }
  }
  std::cerr << "\n";
  static constexpr int rng = 1 << 12;
  ull *problem_hash_array = new ull[(problem_data.L-range) / rng];
  rep(i, (problem_data.L - range) / rng){
    assert(i * rng + range < problem_data.L);
    problem_hash_array[i] = problem_hash.query(i*rng);
  }
  std::vector<int> result;
  rep(i, n){
    std::cerr << i << " ";
    std::vector<std::vector<int>> res(audio_waves[i].L - range);
    #pragma omp parallel for
    rep(j, audio_waves[i].L - range){
      const ull cur_audio_hash = roliha.mod - audio_hash[i].query(j);
      // rng飛ばしで探索
      rep(k, (problem_data.L - range) / rng){
        const ull hash = problem_hash_array[k] + cur_audio_hash;
        if(audio_map.find(hash)){
          res[j].push_back((int)audio_map.get(hash));
          std::cerr << "Find!! ";
        }
      }
    }
    rep(j, audio_waves[i].L - range) if(!res[j].empty()){
      result.push_back(i);
      for(const int x : res[j]) result.push_back(x);
    }
  }
  delete[] problem_hash_array;
  std::cerr << "\n";
  return result;
}

[[nodiscard]]
std::vector<int> find_audio(){
  std::cerr << "reading wave data...\n";
  Wave problem_wave;
  read_audio(problem_wave, "test/problem.wav");
  std::vector<Wave> audio_waves(n);
  char buf[64];
  rep(i, half_n){
    sprintf(buf, "audio/JKspeech/J%02d.wav", i+1);
    read_audio(audio_waves[i], buf);
  }
  for(int i = half_n; i < n; i++){
    sprintf(buf, "audio/JKspeech/E%02d.wav", i-44+1);
    read_audio(audio_waves[i], buf);
  }
  const std::vector<int> single_audio = find_single_audio(problem_wave, audio_waves);
  const std::vector<int> double_audio = find_double_audio(problem_wave, audio_waves);
  std::vector<int> result = single_audio;
  result.insert(result.end(), double_audio.begin(), double_audio.end());
  std::sort(result.begin(), result.end());
  result.erase(std::unique(result.begin(), result.end()), result.end());
  return result;
}

} // namespace Calc

namespace Solver {

struct RndInfo {
  int idx; //変更する値
  int pos; //変更後の貼り付け位置
  int nxt_idx; //新しく更新する札
  int st; //次の札の再生開始位置
  int len; //次の札の再生の長さ
};

Data best[m];
uint64_t used_idx = 0;
Val_Type best_sub[ans_length];

Score_Type problem_wave_score = inf_score;
Score_Type best_score = inf_score;

void init(){
  memcpy(best_sub, problem, sizeof(problem));
  problem_wave_score = calc_score(best_sub);
  const std::vector<int> surely_contain = Calc::find_audio();
  contains_num = (int)surely_contain.size();
  std::cerr << "Surely Contains Num: " << contains_num << "\n";
  for(const int x : surely_contain) std::cerr << x << " ";
  std::cerr << "\n";
  rep(i, contains_num){
    best[i].idx = surely_contain[i];
    best[i].pos = 0;
    best[i].st = 0;
    best[i].len = min(problem_length, audio_length[best[i].idx]);
    used_idx |= 1ULL << (best[i].idx % half_n);
  }
  // 最初はランダムに値を入れておく
  for(int i = contains_num; i < m; i++){
    int idx = rnd(n);
    while(used_idx >> (idx % half_n) & 1){
      idx = rnd(n);
    }
    best[i].idx = idx;
    best[i].pos = 0;
    best[i].st = 0;
    best[i].len = min(problem_length, audio_length[best[i].idx]);
    used_idx |= 1ULL << (best[i].idx % half_n);
  }
  rep(i, m){
    rep(j, best[i].len){
      best_sub[j + best[i].pos] -= arrays[best[i].idx][j + best[i].st];
    }
  }
  best_score = calc_score(best_sub);

  std::cerr << "Problem Wave: " << problem_wave_score << "\n";
  std::cerr << "First Score: " << best_score << "\n";
}
void init_values(const Data pre_result[m], const int contain){
  memcpy(best_sub, problem, sizeof(problem));
  problem_wave_score = calc_score(best_sub);
  contains_num = contain;
  memcpy(best, pre_result, sizeof(best));
  rep(i, m){
    used_idx |= 1ULL << (best[i].idx % half_n);
    rep(j, best[i].len){
      best_sub[j + best[i].pos] -= arrays[best[i].idx][j + best[i].st];
    }
  }
  best_score = calc_score(best_sub);

  std::cerr << "Problem Wave: " << problem_wave_score << "\n";
  std::cerr << "First Score: " << best_score << "\n";
}

// import to best from data
void init_array(const Data data[]) noexcept{
  memcpy(best, data, sizeof(best));
  used_idx = 0;
  memcpy(best_sub, problem, sizeof(problem));
  rep(i, m){
    used_idx |= 1ULL << (best[i].idx % half_n);
    rep(j, best[i].len){
      best_sub[j + best[i].pos] -= arrays[best[i].idx][j + best[i].st];
    }
  }
}


inline void rnd_create(RndInfo &change, const int rng = hz) noexcept{
  //constexpr int rng = hz;
  const int t = rnd(10);
  // select wav and change pos
  if(t == 0){
    change.idx = rnd(m);
    change.nxt_idx = best[change.idx].idx;
    change.len = rnd(hz, min(problem_length, audio_length[change.nxt_idx]) + 1);
    change.st = rnd(audio_length[change.nxt_idx] - change.len + 1);
    change.pos = rnd(problem_length - change.len + 1);
  }
  // select other wav and swap and change pos
  else if(t == 1 || t == 6){
    change.idx = rnd(contains_num, m);
    change.nxt_idx = rnd(n);
    while((used_idx >> (change.nxt_idx % half_n) & 1) && best[change.idx].idx % half_n != change.nxt_idx % half_n){
      change.nxt_idx = rnd(n);
    }
    change.len = rnd(hz, min(problem_length, audio_length[change.nxt_idx]) + 1);
    change.st = rnd(audio_length[change.nxt_idx] - change.len + 1);
    change.pos = rnd(problem_length - change.len + 1);
  }
  // change len
  else if(t == 2){
    change.idx = rnd(m);
    change.nxt_idx = best[change.idx].idx;
    change.st = best[change.idx].st;
    change.pos = best[change.idx].pos;
    //change.len = rnd(hz, min(problem_length-change.pos, audio_length[change.nxt_idx]-change.st) + 1);
    change.len = rnd(max(hz, best[change.idx].len-rng), min(min(problem_length-change.pos, audio_length[change.nxt_idx]-change.st), best[change.idx].len+rng) + 1);
  }
  // change pos
  else if(t == 3){
    change.idx = rnd(m);
    change.nxt_idx = best[change.idx].idx;
    change.st = best[change.idx].st;
    change.len = best[change.idx].len;
    //change.pos = rnd(problem_length - change.len + 1);
    change.pos = rnd(max(0, best[change.idx].pos-rng), min(problem_length-change.len, best[change.idx].pos+rng) + 1);
  }
  // change st
  else if(t == 4){
    change.idx = rnd(m);
    change.nxt_idx = best[change.idx].idx;
    change.len = best[change.idx].len;
    change.pos = best[change.idx].pos;
    //change.st = rnd(min(audio_length[change.nxt_idx] - change.len, problem_length-change.pos) + 1);
    change.st = rnd(max(0, best[change.idx].st-rng), min(audio_length[change.nxt_idx]-change.len, best[change.idx].st+rng) + 1);
  }
  // change wav type
  else if(t == 5 || t == 7){
    change.idx = rnd(contains_num, m);
    change.nxt_idx = rnd(n);
    while((used_idx >> (change.nxt_idx % half_n) & 1) && best[change.idx].idx % half_n != change.nxt_idx % half_n){
      change.nxt_idx = rnd(n);
    }
    change.len = min(best[change.idx].len, audio_length[change.nxt_idx]);
    change.st = min(best[change.idx].st, audio_length[change.nxt_idx] - change.len);
    change.pos = min(best[change.idx].pos, problem_length - change.len);
  }
  // change st,pos and len
  else if(t == 8){
    change.idx = rnd(m);
    change.nxt_idx = best[change.idx].idx;
    change.pos = rnd(best[change.idx].pos - min(rng, min(best[change.idx].pos, best[change.idx].st)), min(best[change.idx].pos+rng, best[change.idx].pos+best[change.idx].len-hz) + 1);
    change.st = (change.pos - best[change.idx].pos) + best[change.idx].st;
    change.len = best[change.idx].len - (change.pos - best[change.idx].pos);
  }
  // copy other wav info
  else if(t == 9){
    change.idx = rnd(m);
    change.nxt_idx = best[change.idx].idx;
    int sel_idx = rnd(m);
    while(sel_idx != change.idx) sel_idx = rnd(m);
    change.pos = best[sel_idx].pos;
    change.st = min(audio_length[change.nxt_idx] - hz, best[sel_idx].st);
    change.len = min(audio_length[change.nxt_idx]-change.st, best[sel_idx].len);
  }
}
inline void rnd_create2(RndInfo &change) noexcept{
  const int t = rnd(4);
  // select wav and change pos
  if(t == 0){
    change.idx = rnd(m);
    change.nxt_idx = best[change.idx].idx;
    change.len = rnd(hz, min(problem_length, audio_length[change.nxt_idx]) + 1);
    change.st = rnd(audio_length[change.nxt_idx] - change.len + 1);
    change.pos = rnd(problem_length - change.len + 1);
  }
  // select other wav and swap and change pos
  else if(t == 1 || t == 3){
    change.idx = rnd(contains_num, m);
    change.nxt_idx = rnd(n);
    while((used_idx >> (change.nxt_idx % half_n) & 1) && best[change.idx].idx % half_n != change.nxt_idx % half_n){
      change.nxt_idx = rnd(n);
    }
    change.len = rnd(hz, min(problem_length, audio_length[change.nxt_idx]) + 1);
    change.st = rnd(audio_length[change.nxt_idx] - change.len + 1);
    change.pos = rnd(problem_length - change.len + 1);
  }
  // change wav type
  else if(t == 2){
    change.idx = rnd(contains_num, m);
    change.nxt_idx = rnd(n);
    while((used_idx >> (change.nxt_idx % half_n) & 1) && best[change.idx].idx % half_n != change.nxt_idx % half_n){
      change.nxt_idx = rnd(n);
    }
    change.len = min(best[change.idx].len, audio_length[change.nxt_idx]);
    change.st = min(best[change.idx].st, audio_length[change.nxt_idx] - change.len);
    change.pos = min(best[change.idx].pos, problem_length - change.len);
  }
}

inline Score_Type calc_one_changed_ans(const RndInfo &info) noexcept{
  const Data &pre = best[info.idx];
  const int info_rig = info.pos + info.len;
  const int pre_rig = pre.pos + pre.len;
  const Val_Type *arr_info = arrays[info.nxt_idx] + info.st;
  const Val_Type *arr_pre = arrays[pre.idx] + pre.st;
  Score_Type score = best_score;
  // cross
  if(is_cross(info.pos, info_rig, pre.pos, pre_rig)){
    // 左側がleft
    if(info.pos <= pre.pos){
      const int rightest = min(info_rig, pre.pos);
      for(int i = info.pos; i < rightest; i++){
        score += Weight(best_sub[i] - arr_info[i-info.pos]) - Weight(best_sub[i]);
      }
    }
    // 左側がright
    else{
      const int rightest = min(pre_rig, info.pos);
      for(int i = pre.pos; i < rightest; i++){
        score += Weight(best_sub[i] + arr_pre[i-pre.pos]) - Weight(best_sub[i]);
      }
    }
    // 右側がright
    if(info_rig <= pre_rig){
      const int leftest = max(pre.pos, info_rig);
      const int s = max(info_rig - pre.pos, 0);
      for(int i = leftest; i < pre_rig; i++){
        score += Weight(best_sub[i] + arr_pre[i+s-leftest]) - Weight(best_sub[i]);
      }
    }
    // 右側がleft
    else{
      const int leftest = max(info.pos, pre_rig);
      const int s = max(pre_rig - info.pos, 0);
      for(int i = leftest; i < info_rig; i++){
        score += Weight(best_sub[i] - arr_info[i+s-leftest]) - Weight(best_sub[i]);
      }
    }
    // middle
    const int leftest = max(info.pos, pre.pos);
    const int range = min(info_rig, pre_rig) - leftest;
    const int sl = max(pre.pos - info.pos, 0);
    const int sr = max(info.pos - pre.pos, 0);
    rep(i, range){
      score -= Weight(best_sub[i+leftest]);
      score += Weight(best_sub[i+leftest] - arr_info[i+sl] + arr_pre[i+sr]);
    }
  }
  // not cross
  else{
    for(int i = 0; i < info.len; i++){
      score += Weight(best_sub[i+info.pos] - arr_info[i]) - Weight(best_sub[i+info.pos]);
    }
    for(int i = 0; i < pre.len; i++){
      score += Weight(best_sub[i+pre.pos] + arr_pre[i]) - Weight(best_sub[i+pre.pos]);
    }
  }
  return score;
}


inline void update_values(const RndInfo &info) noexcept{
  // update best_sub
  const Data &pre = best[info.idx];
  rep(i, pre.len){
    best_sub[i + pre.pos] += arrays[pre.idx][i + pre.st];
  }
  rep(i, info.len){
    best_sub[i + info.pos] -= arrays[info.nxt_idx][i + info.st];
  }
  // update info
  used_idx ^= 1ULL << (best[info.idx].idx % half_n);
  used_idx ^= 1ULL << (info.nxt_idx % half_n);
  best[info.idx].idx = info.nxt_idx;
  best[info.idx].pos = info.pos;
  best[info.idx].st = info.st;
  best[info.idx].len = info.len;
}

}; // namespace solver


struct StopWatch {
  const std::chrono::system_clock::time_point start_time;
  StopWatch() : start_time(std::chrono::system_clock::now()){}
  inline double get_time() const noexcept{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - start_time).count() * 1e-6;
  }
};

struct ios_do_not_sync {
  ios_do_not_sync(){
    std::cin.tie(nullptr);
    std::ios::sync_with_stdio(false);
  }
} ios_do_not_sync_instance;