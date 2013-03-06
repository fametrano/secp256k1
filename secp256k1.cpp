#include <stdint.h>
#include <string>
#include <stdio.h>
#include <openssl/bn.h>
#include <assert.h>

// #define VERIFY_MAGNITUDE 1

namespace secp256k1 {


/** Implements arithmetic modulo FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFE FFFFFC2F,
 *  represented as 5 uint64_t's in base 2^52. The values are allowed to contain >52 each. In particular,
 *  each FieldElem has a 'magnitude' associated with it. Internally, a magnitude M means each element
 *  is at most M*(2^53-1), except the most significant one, which is limited to M*(2^49-1). All operations
 *  accept any input with magnitude at most M, and have different rules for propagating magnitude to their
 *  output.
 */
class FieldElem {
private:
    // X = sum(i=0..4, elem[i]*2^52)
    uint64_t n[5];
#ifdef VERIFY_MAGNITUDE
    int magnitude;
#endif

public:

    /** Creates a constant field element. Magnitude=1 */
    FieldElem(int x = 0) {
        n[0] = x;
        n[1] = n[2] = n[3] = n[4] = 0;
#ifdef VERIFY_MAGNITUDE
        magnitude = 1;
#endif
    }

    /** Normalizes the internal representation entries. Magnitude=1 */
    void Normalize() {
        uint64_t c;
        if (n[0] > 0xFFFFFFFFFFFFFULL || n[1] > 0xFFFFFFFFFFFFFULL || n[2] > 0xFFFFFFFFFFFFFULL || n[3] > 0xFFFFFFFFFFFFFULL || n[4] > 0xFFFFFFFFFFFFULL) {
            c = n[0];
            uint64_t t0 = c & 0xFFFFFFFFFFFFFULL;
            c = (c >> 52) + n[1];
            uint64_t t1 = c & 0xFFFFFFFFFFFFFULL;
            c = (c >> 52) + n[2];
            uint64_t t2 = c & 0xFFFFFFFFFFFFFULL;
            c = (c >> 52) + n[3];
            uint64_t t3 = c & 0xFFFFFFFFFFFFFULL;
            c = (c >> 52) + n[4];
            uint64_t t4 = c & 0x0FFFFFFFFFFFFULL;
            c >>= 48;
            if (c) {
                c = c * 0x1000003D1ULL + t0;
                t0 = c & 0xFFFFFFFFFFFFFULL;
                c = (c >> 52) + t1;
                t1 = c & 0xFFFFFFFFFFFFFULL;
                c = (c >> 52) + t2;
                t2 = c & 0xFFFFFFFFFFFFFULL;
                c = (c >> 52) + t3;
                t3 = c & 0xFFFFFFFFFFFFFULL;
                c = (c >> 52) + t4;
                t4 = c & 0x0FFFFFFFFFFFFULL;
                c >>= 48;
            }
            n[0] = t0; n[1] = t1; n[2] = t2; n[3] = t3; n[4] = t4;
        }
        if (n[4] == 0xFFFFFFFFFFFFULL && n[3] == 0xFFFFFFFFFFFFFULL && n[2] == 0xFFFFFFFFFFFFFULL && n[1] == 0xFFFFFFFFFFFFF && n[0] >= 0xFFFFEFFFFFC2FULL) {
            n[4] = 0;
            n[3] = 0;
            n[2] = 0;
            n[1] = 0;
            n[0] -= 0xFFFFEFFFFFC2FULL;
        }
#ifdef VERIFY_MAGNITUDE
        magnitude = 1;
#endif
    }

    bool IsZero() {
        Normalize();
        return n[0] == 0 && n[1] == 0 && n[2] == 0 && n[3] == 0 && n[4] == 0;
    }

    bool friend operator==(FieldElem &a, FieldElem &b) {
        a.Normalize();
        b.Normalize();
        return a.n[0] == b.n[0] && a.n[1] == b.n[1] && a.n[2] == b.n[2] && a.n[3] == b.n[3] && a.n[4] == b.n[4];
    }

    void Get(uint64_t *out) {
        Normalize();
        out[0] = n[0] | (n[1] << 52);
        out[1] = (n[1] >> 12) | (n[2] << 40);
        out[2] = (n[2] >> 24) | (n[3] << 28);
        out[3] = (n[3] >> 36) | (n[4] << 16);
    }

