#pragma once
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
// check is crossed [a, b), [c, d)
#define is_cross(a,b,c,d) (max(a, c) < min(b, d))

namespace Math {
constexpr double b1table[32] = {
  0x1.0000000000000p+0,
  0x1.059b0d3158574p+0,
  0x1.0b5586cf9890fp+0,
  0x1.11301d0125b51p+0,
  0x1.172b83c7d517bp+0,
  0x1.1d4873168b9aap+0,
  0x1.2387a6e756238p+0,
  0x1.29e9df51fdee1p+0,
  0x1.306fe0a31b715p+0,
  0x1.371a7373aa9cbp+0,
  0x1.3dea64c123422p+0,
  0x1.44e086061892dp+0,
  0x1.4bfdad5362a27p+0,
  0x1.5342b569d4f82p+0,
  0x1.5ab07dd485429p+0,
  0x1.6247eb03a5585p+0,
  0x1.6a09e667f3bcdp+0,
  0x1.71f75e8ec5f74p+0,
  0x1.7a11473eb0187p+0,
  0x1.82589994cce13p+0,
  0x1.8ace5422aa0dbp+0,
  0x1.93737b0cdc5e5p+0,
  0x1.9c49182a3f090p+0,
  0x1.a5503b23e255dp+0,
  0x1.ae89f995ad3adp+0,
  0x1.b7f76f2fb5e47p+0,
  0x1.c199bdd85529cp+0,
  0x1.cb720dcef9069p+0,
  0x1.d5818dcfba487p+0,
  0x1.dfc97337b9b5fp+0,
  0x1.ea4afa2a490dap+0,
  0x1.f50765b6e4540p+0,
};
constexpr double b2table[32] = {
  0x1.0000000000000p+0,
  0x1.002c605e2e8cfp+0,
  0x1.0058c86da1c0ap+0,
  0x1.0085382faef83p+0,
  0x1.00b1afa5abcbfp+0,
  0x1.00de2ed0ee0f5p+0,
  0x1.010ab5b2cbd11p+0,
  0x1.0137444c9b5b5p+0,
  0x1.0163da9fb3335p+0,
  0x1.019078ad6a19fp+0,
  0x1.01bd1e77170b4p+0,
  0x1.01e9cbfe113efp+0,
  0x1.02168143b0281p+0,
  0x1.02433e494b755p+0,
  0x1.027003103b10ep+0,
  0x1.029ccf99d720ap+0,
  0x1.02c9a3e778061p+0,
  0x1.02f67ffa765e6p+0,
  0x1.032363d42b027p+0,
  0x1.03504f75ef071p+0,
  0x1.037d42e11bbccp+0,
  0x1.03aa3e170aafep+0,
  0x1.03d7411915a8ap+0,
  0x1.04044be896ab6p+0,
  0x1.04315e86e7f85p+0,
  0x1.045e78f5640b9p+0,
  0x1.048b9b35659d8p+0,
  0x1.04b8c54847a28p+0,
  0x1.04e5f72f654b1p+0,
  0x1.051330ec1a03fp+0,
  0x1.0540727fc1762p+0,
  0x1.056dbbebb786bp+0,
};
constexpr inline uint64_t exp_table(const uint64_t s) noexcept{
  const double b = b1table[s>>5&31] * b2table[s&31];
  return *(uint64_t*)&b + ((s >> 10) << 52);
}
constexpr inline double fast_exp(const double x) noexcept{
  if(x < -104.0f) return 0.0;
  if(x > 0x1.62e42ep+6f) return HUGE_VALF;
  constexpr double R = 0x3.p+51f;
  constexpr double iln2 = 0x1.71547652b82fep+10;
  constexpr double ln2h = 0x1.62e42fefc0000p-11;
  constexpr double ln2l = -0x1.c610ca86c3899p-47;
  const double k_R = x*iln2+R;
  const double t = x*iln2*(-ln2l+ln2h+x);
  const uint64_t exp_s = exp_table(*(uint64_t*)&k_R);
  return *(double*)&exp_s * ((1.0/6.0*t+1.0/2.0)*t*t+t+1);
}
} // namespace Math

using Math::fast_exp;