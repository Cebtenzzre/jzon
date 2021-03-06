#pragma once

#include "dtoa_milo.h"
#include "gason2.h"
#include <stdio.h>
#include <math.h>

namespace gason2 {
struct dump {
    static void writeDouble(vector<char> &s, double d) {
        if (isnan(d)) {
            s.append("NaN", 3);
            return;
        }

        if (isinf(d)) {
            if (signbit(d)) {
                s.reserve(s.size() + 9);
                s.push_back('-');
            }

            s.append("Infinity", 8);
            return;
        }

        if (d == 0) {
            if (signbit(d)) {
                s.reserve(s.size() + 4);
                s.push_back('-');
            }
            s.append("0.0", 3);
            return;
        }

        char buffer[25];
        char *buf_p = buffer;

        if (signbit(d)) {
            *buf_p++ = '-';
            d = -d;
        }

        int length, K;
        Grisu2(d, buf_p, &length, &K);
        char *end = Prettify(buf_p, length, K, 324);

        s.append(buf_p, end - buf_p);
    }

    static void stringify(vector<char> &s, value v) {
        switch (v.type()) {
        case type::number:
            writeDouble(s, v.to_number());
            break;

        case type::null:
            s.append("null", 4);
            break;

        case type::boolean:
            if (v.to_bool())
                s.append("true", 4);
            else
                s.append("false", 5);
            break;

        case type::string:
            s.push_back('"');
            for (const char *p = v.to_string(); *p;) {
                char c = *p++;
                switch (c) {
                case '\b':
                    s.append("\\b", 2);
                    break;
                case '\f':
                    s.append("\\f", 2);
                    break;
                case '\n':
                    s.append("\\n", 2);
                    break;
                case '\r':
                    s.append("\\r", 2);
                    break;
                case '\t':
                    s.append("\\t", 2);
                    break;
                case '\\':
                    s.append("\\\\", 2);
                    break;
                case '"':
                    s.append("\\\"", 2);
                    break;
                default:
                    s.push_back(c);
                }
            }
            s.push_back('"');
            break;

        case type::array:
            if (v.size()) {
                char comma = '[';
                for (auto i : v.elements()) {
                    s.push_back(comma);
                    stringify(s, i);
                    comma = ',';
                }
            } else {
                s.push_back('[');
            }
            s.push_back(']');
            break;

        case type::object:
            if (v.size()) {
                char comma = '{';
                for (auto i : v.members()) {
                    s.push_back(comma);
                    stringify(s, i.name());
                    s.push_back(':');
                    stringify(s, i.value());
                    comma = ',';
                }
            } else {
                s.push_back('{');
            }
            s.push_back('}');
            break;
        default:
            break;
        }
    }

    static void indent(vector<char> &s, size_t depth) {
        if (s.size())
            s.push_back('\n');
        while (depth--)
            s.append("\x20\x20\x20\x20", 4);
    }

    static void prettify(vector<char> &s, value v, size_t depth = 0) {
        switch (v.type()) {
        case type::array:
            if (v.size()) {
                char comma = '[';
                for (auto i : v.elements()) {
                    s.push_back(comma);
                    indent(s, depth + 1);
                    prettify(s, i, depth + 1);
                    comma = ',';
                }
                indent(s, depth);
            } else {
                s.push_back('[');
            }
            s.push_back(']');
            break;

        case type::object:
            if (v.size()) {
                char comma = '{';
                for (auto i : v.members()) {
                    s.push_back(comma);
                    indent(s, depth + 1);
                    stringify(s, i.name());
                    s.append(": ", 2);
                    prettify(s, i.value(), depth + 1);
                    comma = ',';
                }
                indent(s, depth);
            } else {
                s.push_back('{');
            }
            s.push_back('}');
            break;

        default:
            stringify(s, v);
        }
    }

    static int format_error(char *str, size_t n, const char *filename, char *json, const char *end, const document &doc) {
        assert(json <= end);
        int lineno = 1;
        const char *left = json;
        char *right = json;
        char *endptr = json + doc.error_offset();
        while (right < end) {
            if (*right == '\n') {
                ++right;
                if (endptr < right)
                    break;
                left = right;
                ++lineno;
                continue;
            }
            if (!*right)
                *right = ' ';
            ++right;
        }

        int column = endptr - left;
        if (column > 80)
            left = endptr - 40;
        if (right - left > 80)
            right = endptr + 80 - (endptr - left);

        const char *desc = "";
        // clang-format off
        switch (doc.error_code()) {
        case error::expecting_string: desc = "expecting string"; break;
        case error::expecting_value: desc = "expecting value"; break;
        case error::invalid_literal_name: desc = "invalid literal name"; break;
        case error::invalid_number: desc = "invalid number"; break;
        case error::invalid_string_char: desc = "invalid string char"; break;
        case error::invalid_string_escape: desc = "invalid string escape"; break;
        case error::invalid_surrogate_pair: desc = "invalid surrogate pair"; break;
        case error::missing_colon: desc = "missing colon"; break;
        case error::missing_comma_or_bracket: desc = "missing comma or bracket"; break;
        case error::unexpected_character: desc = "unexpected character"; break;
        }
        // clang-format on

        return snprintf(str, n, "%s:%d:%d: error: %s\n%.*s\n%*s\n", filename, lineno, column, desc, int(right - left), left, int(endptr - left), "^");
    }

    static int print_error(const char *filename, char *json, char const *end, const document &doc) {
        char buffer[256];
        int n = format_error(buffer, sizeof(buffer), filename, json, end, doc);
        if (n > 0)
            return fwrite(buffer, 1, n, stderr);
        return n;
    }
};
} // namespace gason2
