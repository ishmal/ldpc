
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "ldpcEncoder.h"
#include "codes.h"
#include "util.h"


void add(uint8_t *out, uint8_t *a, uint8_t *b, int len) {
	while (len--) {
		*out++ = *a++ ^ *b++;
	}
}


/**
 * Create a new encoder context configured for the given code
 */
LdpcEncoder *ldpcEncoderCreate(Code *code) {

	LdpcEncoder *enc = (LdpcEncoder *) malloc(sizeof(LdpcEncoder));
	if (!enc) {
		return (LdpcEncoder *)0;
	}

	enc->msgLen = 0;
	enc->code = code;
	enc->Ast = (uint8_t *) malloc(code->Alen * sizeof(uint8_t));
	enc->Cst = (uint8_t *) malloc(code->Clen * sizeof(uint8_t));
	enc->TinvAst = (uint8_t *) malloc(code->Tlen * sizeof(uint8_t));
	enc->ETinvAst = (uint8_t *) malloc(code->Elen * sizeof(uint8_t));
	enc->p1 = (uint8_t *) malloc(code->Clen * sizeof(uint8_t));
	enc->Bp1 = (uint8_t *) malloc(code->Blen * sizeof(uint8_t));
	enc->AstBp1 = (uint8_t *) malloc(code->Alen * sizeof(uint8_t));
	enc->p2 = (uint8_t *) malloc(code->Tlen * sizeof(uint8_t));
	enc->x = (uint8_t *) malloc(code->N * sizeof(uint8_t));

	return enc;
}

void ldpcEncoderDestroy(LdpcEncoder *enc) {
	if (!enc) {
		return;
	}
	if (enc->Ast) {
		free(enc->Ast);
	}
	if (enc->Cst) {
		free(enc->Cst);
	}
	if (enc->TinvAst) {
		free(enc->TinvAst);
	}
	if (enc->ETinvAst) {
		free(enc->ETinvAst);
	}
	if (enc->p1) {
		free(enc->p1);
	}
	if (enc->Bp1) {
		free(enc->Bp1);
	}
	if (enc->AstBp1) {
		free(enc->AstBp1);
	}
	if (enc->p2) {
		free(enc->p2);
	}
	if (enc->x) {
		free(enc->x);
	}
	free(enc);
}


static uint8_t *doEncode(LdpcEncoder *enc) {
	Code *code = enc->code;

	// step 1
	multiplySparse(enc->Ast, code->A, code->Alen, enc->x);
	multiplySparse(enc->Cst, code->C, code->Clen, enc->x);
	// step 2
	substituteSparse(enc->TinvAst, code->T, code->Tlen, enc->Ast);
	multiplySparse(enc->ETinvAst, code->E, code->Elen, enc->TinvAst);
	// step 3
	add(enc->p1, enc->ETinvAst, enc->Cst, code->Elen);
	// step 4
	multiplySparse(enc->Bp1, code->B, code->Blen, enc->p1);
	add(enc->AstBp1, enc->Ast, enc->Bp1, code->Alen);
	substituteSparse(enc->p2, code->T, code->Tlen, enc->AstBp1);
	// step 5
	uint8_t *dest = enc->x + code->messageBits;
	int len = code->Alen;
	uint8_t *src = enc->p1;
	while (len--) {
		*dest++ = *src++;
	}
	len = code->Alen;
	src = enc->p2;
	while (len--) {
		*dest++ = *src++;
	}
	return enc->x;
}

/**
 * @param {array} s array of bits, size code->messageBits
 * @return {array} array of encoded bits, size N
 */
uint8_t *ldpcEncode(LdpcEncoder *enc, uint8_t *s, int len) {
	if (len > enc->code->messageBits) {
		printf("message size too large: %d > %d", len, 
			enc->code->messageBits);
		return (uint8_t *)0;
	}
	uint8_t *dest = enc->x;
	while (len--) {
		*dest++ = *s++;
	}
	return doEncode(enc);
}

/**
 * Encode a message array of bytes
 * @param {array} bytes an array of bytes to encode 
 */
uint8_t *ldpcEncodeBytes(LdpcEncoder *enc, uint8_t *bytes, int nrBytes) {
	int len = nrBytes * 8;
	if (len > enc->code->messageBits + 8) {
		printf("message size too large: %d > %d", len, 
			enc->code->messageBits);
		return (uint8_t *)0;
	}
	bytesToBitsBE(enc->x, bytes, nrBytes);
	return doEncode(enc);
}


