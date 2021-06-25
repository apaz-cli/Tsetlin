#ifndef TBITSET_INCLUDE
#define TBITSET_INCLUDE

#include <array>
#include <bit>
#include <bitset>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>

#ifndef BITNUM
#define BITNUM 64
#endif

#if BITNUM == 64
#define TINT_PRI "%" PRIu64
#define TINT_BIT_NUM 64
#define TINT_MAX 0xFFFFFFFFFFFFFFFF
#define tint uint64_t
#define tint_count(x) __builtin_popcountl(x)
static_assert(
    sizeof(uint64_t) == sizeof(unsigned long),
    "sizeof(uint64_t) != sizeof(unsigned long). Please edit TsetlinBitset.h.");
#else
#define TINT_PRI "%" PRIu32
#define TINT_BIT_NUM 32
#define TINT_MAX 0xFFFFFFFF
#define tint uint32_t
#define tint_count(x) __builtin_popcount(x)
static_assert(
    sizeof(uint32_t) == sizeof(unsigned int),
    "sizeof(uint32_t) != sizeof(unsigned int). Please edit TsetlinBitset.h.");
#endif  // BITNUM

// Because of efficiency over the STL version
template <size_t num_bits>
class TBitset {
    constexpr static size_t buf_len =
        (num_bits / TINT_BIT_NUM) + ((num_bits % TINT_BIT_NUM) != 0);

    friend class BitRef;
    // Members
    std::array<tint, buf_len> buf;

   public:
    ///////////////////
    // Inner Classes //
    ///////////////////

    // Reference class for operator[]
    class TBitRef {
       private:
        TBitset& owner;
        size_t buf_idx;
        size_t offset;

       public:
        constexpr TBitRef(TBitset& owner, size_t idx)
            : owner(owner),
              buf_idx(idx / TINT_BIT_NUM),
              offset(idx % TINT_BIT_NUM) {}
        constexpr TBitRef(TBitset& owner, size_t buf_idx, size_t offset)
            : owner(owner), buf_idx(buf_idx), offset(offset) {}
        friend class Bitset;
        TBitRef() noexcept;  // no public constructor

        // convert to bool
        operator bool() const noexcept {
            tint b = owner.buf[buf_idx];
            b &= (1 << offset);
            return b ? 1 : 0;
        }
        // assign bool
        TBitRef&
        operator=(bool x) noexcept {
            // Get the part of the buffer we're working on
            tint orig = owner.buf[buf_idx];
            // Create 0s, except for one 1 at the offset position.
            // Negate, creating 1s, except for one 0 at the offset position.
            tint negmask = ~(1 << offset);
            // Set the bit in the original at the offset to 0.
            orig &= negmask;
            // xor x to assign x to the offset bit. (xor identity)
            orig ^= (x << offset);
            // Put back in the buffer.
            owner.buf[buf_idx] = orig;

            // Return this so ops can be chained.
            return *this;
        }
        // assign bit
        TBitRef&
        operator=(const TBitRef& x) noexcept {
            // uses previous operator=()
            // and operator bool()
            *this = (bool)x;
            return *this;
        }
        // flip bit value
        TBitRef&
        flip() noexcept {
            tint elem = owner.buf[buf_idx];
            elem ^= (1 << offset);
            owner.buf[buf_idx] = elem;
            return *this;
        }
        // return inverse value
        bool
        operator~() const noexcept {
            return ~((bool)this);
        }
    };

    ////////////////
    // Main class //
    ////////////////

    // Constructors
    constexpr TBitset() noexcept
        requires((num_bits % TINT_BIT_NUM) == 0) = default;
    TBitset(bool zeros = false) noexcept
        requires((num_bits % TINT_BIT_NUM) != 0) {
        if (zeros)
            buf.fill(0);
        else if (buf_len % TINT_BIT_NUM)
            buf[buf_len - 1] = 0;
    }

    constexpr TBitset(std::bitset<num_bits> bs) noexcept {
        // TBitRef assignment operator with bitset::reference (bool) operator.
        for (size_t i = 0; i < bs.size(); i++) this[i] = bs[i];
    }

    // Member Functions
    constexpr size_t
    size() const noexcept {
        return num_bits;
    }

