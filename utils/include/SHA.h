//
// Created by Stepan Usatiuk on 15.04.2023.
//

#ifndef SEMBACKUP_SHA_H
#define SEMBACKUP_SHA_H

#include <array>
#include <memory>
#include <vector>

#include <openssl/evp.h>

/// Class to handle SHA hashing
/**
 * Based on: https://wiki.openssl.org/index.php/EVP_Message_Digests
 */
class SHA {
public:
    /// Constructs an empty SHA hasher instance
    /// \throws     Exception on initialization error
    SHA();

    /// Calculates the hash for a given \p in char vector
    /// \param in   Constant reference to an input vector
    /// \return     SHA hash of \p in
    static std::string calculate(const std::vector<char> &in);

    /// Calculates the hash for a given \p in string
    /// \param in   Constant reference to an input string
    /// \return     SHA hash of \p in
    static std::string calculate(const std::string &in);

    /// Append a vector of chars to the current hash
    /// \param in   Constant reference to an input vector
    /// \throws     Exception on any error
    void feedData(const std::vector<char> &in);

    /// Returns the hash, resets the hashing context
    /// \throws     Exception on any error
    std::string getHash();

private:
    const std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> mdctx{EVP_MD_CTX_new(),
                                                                        &EVP_MD_CTX_free};///< Current hashing context
};


#endif//SEMBACKUP_SHA_H
