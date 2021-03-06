// Copyright (c) 2012-2018 The Elastos Open Source Project
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef __ELASTOS_SDK_SPVSDK_ASSET_H
#define __ELASTOS_SDK_SPVSDK_ASSET_H

#include "SDK/Plugin/Interface/ELAMessageSerializable.h"

namespace Elastos {
	namespace ElaWallet {
		class Asset :
				public ELAMessageSerializable {
		public:
			enum AssetType {
				Token = 0x00,
				Share = 0x01,
			};
			enum AssetRecordType {
				Unspent = 0x00,
				Balance = 0x01,
			};
			const static short maxPrecision = 8;
			const static short MinPrecidion = 0;
		public:
			Asset();

			~Asset();

			void setName(const std::string &name);

			std::string getName() const;

			void setDescription(const std::string &desc);

			std::string getDescription() const;

			void setAssetType(Asset::AssetType type);

			Asset::AssetType getAssetType() const;

			void setAssetRecordType(Asset::AssetRecordType type);

			Asset::AssetRecordType getAssetRecordType() const;

			void setPrecision(uint8_t precision);

			uint8_t getPrecision() const;

			virtual void Serialize(ByteStream &ostream) const;

			virtual bool Deserialize(ByteStream &istream);

			virtual nlohmann::json toJson() const;

			virtual void fromJson(const nlohmann::json &jsonData);

		private:
			std::string _name;
			std::string _description;
			uint8_t _precision;
			AssetType _assetType;
			AssetRecordType _recordType;
		};
	}
}

#endif //__ELASTOS_SDK_SPVSDK_ASSET_H
