// Copyright (c) 2012-2018 The Elastos Open Source Project
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef __ELASTOS_SDK_MASTERWALLETMANAGER_H__
#define __ELASTOS_SDK_MASTERWALLETMANAGER_H__

#include <map>

#include "Interface/IMasterWalletManager.h"

namespace Elastos {
	namespace SDK {

		class MasterWalletManager : public IMasterWalletManager {
		public:
			MasterWalletManager();

			virtual ~MasterWalletManager();

			void SaveConfigs();

			virtual IMasterWallet *CreateMasterWallet(
					const std::string &masterWalletId,
					const std::string &language = "english");

			/**
			 * Create an new master wallet, an random seed will be generated combined with phrase password to create
			 * 	root key and chain code.
			 * @param phrasePassword combine with random seed to generate root key and chain code. Phrase password can be empty or between 8 and 128, otherwise will throw invalid argument exception.
			 * @param payPassword use to encrypt important things(such as private key) in memory. Pay password should between 8 and 128, otherwise will throw invalid argument exception.
			 * @param language specify language of mnemonic. Language should not be empty, and exit corresponding language config file under the root path. The config begin with fixed prefix "mnemonic_" and end with ".txt" extension, for example mnemonic of Chinese config will be "mnemonic_chinese.txt".
			 * @param rootPath specify directory for all config files, including mnemonic config files and peer connection config files. Root should not be empty, otherwise will throw invalid argument exception.
			 * @return If success will return a pointer of master wallet interface.
			 */
			virtual bool InitializeMasterWallet(
					const std::string &masterWalletId,
					const std::string &mnemonic,
					const std::string &phrasePassword,
					const std::string &payPassword);

			/**
			 * Get manager existing master wallets.
			 * @return existing master wallet array.
			 */
			virtual std::vector<IMasterWallet *> GetAllMasterWallets() const;

			/**
			 * Destroy a master wallet.
			 * @param masterWallet A pointer of master wallet interface create or imported by wallet factory object.
			 */
			virtual void DestroyWallet(
					const std::string &masterWalletId);

			/**
			 * Import master wallet by key store file.
			 * @param keystorePath specify key store file path to import wallet. Key store path should not be empty and really exist.
			 * @param backupPassword use to encrypt key store file. Backup password should between 8 and 128, otherwise will throw invalid argument exception.
			 * @param payPassword use to encrypt important things(such as private key) in memory. Pay password should between 8 and 128, otherwise will throw invalid argument exception.
			 * @param phrasePassword combine with random seed to generate root key and chain code. Phrase password can be empty or between 8 and 128, otherwise will throw invalid argument exception.
			 * @param rootPath specify directory for all config files, including mnemonic config files and peer connection config files. Root should not be empty, otherwise will throw invalid argument exception.
			 * @return If success will return a pointer of master wallet interface.
			 */
			virtual IMasterWallet *ImportWalletWithKeystore(
					const std::string &masterWalletId,
					const std::string &keystorePath,
					const std::string &backupPassword,
					const std::string &payPassword,
					const std::string &phrasePassword = "");

			/**
			 * Import master wallet by mnemonic.
			 * @param mnemonic for importing the master wallet.
			 * @param phrasePassword combine with mnemonic to generate root key and chain code. Phrase password can be empty or between 8 and 128, otherwise will throw invalid argument exception.
			 * @param payPassword use to encrypt important things(such as private key) in memory. Pay password should between 8 and 128, otherwise will throw invalid argument exception.
			 * @param language specify language of mnemonic. Language should not be empty, and exit corresponding language config file under the root path. The config begin with fixed prefix "mnemonic_" and end with ".txt" extension, for example mnemonic of Chinese config will be "mnemonic_chinese.txt".
			 * @param rootPath specify directory for all config files, including mnemonic config files and peer connection config files. Root should not be empty, otherwise will throw invalid argument exception.
			 * @return If success will return a pointer of master wallet interface.
			 */
			virtual IMasterWallet *ImportWalletWithMnemonic(
					const std::string &masterWalletId,
					const std::string &mnemonic,
					const std::string &phrasePassword,
					const std::string &payPassword,
					const std::string &language = "english");

			/**
			 * Export key store of the master wallet.
			 * @param masterWallet A pointer of master wallet interface create or imported by wallet factory object.
			 * @param backupPassword use to decrypt key store file. Backup password should between 8 and 128, otherwise will throw invalid argument exception.
			 * @param keystorePath specify the key store path to export. Key store path should not be empty.
			 */
			virtual void ExportWalletWithKeystore(
					IMasterWallet *masterWallet,
					const std::string &backupPassword,
					const std::string &payPassword,
					const std::string &keystorePath);

			/**
			 * Export mnemonic of the master wallet.
			 * @param masterWallet A pointer of master wallet interface create or imported by wallet factory object.
			 * @param payPassword use to decrypt and generate mnemonic temporarily. Pay password should between 8 and 128, otherwise will throw invalid argument exception.
			 * @return If success will return the mnemonic of master wallet.
			 */
			virtual std::string ExportWalletWithMnemonic(
					IMasterWallet *masterWallet,
					const std::string &payPassword);

		protected:
			typedef std::map<std::string, IMasterWallet *> MasterWalletMap;

			explicit MasterWalletManager(const MasterWalletMap& walletMap);

			void initMasterWallets();

		protected:
			MasterWalletMap _masterWalletMap;
		};
	}
}

#endif //__ELASTOS_SDK_MASTERWALLETMANAGER_H__