// Copyright (c) 2012-2018 The Elastos Open Source Project
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define CATCH_CONFIG_MAIN

#include <boost/shared_ptr.hpp>

#include "Key.h"
#include "catch.hpp"
#include "Log.h"

using namespace Elastos::SDK;

TEST_CASE("Key test", "[Key]") {

	SECTION("Default contructor test") {
		Key key;
		REQUIRE(key.getRaw() != nullptr);
	}

	SECTION("Contructor with BRKey") {
		boost::shared_ptr<BRKey> brkey = boost::shared_ptr<BRKey>(new BRKey);
		Key key(brkey);
		REQUIRE(key.getRaw() != nullptr);
	}

	SECTION("Contructor with string and ByteData") {
		std::string priKey = "S6c56bnXQiBjk9mqSYE7ykVQ7NzrRy";
		const char *str = priKey.c_str();
		Key key(priKey);
		REQUIRE(key.getRaw() != nullptr);

		int len = 0;
		while (str[len] != '\0') {
			len++;
		}
		uint8_t data[len];
		memcpy(data, str, len);
		ByteData byteData(data, len);
		Key key1(byteData);

		std::string privateKey = key.getPrivKey();
		REQUIRE(privateKey.empty() == false);
		REQUIRE(privateKey == key1.getPrivKey());

		ByteData byteData1 = key.getPubkey();
		REQUIRE(byteData1.length > 0);
		REQUIRE(byteData1.data != nullptr);
		ByteData byteData2 = key1.getPubkey();
		REQUIRE(byteData1.length == byteData2.length);
		for (int i = 0; i < byteData1.length; i++) {
			REQUIRE(byteData1.data[i] == byteData2.data[i]);
		}

		std::string addr = key.address();
		REQUIRE(addr.empty() == false);
		REQUIRE(addr == key1.address());
	}

	SECTION("Contructor with secret") {
		UInt256 secret = uint256("0000000000000000000000000000000000000000000000000000000000000001");
		Key key(secret, true);
		REQUIRE(key.getRaw() != nullptr);

		UInt256 hash = key.getSecret();
		int result = UInt256Eq(&secret, &hash);
		REQUIRE(result == 1);
		REQUIRE(true == key.getCompressed());

		Key key1(secret, false);
		REQUIRE(false == key1.getCompressed());
	}

	SECTION("Contructor width seed") {
		UInt128 seed = *(UInt128 *) "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F";
		ByteData seedByte(seed.u8, sizeof(seed.u8));
		Key key(seedByte, 0, 97);
		REQUIRE(key.getRaw() != nullptr);
		REQUIRE(key.getPrivKey().empty() == false);

		UInt256 hash = key.getSecret();
		UInt256 target = uint256("00136c1ad038f9a00871895322a487ed14f1cdc4d22ad351cfa1a0d235975dd7");
		int res = UInt256Eq(&hash, &target);
		REQUIRE(res == 1);
	}

	SECTION("Key setPrivKey test") {
		Key key1;
		key1.setPrivKey("S6c56bnXQiBjk9mqSYE7ykVQ7NzrRy");
		REQUIRE(key1.getPrivKey().empty() == false);

		Key key2("S6c56bnXQiBjk9mqSYE7ykVQ7NzrRy");
		REQUIRE(key1.getPrivKey() == key2.getPrivKey());
	}

	SECTION("Key encodeSHA256, sign ,verify and compactSign test") {
		std::string str = "Everything should be made as simple as possible, but not simpler.";
		UInt256 hash = Key::encodeSHA256(str);
		UInt256 zero = UINT256_ZERO;
		int res = UInt256Eq(&hash, &zero);
		REQUIRE(res == 0);

		uint8_t signedData[] = "\x30\x44\x02\x20\x33\xa6\x9c\xd2\x06\x54\x32\xa3\x0f\x3d\x1c\xe4\xeb\x0d\x59\xb8\xab"
				"\x58\xc7\x4f\x27\xc4\x1a\x7f\xdb\x56\x96\xad\x4e\x61\x08\xc9\x02\x20\x6f\x80\x79\x82"
				"\x86\x6f\x78\x5d\x3f\x64\x18\xd2\x41\x63\xdd\xae\x11\x7b\x7d\xb4\xd5\xfd\xf0\x07\x1d"
				"\xe0\x69\xfa\x54\x34\x22\x62";
		UInt256 secret = uint256("0000000000000000000000000000000000000000000000000000000000000001");
		Key key(secret, true);
		ByteData byteData = key.sign(hash);
		REQUIRE(byteData.length > 0);
		REQUIRE(byteData.data != nullptr);
		for (int i = 0; i < sizeof(signedData) - 1; i++) {
			REQUIRE(signedData[i] == byteData.data[i]);
		}

		bool result = key.verify(hash, byteData);
		REQUIRE(result == true);

		ByteData compactData = key.compactSign(byteData);
		REQUIRE(compactData.length > 0);
		REQUIRE(compactData.data != nullptr);
	}

	SECTION("Key encryptNative test") {
		uint8_t bytes1[] = "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F";
		ByteData data(bytes1, sizeof(bytes1));
		ByteData nonce;
		uint8_t bytes2[] = "\x50\x51\x52\x53\xc0\xc1\xc2\xc3\xc4\xc5\xc6\xc7";
		nonce.data = bytes2;
		nonce.length = sizeof(bytes2);

		Key key("S6c56bnXQiBjk9mqSYE7ykVQ7NzrRy");
		ByteData byteData = key.encryptNative(data, nonce);
		REQUIRE(byteData.length >= 16);
		REQUIRE(byteData.data != nullptr);

		ByteData decode = key.decryptNative(byteData, nonce);
		REQUIRE(decode.length == data.length);
		int r = memcmp(bytes1, decode.data, decode.length);
		REQUIRE(r == 0);
	}

	SECTION("Key getSeedFromPhrase, getAuthPrivKeyForAPI, getAuthPublicKeyForAPI test") {
		uint8_t s[] = "bless bird birth blind blossom boil bonus entry equal error fence fetch";
		ByteData phrase(s, sizeof(s));

		ByteData seedByte = Key::getSeedFromPhrase(phrase);
		REQUIRE(seedByte.length > 0);
		REQUIRE(seedByte.data != nullptr);

		ByteData privateByte = Key::getAuthPrivKeyForAPI(seedByte);
		REQUIRE(privateByte.length > 0);
		REQUIRE(privateByte.data != nullptr);

		std::string pubKeyStr = Key::getAuthPublicKeyForAPI(privateByte);
		REQUIRE(pubKeyStr.empty() == false);
	}

	SECTION("Key isValidBitcoinPrivateKey test") {
		bool res = Key::isValidBitcoinPrivateKey("S6c56bnXQiBjk9mqSYE7ykVQ7NzrRz");
		REQUIRE(res == false);

		Key key;
		res = Key::isValidBitcoinPrivateKey(key.getPrivKey());
		REQUIRE(res == true);
	}

	SECTION("Key encodeHex and decodeHex test") {
		uint8_t bytes1[] = "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F";
		ByteData data(bytes1, sizeof(bytes1) - 1);

		std::string str = Key::encodeHex(data);
		REQUIRE(str == "000102030405060708090a0b0c0d0e0f");

		ByteData decodeByte = Key::decodeHex(str);
		int res = memcmp(bytes1, decodeByte.data, decodeByte.length);
		REQUIRE(res == 0);
	}
}