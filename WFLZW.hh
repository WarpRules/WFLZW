#ifndef WFLZW_INCLUDE_GUARD
#define WFLZW_INCLUDE_GUARD
#include <cstdint>
#include <type_traits>
#include <cassert>

#define WFLZW_VERSION 0x010000
#define WFLZW_VERSION_STRING "1.0.0"
#define WFLZW_COPYRIGHT_STRING "WFLZW v" WFLZW_VERSION_STRING " (C)2018 Juha Nieminen"

namespace WFLZW
{
    using Byte = std::uint8_t;

    enum class DictionaryType { list, tree };

    template<unsigned kDictionaryMaxSize, DictionaryType, unsigned kOutputBufferSize>
    class Encoder;

    template<unsigned kDictionaryMaxSize>
    class Decoder;

    struct ByteRemapper;

    using Encoder64k = Encoder<65536, DictionaryType::tree, 256>;
    using Encoder32k = Encoder<32768, DictionaryType::tree, 256>;
    using Encoder16k = Encoder<16384, DictionaryType::tree, 256>;
    using Decoder64k = Decoder<65536>;
    using Decoder32k = Decoder<32768>;
    using Decoder16k = Decoder<16384>;
}

//============================================================================
// Encoder
//============================================================================
template<unsigned kDictionaryMaxSize,
         WFLZW::DictionaryType kDictionaryType = WFLZW::DictionaryType::tree,
         unsigned kOutputBufferSize = 256>
class WFLZW::Encoder
{
    static_assert(kDictionaryMaxSize > 1,
                  "WFLZW::Encoder kDictionaryMaxSize template parameter is too small");

    static const WFLZW::Byte kMaxInputByteValueDefault =
        (kDictionaryMaxSize <= 256U ? WFLZW::Byte(kDictionaryMaxSize - 2) : WFLZW::Byte(255U));

 public:
    Encoder(WFLZW::Byte maxInputByteValue = kMaxInputByteValueDefault);

    void initialize(WFLZW::Byte maxInputByteValue = kMaxInputByteValueDefault);

    WFLZW::Byte maxByteValue() const { return mMaxInputByteValue; }

    void encodeBytes(const WFLZW::Byte*, const std::size_t amount);
    void encodeBytes(const WFLZW::Byte*, const std::size_t amount, const WFLZW::ByteRemapper&);
    void encodeByte(WFLZW::Byte);
    void encodeByte(WFLZW::Byte, const WFLZW::ByteRemapper&);
    void finalizeEncoding();

    virtual void outputEncodedBytes(const WFLZW::Byte*, unsigned) {}


 private:
    using Index_t = typename
        std::conditional<(kDictionaryMaxSize <= 0x100U), std::uint8_t,
        typename std::conditional<(kDictionaryMaxSize <= 0x10000U), std::uint16_t,
        std::uint32_t>::type>::type;

    static_assert(kOutputBufferSize >= sizeof(Index_t),
                  "WFLZW::Encoder kOutputBufferSize template parameter is too small");

    class DictionaryList
    {
     public:
        static const Index_t kEmptyIndex = ~Index_t();

        void initialize(WFLZW::Byte);
        Index_t addIfNotExistent(const Index_t, const WFLZW::Byte);
        unsigned size() const { return mEntriesAmount; }
        bool isFull() const { return mEntriesAmount >= kDictionaryMaxSize; }

     private:
        struct ListIndices
        {
            Index_t first, next;
        };

        ListIndices mListIndices[kDictionaryMaxSize];
        WFLZW::Byte mBytes[kDictionaryMaxSize];
        unsigned mEntriesAmount;
    };

    class DictionaryTree
    {
     public:
        static const Index_t kEmptyIndex = ~Index_t();

        void initialize(WFLZW::Byte);
        Index_t addIfNotExistent(const Index_t, const WFLZW::Byte);
        unsigned size() const { return mEntriesAmount; }
        bool isFull() const { return mEntriesAmount >= kDictionaryMaxSize; }

     private:
        struct ListIndices
        {
            Index_t first, left, right;
        };

        ListIndices mListIndices[kDictionaryMaxSize];
        WFLZW::Byte mBytes[kDictionaryMaxSize];
        unsigned mEntriesAmount;
    };

