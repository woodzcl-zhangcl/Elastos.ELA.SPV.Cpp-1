//  Created by Ed Gamble on 1/23/2018
//  Copyright (c) 2018 breadwallet LLC.
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

#include <stdlib.h>
#include <malloc.h>
#include <assert.h>
#include <BRBIP39Mnemonic.h>
#include <android/log.h>
#include "BRWallet.h"
#include "BRAddress.h"
#include "BRCoreJni.h"
#include "BRTransaction.h"
#include "BRPeerManager.h"
#include "com_elastos_spvcore_CoreWallet.h"
#include "com_elastos_spvcore_CoreTransaction.h"

static BRTransaction *
JNI_COPY_TRANSACTION (BRTransaction *tx) {
	if (com_elastos_spvcore_CoreTransaction_JNI_COPIES_TRANSACTIONS && NULL != tx) {
		return BRTransactionCopy(tx);
	}
	else {
#if defined (__ANDROID_NDK__)
		__android_log_print(ANDROID_LOG_DEBUG, "JNI", "FAILED TO COPY: %p", tx);
#endif
		return NULL;
	}
}

/* Forward Declarations */
static void balanceChanged(void *info, uint64_t balance);
static void txAdded(void *info, BRTransaction *tx);
static void txUpdated(void *info, const UInt256 txHashes[], size_t count,
		uint32_t blockHeight,
		uint32_t timestamp);
static void txDeleted(void *info, UInt256 txHash, int notifyUser, int recommendRescan);

//
// Statically Initialize Java References
//
static jclass addressClass;
static jmethodID addressConstructor;

static jclass transactionClass;
static jmethodID transactionConstructor;


/*
 * Class:     com_elastos_spvcore_CoreWallet
 * Method:    createJniCoreWallet
 * Signature: ([Lcom/breadwallet/core/BRCoreTransaction;Lcom/breadwallet/core/BRCoreMasterPubKey;Lcom/breadwallet/core/BRCoreWallet/Listener;)J
 */
JNIEXPORT jlong JNICALL
Java_com_elastos_spvcore_CoreWallet_createJniCoreWallet
(JNIEnv *env, jclass thisClass,
 jobjectArray objTransactionsArray,
 jobject objMasterPubKey) {

	BRMasterPubKey *masterPubKey = (BRMasterPubKey *) getJNIReference(env, objMasterPubKey);

	// Transactions
	size_t transactionsCount = (*env)->GetArrayLength(env, objTransactionsArray);
	BRTransaction **transactions = (BRTransaction **) calloc (transactionsCount, sizeof (BRTransaction *));

	for (int index = 0; index < transactionsCount; index++) {
		jobject objTransaction = (*env)->GetObjectArrayElement (env, objTransactionsArray, index);
		// TODO: Transaction Copy?  Confirm isRegistered.
		transactions[index] = (BRTransaction *) getJNIReference(env, objTransaction);
		(*env)->DeleteLocalRef (env, objTransaction);
	}

	BRWallet *wallet = BRWalletNew(transactions, transactionsCount, *masterPubKey);

	if (NULL != transactions) free (transactions);

	return (jlong) wallet;
}

/*
 * Class:     com_elastos_spvcore_CoreWallet
 * Method:    installListener
 * Signature: (Lcom/breadwallet/core/BRCoreWallet/Listener;)V
 */
JNIEXPORT void JNICALL Java_com_elastos_spvcore_CoreWallet_installListener
(JNIEnv *env, jobject thisObject, jobject listenerObject) {
	BRWallet *wallet = (BRWallet *) getJNIReference (env, thisObject);

	// Get a WeakGlobalRef - 'weak' to allow for GC; 'global' to allow BRCore thread access
	// TODO: If this is made a WeakGlobal then the App crashes.
	jobject listener = (*env)->NewGlobalRef (env, listenerObject);

	// Assign callbacks
	BRWalletSetCallbacks(wallet, listener,
			balanceChanged,
			txAdded,
			txUpdated,
			txDeleted);
}


