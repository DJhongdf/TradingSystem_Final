#pragma once

#ifndef BONDTRADEBOOKINGSERVICE_HPP
#define BONDTRADEBOOKINGSERVICE_HPP

#include "tradebookingservice.hpp"
#include "BondProductService.hpp"
#include "BondExecutionService.hpp"
#include "soa.hpp"
#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>

using namespace std;

class BondTradeBookingService : public TradeBookingService<Bond>
{
private:
	map<string, Trade<Bond> > tradeMap;
	vector<ServiceListener<Trade<Bond> >*> listeners;
public:
	// constructor
	BondTradeBookingService();

	// override pure virtual functions in base class
	Trade<Bond>& GetData(string id) override;

	void OnMessage(Trade<Bond>& trade) override;

	void AddListener(ServiceListener<Trade<Bond> >* listener) override;

	const vector<ServiceListener<Trade<Bond> >*>& GetListeners() const override;

	void BookTrade(Trade<Bond>& trade) override;
};


BondTradeBookingService::BondTradeBookingService()
{
	tradeMap = map<string, Trade<Bond> >();
}

Trade<Bond>& BondTradeBookingService::GetData(string id)
{
	return tradeMap.at(id);
}

void BondTradeBookingService::OnMessage(Trade<Bond>& trade)
{
	BookTrade(trade);
	return;
}

void BondTradeBookingService::AddListener(ServiceListener<Trade<Bond> >* listener)
{
	listeners.push_back(listener);
	return;
}

const vector<ServiceListener<Trade<Bond> >*>& BondTradeBookingService::GetListeners() const {
	return listeners;
}

void BondTradeBookingService::BookTrade(Trade<Bond>& trade)
{
	auto id = trade.GetProduct().GetProductId();
	if (tradeMap.find(id) != tradeMap.end())
		tradeMap.erase(id);
	tradeMap.insert(pair<string, Trade<Bond> >(id, trade));

	for (auto& listener : listeners) {
		listener->ProcessAdd(trade);
	}
	return;
}

//Connector
class BondTradingBookingServiceConnector : public Connector<Trade<Bond> >
{
private:
	BondTradeBookingService* bondTradeBookingService;
	BondProductService* bondProductService;

	float TransformStringToPrice(string& s);
public:
	// constructor
	BondTradingBookingServiceConnector(BondTradeBookingService* _bondTradeBookingService, BondProductService* _bondProductService);

	// override pure virtual functions in base class
	void Publish(Trade<Bond>& data) override {};

	void Subscribe() override;
};

float BondTradingBookingServiceConnector::TransformStringToPrice(string& s) {
	int n = s.size();
	if (s[n - 1] == '+')s[n - 1] = '4';
	return stoi(s.substr(0, n - 4)) + stoi(s.substr(n - 3, 2)) / 32.0 + stoi(s.substr(n - 1, 1)) / 256.0;
}

BondTradingBookingServiceConnector::BondTradingBookingServiceConnector
(BondTradeBookingService* _bondTradeBookingService, BondProductService* _bondProductService) {
	bondTradeBookingService = _bondTradeBookingService;
	bondProductService = _bondProductService;
}

void BondTradingBookingServiceConnector::Subscribe() {
	ifstream input;
	input.open("trades.txt");

	string row;
	while (getline(input, row)) {
		stringstream line(row);
		string temp;
		getline(line, temp, ',');
		auto bond = bondProductService->GetData(temp);

		getline(line, temp, ',');
		auto tradeID = temp;
		getline(line, temp, ',');
		auto price = TransformStringToPrice(temp);
		getline(line, temp, ',');
		auto book = temp;
		getline(line, temp, ',');
		long quantity = stol(temp);
		getline(line, temp, ',');
		Side side = (temp == "BUY" ? BUY : SELL);

		Trade<Bond> bondNew(bond, tradeID, price, book, quantity, side);
		bondTradeBookingService->OnMessage(bondNew);
	}
	return;
}

//Listener 
class BondTradeBookingServiceListener : public ServiceListener<ExecutionOrder<Bond> > {
private:
	BondTradeBookingService* bondTradeBookingService;

public:
	// constructor
	BondTradeBookingServiceListener(BondTradeBookingService* _bondTradeBookingService);


	void ProcessAdd(ExecutionOrder<Bond>& data) override;


	void ProcessRemove(ExecutionOrder<Bond>& data) override {}


	void ProcessUpdate(ExecutionOrder<Bond>& data) override {}
};

BondTradeBookingServiceListener::BondTradeBookingServiceListener(BondTradeBookingService* _bondTradeBookingService) {
	bondTradeBookingService = _bondTradeBookingService;
}

void BondTradeBookingServiceListener::ProcessAdd(ExecutionOrder<Bond>& data) {
	auto bond = data.GetProduct();
	string tradeID = "Execution";
	auto price = data.GetPrice();
	string book = "Book";
	auto quantity = data.GetVisibleQuantity();
	Side side = BUY;
	if (data.GetSide() == BID)
		side = SELL;
	Trade<Bond> trade(bond, tradeID, price, book, quantity, side);

	bondTradeBookingService->BookTrade(trade);
	return;
}

#endif