    void Set(const uint64_t *in) {
        n[0] = in[0] & 0xFFFFFFFFFFFFFULL;
        n[1] = ((in[0] >> 52) | (in[1] << 12)) & 0xFFFFFFFFFFFFFULL;
        n[2] = ((in[1] >> 40) | (in[2] << 24)) & 0xFFFFFFFFFFFFFULL;
        n[3] = ((in[2] >> 28) | (in[3] << 36)) & 0xFFFFFFFFFFFFFULL;
        n[4] = (in[3] >> 16);
#ifdef VERIFY_MAGNITUDE
        magnitude = 1;
#endif
    }

    /** Set a FieldElem to be the negative of another. Increases magnitude by one. */
    void SetNeg(const FieldElem &a, int magnitudeIn) {
#ifdef VERIFY_MAGNITUDE
        assert(a.magnitude <= magnitudeIn);
        magnitude = magnitudeIn + 1;
#endif
        n[0] = 0xFFFFEFFFFFC2FULL * (magnitudeIn + 1) - a.n[0];
        n[1] = 0xFFFFFFFFFFFFFULL * (magnitudeIn + 1) - a.n[1];
        n[2] = 0xFFFFFFFFFFFFFULL * (magnitudeIn + 1) - a.n[2];
        n[3] = 0xFFFFFFFFFFFFFULL * (magnitudeIn + 1) - a.n[3];
        n[4] = 0x0FFFFFFFFFFFFULL * (magnitudeIn + 1) - a.n[4];
    }

    /** Multiplies this FieldElem with an integer constant. Magnitude is multiplied by v */
    void operator*=(int v) {
#ifdef VERIFY_MAGNITUDE
        magnitude *= v;
#endif
        n[0] *= v;
        n[1] *= v;
        n[2] *= v;
        n[3] *= v;
        n[4] *= v;
    }

    void operator+=(const FieldElem &a) {
#ifdef VERIFY_MAGNITUDE
        magnitude += a.magnitude;
#endif
        n[0] += a.n[0];
        n[1] += a.n[1];
        n[2] += a.n[2];
        n[3] += a.n[3];
        n[4] += a.n[4];
    }

    /** Set this FieldElem to be the multiplication of two others. Magnitude=1 */
    void SetMult(const FieldElem &a, const FieldElem &b) {
#ifdef VERIFY_MAGNITUDE
        assert(a.magnitude <= 8);
        assert(b.magnitude <= 8);
#endif
        __int128 c = (__int128)a.n[0] * b.n[0];
        uint64_t t0 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 0FFFFFFFFFFFFFE0
        c = c + (__int128)a.n[0] * b.n[1] +
                (__int128)a.n[1] * b.n[0];
        uint64_t t1 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 20000000000000BF
        c = c + (__int128)a.n[0] * b.n[2] +
                (__int128)a.n[1] * b.n[1] +
                (__int128)a.n[2] * b.n[0];
        uint64_t t2 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 30000000000001A0
        c = c + (__int128)a.n[0] * b.n[3] +
                (__int128)a.n[1] * b.n[2] +
                (__int128)a.n[2] * b.n[1] +
                (__int128)a.n[3] * b.n[0];
        uint64_t t3 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 4000000000000280
        c = c + (__int128)a.n[0] * b.n[4] +
                (__int128)a.n[1] * b.n[3] +
                (__int128)a.n[2] * b.n[2] +
                (__int128)a.n[3] * b.n[1] +
                (__int128)a.n[4] * b.n[0];
        uint64_t t4 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 320000000000037E
        c = c + (__int128)a.n[1] * b.n[4] +
                (__int128)a.n[2] * b.n[3] +
                (__int128)a.n[3] * b.n[2] +
                (__int128)a.n[4] * b.n[1];
        uint64_t t5 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 22000000000002BE
        c = c + (__int128)a.n[2] * b.n[4] +
                (__int128)a.n[3] * b.n[3] +
                (__int128)a.n[4] * b.n[2];
        uint64_t t6 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 12000000000001DE
        c = c + (__int128)a.n[3] * b.n[4] +
                (__int128)a.n[4] * b.n[3];
        uint64_t t7 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 02000000000000FE
        c = c + (__int128)a.n[4] * b.n[4];
        uint64_t t8 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 001000000000001E
        uint64_t t9 = c;
        c = t0 + (__int128)t5 * 0x1000003D10ULL;
        t0 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 0000001000003D10
        c = c + t1 + (__int128)t6 * 0x1000003D10ULL;
        t1 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 0000001000003D10
        c = c + t2 + (__int128)t7 * 0x1000003D10ULL;
        n[2] = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 0000001000003D10
        c = c + t3 + (__int128)t8 * 0x1000003D10ULL;
        n[3] = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 0000001000003D10
        c = c + t4 + (__int128)t9 * 0x1000003D10ULL;
        n[4] = c & 0x0FFFFFFFFFFFFULL; c = c >> 48; // c max 000001000003D110
        c = t0 + (__int128)c * 0x1000003D1ULL;
        n[0] = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 1000008
        n[1] = t1 + c;
#ifdef VERIFY_MAGNITUDE
        magnitude = 1;
#endif
    }

