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
#include <array>
#include <unordered_map>
#include <boost/container/flat_map.hpp>

#if !defined KEY_SIZE
#define KEY_SIZE 1
#endif

using key_type = std::array<uint32_t, KEY_SIZE>;
using value_t = size_t;

struct hasher_t
{
    typedef std::size_t result_type;

    std::size_t operator()(key_type const &key) const noexcept
    {
        size_t r = 0;
        for (auto e : key)
        {
            r += r / 2 + r * 2 + e;
        }
        return r;
    }
};

template <typename key_t, typename mapped_t>
struct linemap
{
    using key_type = key_t;
    using mapped_type = mapped_t;
    using value_type = std::pair<key_type const, mapped_type>;

    std::vector<value_type> m_;

    using const_iterator = decltype(m_.cend());
    using iterator = decltype(m_.end());

    iterator end() { return m_.end(); }
    iterator begin() { return m_.begin(); }
    const_iterator end() const { return m_.cend(); }
    const_iterator begin() const { return m_.cbegin(); }
    const_iterator cend() const { return m_.cend(); }
    const_iterator cbegin() const { return m_.cbegin(); }

    const_iterator find(key_type const &k) const
    {
        return std::find_if(m_.cbegin(), m_.cend(), [&k](value_type const &v)
                            { return v.first == k; });
    }
    iterator find(key_type const &k)
    {
        return std::find_if(m_.begin(), m_.end(), [&k](value_type const &v)
                            { return v.first == k; });
    }
    mapped_type &operator[](key_type const &k)
    {
        auto it = find(k);
        if (it != end())
        {
            return it->second;
        }
        m_.push_back(value_type(k, mapped_type{}));
        return m_.back().second;
    }
    size_t size() const { return m_.size(); }
};

using linemap_t = linemap<key_type, value_t>;
using uomap_t = std::unordered_map<key_type, value_t, hasher_t>;
using map_t = std::unordered_map<key_type, value_t>;
using bfmap_t = boost::container::flat_map<key_type, value_t>;

key_type key_at(size_t ix)
{
    key_type k = {};
    *k.rbegin() = ix & ~0u;
    *(k.rbegin() + 1) = (ix >> 32) & ~0u;
    return k;
}

template <typename con_t, typename k_t, typename v_t>
struct runner
{
    int trial_count_ = 100;
    const size_t ecount_;
    std::string name_;
    std::mt19937 rng_{0};
    static constexpr size_t KEY_COUNT = 16;
    static constexpr size_t CON_COUNT = 16;
    std::array<k_t, KEY_COUNT> keys_;
    std::array<con_t, CON_COUNT> cons_;

    con_t create_con(size_t i)
    {
        std::mt19937 rng(i);
        std::uniform_int_distribution<size_t> vdist(0, 255u);
        std::vector<size_t> nums;
        for (size_t i = 0; i < ecount_; ++i)
        {
            nums.push_back(i);
        }
        std::shuffle(nums.begin(), nums.end(), rng);
        con_t c;
        for (auto n : nums)
        {
            auto k = key_at(n);
            c[k] = vdist(rng_);
        }
        return c;
    }

    runner(size_t csize, size_t ecount, char const *name)
        : trial_count_(csize),
          ecount_(ecount),
          name_(name)
    {
        for (size_t i = 0; i < cons_.size(); ++i)
        {
            cons_[i] = create_con(i);
        }
        for (size_t i = 0; i < keys_.size(); ++i)
        {
            keys_[i] = key_at(i);
        }
    }

    std::pair<uint64_t, double> run()
    {
        uint64_t sum = 0;
        using clock = std::chrono::high_resolution_clock;
        std::uniform_int_distribution<size_t> key_dist(0, keys_.size() - 1);
        std::uniform_int_distribution<size_t> con_dist(0, cons_.size() - 1);

        auto start = clock::now();
        for (int i = 0; i < trial_count_; ++i)
        {
            auto const &key = keys_[key_dist(rng_)];
            auto const &con = cons_[con_dist(rng_)];
            auto it = con.find(key);
            if (it != con.end())
            {
                sum += it->second;
            }
        }
        auto diff = clock::now() - start;
        double sec = std::chrono::duration_cast<std::chrono::nanoseconds>(diff).count() * 1e-9;
        return {sum, sec};
    }
};

int main(int argc, char const *argv[])
{
    size_t csize = argc < 2 ? 100 : std::atoi(argv[1]);
    size_t ecount = argc < 3 ? 10 : std::atoi(argv[2]);

    std::vector<int64_t> sums;

    // runner<std::vector<std::pair<key_type, value_t>>, key_type, value_t> mvec(csize, ecount, "vector");
    runner<std::map<key_type, value_t>, key_type, value_t> mmap(csize, ecount, "map");
    runner<uomap_t, key_type, value_t> muomap(csize, ecount, "uomap");
    runner<linemap_t, key_type, value_t> mlinemap(csize, ecount, "linemap");
    runner<bfmap_t,key_type, value_t> mbfmap(csize, ecount, "bfmap");
    std::vector<double> ticks;
    for (int i = 0; i < 3; ++i)
    {
        ticks.clear();
        {
            auto [s, t] = mmap.run();
            ticks.push_back(t);
            sums.push_back(s);
        }
        {
            auto [s, t] = muomap.run();
            ticks.push_back(t);
            sums.push_back(s);
        }
        {
            auto [s, t] = mlinemap.run();
            ticks.push_back(t);
            sums.push_back(s);
        }
        {
            auto [s, t] = mbfmap.run();
            ticks.push_back(t);
            sums.push_back(s);
        }
    }
    std::cout << csize << "," << ecount;
    for (auto t : ticks)
    {
        std::cout << "," << t;
    }
    std::cout << std::endl;
    for (auto e : sums)
    {
        std::cerr << e << " ";
    }
    std::cerr << std::endl;
    return 0;
}
