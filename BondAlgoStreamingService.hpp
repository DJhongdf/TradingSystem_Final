#pragma once

#ifndef BONDALGOSTREAMINGSERVICE_HPP
#define BONDALGOSTREAMINGSERVICE_HPP

#include "streamingservice.hpp"
#include "pricingservice.hpp"
#include "products.hpp"
#include "soa.hpp"
#include <map>
#include <vector>
#include <string>
#include <iostream>

using namespace std;

class AlgoStream {
private:
	PriceStream<Bond> priceStream;
public:
	// constructor
	AlgoStream(const PriceStream<Bond>& _priceStream);

	void Run(Price<Bond> price);

	PriceStream<Bond> GetPriceStream() const;
};


AlgoStream::AlgoStream(const PriceStream<Bond>& _priceStream) :priceStream(_priceStream) {
}

void AlgoStream::Run(Price<Bond> price) {
	auto bond = price.GetProduct();
	if (bond.GetProductId() != priceStream.GetProduct().GetProductId()) return;

	float mid = price.GetMid();
	float spread = price.GetBidOfferSpread();
	float bid = mid - spread / 2.0;
	float offer = mid + spread / 2.0;

	if (spread < 2.5 / 256.0) {
		PriceStreamOrder order_bid(bid, 1000000, 2000000, BID);
		PriceStreamOrder order_ask(offer, 1000000, 2000000, OFFER);
		priceStream = PriceStream<Bond>(bond, order_bid, order_ask);
	}
	else {
		PriceStreamOrder order_bid(bid, 0, 0, BID);
		PriceStreamOrder order_ask(offer, 0, 0, OFFER);
		priceStream = PriceStream<Bond>(bond, order_bid, order_ask);
	}
	return;
}


PriceStream<Bond> AlgoStream::GetPriceStream() const {
	return priceStream;
}

class BondAlgoStreamingService : public Service<string, AlgoStream>
{
private:
	map<string, AlgoStream > algoMap;
	vector<ServiceListener<AlgoStream>* > listeners;
public:
	// constructor
	BondAlgoStreamingService();

	// override 
	AlgoStream& GetData(string id) override;

	void OnMessage(AlgoStream&) override;

	void AddListener(ServiceListener<AlgoStream>* listener) override;

	const vector<ServiceListener<AlgoStream>*>& GetListeners() const override;

	void AddPrice(const Price<Bond>& price);
};

BondAlgoStreamingService::BondAlgoStreamingService()
{
	algoMap = map<string, AlgoStream>();
}

AlgoStream& BondAlgoStreamingService::GetData(string id)
{
	return algoMap.at(id);
}

void BondAlgoStreamingService::OnMessage(AlgoStream& algo)
{
}

void BondAlgoStreamingService::AddListener(ServiceListener<AlgoStream>* listener)
{
	listeners.push_back(listener);
}

const vector<ServiceListener<AlgoStream>*>& BondAlgoStreamingService::GetListeners() const {
	return listeners;
}

void BondAlgoStreamingService::AddPrice(const Price<Bond>& price)
{
	auto id = price.GetProduct().GetProductId();
	auto it = algoMap.find(id);
	if (it != algoMap.end())
	{
		(it->second).Run(price);
	}
	else {
		PriceStreamOrder p1(0, 0, 0, BID);
		PriceStreamOrder p2(0, 0, 0, OFFER);
		PriceStream<Bond> ps(price.GetProduct(), p1, p2);
		algoMap.insert(pair<string, PriceStream<Bond> >(id, ps));
		AddPrice(price);
		return;
	}
	for (auto& listener : listeners) {
		listener->ProcessAdd(it->second);
	}

	return;
}

//Listener
class BondAlgoStreamingServiceListener : public ServiceListener<Price<Bond> > {
private:
	BondAlgoStreamingService* bondAlgoStreamingService;
public:
	// constructor
	BondAlgoStreamingServiceListener(BondAlgoStreamingService* _bondAlgoStreamingService);

	// override 
	void ProcessAdd(Price<Bond>& price) override;

	void ProcessRemove(Price<Bond>&) override;

	void ProcessUpdate(Price<Bond>&) override;
};

BondAlgoStreamingServiceListener::BondAlgoStreamingServiceListener(BondAlgoStreamingService* _bondAlgoStreamingService) {
	bondAlgoStreamingService = _bondAlgoStreamingService;
}

void BondAlgoStreamingServiceListener::ProcessAdd(Price<Bond>& price) {
	bondAlgoStreamingService->AddPrice(price);
	return;
}

void BondAlgoStreamingServiceListener::ProcessRemove(Price<Bond>& data) {
	return;
}

void BondAlgoStreamingServiceListener::ProcessUpdate(Price<Bond>& data) {
	return;
}

#endif