    /** Set this FieldElem to be the square of another. Magnitude=1 */
    void SetSquare(const FieldElem &a) {
#ifdef VERIFY_MAGNITUDE
        assert(a.magnitude <= 8);
#endif
        __int128 c = (__int128)a.n[0] * a.n[0];
        uint64_t t0 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 0FFFFFFFFFFFFFE0
        c = c + (__int128)(a.n[0]*2) * a.n[1];
        uint64_t t1 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 20000000000000BF
        c = c + (__int128)(a.n[0]*2) * a.n[2] +
                (__int128)a.n[1] * a.n[1];
        uint64_t t2 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 30000000000001A0
        c = c + (__int128)(a.n[0]*2) * a.n[3] +
                (__int128)(a.n[1]*2) * a.n[2];
        uint64_t t3 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 4000000000000280
        c = c + (__int128)(a.n[0]*2) * a.n[4] +
                (__int128)(a.n[1]*2) * a.n[3] +
                (__int128)a.n[2] * a.n[2];
        uint64_t t4 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 320000000000037E
        c = c + (__int128)(a.n[1]*2) * a.n[4] +
                (__int128)(a.n[2]*2) * a.n[3];
        uint64_t t5 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 22000000000002BE
        c = c + (__int128)(a.n[2]*2) * a.n[4] +
                (__int128)a.n[3] * a.n[3];
        uint64_t t6 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 12000000000001DE
        c = c + (__int128)(a.n[3]*2) * a.n[4];
        uint64_t t7 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 02000000000000FE
        c = c + (__int128)a.n[4] * a.n[4];
        uint64_t t8 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 001000000000001E
        uint64_t t9 = c;
        c = t0 + (__int128)t5 * 0x1000003D10ULL;
        t0 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 0000001000003D10
        c = c + t1 + (__int128)t6 * 0x1000003D10ULL;
        t1 = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 0000001000003D10
        c = c + t2 + (__int128)t7 * 0x1000003D10ULL;
        n[2] = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 0000001000003D10
        c = c + t3 + (__int128)t8 * 0x1000003D10ULL;
        n[3] = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 0000001000003D10
        c = c + t4 + (__int128)t9 * 0x1000003D10ULL;
        n[4] = c & 0x0FFFFFFFFFFFFULL; c = c >> 48; // c max 000001000003D110
        c = t0 + (__int128)c * 0x1000003D1ULL;
        n[0] = c & 0xFFFFFFFFFFFFFULL; c = c >> 52; // c max 1000008
        n[1] = t1 + c;
#ifdef VERIFY_MAGNITUDE
        assert(a.magnitude <= 8);
#endif
    }

    /** Set this to be the (modular) square root of another FieldElem. Magnitude=1 */
    void SetSquareRoot(const FieldElem &a) {
        // calculate a^p, with p={15,780,1022,1023}
        FieldElem a2; a2.SetSquare(a);
        FieldElem a3; a3.SetMult(a2,a);
        FieldElem a6; a6.SetSquare(a3);
        FieldElem a12; a12.SetSquare(a6);
        FieldElem a15; a15.SetMult(a12,a3);
        FieldElem a30; a30.SetSquare(a15);
        FieldElem a60; a60.SetSquare(a30);
        FieldElem a120; a120.SetSquare(a60);
        FieldElem a240; a240.SetSquare(a120);
        FieldElem a255; a255.SetMult(a240,a15);
        FieldElem a510; a510.SetSquare(a255);
        FieldElem a750; a750.SetMult(a510,a240);
        FieldElem a780; a780.SetMult(a750,a30);
        FieldElem a1020; a1020.SetSquare(a510);
        FieldElem a1022; a1022.SetMult(a1020,a2);
        FieldElem a1023; a1023.SetMult(a1022,a);
        FieldElem x = a15;
        for (int i=0; i<21; i++) {
            for (int j=0; j<10; j++) x.SetSquare(x);
            x.SetMult(x,a1023);
        }
        for (int j=0; j<10; j++) x.SetSquare(x);
        x.SetMult(x,a1022);
        for (int i=0; i<2; i++) {
            for (int j=0; j<10; j++) x.SetSquare(x);
            x.SetMult(x,a1023);
        }
        for (int j=0; j<10; j++) x.SetSquare(x);
        SetMult(x,a780);
    }

