/*
Написать программу на C++ 11 (или C++ 14) которая выполняет избыточные вычисления используя несколько потоков:
§  Потоки А-1..А-M генерируют блоки случайных данных. Количество потоков, блоков и размер блока задается параметрами коммандной строки. Количество блоков может быть очень большим.
§  Потоки Б-1..Б-N вычисляют CRC32 (можно использовать готовую реализацию). Количество потоков задается параметром коммандной строки.
§  Когда все потоки Б вычислят CRC32 для какого-то блока, надо сравнить полученные значения и если они не совпадают записать блок в файл и вывести сообщение в std::cout.
§  Потоки A и Б должны работать параллельно.
*/

#include <iostream>
#include <fstream>
#include <thread>
#include <future>
#include <mutex>
#include <string>
#include <atomic>
#include <list>
#include <limits>
#include <cstdlib>
#include <time.h>
#include <zlib.h>

//#define RANDOM_SEED 11
#define RANDOM_SEED time(0)

std::list<unsigned char*> block_queue;
std::mutex mx;

unsigned char* generate_block(int block_size) {
    unsigned char *p = new unsigned char [block_size];
    unsigned int seed = rand();
    for (int i = 0; i < block_size; ++i)
        p[i] = rand_r(&seed);
    return p;
}

void generate_blocks(int number_of_blocks, int block_size) {
    for (int i = 0; i < number_of_blocks; ++i) {
        unsigned char *p = generate_block(block_size);
        std::lock_guard<std::mutex> l(mx);
        block_queue.push_back(p);
    }
}

void producers(int threads, int number_of_blocks, int block_size) {
    std::thread t[threads];
    srand(RANDOM_SEED);
    t[0] = std::thread(generate_blocks, number_of_blocks - (threads - 1)*(number_of_blocks/threads), block_size);
    t[0].detach();
    for (int i = 1; i < threads; i++) {
        t[i] = std::thread(generate_blocks, number_of_blocks/threads, block_size);
        t[i].detach();
    }
}

inline uint32_t consume_block(unsigned char *p, int block_size) {
    return crc32(0, p, block_size);
}

void consumers(int threads, int number_of_blocks, int block_size) {
    std::future<uint32_t> f[threads - 1];
    for (int k = 0; k < number_of_blocks; ++k) {
        while (block_queue.empty()) {
            std::lock_guard<std::mutex> lk(mx);
        }
        unsigned char *p = block_queue.front();
        mx.lock();
        block_queue.pop_front();
        mx.unlock();
        for (int i = 1; i < threads; i++)
            f[i - 1] = std::async(std::launch::async, consume_block, p, block_size);
        int fm = consume_block(p, block_size);
        for (int i = 1; i < threads; i++)
            if (fm != f[i - 1].get()) {
                static int fc = 0;
                std::ofstream fo(std::to_string(++fc));
                fo.write((const char*)p, block_size);
                fo.close();
                std::cout << "bad block saved in file '" << fc << "'\n";
                break;
            }
        delete p;
    }
}

int main(int argc, char **argv) {
    if (argc != 5) {
        std::cerr << "Usage: task M Number-of-blocks Block-size N\n";
        return 1;
    }
    int number_of_producer_threads = std::stoi(argv[1]);
    int block_count = std::stoi(argv[2]);
    int block_size = std::stoi(argv[3]);
    int number_of_consumer_threads = std::stoi(argv[4]);
    std::thread A(producers, number_of_producer_threads, block_count, block_size);
    std::thread B(consumers, number_of_consumer_threads, block_count, block_size);
    A.join();
    B.join();
}

