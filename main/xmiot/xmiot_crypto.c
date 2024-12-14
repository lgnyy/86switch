#include <stdint.h>
#include <string.h>
#include "mbedtls/base64.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/md.h"

int xmiot_crypto_base64_encode(const uint8_t* input, size_t ilen, char* output, size_t max_out_len) {
	size_t olen = 0;
	return mbedtls_base64_encode((uint8_t*)output, max_out_len, &olen, input, ilen);
}

int xmiot_crypto_base64_decode(const char* input, uint8_t* output, size_t max_out_len, size_t* olen) {
	return mbedtls_base64_decode((uint8_t*)output, max_out_len, olen, (const uint8_t*)input, strlen(input));
}

int xmiot_crypto_rand(uint8_t* random, size_t random_len){
  mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context ctr_drbg;

  mbedtls_ctr_drbg_init(&ctr_drbg);
  mbedtls_entropy_init(&entropy);
	int ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
		(const unsigned char*)"xmiot", 5);

	if (ret == 0) {
		ret = mbedtls_ctr_drbg_random(&ctr_drbg, random, random_len);
	}
 
  mbedtls_ctr_drbg_free(&ctr_drbg);
  mbedtls_entropy_free(&entropy);
	return ret;
}

int xmiot_crypto_md(const char* md_name, const uint8_t *input, size_t ilen, uint8_t *output){
	const mbedtls_md_info_t* md_info = mbedtls_md_info_from_string(md_name);
	return mbedtls_md(md_info, input, ilen, output);
}

int xmiot_crypto_md_base64(const char* md_name, const char* input1, const char* input2, char* output, size_t max_out_size){
	int ret;
	uint8_t tmp[256]; // TODO
	size_t olen;
	mbedtls_md_context_t ctx;
	const mbedtls_md_info_t* md_info = mbedtls_md_info_from_string(md_name);

	mbedtls_md_init(&ctx);
	if (!(ret = mbedtls_md_setup(&ctx, md_info, 0)) &&
		!(ret = mbedtls_md_starts(&ctx))) {

		ret = mbedtls_base64_decode(tmp, sizeof(tmp), &olen, (const unsigned char*)input1, strlen(input1));
		if (ret == 0) {
			mbedtls_md_update(&ctx, tmp, olen);

			if (input2 != NULL) {
				ret = mbedtls_base64_decode(tmp, sizeof(tmp), &olen, (const unsigned char*)input2, strlen(input2));
				if (ret == 0) {
					mbedtls_md_update(&ctx, tmp, olen);
				}
			}
		}

		if (ret == 0){
			ret = mbedtls_md_finish(&ctx, tmp);
			if (ret == 0) {
				ret = mbedtls_base64_encode((unsigned char*)output, max_out_size, &olen, tmp, mbedtls_md_get_size(md_info));
			}
		}
	}
	mbedtls_md_free(&ctx);
	return 0;
}

int xmiot_crypto_md_hmac(const char* md_name, const uint8_t *key, size_t keylen, const uint8_t *input, size_t ilen, uint8_t *output){
	const mbedtls_md_info_t* md_info = mbedtls_md_info_from_string(md_name);
	return mbedtls_md_hmac(md_info, key, keylen, input, ilen, output);
}
