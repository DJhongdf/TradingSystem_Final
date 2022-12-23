#pragma once

#ifndef BONDEXECUTIONSERVICE_HPP
#define BONDEXECUTIONSERVICE_HPP

#include "BondAlgoExecutionService.hpp"
#include "executionservice.hpp"
#include "soa.hpp"
#include "products.hpp"
#include <string>
#include <iostream>

using namespace std;

//Connector
class BondExecutionServiceConnector :public Connector<ExecutionOrder<Bond> > {
public:
	// connector
	BondExecutionServiceConnector();

	// override base class
	void Publish(ExecutionOrder<Bond>& data) override;
};

BondExecutionServiceConnector::BondExecutionServiceConnector() {}

void BondExecutionServiceConnector::Publish(ExecutionOrder<Bond>& data) {
	auto bond = data.GetProduct();
	string orderType;
	switch (data.GetOrderType()) {
	case FOK: orderType = "FOK"; break;
	case MARKET: orderType = "MARKET"; break;
	case LIMIT: orderType = "LIMIT"; break;
	case STOP: orderType = "STOP"; break;
	case IOC: orderType = "IOC"; break;
	}
	cout << bond.GetProductId() << " OrderId: " << data.GetOrderId() << "\n"
		<< "    PricingSide: " << (data.GetSide() == BID ? "Bid" : "Ask")
		<< " OrderType: " << orderType << " IsChildOrder: " << (data.IsChildOrder() ? "True" : "False")
		<< "\n"
		<< "    Price: " << data.GetPrice() << " VisibleQuantity: " << data.GetVisibleQuantity()
		<< " HiddenQuantity: " << data.GetHiddenQuantity() << "\n" << endl;
	return;
}

//Service
class BondExecutionService : public ExecutionService<Bond> {
private:
	map<string, ExecutionOrder<Bond> > executionMap;
	vector<ServiceListener<ExecutionOrder<Bond> >*> listeners;
	BondExecutionServiceConnector* bondExecutionServiceConnector;
public:
	// constructor
	BondExecutionService(BondExecutionServiceConnector* _bondExecutionServiceConnector);

	// override base class
	ExecutionOrder<Bond>& GetData(string id) override;

	void OnMessage(ExecutionOrder<Bond>& data) override;

	void AddListener(ServiceListener<ExecutionOrder<Bond> >* listener) override;

	const vector<ServiceListener<ExecutionOrder<Bond> >*>& GetListeners() const override;

	void ExecuteOrder(ExecutionOrder<Bond>& order, Market market);

	void AddAlgoExecution(const AlgoExecution& algo);
};

class BondExecutionService;
BondExecutionService::BondExecutionService(BondExecutionServiceConnector* _bondExecutionServiceConnector) {
	bondExecutionServiceConnector = _bondExecutionServiceConnector;
}

ExecutionOrder<Bond>& BondExecutionService::GetData(string id) {
	return executionMap.at(id);
}

void BondExecutionService::OnMessage(ExecutionOrder<Bond>&) {
	return;
}

void BondExecutionService::AddListener(ServiceListener<ExecutionOrder<Bond> >* listener) {
	listeners.push_back(listener);
	return;
}

const vector<ServiceListener<ExecutionOrder<Bond> >*>& BondExecutionService::GetListeners() const {
	return listeners;
}


void BondExecutionService::ExecuteOrder(ExecutionOrder<Bond>& order, Market market) {
	bondExecutionServiceConnector->Publish(order);
}


void BondExecutionService::AddAlgoExecution(const AlgoExecution& algo) {
	auto executionOrder = algo.GetExecutionOrder();
	string id = executionOrder.GetProduct().GetProductId();

	if (executionMap.find(id) != executionMap.end())
		executionMap.erase(id);
	executionMap.insert(pair<string, ExecutionOrder<Bond> >(id, executionOrder));

	for (auto& listener : listeners) {
		listener->ProcessAdd(executionOrder);
	}
	return;
}

//Listener
class BondExecutionServiceListener : public ServiceListener<AlgoExecution> {
private:
	BondExecutionService* bondExecutionService;

public:
	// Constructor
	BondExecutionServiceListener(BondExecutionService* _bondExecutionService);

	// override base class
	void ProcessAdd(AlgoExecution& data) override;

	void ProcessRemove(AlgoExecution& data) override {};

	void ProcessUpdate(AlgoExecution& data) override {};
};

BondExecutionServiceListener::BondExecutionServiceListener(BondExecutionService* _bondExecutionService) {
	bondExecutionService = _bondExecutionService;
}

void BondExecutionServiceListener::ProcessAdd(AlgoExecution& data) {
	auto order = data.GetExecutionOrder();
	bondExecutionService->AddAlgoExecution(data);
	bondExecutionService->ExecuteOrder(order, BROKERTEC);
	return;
}

#endif