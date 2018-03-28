#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/insert_linebreaks.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/ostream_iterator.hpp>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <vector>
#include <algorithm>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <streambuf>
#include <regex>
#include <iterator>
#include <cstdlib>
#include <nan.h>

#include "urilite.h"
//using namespace boost::archive::iterators;
using namespace urilite;

namespace std {

    template<class BidirIt, class Traits, class CharT, class UnaryFunction>
    std::basic_string<CharT> regex_replace(BidirIt first, BidirIt last,
                                           const std::basic_regex<CharT,Traits>& re, UnaryFunction f)
    {
        std::basic_string<CharT> s;

        typename std::match_results<BidirIt>::difference_type
        positionOfLastMatch = 0;
        auto endOfLastMatch = first;

        auto callback = [&](const std::match_results<BidirIt>& match)
        {
            auto positionOfThisMatch = match.position(0);
            auto diff = positionOfThisMatch - positionOfLastMatch;

            auto startOfThisMatch = endOfLastMatch;
            std::advance(startOfThisMatch, diff);

            s.append(endOfLastMatch, startOfThisMatch);
            s.append(f(match));

            auto lengthOfMatch = match.length(0);

            positionOfLastMatch = positionOfThisMatch + lengthOfMatch;

            endOfLastMatch = startOfThisMatch;
            std::advance(endOfLastMatch, lengthOfMatch);
        };

        std::sregex_iterator begin(first, last, re), end;
        std::for_each(begin, end, callback);

        s.append(endOfLastMatch, last);

        return s;
    }

    template<class Traits, class CharT, class UnaryFunction>
    std::string regex_replace(const std::string& s,
                              const std::basic_regex<CharT,Traits>& re, UnaryFunction f)
    {
        return regex_replace(s.cbegin(), s.cend(), re, f);
    }

}

using namespace std;
using namespace v8;


void aes_init()
{
    static int init=0;
    if (init==0)
    {
        EVP_CIPHER_CTX e_ctx, d_ctx;

        //initialize openssl ciphers
        OpenSSL_add_all_ciphers();

        //initialize random number generator (for IVs)
        int rv = RAND_load_file("/dev/urandom", 32);
    }
}

std::vector<unsigned char> aes_128_gcm_encrypt(std::string plaintext, std::string key)
{
    aes_init();

    size_t enc_length = plaintext.length()*3;
    std::vector<unsigned char> output;
    output.resize(enc_length,'\0');

    unsigned char tag[AES_BLOCK_SIZE];
    unsigned char iv[AES_BLOCK_SIZE];
    RAND_bytes(iv, sizeof(iv));
    std::copy( iv, iv+16, output.begin()+16);

    int actual_size=0, final_size=0;
    EVP_CIPHER_CTX* e_ctx = EVP_CIPHER_CTX_new();
    EVP_CIPHER_CTX_ctrl(e_ctx, EVP_CTRL_GCM_SET_IVLEN, 16, NULL);
    EVP_EncryptInit(e_ctx, EVP_aes_128_gcm(), (const unsigned char*)key.c_str(), iv);
    EVP_EncryptUpdate(e_ctx, &output[32], &actual_size, (const unsigned char*)plaintext.data(),plaintext.length() );
    EVP_EncryptFinal(e_ctx, &output[32+actual_size], &final_size);
    EVP_CIPHER_CTX_ctrl(e_ctx, EVP_CTRL_GCM_GET_TAG, 16, tag);
    std::copy( tag, tag+16, output.begin() );
    std::copy( iv, iv+16, output.begin()+16);
    output.resize(32 + actual_size+final_size);
    EVP_CIPHER_CTX_free(e_ctx);
    return output;
}

std::string aes_128_gcm_decrypt(std::vector<unsigned char> ciphertext, std::string key)
{
    aes_init();

    unsigned char tag[AES_BLOCK_SIZE];
    unsigned char iv[AES_BLOCK_SIZE];
    std::copy( ciphertext.begin(),    ciphertext.begin()+16, tag);
    std::copy( ciphertext.begin()+16, ciphertext.begin()+32, iv);
    std::vector<unsigned char> plaintext; plaintext.resize(ciphertext.size(), '\0');

    int actual_size=0, final_size=0;
    EVP_CIPHER_CTX *d_ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit(d_ctx, EVP_aes_128_gcm(), (const unsigned char*)key.c_str(), iv);
    EVP_DecryptUpdate(d_ctx, &plaintext[0], &actual_size, &ciphertext[32], ciphertext.size()-32 );
    EVP_CIPHER_CTX_ctrl(d_ctx, EVP_CTRL_GCM_SET_TAG, 16, tag);
    EVP_DecryptFinal(d_ctx, &plaintext[actual_size], &final_size);
    EVP_CIPHER_CTX_free(d_ctx);
    plaintext.resize(actual_size + final_size, '\0');

    return std::string(plaintext.begin(),plaintext.end());
}

