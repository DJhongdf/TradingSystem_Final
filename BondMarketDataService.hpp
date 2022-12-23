#pragma once

#ifndef BONDMARKETDATASERVICE_HPP
#define BONDMARKETDATASERVICE_HPP

using namespace std;

#include "soa.hpp"
#include "marketdataservice.hpp"
#include "BondProductService.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>


class BondMarketDataService :public MarketDataService<Bond> {
private:
	map<string, OrderBook<Bond> > orderMap;
	vector<ServiceListener<OrderBook<Bond> >*> listeners;

public:
	// constructor
	BondMarketDataService();

	// override base class
	OrderBook<Bond>& GetData(string id) override;

	void OnMessage(OrderBook<Bond>& orderBook) override;

	void AddListener(ServiceListener<OrderBook<Bond> >* listener) override;

	const vector<ServiceListener<OrderBook<Bond> >*>& GetListeners() const override;

	BidOffer& GetBestBidOffer(const string& productId) override;

	OrderBook<Bond>& AggregateDepth(const string& productId) override;
};

BondMarketDataService::BondMarketDataService()
{
	orderMap = map<string, OrderBook<Bond> >();
}

OrderBook<Bond>& BondMarketDataService::GetData(string id)
{
	return orderMap.at(id);
}

void BondMarketDataService::OnMessage(OrderBook<Bond>& orderBook)
{
	auto id = orderBook.GetProduct().GetProductId();
	if (orderMap.find(id) != orderMap.end())
		orderMap.erase(id);
	orderMap.insert(pair<string, OrderBook<Bond> >(id, orderBook));

	auto bestOrder = GetBestBidOffer(id);
	vector<Order> bid, ask;
	bid.push_back(bestOrder.GetBidOrder());
	ask.push_back(bestOrder.GetOfferOrder());
	OrderBook<Bond> bestBook = OrderBook<Bond>(orderBook.GetProduct(), bid, ask);

	for (auto& listener : listeners)
	{
		listener->ProcessAdd(bestBook);
	}
	return;
}

void BondMarketDataService::AddListener(ServiceListener<OrderBook<Bond> >* listener)
{
	listeners.push_back(listener);
	return;
}

const vector<ServiceListener<OrderBook<Bond> >*>& BondMarketDataService::GetListeners() const {
	return listeners;
}

BidOffer& BondMarketDataService::GetBestBidOffer(const string& productId)
{
	auto orderBook = orderMap.at(productId);
	auto bidStack = orderBook.GetBidStack();
	auto askStack = orderBook.GetOfferStack();
	auto bestBid = bidStack[0];
	auto bestAsk = askStack[0];

	for (auto& item : bidStack)
	{
		if (item.GetPrice() > bestBid.GetPrice())
			bestBid = item;
	}

	for (auto& item : askStack)
	{
		if (item.GetPrice() < bestAsk.GetPrice())
			bestAsk = item;
	}
	BidOffer ans = BidOffer(bestBid, bestAsk);
	return ans;
}

OrderBook<Bond>& BondMarketDataService::AggregateDepth(const string& productId)
{
	auto orderBook = orderMap.at(productId);
	auto bidStack = orderBook.GetBidStack();
	auto askStack = orderBook.GetOfferStack();

	unordered_map<double, long> bidRecord, askRecord;
	for (auto& item : bidStack)
	{
		if (bidRecord.find(item.GetPrice()) != bidRecord.end())
			bidRecord[item.GetPrice()] += item.GetQuantity();
		else
			bidRecord[item.GetPrice()] = item.GetQuantity();
	}
	for (auto& item : askStack)
	{
		if (askRecord.find(item.GetPrice()) != askRecord.end())
			askRecord[item.GetPrice()] += item.GetQuantity();
		else
			askRecord[item.GetPrice()] = item.GetQuantity();
	}

	vector<Order> bidStack_, askStack_;
	for (auto& item : bidRecord)
	{
		bidStack_.push_back(Order(item.first, item.second, BID));
	}
	for (auto& item : askRecord) {
		askStack_.push_back(Order(item.first, item.second, OFFER));
	}

	auto b = orderBook.GetProduct();
	OrderBook<Bond> ans = OrderBook<Bond>(b, bidStack_, askStack_);
	return ans;
}

//Connector
class BondMarketDataServiceConnector : public Connector<OrderBook<Bond> > {
private:
	BondMarketDataService* bondMarketDataService;
	BondProductService* bondProductService;

	float TransformStringToPrice(string& s);
public:
	// constructor
	BondMarketDataServiceConnector(BondMarketDataService* _bondMarketDataService, BondProductService* _bondProductService);

	// override class
	void Publish(OrderBook<Bond>& data) override;

	void Subscribe() override;
};

float BondMarketDataServiceConnector::TransformStringToPrice(string& s)
{
	int n = s.size();
	if (s[n - 1] == '+')s[n - 1] = '4';
	return stoi(s.substr(0, n - 4)) + stoi(s.substr(n - 3, 2)) / 32.0 + stoi(s.substr(n - 1, 1)) / 256.0;
}

BondMarketDataServiceConnector::BondMarketDataServiceConnector(BondMarketDataService* _bondMarketDataService, BondProductService* _bondProductService) {
	bondMarketDataService = _bondMarketDataService;
	bondProductService = _bondProductService;
}

void BondMarketDataServiceConnector::Publish(OrderBook<Bond>& data)
{
	return;
}

void BondMarketDataServiceConnector::Subscribe()
{
	ifstream input;
	input.open("marketdata.txt");
	string row;

	while (getline(input, row))
	{
		stringstream line(row);
		string temp;
		vector<Order> bidStack, askStack;

		getline(line, temp, ',');
		auto bond = bondProductService->GetData(temp);

		for (int i = 0; i < 10; ++i)
		{
			getline(line, temp, ',');
			auto price = TransformStringToPrice(temp);
			getline(line, temp, ',');
			auto quantity = stol(temp);
			getline(line, temp, ',');
			auto side = temp;
			if (i < 5)
			{
				bidStack.push_back(Order(price, quantity, BID));
			}
			else {
				askStack.push_back(Order(price, quantity, OFFER));
			}
		}

		OrderBook<Bond> orderBook(bond, bidStack, askStack);
		bondMarketDataService->OnMessage(orderBook);
	}
	return;
}
#endif