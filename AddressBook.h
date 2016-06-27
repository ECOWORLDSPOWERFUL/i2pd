#ifndef ADDRESS_BOOK_H__
#define ADDRESS_BOOK_H__

#include <string.h>
#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <mutex>
#include <memory>
#include <boost/asio.hpp>
#include "Base.h"
#include "Identity.h"
#include "Log.h"
#include "Destination.h"

namespace i2p
{
namespace client
{
#ifdef MESHNET
	const char DEFAULT_SUBSCRIPTION_ADDRESS[] = "http://opo57rwxz27frnqxrwiyr6snkybuyetv25fyd25y6se6imj54xfq.b32.i2p/hosts.txt";
#else
	const char DEFAULT_SUBSCRIPTION_ADDRESS[] = "http://joajgazyztfssty4w2on5oaqksz6tqoxbduy553y34mf4byv6gpq.b32.i2p/export/alive-hosts.txt";
#endif
	const int INITIAL_SUBSCRIPTION_UPDATE_TIMEOUT = 3; // in minutes	
	const int INITIAL_SUBSCRIPTION_RETRY_TIMEOUT = 1; // in minutes			
	const int CONTINIOUS_SUBSCRIPTION_UPDATE_TIMEOUT = 720; // in minutes (12 hours)			
	const int CONTINIOUS_SUBSCRIPTION_RETRY_TIMEOUT = 5; // in minutes	
	const int SUBSCRIPTION_REQUEST_TIMEOUT = 60; //in second

	const uint16_t ADDRESS_RESOLVER_DATAGRAM_PORT = 53;	
	const uint16_t ADDRESS_RESPONSE_DATAGRAM_PORT = 54;
	
	inline std::string GetB32Address(const i2p::data::IdentHash& ident) { return ident.ToBase32().append(".b32.i2p"); }

	class AddressBookStorage // interface for storage
	{
		public:

			virtual ~AddressBookStorage () {};
			virtual std::shared_ptr<const i2p::data::IdentityEx> GetAddress (const i2p::data::IdentHash& ident) const = 0;	
			virtual void AddAddress (std::shared_ptr<const i2p::data::IdentityEx> address) = 0;
			virtual void RemoveAddress (const i2p::data::IdentHash& ident) = 0;
		
			virtual bool Init () = 0;
			virtual int Load (std::map<std::string, i2p::data::IdentHash>& addresses) = 0;
			virtual int LoadLocal (std::map<std::string, i2p::data::IdentHash>& addresses) = 0;
			virtual int Save (const std::map<std::string, i2p::data::IdentHash>& addresses) = 0;

			virtual void SaveEtag (const i2p::data::IdentHash& subscription, const std::string& etag, const std::string& lastModified) = 0;
			virtual bool GetEtag (const i2p::data::IdentHash& subscription, std::string& etag, std::string& lastModified) = 0;
	};			

	class AddressBookSubscription;
	class AddressResolver;
	class AddressBook
	{
		public:

			AddressBook ();
			~AddressBook ();
			void Start ();
			void StartResolvers ();
			void Stop ();
			bool GetIdentHash (const std::string& address, i2p::data::IdentHash& ident);
			std::shared_ptr<const i2p::data::IdentityEx> GetAddress (const std::string& address);
			const i2p::data::IdentHash * FindAddress (const std::string& address);
			void LookupAddress (const std::string& address);
			void InsertAddress (const std::string& address, const std::string& base64); // for jump service
			void InsertAddress (std::shared_ptr<const i2p::data::IdentityEx> address);

			bool LoadHostsFromStream (std::istream& f);
			void DownloadComplete (bool success, const i2p::data::IdentHash& subscription, const std::string& etag, const std::string& lastModified);
			//This method returns the ".b32.i2p" address
			std::string ToAddress(const i2p::data::IdentHash& ident) { return GetB32Address(ident); }
			std::string ToAddress(std::shared_ptr<const i2p::data::IdentityEx> ident) { return ToAddress(ident->GetIdentHash ()); }

			bool GetEtag (const i2p::data::IdentHash& subscription, std::string& etag, std::string& lastModified);

		private:

			void StartSubscriptions ();
			void StopSubscriptions ();
			
			void LoadHosts ();
			void LoadSubscriptions ();
			void LoadLocal ();

			void HandleSubscriptionsUpdateTimer (const boost::system::error_code& ecode);

			void StartLookups ();
			void StopLookups ();
			void HandleLookupResponse (const i2p::data::IdentityEx& from, uint16_t fromPort, uint16_t toPort, const uint8_t * buf, size_t len);
			
		private:	

			std::mutex m_AddressBookMutex;
			std::map<std::string, i2p::data::IdentHash>  m_Addresses;
			std::map<i2p::data::IdentHash, std::shared_ptr<AddressResolver> > m_Resolvers; // local destination->resolver
			std::mutex m_LookupsMutex;
			std::map<uint32_t, std::string> m_Lookups; // nonce -> address
			AddressBookStorage * m_Storage;
			volatile bool m_IsLoaded, m_IsDownloading;
			std::vector<AddressBookSubscription *> m_Subscriptions;
			std::unique_ptr<AddressBookSubscription> m_DefaultSubscription; // in case if we don't know any addresses yet
			boost::asio::deadline_timer * m_SubscriptionsUpdateTimer;
	};

	class AddressBookSubscription
	{
		public:

			AddressBookSubscription (AddressBook& book, const std::string& link);
			void CheckSubscription ();

		private:

			void Request ();
			bool ProcessResponse (std::stringstream& s, bool isGzip = false);
		
		private:

			AddressBook& m_Book;
			std::string m_Link, m_Etag, m_LastModified;
			// m_Etag must be surrounded by ""
	};

	class AddressResolver
	{
		public:

			AddressResolver (std::shared_ptr<ClientDestination> destination);
			~AddressResolver ();
			void AddAddress (const std::string& name, const i2p::data::IdentHash& ident);

		private:

			void HandleRequest (const i2p::data::IdentityEx& from, uint16_t fromPort, uint16_t toPort, const uint8_t * buf, size_t len);

		private:

			std::shared_ptr<ClientDestination> m_LocalDestination;
			std::map<std::string, i2p::data::IdentHash>  m_LocalAddresses;
	};
}
}

#endif


