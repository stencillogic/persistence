#include "auth/auth_sha3.h"
#include <string.h>
#include <stdio.h>

// straight-forward sha3 implementation

uint16 auth_sha3_bits[8] = {0x01u, 0x02u, 0x04u, 0x08u, 0x10u, 0x20u, 0x40u, 0x80u};

uint8 auth_sha3_get_bit(uint8 *b, uint8 x, uint8 y, uint8 z)
{
    uint16 byte = (5*y + x)*64 + z;
    return ((b[byte / 8] & auth_sha3_bits[byte % 8]) > 0 ? 1u : 0u);
}


uint8 auth_sha3_rc[32] = {0x01, 0x8d, 0x17, 0xfe, 0x09, 0xe5, 0xab, 0x0e,
                          0x46, 0xcd, 0xf4, 0x7b, 0x76, 0xa7, 0x52, 0xa4,
                          0xc5, 0x9c, 0xc7, 0x86, 0xe8, 0x7a, 0xfb, 0xb0,
                          0xac, 0xad, 0x20, 0x37, 0xc9, 0xc0, 0x25, 0x8e};

void auth_sha3_round(uint8 *b, uint32 ir)
{
    uint8 x,y,z,x1,j, _b[200];
    uint16 byte, t;

    // TODO: optimize

    // theta
    uint8 C[5][64], D[5][64];
    memset(_b, 0, 200);
    for(x = 0; x < 5; x++)
        for(z = 0; z < 64; z++)
        {
            C[x][z] = (auth_sha3_get_bit(b, x, 0, z) ^auth_sha3_get_bit(b, x, 1, z) ^auth_sha3_get_bit(b, x, 2, z) ^auth_sha3_get_bit(b, x, 3, z) ^auth_sha3_get_bit(b, x, 4, z));
        }
    for(x = 0; x < 5; x++)
        for(z = 0; z < 64; z++)
        {
            D[x][z] = C[x == 0 ? 4 : x-1][z] ^ C[x == 4? 0 : x+1][z == 0 ? 63 : z-1];
        }
    for(x = 0; x < 5; x++)
        for(y = 0; y < 5; y++)
            for(z = 0; z < 64; z++)
            {
                byte = (5*y + x)*64 + z;
                if((auth_sha3_get_bit(b, x, y, z) ^ D[x][z]) > 0)
                {
                    _b[byte / 8] |= auth_sha3_bits[byte % 8];
                }
                else
                {
                    _b[byte / 8] &= (~auth_sha3_bits[byte % 8]);
                }
            }


    memcpy(b, _b, 200);

    // rho
    memset(_b, 0, 200);

    for(z = 0; z < 64; z++)
        if(auth_sha3_get_bit(b, 0, 0, z) > 0)
        {
            _b[z / 8] |= auth_sha3_bits[z % 8];
        }
        else
        {
            _b[z / 8] &= (~auth_sha3_bits[z % 8]);
        }

    x = 1u;
    y = 0u;

    for(t = 0; t < 24; t++)
    {
        for(z = 0; z < 64; z++)
        {
            byte = (5*y + x)*64 + z;
            if(auth_sha3_get_bit(b, x, y, ((sint16)z + 320 - (sint16)((t + 1) * (t + 2)) / 2) % 64) > 0)
            {
                _b[byte / 8] |= auth_sha3_bits[byte % 8];
            }
            else
            {
                _b[byte / 8] &= (~auth_sha3_bits[byte % 8]);
            }
        }
        x1 = x;
        x = y;
        y = (2*x1 + 3*y) % 5;
    }

    memcpy(b, _b, 200);

    // pi
    for(x = 0; x < 5; x++)
        for(y = 0; y < 5; y++)
            for(z = 0; z < 64; z++)
            {
                byte = (5*y + x)*64 + z;
                if(auth_sha3_get_bit(b, (x + 3*y)%5, x, z) > 0)
                {
                    _b[byte / 8] |= auth_sha3_bits[byte % 8];
                }
                else
                {
                    _b[byte / 8] &= (~auth_sha3_bits[byte % 8]);
                }
            }

    memcpy(b, _b, 200);

    // chi
    for(x = 0; x < 5; x++)
        for(y = 0; y < 5; y++)
            for(z = 0; z < 64; z++)
            {
                byte = (5*y + x)*64 + z;
                if((auth_sha3_get_bit(b, x, y, z) ^ ((auth_sha3_get_bit(b, (x + 1)%5, y, z) ^ 1u) & auth_sha3_get_bit(b, (x+2)%5, y, z))) > 0)
                {
                    _b[byte / 8] |= auth_sha3_bits[byte % 8];
                }
                else
                {
                    _b[byte / 8] &= (~auth_sha3_bits[byte % 8]);
                }
            }

    memcpy(b, _b, 200);

    // iota
    uint8 RC[64];
    memset(RC, 0, 64);
    for(j = 0, t = 1; j <= 6; j++, t <<= 1)
    {
        byte = (j + 7*ir);
        RC[t - 1] = ((auth_sha3_rc[byte / 8] & auth_sha3_bits[byte % 8]) > 0u) ? 1u : 0u;
    }

    for(z = 0; z < 64; z++)
    {
        if((RC[z] ^ auth_sha3_get_bit(b, 0, 0, z)) > 0u)
        {
            b[z / 8] |= auth_sha3_bits[z % 8];
        }
        else
        {
            b[z / 8] &= (~auth_sha3_bits[z % 8]);
        }
    }
}


void auth_sha3_512(const uint8 *input, uint32 isz, uint8 *hash)
{
    uint8 b[200];
    uint32 i, j, k;

    memset(b, 0, 200);

    i = j = 0u;
    while(j < isz)
    {
        b[i] ^= input[j];
        j++;
        i++;
        if(i == 72)
        {
            for(k = 0; k < 24; k++)
            {
                auth_sha3_round(b, k);
            }
            i = 0;
        }
    }

    b[i] ^= 0x06;
    b[71] ^= 0x80;
    for(k = 0; k < 24; k++)
    {
        auth_sha3_round(b, k);
    }

    memcpy(hash, b, 64);
}
