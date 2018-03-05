//
//  rlp
//  breadwallet-core Ethereum
//
//  Created by Ed Gamble on 2/25/18.
//  Copyright (c) 2018 breadwallet LLC
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#ifndef BR_RLP_Coder_H
#define BR_RLP_Coder_H

#include <stddef.h>
#include <stdint.h>
#include "BRInt.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BRRlpDataRecord {
    // type
    size_t bytesAllocated;
    size_t bytesCount;
    uint8_t *bytes;
} BRRlpData;

extern BRRlpData
createRlpDataEmpty (void);

extern BRRlpData
createRlpDataCopy (BRRlpData data);

/**
 * Return a '0x' prefixed hex string of the data bytes.
 *
 * @param data
 * @return
 */
extern const char *
rlpDataAsString (BRRlpData data);

//
//
//
typedef struct BRRlpCoderRecord *BRRlpCoder;

extern BRRlpCoder
createRlpCoder (void);

extern void
rlpCoderRelease (BRRlpCoder coder);

extern void
rlpEncodeItemUInt64(BRRlpCoder coder, uint64_t value);

extern void
rlpEncodeItemUInt256(BRRlpCoder coder, UInt256 value);

extern void
rlpEncodeItemString(BRRlpCoder coder, const char *string);

extern void
rlpEncodeItemBytes(BRRlpCoder coder, uint8_t *bytes, size_t bytesCount);

//extern void
//rlpEncodeList (BRRlpCoder coder, BRRlpData *data, size_t dataCount);

extern BRRlpData
rlpGetData (BRRlpCoder coder);

#ifdef __cplusplus
}
#endif

#endif //BR_RLP_Coder_H