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

using key_type = std::pair<void *volatile, uint64_t volatile>;
using value_t = size_t;

struct hasher_t
{
    typedef std::size_t result_type;
    std::size_t operator()(key_type const &key) const noexcept
    {
        return (intptr_t)key.first ^ (intptr_t)key.second;
    }
};

using uomap_t = std::unordered_map<key_type, value_t, hasher_t>;

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

key_type key_at(size_t ix)
{
    return {(void *)(32 + ix * 32), ix};
}

std::string str(std::pair<key_type, value_t> const &v)
{
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
        using clock = std::chrono::high_resolution_clock;
        auto start = clock::now();
        size_t kix = 0;
        for (auto const & c : cons_)
        {
            // std::cout << "[" << c.size() << "]";
            auto k = key_at(kix);
            kix = (kix + 1) % (ecount_ + 1);
            auto it = c.find(k);
            if (it != c.end())
            {
                // std::cout << it->second << " ";
                sum += it->second;
            }
            else
            {
                // std::cout << " - ";
            }
            // int ix = 0;
        }
        // std::endl(std::cout);
        auto diff = clock::now() - start;
        if (measure)
        {
            std::cout << name_ << ": t=" << std::chrono::duration_cast<std::chrono::microseconds>(diff).count() * 1e-3 << std::endl;
        }
        return sum;
    }
};

int main(int argc, char const *argv[])
{
    size_t csize = argc < 2 ? 100 : std::atoi(argv[1]);
    size_t ecount = argc < 3 ? 10 : std::atoi(argv[2]);

    std::printf("csize=%zu ecount=%zu\n", csize, ecount);

    // runner<std::vector<std::pair<key_type, value_t>>, key_type, value_t> mvec(csize, ecount, "vector");
    runner<std::map<key_type, value_t>, key_type, value_t> mmap(csize, ecount, "map");
    runner<uomap_t, key_type, value_t> muomap(csize, ecount, "uomap");
    runner<linemap_t, key_type, value_t> mlinemap(csize, ecount, "linemap");

    linemap<int, int> m;
    for (int i = 0; i < 256; ++i)
    {
        m[i ^ 0xaa] = i;
    }
    for (int i = 0; i < 256; ++i)
    {
        auto it = m.find(i);
        if (it == m.end())
        {
            printf("failed to find %d\n", i);
            throw "1";
        }
        if (it->second != (i ^ 0xaa))
        {
            printf("it->second is %x, not %x\n", it->second, i ^ 0xaa);
            throw "2";
        }
    }

    std::vector<uint64_t> s;
    for (int i = 0; i < 3; ++i)
    {
        s.push_back(mmap.run(0 < i));
        s.push_back(muomap.run(0 < i));
        s.push_back(mlinemap.run(0 < i));
    }
    for (auto e : s)
    {
        std::cout << e << " ";
    }
    std::cout << std::endl;
    return 0;
}