    std::array<bool, num_bits>&
    unravel(std::array<bool, num_bits>& space) noexcept {
        for (size_t i = 0; i < num_bits; i++) space[i] = this[i];
    }

    size_t
    count() {
        size_t sum = 0;
        for (size_t i = 0; i < buf_len; i++)
            sum += std::popcount<tint>(this->buf[i]);
        return sum;
    }

    std::array<tint, buf_len>&
    get_backing() const noexcept {
        return *(this->buf);
    }

    constexpr size_t
    backing_size() const noexcept {
        return buf_len;
    }

    // Bit access operators
    bool
    operator[](size_t pos) const noexcept {
        return (bool)TBitRef(*this, pos);
    }

    TBitRef
    operator[](size_t pos) noexcept {
        return TBitRef(*this, pos);
    }

    bool
    test(size_t pos) const {
        if (pos >= num_bits)
            throw std::out_of_range("Out of range: " + std::to_string(pos));
        return this[pos];
    }

    // reference operators
    // Inverts the bits of this bitset, placing the results in output.
    static void
    _inverse(TBitset& output, TBitset& to_invert) noexcept {
        for (size_t i = 0; i < buf_len; i++) output.buf[i] = ~to_invert.buf[i];
    }

    // Ands the bits of this and to_and, placing the result in output
    static void
    _and(TBitset& output, TBitset& to_and1, TBitset& to_and2) noexcept {
        for (size_t i = 0; i < buf_len; i++)
            output.buf[i] = to_and1.buf[i] & to_and2.buf[i];
    }

    // Ors the bits of this and to_or, placing the result in output.
    static void
    _or(TBitset& output, TBitset& to_or1, TBitset& to_or2) noexcept {
        for (size_t i = 0; i < buf_len; i++)
            output.buf[i] = to_or1.buf[i] | to_or2.buf[i];
    }

    // Xors the bits of this and to_or, placing the result in output.
    static void
    _xor(TBitset& output, TBitset& to_xor1, TBitset& to_xor2) noexcept {
        for (size_t i = 0; i < buf_len; i++)
            output.buf[i] = to_xor1.buf[i] ^ to_xor2.buf[i];
    }

    // Full operators
    TBitset
    operator~() const noexcept {
        TBitset bs = TBitset();
        _inverse(bs, (TBitset&)*this);
        return bs;
    };
    TBitset
    operator&(TBitset& rhs) const noexcept {
        TBitset bs = TBitset();
        _and(bs, (TBitset&)*this, rhs);
        return bs;
    };
    TBitset
    operator|(TBitset& rhs) const noexcept {
        TBitset bs = TBitset();
        _or(bs, (TBitset&)*this, rhs);
        return bs;
    }
    TBitset
    operator^(TBitset& rhs) const noexcept {
        TBitset bs = TBitset();
        _xor(bs, (TBitset&)*this, rhs);
        return bs;
    }
    TBitset&
    operator&=(TBitset& rhs) noexcept {
        _and((TBitset&)*this, (TBitset&)*this, rhs);
        return *this;
    }
    TBitset&
    operator|=(TBitset& rhs) noexcept {
        _or((TBitset&)*this, (TBitset&)*this, rhs);
        return *this;
    }
    TBitset&
    operator^=(TBitset& rhs) noexcept {
        _xor((TBitset&)*this, (TBitset&)*this, rhs);
        return *this;
    }
    bool
    operator==(TBitset& rhs) const noexcept {
        size_t ans = 0;
        for (size_t i = 0; i < buf_len; i++) ans |= this->buf[i] ^ rhs.buf[i];
        return ans ? 1 : 0;
    }
    bool
    operator!=(TBitset& rhs) const noexcept {
        return !this == rhs;
    }

    std::string
    to_string() {
        // Nothing performance sensitive depends on this.
        std::string s = "";
        for (size_t i = 0; i < num_bits; i++) {
            bool b = (*this)[i];
            s += b ? '1' : '0';
        }
        return s;
    }

    friend std::ostream&
    operator<<(std::ostream& os, TBitset& bs) {
        os << bs.to_string();
        return os;
    }
};

#endif  // TBITSET_INCLUDE
