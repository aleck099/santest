#include <regex>
#include <string>
#include <bitset>
#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>
#include <cassert>

const std::string param_delim = "\uffff";
const std::string message_delim = "\ufffe";
std::string SanitizeParameter(std::string param) {
    return std::regex_replace(std::regex_replace(param, std::regex(param_delim), ""), std::regex(message_delim), "");
}

constexpr std::string_view keywords[] = {"\uffff", "\ufffe"};
constexpr size_t k_size = sizeof(keywords) / sizeof(std::string_view);

std::string SanitizeParameter2(std::string_view param) {
    std::string r;
    r.reserve(param.size());

    std::bitset<k_size> searching_marks;

    size_t candidate_index{};
    for (size_t i = 0; i != param.size(); ++i) {
        if (candidate_index == 0) {
            bool found = false;
            for (size_t j = 0; j != k_size; ++j) {
                assert(!keywords[j].empty());
                if (keywords[j][0] == param[i]) {
                    searching_marks.set(j);
                    found = true;
                }
            }

            if (found)
                candidate_index = 1;
            else
                r += param[i];
        } else {
            bool found = false;
            bool match = false;
            for (size_t j = 0; j != k_size; ++j) {
                if (searching_marks.test(j)) {
                    if (keywords[j][candidate_index] == param[i]) {
                        found = true;
                        if (keywords[j].size() == candidate_index + 1) {
                            match = true;
                            break;
                        }
                    } else {
                        searching_marks.reset(j);
                    }
                }
            }

            if (match) {
                candidate_index = 0;
            } else if (found) {
                ++candidate_index;
            } else {
                r.append(param.substr(i - candidate_index, candidate_index));
                candidate_index = 0;
            }
        }
    }
    if (candidate_index != 0)
        r.append(param.substr(param.length() - candidate_index));
    return r;
}

std::string SanitizeParameter3(std::string param) {
    auto remove_all = [] (std::string& s, std::string_view what) {
        size_t p{};
        size_t count{};
        while (true) {
            p = s.find(what, p);
            if (p == s.npos)
                break;
            s.replace(p, what.size(), "");
            ++count;
            p -= what.size();
        }
        return count;
    };
    while (remove_all(param, keywords[0]));
    while (remove_all(param, keywords[1]));
    return param;
}

std::vector<std::string> load_data(std::istream& s) {
    std::vector<std::string> v;
    while (s && !s.eof()) {
        std::string l;
        std::getline(s, l);
        if (l.empty())
            continue;
        v.emplace_back(std::move(l));
    }
    return v;
}

int main() {
    std::vector<std::string> data;
    {
        std::ifstream f("data.txt");
        if (!f) {
            std::cout << "load data failed" << std::endl;
            return 1;
        }
        data = load_data(f);
    }

    auto perf_test = [&data] (auto f) {
        auto t1 = std::chrono::high_resolution_clock::now();
        for (const auto& e : data) {
            std::invoke(f, e);
        }
        auto t2 = std::chrono::high_resolution_clock::now();
        return (t2 - t1).count();
    };

    std::cout << "Timing:" << std::endl;
    std::cout << "T1 " << perf_test(SanitizeParameter) << std::endl;
    std::cout << "T2 " << perf_test(SanitizeParameter2) << std::endl;
    std::cout << "T3 " << perf_test(SanitizeParameter3) << std::endl;

    std::cout << "Result:" << std::endl;
    for (size_t i = 0; i != data.size(); ++i) {
        auto r1 = SanitizeParameter(data[i]);
        auto r2 = SanitizeParameter2(data[i]);
        auto r3 = SanitizeParameter3(data[i]);
        if (r1 != r2 || r1 != r3) {
            std::cout << "Failed" << i << std::endl;
        }
    }
    return 0;
}

