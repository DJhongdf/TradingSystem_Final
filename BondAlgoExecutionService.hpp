#pragma once

#ifndef BONDALGOEXECUTIONSERVICE_HPP
#define BONDALGOEXECUTIONSERVICE_HPP

#include "BondMarketDataService.hpp"
#include "executionservice.hpp"
#include "soa.hpp"
#include "products.hpp"
#include <string>
#include <random>

using namespace std;

class AlgoExecution {
private:
	static int order_ID;
	ExecutionOrder<Bond> executionOrder;

	string FillCast(string a, int w, char b);
public:
	// constructor
	AlgoExecution(ExecutionOrder<Bond> _executionOrder);

	void RunAlgo(OrderBook<Bond> orderBook);

	ExecutionOrder<Bond> GetExecutionOrder() const;
};

int AlgoExecution::order_ID = 0;

string AlgoExecution::FillCast(string a, int w, char b) {
	string result;
	std::stringstream inter;
	inter << std::setw(w) << std::setfill(b) << a;
	inter >> result;
	return result;
}

AlgoExecution::AlgoExecution(ExecutionOrder<Bond> _executionOrder)
	:executionOrder(_executionOrder) {}

void AlgoExecution::RunAlgo(OrderBook<Bond> orderBook) {
	auto bond = orderBook.GetProduct();
	// not this executionOrder to update
	if (bond.GetProductId() != executionOrder.GetProduct().GetProductId()) return;

	auto orderID = FillCast(to_string(order_ID), 8, '0');

	OrderType orderType;
	int rd = rand() % 5;
	switch (rd) {
	case 0:orderType = FOK; break;
	case 1:orderType = IOC; break;
	case 2:orderType = MARKET; break;
	case 3:orderType = LIMIT; break;
	case 4:orderType = STOP; break;
	}
	PricingSide pricingSide = (rand() % 2 == 0 ? BID : OFFER);


	auto bidOrder = orderBook.GetBidStack().begin();
	auto askOrder = orderBook.GetOfferStack().begin();

	double price;
	long visiableQ = 0, hiddenQ;
	if (pricingSide == BID) {
		price = bidOrder->GetPrice();
		if (askOrder->GetPrice() - bidOrder->GetPrice() < 2.5 / 256.0)
			visiableQ = bidOrder->GetQuantity();
		hiddenQ = 2 * visiableQ;
	}
	else {
		price = askOrder->GetPrice();
		if (askOrder->GetPrice() - bidOrder->GetPrice() < 2.5 / 256.0)
			visiableQ = askOrder->GetQuantity();
		hiddenQ = 2 * visiableQ;
	}

	string parentID = "P" + orderID;

	executionOrder = ExecutionOrder<Bond>(bond, pricingSide, orderID, orderType, price, visiableQ, hiddenQ, parentID, true);

	++order_ID;
	return;
}

ExecutionOrder<Bond> AlgoExecution::GetExecutionOrder() const {
	return executionOrder;
}


class BondAlgoExecutionService : public Service<string, AlgoExecution> {
private:
	map<string, AlgoExecution> algoMap;
	vector<ServiceListener<AlgoExecution >*> listeners;

public:
	// constructor
	BondAlgoExecutionService();

	// override base class 
	AlgoExecution& GetData(string id);

	void OnMessage(AlgoExecution&);

	void AddListener(ServiceListener<AlgoExecution>* listener);

	const vector<ServiceListener<AlgoExecution>*>& GetListeners() const;

	void AddOrderBook(OrderBook<Bond>& orderBook);
};

BondAlgoExecutionService::BondAlgoExecutionService() {
	algoMap = map<string, AlgoExecution>();
}

AlgoExecution& BondAlgoExecutionService::GetData(string id) {
	return algoMap.at(id);
}

void BondAlgoExecutionService::OnMessage(AlgoExecution& a) {
	return;
}

void BondAlgoExecutionService::AddListener(ServiceListener<AlgoExecution>* listener) {
	listeners.push_back(listener);
	return;
}

const vector<ServiceListener<AlgoExecution>*>& BondAlgoExecutionService::GetListeners() const {
	return listeners;
}

void BondAlgoExecutionService::AddOrderBook(OrderBook<Bond>& orderBook) {
	auto id = orderBook.GetProduct().GetProductId();
	auto it = algoMap.find(id);
	if (it != algoMap.end()) {
		(it->second).RunAlgo(orderBook);
	}
	else {
		auto eo = ExecutionOrder<Bond>(orderBook.GetProduct(), BID, "0", FOK, 0, 0, 0, "0", true);
		algoMap.insert(pair<string, AlgoExecution >(id, AlgoExecution(eo)));
		AddOrderBook(orderBook);
		return;
	}
	for (auto& listener : listeners) {
		listener->ProcessAdd(it->second);
	}
	return;
}

//Listener
class BondAlgoExecutionServiceListener : public ServiceListener<OrderBook<Bond> > {
private:
	BondAlgoExecutionService* bondAlgoExecutionService;
public:
	//constructor
	BondAlgoExecutionServiceListener(BondAlgoExecutionService* _bondAlgoExecutionService);

	// override base class
	void ProcessAdd(OrderBook<Bond>& data) override;

	void ProcessRemove(OrderBook<Bond>& data) override {};

	void ProcessUpdate(OrderBook<Bond>& data) override {};
};

BondAlgoExecutionServiceListener::BondAlgoExecutionServiceListener(BondAlgoExecutionService* _bondAlgoExecutionService) {
	bondAlgoExecutionService = _bondAlgoExecutionService;
}

void BondAlgoExecutionServiceListener::ProcessAdd(OrderBook<Bond>& data) {
	bondAlgoExecutionService->AddOrderBook(data);
	return;
}

#endif