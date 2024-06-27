#pragma once
#include "Buffer.h"
#include <openssl/md5.h>
class Crypto
{
public:
    static Buffer MD5(const Buffer &text);
};
