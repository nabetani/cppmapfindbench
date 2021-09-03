#include <vector>
#include <map>
#include <string>
#include <cstdlib>
#include <algorithm>
#include <unordered_map>
#include <chrono>
#include <iostream>

using key_t = std::tuple<void *, uint64_t>;
using value_t = size_t;

key_t key_at(size_t ix)
{
    return {(void *)(ix*32), (uint64_t)~0ull - ix};
}

void insert(std::vector<std::pair<key_t, value_t>> &c, size_t ix, size_t v)
{
    c.push_back(std::pair<key_t, value_t>{key_at(ix), v});
}

std::vector<std::pair<key_t, value_t>>::const_iterator //
find (std::vector<std::pair<key_t, value_t>> const &c, key_t const & k ){
    return std::find_if( c.cbegin(), c.cend(), [&]( std::pair<key_t, value_t> const & v ){
        return v.first == k;
    });
}

template <typename con_t, typename k_t, typename v_t>
struct runner
{
    std::vector<con_t> cons_;
    size_t ecount_;
    std::string name_;

    con_t create_con(size_t i, size_t ec)
    {
        con_t c;
        for (size_t e = 0; e < ec; ++e)
        {
            insert(c, (i * ec + e) % ecount_, i * ec + e);
        }
        return c;
    }

    runner(size_t csize, size_t ecount, char const *name)
        : cons_(csize),
          ecount_(ecount),
          name_(name)
    {
        for (size_t i = 0; i < cons_.size(); ++i)
        {
            cons_[i] = create_con(i, ecount_);
        }
    }

    uint64_t run(bool measure)
    {
        uint64_t sum = 0;
        auto k = key_at(0);
        using clock = std::chrono::high_resolution_clock;
        auto start = clock::now();
        for (auto c : cons_)
        {
            auto it = find(c,k);
            if (it != c.end()){
                sum += it->second;
            }
        }
        auto diff = clock::now() - start;
        std::cout << "t=" << std::chrono::duration_cast<std::chrono::microseconds>(diff).count() * 1e-3 << std::endl;
        return sum;
    }
};

int main(int argc, char const *argv[])
{
    size_t csize = argc < 2 ? 100 : std::atoi(argv[1]);
    size_t ecount = argc < 3 ? 10 : std::atoi(argv[2]);

    runner<std::vector<std::pair<key_t, value_t>>, key_t, value_t> vec(csize, ecount, "vector");

    for (int i = 0; i < 2; ++i)
    {
        std::cout << vec.run(0<i) << std::endl;
        // run<std::map<key_t, value_t>>(csize, ecount, "map");
        // run<std::unordered_map<key_t, value_t>>(csize, ecount, "uomap");
    }
    return 0;
}
