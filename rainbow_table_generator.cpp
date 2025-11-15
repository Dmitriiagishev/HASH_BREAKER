#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <iomanip>
#include <sstream>
#include <unordered_set>
#include <thread>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <openssl/evp.h>
#include <cstdlib>
#include <ctime>
#include <chrono>

// --- Функция хэширования (та же) ---
std::string hash_function(const std::string& value) {
    std::string passwd = "someSalt2" + value + "Additional890";

    const EVP_MD* sha256_md = EVP_sha256();
    EVP_MD_CTX* sha256_ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(sha256_ctx, sha256_md, nullptr);
    EVP_DigestUpdate(sha256_ctx, passwd.c_str(), passwd.length());

    unsigned char sha256_digest[EVP_MAX_MD_SIZE];
    unsigned int sha256_digest_len;
    EVP_DigestFinal_ex(sha256_ctx, sha256_digest, &sha256_digest_len);
    EVP_MD_CTX_free(sha256_ctx);

    const EVP_MD* sha1_md = EVP_sha1();
    EVP_MD_CTX* sha1_ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(sha1_ctx, sha1_md, nullptr);
    EVP_DigestUpdate(sha1_ctx, sha256_digest, sha256_digest_len);

    unsigned char sha1_digest[EVP_MAX_MD_SIZE];
    unsigned int sha1_digest_len;
    EVP_DigestFinal_ex(sha1_ctx, sha1_digest, &sha1_digest_len);
    EVP_MD_CTX_free(sha1_ctx);

    const EVP_MD* md5_md = EVP_md5();
    EVP_MD_CTX* md5_ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(md5_ctx, md5_md, nullptr);
    EVP_DigestUpdate(md5_ctx, sha1_digest, sha1_digest_len);

    unsigned char md5_digest[EVP_MAX_MD_SIZE];
    unsigned int md5_digest_len;
    EVP_DigestFinal_ex(md5_ctx, md5_digest, &md5_digest_len);
    EVP_MD_CTX_free(md5_ctx);

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < md5_digest_len; ++i) {
        oss << std::setw(2) << static_cast<unsigned int>(md5_digest[i]);
    }

    return oss.str();
}

inline int hex_to_int(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0; // На случай ошибки, хотя в корректном hex такого не будет
}

std::string reduction(const std::string& prev_hash_hex, int chain_index) {
    // 1. Преобразовать hex-строку хэша в байты (16 байт для MD5)
    // Используем более быстрый способ, чем sscanf
    unsigned char hash_bytes[16];
    for (int i = 0; i < 16; ++i) {
        // Читаем пару шестнадцатеричных символов как байт
        // Это стандартный способ конвертации hex в байты
        char high_nibble = prev_hash_hex[i * 2];
        char low_nibble = prev_hash_hex[i * 2 + 1];
        hash_bytes[i] = (hex_to_int(high_nibble) << 4) | hex_to_int(low_nibble);
    }

    // 2. Настройки
    const char alphabet[] = "2468*WuiopZbvcs"; // 15 символов
    const int alphabet_len = 15; // Задаем константой для скорости
    const int head_len = 9;

    // 3. Генерация head
    std::string out;
    out.reserve(head_len); // Резервируем память

    for (int i = 0; i < head_len; ++i) {
        unsigned int val = hash_bytes[i % 16];
        val ^= static_cast<unsigned int>(chain_index) << (i % 8); // XOR с chain_index, сдвинутым
        val += i; // Добавляем позицию

        // Получаем индекс символа
        int char_index = val % alphabet_len;
        out += alphabet[char_index];
    }
    return out;
}

std::string random_head() {
    //===============================================
    const char alphabet[] = "2468*WuiopZbvcs"; // 15 символов
    //===============================================
    const int alphabet_len = 15;
    const int head_len = 9;

    std::string out;
    out.reserve(head_len);

    for (int i = 0; i < head_len; ++i) {
        int random_index = rand() % alphabet_len;
        out += alphabet[random_index];
    }
    return out;
}

std::string calculate_chain(std::string head, int c_l){
    int chain_length = 1;
    std::string new_step = reduction(hash_function(head), chain_length);
    chain_length ++;
    while (chain_length < c_l){
        new_step = reduction(hash_function(new_step), chain_length);
        chain_length++;
    }
    return hash_function(new_step);
}