/*
 * Class:     com_elastos_spvcore_CoreWallet
 * Method:    getReceiveAddress
 * Signature: ()Lcom/breadwallet/core/BRCoreAddress;
 */
JNIEXPORT jobject JNICALL
Java_com_elastos_spvcore_CoreWallet_getReceiveAddress
(JNIEnv *env, jobject thisObject) {
	BRWallet *wallet = (BRWallet *) getJNIReference (env, thisObject);

	BRAddress *address = (BRAddress *) malloc (sizeof (BRAddress));
	*address = BRWalletReceiveAddress (wallet);

	return (*env)->NewObject (env, addressClass, addressConstructor, (jlong) address);
}

/*
 * Class:     com_elastos_spvcore_CoreWallet
 * Method:    getAllAddresses
 * Signature: ()[Lcom/breadwallet/core/BRCoreAddress;
 */
JNIEXPORT jobjectArray JNICALL
Java_com_elastos_spvcore_CoreWallet_getAllAddresses
(JNIEnv *env, jobject thisObject) {
	BRWallet *wallet = (BRWallet *) getJNIReference (env, thisObject);

	// Get *all* addresses
	size_t addrCount = wallet->WalletAllAddrs (wallet, NULL, 0);

	BRAddress *addresses = (BRAddress *) calloc (addrCount, sizeof (BRAddress));
	wallet->WalletAllAddrs (wallet, addresses, addrCount);

	jobjectArray addrArray = (*env)->NewObjectArray (env, addrCount, addressClass, 0);

	for (int i = 0; i < addrCount; i++) {
		// Get the JNI Reference Address
		BRAddress *address = (BRAddress *) malloc (sizeof (BRAddress));
		*address = addresses[i];

		jobject addrObject = (*env)->NewObject (env, addressClass, addressConstructor, (jlong) address);

		(*env)->SetObjectArrayElement (env, addrArray, i, addrObject);
		(*env)->DeleteLocalRef (env, addrObject);
	}

	if (NULL != addresses) free (addresses);

	return addrArray;
}


/*
 * Class:     com_elastos_spvcore_CoreWallet
 * Method:    containsAddress
 * Signature: (Lcom/breadwallet/core/BRCoreAddress;)Z
 */
JNIEXPORT jboolean JNICALL
Java_com_elastos_spvcore_CoreWallet_containsAddress
(JNIEnv *env, jobject thisObject, jobject objAddress) {
	BRWallet  *wallet  = (BRWallet  *) getJNIReference (env, thisObject);
	BRAddress *address = (BRAddress *) getJNIReference (env, objAddress);

	return (jboolean) BRWalletContainsAddress(wallet, (const char *) address);
}

/*
 * Class:     com_elastos_spvcore_CoreWallet
 * Method:    addressIsUsed
 * Signature: (Lcom/breadwallet/core/BRCoreAddress;)Z
 */
JNIEXPORT jboolean JNICALL
Java_com_elastos_spvcore_CoreWallet_addressIsUsed
(JNIEnv *env, jobject thisObject, jobject objAddress) {
	BRWallet  *wallet  = (BRWallet  *) getJNIReference (env, thisObject);
	BRAddress *address = (BRAddress *) getJNIReference (env, objAddress);

	return (jboolean) BRWalletAddressIsUsed(wallet, (const char *) address);
}

/*
 * Class:     com_elastos_spvcore_CoreWallet
 * Method:    jniGetTransactions
 * Signature: ()[Lcom/breadwallet/core/BRCoreTransaction;
 */