    bool IsOdd() {
        Normalize();
        return n[0] & 1;
    }

    /** Set this to be the (modular) inverse of another FieldElem. Magnitude=1 */
    void SetInverse(const FieldElem &a) {
        // calculate a^p, with p={45,63,1019,1023}
        FieldElem a2; a2.SetSquare(a);
        FieldElem a3; a3.SetMult(a2,a);
        FieldElem a4; a4.SetSquare(a);
        FieldElem a5; a5.SetMult(a4,a);
        FieldElem a10; a10.SetSquare(a5);
        FieldElem a11; a11.SetMult(a10,a);
        FieldElem a21; a21.SetMult(a11,a10);
        FieldElem a42; a42.SetSquare(a21);
        FieldElem a45; a45.SetMult(a42,a3);
        FieldElem a63; a63.SetMult(a42,a21);
        FieldElem a126; a126.SetSquare(a63);
        FieldElem a252; a252.SetSquare(a126);
        FieldElem a504; a504.SetSquare(a252);
        FieldElem a1008; a1008.SetSquare(a504);
        FieldElem a1019; a1019.SetMult(a1008,a11);
        FieldElem a1023; a1023.SetMult(a1019,a4);
        FieldElem x = a63;
        for (int i=0; i<21; i++) {
            for (int j=0; j<10; j++) x.SetSquare(x);
            x.SetMult(x,a1023);
        }
        for (int j=0; j<10; j++) x.SetSquare(x);
        x.SetMult(x,a1019);
        for (int i=0; i<2; i++) {
            for (int j=0; j<10; j++) x.SetSquare(x);
            x.SetMult(x,a1023);
        }
        for (int j=0; j<10; j++) x.SetSquare(x);
        SetMult(x,a45);
    }

    std::string ToString() {
        uint64_t tmp[4];
        Get(tmp);
        std::string ret;
        for (int i=63; i>=0; i--) {
            int val = (tmp[i/16] >> ((i%16)*4)) & 0xF;
            static const char *c = "0123456789ABCDEF";
            ret += c[val];
        }
        return ret;
    }

