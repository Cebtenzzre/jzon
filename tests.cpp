#include "jzon.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>

#if __GNUC__ >= 3
#define __noinline __attribute__((noinline))
#define __used __attribute__((used))
#define __unused __attribute__((unused))
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define __noinline
#define __used
#define __unused
#define likely(x) (x)
#define unlikely(x) (x)
#endif

void pvalue(jzon::value v) {
    printf("%2d %6d %4x %15lu %13g\n", v.number == v.number, v.is_nan(), v.tag, v.payload, v.number);
}

bool parse_double(const char *json, double *d) {
    auto doc = jzon::parser::parse(json);
    if (!doc.empty()) {
        auto root = jzon::view(doc);
        if (root.is_array() && root.size() == 1 && root[0].is_number()) {
            *d = root[0].to_number();
            return true;
        }
    }
    return false;
}

int main(int __unused argc, char __unused **argv) {
    for (size_t i = 0; i < 310; ++i)
        printf("exp10(%zd) = %e\n", i, jzon::parser::exp10(i));
    printf("exp10(%zu) = %e\n", ~0ul, jzon::parser::exp10(~0ul));

    printf("%2s %6s %4s %15s %13s\n", "==", "is_nan", "tag", "payload", "number");

    double numbers[] = {0.0, 1.0, 1.0 / 3.0, 5.45, 7.62, 1e40, DBL_MIN, DBL_EPSILON, DBL_MAX, INFINITY, NAN};
    for (auto n : numbers) {
        pvalue(-n);
        pvalue(n);
    }

    pvalue({0, jzon::null_tag});
    pvalue({false, jzon::bool_tag});
    pvalue({true, jzon::bool_tag});
    pvalue({0xDEADBEEF, jzon::string_tag});
    pvalue({0xBADCAB1E, jzon::string_tag});
    pvalue({~1ul, jzon::array_tag});
    pvalue({0xFFFFFFFFul, jzon::object_tag});

#define TEST_DOUBLE(json, rexpect)                                                                                                     \
    {                                                                                                                                  \
        double actual = 0.0;                                                                                                           \
        double expect = rexpect;                                                                                                       \
        if (parse_double(json, &actual))                                                                                               \
            if (expect != actual)                                                                                                      \
                fprintf(stderr, "%s:%d: error: parsing %s failed\n    expect: %24.17g (0x%016lx)\n    actual: %24.17g (0x%016lx)\n\n", \
                        __FILE__, __LINE__, json, expect, *(unsigned long *)&expect, actual, *(unsigned long *)&actual);               \
    }
    TEST_DOUBLE("[0.0]", 0.0);
    TEST_DOUBLE("[-0.0]", -0.0);
    TEST_DOUBLE("[1.0]", 1.0);
    TEST_DOUBLE("[-1.0]", -1.0);
    TEST_DOUBLE("[1.5]", 1.5);
    TEST_DOUBLE("[-1.5]", -1.5);
    TEST_DOUBLE("[3.1416]", 3.1416);
    TEST_DOUBLE("[1E10]", 1E10);
    TEST_DOUBLE("[1e10]", 1e10);
    TEST_DOUBLE("[1E+10]", 1E+10);
    TEST_DOUBLE("[1E-10]", 1E-10);
    TEST_DOUBLE("[-1E10]", -1E10);
    TEST_DOUBLE("[-1e10]", -1e10);
    TEST_DOUBLE("[-1E+10]", -1E+10);
    TEST_DOUBLE("[-1E-10]", -1E-10);
    TEST_DOUBLE("[1.234E+10]", 1.234E+10);
    TEST_DOUBLE("[1.234E-10]", 1.234E-10);
    TEST_DOUBLE("[1.79769e+308]", 1.79769e+308);
    TEST_DOUBLE("[2.22507e-308]", 2.22507e-308);
    TEST_DOUBLE("[-1.79769e+308]", -1.79769e+308);
    TEST_DOUBLE("[-2.22507e-308]", -2.22507e-308);
    TEST_DOUBLE("[4.9406564584124654e-324]", 4.9406564584124654e-324); // minimum denormal
    TEST_DOUBLE("[2.2250738585072009e-308]", 2.2250738585072009e-308); // Max subnormal double
    TEST_DOUBLE("[2.2250738585072014e-308]", 2.2250738585072014e-308); // Min normal positive double
    TEST_DOUBLE("[1.7976931348623157e+308]", 1.7976931348623157e+308); // Max double
    TEST_DOUBLE("[1e-10000]", 0.0);                                    // must underflow
    TEST_DOUBLE("[18446744073709551616]", 18446744073709551616.0);     // 2^64 (max of uint64_t + 1, force to use double)
    TEST_DOUBLE("[-9223372036854775809]", -9223372036854775809.0);     // -2^63 - 1(min of int64_t + 1, force to use double)
    TEST_DOUBLE("[0.9868011474609375]", 0.9868011474609375);           // https://github.com/miloyip/rapidjson/issues/120
    TEST_DOUBLE("[123e34]", 123e34);                                   // Fast Path Cases In Disguise
    TEST_DOUBLE("[45913141877270640000.0]", 45913141877270640000.0);
    TEST_DOUBLE("[2.2250738585072011e-308]", 2.2250738585072011e-308); // http://www.exploringbinary.com/php-hangs-on-numeric-value-2-2250738585072011e-308/
    //TEST_DOUBLE("[1e-00011111111111]", 0.0);
    //TEST_DOUBLE("[-1e-00011111111111]", -0.0);
    TEST_DOUBLE("[1e-214748363]", 0.0);
    TEST_DOUBLE("[1e-214748364]", 0.0);
    //TEST_DOUBLE("[1e-21474836311]", 0.0);
    TEST_DOUBLE("[0.017976931348623157e+310]", 1.7976931348623157e+308); // Max double in another form

    // Since
    // abs((2^-1022 - 2^-1074) - 2.2250738585072012e-308) = 3.109754131239141401123495768877590405345064751974375599... ¡Á 10^-324
    // abs((2^-1022) - 2.2250738585072012e-308) = 1.830902327173324040642192159804623318305533274168872044... ¡Á 10 ^ -324
    // So 2.2250738585072012e-308 should round to 2^-1022 = 2.2250738585072014e-308
    TEST_DOUBLE("[2.2250738585072012e-308]", 2.2250738585072014e-308); // http://www.exploringbinary.com/java-hangs-when-converting-2-2250738585072012e-308/

    // More closer to normal/subnormal boundary
    // boundary = 2^-1022 - 2^-1075 = 2.225073858507201136057409796709131975934819546351645648... ¡Á 10^-308
    TEST_DOUBLE("[2.22507385850720113605740979670913197593481954635164564e-308]", 2.2250738585072009e-308);
    TEST_DOUBLE("[2.22507385850720113605740979670913197593481954635164565e-308]", 2.2250738585072014e-308);

    // 1.0 is in (1.0 - 2^-54, 1.0 + 2^-53)
    // 1.0 - 2^-54 = 0.999999999999999944488848768742172978818416595458984375
    TEST_DOUBLE("[0.999999999999999944488848768742172978818416595458984375]", 1.0);                 // round to even
    TEST_DOUBLE("[0.999999999999999944488848768742172978818416595458984374]", 0.99999999999999989); // previous double
    TEST_DOUBLE("[0.999999999999999944488848768742172978818416595458984376]", 1.0);                 // next double
    // 1.0 + 2^-53 = 1.00000000000000011102230246251565404236316680908203125
    TEST_DOUBLE("[1.00000000000000011102230246251565404236316680908203125]", 1.0);                 // round to even
    TEST_DOUBLE("[1.00000000000000011102230246251565404236316680908203124]", 1.0);                 // previous double
    TEST_DOUBLE("[1.00000000000000011102230246251565404236316680908203126]", 1.00000000000000022); // next double

    // Numbers from https://github.com/floitsch/double-conversion/blob/master/test/cctest/test-strtod.cc

    TEST_DOUBLE("[72057594037927928.0]", 72057594037927928.0);
    TEST_DOUBLE("[72057594037927936.0]", 72057594037927936.0);
    TEST_DOUBLE("[72057594037927932.0]", 72057594037927936.0);
    TEST_DOUBLE("[7205759403792793199999e-5]", 72057594037927928.0);
    TEST_DOUBLE("[7205759403792793200001e-5]", 72057594037927936.0);

    TEST_DOUBLE("[9223372036854774784.0]", 9223372036854774784.0);
    TEST_DOUBLE("[9223372036854775808.0]", 9223372036854775808.0);
    TEST_DOUBLE("[9223372036854775296.0]", 9223372036854775808.0);
    TEST_DOUBLE("[922337203685477529599999e-5]", 9223372036854774784.0);
    TEST_DOUBLE("[922337203685477529600001e-5]", 9223372036854775808.0);

    TEST_DOUBLE("[10141204801825834086073718800384]", 10141204801825834086073718800384.0);
    TEST_DOUBLE("[10141204801825835211973625643008]", 10141204801825835211973625643008.0);
    TEST_DOUBLE("[10141204801825834649023672221696]", 10141204801825835211973625643008.0);
    TEST_DOUBLE("[1014120480182583464902367222169599999e-5]", 10141204801825834086073718800384.0);
    TEST_DOUBLE("[1014120480182583464902367222169600001e-5]", 10141204801825835211973625643008.0);

    TEST_DOUBLE("[5708990770823838890407843763683279797179383808]", 5708990770823838890407843763683279797179383808.0);
    TEST_DOUBLE("[5708990770823839524233143877797980545530986496]", 5708990770823839524233143877797980545530986496.0);
    TEST_DOUBLE("[5708990770823839207320493820740630171355185152]", 5708990770823839524233143877797980545530986496.0);
    TEST_DOUBLE("[5708990770823839207320493820740630171355185151999e-3]", 5708990770823838890407843763683279797179383808.0);
    TEST_DOUBLE("[5708990770823839207320493820740630171355185152001e-3]", 5708990770823839524233143877797980545530986496.0);

    {
        char n1e308[312]; // '1' followed by 308 '0'
        n1e308[0] = '[';
        n1e308[1] = '1';
        for (int j = 2; j < 310; j++)
            n1e308[j] = '0';
        n1e308[310] = ']';
        n1e308[311] = '\0';
        TEST_DOUBLE(n1e308, 1E308);
    }

    // Cover trimming
    TEST_DOUBLE(
        "[2.22507385850720113605740979670913197593481954635164564802342610972482222202107694551652952390813508"
        "7914149158913039621106870086438694594645527657207407820621743379988141063267329253552286881372149012"
        "9811224514518898490572223072852551331557550159143974763979834118019993239625482890171070818506906306"
        "6665599493827577257201576306269066333264756530000924588831643303777979186961204949739037782970490505"
        "1080609940730262937128958950003583799967207254304360284078895771796150945516748243471030702609144621"
        "5722898802581825451803257070188608721131280795122334262883686223215037756666225039825343359745688844"
        "2390026549819838548794829220689472168983109969836584681402285424333066033985088644580400103493397042"
        "7567186443383770486037861622771738545623065874679014086723327636718751234567890123456789012345678901"
        "e-308]",
        2.2250738585072014e-308);

    return 0;
}