JNIEXPORT jobjectArray JNICALL
Java_com_elastos_spvcore_CoreWallet_jniGetTransactions
(JNIEnv *env, jobject thisObject) {
	BRWallet  *wallet  = (BRWallet  *) getJNIReference (env, thisObject);

	size_t transactionCount = BRWalletTransactions (wallet, NULL, 0);
	BRTransaction **transactions = (BRTransaction **) calloc (transactionCount, sizeof (BRTransaction *));
	transactionCount = BRWalletTransactions (wallet, transactions, transactionCount);

	jobjectArray transactionArray = (*env)->NewObjectArray (env, transactionCount, transactionClass, 0);

	// TODO: Decide if copy is okay; if not, be sure to mark 'isRegistered = true'
	//   We should not copy; but we need to deal with wallet-initiated 'free'
	for (int index = 0; index < transactionCount; index++) {
		jobject transactionObject =
			(*env)->NewObject (env, transactionClass, transactionConstructor,
					(jlong) JNI_COPY_TRANSACTION(transactions[index]));
		assert (!(*env)->IsSameObject (env, transactionObject, NULL));

		(*env)->SetObjectArrayElement (env, transactionArray, index, transactionObject);
		(*env)->DeleteLocalRef (env, transactionObject);
	}

	if (NULL != transactions) free (transactions);

	return transactionArray;
}

/*
 * Class:     com_elastos_spvcore_CoreWallet
 * Method:    getTransactionsConfirmedBefore
 * Signature: (J)[Lcom/breadwallet/core/BRCoreTransaction;
 */
JNIEXPORT jobjectArray JNICALL
Java_com_elastos_spvcore_CoreWallet_getTransactionsConfirmedBefore
(JNIEnv *env, jobject thisObject, jlong blockHeight) {
	BRWallet  *wallet  = (BRWallet  *) getJNIReference (env, thisObject);

	size_t transactionCount = BRWalletTxUnconfirmedBefore (wallet, NULL, 0, blockHeight);
	BRTransaction **transactions = (BRTransaction **) calloc (transactionCount, sizeof (BRTransaction *));
	BRWalletTxUnconfirmedBefore (wallet, transactions, transactionCount, blockHeight);

	jobjectArray transactionArray = (*env)->NewObjectArray (env, transactionCount, transactionClass, 0);

	for (int index = 0; index < transactionCount; index++) {
		jobject transactionObject =
			(*env)->NewObject (env, transactionClass, transactionConstructor,
					(jlong) JNI_COPY_TRANSACTION(transactions[index]));

		(*env)->SetObjectArrayElement (env, transactionArray, index, transactionObject);
		(*env)->DeleteLocalRef (env, transactionObject);
	}

	if (NULL != transactions) free (transactions);

	return transactionArray;
}

/*
 * Class:     com_elastos_spvcore_CoreWallet
 * Method:    getBalance
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL
Java_com_elastos_spvcore_CoreWallet_getBalance
(JNIEnv *env, jobject thisObject) {
	BRWallet  *wallet  = (BRWallet  *) getJNIReference (env, thisObject);
	return (jlong) BRWalletBalance (wallet);
}

/*
 * Class:     com_elastos_spvcore_CoreWallet
 * Method:    getTotalSend
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL
Java_com_elastos_spvcore_CoreWallet_getTotalSent
(JNIEnv *env, jobject thisObject) {
	BRWallet  *wallet  = (BRWallet  *) getJNIReference (env, thisObject);
	return (jlong) BRWalletTotalSent (wallet);
}

/*
 * Class:     com_elastos_spvcore_CoreWallet
 * Method:    getTotalReceived
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL
Java_com_elastos_spvcore_CoreWallet_getTotalReceived
(JNIEnv *env, jobject thisObject) {
	BRWallet  *wallet  = (BRWallet  *) getJNIReference (env, thisObject);
	return (jlong) BRWalletTotalReceived (wallet);
}

/*
 * Class:     com_elastos_spvcore_CoreWallet
 * Method:    getFeePerKb
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL
Java_com_elastos_spvcore_CoreWallet_getFeePerKb
(JNIEnv *env, jobject thisObject) {
	BRWallet  *wallet  = (BRWallet  *) getJNIReference (env, thisObject);
	return (jlong) BRWalletFeePerKb (wallet);
}

/*
 * Class:     com_elastos_spvcore_CoreWallet
 * Method:    setFeePerKb
 * Signature: (J)V
 */
JNIEXPORT void JNICALL
Java_com_elastos_spvcore_CoreWallet_setFeePerKb
(JNIEnv *env, jobject thisObject, jlong feePerKb) {
	BRWallet  *wallet  = (BRWallet  *) getJNIReference (env, thisObject);
	BRWalletSetFeePerKb (wallet, feePerKb);
}

