//
// Created by Stepan Usatiuk on 15.04.2023.
//

#include "SHA.h"

#include "Exception.h"

std::string SHA::calculate(const std::vector<char> &in) {
    SHA hasher;
    hasher.feedData(in);
    return hasher.getHash();
}

SHA::SHA() {
    if (!mdctx) throw Exception("Can't create hashing context!");

    if (!EVP_DigestInit_ex(mdctx.get(), EVP_sha256(), nullptr)) throw Exception("Can't create hashing context!");
}

void SHA::feedData(const std::vector<char> &in) {
    if (in.empty()) return;
    if (!EVP_DigestUpdate(mdctx.get(), in.data(), in.size())) throw Exception("Error hashing!");
}

std::string SHA::getHash() {
    std::array<char, 32> out;
    unsigned int s = 0;

    if (!EVP_DigestFinal_ex(mdctx.get(), reinterpret_cast<unsigned char *>(out.data()), &s))
        throw Exception("Error hashing!");

    if (s != out.size()) throw Exception("Error hashing!");

    if (!EVP_MD_CTX_reset(mdctx.get())) throw Exception("Error hashing!");

    return {out.begin(), out.end()};
}

std::string SHA::calculate(const std::string &in) {
    std::vector<char> tmp(in.begin(), in.end());
    return SHA::calculate(tmp);
}
