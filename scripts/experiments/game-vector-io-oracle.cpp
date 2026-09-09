// Native semantic fixture only; never included in a matching VC6 translation unit.
#include <algorithm>
#include <climits>
#include <cstring>
#include <vector>
#include "abstractfile.h"
#include "secondaryskill.h"
#define SIZE(type, bytes)
#include "struct.h"

// @UNIVERSITY@
// @UNIVERSITY_CTOR@

struct ScriptFile : TAbstractFile {
    enum { FULL = INT_MAX };
    std::vector<unsigned char> input, output;
    std::vector<int> requests;
    int firstResult, secondResult;
    unsigned position;
    bool valid;
    std::vector<long>* watchedPairs;
    long previousPair;

    ScriptFile(int first = FULL, int second = FULL)
        : firstResult(first), secondResult(second), position(0), valid(true),
          watchedPairs(0), previousPair(0) {}

    int read(void* buffer, int size) {
        requests.push_back(size);
        int report = requests.size() == 1 ? firstResult : secondResult;
        if (requests.size() == 1 && size != 2)
            valid = false;
        if (requests.size() == 2 && watchedPairs) {
            if (watchedPairs->size() != 3 || (*watchedPairs)[0] != previousPair
                || (*watchedPairs)[1] != 0 || (*watchedPairs)[2] != 0)
                valid = false;
        }
        if (report == FULL)
            report = size;
        if (size < 0) {
            valid = false;
            return 0;
        }
        unsigned copied = report > 0 ? std::min(size, report) : 0;
        copied = std::min(copied, unsigned(input.size() - position));
        if (copied)
            std::memcpy(buffer, &input[position], copied);
        position += copied;
        return report;
    }

    int write(const void* buffer, int size) {
        requests.push_back(size);
        int report = requests.size() == 1 ? firstResult : secondResult;
        if (requests.size() == 1 && size != 2)
            valid = false;
        if (report == FULL)
            report = size;
        int copied = report > 0 ? std::min(size, report) : 0;
        if (copied > 0) {
            const unsigned char* bytes = static_cast<const unsigned char*>(buffer);
            output.insert(output.end(), bytes, bytes + copied);
        }
        return report;
    }
};

template<class T> T item(unsigned i);
template<> long item<long>(unsigned i) { return long(17 * i + 2); }
template<> int item<int>(unsigned i) { return int(17 * i + 2); }
template<> type_point item<type_point>(unsigned i) {
    type_point point;
    std::memset(&point, 0, sizeof(point));
    point.m_x = short(i % 200);
    point.m_y = short(-int(i % 200));
    point.m_z = short(i % 2);
    return point;
}
template<> type_university item<type_university>(unsigned i) {
    type_university value;
    std::rotate(value.m_skills, value.m_skills + i % 4, value.m_skills + 4);
    return value;
}

template<class T> std::vector<unsigned char> bytes(const std::vector<T>& values) {
    std::vector<unsigned char> result(values.size() * sizeof(T));
    if (!result.empty())
        std::memcpy(&result[0], &values[0], result.size());
    return result;
}

template<class T> void inputFor(ScriptFile& file, const std::vector<T>& values) {
    file.input.push_back(unsigned(values.size()) & 255);
    file.input.push_back((unsigned(values.size()) >> 8) & 255);
    std::vector<unsigned char> payload = bytes(values);
    file.input.insert(file.input.end(), payload.begin(), payload.end());
}

template<class Ops, class T> bool roundTrips() {
    const unsigned counts[] = {0, 1, 2, 7, 16};
    const unsigned initial[] = {0, 1, 12};
    for (unsigned n = 0; n < 5; ++n) {
        std::vector<T> source;
        for (unsigned i = 0; i < counts[n]; ++i)
            source.push_back(item<T>(i));
        // The historical &vector[0] idiom is intentional. Reserved storage
        // lets the native fixture avoid a null address for zero-byte I/O;
        // the pinned Dinkumware implementation remains the codegen oracle.
        source.reserve(32);
        ScriptFile writer;
        if (!Ops::template save<T>(&writer, source) || !writer.valid
            || writer.requests.size() != 2 || writer.requests[0] != 2
            || writer.requests[1] != int(counts[n] * sizeof(T)))
            return false;
        ScriptFile expected;
        inputFor(expected, source);
        if (writer.output != expected.input)
            return false;
        for (unsigned old = 0; old < 3; ++old) {
            std::vector<T> destination(initial[old], item<T>(29));
            destination.reserve(32);
            ScriptFile reader;
            reader.input = writer.output;
            if (!Ops::template load<T>(&reader, destination) || !reader.valid
                || reader.requests.size() != 2 || reader.requests[0] != 2
                || reader.requests[1] != int(counts[n] * sizeof(T))
                || bytes(destination) != bytes(source))
                return false;
        }
    }
    return true;
}