// --- Глобальные переменные для синхронизации ---
std::unordered_map<std::string, std::string> g_chains;
std::mutex g_chains_mutex;
std::atomic<int> g_sticked(0);
std::atomic<int> g_found_count(0);
std::atomic<long long> g_total_calculations(0); 

// --- Функция для одного потока ---
void workerThread(int thread_id, long long total_variants, int target_chains_per_thread, int c_l) {
    int local_found = 0;
    int local_sticked = 0;
    long long local_calculations = 0; // Локальный счётчик для потока

    auto last_report_time = std::chrono::high_resolution_clock::now();
    const long long report_interval = 20000; 

    while (local_found < target_chains_per_thread) {
        int random_num = rand();
        std::string head = random_head();
        std::string tail = calculate_chain(head, c_l);
        local_calculations++;

        {
            std::lock_guard<std::mutex> lock(g_chains_mutex);
            if (g_chains.count(tail) == 0) {
                g_chains[tail] = head;
                local_found++;
                g_found_count.fetch_add(1);
            } else {
                local_sticked++;
                g_sticked.fetch_add(1);
            }
        }

        if (local_calculations % report_interval == 0) {
            auto current_time = std::chrono::high_resolution_clock::now();
            auto elapsed_since_report = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - last_report_time);

            g_total_calculations.fetch_add(report_interval);

            {
                 std::lock_guard<std::mutex> output_lock(g_chains_mutex); 
                 std::cout << "[Поток " << thread_id << "] Выполнено: " << local_calculations << " вычислений. "
                           << "Найдено: " << local_found << "/" << target_chains_per_thread << ". "
                           << "Общие: Найдено: " << g_found_count.load() << ", Коллизии: " << g_sticked.load() << ", Вычисления: " << g_total_calculations.load() << ". "
                           << "Время с последнего отчета: " << elapsed_since_report.count() / 1000.0 << " сек." << std::endl;
            }
            last_report_time = current_time;
        }
    }

    std::cout << "[Поток " << thread_id << "] Завершён. Найдено: " << local_found << ", Коллизий: " << local_sticked << std::endl;
}

int main() {
    int alphabet_len = 15;
    int passwd_len = 9;
    long long int total_variants = 1;
    for (int i = 0; i < passwd_len; i++){
        total_variants *= alphabet_len;
    }
    //===============================================
    std::string output_filename = "rainbow_table.txt";
    int target_chains = 38450000; 
    int target_length = 1000;
    //===============================================
    const int num_threads = 8;
    int target_chains_per_thread = target_chains / num_threads;
    int remaining_chains = target_chains % num_threads;

    std::cout << "Генерация " << target_chains << " цепочек с использованием " << num_threads << " потоков." << std::endl;

    srand(static_cast<unsigned int>(std::time(nullptr)));

    auto start_time = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> threads;
    for (int t = 0; t < num_threads; ++t) {
        int current_target = target_chains_per_thread;
        if (t < remaining_chains) {
            current_target++;
        }
        threads.emplace_back(workerThread, t, total_variants, current_target, target_length);
    }

    for (auto& t : threads) {
        t.join();
    }

    auto end_time = std::chrono::high_resolution_clock::now();


    auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    std::cout << "\n--- Сборка результата ---" << std::endl;
    std::cout << "Всего найдено уникальных цепочек: " << g_found_count.load() << std::endl;
    std::cout << "Всего коллизий: " << g_sticked.load() << std::endl;
    std::cout << "Всего выполнено вычислений (head -> tail): " << g_total_calculations.load() << std::endl;
    std::cout << "Общее время выполнения: " << total_duration.count() / 1000.0 << " секунд." << std::endl;

    std::ofstream out_file(output_filename);
    if (!out_file.is_open()) {
        std::cerr << "Ошибка: невозможно открыть файл " << output_filename << " для записи." << std::endl;
        return 1;
    }

    for (const auto& pair : g_chains) {
        out_file << pair.second << " " << pair.first << "\n";
    }
    out_file.close();

    std::cout << "Результаты записаны в " << output_filename << std::endl;

    return 0;
}