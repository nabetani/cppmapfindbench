
def nums(n)
    s = Math.log2(n).round
    (0..(s*2)).map{ |x|
        (2**(x/2.0)).round
    }.sort.uniq
end

(0...5).each do |b|
    key_size = 16<<b
    c = (2**20)/key_size
    build = %Q!clang++ -DKEY_SIZE=#{key_size} -O2 -std=c++17 -Wall main.cpp!
    p build
    %x(#{build})
    nums(500).each do |e|
        fn = "data/k#{key_size}_c#{c}_e#{e}.csv"
        cmd = %Q!./a.out #{c} #{e} > #{fn}!
        p cmd
        %x(#{cmd})
    end
end