/*
 * Class:     com_elastos_spvcore_CoreWallet
 * Method:    getMaxFeePerKb
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_com_elastos_spvcore_CoreWallet_getMaxFeePerKb
(JNIEnv *env, jobject thisObject) {
	return MAX_FEE_PER_KB;
}

/*
 * Class:     com_elastos_spvcore_CoreWallet
 * Method:    getDefaultFeePerKb
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_com_elastos_spvcore_CoreWallet_getDefaultFeePerKb
(JNIEnv *env, jobject thisObject) {
	return DEFAULT_FEE_PER_KB;
}

/*
 * Class:     com_elastos_spvcore_CoreWallet
 * Method:    createTransaction
 * Signature: (JLcom/breadwallet/core/BRCoreAddress;)Lcom/breadwallet/core/BRCoreTransaction;
 */
JNIEXPORT jobject JNICALL
Java_com_elastos_spvcore_CoreWallet_createTransaction
(JNIEnv *env, jobject thisObject, jlong amount, jobject addressObject) {
	BRWallet  *wallet  = (BRWallet  *) getJNIReference(env, thisObject);
	BRAddress *address = (BRAddress *) getJNIReference(env, addressObject);

	// transaction may be NULL - like if the wallet does not have a large enough balance
	// to cover the transaction amount
	BRTransaction *transaction = BRWalletCreateTransaction(wallet,
			(uint64_t) amount,
			(const char *) address->s);

	return NULL == transaction
		? NULL
		: (*env)->NewObject(env, transactionClass, transactionConstructor, (jlong) transaction);
}

/*
 * Class:     com_elastos_spvcore_CoreWallet
 * Method:    createTransactionForOutputs
 * Signature: ([Lcom/breadwallet/core/BRCoreTransactionOutput;)Lcom/breadwallet/core/BRCoreTransaction;
 */
JNIEXPORT jobject JNICALL Java_com_elastos_spvcore_CoreWallet_createTransactionForOutputs
(JNIEnv *env, jobject thisObject, jobjectArray outputsArray) {
	BRWallet *wallet = (BRWallet *) getJNIReference(env, thisObject);

	size_t outputsCount = (size_t) (*env)->GetArrayLength (env, outputsArray);
	BRTxOutput *outputs = (BRTxOutput *) calloc (outputsCount, sizeof (BRTxOutput));

	for (int i = 0; i < outputsCount; i++) {
		jobject outputObject = (*env)->GetObjectArrayElement (env, outputsArray, i);
		outputs[i] = *(BRTxOutput *) getJNIReference (env, outputObject);
		(*env)->DeleteLocalRef (env, outputObject);
	}

	// It appears that the outputs and their content are not 'held' by the Core, in any way.
	BRTransaction *transaction = BRWalletCreateTxForOutputs(wallet, outputs, outputsCount);

	if (NULL != outputs) free (outputs);

	return NULL == transaction
		? NULL
		: (*env)->NewObject(env, transactionClass, transactionConstructor, (jlong) transaction);
}


/*
 * Class:     com_elastos_spvcore_CoreWallet
 * Method:    signTransaction
 * Signature: (Lcom/breadwallet/core/BRCoreTransaction;I[B)Z
 */
JNIEXPORT jboolean JNICALL
Java_com_elastos_spvcore_CoreWallet_signTransaction
(JNIEnv *env, jobject thisObject,
 jobject transactionObject,
 jint forkId,
 jbyteArray phraseByteArray) {
	BRWallet *wallet = (BRWallet *) getJNIReference(env, thisObject);
	BRTransaction *transaction = (BRTransaction *) getJNIReference(env, transactionObject);

	// Convert phraseByteArray to a char* phrase
	size_t phraseLen = (size_t) (*env)->GetArrayLength(env, phraseByteArray);
	const jbyte *phraseBytes = (const jbyte *) (*env)->GetByteArrayElements(env, phraseByteArray, 0);

	char phrase [1 + phraseLen];
	memcpy (phrase, phraseBytes, phraseLen);
	phrase[phraseLen] = '\0';

	// Convert phrase to its BIP38 512 bit seed.
	UInt512 seed;
	BRBIP39DeriveKey (&seed, phrase, NULL);

	// Sign with the seed
	return (jboolean) (1 == BRWalletSignTransaction(wallet, transaction, forkId, &seed, sizeof(seed))
			? JNI_TRUE
			: JNI_FALSE);
}

