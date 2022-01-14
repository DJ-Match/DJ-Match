[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.5851268.svg)](https://doi.org/10.5281/zenodo.5851268)

# DJ Match

**DJ Match** implements a number of different algorithms to solve the **k-Disjoint Matchings**
problem: Given an edge-weighted graph, find a set of k pairwise edge-disjoint matchings
such that the total weight of the edges contained in one of the k matchings is maximized.
This problem appears in the context of reconfigurable optical technologies for
data centers.

This repository contains the source code to our INFOCOM'22 paper
**Fast and Heavy Disjoint Weighted Matchings for Demand-Aware Datacenter Topologies**.


## Compiling the program

This program is written in C++17 and built on top of the modular algorithm framework
[Algora](https://libalgora.gitlab.io).
Building it requires selected boost libraries as well as `qmake` version 5.

On Debian/Ubuntu, all dependencies can be installed by running: `# apt install
qt5-qmake libboost-dev`.
On Fedora, run `# dnf install qt5-qtbase-devel boost-devel`.
On FreeBSD, dependencies can be installed with  `# pkg install qt5-qmake boost-libs`.

Before you can compile **DJ Match**, you need to obtain and compile the Algora
libraries
[Algora|Core](https://gitlab.com/libalgora/AlgoraCore), version 1.3 or greater,
and
[Algora|Dyn](https://gitlab.com/libalgora/AlgoraDyn), version 1.2 or greater.
The build configuration expects to find them in `Algora/AlgoraCore` and `Algora/AlgoraDyn`.
You can simply run `$ ./downloadAndCompileAlgora` to clone and compile both libraries.
Alternatively, you can put these libraries also elsewhere and manually
create the corresponding symlinks, for example.
Do not forget to compile Algora|Core and Algora|Dyn before you try to build
**DJ Match**!
The respective READMEs provide for further instructions.

Once these preparations are done, you can run `$ ./compile` to build **DJ Match**
with the standard compiler. Run `$ ./compile --help` to see additional options.
The compiled binaries can be found in the `build/Debug` and `build/Release`
subdirectories.

Alternatively or on other OSes, you can manually run qmake on `src/DJMatch.pro`
or open the file in an IDE like QtCreator.

## Running DJ Match

The `examples` directory contains three small Kronecker instances with 1024 vertices.
To find 4 and 8 disjoint matchings using the algorithms **k-EC**, **GreedyIt**, and **NodeCentered** on instance `examples/rmat_b_10_13_exp.graph`,
where **GreedyIt** is run both with the postprocessing routine **LocalSwaps** and without
and **NodeCentered** with thresholds 0.2 and 0.5 and aggregation functions kSUM and SUM,
run the following command:
```
build/Release/DJMatch --b=4 --b=8 -a k-ec -a greedy-it -a nodecentered -g bsum -g sum -t 0.2 -t 0.5 --swaps-and-normal  examples/rmat_b_10_13_exp.graph
```
Note that for historic reasons, the number of matchings is denoted by b here, not k.
The expected format of the input files is one line per edge, where each line has the format
```
<node id> <node id> <weight/demand> 0
```

## External Projects

Besides Algora, we use or adapted code from the following projects:
- [argtable3](https://github.com/argtable/argtable3), shipped in `src/extern/`
- [Boost 1.74](https://www.boost.org/users/history/version_1_74_0.html)

## License

**Algora** and **DJ Match** are free software and licensed under the GNU General Public License
version 3.  See also the file `COPYING`.

The licenses of files (derived) from external projects (argtable3, Boost) may deviate and
can be found in the respective head sections.

If you publish results using our algorithms, please acknowledge our work by
citing the corresponding paper:

```
@inproceedings{djmatch,
  author    = {Hanauer, Kathrin and Henzinger, Monika and Schmid, Stefan and Trummer, Jonathan},
  title     = {Fast and Heavy Disjoint Weighted Matchings for Demand-Aware Datacenter Topologies},
  booktitle = {{IEEE} Conference on Computer Communications, {INFOCOM} 2022,
               May 2-5, 2022},
  publisher = {{IEEE}},
  year      = {2022},
  note      = {to appear}
}
```

## Code Contributors (in alphabetic order by last name)

- Kathrin Hanauer
- Jonathan Trummer
