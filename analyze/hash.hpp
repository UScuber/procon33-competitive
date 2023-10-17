#include <vector>
#include <chrono>

namespace HashImpl {

using ull = unsigned long long;
using uint = unsigned int;

constexpr uint w = 1 << 6;
constexpr ull m = (1ULL << 61) - 1;

ull gen_rnd() noexcept{
  ull m = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
  m ^= m >> 16;
  m ^= m << 32;
  return m;
}
inline constexpr ull add(ull a, const ull b) noexcept{
  if((a += b) >= m) a -= m;
  return a;
}
inline constexpr ull mul(const ull a, const ull b) noexcept{
  const __uint128_t c = (__uint128_t)a * b;
  return add(c >> 61, c & m);
}
inline constexpr ull fma(const ull a, const ull b, const ull c) noexcept{
  const __uint128_t d = (__uint128_t)a * b + c;
  return add(d >> 61, d & m);
}

template <class Key, int logn>
struct HashSet {
  private:
  static constexpr uint N = 1 << logn;
  #define rem(x) ((x) & (w - 1U))
  Key *keys;
  ull *flag;
  const ull r;
  static constexpr uint shift = 64 - logn;
  public:
  constexpr HashSet() : keys(new Key[N]), flag(new ull[N/w]()), r(gen_rnd()){}
  ~HashSet(){
    delete[] keys;
    delete[] flag;
  }
  inline constexpr void set(const Key &i) noexcept{
    uint hash = (ull(i) * r) >> shift;
    while(true){
      if(!(flag[hash/w] >> rem(hash) & 1U)){
        keys[hash] = i;
        flag[hash/w] |= 1ULL << rem(hash);
        return;
      }
      if(keys[hash] == i) return;
      hash = (hash + 1) & (N - 1);
    }
  }
  inline constexpr bool find(const Key &i) const noexcept{
    uint hash = (ull(i) * r) >> shift;
    while(true){
      if(!(flag[hash/w] >> rem(hash) & 1U)) return false;
      if(keys[hash] == i) return true;
      hash = (hash + 1) & (N - 1);
    }
  }
  #undef rem
};

template <class Key, class Val, int logn>
struct HashMap {
  private:
  static constexpr uint N = 1 << logn;
  #define rem(x) ((x) & (w - 1U))
  Key *keys;
  Val *vals;
  ull *flag;
  const ull r;
  static constexpr uint shift = 64 - logn;
  public:
  constexpr HashMap() : keys(new Key[N]), vals(new Val[N]), flag(new ull[N/w]()), r(gen_rnd()){}
  ~HashMap(){
    delete[] keys;
    delete[] vals;
    delete[] flag;
  }
  inline constexpr void set(const Key &i, const Val v) noexcept{
    uint hash = (ull(i) * r) >> shift;
    while(true){
      if(!(flag[hash/w] >> rem(hash) & 1U)){
        keys[hash] = i;
        flag[hash/w] |= 1ULL << rem(hash);
        vals[hash] = v;
        return;
      }
      if(keys[hash] == i){
        vals[hash] = v;
        return;
      }
      hash = (hash + 1) & (N - 1);
    }
  }
  inline constexpr Val get(const Key &i) noexcept{
    uint hash = (ull(i) * r) >> shift;
    while(true){
      if(!(flag[hash/w] >> rem(hash) & 1U)){
        assert(0);
      }
      if(keys[hash] == i) return vals[hash];
      hash = (hash + 1) & (N - 1);
    }
  }
  inline constexpr bool find(const Key &i) const noexcept{
    uint hash = (ull(i) * r) >> shift;
    while(true){
      if(!(flag[hash/w] >> rem(hash) & 1U)) return false;
      if(keys[hash] == i) return true;
      hash = (hash + 1) & (N - 1);
    }
  }
  #undef rem
};


template <int max_len>
struct Hash {
  const ull base;
  std::vector<ull> h;
  constexpr Hash(const ull base, const ull power[]) : base(base), power(power){}
  inline constexpr ull query(const int l, const int r) const noexcept{
    assert(max_len >= r - l);
    assert(0 <= l && l <= r && r < (int)h.size());
    return add(h[r], m - mul(h[l], power[r - l]));
  }
  // query(i, i+max_len)
  inline constexpr ull query(const int i) const noexcept{
    return add(h[i+max_len], m - mul(h[i], power[max_len]));
  }
  inline int size() const noexcept{ return h.size(); }
  private:
  const ull *power;
};

template <int max_len>
struct RollingHash {
  static constexpr ull mod = m;
  const ull base;
  constexpr RollingHash() : base(rnd()){
    power[0] = 1;
    for(int i = 0; i < max_len; i++){
      power[i + 1] = mul(power[i], base);
    }
  }
  template <class T>
  constexpr Hash<max_len> gen(const std::vector<T> &s) const noexcept{
    const int len = s.size();
    Hash<max_len> hash(base, power);
    hash.h.resize(len + 1);
    for(int i = 0; i < len; i++){
      if(s[i] < 0) hash.h[i+1] = fma(hash.h[i], base, (long long)m + (long long)s[i]);
      else hash.h[i+1] = fma(hash.h[i], base, s[i]);
    }
    return hash;
  }
  private:
  ull power[max_len + 1];
  constexpr ull rnd() const noexcept{
    ull b = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    b ^= b >> 16;
    b ^= b << 32;
    return 1930499541427916139ULL;
    //return b % (m - 2) + 2;
  }
};

}; // namespace HashImpl