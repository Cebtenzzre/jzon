#include "jzon.h"
#include <stdio.h>
#include <stdlib.h>
#include <vector>

struct Stat {
    size_t objectCount;
    size_t arrayCount;
    size_t numberCount;
    size_t stringCount;
    size_t trueCount;
    size_t falseCount;
    size_t nullCount;

    size_t memberCount;  // Number of members in all objects
    size_t elementCount; // Number of elements in all arrays
    size_t stringLength; // Number of code units in all strings
};

static void GenStat(Stat &stat, jzon::view v) {
    switch (v.tag()) {
    case jzon::array_tag:
        for (size_t i = 0; i < v.size(); ++i)
            GenStat(stat, v[i]);
        stat.elementCount += v.size();
        stat.arrayCount++;
        break;

    case jzon::object_tag:
        for (size_t i = 0; i < v.size(); i += 2) {
            stat.stringLength += strlen(v[i].to_string());
            GenStat(stat, v[i + 1]);
        }
        stat.memberCount += v.size() / 2;
        stat.stringCount += v.size() / 2;
        stat.objectCount++;
        break;

    case jzon::string_tag:
        stat.stringCount++;
        stat.stringLength += strlen(v.to_string());
        break;

    case jzon::number_tag:
        stat.numberCount++;
        break;

    case jzon::bool_tag:
        if (v.to_bool())
            stat.trueCount++;
        else
            stat.falseCount++;
        break;

    case jzon::null_tag:
        stat.nullCount++;
        break;
    }
}

int main(int argc, char **argv) {
    for (int i = 1; i < argc; ++i) {
        FILE *fp = fopen(argv[i], "r");
        if (!fp) {
            perror(argv[i]);
            exit(EXIT_FAILURE);
        }

        fseek(fp, 0, SEEK_END);
        size_t size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        std::vector<char> source;
        source.resize(size + 1);
        fread(source.data(), 1, size, fp);
        fclose(fp);

        auto doc = jzon::parser::parse(source.data());
        auto root = jzon::view(doc);
        Stat stat = {};
        GenStat(stat, root);

        printf("%s: %zd %zd %zd %zd %zd %zd %zd %zd %zd %zd\n",
               argv[i],
               stat.objectCount,
               stat.arrayCount,
               stat.numberCount,
               stat.stringCount,
               stat.trueCount,
               stat.falseCount,
               stat.nullCount,
               stat.memberCount,
               stat.elementCount,
               stat.stringLength);
    }

    return 0;
}