    using Dictionary = typename
        std::conditional<kDictionaryType == WFLZW::DictionaryType::list,
                         DictionaryList, DictionaryTree>::type;

    Dictionary mDictionary;
    WFLZW::Byte mOutputBuffer[kOutputBufferSize];
    unsigned mOutputBufferIndex, mOutputBufferBitOffset, mBitSize;
    Index_t mIndex, mMaxOutputValueForCurrentBitSize;
    WFLZW::Byte mMaxInputByteValue;
    bool mDictionaryHasBeenReset;

    void reset();
    void outputIndex(Index_t);
    void incrementOutputBufferIndex();
    void outputByte(WFLZW::Byte, unsigned);
};


//============================================================================
// Decoder
//============================================================================
template<unsigned kDictionaryMaxSize>
class WFLZW::Decoder
{
    static_assert(kDictionaryMaxSize > 1,
                  "WFLZW::Decoder kDictionaryMaxSize template parameter is too small");

    static const WFLZW::Byte kMaxInputByteValueDefault =
        (kDictionaryMaxSize <= 256U ? WFLZW::Byte(kDictionaryMaxSize - 2) : WFLZW::Byte(255U));

 public:
    Decoder(WFLZW::Byte maxInputByteValue = kMaxInputByteValueDefault);

    void initialize(WFLZW::Byte maxInputByteValue = kMaxInputByteValueDefault);

    WFLZW::Byte maxByteValue() const { return mMaxInputByteValue; }

    /* Returns true if the end-of-stream code was encountered (signifying
       that all the data had been decoded), else false. */
    bool decodeBytes(const WFLZW::Byte*, const std::size_t amount);
    bool decodeByte(WFLZW::Byte);

    virtual void outputDecodedBytes(WFLZW::Byte*, unsigned) {}


 private:
    using Index_t = typename
        std::conditional<(kDictionaryMaxSize <= 0x10000U), std::uint16_t, std::uint32_t>::type;
    static const Index_t kEmptyIndex = ~Index_t();

    Index_t mPrefixIndices[kDictionaryMaxSize];
    WFLZW::Byte mBytes[kDictionaryMaxSize];
    WFLZW::Byte mDecodeBuffer[kDictionaryMaxSize];
    unsigned mEntriesAmount;
    unsigned mBitSize, mBitOffset;
    Index_t mOldIndex;
    Index_t mInputBuffer, mMaxInputValueForCurrentBitSize;
    WFLZW::Byte mMaxInputByteValue, mOldFirstByte;

    void reset();
    bool decodeIndex(Index_t);
    const WFLZW::Byte* extractAndOutputStringAt(Index_t);
    void addToDictionary(Index_t, WFLZW::Byte);
};


//============================================================================
// Byte remapper
//============================================================================
struct WFLZW::ByteRemapper
{
    WFLZW::Byte encodeMap[256] = {}, decodeMap[256] = {};
    unsigned decodeMapSize = 0;

    void createEncodeMapFromInputBytes(const WFLZW::Byte* bytes, const std::size_t amount);

    void startEncodeMapCreation();
    void addInputByteForEncodeMap(WFLZW::Byte byte);
    void finalizeEncodeMapCreation();

    void decodeBytes(WFLZW::Byte* bytes, const std::size_t amount) const;
};


//============================================================================
// Implementations
//============================================================================
inline void WFLZW::ByteRemapper::createEncodeMapFromInputBytes
(const WFLZW::Byte* bytes, const std::size_t amount)
{
    startEncodeMapCreation();
    for(std::size_t i = 0; i < amount; ++i)
        addInputByteForEncodeMap(bytes[i]);
    finalizeEncodeMapCreation();
}

inline void WFLZW::ByteRemapper::startEncodeMapCreation()
{
    *this = WFLZW::ByteRemapper();
}

inline void WFLZW::ByteRemapper::addInputByteForEncodeMap(WFLZW::Byte byte)
{
    encodeMap[byte] = 1;
}

inline void WFLZW::ByteRemapper::finalizeEncodeMapCreation()
{
    for(unsigned i = 0; i < 256; ++i)
    {
        if(encodeMap[i])
        {
            encodeMap[i] = static_cast<WFLZW::Byte>(decodeMapSize);
            decodeMap[decodeMapSize++] = static_cast<WFLZW::Byte>(i);
        }
    }
}