std::string replace_callback(const std::smatch& m) {
    aes_init();
    //const uri = redirectionutils.getClickLinkURI(r_domain, client_id, p4, message_uuid)
    std::string r_domain = "Stackoverflow.com";
    std::string client_id = std::to_string(123);
    std::string message_uuid = "a86sf992";
    //std::string mstr = m.str(0).c_str();
    std::string p1 = m.str(1).c_str();
    //std::string p2 = m.str(2).c_str();
    //std::string p3 = m.str(3).c_str();
    std::string p4 = m.str(4).c_str();

    std::transform(r_domain.begin(), r_domain.end(), r_domain.begin(), ::tolower);
    std::string encodedUri = uri::encodeURIComponent(p4);
    std::string encodedDomainLower = uri::encodeURIComponent(r_domain);

    //std::cout << p1 << endl << p2 << endl << p3 << endl << p4 << endl;
    std::string plaintext = "u=" + message_uuid + "&c=" + client_id + "&r=" + encodedUri + "&d=" + encodedDomainLower;
    std::string encryptionKey = "18ade47e50a8a679618e20e317535df6";

    std::vector<unsigned char> ciphertext = aes_128_gcm_encrypt(plaintext, encryptionKey);
    std::string encrypted;
    static const char *chars="0123456789ABCDEF";
    for(int i=0; i<ciphertext.size(); i++)
    {
        std::stringstream n1, n2;
        n1 << std::hex << chars[ciphertext[i]/16];
        n2 << std::hex << chars[ciphertext[i]%16];
        encrypted += n1.str() + n2.str();
        //std::cout << chars[ciphertext[i]/16];
        //std::cout << chars[ciphertext[i]%16];
    }

    std::stringstream oscontent;
    typedef
        boost::archive::iterators::base64_from_binary<
        boost::archive::iterators::transform_width<
        const char *,
        6,
        8
        >
        >
        base64_text;

    std::copy(base64_text(encrypted.c_str()), base64_text(encrypted.c_str() + encrypted.size()), boost::archive::iterators::ostream_iterator<char>(oscontent));

    std::string content = oscontent.str();

    unsigned char tag[AES_BLOCK_SIZE];   //AES_BLOCK_SIZE=16
    unsigned char iv[AES_BLOCK_SIZE];
    RAND_bytes(iv, sizeof(iv));

    std::vector<unsigned char> concat;

    std::copy(iv, iv+sizeof(iv), std::back_inserter(concat));
    std::copy(base64_text(content.c_str()), base64_text(content.c_str() + content.size()), std::back_inserter(concat));
    std::copy(tag, tag+sizeof(tag), std::back_inserter(concat));

    std::stringstream data;
    data << concat.data();
    std::string concat_data(data.str());

    std::string plus = R"(\+)";
    std::string slash = R"(\/)";
    std::string equal = R"(=)";
    std::regex reg_plus(plus);
    std::regex reg_slash(slash);
    std::regex reg_equal(equal);
    std::string result, result1, result2;
    std::regex_replace (std::back_inserter(result), concat_data.begin(), concat_data.end(), reg_plus,  "_");
    std::regex_replace (std::back_inserter(result1), result.begin(), result.end(), reg_slash,  "_");
    std::regex_replace (std::back_inserter(result2), result1.begin(), result1.end(), reg_equal,  "");


    std::string uri = "http://" + r_domain + "/tr/cl/" + result2;
    std::string replaced =  p1 + " href=\"" + uri + "\"";
    //std::string uri = p4; //getClickLinkURI(r_domain, client_id, p4, message_uuid);

//
//    const domainLower = domain.toLowerCase()
//      const e = `u=${uuid}&c=${clientID}&r=${encodeURIComponent(uri)}&d=${encodeURIComponent(domainLower)}`
//
//      const aes = new cryptoutils.Aes128Gcm(encryptionKey)
//      const crypted = aes.encrypt(e)
//
//      // concating buffers of iv, content and auth_tag
//      let concat = Buffer.concat([aes.iv, Buffer.from(crypted, 'base64'), aes.tag])
//
//      // URL safe base64 encoding
//      concat = cryptoutils.UrlSafeBase64(concat)
//
//      const urlObject = {
//        protocol: 'http:',
//        host: domain,
//        pathname: pathnameClick + concat,
//      }
//      return url.format(urlObject)

    return replaced;
}

void RegexReplace(const Nan::FunctionCallbackInfo<v8::Value>& info){

    if (info.Length() < 3) {
        Nan::ThrowTypeError("Wrong number of arguments");
        return;
    }

//    if (!info[0]->IsString()){
//        Nan::ThrowTypeError("Regex agrument 1 must be a string");
//        return;
//    }

    if (!info[1]->IsString()){
        Nan::ThrowTypeError("Regex agrument 2 must be a string");
        return;
    }

    if (!info[2]->IsString()){
        Nan::ThrowTypeError("Regex agrument 3 must be a string");
        return;
    }
    //Nan::Local<Object> bufferObj = info[0]->ToObject();
    char* buf = node::Buffer::Data(info[0]);
    size_t bufLength = node::Buffer::Length(info[0]);
    std::string s(buf, bufLength);

    //Nan::Utf8String data(info[0]);
    //std::string s = std::string(*data);

    Nan::Utf8String regexp(info[1]);
    std::regex e = std::regex(*regexp);

     Nan::Utf8String replace(info[2]);
     std::string r = std::string(*replace);

    //std::string result;
    //std::regex_replace (std::back_inserter(result), s.begin(), s.end(), e, r);//"$1 href=\"$4\""

    std::string result = regex_replace(s, e, replace_callback);

    info.GetReturnValue().Set(Nan::New<v8::String>(result).ToLocalChecked());
}

void Init(v8::Local<v8::Object> exports) {
  exports->Set(Nan::New("RegexReplace").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(RegexReplace)->GetFunction());
}

NODE_MODULE(RegexReplace, Init)
