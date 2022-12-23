#pragma once

#ifndef GUISERVICE_HPP
#define GUISERVICE_HPP

#include "boost/date_time/posix_time/posix_time.hpp"
#include <boost/date_time.hpp>
#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
#include "pricingservice.hpp"
#include "soa.hpp"
#include "products.hpp"

using namespace boost::posix_time;
using namespace std;

//PriceWithTimeStamp
class TimeStampPrice : public Price<Bond> {
private:
	ptime timeStamp;
public:
	TimeStampPrice(ptime _timeStamp, Price<Bond> _price)
		:Price<Bond>(_price) {
		timeStamp = _timeStamp;
	}

	ptime GetTimeStamp() {
		return timeStamp;
	}
};


//Connector
class GUIServiceConnector : public Connector<TimeStampPrice > {
public:
	// constructor
	GUIServiceConnector() {};

	void Publish(TimeStampPrice& data) override;

	void Subscribe() override {}
};


void GUIServiceConnector::Publish(TimeStampPrice& data) {
	auto timeStamp = data.GetTimeStamp();
	auto bond = data.GetProduct();
	auto mid = data.GetMid();
	auto spread = data.GetBidOfferSpread();

	ofstream out;
	out.open("gui.txt", ios::app);
	out << timeStamp << "," << bond.GetProductId() << "," << mid << "," << spread << endl;
	return;
}



class GUIService :public PricingService<Bond> {
private:
	GUIServiceConnector* gUIServiceConnector;
	ptime last;
	time_duration throtteTime;
public:
	// constructor
	GUIService(GUIServiceConnector* _gUIServiceConnector);

	void Throttle(Price<Bond>& data);

	// override class
	Price<Bond>& GetData(string id) override {}

	void OnMessage(Price<Bond>& data) override {}

	void AddListener(ServiceListener<Price<Bond> >* listener) override {}

	const vector< ServiceListener<Price<Bond> >* >& GetListeners() const override {}
};

class GUIService;
GUIService::GUIService(GUIServiceConnector* _gUIServiceConnector) {
	gUIServiceConnector = _gUIServiceConnector;
	throtteTime = millisec(3);
	last = microsec_clock::local_time();
}


void GUIService::Throttle(Price<Bond>& data) {
	ptime Now = microsec_clock::local_time();
	time_duration diff = Now - last;

	if (diff < throtteTime) {
		return;
	}
	last = Now;
	auto pts = TimeStampPrice(Now, data);
	gUIServiceConnector->Publish(pts);

	return;
}

//Listener
class GUIServiceListener : public ServiceListener<Price<Bond> > {
private:
	GUIService* gUIService;
public:
	// constructor
	GUIServiceListener(GUIService* _gUIService);

	// override 
	void ProcessAdd(Price<Bond>& price) override;

	void ProcessRemove(Price<Bond>&) override {};

	void ProcessUpdate(Price<Bond>&) override {};
};

GUIServiceListener::GUIServiceListener(GUIService* _gUIService) {
	gUIService = _gUIService;
}

void GUIServiceListener::ProcessAdd(Price<Bond>& price) {
	gUIService->Throttle(price);
	return;
}

#endif