inline void WFLZW::ByteRemapper::decodeBytes(WFLZW::Byte* bytes, const std::size_t amount) const
{
    for(std::size_t i = 0; i < amount; ++i)
        bytes[i] = decodeMap[bytes[i]];
}

template<unsigned kDictionaryMaxSize, WFLZW::DictionaryType kDictType, unsigned kOutputBufferSize>
void WFLZW::Encoder<kDictionaryMaxSize, kDictType, kOutputBufferSize>::DictionaryList::initialize
(WFLZW::Byte maxInputByteValue)
{
    const unsigned maxIndex = static_cast<unsigned>(maxInputByteValue) + 1;
    mEntriesAmount = maxIndex + 1;
    for(unsigned i = 0; i < maxIndex; ++i)
        mListIndices[i].first = kEmptyIndex;
}

template<unsigned kDictionaryMaxSize, WFLZW::DictionaryType kDictType, unsigned kOutputBufferSize>
typename WFLZW::Encoder<kDictionaryMaxSize, kDictType, kOutputBufferSize>::Index_t
WFLZW::Encoder<kDictionaryMaxSize, kDictType, kOutputBufferSize>::DictionaryList::addIfNotExistent
(const Index_t prefixIndex, const WFLZW::Byte byteValue)
{
    if(prefixIndex == kEmptyIndex)
        return static_cast<Index_t>(byteValue);

    const Index_t listStartIndex = mListIndices[prefixIndex].first;
    Index_t index = listStartIndex;
    while(index != kEmptyIndex)
    {
        if(mBytes[index] == byteValue)
            return index;
        index = mListIndices[index].next;
    }

    mBytes[mEntriesAmount] = byteValue;
    mListIndices[mEntriesAmount].first = kEmptyIndex;
    mListIndices[mEntriesAmount].next = listStartIndex;
    mListIndices[prefixIndex].first = mEntriesAmount;
    ++mEntriesAmount;

    return kEmptyIndex;
}

template<unsigned kDictionaryMaxSize, WFLZW::DictionaryType kDictType, unsigned kOutputBufferSize>
void WFLZW::Encoder<kDictionaryMaxSize, kDictType, kOutputBufferSize>::DictionaryTree::initialize
(WFLZW::Byte maxInputByteValue)
{
    const unsigned maxIndex = static_cast<unsigned>(maxInputByteValue) + 1;
    mEntriesAmount = maxIndex + 1;
    for(unsigned i = 0; i < maxIndex; ++i)
        mListIndices[i].first = kEmptyIndex;
}

template<unsigned kDictionaryMaxSize, WFLZW::DictionaryType kDictType, unsigned kOutputBufferSize>
typename WFLZW::Encoder<kDictionaryMaxSize, kDictType, kOutputBufferSize>::Index_t
WFLZW::Encoder<kDictionaryMaxSize, kDictType, kOutputBufferSize>::DictionaryTree::addIfNotExistent
(const Index_t prefixIndex, const WFLZW::Byte byteValue)
{
    if(prefixIndex == kEmptyIndex)
        return static_cast<Index_t>(byteValue);

    Index_t index = mListIndices[prefixIndex].first, prevIndex = kEmptyIndex;
    bool goRight;
    WFLZW::Byte dirBitMask = byteValue;
    while(index != kEmptyIndex)
    {
        if(mBytes[index] == byteValue)
            return index;
        prevIndex = index;
        goRight = (dirBitMask & 1);
        dirBitMask >>= 1;
        index = (goRight ? mListIndices[index].right : mListIndices[index].left);
    }

    mBytes[mEntriesAmount] = byteValue;
    mListIndices[mEntriesAmount].first = kEmptyIndex;
    mListIndices[mEntriesAmount].left = kEmptyIndex;
    mListIndices[mEntriesAmount].right = kEmptyIndex;

    if(prevIndex == kEmptyIndex)
        mListIndices[prefixIndex].first = mEntriesAmount;
    else if(goRight)
        mListIndices[prevIndex].right = mEntriesAmount;
    else
        mListIndices[prevIndex].left = mEntriesAmount;

    ++mEntriesAmount;
    return kEmptyIndex;
}