/*
 * Class:     com_elastos_spvcore_CoreWallet
 * Method:    containsTransaction
 * Signature: (Lcom/breadwallet/core/BRCoreTransaction;)Z
 */
JNIEXPORT jboolean JNICALL
Java_com_elastos_spvcore_CoreWallet_containsTransaction
(JNIEnv *env, jobject thisObject, jobject transactionObject) {
	BRWallet  *wallet  = (BRWallet  *) getJNIReference (env, thisObject);
	BRTransaction *transaction = (BRTransaction *) getJNIReference (env, transactionObject);
	return (jboolean) BRWalletContainsTransaction (wallet, transaction);
}

/*
 * Class:     com_elastos_spvcore_CoreWallet
 * Method:    jniRegisterTransaction
 * Signature: (Lcom/breadwallet/core/BRCoreTransaction;)Z
 */
JNIEXPORT jboolean JNICALL
Java_com_elastos_spvcore_CoreWallet_jniRegisterTransaction
(JNIEnv *env, jobject thisObject, jobject transactionObject) {
	BRWallet  *wallet  = (BRWallet  *) getJNIReference (env, thisObject);
	BRTransaction *transaction = (BRTransaction *) getJNIReference (env, transactionObject);

	// The `transaction` is now owned by Core; it may be freed.
	return (jboolean) BRWalletRegisterTransaction (wallet, transaction);
}

/*
 * Class:     com_elastos_spvcore_CoreWallet
 * Method:    removeTransaction
 * Signature: ([B)V
 */
JNIEXPORT void JNICALL
Java_com_elastos_spvcore_CoreWallet_removeTransaction
(JNIEnv *env, jobject thisObject, jbyteArray hashByteArray) {
	BRWallet  *wallet  = (BRWallet  *) getJNIReference (env, thisObject);

	uint8_t *hashData = (uint8_t *) (*env)->GetByteArrayElements(env, hashByteArray, 0);
	UInt256 hash;
	UInt256Get(&hash, hashData);
	BRWalletRemoveTransaction (wallet, hash);
}

/*
 * Class:     com_elastos_spvcore_CoreWallet
 * Method:    updateTransactions
 * Signature: ([[BJJ)V
 */
JNIEXPORT void JNICALL
Java_com_elastos_spvcore_CoreWallet_updateTransactions
(JNIEnv *env, jobject thisObject,
 jobjectArray transactionsHashesArray,
 jlong blockHeight,
 jlong timestamp) {
	BRWallet  *wallet  = (BRWallet  *) getJNIReference (env, thisObject);

	size_t txCount = (size_t) (*env)->GetArrayLength (env, transactionsHashesArray);
	UInt256 *txHashes = (UInt256 *) calloc (txCount, sizeof (UInt256));

	for (int i = 0; i < txCount; i++) {
		jbyteArray txHashByteArray = (jbyteArray) (*env)->GetObjectArrayElement (env, transactionsHashesArray, 0);
		const jbyte *txHashBytes = (*env)->GetByteArrayElements (env, txHashByteArray, 0);
		UInt256Get(&txHashes[i], txHashBytes);
	}
	BRWalletUpdateTransactions(wallet, txHashes, txCount, (uint32_t) blockHeight, (uint32_t) timestamp);

	if (NULL == txHashes) free (txHashes);
}

/*
 * Class:     com_elastos_spvcore_CoreWallet
 * Method:    jniTransactionForHash
 * Signature: ([B)Lcom/breadwallet/core/BRCoreTransaction;
 */
