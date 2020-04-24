#include <string.h>
#include "crypto_sign.h"
#include "crypto_verify_32.h"
#include "crypto_hash_sha512.h"
#include "sign_ed25519_ref10.h"
#include "ge25519.h"
#include "Dlog.h"
#include "utils.h"


/* Packed coordinates of the base point */
const ge25519 ge25519_base = {{{0x1A, 0xD5, 0x25, 0x8F, 0x60, 0x2D, 0x56, 0xC9, 0xB2, 0xA7, 0x25, 0x95, 0x60, 0xC7, 0x2C, 0x69,
                                0x5C, 0xDC, 0xD6, 0xFD, 0x31, 0xE2, 0xA4, 0xC0, 0xFE, 0x53, 0x6E, 0xCD, 0xD3, 0x36, 0x69, 0x21}},
                              {{0x58, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
                                0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66}},
                              {{0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
                              {{0xA3, 0xDD, 0xB7, 0xA5, 0xB3, 0x8A, 0xDE, 0x6D, 0xF5, 0x52, 0x51, 0x77, 0x80, 0x9F, 0xF0, 0x20,
                                0x7D, 0xE3, 0xAB, 0x64, 0x8E, 0x4E, 0xEA, 0x66, 0x65, 0x76, 0x8B, 0xD7, 0x0F, 0x5F, 0x87, 0x67}}};

int crypto_sign_open(
    unsigned char *m,unsigned long long *mlen,
    const unsigned char *sm,unsigned long long smlen,
    const unsigned char *pk
    )
{
  unsigned char pkcopy[32];
  unsigned char rcopy[32];
  unsigned char mcopy[smlen];
  unsigned char hram[64];
  unsigned char rcheck[32];
  ge25519 get1, get2;
  sc25519 schram, scs;

  if (smlen < 64)
	  {
	  Dlog_printf("\nerror 1");
	  }
  else if (sm[63] & 224)
	  {
	  Dlog_printf("\nerror 2");
	  }
  else if (ge25519_unpackneg_vartime(&get1,pk))
	  {
	  Dlog_printf("\nerror 3");
	  }
  else
  {
	  Dlog_printf("\nNo error");
  }

  if (smlen < 64) goto badsig;
  if (sm[63] & 224) goto badsig;
  if (ge25519_unpackneg_vartime(&get1,pk)) goto badsig;

  memmove(pkcopy,pk,32);
  memmove(rcopy,sm,32);

  sc25519_from32bytes(&scs, sm+32);

  memmove(mcopy,sm,smlen);
  memmove(mcopy + 32,pkcopy,32);
  crypto_hash_sha512(hram,mcopy,smlen);

  sc25519_from64bytes(&schram, hram);

  ge25519_double_scalarmult_vartime(&get2, &get1, &schram, &ge25519_base, &scs);
  ge25519_pack(rcheck, &get2);

  if (crypto_verify_32(rcopy,rcheck) == 0) {
    memmove(m,mcopy + 64,smlen - 64);
    memset(mcopy + smlen - 64,0,64);
    *mlen = smlen - 64;
    return 0;
  }

badsig:
  *mlen = (unsigned long long) -1;
  memset(m,0,smlen);
  return -1;
}

int crypto_sign_verify_detached(const unsigned char *sig,
                                     const unsigned char *m,
                                     unsigned long long   mlen,
                                     const unsigned char *pk)
{
  crypto_hash_sha512_state hs;
  unsigned char            h[64];
  unsigned char            rcheck[32];
  ge25519_p3               A;
  ge25519_p2               R;

  if (sig[63] & 240 &&
      sc25519_is_canonical(sig + 32) == 0) {
      return -1;
  }
  if (ge25519_has_small_order(sig) != 0) {
      return -1;
  }
  if (ge25519_is_canonical(pk) == 0 ||
      ge25519_has_small_order(pk) != 0) {
      return -1;
  }

  if (ge25519_frombytes_negate_vartime(&A, pk) != 0) {
      return -1;
  }
  crypto_sign_ed25519_ref10_hinit(&hs, 0);
  crypto_hash_sha512_update(&hs, sig, 32);
  crypto_hash_sha512_update(&hs, pk, 32);
  crypto_hash_sha512_update(&hs, m, mlen);
  crypto_hash_sha512_final(&hs, h);
  sc25519_reduce(h);

  ge25519_double_scalarmult_vartime_new(&R, h, &A, sig + 32);
  ge25519_tobytes(rcheck, &R);

  return crypto_verify_32(rcheck, sig) | (-(rcheck == sig)) |
	 sodium_memcmp(sig, rcheck, 32);
}