template<unsigned kDictionaryMaxSize, WFLZW::DictionaryType kDictType, unsigned kOutputBufferSize>
WFLZW::Encoder<kDictionaryMaxSize, kDictType, kOutputBufferSize>::Encoder
(WFLZW::Byte maxInputByteValue)
{
    initialize(maxInputByteValue);
}

template<unsigned kDictionaryMaxSize, WFLZW::DictionaryType kDictType, unsigned kOutputBufferSize>
void WFLZW::Encoder<kDictionaryMaxSize, kDictType, kOutputBufferSize>::initialize
(WFLZW::Byte maxInputByteValue)
{
    assert(static_cast<unsigned>(maxInputByteValue) + 1 < kDictionaryMaxSize);
    mMaxInputByteValue = maxInputByteValue;
    mOutputBufferIndex = 0;
    mOutputBufferBitOffset = 0;
    reset();
}

template<unsigned kDictionaryMaxSize, WFLZW::DictionaryType kDictType, unsigned kOutputBufferSize>
void WFLZW::Encoder<kDictionaryMaxSize, kDictType, kOutputBufferSize>::reset()
{
    mIndex = Dictionary::kEmptyIndex;
    mDictionary.initialize(mMaxInputByteValue);
    mDictionaryHasBeenReset = true;

    unsigned dictSize = mDictionary.size();
    mBitSize = 1;
    while((dictSize >>= 1)) ++mBitSize;
    mMaxOutputValueForCurrentBitSize = (1U << mBitSize);
}

template<unsigned kDictionaryMaxSize, WFLZW::DictionaryType kDictType, unsigned kOutputBufferSize>
void WFLZW::Encoder<kDictionaryMaxSize, kDictType, kOutputBufferSize>::encodeByte
(WFLZW::Byte byte)
{
    assert(byte <= mMaxInputByteValue);

    const Index_t existingIndex = mDictionary.addIfNotExistent(mIndex, byte);
    mDictionaryHasBeenReset = false;

    if(existingIndex != Dictionary::kEmptyIndex)
    {
        mIndex = existingIndex;
    }
    else
    {
        outputIndex(mIndex);
        mIndex = static_cast<Index_t>(byte);

        if(mDictionary.isFull())
        {
            outputIndex(mIndex);
            reset();
        }
        else if(mDictionary.size() == mMaxOutputValueForCurrentBitSize)
        {
            ++mBitSize;
            mMaxOutputValueForCurrentBitSize = (1U << mBitSize);
        }
    }
}

template<unsigned kDictionaryMaxSize, WFLZW::DictionaryType kDictType, unsigned kOutputBufferSize>
void WFLZW::Encoder<kDictionaryMaxSize, kDictType, kOutputBufferSize>::encodeByte
(WFLZW::Byte byte, const WFLZW::ByteRemapper& remapper)
{
    encodeByte(remapper.encodeMap[byte]);
}

template<unsigned kDictionaryMaxSize, WFLZW::DictionaryType kDictType, unsigned kOutputBufferSize>
void WFLZW::Encoder<kDictionaryMaxSize, kDictType, kOutputBufferSize>::encodeBytes
(const WFLZW::Byte* bytes, const std::size_t amount)
{
    for(std::size_t i = 0; i < amount; ++i)
        encodeByte(bytes[i]);
}

template<unsigned kDictionaryMaxSize, WFLZW::DictionaryType kDictType, unsigned kOutputBufferSize>
void WFLZW::Encoder<kDictionaryMaxSize, kDictType, kOutputBufferSize>::encodeBytes
(const WFLZW::Byte* bytes, const std::size_t amount, const WFLZW::ByteRemapper& remapper)
{
    for(std::size_t i = 0; i < amount; ++i)
        encodeByte(remapper.encodeMap[bytes[i]]);
}

template<unsigned kDictionaryMaxSize, WFLZW::DictionaryType kDictType, unsigned kOutputBufferSize>
void WFLZW::Encoder<kDictionaryMaxSize, kDictType, kOutputBufferSize>::finalizeEncoding()
{
    if(!mDictionaryHasBeenReset)
        outputIndex(mIndex);
    outputIndex(static_cast<Index_t>(mMaxInputByteValue) + 1);

    if(mOutputBufferIndex > 0 || mOutputBufferBitOffset > 0)
    {
        outputEncodedBytes(mOutputBuffer, mOutputBufferIndex + (mOutputBufferBitOffset > 0));
        mOutputBufferIndex = 0;
        mOutputBufferBitOffset = 0;
        reset();
    }
}