JNIEXPORT jobject JNICALL
Java_com_elastos_spvcore_CoreWallet_jniTransactionForHash
(JNIEnv *env, jobject thisObject, jbyteArray hashByteArray) {
	BRWallet  *wallet  = (BRWallet  *) getJNIReference (env, thisObject);

	uint8_t *hashData = (uint8_t *) (*env)->GetByteArrayElements(env, hashByteArray, 0);
	UInt256 hash;
	UInt256Get(&hash, hashData);
	return (*env)->NewObject (env, transactionClass, transactionConstructor,
			(jlong) JNI_COPY_TRANSACTION(BRWalletTransactionForHash(wallet, hash)));
}

/*
 * Class:     com_elastos_spvcore_CoreWallet
 * Method:    transactionIsValid
 * Signature: (Lcom/breadwallet/core/BRCoreTransaction;)I
 */
JNIEXPORT jboolean JNICALL
Java_com_elastos_spvcore_CoreWallet_transactionIsValid
(JNIEnv *env, jobject thisObject, jobject transactionObject) {
	BRWallet  *wallet  = (BRWallet  *) getJNIReference (env, thisObject);
	BRTransaction *transaction = (BRTransaction *) getJNIReference (env, transactionObject);
	return (jboolean) BRWalletTransactionIsValid (wallet, transaction);
}

/*
 * Class:     com_elastos_spvcore_CoreWallet
 * Method:    transactionIsPending
 * Signature: (Lcom/breadwallet/core/BRCoreTransaction;)I
 */
JNIEXPORT jboolean JNICALL
Java_com_elastos_spvcore_CoreWallet_transactionIsPending
(JNIEnv *env, jobject thisObject, jobject transactionObject) {
	BRWallet  *wallet  = (BRWallet  *) getJNIReference (env, thisObject);
	BRTransaction *transaction = (BRTransaction *) getJNIReference (env, transactionObject);
	return (jboolean) BRWalletTransactionIsPending (wallet, transaction);
}

/*
 * Class:     com_elastos_spvcore_CoreWallet
 * Method:    transactionIsVerified
 * Signature: (Lcom/breadwallet/core/BRCoreTransaction;)I
 */
JNIEXPORT jboolean JNICALL
Java_com_elastos_spvcore_CoreWallet_transactionIsVerified
(JNIEnv *env, jobject thisObject, jobject transactionObject) {
	BRWallet  *wallet  = (BRWallet  *) getJNIReference (env, thisObject);
	BRTransaction *transaction = (BRTransaction *) getJNIReference (env, transactionObject);
	return (jboolean) BRWalletTransactionIsVerified (wallet, transaction);
}


/*
 * Class:     com_elastos_spvcore_CoreWallet
 * Method:    transactionFee
 * Signature: (Lcom/breadwallet/core/BRCoreTransaction;)J
 */
JNIEXPORT jlong JNICALL
Java_com_elastos_spvcore_CoreWallet_getTransactionFee
(JNIEnv *env, jobject thisObject, jobject transactionObject) {
	BRWallet *wallet = (BRWallet *) getJNIReference (env, thisObject);
	BRTransaction *transaction = (BRTransaction *) getJNIReference (env, transactionObject);
	return BRWalletFeeForTx (wallet, transaction);
}

/*
 * Class:     com_elastos_spvcore_CoreWallet
 * Method:    transactionAmountSent
 * Signature: (Lcom/breadwallet/core/BRCoreTransaction;)J
 */
JNIEXPORT jlong JNICALL
Java_com_elastos_spvcore_CoreWallet_getTransactionAmountSent
(JNIEnv *env, jobject thisObject, jobject transactionObject) {
	BRWallet *wallet = (BRWallet *) getJNIReference (env, thisObject);
	BRTransaction *transaction = (BRTransaction *) getJNIReference (env, transactionObject);
	return BRWalletAmountSentByTx (wallet, transaction);
}

/*
 * Class:     com_elastos_spvcore_CoreWallet
 * Method:    transactionAmountReceived
 * Signature: (Lcom/breadwallet/core/BRCoreTransaction;)J
 */
