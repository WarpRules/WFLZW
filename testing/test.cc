#include "../WFLZW.hh"
#include <iostream>
#include <vector>
#include <random>
#include <utility>
#include <algorithm>
#include <memory>

//#define RUN_EXTENSIVE_TESTS

namespace
{
    const WFLZW::DictionaryType kDictType = WFLZW::DictionaryType::tree;

    const unsigned kDataSizes[] =
    {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15, 16, 17, 24, 25, 63, 64, 65, 100, 200, 500,
        1000, 1023, 1024, 1025, 2000, 5000, 8000, 25000, 50000, 100000, 200000, 700000,
        1000000, 10000000
    };
    const unsigned kDataSizesAmount = sizeof(kDataSizes) / sizeof(*kDataSizes);

    const WFLZW::Byte kMaxByteValues[] =
    { 1, 2, 3, 4, 6, 8, 16, 32, 64, 100, 127, 128, 200, 255 };
    const unsigned kMaxByteValuesAmount = sizeof(kMaxByteValues) / sizeof(*kMaxByteValues);

    std::vector<WFLZW::Byte> gInputData, gEncodedData, gDecodedData;
}

static bool printValues() { return false; }

template<typename Type, typename... Rest>
static bool printValues(Type&& value, Rest&&... rest)
{
    std::cerr << value;
    return printValues(std::forward<Rest>(rest)...);
}

#define PRINTERROR(...) return printValues("At ", __LINE__, ": ", __VA_ARGS__)
#define ERRORRET return printValues("Called from line ", __LINE__)

template<unsigned kDictionaryMaxSize>
class TestEncoder: public WFLZW::Encoder<kDictionaryMaxSize, kDictType>
{
 public:
    virtual void outputEncodedBytes(const WFLZW::Byte* bytes, unsigned amount)
    {
        gEncodedData.insert(gEncodedData.end(), bytes, bytes + amount);
    }
};

template<unsigned kDictionaryMaxSize>
class TestDecoder: public WFLZW::Decoder<kDictionaryMaxSize>
{
 public:
    virtual void outputDecodedBytes(WFLZW::Byte* bytes, unsigned amount)
    {
        gDecodedData.insert(gDecodedData.end(), bytes, bytes + amount);
    }
};

template<unsigned kMaxSize, bool = (kMaxSize > 65536)> class TestEncoderContainer;
template<unsigned kMaxSize, bool = (kMaxSize > 65536)> class TestDecoderContainer;

template<unsigned kDictionaryMaxSize>
class TestEncoderContainer<kDictionaryMaxSize, false>
{
    TestEncoder<kDictionaryMaxSize> mEncoder;

 public:
    TestEncoder<kDictionaryMaxSize>& instance() { return mEncoder; }
};

template<unsigned kDictionaryMaxSize>
class TestEncoderContainer<kDictionaryMaxSize, true>
{
    std::unique_ptr<TestEncoder<kDictionaryMaxSize>> mEncoder;

 public:
    TestEncoderContainer(): mEncoder(new TestEncoder<kDictionaryMaxSize>) {}
    TestEncoder<kDictionaryMaxSize>& instance() { return *mEncoder; }
};

template<unsigned kDictionaryMaxSize>
class TestDecoderContainer<kDictionaryMaxSize, false>
{
    TestDecoder<kDictionaryMaxSize> mDecoder;

 public:
    TestDecoder<kDictionaryMaxSize>& instance() { return mDecoder; }
};

template<unsigned kDictionaryMaxSize>
class TestDecoderContainer<kDictionaryMaxSize, true>
{
    std::unique_ptr<TestDecoder<kDictionaryMaxSize>> mDecoder;

 public:
    TestDecoderContainer(): mDecoder(new TestDecoder<kDictionaryMaxSize>) {}
    TestDecoder<kDictionaryMaxSize>& instance() { return *mDecoder; }
};