template<unsigned kDictionaryMaxSize, WFLZW::DictionaryType kDictType, unsigned kOutputBufferSize>
void WFLZW::Encoder<kDictionaryMaxSize, kDictType, kOutputBufferSize>::incrementOutputBufferIndex()
{
    if(++mOutputBufferIndex == kOutputBufferSize)
    {
        outputEncodedBytes(mOutputBuffer, kOutputBufferSize);
        mOutputBufferIndex = 0;
    }
}

template<unsigned kDictionaryMaxSize, WFLZW::DictionaryType kDictType, unsigned kOutputBufferSize>
void WFLZW::Encoder<kDictionaryMaxSize, kDictType, kOutputBufferSize>::outputByte
(WFLZW::Byte byte, unsigned bits)
{
    if(mOutputBufferBitOffset == 0)
    {
        mOutputBuffer[mOutputBufferIndex] = byte;
        if(bits == 8) incrementOutputBufferIndex();
        else mOutputBufferBitOffset += bits;
    }
    else if(mOutputBufferBitOffset + bits <= 8)
    {
        mOutputBuffer[mOutputBufferIndex] |= (byte << mOutputBufferBitOffset);
        if((mOutputBufferBitOffset += bits) == 8)
        {
            incrementOutputBufferIndex();
            mOutputBufferBitOffset = 0;
        }
    }
    else
    {
        mOutputBuffer[mOutputBufferIndex] |= (byte << mOutputBufferBitOffset);
        incrementOutputBufferIndex();
        const unsigned bitsInPreviousOutputByte = 8 - mOutputBufferBitOffset;
        mOutputBuffer[mOutputBufferIndex] = (byte >> bitsInPreviousOutputByte);
        mOutputBufferBitOffset = (bits - bitsInPreviousOutputByte);
    }
}

template<unsigned kDictionaryMaxSize, WFLZW::DictionaryType kDictType, unsigned kOutputBufferSize>
void WFLZW::Encoder<kDictionaryMaxSize, kDictType, kOutputBufferSize>::outputIndex
(Index_t index)
{
    if(mBitSize <= 8)
    {
        outputByte(static_cast<WFLZW::Byte>(index), mBitSize);
    }
    else if(sizeof(Index_t) == 2)
    {
        outputByte(static_cast<WFLZW::Byte>(index), 8);
        outputByte(static_cast<WFLZW::Byte>(index >> 8), mBitSize - 8);
    }
    else
    {
        unsigned bitSize = mBitSize;
        while(bitSize > 8)
        {
            outputByte(static_cast<WFLZW::Byte>(index), 8);
            index >>= 8;
            bitSize -= 8;
        }
        outputByte(static_cast<WFLZW::Byte>(index), bitSize);
    }
}

template<unsigned kDictionaryMaxSize>
WFLZW::Decoder<kDictionaryMaxSize>::Decoder
(WFLZW::Byte maxInputByteValue)
{
    initialize(maxInputByteValue);
}

template<unsigned kDictionaryMaxSize>
void WFLZW::Decoder<kDictionaryMaxSize>::initialize
(WFLZW::Byte maxInputByteValue)
{
    assert(static_cast<unsigned>(maxInputByteValue) + 1 < kDictionaryMaxSize);
    mMaxInputByteValue = maxInputByteValue;
    mBitOffset = 0;
    const unsigned maxIndex = static_cast<unsigned>(maxInputByteValue) + 1;
    for(unsigned i = 0; i < maxIndex; ++i)
    {
        mPrefixIndices[i] = kEmptyIndex;
        mBytes[i] = static_cast<WFLZW::Byte>(i);
    }

    reset();
}

template<unsigned kDictionaryMaxSize>
void WFLZW::Decoder<kDictionaryMaxSize>::reset()
{
    mEntriesAmount = static_cast<unsigned>(mMaxInputByteValue) + 2;
    mOldIndex = kEmptyIndex;

    unsigned dictSize = mEntriesAmount;
    mBitSize = 1;
    while((dictSize >>= 1)) ++mBitSize;
    mMaxInputValueForCurrentBitSize = (1U << mBitSize) - 1;
}

