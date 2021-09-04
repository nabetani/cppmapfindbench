#include <vector>
#include <map>
#include <string>
#include <cstdlib>
#include <algorithm>
#include <unordered_map>
#include <chrono>
#include <iostream>
#include <sstream>
#include <random>
#include <unordered_map>

using key_type = std::pair<void *, uint64_t>;
using value_t = size_t;


struct hasher_t {
    typedef std::size_t result_type;
    std::size_t operator()( key_type const & key) const noexcept
    {
        return (intptr_t)key.first ^ (intptr_t)key.second;
    }
};

using uomap_t = std::unordered_map<key_type, value_t, hasher_t>;

key_type key_at(size_t ix)
{
    return {(void *)(ix*32), (uint64_t)~0ull - ix};
}

void insert(std::vector<std::pair<key_type, value_t>> &c, size_t ix, size_t v)
{
    c.push_back(std::pair<key_type, value_t>{key_at(ix), v});
}

std::vector<std::pair<key_type, value_t>>::const_iterator //
find (std::vector<std::pair<key_type, value_t>> const &c, key_type const & k ){
    return std::find_if( c.cbegin(), c.cend(), [&]( std::pair<key_type, value_t> const & v ){
        return v.first == k;
    });
}

void insert(std::map<key_type, value_t> &c, size_t ix, size_t v)
{
    c[key_at(ix)] = v;
}

std::map<key_type, value_t>::const_iterator //
find (std::map<key_type, value_t> const &c, key_type const & k ){
    return c.find(k);
}

void insert(uomap_t &c, size_t ix, size_t v)
{
    c[key_at(ix)] = v;
}

uomap_t::const_iterator //
find (uomap_t const &c, key_type const & k ){
    return c.find(k);
}

std::string str( std::pair<key_type, value_t> const & v ){
    std::stringstream ss;
    ss << (intptr_t)v.first.first << " " << v.first.second;
    return ss.str();
}

template <typename con_t, typename k_t, typename v_t>
struct runner
{
    std::vector<con_t> cons_;
    size_t ecount_;
    std::string name_;
    std::mt19937 rng_{0};

    con_t create_con(size_t i)
    {
        std::uniform_int_distribution<size_t> dist(0,ecount_);

        con_t c;
        for (size_t e = 0; e < ecount_; ++e)
        {
            insert(c, (dist(rng_)) % ecount_, i * ecount_ + e);
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
            cons_[i] = create_con(i);
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
        if (measure){
            std::cout << name_ << ": t=" << std::chrono::duration_cast<std::chrono::microseconds>(diff).count() * 1e-3 << std::endl;
        }
        return sum;
    }
};

int main(int argc, char const *argv[])
{
    size_t csize = argc < 2 ? 100 : std::atoi(argv[1]);
    size_t ecount = argc < 3 ? 10 : std::atoi(argv[2]);

    runner<std::vector<std::pair<key_type, value_t>>, key_type, value_t> mvec(csize, ecount, "vector");
    runner<std::map<key_type, value_t>, key_type, value_t> mmap(csize, ecount, "map");
    runner<uomap_t, key_type, value_t> muomap(csize, ecount, "uomap");

    uint64_t s=0;
    for (int i = 0; i < 2; ++i)
    {
        s += mvec.run(0<i);
        s += mmap.run(0<i);
        s += muomap.run(0<i);
    }
    std::cout << s << std::endl;
    return 0;
}