template<unsigned kDictionaryMaxSize>
bool testEncoding(TestEncoder<kDictionaryMaxSize>& encoder,
                  TestDecoder<kDictionaryMaxSize>& decoder)
{
    gEncodedData.clear();
    gDecodedData.clear();
    encoder.encodeBytes(&gInputData[0], gInputData.size());
    encoder.finalizeEncoding();
    /*
    std::printf("{");
    for(std::size_t i = 0; i < gEncodedData.size(); ++i)
        std::printf(i?",%u":"%u", gEncodedData[i]);
    std::printf("}\n");
    */
    decoder.initialize(encoder.maxByteValue());
    decoder.decodeBytes(&gEncodedData[0], gEncodedData.size());

    const std::size_t length = std::min(gInputData.size(), gDecodedData.size());

    for(std::size_t i = 0; i < length; ++i)
        if(gInputData[i] != gDecodedData[i])
            PRINTERROR("Error: gInputData[", i, "]=", unsigned(gInputData[i]),
                       ", but gDecodedData[", i, "]=", unsigned(gDecodedData[i]),
                       " (gInputData.size()=", gInputData.size(),
                       ", gDecodedData.size()=", gDecodedData.size(),
                       ", gEncodedData.size()=", gEncodedData.size(), ")\n");

    if(gInputData.size() != gDecodedData.size())
        PRINTERROR("Error: gInputData.size()=", gInputData.size(),
                   ", but gDecodedData.size()=", gDecodedData.size(),
                   " (gEncodedData.size()=", gEncodedData.size(), ")\n");

    return true;
}

template<unsigned kDictionaryMaxSize>
bool testCombinations()
{
#ifdef RUN_EXTENSIVE_TESTS
    const unsigned kMaxSize = 9;
#else
    const unsigned kMaxSize = 8;
#endif

    const unsigned maximumPossibleByteValue = std::min(kDictionaryMaxSize - 3, 255U);
    TestEncoder<kDictionaryMaxSize> encoder;
    TestDecoder<kDictionaryMaxSize> decoder;
    WFLZW::Byte values[kMaxSize+1];

    std::cout << "Running combinations test with kDictionaryMaxSize=" << kDictionaryMaxSize
              << ".\n";

    for(unsigned size = 1; size <= kMaxSize; ++size)
    {
#ifdef RUN_EXTENSIVE_TESTS
        std::cout << "\r" << (size-1)*100/kMaxSize << "%" << std::flush;
#endif
        for(unsigned maxByteValueInd = 0;
            maxByteValueInd < kMaxByteValuesAmount &&
                kMaxByteValues[maxByteValueInd] <= maximumPossibleByteValue &&
                kMaxByteValues[maxByteValueInd] <= size;
            ++maxByteValueInd)
        {
            const WFLZW::Byte maxByteValue = kMaxByteValues[maxByteValueInd];

            for(unsigned i = 0; i < size; ++i)
                values[i] = 0;

            bool combinationsLeft = true;
            while(combinationsLeft)
            {
                gInputData.assign(values, values+size);
                encoder.initialize(maxByteValue);
                if(!testEncoding(encoder, decoder))
                {
                    std::cout << "kDictionaryMaxSize=" << kDictionaryMaxSize
                              << ", size=" << size
                              << ", maxByteValue=" << unsigned(maxByteValue)
                              << "\nInput data={";
                    for(unsigned i = 0; i < size; ++i)
                        std::cout << (i?",":"") << unsigned(values[i]);
                    std::cout << "}\n";
                    return false;
                }

                combinationsLeft = false;
                for(unsigned i = 0; i < size; ++i)
                {
                    if(values[i] == maxByteValue)
                        values[i] = 0;
                    else
                    {
                        ++values[i];
                        combinationsLeft = true;
                        break;
                    }
                }
            }
        }
    }

#ifdef RUN_EXTENSIVE_TESTS
    std::cout << "\r100%\n";
#endif
    return true;
}