template<unsigned kDictionaryMaxSize>
bool WFLZW::Decoder<kDictionaryMaxSize>::decodeBytes
(const WFLZW::Byte* bytes, const std::size_t amount)
{
    for(std::size_t i = 0; i < amount; ++i)
        if(decodeByte(bytes[i]))
            return true;

    return false;
}

template<unsigned kDictionaryMaxSize>
bool WFLZW::Decoder<kDictionaryMaxSize>::decodeByte
(WFLZW::Byte byte)
{
    const Index_t value = static_cast<Index_t>(byte);

    if(mBitOffset == 0) mInputBuffer = value;
    else mInputBuffer |= (value << mBitOffset);

    const unsigned newBitOffset = mBitOffset + 8;

    const unsigned kIndexBits = sizeof(Index_t) * 8;
    const unsigned extraBitsAmount = (newBitOffset > kIndexBits ? newBitOffset - kIndexBits : 0);
    const Index_t extraBits =
        (newBitOffset > kIndexBits ? value >> (kIndexBits - mBitOffset) : 0);

    mBitOffset += 8;

    while(mBitOffset >= mBitSize)
    {
        const Index_t index = (mInputBuffer & static_cast<Index_t>((1U << mBitSize) - 1));
        mBitOffset -= mBitSize;
        mInputBuffer >>= mBitSize;
        if(extraBitsAmount > 0)
        {
            if(mBitOffset == extraBitsAmount)
                mInputBuffer = extraBits;
            else if(mBitOffset > extraBitsAmount)
                mInputBuffer |= (extraBits << (mBitOffset - extraBitsAmount));
            else
                mInputBuffer |= (extraBits >> (extraBitsAmount - mBitOffset));
        }

        if(decodeIndex(index)) return true;
    }

    return false;
}

template<unsigned kDictionaryMaxSize>
const WFLZW::Byte* WFLZW::Decoder<kDictionaryMaxSize>::extractAndOutputStringAt(Index_t index)
{
    WFLZW::Byte* endOfBuffer = mDecodeBuffer + kDictionaryMaxSize;
    WFLZW::Byte* decodedString = endOfBuffer;

    while(index != kEmptyIndex)
    {
        *(--decodedString) = mBytes[index];
        index = mPrefixIndices[index];
    }

    outputDecodedBytes(decodedString, endOfBuffer - decodedString);
    return decodedString;
}

template<unsigned kDictionaryMaxSize>
void WFLZW::Decoder<kDictionaryMaxSize>::addToDictionary(Index_t prefixIndex, WFLZW::Byte byteValue)
{
    mPrefixIndices[mEntriesAmount] = prefixIndex;
    mBytes[mEntriesAmount] = byteValue;
    ++mEntriesAmount;
}

template<unsigned kDictionaryMaxSize>
bool WFLZW::Decoder<kDictionaryMaxSize>::decodeIndex(Index_t index)
{
    assert(index < kDictionaryMaxSize);

    if(index == static_cast<Index_t>(mMaxInputByteValue) + 1) return true;

    if(index < mEntriesAmount)
    {
        const WFLZW::Byte* decodedString = extractAndOutputStringAt(index);
        mOldFirstByte = *decodedString;
        if(mOldIndex != kEmptyIndex)
            addToDictionary(mOldIndex, mOldFirstByte);
        mOldIndex = index;
    }
    else
    {
        const Index_t newIndex = static_cast<Index_t>(mEntriesAmount);
        addToDictionary(mOldIndex, mOldFirstByte);
        const WFLZW::Byte* decodedString = extractAndOutputStringAt(newIndex);
        mOldFirstByte = *decodedString;
        mOldIndex = newIndex;
    }

    if(mEntriesAmount == kDictionaryMaxSize)
    {
        reset();
    }
    else if(mEntriesAmount == mMaxInputValueForCurrentBitSize &&
            mEntriesAmount < kDictionaryMaxSize - 1)
    {
        ++mBitSize;
        mMaxInputValueForCurrentBitSize = (1U << mBitSize) - 1;
    }

    return false;
}

#endif
