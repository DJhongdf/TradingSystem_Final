#pragma once

#ifndef BONDPOSITIONSERVICE_HPP
#define BONDPOSITIONSERVICE_HPP

#include "BondTradeBookingService.hpp"
#include "positionservice.hpp"
#include "soa.hpp"

using namespace std;

class BondPositionService : public PositionService<Bond> {
private:
	map<string, Position<Bond> >positionMap;
	vector<ServiceListener<Position<Bond> >*> listeners;
public:
	// constructor
	BondPositionService();

	// override base class
	Position<Bond>& GetData(string id) override;

	void OnMessage(Position<Bond>& position) override;

	void AddListener(ServiceListener<Position<Bond> >* listener) override;

	const vector<ServiceListener<Position<Bond> >*>& GetListeners() const override;

	void AddTrade(const Trade<Bond>& trade) override;
};

BondPositionService::BondPositionService()
{
	positionMap = map<string, Position<Bond> >();
}

Position<Bond>& BondPositionService::GetData(string id)
{
	return positionMap.at(id);
}

void BondPositionService::OnMessage(Position<Bond>& position)
{
	return;
}

void BondPositionService::AddListener(ServiceListener<Position<Bond> >* listener)
{
	listeners.push_back(listener);
	return;
}

const vector<ServiceListener<Position<Bond> >*>& BondPositionService::GetListeners() const {
	return listeners;
}

void BondPositionService::AddTrade(const Trade<Bond>& trade)
{
	long quantity = trade.GetQuantity();
	if (trade.GetSide() == SELL)quantity = -quantity;
	auto bond = trade.GetProduct();
	auto id = bond.GetProductId();

	auto it = positionMap.find(id);
	if (it != positionMap.end())
	{
		(it->second).AddPosition(trade.GetBook(), quantity);
	}
	else {
		positionMap.insert(pair<string, Position<Bond> >(id, Position<Bond>(bond)));
		AddTrade(trade);
		return;
	}

	for (auto& listener : listeners)
	{
		listener->ProcessAdd(it->second);
	}
	return;
}

//Listener
class BondPositionServiceListener : public ServiceListener<Trade<Bond> >
{
private:
	BondPositionService* bondPositionService;

public:
	// constructor
	BondPositionServiceListener(BondPositionService* _bondPositionService);

	// override base class
	void ProcessAdd(Trade<Bond>& data);

	void ProcessRemove(Trade<Bond>& data) override {}

	void ProcessUpdate(Trade<Bond>& data) override {}
};

BondPositionServiceListener::BondPositionServiceListener(BondPositionService* _bondPositionService)
{
	bondPositionService = _bondPositionService;
}

void BondPositionServiceListener::ProcessAdd(Trade<Bond>& data)
{
	bondPositionService->AddTrade(data);
	return;
}
#endif