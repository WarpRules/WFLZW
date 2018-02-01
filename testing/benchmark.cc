#include "../WFLZW.hh"
#include <vector>
#include <memory>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#ifndef WFLZW_DICT_SIZE
#define WFLZW_DICT_SIZE 65536
#endif

#ifndef WFLZW_DICT_TYPE
#define WFLZW_DICT_TYPE tree
#endif

namespace
{
    std::vector<WFLZW::Byte> gInputData, gEncodedData, gDecodedData;
}

class TestEncoder: public WFLZW::Encoder<WFLZW_DICT_SIZE, WFLZW::DictionaryType::WFLZW_DICT_TYPE>
{
 public:
    virtual void outputEncodedBytes(const WFLZW::Byte* bytes, unsigned amount)
    {
        gEncodedData.insert(gEncodedData.end(), bytes, bytes + amount);
    }
};

class TestDecoder: public WFLZW::Decoder<WFLZW_DICT_SIZE>
{
 public:
    virtual void outputDecodedBytes(WFLZW::Byte* bytes, unsigned amount)
    {
        gDecodedData.insert(gDecodedData.end(), bytes, bytes + amount);
    }
};

#if WFLZW_DICT_SIZE <= 65536
class TestEncoderContainer
{
    TestEncoder mEncoder;

 public:
    TestEncoder& instance() { return mEncoder; }
};

class TestDecoderContainer
{
    TestDecoder mDecoder;

 public:
    TestDecoder& instance() { return mDecoder; }
};
#else
class TestEncoderContainer
{
    std::unique_ptr<TestEncoder> mEncoder;

 public:
    TestEncoderContainer(): mEncoder(new TestEncoder) {}
    TestEncoder& instance() { return *mEncoder; }
};

class TestDecoderContainer
{
    std::unique_ptr<TestDecoder> mDecoder;

 public:
    TestDecoderContainer(): mDecoder(new TestDecoder) {}
    TestDecoder& instance() { return *mDecoder; }
};
#endif

static double runEncoder(unsigned iterations)
{
    TestEncoderContainer encoder;
    std::clock_t iClock = std::clock();
    for(unsigned i = 0; i < iterations; ++i)
    {
        gEncodedData.clear();
        encoder.instance().initialize();
        encoder.instance().encodeBytes(&gInputData[0], gInputData.size());
        encoder.instance().finalizeEncoding();
    }
    return double(std::clock() - iClock) / double(CLOCKS_PER_SEC);
}

static double runEncoder(unsigned iterations, const WFLZW::ByteRemapper& remapper)
{
    TestEncoderContainer encoder;
    std::clock_t iClock = std::clock();
    for(unsigned i = 0; i < iterations; ++i)
    {
        gEncodedData.clear();
        encoder.instance().initialize(remapper.decodeMapSize - 1);
        encoder.instance().encodeBytes(&gInputData[0], gInputData.size(), remapper);
        encoder.instance().finalizeEncoding();
    }
    return double(std::clock() - iClock) / double(CLOCKS_PER_SEC);
}

static double runDecoder(unsigned iterations, WFLZW::Byte maxByteValue = 255)
{
    TestDecoderContainer decoder;
    std::clock_t iClock = std::clock();
    for(unsigned i = 0; i < iterations; ++i)
    {
        gDecodedData.clear();
        decoder.instance().initialize(maxByteValue);
        decoder.instance().decodeBytes(&gEncodedData[0], gEncodedData.size());
    }
    return double(std::clock() - iClock) / double(CLOCKS_PER_SEC);
}

static void printSize(unsigned size)
{
    if(size < 16*1024)
        std::printf("%u", size);
    else if(size < 16*1024*1024)
        std::printf("%u kB", (size + 512) / 1024);
    else
        std::printf("%u MB", (size + 512*1024) / (1024*1024));
}

