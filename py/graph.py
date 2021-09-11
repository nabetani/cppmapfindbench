import pathlib
import glob
import matplotlib.pyplot as plt
import numpy as np
import matplotlib
matplotlib.use('Agg')


def plot(k, data0):
    data = [row for row in data0 if row[2]<300]
    fig = plt.figure(figsize=(6, 3), dpi=200)
    ax = fig.add_subplot(1, 1, 1)
    ax.set_yscale("log")
    labels = [row[2] for row in data if row[0]==k]
    print(repr(labels))
    nmax=len(labels)//3
    ticks = [ (len(labels)-1)*n//nmax for n in range(nmax+1)]
    ax.set_xticks(ticks)
    ax.set_xticklabels([int(labels[ti]) for ti in ticks])

    algos = [
        ["std::map", 3],
        ["std::unordered_map", 4],
        ["linear", 5],
        ["boost flat_map", 6]]
    for name, ix in algos:
        t = [row[ix] for row in data if row[0] == k]
        ax.plot(t,
                label=name)


    title = "key is uint32 × %d" % k
    ax.set_title(title)
    ax.legend()
    fig.subplots_adjust()
    plt.savefig(title+".png")


def main():
    r = []
    for fn in glob.glob("../data/*.csv"):
        v = [int(fn.split("/")[-1].split("_")[0][1:10])]
        with open(fn) as f:
            line = f.readline().strip()
            v += [float(x) for x in line.split(",")]
        r.append(v)
    r.sort()
    for k in [1, 2, 4, 16, 256]:
        plot(k, r)


main()
