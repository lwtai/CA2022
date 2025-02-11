# Cache Behavior Simulation Project

## Course Information
- **Course**: CS 410000 - Computer Architecture
- **Semester**: Fall 2022
- **Professor**: Prof. Ting-Ting Hwang

## Project Overview
This project implements a cache behavior simulator that analyzes and optimizes cache performance. The project focuses on:

1. Implementation of cache policies (LRU replacement policy) - 70%
2. Design of an optimized cache indexing scheme to minimize cache conflict misses - 30%

## Project Components

### Two Main Programs
1. `arch_final_lsb`: Implements LSB (Least Significant Bits) indexing scheme
2. `arch_final_opt`: Implements optimized indexing scheme

### Features
- Variable cache configurations (sets, associativity, block size)
- LRU (Least Recently Used) replacement policy
- Configurable address bit width
- Cache hit/miss tracking
- Performance analysis

## Input/Output Format

### Input Files
1. `cache.org`: Cache configuration file
   ```
   Address_bits: 6
   Number_of_sets: 4
   Associativity: 2
   Block_size: 1
   ```

2. `reference.lst`: Memory access pattern file
   ```
   .benchmark testcase1
   000000
   000100
   ...
   .end
   ```

### Output File
- `index.rpt`: Report file containing
  - Cache configuration details
  - Indexing scheme information
  - Access results (hit/miss)
  - Total cache miss count

## Requirements

### Compilation
- Must be implemented in C/C++
- Compilable with g++ compiler

```bash
# Compile LSB version
g++ -o arch_final_lsb arch_final_lsb.cpp

# Compile optimized version
g++ -o arch_final_opt arch_final_opt.cpp
```

### Execution
```bash
./arch_final_lsb cache.org reference.lst index.rpt
./arch_final_opt cache.org reference.lst index.rpt
```