static void runBenchmark(const char* inputFileName, unsigned iterations, bool useRemapper)
{
    WFLZW::ByteRemapper remapper;
    double encodeTime = 0, decodeTime = 0;

    if(useRemapper)
    {
        remapper.createEncodeMapFromInputBytes(&gInputData[0], gInputData.size());
        encodeTime = runEncoder(iterations, remapper);
        decodeTime = runDecoder(iterations, remapper.decodeMapSize - 1);
        remapper.decodeBytes(&gDecodedData[0], gDecodedData.size());
    }
    else
    {
        encodeTime = runEncoder(iterations);
        decodeTime = runDecoder(iterations);
    }

    if(gInputData != gDecodedData)
    {
        std::printf("FATAL ERROR: Decoded data is not equal to the input data!\n");
        return;
    }

    std::printf("Input file: %s\n"
                "Dictionary size: %u, sizeof encoder: ",
                inputFileName, WFLZW_DICT_SIZE);
    printSize(sizeof(TestEncoder));
    std::printf(", sizeof decoder: ");
    printSize(sizeof(TestDecoder));
#define STRINGIFY_HELP(name) #name
#define STRINGIFY(name) STRINGIFY_HELP(name)
    std::printf
        (" (dictionary type: " STRINGIFY(WFLZW_DICT_TYPE) ")\n"
         "Input file size: %zu bytes\n", gInputData.size());
    if(useRemapper)
        std::printf("Using byte remapping; number of distinct bytes: %u\n",
                    remapper.decodeMapSize);
    std::printf
        ("Compressed size: %zu bytes (%.1f%%)\n"
         "Compression time (average of %u iterations): %.2f ms\n"
         "Decompression time (average of %u iterations): %.2f ms\n",
         gEncodedData.size(),
         double(gEncodedData.size())*100.0/double(gInputData.size()),
         iterations, encodeTime*1000.0 / iterations,
         iterations, decodeTime*1000.0 / iterations);
}

int main(int argc, char* argv[])
{
    const char* inputFileName = nullptr;
    unsigned iterations = 100;
    bool useRemapper = false;

    for(int i = 1; i < argc; ++i)
    {
        if(std::strcmp(argv[i], "-iterations") == 0)
        {
            if(++i == argc)
            { std::printf("Error: expecting parameter after -iterations\n"); return 1; }
            iterations = std::atoi(argv[i]);
            if(iterations < 1) iterations = 1;
        }
        else if(std::strcmp(argv[i], "-remapBytes") == 0)
            useRemapper = true;
        else
            inputFileName = argv[i];
    }

    if(!inputFileName)
    {
        std::printf
            ("Usage: benchmark [<options>] <input file>\n\n"
             "<options>:\n"
             " -iterations <amount> : Run the encoder and decoder this many times (default: 100)\n"
             " -remapBytes : Use the byte remapper\n\n"
             "You can compile the benchmark program specifying the WFLZW_DICT_SIZE\n"
             "preprocessor macro with a maximum dictionary size to use some value\n"
             "other than the default (which is 65536). For example:\n"
             "  g++ -O3 -DWFLZW_DICT_SIZE=16384 benchmark.cc -o benchmark\n\n"
             "Likewise you can specify the preprocessor macro WFLZW_DICT_TYPE=list\n"
             "to use the list dictionary type instead of the tree type.\n");
        return 0;
    }

    std::FILE* inputFile = std::fopen(inputFileName, "rb");
    if(!inputFile) { std::perror(inputFileName); return 1; }
    std::fseek(inputFile, 0, SEEK_END);
    const long fileSize = std::ftell(inputFile);
    std::fseek(inputFile, 0, SEEK_SET);
    gInputData.resize(fileSize);
    std::fread(&gInputData[0], 1, gInputData.size(), inputFile);
    std::fclose(inputFile);

    gEncodedData.reserve(gInputData.size());
    gDecodedData.reserve(gInputData.size());

    runBenchmark(inputFileName, iterations, useRemapper);
}
