// Copyright (c) 2012-2018 The Elastos Open Source Project
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef __ELASTOS_SDK_IACCOUNT_H__
#define __ELASTOS_SDK_IACCOUNT_H__

#include <boost/shared_ptr.hpp>
#include <nlohmann/json.hpp>

#include "BRInt.h"
#include "Key.h"
#include "MasterPubKey.h"

namespace Elastos {
	namespace ElaWallet {

		class IAccount {
		public:
			virtual ~IAccount() {}

			virtual nlohmann::json GetBasicInfo() const = 0;

			virtual void ChangePassword(const std::string &oldPassword, const std::string &newPassword) = 0;

			virtual UInt512 DeriveSeed(const std::string &payPassword) = 0;

			virtual Key DeriveKey(const std::string &payPassword) = 0;

			virtual std::string GetType() const = 0;

			virtual nlohmann::json ToJson() const = 0;

			virtual void FromJson(const nlohmann::json &j) = 0;

			virtual bool IsReadOnly() const = 0;

			virtual bool IsEqual(const IAccount &account) const = 0;

		public: //properties

			virtual const std::string &GetEncryptedKey() const = 0;

			virtual const std::string &GetEncryptedMnemonic() const = 0;

			virtual const std::string &GetEncryptedPhrasePassword() const = 0;

			virtual const std::string &GetPublicKey() const = 0;

			virtual const MasterPubKey &GetIDMasterPubKey() const = 0;

			virtual std::string GetAddress() const = 0;
		};

		typedef boost::shared_ptr<IAccount> AccountPtr;
	}
}

#endif //__ELASTOS_SDK_IACCOUNT_H__
