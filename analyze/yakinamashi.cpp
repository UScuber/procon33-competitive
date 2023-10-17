#include "library.hpp"

constexpr double limit_time = 180.0/17*(m-3) + 45;


namespace Solver {

Data awesome[m];
Score_Type awesome_score = inf_score;

void solve(){
  awesome_score = best_score;
  memcpy(awesome, best, sizeof(best));

  int update_num = 0;
  double last_upd_time = -1;
  int steps = 0;

  constexpr double t0 = 2.5e3 * analyze_sampling_hz / 6000.0;
  double t1 = 1.0e2 * analyze_sampling_hz / 6000.0;
  //t1 = (t1 - t0) * 2.3 / 3.0 + t0;
  double temp = t0;
  double spend_time = 0, p = 0;
  StopWatch sw;
  int best_cnt[n] = {};
  if(contains_num == m) goto END;
  // 焼きなまし法
  for(; ; steps++){
    constexpr int mask = (1 << 10) - 1;
    if(!(steps & mask)){
      spend_time = sw.get_time();
      if(spend_time > limit_time) break;
      p = spend_time / limit_time;
      //temp = pow(t0, 1.0-p) * pow(t1, p);
      temp = (t1 - t0) * p + t0;
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
      if(p >= 0.5) rep(i, m) best_cnt[best[i].idx]++;
      last_upd_time = spend_time;
    }else if(fast_exp((double)(best_score - score) / temp) > rnd(1024)/1024.0){
      best_score = score;
      update_values(change);
    }
  }
  END:
  std::cerr << "\n";
  std::cerr << "Steps: " << steps << "\n";
  std::cerr << "Updated: " << update_num << "\n";
  std::cerr << "Last Update: " << last_upd_time << "\n";
  std::cerr << "Time per loop: " << spend_time/steps << "\n";
  std::cerr << "Final Score: " << awesome_score << "\n";
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
  std::cerr << "\n";

  File::output_result(best, awesome_score);
}

}; // namespace solver


int main(){
  srand(time(NULL));
  File::read_values();
  Solver::solve();
}