    void SetHex(const std::string &str) {
        uint64_t tmp[4] = {0,0,0,0};
        static const int cvt[256] = {0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
                                     0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
                                     0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
                                     0, 1, 2, 3, 4, 5, 6,7,8,9,0,0,0,0,0,0,
                                     0,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,
                                     0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
                                     0,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,
                                     0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
                                     0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
                                     0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
                                     0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
                                     0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
                                     0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
                                     0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
                                     0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0,
                                     0, 0, 0, 0, 0, 0, 0,0,0,0,0,0,0,0,0,0};
        for (int i=0; i<64; i++) {
            if (str.length() > (63-i))
                tmp[i/16] |= (uint64_t)cvt[(unsigned char)str[63-i]] << ((i%16)*4);
        }
        Set(tmp);
    }
};

template<typename F> class GroupElemJac;

template<typename F> class GroupElem {
protected:
    bool fInfinity;
    F x;
    F y;

public:

    void SetXY(const F &xin, const F &yin) {
        fInfinity = false;
        this->x = xin;
        this->y = yin;
    }

    /** Creates the point at infinity */
    GroupElem() {
        this->fInfinity = true;
    }

    /** Creates the point with given affine coordinates */
    GroupElem(const F &xin, const F &yin) {
        SetXY(xin,yin);
    }

    /** Checks whether this is the point at infinity */
    bool IsInfinity() const {
        return this->fInfinity;
    }

    void SetNeg(GroupElem<F> &p) {
        this->fInfinity = p.fInfinity;
        this->x = p.x;
        p.y.Normalize();
        this->y.SetNeg(p.y, 1);
    }

    std::string ToString() {
        if (this->fInfinity)
            return "(inf)";
        return "(" + this->x.ToString() + "," + this->y.ToString() + ")";
    }

    friend class GroupElemJac<F>;
};

template<typename F> class GroupElemJac : public GroupElem<F> {
protected:
    F z;

public:
    /** Creates the point at infinity */
    GroupElemJac() : GroupElem<F>(), z(1) {}

    /** Creates the point with given affine coordinates */
    GroupElemJac(const F &xin, const F &yin) : GroupElem<F>(xin,yin), z(1) {}

    /** Checks whether this is a non-infinite point on the curve */
    bool IsValid() {
        if (this->IsInfinity())
            return false;
        // y^2 = x^3 + 7
        // (Y/Z^3)^2 = (X/Z^2)^3 + 7
        // Y^2 / Z^6 = X^3 / Z^6 + 7
        // Y^2 = X^3 + 7*Z^6
        F y2; y2.SetSquare(this->y);
        F x3; x3.SetSquare(this->x); x3.SetMult(x3,this->x);
        F z2; z2.SetSquare(this->z);
        F z6; z6.SetSquare(z2); z6.SetMult(z6,z2);
        z6 *= 7;
        x3 += z6;
        return y2 == x3;
    }

    /** Returns the affine coordinates of this point */
    void GetAffine(GroupElem<F> &aff) {
        z.SetInverse(z);
        F z2;
        z2.SetSquare(z);
        F z3;
        z3.SetMult(z,z2);
        this->x.SetMult(this->x,z2);
        this->y.SetMult(this->y,z3);
        this->z = F(1);
        aff.SetXY(this->x,this->y);
    }

    /** Sets this point to have a given X coordinate & given Y oddness */
    void SetCompressed(const F &xin, bool fOdd) {
        this->x = xin;
        F x2; x2.SetSquare(this->x);
        F x3; x3.SetMult(this->x,x2);
        this->fInfinity = false;
        F c(7);
        c += x3;
        this->y.SetSquareRoot(c);
        this->z = F(1);
        if (this->y.IsOdd() != fOdd)
            this->y.SetNeg(this->y,1);
    }

    /** Sets this point to be the EC double of another */
    void SetDouble(const GroupElemJac<F> &p) {
        if (p.fInfinity || this->y.IsZero()) {
            this->fInfinity = true;
            return;
        }

        F t1,t2,t3,t4,t5;
        this->z.SetMult(p.y,p.z);
        this->z *= 2;                // Z' = 2*Y*Z (2)
        t1.SetSquare(p.x);
        t1 *= 3;               // T1 = 3*X^2 (3)
        t2.SetSquare(t1);      // T2 = 9*X^4 (1)
        t3.SetSquare(p.y);
        t3 *= 2;               // T3 = 2*Y^2 (2)
        t4.SetSquare(t3);
        t4 *= 2;               // T4 = 8*Y^4 (2)
        t3.SetMult(p.x,t3);      // T3 = 2*X*Y^2 (1)
        this->x = t3;
        this->x *= 4;                // X' = 8*X*Y^2 (4)
        this->x.SetNeg(this->x,4);         // X' = -8*X*Y^2 (5)
        this->x += t2;               // X' = 9*X^4 - 8*X*Y^2 (6)
        t2.SetNeg(t2,1);       // T2 = -9*X^4 (2)
        t3 *= 6;               // T3 = 12*X*Y^2 (6)
        t3 += t2;              // T3 = 12*X*Y^2 - 9*X^4 (8)
        this->y.SetMult(t1,t3);      // Y' = 36*X^3*Y^2 - 27*X^6 (1)
        t2.SetNeg(t4,2);       // T2 = -8*Y^4 (3)
        this->y += t2;               // Y' = 36*X^3*Y^2 - 27*X^6 - 8*Y^4 (4)
    }

    /** Sets this point to be the EC addition of two others */
    void SetAdd(const GroupElemJac<F> &p, const GroupElemJac<F> &q) {
        if (p.fInfinity) {
            *this = q;
            return;
        }
        if (q.fInfinity) {
            *this = p;
            return;
        }
        this->fInfinity = false;
        const F &x1 = p.x, &y1 = p.y, &z1 = p.z, &x2 = q.x, &y2 = q.y, &z2 = q.z;
        F z22; z22.SetSquare(z2);
        F z12; z12.SetSquare(z1);
        F u1; u1.SetMult(x1, z22);
        F u2; u2.SetMult(x2, z12);
        F s1; s1.SetMult(y1, z22); s1.SetMult(s1, z2);
        F s2; s2.SetMult(y2, z12); s2.SetMult(s2, z1);
        if (u1 == u2) {
            if (s1 == s2) {
                SetDouble(p);
            } else {
                this->fInfinity = true;
            }
            return;
        }
        F h; h.SetNeg(u1,1); h += u2;
        F r; r.SetNeg(s1,1); r += s2;
        F r2; r2.SetSquare(r);
        F h2; h2.SetSquare(h);
        F h3; h3.SetMult(h,h2);
        this->z.SetMult(z1,z2); this->z.SetMult(z, h);
        F t; t.SetMult(u1,h2);
        this->x = t; this->x *= 2; this->x += h3; this->x.SetNeg(this->x,3); this->x += r2;
        this->y.SetNeg(this->x,5); this->y += t; this->y.SetMult(this->y,r);
        h3.SetMult(h3,s1); h3.SetNeg(h3,1);
        this->y += h3;
    }

    /** Sets this point to be the EC addition of two others (one of which is in affine coordinates) */
    void SetAdd(const GroupElemJac<F> &p, const GroupElem<F> &q) {
        if (p.fInfinity) {
            this->x = q.x;
            this->y = q.y;
            this->fInfinity = q.fInfinity;
            this->z = F(1);
            return;
        }
        if (q.fInfinity) {
            *this = p;
            return;
        }
        this->fInfinity = false;
        const F &x1 = p.x, &y1 = p.y, &z1 = p.z, &x2 = q.x, &y2 = q.y;
        F z12; z12.SetSquare(z1);
        F u1 = x1; u1.Normalize();
        F u2; u2.SetMult(x2, z12);
        F s1 = y1; s1.Normalize();
        F s2; s2.SetMult(y2, z12); s2.SetMult(s2, z1);
        if (u1 == u2) {
            if (s1 == s2) {
                SetDouble(p);
            } else {
                this->fInfinity = true;
            }
            return;
        }
        F h; h.SetNeg(u1,1); h += u2;
        F r; r.SetNeg(s1,1); r += s2;
        F r2; r2.SetSquare(r);
        F h2; h2.SetSquare(h);
        F h3; h3.SetMult(h,h2);
        this->z = p.z; this->z.SetMult(z, h);
        F t; t.SetMult(u1,h2);
        this->x = t; this->x *= 2; this->x += h3; this->x.SetNeg(this->x,3); this->x += r2;
        this->y.SetNeg(this->x,5); this->y += t; this->y.SetMult(this->y,r);
        h3.SetMult(h3,s1); h3.SetNeg(h3,1);
        this->y += h3;
    }

    std::string ToString() {
        GroupElem<F> aff;
        this->GetAffine(aff);
        return aff.ToString();
    }
};

}

using namespace secp256k1;

int main() {
    FieldElem f1,f2;
    f1.SetHex("8b30bbe9ae2a990696b22f670709dff3727fd8bc04d3362c6c7bf458e2846004");
    f2.SetHex("a357ae915c4a65281309edf20504740f1eb3333990216b4f81063cb65f2f7e0f");
    GroupElemJac<FieldElem> g1; g1.SetCompressed(f1,false);
    GroupElemJac<FieldElem> g2; g2.SetCompressed(f2,false);
    printf("g1: %s (%s)\n", g1.ToString().c_str(), g1.IsValid() ? "ok" : "fail");
    printf("g2: %s (%s)\n", g2.ToString().c_str(), g2.IsValid() ? "ok" : "fail");
    GroupElem<FieldElem> g2a; g2.GetAffine(g2a);
    printf("g2a:%s\n", g2a.ToString().c_str());
    GroupElemJac<FieldElem> x1 = g1, x2 = g1;
    for (int i=0; i<100000000; i++) {
      x1.SetAdd(x1,g2a);
    }
    printf("res:%s (%s)\n", x1.ToString().c_str(), x1.IsValid() ? "ok" : "fail");
    return 0;
}
