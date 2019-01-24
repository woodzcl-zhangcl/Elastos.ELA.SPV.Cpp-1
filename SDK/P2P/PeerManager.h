// Copyright (c) 2012-2018 The Elastos Open Source Project
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef __ELASTOS_SDK_PEERMANAGER_H__
#define __ELASTOS_SDK_PEERMANAGER_H__

#include "Peer.h"
#include "TransactionPeerList.h"
#include "PublishedTransaction.h"

#include <SDK/TransactionHub/TransactionHub.h>
#include <SDK/Common/CMemBlock.h>
#include <SDK/Base/Lockable.h>
#include <SDK/Base/BloomFilter.h>
#include <SDK/Wrapper/ChainParams.h>
#include <SDK/Wrapper/WrapperList.h>
#include <SDK/Plugin/Interface/IMerkleBlock.h>
#include <SDK/Plugin/Block/MerkleBlock.h>
#include <SDK/Plugin/Registry.h>

#include <string>
#include <vector>
#include <boost/weak_ptr.hpp>
#include <boost/thread/mutex.hpp>

#define PEER_MAX_CONNECTIONS 1

namespace Elastos {
	namespace ElaWallet {

		typedef ElementSet<MerkleBlockPtr> BlockSet;

		class PeerManager :
				public Lockable,
				public Peer::Listener {
		public:

			class Listener {
			public:
				Listener(const PluginType &pluginTypes);

				virtual ~Listener() {}

				// func syncStarted()
				virtual void syncStarted() = 0;

				virtual void syncProgress(uint32_t currentHeight, uint32_t estimateHeight) = 0;

				// func syncStopped(_ error: BRPeerManagerError?)
				virtual void syncStopped(const std::string &error) = 0;

				// func txStatusUpdate()
				virtual void txStatusUpdate() = 0;

				// func saveBlocks(_ replace: Bool, _ blocks: [BRBlockRef?])
				virtual void saveBlocks(bool replace, const std::vector<MerkleBlockPtr> &blocks) = 0;

				// func savePeers(_ replace: Bool, _ peers: [BRPeer])
				virtual void savePeers(bool replace, const std::vector<PeerInfo> &peers) = 0;

				// func networkIsReachable() -> Bool}
				virtual bool networkIsReachable() = 0;

				// Called on publishTransaction
				virtual void txPublished(const std::string &hash, const nlohmann::json &result) = 0;

				virtual void blockHeightIncreased(uint32_t blockHeight) = 0;

				virtual void syncIsInactive(uint32_t time) = 0;

				const PluginType &getPluginTypes() const { return _pluginTypes; }

			protected:
				PluginType _pluginTypes;
			};

		public:
			PeerManager(const ChainParams &params,
						const WalletPtr &wallet,
						uint32_t earliestKeyTime,
						uint32_t reconnectSeconds,
						const std::vector<MerkleBlockPtr> &blocks,
						const std::vector<PeerInfo> &peers,
						const boost::shared_ptr<Listener> &listener,
						const PluginType &plugin);

			~PeerManager();

			/**
			* Connect to bitcoin peer-to-peer network (also call this whenever networkIsReachable()
			* status changes)
			*/
			void connect();

			/**
			* Disconnect from bitcoin peer-to-peer network (may cause syncFailed(), saveBlocks() or
			* savePeers() callbacks to fire)
			*/
			void disconnect();

			void rescan();

			uint32_t getSyncStartHeight() const;

			uint32_t getEstimatedBlockHeight() const;

			uint32_t GetLastBlockHeight() const;

			uint32_t GetLastBlockTimestamp() const;

			time_t getKeepAliveTimestamp() const;

			void SetKeepAliveTimestamp(time_t t);

			double getSyncProgress(uint32_t startHeight);

			Peer::ConnectStatus getConnectStatus() const;

			bool SyncSucceeded() const;

			void SetSyncSucceeded(bool succeeded);

			bool IsReconnectEnable() const;

			void SetReconnectEnableStatus(bool status);

			void setFixedPeer(UInt128 address, uint16_t port);

			void setFixedPeers(const std::vector<PeerInfo> &peers);

			bool useFixedPeer(const std::string &node, int port);

			std::string getCurrentPeerName() const;

			std::string getDownloadPeerName() const;

			const PeerPtr getDownloadPeer() const;

			size_t getPeerCount() const;

			void publishTransaction(const TransactionPtr &transaction);

			void publishTransaction(const TransactionPtr &transaction, const Peer::PeerPubTxCallback &callback);

			uint64_t getRelayCount(const UInt256 &txHash) const;

			void asyncConnect(const boost::system::error_code &error);

			const std::vector<PublishedTransaction> getPublishedTransaction() const;

			const std::vector<UInt256> getPublishedTransactionHashes() const;

			int ReconnectTaskCount() const;

			void SetReconnectTaskCount(int count);

			const PluginType &GetPluginType() const;

			const std::vector<PeerInfo> &GetPeers() const;

			void SetPeers(const std::vector<PeerInfo> &peers);

			const std::string &GetID() const;

		public:
			virtual void OnConnected(const PeerPtr &peer);

			virtual void OnDisconnected(const PeerPtr &peer, int error);

			virtual void OnRelayedPeers(const PeerPtr &peer, const std::vector<PeerInfo> &peers);

			virtual void OnRelayedTx(const PeerPtr &peer, const TransactionPtr &tx);

			virtual void OnHasTx(const PeerPtr &peer, const UInt256 &txHash);

			virtual void OnRejectedTx(const PeerPtr &peer, const UInt256 &txHash, uint8_t code, const std::string &reason);

			virtual void OnRelayedBlock(const PeerPtr &peer, const MerkleBlockPtr &block);

			virtual void OnRelayedPingMsg(const PeerPtr &peer);

			virtual void OnNotfound(const PeerPtr &peer, const std::vector<UInt256> &txHashes,
									const std::vector<UInt256> &blockHashes);

			virtual void OnSetFeePerKb(const PeerPtr &peer, uint64_t feePerKb);

			virtual const TransactionPtr &OnRequestedTx(const PeerPtr &peer, const UInt256 &txHash);

			virtual bool OnNetworkIsReachable(const PeerPtr &peer);

			virtual void OnThreadCleanup(const PeerPtr &peer);

		private:
			void fireSyncStarted();

			void fireSyncProgress(uint32_t currentHeight, uint32_t estimatedHeight);

			void fireSyncStopped(int error);

			void fireTxStatusUpdate();

			void fireSaveBlocks(bool replace, const std::vector<MerkleBlockPtr> &blocks);

			void fireSavePeers(bool replace, const std::vector<PeerInfo> &peers);

			bool fireNetworkIsReachable();

			void fireTxPublished(const UInt256 &hash, int code, const std::string &reason);

			void fireThreadCleanup();

			void fireBlockHeightIncreased(uint32_t height);

			void fireSyncIsInactive(uint32_t time);

			int verifyDifficulty(const ChainParams &params, const MerkleBlockPtr &block,
								 const BlockSet &blockSet);

			int verifyDifficultyInner(const MerkleBlockPtr &block, const MerkleBlockPtr &previous,
									  uint32_t transitionTime, uint32_t targetTimeSpan, uint32_t targetTimePerBlock);

			void loadBloomFilter(const PeerPtr &peer);

			void updateBloomFilter();

			void findPeers();

			void sortPeers();

			void syncStopped();

			void addTxToPublishList(const TransactionPtr &tx, const Peer::PeerPubTxCallback &callback);

			void publishPendingTx(const PeerPtr &peer);

			size_t
			addPeerToList(const PeerPtr &peer, const UInt256 &txHash, std::vector<TransactionPeerList> &peerList);

			bool
			removePeerFromList(const PeerPtr &peer, const UInt256 &txHash, std::vector<TransactionPeerList> &peerList);

			void peerMisbehaving(const PeerPtr &peer);

			std::vector<UInt128> addressLookup(const std::string &hostname);

			bool verifyBlock(const MerkleBlockPtr &block, const MerkleBlockPtr &prev, const PeerPtr &peer);

			std::vector<UInt256> getBlockLocators();

			void loadMempools();

			void ResendUnconfirmedTx(const PeerPtr &peer);

			void requestUnrelayedTx(const PeerPtr &peer);

			bool peerListHasPeer(const std::vector<TransactionPeerList> &peerList, const UInt256 &txhash,
								 const PeerPtr &peer);

			size_t PeerListCount(const std::vector<TransactionPeerList> &list, const UInt256 &txhash);

			void loadBloomFilterDone(const PeerPtr &peer, int success);

			void updateFilterRerequestDone(const PeerPtr &peer, int success);

			void updateFilterLoadDone(const PeerPtr &peer, int success);

			void updateFilterPingDone(const PeerPtr &peer, int success);

			void mempoolDone(const PeerPtr &peer, int success);

			void requestUnrelayedTxGetDataDone(const PeerPtr &peer, int success);

			void publishTxInivDone(const PeerPtr &peer, int success);

		private:
			int _isConnected, _connectFailureCount, _misbehavinCount, _dnsThreadCount, _maxConnectCount, _reconnectTaskCount;
			bool _syncSucceeded, _enableReconnect, _needGetAddr;

			std::vector<PeerInfo> _peers;
			std::vector<PeerInfo> _fiexedPeers;
			PeerInfo _fixedPeer;

			std::vector<PeerPtr> _connectedPeers;
			PeerPtr _downloadPeer;

			mutable std::string _downloadPeerName;
			time_t _keepAliveTimestamp;
			uint32_t _earliestKeyTime, _reconnectSeconds, _syncStartHeight, _filterUpdateHeight, _estimatedHeight;
			BloomFilterPtr _bloomFilter;
			double _fpRate, _averageTxPerBlock;
			BlockSet _blocks;
			std::set<MerkleBlockPtr> _orphans;
			BlockSet _checkpoints;
			MerkleBlockPtr _lastBlock, _lastOrphan;
			std::vector<TransactionPeerList> _txRelays, _txRequests;
			std::vector<PublishedTransaction> _publishedTx;
			std::vector<UInt256> _publishedTxHashes;

			PluginType _pluginType;
			WalletPtr _wallet;
			ChainParams _chainParams;
			boost::weak_ptr<Listener> _listener;
		};

		typedef boost::shared_ptr<PeerManager> PeerManagerPtr;

	}
}

#endif //__ELASTOS_SDK_PEERMANAGER_H__