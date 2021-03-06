#ifndef EIGEN_COMPLEX_WASM_H
#define EIGEN_COMPLEX_WASM_H

namespace Eigen {
namespace internal {

struct Packet2cf
{
  EIGEN_STRONG_INLINE Packet2cf() {}
  EIGEN_STRONG_INLINE explicit Packet2cf(const __f32x4& a) : v(a) {}
    __f32x4 v;
};

template<> struct packet_traits<std::complex<float> >  : default_packet_traits
{
  typedef Packet2cf type;
  typedef Packet2cf half;
  enum {
    Vectorizable = 1,
    AlignedOnScalar = 1,
    size = 2,
    HasHalfPacket = 0,

    HasAdd    = 1,
    HasSub    = 1,
    HasMul    = 1,
    HasDiv    = 0,
    HasConj   = 1,
    HasNegate = 1,
    HasAbs    = 0,
    HasAbs2   = 0,
    HasMin    = 0,
    HasMax    = 0,
    HasSetLinear = 0,
    HasBlend = 0
  };
};

template<> struct unpacket_traits<Packet2cf> {
    typedef std::complex<float> type;
    enum {
        size=2,
        alignment=Aligned16,
        vectorizable=true,
        masked_load_available=false,
        masked_store_available=false
    };
    typedef Packet2cf half;
};

template<> EIGEN_STRONG_INLINE Packet2cf padd<Packet2cf>(const Packet2cf& a, const Packet2cf& b) { return Packet2cf(wasm_f32x4_add(a.v, b.v)); }
template<> EIGEN_STRONG_INLINE Packet2cf psub<Packet2cf>(const Packet2cf& a, const Packet2cf& b) { return Packet2cf(wasm_f32x4_sub(a.v, b.v)); }
template<> EIGEN_STRONG_INLINE Packet2cf pnegate(const Packet2cf& a) { return Packet2cf(wasm_f32x4_neg(a.v)); }

template<> EIGEN_STRONG_INLINE Packet2cf pconj(const Packet2cf& a) {
    // Complex conjugate
    // conj(a + bi) = a - bi
    const v128_t conjMask = wasm_f32x4_const(0.0f, -0.0f, 0.0f, -0.0f);
    return Packet2cf(wasm_v128_xor(a.v, conjMask));
}

template<> EIGEN_STRONG_INLINE Packet2cf pmul<Packet2cf>(const Packet2cf& a, const Packet2cf& b) {
    // Complex multiplication on two sets of two numbers.
    // (a + xi) * (b + yi) = (ab - xy) + (ay + bx)i

    // WASM shuffling only available in chunks of 8 bytes so for a given complex pair, the byte indices are:
    // a = 0, 1, 2, 3
    // x = 4, 5, 6, 7
    // b = 8, 9, 10, 11
    // y = 12, 13, 14, 15

    // Reals of first multiplied by second
    Packet4f reals = pmul<Packet4f>(
            wasm_v8x16_shuffle(a.v, a.v, 0, 1, 2, 3, 0, 1, 2, 3, 8, 9, 10, 11, 8, 9, 10, 11),
            b.v
    );

    // Imaginary of first multiplied by second
    const v128_t mask = wasm_f32x4_const(-0.0f, 0.0f, -0.0f, 0.0f);
    Packet4f imags = wasm_v128_xor(pmul<Packet4f>(
            wasm_v8x16_shuffle(a.v, a.v, 4, 5, 6, 7, 4, 5, 6, 7, 12, 13, 14, 15, 12, 13, 14, 15),
            wasm_v8x16_shuffle(b.v, b.v, 4, 5, 6, 7, 0, 1, 2, 3, 12, 13, 14, 15, 8, 9, 10, 11)
    ), mask);

    return Packet2cf(padd<Packet4f>(reals, imags));
}

template<> EIGEN_STRONG_INLINE Packet2cf ptrue<Packet2cf>(const Packet2cf& a) { return Packet2cf(ptrue(Packet4f(a.v))); }
template<> EIGEN_STRONG_INLINE Packet2cf pnot<Packet2cf>(const Packet2cf& a) { return Packet2cf(pnot(Packet4f(a.v))); }

template<> EIGEN_STRONG_INLINE Packet2cf pand<Packet2cf>(const Packet2cf& a, const Packet2cf& b) { return Packet2cf(wasm_v128_and(a.v,b.v)); }
template<> EIGEN_STRONG_INLINE Packet2cf por<Packet2cf>(const Packet2cf& a, const Packet2cf& b) { return Packet2cf(wasm_v128_or(a.v,b.v)); }
template<> EIGEN_STRONG_INLINE Packet2cf pxor<Packet2cf>(const Packet2cf& a, const Packet2cf& b) { return Packet2cf(wasm_v128_xor(a.v,b.v)); }
template<> EIGEN_STRONG_INLINE Packet2cf pandnot<Packet2cf>(const Packet2cf& a, const Packet2cf& b) { return Packet2cf(wasm_v128_and(a.v, wasm_v128_not(b.v))); }

template<> EIGEN_STRONG_INLINE Packet2cf pload<Packet2cf>(const std::complex<float>* from) { EIGEN_DEBUG_ALIGNED_LOAD return Packet2cf(pload<Packet4f>(&numext::real_ref(*from))); }
template<> EIGEN_STRONG_INLINE Packet2cf ploadu<Packet2cf>(const std::complex<float>* from) { EIGEN_DEBUG_UNALIGNED_LOAD return Packet2cf(ploadu<Packet4f>(&numext::real_ref(*from))); }

template<> EIGEN_STRONG_INLINE Packet2cf pset1<Packet2cf>(const std::complex<float>& from) {
    return Packet2cf(wasm_f32x4_make(
            from.real(),
            from.imag(),
            from.real(),
            from.imag()
    ));
}

template<> EIGEN_STRONG_INLINE Packet2cf ploaddup<Packet2cf>(const std::complex<float>* from) { return pset1<Packet2cf>(*from); }

template<> EIGEN_STRONG_INLINE void pstore<std::complex<float>>(std::complex<float> * to, const Packet2cf& from) { EIGEN_DEBUG_ALIGNED_STORE pstore(&numext::real_ref(*to), Packet4f(from.v)); }
template<> EIGEN_STRONG_INLINE void pstoreu<std::complex<float>>(std::complex<float> * to, const Packet2cf& from) { EIGEN_DEBUG_UNALIGNED_STORE pstoreu(&numext::real_ref(*to), Packet4f(from.v)); }

template<> EIGEN_STRONG_INLINE std::complex<float> pfirst<Packet2cf>(const Packet2cf& a) {
  std::complex<float> res(
          wasm_f32x4_extract_lane(a.v, 0),
          wasm_f32x4_extract_lane(a.v, 1)
  );
  return res;
}

template<> EIGEN_STRONG_INLINE std::complex<float> predux<Packet2cf>(const Packet2cf& a)
{
  std::complex<float> res(
      wasm_f32x4_extract_lane(a.v, 0) + wasm_f32x4_extract_lane(a.v, 2),
      wasm_f32x4_extract_lane(a.v, 1) + wasm_f32x4_extract_lane(a.v, 3)
  );
  return res;
}

template<> EIGEN_STRONG_INLINE std::complex<float> predux_mul<Packet2cf>(const Packet2cf& a)
{
    float real = (wasm_f32x4_extract_lane(a.v, 0) * wasm_f32x4_extract_lane(a.v, 3))
        - (wasm_f32x4_extract_lane(a.v, 1) * wasm_f32x4_extract_lane(a.v, 3));

    float imag = (wasm_f32x4_extract_lane(a.v, 0) * wasm_f32x4_extract_lane(a.v, 3))
                 + (wasm_f32x4_extract_lane(a.v, 2) * wasm_f32x4_extract_lane(a.v, 1));

    std::complex<float> res(real, imag);
    return res;
}

template<> struct conj_helper<Packet2cf, Packet2cf, false,true>
{
  EIGEN_STRONG_INLINE Packet2cf pmadd(const Packet2cf& x, const Packet2cf& y, const Packet2cf& c) const
  { return padd(pmul(x,y),c); }

  EIGEN_STRONG_INLINE Packet2cf pmul(const Packet2cf& a, const Packet2cf& b) const
  {
    return internal::pmul(a, pconj(b));
  }
};

template<> struct conj_helper<Packet2cf, Packet2cf, true,false>
{
  EIGEN_STRONG_INLINE Packet2cf pmadd(const Packet2cf& x, const Packet2cf& y, const Packet2cf& c) const
  { return padd(pmul(x,y),c); }

  EIGEN_STRONG_INLINE Packet2cf pmul(const Packet2cf& a, const Packet2cf& b) const
  {
    return internal::pmul(pconj(a), b);
  }
};

template<> struct conj_helper<Packet2cf, Packet2cf, true,true>
{
  EIGEN_STRONG_INLINE Packet2cf pmadd(const Packet2cf& x, const Packet2cf& y, const Packet2cf& c) const
  { return padd(pmul(x,y),c); }

  EIGEN_STRONG_INLINE Packet2cf pmul(const Packet2cf& a, const Packet2cf& b) const
  {
    return pconj(internal::pmul(a, b));
  }
};

EIGEN_MAKE_CONJ_HELPER_CPLX_REAL(Packet2cf,Packet4f)

}

// TODO - Implement division

}

#endif