template<unsigned kDictionaryMaxSize>
bool testRandomData()
{
#ifdef RUN_EXTENSIVE_TESTS
    std::cout << "Testing random data\n";
#endif

    const unsigned maximumPossibleByteValue = std::min(kDictionaryMaxSize - 3, 255U);

    gInputData.reserve(kDataSizes[kDataSizesAmount - 1]);
    gDecodedData.reserve(kDataSizes[kDataSizesAmount - 1]);

    TestEncoderContainer<kDictionaryMaxSize> encoderContainer;
    TestDecoderContainer<kDictionaryMaxSize> decoderContainer;
    TestEncoder<kDictionaryMaxSize>& encoder = encoderContainer.instance();
    TestDecoder<kDictionaryMaxSize>& decoder = decoderContainer.instance();
    std::mt19937 rngEngine(0);

    for(unsigned dataSizeInd = 0; dataSizeInd < kDataSizesAmount; ++dataSizeInd)
    {
#ifdef RUN_EXTENSIVE_TESTS
        std::cout << "\r" << dataSizeInd*100/kDataSizesAmount << "%" << std::flush;
#endif
        const std::size_t inputDataSize = kDataSizes[dataSizeInd];
        gInputData.resize(inputDataSize);

        for(unsigned maxByteValueInd = 0;
            maxByteValueInd < kMaxByteValuesAmount &&
            kMaxByteValues[maxByteValueInd] <= maximumPossibleByteValue;
            ++maxByteValueInd)
        {
            const WFLZW::Byte maxByteValue = kMaxByteValues[maxByteValueInd];
            std::uniform_int_distribution<unsigned> distribution(0, maxByteValue);

#ifdef RUN_EXTENSIVE_TESTS
            for(unsigned iteration = 0; iteration < 4; ++iteration)
#endif
            {
                for(std::size_t i = 0; i < inputDataSize; ++i)
                    gInputData[i] = distribution(rngEngine);

                encoder.initialize(maxByteValue);

                if(!testEncoding(encoder, decoder))
                    PRINTERROR("kDictionaryMaxSize=", kDictionaryMaxSize,
                               ", inputDataSize=", inputDataSize,
                               ", maxByteValue=", unsigned(maxByteValue), "\n");
            }
        }
    }

#ifdef RUN_EXTENSIVE_TESTS
    std::cout << "\r100%\n";
#endif
    return true;
}

template<unsigned kDictionaryMaxSize>
bool testWordCombinations()
{
#ifdef RUN_EXTENSIVE_TESTS
    std::cout << "Testing word combinations\n";
#endif

    const unsigned maximumPossibleByteValue = std::min(kDictionaryMaxSize - 3, 255U);
    const WFLZW::Byte maxByteValue = maximumPossibleByteValue;

    TestEncoderContainer<kDictionaryMaxSize> encoderContainer;
    TestDecoderContainer<kDictionaryMaxSize> decoderContainer;
    TestEncoder<kDictionaryMaxSize>& encoder = encoderContainer.instance();
    TestDecoder<kDictionaryMaxSize>& decoder = decoderContainer.instance();
    std::mt19937 rngEngine(123);
    std::uniform_int_distribution<unsigned> randomByte(0, maxByteValue);
    std::uniform_int_distribution<unsigned> randomWordLength(1, 10);

    std::vector<std::vector<WFLZW::Byte>> words(500);

    for(auto& word: words)
    {
        word.resize(randomWordLength(rngEngine));
        for(std::size_t i = 0; i < word.size(); ++i)
            word[i] = randomByte(rngEngine);
    }

    const unsigned kVocabularySizes[] = { 2, 3, 4, 10, 25, 50, 100, 500 };
    const unsigned kWordsAmounts[] = { 2, 10, 50, 150, 500, 2500, 10000, 200000 };

#ifdef RUN_EXTENSIVE_TESTS
    const unsigned kMaxIterations = 12;
    for(unsigned iteration = 0; iteration < kMaxIterations; ++iteration)
#endif
    {
#ifdef RUN_EXTENSIVE_TESTS
        std::cout << "\r" << iteration*100/kMaxIterations << "%" << std::flush;
#endif
        for(unsigned vocabularySize: kVocabularySizes)
        {
            std::uniform_int_distribution<unsigned> randomWordIndex(0, vocabularySize - 1);
            for(unsigned wordsAmount: kWordsAmounts)
            {
                gInputData.clear();
                for(unsigned wordCounter = 0; wordCounter < wordsAmount; ++wordCounter)
                {
                    const unsigned wordIndex = randomWordIndex(rngEngine);
                    gInputData.insert(gInputData.end(),
                                      words[wordIndex].begin(), words[wordIndex].end());
                }

                encoder.initialize(maxByteValue);

                if(!testEncoding(encoder, decoder))
                    PRINTERROR("kDictionaryMaxSize=", kDictionaryMaxSize,
                               ", vocabularySize=", vocabularySize,
                               ", wordsAmount=", wordsAmount, "\n");
            }
        }
    }

#ifdef RUN_EXTENSIVE_TESTS
    std::cout << "\r100%\n";
#endif
    return true;
}

