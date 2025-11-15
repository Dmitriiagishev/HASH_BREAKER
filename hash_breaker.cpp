#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <atomic>
#include <future>
#include <sstream>
#include <iomanip>
#include <iomanip>
#include <chrono>
#include <openssl/evp.h>
#include <bits/stdc++.h>

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
    return 0;
}

std::string reduction(const std::string& prev_hash_hex, int chain_index) {
    unsigned char hash_bytes[16];
    for (int i = 0; i < 16; ++i) {
        char high_nibble = prev_hash_hex[i * 2];
        char low_nibble = prev_hash_hex[i * 2 + 1];
        hash_bytes[i] = (hex_to_int(high_nibble) << 4) | hex_to_int(low_nibble);
    }

    const char alphabet[] = "2468*WuiopZbvcs"; 
    const int alphabet_len = 15; 
    const int head_len = 9;

    std::string out;
    out.reserve(head_len); 
    for (int i = 0; i < head_len; ++i) {

        unsigned int val = hash_bytes[i % 16];
        val ^= static_cast<unsigned int>(chain_index) << (i % 8); 
        val += i; 
        int char_index = val % alphabet_len;
        out += alphabet[char_index];
    }
    return out;
}

std::string calculate_chain(std::string& head, int length){
    if (length <= 1){
        return hash_function(head);
    }
    int chain_length = 1;
    std::string new_step = reduction(hash_function(head), chain_length);
    chain_length ++;
    while (chain_length <= length){
        new_step = reduction(hash_function(new_step), chain_length);
        chain_length++;
    }
    return new_step;
}

std::string calculate_chain_part(std::string head, int start, int length){
    if (start <= 1){
        return hash_function(head);
    }
    int chain_length = start + 1;
    std::string new_step = head;
    while (chain_length <= length){
        new_step = reduction(hash_function(new_step), chain_length);
        chain_length++;
    }
    return new_step;
}

bool map_find(std::unordered_map<std::string, std::string>& map, std::string key){
    if (map.find(key) == map.end())
        return false;

    return true;
}

void calculate_tails(std::unordered_map<std::string, std::string>& table, std::vector<std::string> tails, int chain_length){
    for (size_t i = 0; i < tails.size(); i ++){
        std::string new_passwd = calculate_chain(table[tails[i]], chain_length);
        if (hash_function(new_passwd) == tails[i]){
                std::cout << new_passwd << '\t' << tails[i] << '\t' << hash_function(new_passwd) << '\n' ;
            }   
    }
}

std::vector<std::string> shift_links(std::vector<std::string>& links, int shift, int chain_length){
    std::vector<std::string> new_links;
    for (size_t i = 0; i < links.size(); i ++){
        new_links.push_back(hash_function(calculate_chain_part(reduction(links[i], shift), shift, chain_length)));
    }
    return new_links;
}

std::vector<std::string> shift_links(std::vector<std::string>& links, int chain_length){
    std::vector<std::string> new_links;
    for (size_t i = 0; i < links.size(); i ++){
        new_links.push_back(hash_function(reduction(links[i], chain_length)));
    }
    return new_links;
}

std::vector<std::string> split_string(std::string in_str, char sep) {
    std::vector<std::string> out_vec;
    std::string word = "";

    for (size_t i = 0; i < in_str.length(); ++i) { 
        if (in_str[i] != sep) {
            word += in_str[i];
        } else {

            out_vec.push_back(word);
            word = "";
        }
    }

    out_vec.push_back(word);

    return out_vec;
}
std::vector<std::string> load_hashes(std::string filename){
    std::string line;
    std::ifstream in_file(filename); 
    std::vector<std::string> out_vec;
    
    if (in_file.is_open())
    {
        while (std::getline(in_file, line))
        {
            out_vec.push_back(line);
        }
    }
    in_file.close();  
    out_vec.erase(out_vec.begin());
    std::cout << "loaded " << out_vec.size() << " hashes\n";
    return out_vec;
}
std::unordered_map<std::string, std::string> load_table(std::string filename){
    std::string line;

    std::ifstream in_file(filename); 
    std::unordered_map<std::string, std::string> out_map;
    
    if (in_file.is_open())
    {
        while (std::getline(in_file, line))
        {
            out_map[split_string(line, ' ')[1]] = split_string(line, ' ')[0];
        }
    }
    in_file.close();    
    return out_map;
}
std::vector<std::string> check_links(std::unordered_map<std::string, std::string>& table, std::vector<std::string>& links, int i, int chain_length){
    std::vector<std::string> out;
    std::vector<std::string> shifted_links = shift_links(links, i, chain_length);
    
    for (size_t j = 0; j < shifted_links.size(); j++){
        if (map_find(table, shifted_links[j])){
            std::string new_passwd = calculate_chain(table[shifted_links[j]], i - 1);
            if (hash_function(new_passwd) == links[j]){
                if (hash_function(new_passwd) == links[j]){
                    std::cout << new_passwd << '\t' << links[j] << '\t' << hash_function(new_passwd) << '\n' ;
                }   

            }   
            else{
                out.push_back(links[j]);
            }         
        }
        else{
            out.push_back(links[j]);
        }
    }
    return out;

}
void find_hashes(std::unordered_map<std::string, std::string>& table, std::vector<std::string>& hashes, int chain_length){
    std::vector<std::string> tails;
    std::vector<std::string> links_1;
    std::vector<std::string> links;
    for (size_t i = 0; i < hashes.size(); i++){
        if (map_find(table, hashes[i])){
            tails.push_back(hashes[i]);
        } else{
            links_1.push_back(hashes[i]);
        }
    }
    std::cout << "\nPasswd\t\tinput_hash\t\t\t\tcalculated_hash\n";
    calculate_tails(table, tails, chain_length);

    std::vector<std::string> shifted_links_1 = shift_links(links_1, chain_length);
    for (size_t i = 0; i < shifted_links_1.size(); i++){
        if (map_find(table, shifted_links_1[i])){
            std::string new_passwd = calculate_chain(table[shifted_links_1[i]], chain_length - 1);
            std::cout << new_passwd << '\t' << links_1[i] << '\t' << hash_function(new_passwd) << '\n' ;
        } else{
            links.push_back(links_1[i]);
        }
    }
    for (size_t i = chain_length - 1; i >= 1; i--){
        links = check_links(table, links, i, chain_length);
    }
    for (auto i : table){
        std::string temp_hash = hash_function(i.second);
        if (std::count(links.begin(), links.end(), temp_hash) > 0 ){
            std::cout << i.second << '\t' << temp_hash << '\t' << hash_function(i.second) << '\n' ;
        }
    }
}



int main() {
    std::cout << "loading rainbow table...\n";
    std::unordered_map chains = load_table("rainbow_table.txt");
    std::cout << "done\nloading hashes...\n";
    std::vector hashes = load_hashes("hashes.txt");
    
    std::cout << "done\n\nWE\'RE READY TO ROCK\n";
    find_hashes(chains, hashes, 999); //длину цепочки нужно указывать на 1 меньше, чем есть на самом деле
    return 0;
}