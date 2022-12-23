#pragma once

#ifndef BONDPRODUCTSERVICE_HPP
#define BONDPRODUCTSERVICE_HPP

#include "boost/date_time/gregorian/gregorian.hpp"
#include "products.hpp"
#include "soa.hpp"
#include <iostream>
#include <map>
#include <vector>
#include <string>

using namespace boost::gregorian;
using namespace std;


class BondProductService : public Service<string, Bond> {

public:
	// constructor
	BondProductService();

	Bond& GetData(string productId);

	void Add(Bond& bond);

	vector<Bond> GetBonds(string& _ticker);

	// override 
	void OnMessage(Bond& data) override {};

	void AddListener(ServiceListener<Bond >* listener) override {};

	const vector< ServiceListener<Bond >* >& GetListeners() const override {};


private:
	map<string, Bond> bondMap;

};

BondProductService::BondProductService() {
	bondMap = map<string, Bond>();
}

Bond& BondProductService::GetData(string productId) {
	return bondMap[productId];
}

void BondProductService::Add(Bond& bond) {
	bondMap.insert(pair<string, Bond>(bond.GetProductId(), bond));
}

vector<Bond> BondProductService::GetBonds(string& _ticker) {
	vector<Bond> res;
	for (auto it = bondMap.begin(); it != bondMap.end(); ++it) {
		if ((it->second).GetTicker() == _ticker)
			res.push_back(it->second);
	}
	return res;
}

vector<string> bondCusip = { "91282CFX4", // 2Years
"91282CGA3", // 3Years
"91282CFZ9", // 5Years
"91282CFY2", // 7Years
"91282CFV8", // 10Years
"912810TM0", // 20Years
"912810TL2" }; // 30Years

vector<float> bondCoupon = { 0.04505, // 2Years
0.04093, // 3Years
0.03974, // 5Years
0.03890, // 7Years
0.03625, // 10Years
0.04027, // 20Years
0.03513 }; // 30Years

vector<date> bondMaturity = { date(2024,Dec,30), // 2Years
date(2025,Dec,15), // 3Years
date(2027,Dec,30), // 5Years
date(2029,Dec,30), // 7Years
date(2032,Dec,15), // 10Years
date(2042,Dec,30), // 20Years
date(2052,Dec,15) }; // 30Years

map<string, float> bondRisk = { {"91282CFX4",0.02}, // 2Years
{"91282CGA3",0.03}, // 3Years
{"91282CFZ9",0.05}, // 5Years
{"91282CFY2",0.06}, // 7Years
{"91282CFV8",0.09}, // 10Years
{"912810TM0",0.09}, // 20Years
{"912810TL2",0.2} }; // 30Years

#endif