template<unsigned kDictionaryMaxSize>
bool testPatterns()
{
#ifdef RUN_EXTENSIVE_TESTS
    std::cout << "Testing patterns\n";
#endif

    const unsigned maximumPossibleByteValue = std::min(kDictionaryMaxSize - 3, 255U);
    TestEncoderContainer<kDictionaryMaxSize> encoderContainer;
    TestDecoderContainer<kDictionaryMaxSize> decoderContainer;
    TestEncoder<kDictionaryMaxSize>& encoder = encoderContainer.instance();
    TestDecoder<kDictionaryMaxSize>& decoder = decoderContainer.instance();

#ifdef RUN_EXTENSIVE_TESTS
    const unsigned kMaxDataSize = 3500;
#else
    const unsigned kMaxDataSize = 2000;
#endif

    for(unsigned dataSize = 1; dataSize <= kMaxDataSize; ++dataSize)
    {
#ifdef RUN_EXTENSIVE_TESTS
        std::cout << "\r" << (dataSize-1)*100/kMaxDataSize << "%" << std::flush;
#endif
        gInputData.resize(dataSize);

        const unsigned maxByteValue = maximumPossibleByteValue;

        for(unsigned sizeDivisor = 1, patternLength = dataSize; true;)
        {
            unsigned byteValue = 0;

            for(unsigned ind = 0, patternCounter = 0; ind < dataSize; ++ind)
            {
                gInputData[ind] = WFLZW::Byte(byteValue);
                if(++patternCounter >= patternLength)
                {
                    byteValue = (byteValue + 1) % (maxByteValue + 1);
                    patternCounter = 0;
                }
            }

            encoder.initialize(WFLZW::Byte(maxByteValue));

            if(!testEncoding(encoder, decoder))
                PRINTERROR("kDictionaryMaxSize=", kDictionaryMaxSize,
                           ", dataSize=", dataSize,
                           ", maxByteValue=", unsigned(maxByteValue),
                           ", patternLength=", patternLength, "\n");

            if(patternLength == 1) break;

            unsigned newPatternLength = patternLength;
            while(newPatternLength == patternLength)
                newPatternLength = dataSize / (++sizeDivisor);
            patternLength = newPatternLength;
        }
    }

#ifdef RUN_EXTENSIVE_TESTS
    std::cout << "\r100%\n";
#endif
    return true;
}

void printSize(unsigned size)
{
    if(size < 16*1024)
        std::cout << size;
    else if(size < 16*1024*1024)
        std::cout << (size + 512) / 1024 << " kB";
    else
        std::cout << (size + 512*1024) / (1024*1024) << " MB";
}

template<unsigned kDictionaryMaxSize>
bool runTests()
{
    std::cout << "Running tests with kDictionaryMaxSize=" << kDictionaryMaxSize
              << ", sizeof(encoder)=";
    printSize(sizeof(TestEncoder<kDictionaryMaxSize>));
    std::cout << ", sizeof(decoder)=";
    printSize(sizeof(TestDecoder<kDictionaryMaxSize>));
    std::cout << "\n";

    if(!testRandomData<kDictionaryMaxSize>()) ERRORRET;
    if(!testWordCombinations<kDictionaryMaxSize>()) ERRORRET;
    if(!testPatterns<kDictionaryMaxSize>()) ERRORRET;

    return true;
}

bool runCombinationsTests()
{
    if(!testCombinations<6>()) ERRORRET;
    if(!testCombinations<8>()) ERRORRET;
    if(!testCombinations<12>()) ERRORRET;
    if(!testCombinations<16>()) ERRORRET;
    if(!testCombinations<20>()) ERRORRET;
    if(!testCombinations<30>()) ERRORRET;
    return true;
}

bool runGenericTests()
{
    if(!runTests<8>()) ERRORRET;
    if(!runTests<16>()) ERRORRET;
    if(!runTests<128>()) ERRORRET;
    if(!runTests<129>()) ERRORRET;
    if(!runTests<255>()) ERRORRET;
    if(!runTests<256>()) ERRORRET;
    if(!runTests<257>()) ERRORRET;
    if(!runTests<512>()) ERRORRET;
    if(!runTests<1024>()) ERRORRET;
    if(!runTests<1234>()) ERRORRET;
    if(!runTests<(1U<<12)>()) ERRORRET;
    if(!runTests<6000>()) ERRORRET;
    if(!runTests<(1U<<13)>()) ERRORRET;
    if(!runTests<(1U<<14)>()) ERRORRET;
    if(!runTests<(1U<<15)>()) ERRORRET;
    if(!runTests<(1U<<16)>()) ERRORRET;
    if(!runTests<(1U<<16)+1>()) ERRORRET;
    if(!runTests<(1U<<17)>()) ERRORRET;
    if(!runTests<(1U<<18)>()) ERRORRET;
    return true;
}

int main()
{
    if(!runCombinationsTests()) return 1;
    if(!runGenericTests()) return 1;

    std::cout << "All tests ok.\n";
}
