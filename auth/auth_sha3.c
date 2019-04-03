#include "auth/auth_sha3.h"
#include <string.h>


// straight-forward sha3 implementation

uint16 auth_sha3_bits[8] = {0x01u, 0x02u, 0x04u, 0x08u, 0x10u, 0x20u, 0x40u, 0x80u};

uint8 auth_sha3_get_bit(uint8 *b, uint8 x, uint8 y, uint8 z)
{
    uint16 byte = (5*y + x)*64 + z;
    return (b[byte / 8] & auth_sha3_bits[byte % 8]) > 0 ? 1u : 0u;
}

/*
uint8 auth_sha3_rc_bit(uint8 t)
{
    if(0 == t % 255) return 1u;
    uint8 R[] = {1, 0, 0, 0, 0, 0, 0, 0, 0};
    sint16 i, j;
    for(i = 0; i < t % 255; i++)
    {
        for(j = 7; j >= 0; j--)
            R[j+1] = R[j];

        R[0] = R[8];
        R[4] ^= R[8];
        R[5] ^= R[8];
        R[6] ^= R[8];
    }
    return R[0];
}
*/

uint8 auth_sha3_rc[32] = {0x01, 0x8d, 0x17, 0xfe, 0x09, 0xe5, 0xab, 0x0e,
                          0x46, 0xcd, 0xf4, 0x7b, 0x76, 0xa7, 0x52, 0xa4,
                          0xc5, 0x9c, 0xc7, 0x86, 0xe8, 0x7a, 0xfb, 0xb0,
                          0xac, 0xad, 0x20, 0x37, 0xc9, 0xc0, 0x25, 0x8e};

void auth_sha3_round(uint8 *b, uint32 ir)
{
    uint8 x,y,z,t,x1,i,j,c[2], _b[200];
    uint16 byte;

    // TODO: optimize

    // theta
    memcpy(_b, b, 200);
    for(x = 0; x < 5; x++)
        for(y = 0; y < 5; y++)
            for(z = 0; z < 64; z++)
            {
                byte = (5*y + x) + z;

                c[0] = c[1] = 0u;
                for(i = 0; i < 5; i++)
                {
                    c[0] ^= auth_sha3_get_bit(b, (x-1)%5, i, z);
                    c[1] ^= auth_sha3_get_bit(b, (x+1)%5, i, (z-1)%64);
                }

                if((c[0] ^ c[1]) > 0)
                {
                    _b[byte / 8] ^= auth_sha3_bits[byte % 8];
                }
            }
    memcpy(b, _b, 200);

    // rho
    memset(_b, 0, 200);

    for(z = 0; z < 64; z++)
        _b[z / 8] |= auth_sha3_get_bit(b, 0, 0, z);

    x = 1u;
    y = 0u;

    for(t = 0; t < 25; t++)
    {
        for(z = 0; z < 64; z++)
        {
            byte = (5*y + x) + z;
            if(auth_sha3_get_bit(b, x, y, (z - (t + 1) * (t + 2) / 2) % 64) > 0)
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
                byte = (5*y + x) + z;
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
                byte = (5*y + x) + z;
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
        byte = (j + 7*ir) % 255;
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

void auth_sha3_512(uint8 *input, uint32 isz, uint8 *hash)
{
    uint8 b[200];
    uint32 i, j, k;

    memset(b, 0, 200);

    j = 0;
    while(j < isz)
    {
        i = 0;
        while(i < 128)
        {
            b[i] ^= input[j];
            j++;
            i++;
            if(j == isz) break;
        }

        for(k = 0; k < 24; k++)
        {
            auth_sha3_round(b, k);
        }
    }

    memcpy(hash, b+128, 64);
}