JNIEXPORT jlong JNICALL
Java_com_elastos_spvcore_CoreWallet_getTransactionAmountReceived
(JNIEnv *env, jobject thisObject, jobject transactionObject) {
	BRWallet *wallet = (BRWallet *) getJNIReference (env, thisObject);
	BRTransaction *transaction = (BRTransaction *) getJNIReference (env, transactionObject);
	return BRWalletAmountReceivedFromTx (wallet, transaction);
}

/*
 * Class:     com_elastos_spvcore_CoreWallet
 * Method:    getBalanceAfterTransaction
 * Signature: (Lcom/breadwallet/core/BRCoreTransaction;)J
 */
JNIEXPORT jlong JNICALL
Java_com_elastos_spvcore_CoreWallet_getBalanceAfterTransaction
(JNIEnv *env, jobject thisObject, jobject transactionObject) {
	BRWallet *wallet = (BRWallet *) getJNIReference (env, thisObject);
	BRTransaction *transaction = (BRTransaction *) getJNIReference (env, transactionObject);
	return (jlong) wallet->balanceAfterTx (wallet, transaction);
}

/*
 * Class:     com_elastos_spvcore_CoreWallet
 * Method:    getFeeForTransactionSize
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL
Java_com_elastos_spvcore_CoreWallet_getFeeForTransactionSize
(JNIEnv *env, jobject thisObject, jlong size) {
	BRWallet *wallet = (BRWallet *) getJNIReference (env, thisObject);
	return (jlong) BRWalletFeeForTxSize (wallet, (size_t) size);
}

/*
 * Class:     com_elastos_spvcore_CoreWallet
 * Method:    getFeeForTransactionAmount
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL
Java_com_elastos_spvcore_CoreWallet_getFeeForTransactionAmount
(JNIEnv *env, jobject thisObject, jlong amount) {
	BRWallet *wallet = (BRWallet *) getJNIReference (env, thisObject);
	return (jlong) BRWalletFeeForTxAmount (wallet, (uint64_t) amount);
}

/*
 * Class:     com_elastos_spvcore_CoreWallet
 * Method:    getMinOutputAmount
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_com_elastos_spvcore_CoreWallet_getMinOutputAmount
(JNIEnv *env, jobject thisObject) {
	BRWallet *wallet = (BRWallet *) getJNIReference (env, thisObject);
	return (jlong) BRWalletMinOutputAmount (wallet);
}

/*
 * Class:     com_elastos_spvcore_CoreWallet
 * Method:    getMaxOutputAmount
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_com_elastos_spvcore_CoreWallet_getMaxOutputAmount
(JNIEnv *env, jobject thisObject) {
	BRWallet *wallet = (BRWallet *) getJNIReference (env, thisObject);
	return (jlong) BRWalletMaxOutputAmount (wallet);
}

/*
 * Class:     com_elastos_spvcore_CoreWallet
 * Method:    disposeNative
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_com_elastos_spvcore_CoreWallet_disposeNative
(JNIEnv *env, jobject thisObject) {
	BRWallet *wallet = (BRWallet *) getJNIReference(env, thisObject);

	if (NULL != wallet) {
		// TODO: Locate `installListener` WeakGlobal reference - delete it.

		BRWalletFree(wallet);
	}
}

/*
 * Class:     com_elastos_spvcore_CoreWallet
 * Method:    initializeNative
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_elastos_spvcore_CoreWallet_initializeNative
(JNIEnv *env, jclass thisClass) {
	addressClass = (*env)->FindClass(env, "com/breadwallet/core/BRCoreAddress");
	assert (NULL != addressClass);
	addressClass = (*env)->NewGlobalRef (env, addressClass);

	addressConstructor = (*env)->GetMethodID(env, addressClass, "<init>", "(J)V");
	assert (NULL != addressConstructor);

	transactionClass = (*env)->FindClass (env, "com/breadwallet/core/BRCoreTransaction");
	assert (NULL != transactionClass);
	transactionClass = (*env)->NewGlobalRef (env, transactionClass);

	transactionConstructor = (*env)->GetMethodID(env, transactionClass, "<init>", "(J)V");
	assert (NULL != transactionConstructor);
}

//
//
//
static jmethodID
lookupListenerMethod (JNIEnv *env, jobject listener, char *name, char *type) {
	return (*env)->GetMethodID(env,
			(*env)->GetObjectClass(env, listener),
			name,
			type);
}

static void
balanceChanged(void *info, uint64_t balance) {
	JNIEnv *env = getEnv();
	if (NULL == env) return;

	jobject listener = (*env)->NewLocalRef (env, (jobject) info);
	if ((*env)->IsSameObject (env, listener, NULL)) return; // GC reclaimed

	// The onBalanceChanged callback
	jmethodID listenerMethod =
		lookupListenerMethod(env, listener,
				"balanceChanged",
				"(J)V");
	assert (NULL != listenerMethod);

	(*env)->CallVoidMethod(env, listener, listenerMethod, balance);
	(*env)->DeleteLocalRef (env, listener);
}

static void
txAdded(void *info, BRTransaction *tx) {
	// void onTxAdded(BRCoreTransaction);

	JNIEnv *env = getEnv();
	if (NULL == env) return;

	jobject listener = (*env)->NewLocalRef(env, (jobject) info);
	if ((*env)->IsSameObject (env, listener, NULL)) return; // GC reclaimed

	// The onTxAdded listener
	jmethodID listenerMethod =
		lookupListenerMethod(env, listener,
				"onTxAdded",
				"(Lcom/breadwallet/core/BRCoreTransaction;)V");
	assert (NULL != listenerMethod);

	// Create the BRCoreTransaction
	jobject transaction = (*env)->NewObject (env, transactionClass, transactionConstructor,
			(jlong) JNI_COPY_TRANSACTION(tx));

	// Invoke the callback with the provided transaction
	(*env)->CallVoidMethod(env, listener,
			listenerMethod,
			transaction);
	(*env)->DeleteLocalRef (env, listener);
	(*env)->DeleteLocalRef (env, transaction);
}

static void
txUpdated(void *info, const UInt256 txHashes[], size_t count, uint32_t blockHeight,
		uint32_t timestamp) {

	JNIEnv *env = getEnv();
	if (NULL == env) return;

	jobject listener = (*env)->NewLocalRef (env, (jobject) info);
	if ((*env)->IsSameObject (env, listener, NULL)) return; // GC reclaimed

	// The onTxUpdated callback
	jmethodID listenerMethod =
		lookupListenerMethod(env, listener,
				"onTxUpdated",
				"(Ljava/lang/String;II)V");
	assert (NULL != listenerMethod);

	// Invoke the callback for each of txHashes.
	for (size_t i = 0; i < count; i++) {
		jstring hash = (*env)->NewStringUTF(env, u256hex(txHashes[i]));

		(*env)->CallVoidMethod(env, listener, listenerMethod,
				hash,
				blockHeight,
				timestamp);
		(*env)->DeleteLocalRef(env, hash);
	}
	(*env)->DeleteLocalRef(env, listener);
}

static void
txDeleted(void *info, UInt256 txHash, int notifyUser, int recommendRescan) {
	JNIEnv *env = getEnv();
	if (NULL == env) return;

	jobject listener = (*env)->NewLocalRef (env, (jobject) info);
	if ((*env)->IsSameObject (env, listener, NULL)) return; // GC reclaimed

	// The onTxDeleted callback
	jmethodID listenerMethod =
		lookupListenerMethod(env, listener,
				"onTxDeleted",
				"(Ljava/lang/String;II)V");
	assert (NULL != listenerMethod);

	// Invoke the callback for the provided txHash
	jstring hash = (*env)->NewStringUTF(env, u256hex(txHash));

	(*env)->CallVoidMethod(env, listener, listenerMethod,
			hash,
			notifyUser,
			recommendRescan);

	(*env)->DeleteLocalRef(env, hash);
	(*env)->DeleteLocalRef(env, listener);
}
