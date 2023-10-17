#define USE_MULTI_THREAD
#include "library.hpp"

constexpr double limit_time = 60.0/17*(m-3) + 15;

namespace Solver {

constexpr int thread_num = 12 + 4;
constexpr int max_tasks_num = 4096 * 4;
int tasks_num = max_tasks_num;

RndInfo rnd_arrays[thread_num * max_tasks_num];

Data awesome[m];
Score_Type awesome_score = inf_score;

Score_Type scores[thread_num * max_tasks_num];

void solve(){
  awesome_score = best_score;
  memcpy(awesome, best, sizeof(best));

  int update_num = 0;
  double last_upd_time = -1;
  int steps = 0;

  constexpr double t0 = 2.5e3 * analyze_sampling_hz / 6000.0 * 0.9;
  constexpr double t1 = 1.0e2 * analyze_sampling_hz / 6000.0 * 0.8;
  double temp = t0;
  StopWatch sw;
  double spend_time = 0, p = 0;
  int cnt = 0;
  double temp_time = -1;
  int best_cnt[n] = {};
  if(contains_num == m) goto END;
  // 焼きなまし法(single thread)
  std::cerr << "Start Single Thread\n";
  for(; ; steps++){
    constexpr int mask = (1 << 9) - 1;
    if(!(steps & mask)){
      spend_time = sw.get_time();
      if(spend_time > limit_time*0.1) break;
      p = spend_time / limit_time;
      temp = pow(t0, 1.0-p) * pow(t1, p);
      //temp = (t1 - t0) * p + t0;
    }
    RndInfo change;
    rnd_create(change);
    const Score_Type score = calc_one_changed_ans(change);
    if(awesome_score > score){
      awesome_score = score;
      best_score = score;
      update_values(change);
      memcpy(awesome, best, sizeof(awesome));
      std::cerr << "u";
      update_num++;
      last_upd_time = spend_time;
    }else if(fast_exp((double)(best_score - score) / temp) > rnd(1024)/1024.0){
      best_score = score;
      update_values(change);
    }
  }
  best_score = awesome_score;
  init_array(awesome);

  std::cerr << "\n";
  std::cerr << "Score: " << best_score << "\n";
  std::cerr << "Start Multi Thread\n";
  // 焼きなまし法(multi thread)
  temp_time = spend_time;
  for(; ; steps += thread_num * tasks_num){
    {
      spend_time = sw.get_time();
      if(spend_time > limit_time) break;
      p = spend_time / limit_time;
      temp = pow(t0, 1.0-p) * pow(t1, p);
      //temp = (t1 - t0) * p + t0;
      if(spend_time-last_upd_time > 7.0){
        std::cerr << "r";
        last_upd_time = spend_time;
        best_score = awesome_score;
        init_array(awesome);
      }
      if(spend_time - last_upd_time >= 1.0){
        tasks_num = max_tasks_num;
      }else{
        tasks_num = max_tasks_num >> 5;
      }
    }
    cnt += thread_num * tasks_num;

    const int calc_num = thread_num * tasks_num;
    if(spend_time - last_upd_time <= 3.0 || !rnd(8)){
      rep(i, calc_num) rnd_create(rnd_arrays[i]);
    }else{
      rep(i, calc_num) rnd_create2(rnd_arrays[i]);
    }
    // multi thread
    #pragma omp parallel for
    rep(i, calc_num) scores[i] = calc_one_changed_ans(rnd_arrays[i]);
    int best_change_idx = -1;
    Score_Type good_score = inf_score;
    rep(i, calc_num){
      if(good_score > scores[i]){
        good_score = scores[i];
        best_change_idx = i;
      }
    }
    const RndInfo &best_change = rnd_arrays[best_change_idx];
    if(awesome_score > good_score){
      awesome_score = good_score;
      best_score = good_score;
      update_values(best_change);
      memcpy(awesome, best, sizeof(awesome));
      std::cerr << "u";
      update_num++;
      if(p >= 0.5) rep(i, m) best_cnt[best[i].idx]++;
      last_upd_time = spend_time;
    }else if(fast_exp((double)(best_score - good_score) / temp) > rnd(1024)/1024.0){
      best_score = good_score;
      update_values(best_change);
    }
  }
  END:
  best_score = awesome_score;
  init_array(awesome);
  std::cerr << "\n";
  std::cerr << "Steps: " << steps << "\n";
  std::cerr << "Updated: " << update_num << "\n";
  std::cerr << "Last Update: " << last_upd_time << "\n";
  std::cerr << "Time per loop: " << (spend_time-temp_time)/cnt << "\n";
  std::cerr << "Final Score: " << best_score << "\n";
  rep(i, half_n) std::cerr << best_cnt[i] << " ";
  std::cerr << "\n";
  rep(i, half_n) std::cerr << best_cnt[i + half_n] << " ";
  std::cerr << "\n";
  std::vector<std::pair<int,int>> v(n);
  rep(i, n) v[i] = { best_cnt[i], i };
  std::sort(v.rbegin(), v.rend());
  int top[n] = {};
  rep(i, m) top[v[i].second] = 1;
  rep(i, half_n){
    if(top[i]) std::cerr << "\x1b[1m";
    rep(j, m) if(best[j].idx == i){
      std::cerr << "\x1b[42m";
      break;
    }
    std::cerr << best_cnt[i];
    std::cerr << "(J" << i+1 << ")";
    std::cerr << "\x1b[49m"; // background color
    std::cerr << "\x1b[39m"; // font color
    std::cerr << "\x1b[0m"; // under bar
    std::cerr << " ";
  }
  std::cerr << "\n";
  rep(i, half_n){
    if(top[i+half_n]) std::cerr << "\x1b[1m";
    rep(j, m) if(best[j].idx == i+half_n){
      std::cerr << "\x1b[42m";
      break;
    }
    std::cerr << best_cnt[i + half_n];
    std::cerr << "(E" << i+1 << ")";
    std::cerr << "\x1b[49m"; // background color
    std::cerr << "\x1b[39m"; // font color
    std::cerr << "\x1b[0m"; // under bar
    std::cerr << " ";
  }
  File::output_result(best, awesome_score);
}

}; // namespace solver


int main(){
  srand(time(NULL));
  File::read_values();
  Solver::solve();
}