template<class Ops, class T> bool partialReads() {
    std::vector<T> payload(3);
    for (unsigned i = 0; i < 3; ++i)
        payload[i] = item<T>(i);
    for (int first = 0; first < 2; ++first) {
        std::vector<T> destination(1, item<T>(29));
        const std::vector<unsigned char> before = bytes(destination);
        ScriptFile reader(first);
        inputFor(reader, payload);
        if (Ops::template load<T>(&reader, destination) || !reader.valid
            || reader.requests.size() != 1 || bytes(destination) != before)
            return false;
    }
    const int reports[] = {0, 1, int(sizeof(T)), int(3 * sizeof(T) - 1),
                           int(3 * sizeof(T)), -1};
    for (unsigned r = 0; r < 6; ++r) {
        std::vector<T> destination(1, item<T>(29));
        std::vector<T> expected(destination);
        expected.push_back(T());
        expected.push_back(T());
        std::vector<unsigned char> expectedBytes = bytes(expected);
        const std::vector<unsigned char> payloadBytes = bytes(payload);
        if (reports[r] > 0)
            std::memcpy(&expectedBytes[0], &payloadBytes[0], reports[r]);
        ScriptFile reader(ScriptFile::FULL, reports[r]);
        inputFor(reader, payload);
        bool result = Ops::template load<T>(&reader, destination);
        // Retail compares unsigned byte counts. A negative payload report
        // consequently passes that comparison; do not silently "fix" it.
        bool expectedResult = reports[r] < 0 || reports[r] == int(payloadBytes.size());
        if (result != expectedResult || !reader.valid || reader.requests.size() != 2
            || bytes(destination) != expectedBytes)
            return false;
    }
    return true;
}

template<class Ops> bool check() {
    if (sizeof(type_point) != 4 || sizeof(type_university) != 16
        || sizeof(short) != 2 || sizeof(int) != 4)
        return false;
    if (!roundTrips<Ops, int>() || !roundTrips<Ops, long>()
        || !roundTrips<Ops, type_point>() || !roundTrips<Ops, type_university>()
        || !partialReads<Ops, int>() || !partialReads<Ops, long>()
        || !partialReads<Ops, type_university>())
        return false;

    std::vector<long> destination(1, 77), payload(3, 123);
    ScriptFile reader(ScriptFile::FULL, 0);
    inputFor(reader, payload);
    reader.watchedPairs = &destination;
    reader.previousPair = 77;
    if (Ops::template load<long>(&reader, destination) || !reader.valid)
        return false;

    const unsigned counts[] = {0, 1, 7, 32767, 32768, 65535, 65536};
    for (unsigned i = 0; i < 7; ++i) {
        std::vector<int> source(counts[i], 19);
        source.reserve(source.size() + 1);
        ScriptFile writer(ScriptFile::FULL, 0);
        bool result = Ops::template save<int>(&writer, source);
        int signedCount = int(counts[i] & 65535);
        if (signedCount >= 32768)
            signedCount -= 65536;
        if (!writer.valid || writer.requests.size() != 2
            || writer.requests[1] != signedCount * 4 || result != (signedCount == 0)
            || writer.output.size() != 2 || writer.output[0] != (counts[i] & 255)
            || writer.output[1] != ((counts[i] >> 8) & 255))
            return false;
    }
    for (int first = 0; first < 2; ++first) {
        std::vector<long> source(3, 17);
        ScriptFile writer(first);
        if (Ops::template save<long>(&writer, source) || !writer.valid
            || writer.requests.size() != 1)
            return false;
    }
    return true;
}

// @CANDIDATES@

int main() {
    // @CHECKS@
    return 0;
}
