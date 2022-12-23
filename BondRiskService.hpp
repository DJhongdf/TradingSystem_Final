#pragma once

#ifndef BONDRISKSERVICE_HPP
#define BONDRISKSERVICE_HPP

#include "BondPositionService.hpp"
#include "riskservice.hpp"
#include "soa.hpp"

using namespace std;

class BondRiskService : public RiskService<Bond> {
private:
	map<string, PV01<Bond> > pv01Map;
	vector<ServiceListener<PV01<Bond> >*> listeners;
public:
	// constructor
	BondRiskService();

	// override base class
	PV01<Bond>& GetData(string id) override;

	double GetBucketedRisk(const BucketedSector<Bond>& sector) override;

	void AddListener(ServiceListener<PV01<Bond> >* listener) override;

	void OnMessage(PV01<Bond>& pv01) override { return; }

	void AddPosition(Position<Bond>& position) override;

	const vector<ServiceListener<PV01<Bond> >*>& GetListeners() const override;
};


BondRiskService::BondRiskService()
{
	pv01Map = map<string, PV01<Bond> >();
}

PV01<Bond>& BondRiskService::GetData(string id)
{
	return pv01Map.at(id);
}

double BondRiskService::GetBucketedRisk(const BucketedSector<Bond>& sector) {
	double res = 0;
	for (auto& product : sector.GetProducts()) {
		res += pv01Map.at(product.GetProductId()).GetPV01();
	}
	return res;
}

void BondRiskService::AddListener(ServiceListener<PV01<Bond> >* listener) {
	listeners.push_back(listener);
	return;
}

void BondRiskService::AddPosition(Position<Bond>& position)
{
	auto bond = position.GetProduct();
	auto id = bond.GetProductId();
	auto it = pv01Map.find(id);
	long quantity = position.GetAggregatePosition();
	if (it != pv01Map.end())
	{
		(it->second).AddQuantity(quantity);
	}
	else {
		pv01Map.insert(pair<string, PV01<Bond> >(id, PV01<Bond>(bond, bondRisk[id], quantity)));
	}
	it = pv01Map.find(id);

	for (auto& listener : listeners)
	{
		listener->ProcessAdd(it->second);
	}
	return;
}

const vector<ServiceListener<PV01<Bond> >*>& BondRiskService::GetListeners() const {
	return listeners;
}


//Listener
class BondRiskServiceListener : public ServiceListener<Position<Bond> > {
private:
	BondRiskService* bondRiskService;
public:
	//constructor
	BondRiskServiceListener(BondRiskService* _bondRiskService);

	// override base class
	void ProcessAdd(Position<Bond>& data) override;

	void ProcessRemove(Position<Bond>& data) override {};

	void ProcessUpdate(Position<Bond>& data) override {};
};

void BondRiskServiceListener::ProcessAdd(Position<Bond>& data)
{
	bondRiskService->AddPosition(data);
	return;
}

BondRiskServiceListener::BondRiskServiceListener(BondRiskService* _bondRiskService)
{
	bondRiskService = _bondRiskService;
}

#endif