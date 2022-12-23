#pragma once

#ifndef BONDPRICINGSERVICE_HPP
#define BONDPRICINGSERVICE_HPP

#include "pricingservice.hpp"
#include "products.hpp"
#include "soa.hpp"
#include "BondProductService.hpp"
#include <map>
#include <vector>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>

using namespace std;

class BondPricingService : public PricingService<Bond> {
private:
	map<string, Price<Bond> > priceMap;
	vector<ServiceListener<Price<Bond> >* > listeners;
public:
	// constructor
	BondPricingService();

	// override pure virtual functions in Service class
	Price<Bond>& GetData(string id) override;

	void OnMessage(Price<Bond>& data) override;

	void AddListener(ServiceListener<Price<Bond> >* listener) override;

	const vector< ServiceListener<Price<Bond> >* >& GetListeners() const override;

};

BondPricingService::BondPricingService() {
	priceMap = map<string, Price<Bond> >();
}

Price<Bond>& BondPricingService::GetData(string id) {
	return priceMap.at(id);
}

void BondPricingService::OnMessage(Price<Bond>& data) {
	auto id = data.GetProduct().GetProductId();

	if (priceMap.find(id) != priceMap.end())
		priceMap.erase(id);
	priceMap.insert(pair<string, Price<Bond> >(id, data));

	for (auto& listener : listeners) {
		listener->ProcessAdd(data);
	}
	return;
}

void BondPricingService::AddListener(ServiceListener<Price<Bond> >* listener) {
	listeners.push_back(listener);
	return;
}

const vector< ServiceListener<Price<Bond> >* >& BondPricingService::GetListeners() const {
	return listeners;
}



//connector
class BondPricingServiceConnector : public Connector<Price<Bond> > {
private:
	BondPricingService* bondPricingService;
	BondProductService* bondProductService;

	float TransformStringToPrice(string& s);
public:
	// constructor
	BondPricingServiceConnector(BondPricingService* _bondPricingService, BondProductService* _bondProductService);

	void Subscribe() override;

	void Publish(Price<Bond>& data) override;
};


float BondPricingServiceConnector::TransformStringToPrice(string& s) {
	int n = s.size();
	if (s[n - 1] == '+')s[n - 1] = '4';
	return stoi(s.substr(0, n - 4)) + stoi(s.substr(n - 3, 2)) / 32.0 + stoi(s.substr(n - 1, 1)) / 256.0;
}


BondPricingServiceConnector::BondPricingServiceConnector(BondPricingService* _bondPricingService, BondProductService* _bondProductService) {
	bondPricingService = _bondPricingService;
	bondProductService = _bondProductService;
}


void BondPricingServiceConnector::Subscribe() {
	ifstream input;
	input.open("prices.txt");

	string row;
	while (getline(input, row)) {
		stringstream line(row);
		string temp;
		getline(line, temp, ',');
		auto bond = bondProductService->GetData(temp);
		getline(line, temp, ',');
		auto price = TransformStringToPrice(temp);
		getline(line, temp, ',');
		auto spread = TransformStringToPrice(temp);

		Price<Bond> bondPrice(bond, price, spread);

		bondPricingService->OnMessage(bondPrice);
	}
	return;
}


void BondPricingServiceConnector::Publish(Price<Bond>& data) {
	return;
}
#endif