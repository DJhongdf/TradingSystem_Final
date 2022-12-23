#pragma once

#ifndef BONDSTREAMINGSERVICE_HPP
#define BONDSTREAMINGSERVICE_HPP

#include "products.hpp"
#include "streamingservice.hpp"
#include "BondAlgoStreamingService.hpp"
#include "soa.hpp"
#include <string>
#include <iostream>

using namespace std;

//Connector
class BondStreamingServiceConnector : public Connector<PriceStream<Bond> > {
public:
	// constructor
	BondStreamingServiceConnector();

	void Subscribe() override;

	void Publish(PriceStream<Bond>& data) override;
};

BondStreamingServiceConnector::BondStreamingServiceConnector() {
}

void BondStreamingServiceConnector::Publish(PriceStream<Bond>& data) {
	auto bond = data.GetProduct();
	auto bidOrder = data.GetBidOrder();
	auto askOrder = data.GetOfferOrder();

	cout << bond.GetProductId() << "\n"
		<< "    BidOrder: " << "Price: " << bidOrder.GetPrice() << "  VisibleQuantity: " << bidOrder.GetVisibleQuantity()
		<< "  HiddenQuantity: " << bidOrder.GetHiddenQuantity() << "\n"
		<< "    offerOrder: " << "Price: " << askOrder.GetPrice() << "  VisibleQuantity: " << askOrder.GetVisibleQuantity()
		<< "  HiddenQuantity: " << askOrder.GetHiddenQuantity() << "\n"
		<< endl;

	return;
}

void BondStreamingServiceConnector::Subscribe() {
}



//Service
class BondStreamingService : public StreamingService<Bond> {
private:
	map<string, PriceStream<Bond> > streamMap;
	vector<ServiceListener<PriceStream<Bond> >*> listeners;
	BondStreamingServiceConnector* bondStreamingServiceConnector;
public:
	// constructor
	BondStreamingService(BondStreamingServiceConnector* _bondStreamingServiceConnector);

	// override base class
	PriceStream<Bond>& GetData(string id) override;

	void OnMessage(PriceStream<Bond>&) override;

	void AddListener(ServiceListener<PriceStream<Bond> >* listener) override;

	const vector<ServiceListener<PriceStream<Bond> >*>& GetListeners() const override;

	void PublishPrice(PriceStream<Bond>& priceStream) override;

	void AddAlgo(const AlgoStream& algo);
};

class BondStreamingService;
BondStreamingService::BondStreamingService(BondStreamingServiceConnector* _bondStreamingServiceConnector) {
	bondStreamingServiceConnector = _bondStreamingServiceConnector;
	streamMap = map<string, PriceStream<Bond> >();
}

PriceStream<Bond>& BondStreamingService::GetData(string id) {
	return streamMap.at(id);
}

void BondStreamingService::OnMessage(PriceStream<Bond>& stream) {
	return;
}

void BondStreamingService::AddListener(ServiceListener<PriceStream<Bond> >* listener) {
	listeners.push_back(listener);
	return;
}

const vector<ServiceListener<PriceStream<Bond> >*>& BondStreamingService::GetListeners() const {
	return listeners;
}

void BondStreamingService::PublishPrice(PriceStream<Bond>& priceStream) {
	bondStreamingServiceConnector->Publish(priceStream);
	return;
}
void BondStreamingService::AddAlgo(const AlgoStream& algo) {
	auto stream = algo.GetPriceStream();
	auto id = stream.GetProduct().GetProductId();
	if (streamMap.find(id) != streamMap.end())
		streamMap.erase(id);
	streamMap.insert(pair<string, PriceStream<Bond> >(id, stream));
	for (auto& listener : listeners) {
		listener->ProcessAdd(stream);
	}

	return;
}

//Listener
class BondStreamingServiceListener : public ServiceListener<AlgoStream> {
private:
	BondStreamingService* bondStreamingService;

public:
	// constructor
	BondStreamingServiceListener(BondStreamingService* _bondStreamingService);

	// override class
	void ProcessAdd(AlgoStream& algoStream) override;

	void ProcessRemove(AlgoStream&) override;

	void ProcessUpdate(AlgoStream&) override;
};


BondStreamingServiceListener::BondStreamingServiceListener(BondStreamingService* _bondStreamingService) {
	bondStreamingService = _bondStreamingService;
}

void BondStreamingServiceListener::ProcessAdd(AlgoStream& algoStream) {
	bondStreamingService->AddAlgo(algoStream);
	auto priceStream = algoStream.GetPriceStream();
	bondStreamingService->PublishPrice(priceStream);
	return;
}

void BondStreamingServiceListener::ProcessRemove(AlgoStream& data) {
	return;
}

void BondStreamingServiceListener::ProcessUpdate(AlgoStream& data) {
	return;
}
#endif 